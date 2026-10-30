#include "telegram_auth.h"

#include <QCoreApplication>
#include <QDir>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QDesktopServices>
#include <QByteArray>

#ifdef AERO7_WITH_TDLIB
#include <td/telegram/td_json_client.h>
#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#include <bcrypt.h>
#endif
#endif

TelegramAuth::TelegramAuth(QObject *parent) : QObject(parent)
{
#ifdef AERO7_WITH_TDLIB
    connect(&m_pollTimer, &QTimer::timeout, this, [this]() { poll(); });
    m_pollTimer.setInterval(35);
#endif
}

TelegramAuth::~TelegramAuth()
{
#ifdef AERO7_WITH_TDLIB
    m_pollTimer.stop();
    if (m_client) {
        td_json_client_destroy(m_client);
        m_client = nullptr;
    }
#endif
}

bool TelegramAuth::isAvailable() const
{
#ifdef AERO7_WITH_TDLIB
    return true;
#else
    return false;
#endif
}

void TelegramAuth::start()
{
#ifndef AERO7_WITH_TDLIB
    if (errorOccurred)
        errorOccurred("This build was compiled without TDLib. Install a Windows 7-compatible TDLib build and rebuild with AERO7_ENABLE_TDLIB=ON.");
#else
    if (m_client) {
        if (statusChanged) statusChanged("Telegram authorization is already running.");
        return;
    }
    if (!loadCredentials())
        return;

    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_databasePath = QDir(appData).filePath("Telegram/database");
    m_filesPath = QDir(appData).filePath("Telegram/files");
    QDir().mkpath(m_databasePath);
    QDir().mkpath(m_filesPath);

    m_client = td_json_client_create();
    if (!m_client) {
        if (errorOccurred) errorOccurred("TDLib could not create its client instance.");
        return;
    }
    m_pollTimer.start();
    if (statusChanged) statusChanged("Starting secure Telegram connection…");
    send("{\"@type\":\"getAuthorizationState\"}");
#endif
}

void TelegramAuth::logOut()
{
#ifdef AERO7_WITH_TDLIB
    if (m_client)
        send("{\"@type\":\"logOut\"}");
#endif
}

#ifdef AERO7_WITH_TDLIB
bool TelegramAuth::loadCredentials()
{
    QSettings settings;
    m_apiId = settings.value("telegram/api_id").toString();
    m_apiHash = settings.value("telegram/api_hash").toString();

    if (m_apiId.isEmpty() || m_apiHash.isEmpty()) {
        bool ok = false;
        const QString id = QInputDialog::getText(nullptr, "Telegram API credentials",
            "Enter your Telegram API ID from my.telegram.org:", QLineEdit::Normal, QString(), &ok);
        if (!ok) return false;
        bool numeric = false;
        id.toInt(&numeric);
        if (!numeric || id.trimmed().isEmpty()) {
            if (errorOccurred) errorOccurred("The API ID must be a number.");
            return false;
        }
        const QString hash = QInputDialog::getText(nullptr, "Telegram API credentials",
            "Enter your Telegram API hash:", QLineEdit::Normal, QString(), &ok);
        if (!ok) return false;
        if (hash.trimmed().size() < 16) {
            if (errorOccurred) errorOccurred("The API hash looks incomplete.");
            return false;
        }
        m_apiId = id.trimmed();
        m_apiHash = hash.trimmed();
        settings.setValue("telegram/api_id", m_apiId);
        settings.setValue("telegram/api_hash", m_apiHash);
    }
    return true;
}

QByteArray TelegramAuth::loadDatabaseKey()
{
#ifdef Q_OS_WIN
    QSettings settings;
    const QByteArray protectedValue = QByteArray::fromBase64(
        settings.value("telegram/protected_database_key").toByteArray());
    if (!protectedValue.isEmpty()) {
        DATA_BLOB input;
        input.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(protectedValue.constData()));
        input.cbData = static_cast<DWORD>(protectedValue.size());
        DATA_BLOB output = {};
        if (CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, 0, &output)) {
            QByteArray key(reinterpret_cast<const char *>(output.pbData), static_cast<int>(output.cbData));
            SecureZeroMemory(output.pbData, output.cbData);
            LocalFree(output.pbData);
            return key;
        }
    }

    QByteArray key(32, '\0');
    if (BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(key.data()),
                        static_cast<ULONG>(key.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
        return QByteArray();

    DATA_BLOB input;
    input.pbData = reinterpret_cast<BYTE *>(key.data());
    input.cbData = static_cast<DWORD>(key.size());
    DATA_BLOB output = {};
    if (!CryptProtectData(&input, L"AERO7 GRAM TDLib database key", nullptr,
                          nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        SecureZeroMemory(key.data(), static_cast<size_t>(key.size()));
        return QByteArray();
    }
    const QByteArray encoded(reinterpret_cast<const char *>(output.pbData),
                             static_cast<int>(output.cbData));
    settings.setValue("telegram/protected_database_key", encoded.toBase64());
    SecureZeroMemory(output.pbData, output.cbData);
    LocalFree(output.pbData);
    return key;
#else
    return QByteArray();
#endif
}

void TelegramAuth::send(const QString &json)
{
    if (!m_client) return;
    const QByteArray bytes = json.toUtf8();
    td_json_client_send(m_client, bytes.constData());
}

void TelegramAuth::poll()
{
    if (!m_client) return;
    const char *result = td_json_client_receive(m_client, 0.0);
    if (result)
        handleUpdate(QString::fromUtf8(result));
}

void TelegramAuth::handleUpdate(const QString &json)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return;
    const QJsonObject object = document.object();
    const QString type = object.value("@type").toString();

    if (type == "updateAuthorizationState") {
        handleAuthorizationState(object.value("authorization_state").toObject());
    } else if (type == "error") {
        const QString message = object.value("message").toString("Unknown TDLib error");
        if (errorOccurred) errorOccurred(message);
    } else if (type == "updateConnectionState") {
        const QString state = object.value("state").toObject().value("@type").toString();
        if (statusChanged) statusChanged(state == "connectionStateReady"
            ? "Connected to Telegram servers" : "Connecting to Telegram servers…");
    }
}

void TelegramAuth::handleAuthorizationState(const QJsonObject &state)
{
    const QString type = state.value("@type").toString();
    if (type == "authorizationStateWaitTdlibParameters") {
        const QByteArray key = loadDatabaseKey();
        if (key.isEmpty()) {
            if (errorOccurred) errorOccurred("Could not create or decrypt the local database key.");
            return;
        }
        QJsonObject parameters;
        parameters["@type"] = "setTdlibParameters";
        parameters["database_directory"] = m_databasePath;
        parameters["files_directory"] = m_filesPath;
        parameters["use_file_database"] = true;
        parameters["use_chat_info_database"] = true;
        parameters["use_message_database"] = true;
        parameters["use_secret_chats"] = false;
        parameters["api_id"] = m_apiId.toInt();
        parameters["api_hash"] = m_apiHash;
        parameters["system_language_code"] = QLocale::system().name().left(2);
        parameters["device_model"] = "AERO7 GRAM";
        parameters["system_version"] = "Windows 7 SP1";
        parameters["application_version"] = "0.2.0";
        parameters["enable_storage_optimizer"] = true;
        parameters["database_encryption_key"] = QString::fromLatin1(key.toBase64());
        send(QString::fromUtf8(QJsonDocument(parameters).toJson(QJsonDocument::Compact)));
        if (statusChanged) statusChanged("Preparing encrypted local Telegram storage…");
    } else if (type == "authorizationStateWaitPhoneNumber") {
        askPhoneNumber();
    } else if (type == "authorizationStateWaitCode") {
        askCode();
    } else if (type == "authorizationStateWaitPassword") {
        askPassword();
    } else if (type == "authorizationStateWaitRegistration") {
        askRegistration();
    } else if (type == "authorizationStateWaitEmailAddress") {
        askEmail();
    } else if (type == "authorizationStateWaitEmailCode") {
        askEmailCode();
    } else if (type == "authorizationStateWaitOtherDeviceConfirmation") {
        const QString link = state.value("link").toString();
        if (!link.isEmpty()) QDesktopServices::openUrl(QUrl(link));
        if (statusChanged) statusChanged("Confirm this login on your other Telegram device.");
    } else if (type == "authorizationStateReady") {
        m_authorized = true;
        if (statusChanged) statusChanged("Telegram account connected");
        send("{\"@type\":\"getMe\"}");
        if (authorized) authorized("Telegram authorization completed.");
    } else if (type == "authorizationStateLoggingOut") {
        if (statusChanged) statusChanged("Signing out…");
    } else if (type == "authorizationStateClosing") {
        if (statusChanged) statusChanged("Closing Telegram session…");
    } else if (type == "authorizationStateClosed") {
        m_pollTimer.stop();
        if (m_client) {
            td_json_client_destroy(m_client);
            m_client = nullptr;
        }
        m_authorized = false;
        if (statusChanged) statusChanged("Not connected");
    }
}

void TelegramAuth::askPhoneNumber()
{
    bool ok = false;
    const QString phone = QInputDialog::getText(nullptr, "Sign in to Telegram",
        "Phone number (international format):", QLineEdit::Normal, "+", &ok);
    if (!ok) return;
    if (phone.trimmed().isEmpty()) return;
    QJsonObject request;
    request["@type"] = "setAuthenticationPhoneNumber";
    request["phone_number"] = phone.trimmed();
    send(QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact)));
    if (statusChanged) statusChanged("Requesting Telegram login code…");
}

void TelegramAuth::askCode()
{
    bool ok = false;
    const QString code = QInputDialog::getText(nullptr, "Telegram login code",
        "Enter the code sent by Telegram:", QLineEdit::Normal, QString(), &ok);
    if (!ok) return;
    QJsonObject request;
    request["@type"] = "checkAuthenticationCode";
    request["code"] = code.trimmed();
    send(QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact)));
}

void TelegramAuth::askPassword()
{
    bool ok = false;
    const QString password = QInputDialog::getText(nullptr, "Two-step verification",
        "Enter your Telegram cloud password:", QLineEdit::Password, QString(), &ok);
    if (!ok) return;
    QJsonObject request;
    request["@type"] = "checkAuthenticationPassword";
    request["password"] = password;
    send(QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact)));
}

void TelegramAuth::askRegistration()
{
    bool ok = false;
    const QString first = QInputDialog::getText(nullptr, "Create Telegram profile",
        "First name:", QLineEdit::Normal, QString(), &ok);
    if (!ok || first.trimmed().isEmpty()) return;
    const QString last = QInputDialog::getText(nullptr, "Create Telegram profile",
        "Last name (optional):", QLineEdit::Normal, QString(), &ok);
    if (!ok) return;
    QJsonObject request;
    request["@type"] = "registerUser";
    request["first_name"] = first.trimmed();
    request["last_name"] = last.trimmed();
    send(QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact)));
}

void TelegramAuth::askEmail()
{
    bool ok = false;
    const QString email = QInputDialog::getText(nullptr, "Telegram email verification",
        "Enter your email address:", QLineEdit::Normal, QString(), &ok);
    if (!ok || email.trimmed().isEmpty()) return;
    QJsonObject request;
    request["@type"] = "setAuthenticationEmailAddress";
    request["email_address"] = email.trimmed();
    send(QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact)));
}

void TelegramAuth::askEmailCode()
{
    bool ok = false;
    const QString code = QInputDialog::getText(nullptr, "Telegram email code",
        "Enter the verification code sent to your email:", QLineEdit::Normal, QString(), &ok);
    if (!ok || code.trimmed().isEmpty()) return;
    QJsonObject request;
    request["@type"] = "checkAuthenticationEmailCode";
    QJsonObject emailCode;
    emailCode["@type"] = "emailAddressAuthenticationCode";
    emailCode["code"] = code.trimmed();
    request["code"] = emailCode;
    send(QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact)));
}
#endif
