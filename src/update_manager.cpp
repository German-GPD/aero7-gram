#include "update_manager.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressDialog>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>
#include <QVersionNumber>
#include <QDir>
#include <QRegularExpression>

namespace {
const qint64 kMaximumUpdateSize = 256LL * 1024LL * 1024LL;
}

UpdateManager::UpdateManager(QWidget *owner)
    : QObject(owner), m_owner(owner)
{
    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, [this]() {
        if (!m_reply) return;
        m_requestTimedOut = true;
        m_reply->abort();
    });
}

void UpdateManager::checkForUpdates(const QString &currentVersion, const QString &architecture)
{
    if (m_reply) {
        QMessageBox::information(m_owner, "AERO7 GRAM", "An update check is already running.");
        return;
    }

    const QUrl apiUrl(QStringLiteral("https://api.github.com/repos/German-GPD/aero7-gram/releases/tags/test-latest"));
    QNetworkRequest request(apiUrl);
    request.setRawHeader("User-Agent", "AERO7-GRAM-Updater");
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_requestTimedOut = false;
    m_reply = m_network.get(request);
    m_timeout.start(30000);
    connect(m_reply, &QNetworkReply::finished, this, [this, currentVersion, architecture]() {
        m_timeout.stop();
        const bool timedOut = m_requestTimedOut;
        m_requestTimedOut = false;
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        if (!reply) return;

        const QByteArray payload = reply->readAll();
        const QNetworkReply::NetworkError networkError = reply->error();
        const QString errorText = reply->errorString();
        reply->deleteLater();

        if (networkError != QNetworkReply::NoError) {
            fail(timedOut ? "The update check timed out. Please try again." :
                QString("Could not check for updates: %1").arg(errorText));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            fail("GitHub returned an invalid update description.");
            return;
        }

        const QJsonObject release = document.object();
        const QString body = release.value("body").toString();
        const QRegularExpression versionPattern(QStringLiteral("Version\\s*:\\s*([0-9]+\\.[0-9]+\\.[0-9]+)"));
        const QRegularExpressionMatch match = versionPattern.match(body);
        if (!match.hasMatch()) {
            fail("The published release does not contain a valid version number.");
            return;
        }

        const QString latestVersion = match.captured(1);
        const QVersionNumber current = QVersionNumber::fromString(currentVersion);
        const QVersionNumber latest = QVersionNumber::fromString(latestVersion);
        if (current.isNull() || latest.isNull()) {
            fail("Could not compare the installed and published versions.");
            return;
        }
        if (QVersionNumber::compare(latest, current) <= 0) {
            QMessageBox::information(m_owner, "AERO7 GRAM",
                QString("You are already using the latest version (%1).").arg(currentVersion));
            return;
        }

        const QString assetName = QString("AERO7-GRAM-%1-Setup.exe").arg(architecture);
        const QJsonArray assets = release.value("assets").toArray();
        for (const QJsonValue &value : assets) {
            const QJsonObject asset = value.toObject();
            if (asset.value("name").toString() != assetName) continue;

            const QUrl downloadUrl(asset.value("browser_download_url").toString());
            const QString digest = asset.value("digest").toString();
            if (downloadUrl.scheme() != "https" || downloadUrl.host() != "github.com"
                || !QRegularExpression(QStringLiteral("^sha256:[0-9a-fA-F]{64}$")).match(digest).hasMatch()) {
                fail("The update package is missing a trusted HTTPS URL or SHA-256 digest.");
                return;
            }

            const QMessageBox::StandardButton choice = QMessageBox::question(
                m_owner, "AERO7 GRAM update available",
                QString("Version %1 is available.\n\nInstalled version: %2\nArchitecture: %3\n\nDownload and install it now? The application will restart.")
                    .arg(latestVersion, currentVersion, architecture),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (choice == QMessageBox::Yes)
                downloadInstaller(downloadUrl, digest, latestVersion);
            return;
        }

        fail(QString("The release does not contain the %1 installer.").arg(architecture));
    });
}

void UpdateManager::downloadInstaller(const QUrl &url, const QString &digest, const QString &version)
{
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (tempDir.isEmpty()) {
        fail("Windows did not provide a temporary folder for the update.");
        return;
    }

    m_downloadPath = QDir(tempDir).filePath(QString("AERO7-GRAM-%1-update.exe").arg(version));
    if (m_downloadFile) {
        delete m_downloadFile;
        m_downloadFile = nullptr;
    }
    m_downloadFile = new QFile(m_downloadPath, this);
    if (!m_downloadFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail("Could not create the update package in the temporary folder.");
        return;
    }

    m_expectedDigest = digest.mid(QStringLiteral("sha256:").size()).toLower();
    m_downloadedBytes = 0;
    m_downloadTooLarge = false;
    m_downloadWriteFailed = false;

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "AERO7-GRAM-Updater");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_requestTimedOut = false;
    m_reply = m_network.get(request);
    m_timeout.start(10 * 60 * 1000);

    m_progress = new QProgressDialog("Downloading AERO7 GRAM update…", "Cancel", 0, 100, m_owner);
    m_progress->setWindowTitle("AERO7 GRAM update");
    m_progress->setWindowModality(Qt::WindowModal);
    m_progress->setMinimumDuration(0);
    m_progress->setValue(0);

    connect(m_progress, &QProgressDialog::canceled, this, [this]() {
        if (m_reply) m_reply->abort();
    });
    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (!m_progress) return;
        if (total > 0) {
            m_progress->setRange(0, 100);
            m_progress->setValue(static_cast<int>((received * 100) / total));
        } else {
            m_progress->setRange(0, 0);
        }
    });
    connect(m_reply, &QIODevice::readyRead, this, [this]() {
        if (!m_reply || !m_downloadFile) return;
        const QByteArray chunk = m_reply->readAll();
        if (m_downloadedBytes + chunk.size() > kMaximumUpdateSize) {
            m_downloadTooLarge = true;
            m_reply->abort();
            return;
        }
        if (m_downloadFile->write(chunk) != chunk.size()) {
            m_downloadWriteFailed = true;
            m_reply->abort();
            return;
        }
        m_downloadedBytes += chunk.size();
    });
    connect(m_reply, &QNetworkReply::finished, this, [this, version]() {
        m_timeout.stop();
        const bool timedOut = m_requestTimedOut;
        m_requestTimedOut = false;
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        if (!reply) return;
        const QNetworkReply::NetworkError networkError = reply->error();
        const QString errorText = reply->errorString();
        if (m_downloadFile && reply->bytesAvailable() > 0) {
            const QByteArray tail = reply->readAll();
            if (m_downloadedBytes + tail.size() > kMaximumUpdateSize) {
                m_downloadTooLarge = true;
            } else if (m_downloadFile->write(tail) == tail.size()) {
                m_downloadedBytes += tail.size();
            } else {
                m_downloadWriteFailed = true;
            }
        }
        reply->deleteLater();
        if (m_progress) {
            m_progress->close();
            m_progress->deleteLater();
            m_progress = nullptr;
        }
        if (m_downloadFile) {
            m_downloadFile->close();
            delete m_downloadFile;
            m_downloadFile = nullptr;
        }

        if (m_downloadTooLarge) {
            m_downloadTooLarge = false;
            QFile::remove(m_downloadPath);
            fail("The update package is larger than the 256 MB safety limit.");
            return;
        }
        if (m_downloadWriteFailed) {
            m_downloadWriteFailed = false;
            QFile::remove(m_downloadPath);
            fail("Windows could not write the update package to the temporary folder. Check free disk space and permissions.");
            return;
        }
        if (networkError != QNetworkReply::NoError) {
            QFile::remove(m_downloadPath);
            if (networkError == QNetworkReply::OperationCanceledError && !timedOut)
                return;
            fail(timedOut ? "The update download timed out. Please try again." :
                QString("The update download failed: %1").arg(errorText));
            return;
        }
        finishDownload(version);
    });
}

void UpdateManager::finishDownload(const QString &version)
{
    QFile file(m_downloadPath);
    if (!file.open(QIODevice::ReadOnly)) {
        fail("The downloaded update package could not be opened.");
        return;
    }

    const QByteArray signature = file.read(2);
    if (signature != "MZ") {
        file.close();
        QFile::remove(m_downloadPath);
        fail("The downloaded file is not a valid Windows installer.");
        return;
    }

    file.seek(0);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd())
        hash.addData(file.read(64 * 1024));
    file.close();

    if (QString::fromLatin1(hash.result().toHex()).toLower() != m_expectedDigest) {
        QFile::remove(m_downloadPath);
        fail("The update package failed its SHA-256 integrity check. Installation was cancelled.");
        return;
    }

    const QMessageBox::StandardButton choice = QMessageBox::question(
        m_owner, "Ready to install",
        QString("AERO7 GRAM %1 has been downloaded and verified.\n\nThe installer will update the existing installation.").arg(version),
        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);
    if (choice != QMessageBox::Ok) {
        QFile::remove(m_downloadPath);
        return;
    }

    const QStringList arguments = QStringList()
        << "/VERYSILENT" << "/SP-" << "/NORESTART"
        << "/CLOSEAPPLICATIONS";
    if (!QProcess::startDetached(m_downloadPath, arguments)) {
        fail("Windows could not start the verified update installer.");
        return;
    }

    QMessageBox::information(m_owner, "AERO7 GRAM update",
        "The installer has started. Approve the Windows prompt if it appears; Setup will close the client, update the existing files and restart it.");
}

void UpdateManager::fail(const QString &message)
{
    m_timeout.stop();
    if (m_progress) {
        m_progress->close();
        m_progress->deleteLater();
        m_progress = nullptr;
    }
    if (m_downloadFile) {
        m_downloadFile->close();
        delete m_downloadFile;
        m_downloadFile = nullptr;
    }
    if (!m_downloadPath.isEmpty())
        QFile::remove(m_downloadPath);
    QMessageBox::warning(m_owner, "AERO7 GRAM updater", message);
}
