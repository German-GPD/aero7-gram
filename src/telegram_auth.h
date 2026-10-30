#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <functional>

#ifdef AERO7_WITH_TDLIB
struct TdJsonClient;
#endif

class TelegramAuth final : public QObject
{
public:
    explicit TelegramAuth(QObject *parent = nullptr);
    ~TelegramAuth() override;

    bool isAvailable() const;
    void start();
    void logOut();

    std::function<void(const QString &)> statusChanged;
    std::function<void(const QString &)> authorized;
    std::function<void(const QString &)> errorOccurred;

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