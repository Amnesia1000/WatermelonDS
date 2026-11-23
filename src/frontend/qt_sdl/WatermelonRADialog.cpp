#include "WatermelonRADialog.h"
#include "WatermelonSettingsDialog.h"
#include "Config.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QScrollArea>
#include <QFrame>
#include <QPainter>
#include <QPainterPath>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDesktopServices>

namespace
{
constexpr const char* RA_API_HOST = "https://retroachievements.org/dorequest.php";
constexpr const char* RA_MEDIA_HOST = "https://media.retroachievements.org/UserPic/";
}

WatermelonRADialog::WatermelonRADialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("RetroAchievements — Resumen de Usuario"));
    resize(520, 480);
    setMinimumSize(480, 420);
    setModal(true);

    networkManager = new QNetworkAccessManager(this);

    setupUI();
    loadUserData();
}

WatermelonRADialog::~WatermelonRADialog() = default;

bool WatermelonRADialog::isEnabled()
{
    Config::Table globalCfg = Config::GetGlobalTable();
    return globalCfg.GetBool("RA.Enabled");
}

bool WatermelonRADialog::isHardcore()
{
    Config::Table globalCfg = Config::GetGlobalTable();
    return globalCfg.GetBool("RA.Hardcore");
}

QString WatermelonRADialog::getUsername()
{
    Config::Table globalCfg = Config::GetGlobalTable();
    return globalCfg.GetQString("RA.Username");
}

QString WatermelonRADialog::getToken()
{
    Config::Table globalCfg = Config::GetGlobalTable();
    return globalCfg.GetQString("RA.Token");
}

void WatermelonRADialog::setupUI()
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(18, 18, 18, 18);
    outerLayout->setSpacing(14);

    // Title Header
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(8);
    auto* titleLabel = new QLabel(QStringLiteral("🏆 <span style='font-size: 18px; font-weight: 800; color: #f6f5f3;'>RetroAchievements</span>"), this);
    titleLabel->setTextFormat(Qt::RichText);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    outerLayout->addLayout(headerLayout);

    // --- Profile Section (When logged in) ---
    profileSection = new QWidget(this);
    auto* profMainLayout = new QVBoxLayout(profileSection);
    profMainLayout->setContentsMargins(0, 0, 0, 0);
    profMainLayout->setSpacing(14);

    // User Info Card
    auto* userCard = new QFrame(profileSection);
    userCard->setStyleSheet(QStringLiteral(
        "QFrame {"
        "  background: #1a1921;"
        "  border: 1px solid #272632;"
        "  border-radius: 10px;"
        "  padding: 14px;"
        "}"
    ));
    auto* userCardLay = new QHBoxLayout(userCard);
    userCardLay->setSpacing(16);

    avatarLabel = new QLabel(userCard);
    avatarLabel->setFixedSize(68, 68);
    avatarLabel->setStyleSheet(QStringLiteral("border-radius: 10px; background: #252430; border: 2px solid #6fbf4a;"));
    avatarLabel->setScaledContents(true);
    userCardLay->addWidget(avatarLabel);

    auto* userDetailsLay = new QVBoxLayout();
    userDetailsLay->setSpacing(4);

    profileNameLabel = new QLabel(userCard);
    profileNameLabel->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: 800; color: #f6f5f3; border: none; background: transparent;"));
    userDetailsLay->addWidget(profileNameLabel);

    profileStatusBadge = new QLabel(tr("🟢 Conectado"), userCard);
    profileStatusBadge->setStyleSheet(QStringLiteral(
        "color: #6fbf4a; background: #1c2b18; border: 1px solid #2d4526; border-radius: 10px;"
        "font-size: 11px; font-weight: 700; padding: 2px 10px; max-width: 90px;"
    ));
    userDetailsLay->addWidget(profileStatusBadge);

    userCardLay->addLayout(userDetailsLay, 1);
    profMainLayout->addWidget(userCard);

    // Stat Cards (3 Columns)
    auto* statsRow = new QHBoxLayout();
    statsRow->setSpacing(10);

    auto makeStatBox = [](const QString& title, QLabel*& lblValue, const QString& color) -> QWidget* {
        auto* box = new QFrame();
        box->setStyleSheet(QStringLiteral(
            "QFrame {"
            "  background: #181720; border: 1px solid #272632; border-radius: 8px; padding: 10px;"
            "}"
        ));
        auto* lay = new QVBoxLayout(box);
        lay->setContentsMargins(6, 6, 6, 6);
        lay->setSpacing(4);
        lay->setAlignment(Qt::AlignCenter);

        lblValue = new QLabel(tr("—"), box);
        lblValue->setAlignment(Qt::AlignCenter);
        lblValue->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 800; color: %1; border: none; background: transparent;").arg(color));
        lay->addWidget(lblValue);

        auto* lblTitle = new QLabel(title, box);
        lblTitle->setAlignment(Qt::AlignCenter);
        lblTitle->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 600; color: #8e8c99; border: none; background: transparent;"));
        lay->addWidget(lblTitle);

        return box;
    };

    statsRow->addWidget(makeStatBox(tr("Puntos Hardcore"), lblHardcorePoints, QStringLiteral("#ffd700")));
    statsRow->addWidget(makeStatBox(tr("Puntos Softcore"), lblSoftcorePoints, QStringLiteral("#6fbf4a")));
    statsRow->addWidget(makeStatBox(tr("Modo de Juego"), lblGameMode, QStringLiteral("#f6f5f3")));

    profMainLayout->addLayout(statsRow);

    // Action Buttons
    auto* actionsCard = new QFrame(profileSection);
    actionsCard->setStyleSheet(QStringLiteral(
        "QFrame {"
        "  background: #1a1921; border: 1px solid #272632; border-radius: 8px; padding: 8px;"
        "}"
    ));
    auto* actLay = new QVBoxLayout(actionsCard);
    actLay->setSpacing(8);

    btnViewWebProfile = new QPushButton(tr("🌐 Ver Perfil en RetroAchievements.org"), actionsCard);
    btnViewWebProfile->setCursor(Qt::PointingHandCursor);
    btnViewWebProfile->setFixedHeight(32);
    btnViewWebProfile->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #252430; border: 1px solid #363444; border-radius: 6px;"
        "  color: #f6f5f3; font-weight: 600; font-size: 12px;"
        "}"
        "QPushButton:hover { background: #31303e; border-color: #6fbf4a; color: #6fbf4a; }"
    ));
    connect(btnViewWebProfile, &QPushButton::clicked, this, &WatermelonRADialog::onOpenWebProfileClicked);
    actLay->addWidget(btnViewWebProfile);

    auto* actBtnRow = new QHBoxLayout();
    actBtnRow->setSpacing(8);

    btnOpenSettings = new QPushButton(tr("⚙ Ajustes de Logros..."), actionsCard);
    btnOpenSettings->setCursor(Qt::PointingHandCursor);
    btnOpenSettings->setFixedHeight(30);
    btnOpenSettings->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #22212c; border: 1px solid #32303e; border-radius: 6px;"
        "  color: #f6f5f3; font-weight: 600; font-size: 12px;"
        "}"
        "QPushButton:hover { background: #2d2b3a; border-color: #ff5477; color: #ff5477; }"
    ));
    connect(btnOpenSettings, &QPushButton::clicked, this, &WatermelonRADialog::onOpenSettingsClicked);
    actBtnRow->addWidget(btnOpenSettings);

    btnLogout = new QPushButton(tr("🚪 Cerrar Sesión"), actionsCard);
    btnLogout->setCursor(Qt::PointingHandCursor);
    btnLogout->setFixedHeight(30);
    btnLogout->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #2a181e; border: 1px solid #4a222c; border-radius: 6px;"
        "  color: #ff6584; font-weight: 600; font-size: 12px;"
        "}"
        "QPushButton:hover { background: #3a1e27; border-color: #ff3366; color: #ffffff; }"
    ));
    connect(btnLogout, &QPushButton::clicked, this, &WatermelonRADialog::onLogoutClicked);
    actBtnRow->addWidget(btnLogout);

    actLay->addLayout(actBtnRow);
    profMainLayout->addWidget(actionsCard);

    outerLayout->addWidget(profileSection);

    // --- Login Section (When logged out) ---
    loginSection = new QWidget(this);
    auto* logLayout = new QVBoxLayout(loginSection);
    logLayout->setContentsMargins(0, 0, 0, 0);
    logLayout->setSpacing(12);

    auto* cardLog = new QFrame(loginSection);
    cardLog->setStyleSheet(QStringLiteral(
        "QFrame {"
        "  background: #1a1921; border: 1px solid #272632; border-radius: 10px; padding: 16px;"
        "}"
    ));
    auto* cardLogLay = new QVBoxLayout(cardLog);
    cardLogLay->setSpacing(12);

    auto* logTitle = new QLabel(tr("Iniciar sesión con tu cuenta de RetroAchievements:"), cardLog);
    logTitle->setStyleSheet(QStringLiteral("font-weight: 700; font-size: 13px; color: #f6f5f3; border: none; background: transparent;"));
    cardLogLay->addWidget(logTitle);

    auto* logDesc = new QLabel(tr("Conéctate para desbloquear logros, registrar tu puntuación y competir en clasificaciones globales."), cardLog);
    logDesc->setStyleSheet(QStringLiteral("color: #8e8c99; font-size: 11px; border: none; background: transparent;"));
    logDesc->setWordWrap(true);
    cardLogLay->addWidget(logDesc);

    userInput = new QLineEdit(cardLog);
    userInput->setPlaceholderText(tr("Nombre de usuario"));
    userInput->setFixedHeight(30);
    userInput->setStyleSheet(QStringLiteral("background: #141318; border: 1px solid #2d2c38; border-radius: 6px; color: #f6f5f3; padding: 4px 10px;"));
    cardLogLay->addWidget(userInput);

    passInput = new QLineEdit(cardLog);
    passInput->setPlaceholderText(tr("Contraseña o Clave Web API"));
    passInput->setEchoMode(QLineEdit::Password);
    passInput->setFixedHeight(30);
    passInput->setStyleSheet(QStringLiteral("background: #141318; border: 1px solid #2d2c38; border-radius: 6px; color: #f6f5f3; padding: 4px 10px;"));
    cardLogLay->addWidget(passInput);

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);
    btnLogin = new QPushButton(tr("Iniciar Sesión"), cardLog);
    btnLogin->setCursor(Qt::PointingHandCursor);
    btnLogin->setFixedHeight(32);
    btnLogin->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #e54d5e; border: none; border-radius: 6px;"
        "  color: #ffffff; font-weight: 700; font-size: 13px; padding: 0 16px;"
        "}"
        "QPushButton:hover { background: #ff5c6f; }"
    ));
    connect(btnLogin, &QPushButton::clicked, this, &WatermelonRADialog::onLoginClicked);
    btnRow->addWidget(btnLogin);

    auto* btnRegister = new QPushButton(tr("Crear Cuenta..."), cardLog);
    btnRegister->setCursor(Qt::PointingHandCursor);
    btnRegister->setFixedHeight(32);
    btnRegister->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #252430; border: 1px solid #363444; border-radius: 6px;"
        "  color: #f6f5f3; font-weight: 600; font-size: 12px; padding: 0 14px;"
        "}"
        "QPushButton:hover { background: #31303e; border-color: #6fbf4a; color: #6fbf4a; }"
    ));
    connect(btnRegister, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://retroachievements.org/createaccount.php")));
    });
    btnRow->addWidget(btnRegister);
    btnRow->addStretch();
    cardLogLay->addLayout(btnRow);

    loginErrorLabel = new QLabel(cardLog);
    loginErrorLabel->setStyleSheet(QStringLiteral("color: #f44336; font-size: 11px; border: none; background: transparent;"));
    loginErrorLabel->setVisible(false);
    cardLogLay->addWidget(loginErrorLabel);

    loginProgress = new QProgressBar(cardLog);
    loginProgress->setRange(0, 0);
    loginProgress->setFixedHeight(4);
    loginProgress->setVisible(false);
    cardLogLay->addWidget(loginProgress);

    logLayout->addWidget(cardLog);
    outerLayout->addWidget(loginSection);

    outerLayout->addStretch();

    // Bottom Close Button
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    auto* btnClose = new QPushButton(tr("Cerrar"), this);
    btnClose->setCursor(Qt::PointingHandCursor);
    btnClose->setFixedSize(88, 30);
    btnClose->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #24222c; border: 1px solid #32303c; border-radius: 6px;"
        "  color: #f6f5f3; font-weight: 600; font-size: 12px;"
        "}"
        "QPushButton:hover { background: #2f2d3a; border-color: #403e4d; }"
    ));
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(btnClose);
    outerLayout->addLayout(bottomLayout);
}

void WatermelonRADialog::loadUserData()
{
    QString username = getUsername();
    QString token = getToken();

    isLoggedIn = !username.isEmpty() && !token.isEmpty();
    updateProfileView();

    if (isLoggedIn)
    {
        verifyToken(username, token);
    }
}

void WatermelonRADialog::verifyToken(const QString& username, const QString& token)
{
    QUrl url(QString::fromUtf8(RA_API_HOST));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("r"), QStringLiteral("login"));
    query.addQueryItem(QStringLiteral("u"), username);
    query.addQueryItem(QStringLiteral("t"), token);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("melonDualDS-android/0.7.0"));

    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError)
        {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject())
            {
                QJsonObject obj = doc.object();
                if (obj.value(QStringLiteral("Success")).toBool())
                {
                    userPoints = obj.value(QStringLiteral("Score")).toInt();
                    userSoftcorePoints = obj.value(QStringLiteral("SoftcoreScore")).toInt();

                    if (lblHardcorePoints) lblHardcorePoints->setText(QString::number(userPoints));
                    if (lblSoftcorePoints) lblSoftcorePoints->setText(QString::number(userSoftcorePoints));
                }
            }
        }
    });
}

void WatermelonRADialog::updateProfileView()
{
    loginSection->setVisible(!isLoggedIn);
    profileSection->setVisible(isLoggedIn);

    if (isLoggedIn)
    {
        QString user = getUsername();
        profileNameLabel->setText(user);

        if (lblHardcorePoints) lblHardcorePoints->setText(QString::number(userPoints));
        if (lblSoftcorePoints) lblSoftcorePoints->setText(QString::number(userSoftcorePoints));
        if (lblGameMode) lblGameMode->setText(isHardcore() ? tr("Hardcore 🛡") : tr("Estándar"));

        // Fetch UserPic
        QUrl avatarUrl(QStringLiteral("%1%2.png").arg(QString::fromUtf8(RA_MEDIA_HOST), user));
        QNetworkRequest req(avatarUrl);
        QNetworkReply* reply = networkManager->get(req);
        connect(reply, &QNetworkReply::finished, this, &WatermelonRADialog::onUserPicDownloaded);
    }
}

void WatermelonRADialog::onOpenWebProfileClicked()
{
    QString user = getUsername();
    if (!user.isEmpty()) {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://retroachievements.org/user/%1").arg(user)));
    }
}

void WatermelonRADialog::onOpenSettingsClicked()
{
    WatermelonSettingsDialog dlg(this, 6); // Category 6 = RetroAchievements
    dlg.exec();
    loadUserData();
}

void WatermelonRADialog::onLoginClicked()
{
    QString user = userInput->text().trimmed();
    QString pass = passInput->text();

    if (user.isEmpty() || pass.isEmpty())
    {
        loginErrorLabel->setText(tr("Por favor introduce usuario y contraseña."));
        loginErrorLabel->setVisible(true);
        return;
    }

    loginErrorLabel->setVisible(false);
    loginProgress->setVisible(true);
    btnLogin->setEnabled(false);

    testAuthentication(user, pass);
}

void WatermelonRADialog::testAuthentication(const QString& username, const QString& password)
{
    QUrl url(QString::fromUtf8(RA_API_HOST));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("r"), QStringLiteral("login"));
    query.addQueryItem(QStringLiteral("u"), username);
    query.addQueryItem(QStringLiteral("p"), password);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("melonDualDS-android/0.7.0"));

    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &WatermelonRADialog::onLoginReplyFinished);
}

void WatermelonRADialog::onLoginReplyFinished()
{
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    reply->deleteLater();
    loginProgress->setVisible(false);
    btnLogin->setEnabled(true);

    if (reply->error() != QNetworkReply::NoError)
    {
        loginErrorLabel->setText(tr("Error de red: ") + reply->errorString());
        loginErrorLabel->setVisible(true);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject())
    {
        loginErrorLabel->setText(tr("Respuesta no válida del servidor."));
        loginErrorLabel->setVisible(true);
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.value(QStringLiteral("Success")).toBool())
    {
        QString errorMsg = obj.value(QStringLiteral("Error")).toString();
        loginErrorLabel->setText(errorMsg.isEmpty() ? tr("Credenciales inválidas.") : errorMsg);
        loginErrorLabel->setVisible(true);
        return;
    }

    QString user = obj.value(QStringLiteral("User")).toString();
    QString token = obj.value(QStringLiteral("Token")).toString();
    userPoints = obj.value(QStringLiteral("Score")).toInt();
    userSoftcorePoints = obj.value(QStringLiteral("SoftcoreScore")).toInt();

    Config::Table globalCfg = Config::GetGlobalTable();
    globalCfg.SetQString("RA.Username", user);
    globalCfg.SetQString("RA.Token", token);
    globalCfg.SetBool("RA.Enabled", true);
    Config::Save();

    isLoggedIn = true;
    passInput->clear();
    updateProfileView();
    emit loginStateChanged(true);
}

void WatermelonRADialog::onUserPicDownloaded()
{
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError)
    {
        QPixmap pixmap;
        if (pixmap.loadFromData(reply->readAll()))
        {
            avatarLabel->setPixmap(pixmap);
        }
    }
}

void WatermelonRADialog::onLogoutClicked()
{
    Config::Table globalCfg = Config::GetGlobalTable();
    globalCfg.SetQString("RA.Username", "");
    globalCfg.SetQString("RA.Token", "");
    Config::Save();
    isLoggedIn = false;
    userPoints = 0;
    userSoftcorePoints = 0;
    avatarLabel->clear();
    updateProfileView();
    emit loginStateChanged(false);
}
