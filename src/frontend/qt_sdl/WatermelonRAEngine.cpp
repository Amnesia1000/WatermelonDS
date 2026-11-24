#include "WatermelonRAEngine.h"
#include "Config.h"
#include "NDS.h"
#include "ARM.h"

#include "rc_client.h"
#include "rc_consoles.h"
#include "rc_api_request.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMetaObject>
#include <QUrl>
#include <cstring>

WatermelonRAEngine::WatermelonRAEngine(QObject* parent)
    : QObject(parent)
{
    m_net = new QNetworkAccessManager(this);
    m_client = rc_client_create(&WatermelonRAEngine::readMemoryCallback,
                                &WatermelonRAEngine::serverCallCallback);
    if (m_client)
    {
        rc_client_set_userdata(m_client, this);
        rc_client_set_event_handler(m_client, &WatermelonRAEngine::eventHandlerCallback);
        updateSettings();
    }
}

WatermelonRAEngine::~WatermelonRAEngine()
{
    stopSession();
    if (m_client)
    {
        rc_client_destroy(m_client);
        m_client = nullptr;
    }
}

void WatermelonRAEngine::updateSettings()
{
    if (!m_client) return;
    Config::Table cfg = Config::GetGlobalTable();
    std::lock_guard<std::mutex> lock(m_clientMutex);
    rc_client_set_hardcore_enabled(m_client, cfg.GetBool("RA.Hardcore") ? 1 : 0);
    rc_client_set_unpromoted_enabled(m_client, cfg.GetBool("RA.Unofficial") ? 1 : 0);
    rc_client_set_encore_mode_enabled(m_client, cfg.GetBool("RA.Encore") ? 1 : 0);
}

void WatermelonRAEngine::startSession(const QString& romPath, melonDS::NDS* nds)
{
    if (!m_client || romPath.isEmpty() || !nds) return;

    Config::Table cfg = Config::GetGlobalTable();
    if (!cfg.GetBool("RA.Enabled")) return;

    QString user = cfg.GetQString("RA.Username");
    QString token = cfg.GetQString("RA.Token");
    if (user.isEmpty() || token.isEmpty()) return;

    m_nds.store(nds);
    m_pendingRomPath = romPath;
    updateSettings();

    if (m_loggedInUser == user && rc_client_get_user_info(m_client) != nullptr)
    {
        loadGameInternal(romPath);
        return;
    }

    QByteArray uUtf8 = user.toUtf8();
    QByteArray tUtf8 = token.toUtf8();

    std::lock_guard<std::mutex> lock(m_clientMutex);
    rc_client_begin_login_with_token(m_client, uUtf8.constData(), tUtf8.constData(),
        [](int result, const char* errorMessage, rc_client_t* client, void* userdata) {
            auto* self = static_cast<WatermelonRAEngine*>(userdata);
            if (!self) return;
            if (result == RC_OK)
            {
                const auto* info = rc_client_get_user_info(client);
                if (info && info->username)
                    self->m_loggedInUser = QString::fromUtf8(info->username);
                self->loadGameInternal(self->m_pendingRomPath);
            }
            else if (errorMessage)
            {
                emit self->osdNotification(
                    QStringLiteral("RetroAchievements: %1").arg(QString::fromUtf8(errorMessage)),
                    0xFFFF5555);
            }
        }, this);
}

void WatermelonRAEngine::loadGameInternal(const QString& romPath)
{
    if (!m_client || romPath.isEmpty()) return;

    QByteArray pathUtf8 = romPath.toUtf8();
    rc_client_begin_identify_and_load_game(m_client, RC_CONSOLE_NINTENDO_DS,
        pathUtf8.constData(), nullptr, 0,
        [](int result, const char* errorMessage, rc_client_t* client, void* userdata) {
            auto* self = static_cast<WatermelonRAEngine*>(userdata);
            if (!self) return;
            if (result == RC_OK)
            {
                self->m_sessionActive.store(true);
                const auto* game = rc_client_get_game_info(client);
                if (game && game->title && rc_client_has_achievements(client))
                {
                    emit self->osdNotification(
                        QStringLiteral("🏆 RetroAchievements: %1").arg(QString::fromUtf8(game->title)),
                        0xFF6FBF4A);
                }
            }
            else if (result != RC_NO_GAME_LOADED && errorMessage)
            {
                emit self->osdNotification(
                    QStringLiteral("RA: %1").arg(QString::fromUtf8(errorMessage)),
                    0xFFFFAA00);
            }
        }, this);
}

void WatermelonRAEngine::stopSession()
{
    m_sessionActive.store(false);
    m_nds.store(nullptr);
    if (m_client)
    {
        std::lock_guard<std::mutex> lock(m_clientMutex);
        rc_client_unload_game(m_client);
    }
}

void WatermelonRAEngine::resetSession()
{
    if (m_client && m_sessionActive.load())
    {
        std::lock_guard<std::mutex> lock(m_clientMutex);
        rc_client_reset(m_client);
    }
}

void WatermelonRAEngine::doFrame()
{
    if (m_client && m_sessionActive.load())
    {
        std::lock_guard<std::mutex> lock(m_clientMutex);
        rc_client_do_frame(m_client);
    }
}

void WatermelonRAEngine::idle()
{
    if (m_client && m_sessionActive.load())
    {
        std::lock_guard<std::mutex> lock(m_clientMutex);
        rc_client_idle(m_client);
    }
}

uint32_t WatermelonRAEngine::readMemoryCallback(uint32_t address, uint8_t* buffer, uint32_t numBytes, rc_client_t* client)
{
    auto* self = static_cast<WatermelonRAEngine*>(rc_client_get_userdata(client));
    if (!self || !buffer || numBytes == 0) return 0;

    melonDS::NDS* nds = self->m_nds.load();
    if (!nds) return 0;

    if (address < 0x1000000U)
    {
        if (!nds->MainRAM) return 0;
        uint32_t ramSize = nds->MainRAMMask + 1;
        if (address >= ramSize) return 0;
        uint32_t avail = ramSize - address;
        uint32_t toRead = numBytes < avail ? numBytes : avail;
        std::memcpy(buffer, nds->MainRAM + address, toRead);
        return toRead;
    }
    else if (address >= 0x1000000U && address < 0x1004000U)
    {
        if (!nds->ARM9.DTCM) return 0;
        uint32_t offset = address - 0x1000000U;
        uint32_t avail = 0x4000U - offset;
        uint32_t toRead = numBytes < avail ? numBytes : avail;
        std::memcpy(buffer, nds->ARM9.DTCM + offset, toRead);
        return toRead;
    }

    return 0;
}

void WatermelonRAEngine::serverCallCallback(const rc_api_request_t* request,
                                              void (*callback)(const struct rc_api_server_response_t*, void*),
                                              void* callbackData,
                                              rc_client_t* client)
{
    auto* self = static_cast<WatermelonRAEngine*>(rc_client_get_userdata(client));
    if (!self || !request || !request->url)
    {
        rc_api_server_response_t err{};
        err.http_status_code = 0;
        if (callback) callback(&err, callbackData);
        return;
    }

    QString url = QString::fromUtf8(request->url);
    QByteArray postData = request->post_data ? QByteArray(request->post_data) : QByteArray();
    QString contentType = request->content_type ? QString::fromUtf8(request->content_type)
                                                : QStringLiteral("application/x-www-form-urlencoded");

    QMetaObject::invokeMethod(self, [self, url, postData, contentType, callback, callbackData]() {
        self->performHttpRequest(url, postData, contentType, callback, callbackData);
    }, Qt::QueuedConnection);
}

void WatermelonRAEngine::performHttpRequest(const QString& url, const QByteArray& postData, const QString& contentType,
                                              void (*callback)(const struct rc_api_server_response_t*, void*),
                                              void* callbackData)
{
    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("melonDS/1.1"));
    if (!postData.isEmpty())
        req.setHeader(QNetworkRequest::ContentTypeHeader, contentType);

    QNetworkReply* reply = postData.isEmpty() ? m_net->get(req) : m_net->post(req, postData);
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, callbackData]() {
        QByteArray body = reply->readAll();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();

        rc_api_server_response_t resp{};
        resp.body = body.constData();
        resp.body_length = static_cast<size_t>(body.size());
        resp.http_status_code = status > 0 ? status : RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR;

        std::lock_guard<std::mutex> lock(m_clientMutex);
        if (callback)
            callback(&resp, callbackData);
    });
}

void WatermelonRAEngine::eventHandlerCallback(const rc_client_event_t* event, rc_client_t* client)
{
    auto* self = static_cast<WatermelonRAEngine*>(rc_client_get_userdata(client));
    if (!self || !event) return;

    switch (event->type)
    {
        case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
            if (event->achievement)
            {
                QString title = QString::fromUtf8(event->achievement->title);
                uint32_t pts = event->achievement->points;
                emit self->osdNotification(
                    QStringLiteral("🏆 Logro desbloqueado: %1 (%2 pts)").arg(title).arg(pts),
                    0xFFF5B041);
            }
            break;

        case RC_CLIENT_EVENT_GAME_COMPLETED:
            if (const auto* game = rc_client_get_game_info(client))
            {
                QString title = game->title ? QString::fromUtf8(game->title) : QStringLiteral("Juego");
                emit self->osdNotification(
                    QStringLiteral("👑 ¡Juego completado al 100%%! %1").arg(title),
                    0xFFF5B041);
            }
            break;

        case RC_CLIENT_EVENT_SERVER_ERROR:
            if (event->server_error && event->server_error->error_message)
            {
                emit self->osdNotification(
                    QStringLiteral("RA: %1").arg(QString::fromUtf8(event->server_error->error_message)),
                    0xFFFFAA00);
            }
            break;

        default:
            break;
    }
}
