#ifndef WATERMELONRAENGINE_H
#define WATERMELONRAENGINE_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <atomic>
#include <mutex>

struct rc_client_t;
struct rc_client_event_t;
struct rc_api_request_t;

namespace melonDS { class NDS; }

class WatermelonRAEngine : public QObject
{
    Q_OBJECT
public:
    explicit WatermelonRAEngine(QObject* parent = nullptr);
    ~WatermelonRAEngine() override;

    void startSession(const QString& romPath, melonDS::NDS* nds);
    void stopSession();
    void resetSession();
    void doFrame();
    void idle();
    void updateSettings();

signals:
    void osdNotification(const QString& message, unsigned int color);

private:
    static uint32_t readMemoryCallback(uint32_t address, uint8_t* buffer, uint32_t numBytes, rc_client_t* client);
    static void serverCallCallback(const rc_api_request_t* request,
                                   void (*callback)(const struct rc_api_server_response_t*, void*),
                                   void* callbackData,
                                   rc_client_t* client);
    static void eventHandlerCallback(const rc_client_event_t* event, rc_client_t* client);

    void performHttpRequest(const QString& url, const QByteArray& postData, const QString& contentType,
                            void (*callback)(const struct rc_api_server_response_t*, void*),
                            void* callbackData);
    void loadGameInternal(const QString& romPath);

    rc_client_t* m_client = nullptr;
    QNetworkAccessManager* m_net = nullptr;
    std::atomic<melonDS::NDS*> m_nds{nullptr};
    std::atomic<bool> m_sessionActive{false};
    QString m_pendingRomPath;
    QString m_loggedInUser;
    std::mutex m_clientMutex;
};

#endif // WATERMELONRAENGINE_H
