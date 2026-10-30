#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <functional>

class QJsonObject;

#ifdef AERO7_WITH_TDLIB
struct TdJsonClient;
#endif

class TelegramAuth final : public QObject
{
public:
    explicit TelegramAuth(QObject *parent = nullptr);
    ~TelegramAuth() override;

    bool isAvailable() const;
    bool isAuthorized() const;
    void start();
    void logOut();
    void requestChats(int limit = 100);
    void requestHistory(qint64 chatId, int limit = 50);
    void sendTextMessage(qint64 chatId, const QString &text);

    std::function<void(const QString &)> statusChanged;
    std::function<void(const QString &)> authorized;
    std::function<void(const QString &)> errorOccurred;
    std::function<void(const QString &)> chatReceived;
    std::function<void(const QString &)> messageReceived;
    std::function<void()> chatsLoaded;
    std::function<void(qint64)> historyLoaded;

private:
#ifdef AERO7_WITH_TDLIB
    void poll();
    void send(const QString &json);
    void handleUpdate(const QString &json);
    void handleAuthorizationState(const QJsonObject &state);
    bool loadCredentials();
    QByteArray loadDatabaseKey();
    void askPhoneNumber();
    void askCode();
    void askPassword();
    void askRegistration();
    void askEmail();
    void askEmailCode();

    void *m_client = nullptr;
    QTimer m_pollTimer;
    QString m_apiId;
    QString m_apiHash;
    QString m_databasePath;
    QString m_filesPath;
    bool m_authorized = false;
#endif
};