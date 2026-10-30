#include <QMainWindow>
#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStackedWidget>
#include <QScrollArea>
#include <QShowEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QStyleFactory>
#include <QFont>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QPixmap>
#include <QIcon>
#include <QSvgRenderer>
#include <QSizePolicy>
#include <QPoint>
#include <QSettings>
#include <QMessageBox>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QDateTime>
#include <QTimer>
#include <QDate>
#include <QSet>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include "telegram_auth.h"

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <dwmapi.h>
#  include <windowsx.h>
#endif

namespace {
const char *kAppName = "AERO7 GRAM";
const char *kOrganization = "German-GPD";
const char *kVersion = "0.2.0";

QIcon createTelegramAeroIcon()
{
    QIcon icon;
    QSvgRenderer renderer(QStringLiteral(":/icons/telegram-aero.svg"));
    if (!renderer.isValid())
        return icon;

    const int sizes[] = {16, 20, 24, 32, 40, 48, 64, 96, 128, 256};
    for (int size : sizes) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        renderer.render(&painter, QRectF(0, 0, size, size));
        icon.addPixmap(pixmap);
    }
    return icon;
}

QLabel *makeLabel(const QString &text, const QString &objectName = QString())
{
    QLabel *label = new QLabel(text);
    if (!objectName.isEmpty())
        label->setObjectName(objectName);
    return label;
}

QToolButton *makeNavButton(const QString &glyph, const QString &text, QWidget *parent)
{
    QToolButton *button = new QToolButton(parent);
    button->setText(glyph + "   " + text);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setCursor(Qt::PointingHandCursor);
    button->setCheckable(true);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setFixedHeight(37);
    button->setObjectName("navigationButton");
    button->setProperty("navButton", true);
    return button;
}
}

class DragTitleBar final : public QWidget
{
public:
    explicit DragTitleBar(QWidget *parent = nullptr) : QWidget(parent)
    {
        setAttribute(Qt::WA_StyledBackground, true);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && window() && !window()->isMaximized()) {
            m_dragOffset = event->globalPos() - window()->frameGeometry().topLeft();
            m_dragging = true;
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton) && window()) {
            window()->move(event->globalPos() - m_dragOffset);
            event->accept();
            return;
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        m_dragging = false;
        QWidget::mouseReleaseEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && window()) {
            window()->isMaximized() ? window()->showNormal() : window()->showMaximized();
            event->accept();
            return;
        }
        QWidget::mouseDoubleClickEvent(event);
    }

private:
    QPoint m_dragOffset;
    bool m_dragging = false;
};

class AeroWindow final : public QMainWindow
{
public:
    AeroWindow()
    {
        setWindowTitle(QString::fromLatin1(kAppName));
        setWindowIcon(createTelegramAeroIcon());
        setWindowIconText(QString::fromLatin1(kAppName));
        setMinimumSize(820, 540);
        resize(1120, 720);
        // Keep the real Windows non-client frame: caption buttons, borders,
        // resizing and system menu are drawn by Windows 7 itself.
        setWindowFlags(Qt::Window);
        setAttribute(Qt::WA_TranslucentBackground, false);

        buildInterface();
        applyStyle();
        restorePreferences();

        m_auth = new TelegramAuth(this);
        if (!m_auth->isAvailable()) {
            const QString previewStatus = "Preview build · Telegram engine not bundled";
            if (m_connectionState) m_connectionState->setText(previewStatus);
            if (m_statusText) m_statusText->setText(previewStatus);
            QPushButton *connectButton = findChild<QPushButton *>("connectButton");
            if (connectButton) {
                connectButton->setText("Unavailable");
                connectButton->setEnabled(false);
                connectButton->setToolTip("This test build was compiled without TDLib. Telegram sign-in is unavailable.");
            }
            if (m_messageInput) { m_messageInput->setEnabled(false); m_messageInput->setPlaceholderText("Telegram engine is not included in this build"); }
            if (m_sendButton) { m_sendButton->setEnabled(false); m_sendButton->setToolTip("Sending requires a connected Telegram account."); }
        } else {
            if (m_messageInput) { m_messageInput->setEnabled(false); m_messageInput->setPlaceholderText("Connect Telegram to send messages"); }
            if (m_sendButton) { m_sendButton->setEnabled(false); m_sendButton->setToolTip("Connect your Telegram account first."); }
        }
        m_auth->statusChanged = [this](const QString &status) {
            const QString label = QString::fromUtf8("●  ") + status;
            if (m_connectionState) m_connectionState->setText(label);
            if (m_statusText) m_statusText->setText(label);
        };
        // Begin first-run Telegram sign-in automatically after the UI is ready.
        // TDLib requests API credentials, phone, code and optional 2FA password.
        if (m_auth->isAvailable()) {
            QTimer::singleShot(350, this, [this]() {
                if (m_auth && !m_auth->isAuthorized())
                    m_auth->start();
            });
        }
        m_auth->errorOccurred = [this](const QString &message) {
            if (m_statusText) m_statusText->setText(QString("●  Error: %1").arg(message));
            QMessageBox::warning(this, "Telegram", message);
        };
        m_auth->authorized = [this](const QString &) {
            if (m_connectionState) m_connectionState->setText("●  Telegram account connected");
            if (m_messageInput) { m_messageInput->setEnabled(true); m_messageInput->setPlaceholderText("Message"); }
            if (m_sendButton) m_sendButton->setEnabled(true);
        };
        m_auth->chatReceived = [this](const QString &json) {
            const QJsonObject object = QJsonDocument::fromJson(json.toUtf8()).object();
            QJsonObject chat = object.value("chat").toObject();
            if (chat.isEmpty()) chat = object;
            qint64 id = static_cast<qint64>(chat.value("id").toDouble());
            if (!id) id = static_cast<qint64>(object.value("chat_id").toDouble());
            if (!id || !m_chatList) return;

            // TDLib sends updateChatTitle/updateChatLastMessage as update objects:
            // the title and chat_id are at the top level, not inside "chat".
            QListWidgetItem *item = nullptr;
            for (int i = 0; i < m_chatList->count(); ++i) {
                if (m_chatList->item(i)->data(Qt::UserRole).toLongLong() == id) {
                    item = m_chatList->item(i);
                    break;
                }
            }
            if (!item) {
                item = new QListWidgetItem(m_chatList);
                item->setData(Qt::UserRole, QVariant::fromValue<qlonglong>(id));
                item->setData(Qt::UserRole + 1, "Telegram chat");
                item->setData(Qt::UserRole + 2, QString());
                item->setData(Qt::UserRole + 3, 0);
                item->setData(Qt::UserRole + 4, 0);
            }
            const QString incomingTitle = object.value("title").toString(
                chat.value("title").toString());
            if (!incomingTitle.isEmpty())
                item->setData(Qt::UserRole + 1, incomingTitle);

            const QJsonObject lastMessage = object.value("last_message").toObject();
            if (!lastMessage.isEmpty()) {
                const QJsonObject content = lastMessage.value("content").toObject();
                QString preview = content.value("text").toObject().value("text").toString();
                if (preview.isEmpty() && !content.isEmpty())
                    preview = content.value("@type").toString().mid(13);
                item->setData(Qt::UserRole + 2, preview);
                item->setData(Qt::UserRole + 4, lastMessage.value("date").toInt());
            }
            if (object.contains("unread_count"))
                item->setData(Qt::UserRole + 3, object.value("unread_count").toInt());
            else if (chat.contains("unread_count"))
                item->setData(Qt::UserRole + 3, chat.value("unread_count").toInt());

            const QString title = item->data(Qt::UserRole + 1).toString();
            const QString preview = item->data(Qt::UserRole + 2).toString();
            item->setText(title + "\n" + preview);
            item->setToolTip(title + (preview.isEmpty() ? QString() : "\n" + preview));
            refreshChatItem(item);
        };
        m_auth->historyLoaded = [this](qint64 loadedChatId) {
            if (loadedChatId != m_selectedChatId) return;
            if (m_conversationSubtitle && m_selectedChatId)
                m_conversationSubtitle->setText("Telegram conversation");
            if (m_messageList && m_messageList->count() == 1
                && m_messageList->item(0)->data(Qt::UserRole).toString() == "empty-state")
                showMessagePlaceholder("No messages here yet");
        };
        m_auth->messageReceived = [this](const QString &json) {
            if (!m_messageList) return;
            const QJsonObject message = QJsonDocument::fromJson(json.toUtf8()).object();
            if (static_cast<qint64>(message.value("chat_id").toDouble()) != m_selectedChatId) return;
            const qint64 messageId = static_cast<qint64>(message.value("id").toDouble());
            if (messageId && m_seenMessageIds.contains(messageId)) return;
            if (messageId) m_seenMessageIds.insert(messageId);
            const QJsonObject content = message.value("content").toObject();
            const QString contentType = content.value("@type").toString();
            QString body = content.value("text").toObject().value("text").toString();
            if (body.isEmpty()) {
                if (contentType == "messagePhoto") body = "[Photo]";
                else if (contentType == "messageDocument") body = "[File]";
                else if (contentType == "messageVideo") body = "[Video]";
                else if (contentType == "messageAudio" || contentType == "messageVoiceNote") body = "[Audio]";
                else if (contentType == "messageSticker") body = "[Sticker]";
                else if (contentType == "messageAnimation") body = "[GIF]";
                else if (contentType == "messageLocation" || contentType == "messageVenue") body = "[Location]";
                else if (contentType == "messageContact") body = "[Contact]";
                else if (contentType == "messagePoll") body = "[Poll]";
                else body = QString("[%1]").arg(contentType.isEmpty() ? "Message" : contentType);
            }
            const bool outgoing = message.value("is_outgoing").toBool();
            const QDateTime sentAt = QDateTime::fromTime_t(static_cast<uint>(message.value("date").toInt()));
            if (m_messageList->count() == 1
                && m_messageList->item(0)->data(Qt::UserRole).toString() == "empty-state")
                m_messageList->clear();
            if (sentAt.isValid() && sentAt.date() != m_lastMessageDate) {
                appendDateSeparator(sentAt.date());
                m_lastMessageDate = sentAt.date();
            }
            appendMessageBubble(body, sentAt, outgoing);
            m_messageList->scrollToBottom();
        };
    }

protected:
    void showEvent(QShowEvent *event) override
    {
        QMainWindow::showEvent(event);
#ifdef Q_OS_WIN
        enableAeroGlass();
#endif
    }

    void closeEvent(QCloseEvent *event) override
    {
        QSettings settings;
        settings.setValue("window/geometry", saveGeometry());
        settings.setValue("window/maximized", isMaximized());
        QMainWindow::closeEvent(event);
    }

    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override
    {
#ifdef Q_OS_WIN
        Q_UNUSED(eventType);
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_NCHITTEST && !isMaximized()) {
            const POINT point = { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
            RECT rect = {};
            GetWindowRect(reinterpret_cast<HWND>(winId()), &rect);
            const int border = 7;
            const bool left = point.x < rect.left + border;
            const bool right = point.x >= rect.right - border;
            const bool top = point.y < rect.top + border;
            const bool bottom = point.y >= rect.bottom - border;

            if (top && left) *result = HTTOPLEFT;
            else if (top && right) *result = HTTOPRIGHT;
            else if (bottom && left) *result = HTBOTTOMLEFT;
            else if (bottom && right) *result = HTBOTTOMRIGHT;
            else if (left) *result = HTLEFT;
            else if (right) *result = HTRIGHT;
            else if (top) *result = HTTOP;
            else if (bottom) *result = HTBOTTOM;
            else return QMainWindow::nativeEvent(eventType, message, result);
            return true;
        }
#endif
        return QMainWindow::nativeEvent(eventType, message, result);
    }

private:
    QWidget *m_surface = nullptr;
    QStackedWidget *m_pages = nullptr;
    QToolButton *m_chatsNav = nullptr;
    QToolButton *m_contactsNav = nullptr;
    QToolButton *m_callsNav = nullptr;
    QToolButton *m_settingsNav = nullptr;
    QLabel *m_connectionState = nullptr;
    QLabel *m_statusText = nullptr;
    QLabel *m_conversationTitle = nullptr;
    QLabel *m_conversationSubtitle = nullptr;
    QLineEdit *m_search = nullptr;
    QLabel *m_windowTitleLabel = nullptr;
    QLabel *m_brandLabel = nullptr;
    QComboBox *m_appNameChoice = nullptr;
    QComboBox *m_themeChoice = nullptr;
    QColor m_customAccent;
    TelegramAuth *m_auth = nullptr;
    QListWidget *m_chatList = nullptr;
    QListWidget *m_messageList = nullptr;
    QLineEdit *m_messageInput = nullptr;
    QPushButton *m_sendButton = nullptr;
    qint64 m_selectedChatId = 0;
    QSet<qint64> m_seenMessageIds;
    QDate m_lastMessageDate;

    void buildInterface()
    {
        m_surface = new QWidget(this);
        m_surface->setObjectName("surface");
        setCentralWidget(m_surface);

        QVBoxLayout *root = new QVBoxLayout(m_surface);
        root->setContentsMargins(1, 1, 1, 1);
        root->setSpacing(0);
        QWidget *body = new QWidget(m_surface);
        QHBoxLayout *bodyLayout = new QHBoxLayout(body);
        bodyLayout->setContentsMargins(0, 0, 0, 0);
        bodyLayout->setSpacing(0);

        bodyLayout->addWidget(createSidebar());
        m_pages = new QStackedWidget(body);
        m_pages->setObjectName("pageStack");
        m_pages->addWidget(createChatsPage());
        m_pages->addWidget(createContactsPage());
        m_pages->addWidget(createCallsPage());
        m_pages->addWidget(createSettingsPage());
        bodyLayout->addWidget(m_pages, 1);

        root->addWidget(body, 1);
        root->addWidget(createStatusBar());
        connectNavigation();
    }

    QWidget *createTitleBar()
    {
        DragTitleBar *bar = new DragTitleBar(this);
        bar->setObjectName("titleBar");
        bar->setFixedHeight(38);

        QHBoxLayout *layout = new QHBoxLayout(bar);
        layout->setContentsMargins(12, 0, 7, 0);
        layout->setSpacing(7);

        QLabel *icon = new QLabel(bar);
        icon->setPixmap(createTelegramAeroIcon().pixmap(24, 24));
        icon->setFixedSize(24, 24);
        icon->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        m_windowTitleLabel = makeLabel(QString::fromLatin1(kAppName), "windowTitle");
        QLabel *title = m_windowTitleLabel;
        title->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        QLabel *version = makeLabel("PREVIEW 0.2", "versionTag");
        version->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        QWidget *stretch = new QWidget(bar);
        stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        stretch->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        QPushButton *minimize = windowButton("—", "windowButton");
        QPushButton *maximize = windowButton("□", "windowButton");
        QPushButton *close = windowButton("×", "closeButton");
        minimize->setFixedSize(34, 23);
        maximize->setFixedSize(34, 23);
        close->setFixedSize(46, 23);

        connect(minimize, &QPushButton::clicked, this, &QWidget::showMinimized);
        connect(maximize, &QPushButton::clicked, this, [this]() {
            isMaximized() ? showNormal() : showMaximized();
        });
        connect(close, &QPushButton::clicked, this, &QWidget::close);

        layout->addWidget(icon);
        layout->addWidget(title);
        layout->addWidget(version);
        layout->addWidget(stretch);
        layout->addWidget(minimize);
        layout->addWidget(maximize);
        layout->addWidget(close);
        return bar;
    }

    QPushButton *windowButton(const QString &text, const QString &name)
    {
        QPushButton *button = new QPushButton(text);
        button->setObjectName(name);
        button->setCursor(Qt::ArrowCursor);
        button->setFocusPolicy(Qt::NoFocus);
        return button;
    }

    void showMessagePlaceholder(const QString &text)
    {
        if (!m_messageList) return;
        m_messageList->clear();
        QListWidgetItem *item = new QListWidgetItem(m_messageList);
        item->setData(Qt::UserRole, "empty-state");
        QWidget *container = new QWidget(m_messageList);
        QVBoxLayout *layout = new QVBoxLayout(container);
        layout->setContentsMargins(12, 24, 12, 24);
        QLabel *label = makeLabel(text, "messagePlaceholder");
        label->setAlignment(Qt::AlignCenter);
        layout->addStretch();
        layout->addWidget(label);
        layout->addStretch();
        item->setSizeHint(QSize(400, 170));
        m_messageList->setItemWidget(item, container);
    }

    void appendDateSeparator(const QDate &date)
    {
        if (!m_messageList || !date.isValid()) return;
        QListWidgetItem *item = new QListWidgetItem(m_messageList);
        QWidget *row = new QWidget(m_messageList);
        QHBoxLayout *layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 5, 0, 5);
        QLabel *label = makeLabel(date == QDate::currentDate()
            ? "Today" : date.toString("d MMMM yyyy"), "messageDateSeparator");
        label->setAlignment(Qt::AlignCenter);
        layout->addStretch();
        layout->addWidget(label);
        layout->addStretch();
        item->setSizeHint(QSize(m_messageList->viewport()->width() - 8, 30));
        m_messageList->setItemWidget(item, row);
        item->setSizeHint(QSize(m_messageList->viewport()->width() - 8, row->sizeHint().height()));
    }

    void appendMessageBubble(const QString &body, const QDateTime &sentAt, bool outgoing)
    {
        if (!m_messageList) return;
        QListWidgetItem *item = new QListWidgetItem(m_messageList);
        QWidget *row = new QWidget(m_messageList);
        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(12, 2, 12, 2);
        rowLayout->setSpacing(0);

        QFrame *bubble = new QFrame(row);
        bubble->setObjectName(outgoing ? "outgoingBubble" : "incomingBubble");
        bubble->setMaximumWidth(560);
        QVBoxLayout *bubbleLayout = new QVBoxLayout(bubble);
        bubbleLayout->setContentsMargins(12, 8, 12, 6);
        bubbleLayout->setSpacing(4);

        QLabel *messageText = makeLabel(body, "messageBubbleText");
        messageText->setWordWrap(true);
        messageText->setTextInteractionFlags(Qt::TextSelectableByMouse);
        messageText->setMaximumWidth(520);
        bubbleLayout->addWidget(messageText);

        QHBoxLayout *meta = new QHBoxLayout;
        meta->addStretch();
        QLabel *time = makeLabel(sentAt.isValid() ? sentAt.toString("hh:mm") : QString(), "messageTime");
        meta->addWidget(time);
        bubbleLayout->addLayout(meta);

        if (outgoing) {
            rowLayout->addStretch(1);
            rowLayout->addWidget(bubble, 0, Qt::AlignRight);
        } else {
            rowLayout->addWidget(bubble, 0, Qt::AlignLeft);
            rowLayout->addStretch(1);
        }
        const int textWidth = qMin(520, qMax(180, messageText->fontMetrics().horizontalAdvance(body) + 4));
        messageText->setMinimumWidth(qMin(260, textWidth));
        item->setSizeHint(QSize(m_messageList->viewport()->width() - 8,
                                qMax(48, bubble->sizeHint().height() + 8)));
        m_messageList->setItemWidget(item, row);
        item->setSizeHint(QSize(m_messageList->viewport()->width() - 8,
                                qMax(48, row->sizeHint().height())));
    }

    void refreshChatItem(QListWidgetItem *item)
    {
        if (!item || !m_chatList) return;
        const QString title = item->data(Qt::UserRole + 1).toString();
        const QString preview = item->data(Qt::UserRole + 2).toString();
        const QString initials = title.trimmed().left(2).toUpper();
        const uint hash = qHash(title);
        const QColor tones[] = {
            QColor("#547f9f"), QColor("#668b79"), QColor("#987b62"),
            QColor("#737f9d"), QColor("#8b718d"), QColor("#668b91")
        };
        const QColor tone = tones[hash % (sizeof(tones) / sizeof(tones[0]))];

        QWidget *row = new QWidget(m_chatList);
        row->setObjectName("chatRow");
        QHBoxLayout *layout = new QHBoxLayout(row);
        layout->setContentsMargins(11, 8, 12, 8);
        layout->setSpacing(10);

        QLabel *avatar = new QLabel(initials.isEmpty() ? "?" : initials, row);
        avatar->setObjectName("chatAvatar");
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setFixedSize(44, 44);
        avatar->setStyleSheet(QString("QLabel#chatAvatar { background: %1; color: white; border: 1px solid rgba(255,255,255,110); border-radius: 22px; font-size: 13px; font-weight: 600; }").arg(tone.name()));

        QVBoxLayout *textLayout = new QVBoxLayout;
        textLayout->setContentsMargins(0, 1, 0, 1);
        textLayout->setSpacing(4);

        QHBoxLayout *topLine = new QHBoxLayout;
        topLine->setSpacing(6);
        QLabel *name = makeLabel(title.isEmpty() ? "Telegram chat" : title, "chatName");
        name->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        const int messageTimestamp = item->data(Qt::UserRole + 4).toInt();
        const QDateTime messageDate = messageTimestamp > 0
            ? QDateTime::fromTime_t(static_cast<uint>(messageTimestamp)) : QDateTime();
        QLabel *time = makeLabel(QString(), "chatTime");
        if (messageDate.isValid()) {
            time->setText(messageDate.date() == QDate::currentDate()
                ? messageDate.toString("hh:mm") : messageDate.toString("dd.MM"));
        }
        topLine->addWidget(name, 1);
        topLine->addWidget(time, 0, Qt::AlignRight);
        textLayout->addLayout(topLine);

        QHBoxLayout *bottomLine = new QHBoxLayout;
        bottomLine->setSpacing(6);
        QLabel *message = makeLabel(preview.isEmpty() ? " " : preview, "chatPreview");
        message->setMaximumHeight(18);
        message->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        bottomLine->addWidget(message, 1);
        const int unreadCount = item->data(Qt::UserRole + 3).toInt();
        if (unreadCount > 0) {
            QLabel *unread = makeLabel(unreadCount > 99 ? "99+" : QString::number(unreadCount), "unreadBadge");
            unread->setAlignment(Qt::AlignCenter);
            unread->setMinimumWidth(20);
            unread->setFixedHeight(19);
            bottomLine->addWidget(unread, 0, Qt::AlignRight);
        }
        textLayout->addLayout(bottomLine);
        layout->addWidget(avatar);
        layout->addLayout(textLayout, 1);
        item->setSizeHint(QSize(300, 66));
        m_chatList->setItemWidget(item, row);
    }

    QWidget *createSidebar()
    {
        QFrame *sidebar = new QFrame;
        sidebar->setObjectName("sidebar");
        sidebar->setFixedWidth(330);
        QVBoxLayout *layout = new QVBoxLayout(sidebar);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        QWidget *top = new QWidget(sidebar);
        QVBoxLayout *topLayout = new QVBoxLayout(top);
        topLayout->setContentsMargins(12, 12, 12, 10);
        topLayout->setSpacing(9);
        QHBoxLayout *toolbar = new QHBoxLayout;
        m_brandLabel = makeLabel("Telegram", "brand");
        toolbar->addWidget(m_brandLabel);
        toolbar->addStretch();
        QPushButton *newMessage = new QPushButton("+");
        newMessage->setObjectName("secondaryButton");
        newMessage->setFixedSize(30, 28);
        newMessage->setToolTip("New message");
        toolbar->addWidget(newMessage);
        QPushButton *settingsButton = new QPushButton("⚙");
        settingsButton->setObjectName("secondaryButton");
        settingsButton->setFixedSize(30, 28);
        settingsButton->setToolTip("Settings");
        toolbar->addWidget(settingsButton);
        topLayout->addLayout(toolbar);
        connect(settingsButton, &QPushButton::clicked, this, [this]() {
            if (m_pages) m_pages->setCurrentIndex(3);
        });

        m_search = new QLineEdit(sidebar);
        m_search->setPlaceholderText("Search");
        m_search->setClearButtonEnabled(true);
        m_search->setObjectName("searchField");
        topLayout->addWidget(m_search);
        layout->addWidget(top);

        m_chatList = new QListWidget(sidebar);
        m_chatList->setObjectName("chatList");
        m_chatList->setSpacing(1);
        m_chatList->setFrameShape(QFrame::NoFrame);
        m_chatList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_chatList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        layout->addWidget(m_chatList, 1);

        QWidget *bottom = new QWidget(sidebar);
        QHBoxLayout *bottomLayout = new QHBoxLayout(bottom);
        bottomLayout->setContentsMargins(12, 7, 12, 8);
        m_connectionState = makeLabel("●  Not connected", "connectionState");
        bottomLayout->addWidget(m_connectionState, 1);
        QPushButton *connectButton = new QPushButton("Connect");
        connectButton->setObjectName("connectButton");
        connectButton->setProperty("class", "secondaryButton");
        bottomLayout->addWidget(connectButton);
        layout->addWidget(bottom);
        connect(connectButton, &QPushButton::clicked, this, [this]() { showConnectionNotice(); });

        connect(m_chatList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current, QListWidgetItem *) {
                if (!current || !m_auth) return;
                m_selectedChatId = current->data(Qt::UserRole).toLongLong();
                const QString chatTitle = current->data(Qt::UserRole + 1).toString();
                if (m_conversationTitle) m_conversationTitle->setText(chatTitle);
                if (m_conversationSubtitle) m_conversationSubtitle->setText("Loading recent messages…");
                if (m_messageList) {
                    m_messageList->clear();
                    showMessagePlaceholder("Loading conversation…");
                }
                m_seenMessageIds.clear();
                m_lastMessageDate = QDate();
                m_auth->requestHistory(m_selectedChatId, 50);
            });
        connect(m_search, &QLineEdit::textChanged, this, [this](const QString &query) {
            if (!m_chatList) return;
            for (int i = 0; i < m_chatList->count(); ++i) {
                QListWidgetItem *item = m_chatList->item(i);
                const QString title = item->data(Qt::UserRole + 1).toString();
                const QString preview = item->data(Qt::UserRole + 2).toString();
                item->setHidden(!title.contains(query, Qt::CaseInsensitive)
                    && !preview.contains(query, Qt::CaseInsensitive));
            }
        });
        connect(newMessage, &QPushButton::clicked, this, [this]() {
            if (m_search) { m_search->clear(); m_search->setFocus(); }
        });
        return sidebar;
    }

    QWidget *createChatsPage()
    {
        QWidget *page = new QWidget;
        page->setObjectName("contentPage");
        QVBoxLayout *layout = new QVBoxLayout(page);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        QWidget *header = new QWidget(page);
        header->setObjectName("conversationHeader");
        QHBoxLayout *headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(18, 10, 18, 10);
        QVBoxLayout *titles = new QVBoxLayout;
        titles->setSpacing(1);
        m_conversationTitle = makeLabel("Telegram", "conversationTitle");
        m_conversationSubtitle = makeLabel("Select a chat to start messaging", "pageSubtitle");
        titles->addWidget(m_conversationTitle);
        titles->addWidget(m_conversationSubtitle);
        headerLayout->addLayout(titles);
        headerLayout->addStretch();
        QPushButton *refresh = new QPushButton("Refresh");
        refresh->setObjectName("secondaryButton");
        headerLayout->addWidget(refresh);
        layout->addWidget(header);

        m_messageList = new QListWidget(page);
        m_messageList->setObjectName("messageList");
        m_messageList->setFrameShape(QFrame::NoFrame);
        m_messageList->setSpacing(8);
        m_messageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_messageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        layout->addWidget(m_messageList, 1);

        QWidget *composer = new QWidget(page);
        QHBoxLayout *composerLayout = new QHBoxLayout(composer);
        composerLayout->setContentsMargins(14, 9, 14, 12);
        composerLayout->setSpacing(8);
        m_messageInput = new QLineEdit(composer);
        m_messageInput->setPlaceholderText("Message");
        QPushButton *attach = new QPushButton("＋");
        attach->setObjectName("secondaryButton");
        attach->setToolTip("File attachments are not available in this preview build");
        attach->setEnabled(false);
        m_sendButton = new QPushButton("Send");
        m_sendButton->setObjectName("primaryButton");
        m_sendButton->setEnabled(false);
        m_sendButton->setToolTip("Connect your Telegram account first.");
        composerLayout->addWidget(attach);
        composerLayout->addWidget(m_messageInput, 1);
        composerLayout->addWidget(m_sendButton);
        layout->addWidget(composer);

        connect(refresh, &QPushButton::clicked, this, [this]() {
            if (m_auth) m_auth->requestChats(100);
        });
        const auto sendMessage = [this]() {
            if (!m_auth || !m_selectedChatId || !m_messageInput) return;
            if (!m_auth->isAuthorized()) {
                QMessageBox::information(this, "Telegram", "Connect your Telegram account before sending messages.");
                return;
            }
            const QString value = m_messageInput->text().trimmed();
            if (value.isEmpty()) return;
            if (value.size() > 4096) {
                QMessageBox::warning(this, "Message too long", "Telegram text messages are limited to 4096 characters.");
                return;
            }
            m_auth->sendTextMessage(m_selectedChatId, value);
            m_messageInput->clear();
        };
        connect(m_sendButton, &QPushButton::clicked, this, sendMessage);
        connect(m_messageInput, &QLineEdit::returnPressed, this, sendMessage);
        return page;
    }

    QWidget *createContactsPage()
    {
        return createEmptyPage("Contacts", "Your contacts will appear here after Telegram is connected.",
                               "Contacts are not loaded in this preview build.");
    }

    QWidget *createCallsPage()
    {
        return createEmptyPage("Calls", "Your call history will appear here after Telegram is connected.",
                               "Calling is not available in this preview build.");
    }

    QWidget *createEmptyPage(const QString &title, const QString &subtitle, const QString &detail)
    {
        QWidget *page = new QWidget;
        page->setObjectName("contentPage");
        QVBoxLayout *layout = new QVBoxLayout(page);
        layout->setContentsMargins(34, 28, 34, 26);
        layout->setSpacing(10);
        layout->addWidget(makeLabel(title, "pageTitle"));
        layout->addWidget(makeLabel(subtitle, "pageSubtitle"));
        layout->addStretch();
        QLabel *empty = makeLabel(detail, "emptyPageText");
        empty->setAlignment(Qt::AlignCenter);
        layout->addWidget(empty, 1);
        layout->addStretch();
        return page;
    }

    QWidget *createSettingsPage()
    {
        QWidget *page = new QWidget;
        page->setObjectName("contentPage");
        QVBoxLayout *layout = new QVBoxLayout(page);
        layout->setContentsMargins(34, 28, 34, 26);
        layout->setSpacing(14);
        QHBoxLayout *settingsHeader = new QHBoxLayout;
        QPushButton *backToChats = new QPushButton("←  Chats");
        backToChats->setObjectName("secondaryButton");
        settingsHeader->addWidget(backToChats, 0, Qt::AlignLeft);
        settingsHeader->addStretch();
        layout->addLayout(settingsHeader);
        connect(backToChats, &QPushButton::clicked, this, [this]() {
            if (m_pages) m_pages->setCurrentIndex(0);
        });
        layout->addWidget(makeLabel("Settings", "pageTitle"));
        layout->addWidget(makeLabel("Application preferences and information.", "pageSubtitle"));
        layout->addSpacing(8);

        QFrame *general = settingsGroup("GENERAL");
        QVBoxLayout *generalLayout = qobject_cast<QVBoxLayout *>(general->layout());
        addSettingRow(generalLayout, "Start with Windows", "Startup integration is not enabled in this build.");
        addSettingRow(generalLayout, "Minimize to tray", "Background mode will be added with notifications.");
        QHBoxLayout *nameRow = new QHBoxLayout;
        nameRow->addWidget(makeLabel("Application name", "settingTitle"));
        m_appNameChoice = new QComboBox(page);
        m_appNameChoice->addItem("AERO7 GRAM", "AERO7 GRAM");
        m_appNameChoice->addItem("Telegram", "Telegram");
        nameRow->addStretch(); nameRow->addWidget(m_appNameChoice);
        generalLayout->addLayout(nameRow);

        QHBoxLayout *themeRow = new QHBoxLayout;
        themeRow->addWidget(makeLabel("Theme color", "settingTitle"));
        m_themeChoice = new QComboBox(page);
        m_themeChoice->addItem("Aero Silver", "#8b9ba5");
        m_themeChoice->addItem("Aero Blue", "#5688a8");
        m_themeChoice->addItem("Aero Green", "#648b78");
        m_themeChoice->addItem("Graphite", "#68747c");
        m_themeChoice->addItem("Custom", "custom");
        themeRow->addStretch(); themeRow->addWidget(m_themeChoice);
        QPushButton *customColor = new QPushButton("Custom…", page);
        customColor->setObjectName("secondaryButton"); themeRow->addWidget(customColor);
        generalLayout->addLayout(themeRow);
        connect(m_appNameChoice, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { saveAppearance(); });
        connect(m_themeChoice, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { saveAppearance(); });
        connect(customColor, &QPushButton::clicked, this, [this]() {
            const QColor color = QColorDialog::getColor(m_customAccent.isValid() ? m_customAccent : QColor("#8b9ba5"), this, "Choose theme color");
            if (!color.isValid()) return;
            m_customAccent = color;
            const int customIndex = m_themeChoice->findData("custom");
            if (customIndex >= 0) m_themeChoice->setCurrentIndex(customIndex);
            QSettings s; s.setValue("appearance/theme", "custom"); s.setValue("appearance/customAccent", color.name());
            applyTheme(color);
        });
        layout->addWidget(general);

        QFrame *account = settingsGroup("TELEGRAM ACCOUNT");
        QVBoxLayout *accountLayout = qobject_cast<QVBoxLayout *>(account->layout());
        addSettingRow(accountLayout, "Connection", "Managed by TDLib");
        addSettingRow(accountLayout, "Session storage", "Local TDLib database · Windows DPAPI-protected key");
        QPushButton *signOut = new QPushButton("Sign out of Telegram");
        signOut->setObjectName("secondaryButton");
        signOut->setCursor(Qt::PointingHandCursor);
        accountLayout->addWidget(signOut, 0, Qt::AlignLeft);
        connect(signOut, &QPushButton::clicked, this, [this]() {
            if (m_auth && m_auth->isAvailable())
                m_auth->logOut();
            else
                showConnectionNotice();
        });
        layout->addWidget(account);

        QFrame *about = settingsGroup("ABOUT");
        QVBoxLayout *aboutLayout = qobject_cast<QVBoxLayout *>(about->layout());
        addSettingRow(aboutLayout, "Version", QString::fromLatin1(kVersion));
        addSettingRow(aboutLayout, "Platform", "Windows 7 SP1 · x86 / x64");
        QPushButton *aboutButton = new QPushButton("License and project information");
        aboutButton->setObjectName("linkButton");
        aboutButton->setCursor(Qt::PointingHandCursor);
        aboutLayout->addWidget(aboutButton, 0, Qt::AlignLeft);
        connect(aboutButton, &QPushButton::clicked, this, [this]() { showAbout(); });
        layout->addWidget(about);
        layout->addStretch();
        return page;
    }

    QFrame *settingsGroup(const QString &title)
    {
        QFrame *frame = new QFrame;
        frame->setObjectName("settingsGroup");
        QVBoxLayout *layout = new QVBoxLayout(frame);
        layout->setContentsMargins(16, 12, 16, 13);
        layout->setSpacing(8);
        layout->addWidget(makeLabel(title, "sectionLabel"));
        return frame;
    }

    void addSettingRow(QVBoxLayout *layout, const QString &title, const QString &value)
    {
        QHBoxLayout *row = new QHBoxLayout;
        QVBoxLayout *labels = new QVBoxLayout;
        labels->setSpacing(2);
        labels->addWidget(makeLabel(title, "settingTitle"));
        QLabel *detail = makeLabel(value, "settingDetail");
        detail->setWordWrap(true);
        labels->addWidget(detail);
        row->addLayout(labels, 1);
        layout->addLayout(row);
    }

    QWidget *createStatusBar()
    {
        QFrame *bar = new QFrame;
        bar->setObjectName("statusBar");
        bar->setFixedHeight(25);
        QHBoxLayout *layout = new QHBoxLayout(bar);
        layout->setContentsMargins(12, 0, 12, 0);
        m_statusText = makeLabel("●  Not connected", "statusText");
        QLabel *right = makeLabel(QString("AERO7 GRAM  ·  %1").arg(QString::fromLatin1(kVersion)), "statusText");
        layout->addWidget(m_statusText);
        layout->addStretch();
        layout->addWidget(right);
        return bar;
    }

    void connectNavigation()
    {
        // The primary screen is now the Telegram-style two-pane chat view.
        // Secondary pages will be exposed through the native application menu.
    }

    void setPage(int index)
    {
        m_pages->setCurrentIndex(index);
        m_chatsNav->setChecked(index == 0);
        m_contactsNav->setChecked(index == 1);
        m_callsNav->setChecked(index == 2);
        m_settingsNav->setChecked(index == 3);
    }

    void showConnectionNotice()
    {
        if (m_auth) {
            m_auth->start();
            return;
        }
        QMessageBox::warning(this, "Telegram connection",
            "The Telegram authorization component could not be initialized.");
    }

    void showAbout()
    {
        QMessageBox box(this);
        box.setWindowTitle("About AERO7 GRAM");
        box.setIconPixmap(createTelegramAeroIcon().pixmap(48, 48));
        box.setText(QString("%1  ·  %2").arg(QString::fromLatin1(kAppName),
                                             QString::fromLatin1(kVersion)));
        box.setInformativeText(
            "An independent Telegram desktop client project for Windows 7 SP1.\n\n"
            "This is an early interface preview, not a connected Telegram client.\n"
            "Not affiliated with Telegram or Microsoft.");
        box.setStandardButtons(QMessageBox::Ok);
        box.exec();
    }

    void applyStyle()
    {
        setStyleSheet(QString::fromLatin1(R"(
            QWidget {
                color: #27343b;
                font-family: "Segoe UI", Tahoma, sans-serif;
                font-size: 9pt;
            }
            #surface {
                background: rgba(235, 241, 244, 246);
                border: 1px solid rgba(255,255,255,245);
            }
            #titleBar {
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 rgba(255,255,255,235), stop:0.18 rgba(255,255,255,235),
                    stop:0.52 rgba(215,222,225,205), stop:1 rgba(158,171,178,195));
                border-bottom: 1px solid rgba(255,255,255,245);
            }
            #windowTitle { color: #26343b; font-size: 9pt; font-weight: 600; }
            #versionTag { color: #63737b; font-size: 7pt; padding-left: 5px; }
            QPushButton#windowButton, QPushButton#closeButton {
                color: #26343b; border: 1px solid rgba(255,255,255,235);
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #faffff, stop:1 #b4c0c6);
                padding: 0; font-size: 10pt;
            }
            QPushButton#windowButton:hover { background: #ffffff; }
            QPushButton#closeButton {
                color: white; font-size: 13pt; font-weight: bold;
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #f5a99a, stop:1 #c92e22);
            }
            QPushButton#closeButton:hover {
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #ffb7a8, stop:1 #e53c2c);
            }
            #sidebar {
                background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                    stop:0 rgba(246,248,249,250), stop:1 rgba(222,228,231,245));
                border-right: 1px solid #b8c2c7;
            }
            #brand { color: #243d4d; font-size: 15pt; font-weight: 600; letter-spacing: -0.3px; }
            #chatList { background: rgba(245,247,248,245); border: none; outline: none; }
            #chatList::item { border: 1px solid transparent; padding: 0; }
            #chatList::item:hover { background: rgba(255,255,255,165); border-color: #d5dde0; }
            #chatList::item:selected {
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 rgba(225,237,244,245), stop:1 rgba(202,220,231,245));
                border: 1px solid #a8bdc9;
            }
            #chatName { color: #26343b; font-size: 9pt; font-weight: 600; }
            #chatPreview { color: #687b86; font-size: 8pt; }
            #messageList { background: #e9eef1; border: none; outline: none; }
            #messageList::item { border: none; padding: 0; }
            #incomingBubble {
                background: #ffffff; border: 1px solid #cbd6dc;
                border-radius: 2px;
            }
            #outgoingBubble {
                background: #dcecf7; border: 1px solid #b7d2e4;
                border-radius: 2px;
            }
            #messageBubbleText { color: #26343b; font-size: 9pt; }
            #messageTime { color: #7b898e; font-size: 7pt; }
            #messagePlaceholder { color: #7b898e; font-size: 9pt; }
            #subtitle, #sectionLabel {
                color: #74838a; font-size: 7pt; font-weight: 700;
                letter-spacing: 1px;
            }
            #connectionState { color: #426b83; font-size: 8pt; padding: 4px 1px; }
            QLineEdit {
                min-height: 31px; padding: 0 9px; color: #27343b;
                border: 1px solid #b7c1c6; border-top-color: #9ba8ae;
                background: rgba(255,255,255,238);
                selection-background-color: #8b9ba5;
            }
            QLineEdit:focus { border: 1px solid #8399a5; background: #fff; }
            #navigationButton {
                text-align: left; padding-left: 11px; color: #45545b;
                border: 1px solid transparent; background: transparent;
                font-size: 9pt;
            }
            #navigationButton:hover {
                border: 1px solid #c6d0d4;
                background: rgba(255,255,255,150);
            }
            #navigationButton:checked {
                color: #26343b; font-weight: 600;
                border: 1px solid #aebbc1;
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 rgba(255,255,255,238), stop:1 rgba(205,215,220,215));
            }
            #sidebarLine { color: #c5cdd1; max-height: 1px; }
            #countBadge {
                color: #5d8195; background: #edf0f1; border: 1px solid #cbd2d5;
                padding: 1px 6px; font-size: 8pt;
            }
            #sidebarEmpty {
                color: #7b878d; font-size: 8pt; padding: 7px 3px;
            }
            QPushButton#connectButton, QPushButton#primaryButton {
                color: #ffffff; font-weight: 600; padding: 7px 17px;
                border: 1px solid #28658b;
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #5e9fc5, stop:0.48 #397fa9, stop:1 #28658b);
            }
            QPushButton#connectButton:hover, QPushButton#primaryButton:hover {
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #75b4d8, stop:1 #397fa9);
            }
            #contentPage { background: rgba(248,249,249,246); }
            #pageTitle { color: #26343b; font-size: 20pt; font-weight: 600; }
            #pageSubtitle { color: #78858b; font-size: 9pt; }
            #welcomePanel {
                border: 1px solid #d1d8db;
                background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                    stop:0 #ffffff, stop:1 rgba(255,255,255,238));
            }
            #welcomeTitle { color: #26343b; font-size: 17pt; font-weight: 600; }
            #welcomeText { color: #637178; font-size: 9pt; }
            QPushButton#secondaryButton {
                color: #35434a; padding: 7px 12px;
                border: 1px solid #b9c3c7;
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #fff, stop:1 #e2e7e9);
            }
            QPushButton#secondaryButton:hover { background: #ffffff; }
            QPushButton#linkButton {
                color: #526e7c; border: 0; background: transparent;
                padding: 7px 9px; text-decoration: underline;
            }
            #footnote { color: #929da2; font-size: 7pt; letter-spacing: 1px; }
            #emptyPageText {
                color: #78858b; font-size: 11pt; padding: 30px;
                border: 1px dashed #cbd3d6; background: #fafbfb;
            }
            QListWidget {
                color: #303d43; background: rgba(255,255,255,225);
                border: 1px solid #cbd3d6; outline: 0; padding: 3px;
            }
            QListWidget::item { padding: 10px 9px; border-bottom: 1px solid rgba(190,201,207,150); }
            QListWidget#chatList { background: #f7f9fa; border: 0; padding: 3px; }
            QListWidget#chatList::item { min-height: 58px; }
            QListWidget#chatList::item:selected { color: #26343b; background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #edf5f8,stop:1 #d4e2e8); border: 1px solid #9db3be; }
            QListWidget#messageList { background: #e9eef1; border: 0; padding: 12px; }
            QListWidget#messageList::item { padding: 0; margin: 2px 0; border: 0; background: transparent; }
            QListWidget#messageList::item:selected { background: #e6f0f3; color: #26343b; }
            #settingsGroup {
                border: 1px solid #d1d8db; background: #fff;
            }
            #settingTitle { color: #35434a; font-weight: 600; }
            #settingDetail { color: #828e93; font-size: 8pt; }
            #statusBar {
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #f5f7f8, stop:1 #dfe5e7);
                border-top: 1px solid #c6cfd3;
            }
            #statusText { color: #77848a; font-size: 7pt; }
            QMessageBox { background: #f0f3f4; }
        )"));
    }

    void saveAppearance() {
        QSettings s;
        s.setValue("appearance/appName", m_appNameChoice->currentData().toString());
        const QString theme = m_themeChoice->currentData().toString();
        s.setValue("appearance/theme", theme);
        updateApplicationName();
        if (theme == "custom") {
            if (m_customAccent.isValid()) applyTheme(m_customAccent);
        } else applyTheme(QColor(theme));
    }

    void updateApplicationName() {
        const QString name = m_appNameChoice ? m_appNameChoice->currentData().toString() : QString::fromLatin1(kAppName);
        setWindowTitle(name);
        if (m_windowTitleLabel) m_windowTitleLabel->setText(name);
        if (m_brandLabel) m_brandLabel->setText(name);
    }

    void applyTheme(const QColor &accent) {
        if (!accent.isValid()) return;
        applyStyle();
        QString css = styleSheet();
        css.replace("#8b9ba5", accent.name());
        css.replace("#5688a8", accent.darker(115).name());
        css.replace("#648b78", accent.darker(115).name());
        css.replace("#68747c", accent.darker(115).name());
        css.replace("#e4e9eb", accent.lighter(145).name());
        css.replace("#c4cdd1", accent.darker(108).name());
        setStyleSheet(css);
    }

    void restorePreferences() {
        QSettings s;
        const QByteArray geometry = s.value("window/geometry").toByteArray();
        if (!geometry.isEmpty()) restoreGeometry(geometry);
        if (s.value("window/maximized", false).toBool()) showMaximized();
        const QString appName = s.value("appearance/appName", "AERO7 GRAM").toString();
        int ni = m_appNameChoice->findData(appName); if (ni >= 0) m_appNameChoice->setCurrentIndex(ni);
        const QString theme = s.value("appearance/theme", "#8b9ba5").toString();
        int ti = m_themeChoice->findData(theme); if (ti >= 0) m_themeChoice->setCurrentIndex(ti);
        if (theme == "custom") { m_customAccent = QColor(s.value("appearance/customAccent", "#8b9ba5").toString()); applyTheme(m_customAccent); }
        else applyTheme(QColor(theme));
        updateApplicationName();
    }
#ifdef Q_OS_WIN
    void enableAeroGlass()
    {
        BOOL composition = FALSE;
        if (SUCCEEDED(DwmIsCompositionEnabled(&composition)) && composition) {
            const MARGINS margins = {-1, -1, -1, -1};
            DwmExtendFrameIntoClientArea(reinterpret_cast<HWND>(winId()), &margins);
            DWM_BLURBEHIND blur = {};
            blur.dwFlags = DWM_BB_ENABLE;
            blur.fEnable = TRUE;
            DwmEnableBlurBehindWindow(reinterpret_cast<HWND>(winId()), &blur);
        }
    }
#endif
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // Qt 5 ships the Windows Vista style plugin, which matches the native
    // Windows 7 control metrics and interaction states when available.
    QStyle *nativeWindowsStyle = QStyleFactory::create("windowsvista");
    if (nativeWindowsStyle)
        app.setStyle(nativeWindowsStyle);
    QApplication::setApplicationName(QString::fromLatin1(kAppName));
    QApplication::setOrganizationName(QString::fromLatin1(kOrganization));
    QApplication::setApplicationVersion(QString::fromLatin1(kVersion));
    QApplication::setWindowIcon(createTelegramAeroIcon());
    AeroWindow window;
    window.show();
    return app.exec();
}
