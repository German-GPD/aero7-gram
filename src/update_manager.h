#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QString>

class QWidget;
class QUrl;
class QNetworkReply;
class QProgressDialog;
class QFile;

class UpdateManager final : public QObject
{
public:
    explicit UpdateManager(QWidget *owner);

    void checkForUpdates(const QString &currentVersion, const QString &architecture);

private:
    QWidget *m_owner = nullptr;
    QNetworkAccessManager m_network;
    QPointer<QProgressDialog> m_progress;
    QPointer<QNetworkReply> m_reply;
    QFile *m_downloadFile = nullptr;
    QString m_downloadPath;
    QString m_expectedDigest;

    void downloadInstaller(const QUrl &url, const QString &digest, const QString &version);
    void finishDownload(const QString &version);
    void fail(const QString &message);
};
