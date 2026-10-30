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
        m_auth->statusChanged = [this](const QString &status) {
            if (m_connectionState)
                m_connectionState->setText(QString::fromUtf8("●  ") + status);
        };
        m_auth->errorOccurred = [this](const QString &message) {
            QMessageBox::warning(this, "Telegram authorization", message);
        };
        m_auth->authorized = [this](const QString &) {
            if (m_connectionState) m_connectionState->setText("●  Telegram account connected");
        };
        m_auth->chatReceived = [this](const QString &json) {
            const QJsonObject object = QJsonDocument::fromJson(json.toUtf8()).object();
            QJsonObject chat = object.value("chat").toObject();
            if (chat.isEmpty()) chat = object;
            qint64 id = static_cast<qint64>(chat.value("id").toDouble());
            if (!id) id = static_cast<qint64>(object.value("chat_id").toDouble());
            if (!id || !m_chatList) return;
            const QString title = chat.value("title").toString("Telegram chat");
            QListWidgetItem *item = nullptr;
            for (int i = 0; i < m_chatList->count(); ++i) {
                if (m_chatList->item(i)->data(Qt::UserRole).toLongLong() == id) { item = m_chatList->item(i); break; }
            }
            if (!item) { item = new QListWidgetItem(m_chatList); item->setData(Qt::UserRole, QVariant::fromValue<qlonglong>(id)); }
            item->setText(title);
            item->setToolTip(title);
        };
        m_auth->messageReceived = [this](const QString &json) {
            if (!m_messageList) return;
            const QJsonObject message = QJsonDocument::fromJson(json.toUtf8()).object();
            if (static_cast<qint64>(message.value("chat_id").toDouble()) != m_selectedChatId) return;
            const QJsonObject content = message.value("content").toObject();
            QString body = content.value("text").toObject().value("text").toString();
            if (body.isEmpty()) body = QString("[%1]").arg(content.value("@type").toString("Message"));
            const QJsonObject sender = message.value("sender_id").toObject();
            const QString senderType = sender.value("@type").toString();
            const QString prefix = message.value("is_outgoing").toBool() ? "You: " : "Them: ";
            m_messageList->addItem(prefix + body);
            m_messageList->scrollToBottom();
        };
    }

protected:
    void showEvent(QShowEvent *event) override
    {
        QMainWindow::showEvent(event);
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
    qint64 m_selectedChatId = 0;

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

    QWidget *createSidebar()
    {
        QFrame *sidebar = new QFrame;
        sidebar->setObjectName("sidebar");
        sidebar->setFixedWidth(292);

        QVBoxLayout *layout = new QVBoxLayout(sidebar);
        layout->setContentsMargins(13, 15, 13, 13);
        layout->setSpacing(8);

        QHBoxLayout *brandRow = new QHBoxLayout;
        QLabel *brandIcon = new QLabel(sidebar);
        brandIcon->setPixmap(createTelegramAeroIcon().pixmap(38, 38));
        brandIcon->setFixedSize(38, 38);
        QVBoxLayout *brandText = new QVBoxLayout;
        brandText->setSpacing(1);
        m_brandLabel = makeLabel("AERO7 GRAM", "brand");
        brandText->addWidget(m_brandLabel);
        brandText->addWidget(makeLabel("TELEGRAM DESKTOP", "subtitle"));
        brandRow->addWidget(brandIcon);
        brandRow->addLayout(brandText, 1);
        layout->addLayout(brandRow);
        layout->addSpacing(9);

        m_connectionState = makeLabel("●  Not connected", "connectionState");
        layout->addWidget(m_connectionState);

        m_search = new QLineEdit(sidebar);
        m_search->setPlaceholderText("Search");
        m_search->setClearButtonEnabled(true);
        m_search->setObjectName("searchField");
        layout->addWidget(m_search);
        layout->addSpacing(4);

        m_chatsNav = makeNavButton("▤", "All chats", sidebar);
        m_contactsNav = makeNavButton("♙", "Contacts", sidebar);
        m_callsNav = makeNavButton("☎", "Calls", sidebar);
        m_settingsNav = makeNavButton("⚙", "Settings", sidebar);
        layout->addWidget(m_chatsNav);
        layout->addWidget(m_contactsNav);
        layout->addWidget(m_callsNav);
        layout->addWidget(m_settingsNav);
        layout->addSpacing(9);

        QFrame *line = new QFrame(sidebar);
        line->setFrameShape(QFrame::HLine);
        line->setObjectName("sidebarLine");
        layout->addWidget(line);

        QHBoxLayout *listHeader = new QHBoxLayout;
        listHeader->addWidget(makeLabel("CONVERSATIONS", "sectionLabel"));
        listHeader->addStretch();
        QLabel *count = makeLabel("0", "countBadge");
        listHeader->addWidget(count);
        layout->addLayout(listHeader);

        QLabel *empty = makeLabel("Your chats will appear here after you connect Telegram.", "sidebarEmpty");
        empty->setWordWrap(true);
        empty->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        layout->addWidget(empty);
        layout->addStretch();

        QPushButton *connectButton = new QPushButton("Connect Telegram");
        connectButton->setObjectName("connectButton");
        connectButton->setCursor(Qt::PointingHandCursor);
        layout->addWidget(connectButton);
        connect(connectButton, &QPushButton::clicked, this, [this]() { showConnectionNotice(); });

        return sidebar;
    }

    QWidget *createChatsPage()
    {
        QWidget *page = new QWidget;
        page->setObjectName("contentPage");
        QVBoxLayout *layout = new QVBoxLayout(page);
        layout->setContentsMargins(24, 20, 24, 18);
        layout->setSpacing(10);

        QHBoxLayout *header = new QHBoxLayout;
        QVBoxLayout *heading = new QVBoxLayout;
        heading->setSpacing(2);
        heading->addWidget(makeLabel("Messages", "pageTitle"));
        heading->addWidget(makeLabel("Your Telegram conversations", "pageSubtitle"));
        header->addLayout(heading);
        header->addStretch();
        QPushButton *refresh = new QPushButton("Refresh chats");
        refresh->setObjectName("secondaryButton");
        header->addWidget(refresh);
        layout->addLayout(header);

        QFrame *workspace = new QFrame(page);
        workspace->setObjectName("welcomePanel");
        QHBoxLayout *split = new QHBoxLayout(workspace);
        split->setContentsMargins(8, 8, 8, 8);
        split->setSpacing(8);

        QWidget *chatPane = new QWidget(workspace);
        chatPane->setMinimumWidth(190);
        QVBoxLayout *chatLayout = new QVBoxLayout(chatPane);
        chatLayout->setContentsMargins(0, 0, 0, 0);
        chatLayout->addWidget(makeLabel("CHATS", "sectionLabel"));
        m_chatList = new QListWidget(chatPane);
        m_chatList->setObjectName("chatList");
        m_chatList->setSpacing(1);
        chatLayout->addWidget(m_chatList, 1);

        QWidget *conversation = new QWidget(workspace);
        QVBoxLayout *conversationLayout = new QVBoxLayout(conversation);
        conversationLayout->setContentsMargins(0, 0, 0, 0);
        conversationLayout->addWidget(makeLabel("CONVERSATION", "sectionLabel"));
        m_messageList = new QListWidget(conversation);
        m_messageList->setObjectName("messageList");
        conversationLayout->addWidget(m_messageList, 1);
        QHBoxLayout *composer = new QHBoxLayout;
        m_messageInput = new QLineEdit(conversation);
        m_messageInput->setPlaceholderText("Write a message…");
        QPushButton *send = new QPushButton("Send");
        send->setObjectName("primaryButton");
        composer->addWidget(m_messageInput, 1);
        composer->addWidget(send);
        conversationLayout->addLayout(composer);

        split->addWidget(chatPane, 1);
        split->addWidget(conversation, 2);
        layout->addWidget(workspace, 1);
        QLabel *hint = makeLabel("Connect an account to load chats. Messages are sent through TDLib.", "footnote");
        layout->addWidget(hint);

        connect(refresh, &QPushButton::clicked, this, [this]() {
            if (m_auth) m_auth->requestChats(100);
        });
        connect(m_chatList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current, QListWidgetItem *) {
                if (!current || !m_auth) return;
                m_selectedChatId = current->data(Qt::UserRole).toLongLong();
                m_messageList->clear();
                m_auth->requestHistory(m_selectedChatId, 50);
            });
        const auto sendMessage = [this]() {
            if (!m_auth || !m_selectedChatId || !m_messageInput) return;
            const QString value = m_messageInput->text().trimmed();
            if (value.isEmpty()) return;
            m_auth->sendTextMessage(m_selectedChatId, value);
            m_messageInput->clear();
        };
        connect(send, &QPushButton::clicked, this, sendMessage);
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
        m_themeChoice->addItem("Aero Blue", "#20a8e4");
        m_themeChoice->addItem("Sky", "#36a9e1");
        m_themeChoice->addItem("Emerald", "#159b78");
        m_themeChoice->addItem("Violet", "#8064c8");
        m_themeChoice->addItem("Rose", "#d45b78");
        m_themeChoice->addItem("Graphite", "#607887");
        m_themeChoice->addItem("Custom", "custom");
        themeRow->addStretch(); themeRow->addWidget(m_themeChoice);
        QPushButton *customColor = new QPushButton("Custom…", page);
        customColor->setObjectName("secondaryButton"); themeRow->addWidget(customColor);
        generalLayout->addLayout(themeRow);
        connect(m_appNameChoice, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { saveAppearance(); });
        connect(m_themeChoice, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { saveAppearance(); });
        connect(customColor, &QPushButton::clicked, this, [this]() {
            const QColor color = QColorDialog::getColor(m_customAccent.isValid() ? m_customAccent : QColor("#20a8e4"), this, "Choose theme color");
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
        QLabel *left = makeLabel("●  Not connected", "statusText");
        QLabel *right = makeLabel(QString("AERO7 GRAM  ·  %1").arg(QString::fromLatin1(kVersion)), "statusText");
        layout->addWidget(left);
        layout->addStretch();
        layout->addWidget(right);
        return bar;
    }

    void connectNavigation()
    {
        connect(m_chatsNav, &QToolButton::clicked, this, [this]() { setPage(0); });
        connect(m_contactsNav, &QToolButton::clicked, this, [this]() { setPage(1); });
        connect(m_callsNav, &QToolButton::clicked, this, [this]() { setPage(2); });
        connect(m_settingsNav, &QToolButton::clicked, this, [this]() { setPage(3); });
        m_chatsNav->setChecked(true);
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
                color: #234b64;
                font-family: "Segoe UI", Tahoma, sans-serif;
                font-size: 9pt;
            }
            #surface {
                background: rgba(221, 241, 249, 246);
                border: 1px solid rgba(255,255,255,245);
            }
            #titleBar {
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 rgba(255,255,255,235), stop:0.18 rgba(225,246,255,225),
                    stop:0.52 rgba(139,197,228,205), stop:1 rgba(67,145,190,195));
                border-bottom: 1px solid rgba(255,255,255,245);
            }
            #windowTitle { color: #174d6b; font-size: 9pt; font-weight: 600; }
            #versionTag { color: #47758d; font-size: 7pt; padding-left: 5px; }
            QPushButton#windowButton, QPushButton#closeButton {
                color: #174c6b; border: 1px solid rgba(255,255,255,235);
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #faffff, stop:1 #a9d8ed);
                padding: 0; font-size: 10pt;
            }
            QPushButton#windowButton:hover { background: #e8fbff; }
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
                    stop:0 rgba(235,249,253,250), stop:1 rgba(213,239,248,245));
                border-right: 1px solid #b8d9e8;
            }
            #brand { color: #155b83; font-size: 14pt; font-weight: 600; }
            #subtitle, #sectionLabel {
                color: #6a8b9d; font-size: 7pt; font-weight: 700;
                letter-spacing: 1px;
            }
            #connectionState { color: #8a7651; font-size: 8pt; padding: 4px 1px; }
            QLineEdit {
                min-height: 31px; padding: 0 9px; color: #204e68;
                border: 1px solid #b5d2df; border-top-color: #91b6c9;
                background: rgba(255,255,255,220);
                selection-background-color: #5db9df;
            }
            QLineEdit:focus { border: 1px solid #66b9dc; background: #fff; }
            #navigationButton {
                text-align: left; padding-left: 11px; color: #416a80;
                border: 1px solid transparent; background: transparent;
                font-size: 9pt;
            }
            #navigationButton:hover {
                border: 1px solid #c1e0ee;
                background: rgba(255,255,255,145);
            }
            #navigationButton:checked {
                color: #174f70; font-weight: 600;
                border: 1px solid #a8d0e4;
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 rgba(255,255,255,220), stop:1 rgba(190,225,241,210));
            }
            #sidebarLine { color: #c5dfe9; max-height: 1px; }
            #countBadge {
                color: #5d8195; background: #e8f5fa; border: 1px solid #c5e0eb;
                padding: 1px 6px; font-size: 8pt;
            }
            #sidebarEmpty {
                color: #7895a5; font-size: 8pt; padding: 7px 3px;
            }
            QPushButton#connectButton, QPushButton#primaryButton {
                color: white; font-weight: 600; padding: 8px 16px;
                border: 1px solid #237eae;
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #69c8ec, stop:0.48 #2499cf, stop:1 #1476ad);
            }
            QPushButton#connectButton:hover, QPushButton#primaryButton:hover {
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #8bdcff, stop:1 #1686bf);
            }
            #contentPage { background: rgba(247,252,254,248); }
            #pageTitle { color: #1c5c7d; font-size: 20pt; font-weight: 600; }
            #pageSubtitle { color: #6d8999; font-size: 9pt; }
            #welcomePanel {
                border: 1px solid #d2e5ed;
                background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                    stop:0 #ffffff, stop:1 #eaf6fb);
            }
            #welcomeTitle { color: #155b83; font-size: 17pt; font-weight: 600; }
            #welcomeText { color: #527386; font-size: 9pt; }
            QPushButton#secondaryButton {
                color: #285f7d; padding: 7px 12px;
                border: 1px solid #b6d3e0;
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #fff, stop:1 #e3f1f7);
            }
            QPushButton#secondaryButton:hover { background: #f4fcff; }
            QPushButton#linkButton {
                color: #287ca8; border: 0; background: transparent;
                padding: 7px 9px; text-decoration: underline;
            }
            #footnote { color: #9aafb9; font-size: 7pt; letter-spacing: 1px; }
            #emptyPageText {
                color: #819aa8; font-size: 11pt; padding: 30px;
                border: 1px dashed #c8dfe9; background: #f8fcfe;
            }
            QListWidget {
                color: #28546b; background: rgba(255,255,255,225);
                border: 1px solid #c8dfe9; outline: 0; padding: 3px;
            }
            QListWidget::item { padding: 7px 6px; border-bottom: 1px solid #edf4f7; }
            QListWidget::item:selected { color: #174f70; background: #d9eef8; border: 1px solid #a8d0e4; }
            #settingsGroup {
                border: 1px solid #d2e4ec; background: #fff;
            }
            #settingTitle { color: #315d75; font-weight: 600; }
            #settingDetail { color: #8299a6; font-size: 8pt; }
            #statusBar {
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #f7fcfe, stop:1 #e0edf3);
                border-top: 1px solid #c6dce6;
            }
            #statusText { color: #7792a1; font-size: 7pt; }
            QMessageBox { background: #eff8fc; }
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
        css.replace("#20a8e4", accent.name());
        css.replace("#0873bd", accent.darker(145).name());
        css.replace("#2499cf", accent.darker(112).name());
        css.replace("#1476ad", accent.darker(145).name());
        css.replace("#69c8ec", accent.lighter(155).name());
        setStyleSheet(css);
    }

    void restorePreferences() {
        QSettings s;
        const QByteArray geometry = s.value("window/geometry").toByteArray();
        if (!geometry.isEmpty()) restoreGeometry(geometry);
        if (s.value("window/maximized", false).toBool()) showMaximized();
        const QString appName = s.value("appearance/appName", "AERO7 GRAM").toString();
        int ni = m_appNameChoice->findData(appName); if (ni >= 0) m_appNameChoice->setCurrentIndex(ni);
        const QString theme = s.value("appearance/theme", "#20a8e4").toString();
        int ti = m_themeChoice->findData(theme); if (ti >= 0) m_themeChoice->setCurrentIndex(ti);
        if (theme == "custom") { m_customAccent = QColor(s.value("appearance/customAccent", "#20a8e4").toString()); applyTheme(m_customAccent); }
        else applyTheme(QColor(theme));
        updateApplicationName();
    }
#ifdef Q_OS_WIN
    void enableAeroGlass()
    {
        BOOL composition = FALSE;
        if (SUCCEEDED(DwmIsCompositionEnabled(&composition)) && composition) {
            const MARGINS margins = {0, 0, 0, 0};
            DwmExtendFrameIntoClientArea(reinterpret_cast<HWND>(winId()), &margins);
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
