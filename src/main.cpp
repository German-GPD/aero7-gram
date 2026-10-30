#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QShowEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QFont>
#include <QColor>
#include <QPixmap>
#include <QIcon>
#include <QSizePolicy>
#include <QPoint>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <dwmapi.h>
#  include <windowsx.h>
#endif

class DragTitleBar final : public QWidget {
public:
    explicit DragTitleBar(QWidget *parent = nullptr) : QWidget(parent) {}

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && window() && !window()->isMaximized()) {
            offset = event->globalPos() - window()->frameGeometry().topLeft();
            dragging = true;
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (dragging && (event->buttons() & Qt::LeftButton) && window()) {
            window()->move(event->globalPos() - offset);
            event->accept();
            return;
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        dragging = false;
        QWidget::mouseReleaseEvent(event);
    }

private:
    QPoint offset;
    bool dragging = false;
};

class AeroWindow final : public QMainWindow {
public:
    AeroWindow() {
        setWindowTitle(QStringLiteral("AERO7 GRAM"));
        setMinimumSize(900, 580);
        resize(1120, 720);
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground, true);

        buildInterface();
        applyFallbackStyle();
    }

protected:
    void showEvent(QShowEvent *event) override {
        QMainWindow::showEvent(event);
#ifdef Q_OS_WIN
        enableAeroGlass();
#endif
    }

    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override {
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
    QWidget *surface = nullptr;

    void buildInterface() {
        surface = new QWidget(this);
        surface->setObjectName(QStringLiteral("surface"));
        setCentralWidget(surface);

        auto *root = new QVBoxLayout(surface);
        root->setContentsMargins(1, 1, 1, 1);
        root->setSpacing(0);

        root->addWidget(createTitleBar());

        auto *body = new QWidget(surface);
        auto *bodyLayout = new QHBoxLayout(body);
        bodyLayout->setContentsMargins(0, 0, 0, 0);
        bodyLayout->setSpacing(0);

        auto *sidebar = new QFrame(body);
        sidebar->setObjectName(QStringLiteral("sidebar"));
        sidebar->setFixedWidth(270);
        auto *sideLayout = new QVBoxLayout(sidebar);
        sideLayout->setContentsMargins(14, 18, 14, 14);
        sideLayout->setSpacing(12);

        auto *brand = new QLabel(QStringLiteral("AERO7 GRAM"), sidebar);
        brand->setObjectName(QStringLiteral("brand"));
        auto *subtitle = new QLabel(QStringLiteral("TELEGRAM · WINDOWS 7 EDITION"), sidebar);
        subtitle->setObjectName(QStringLiteral("subtitle"));

        auto *search = new QLineEdit(sidebar);
        search->setPlaceholderText(QStringLiteral("Search chats"));
        search->setClearButtonEnabled(true);

        auto *chatsLabel = new QLabel(QStringLiteral("MESSAGES"), sidebar);
        chatsLabel->setObjectName(QStringLiteral("sectionLabel"));

        auto *chatList = new QListWidget(sidebar);
        chatList->setObjectName(QStringLiteral("chatList"));
        chatList->setFrameShape(QFrame::NoFrame);
        chatList->setFocusPolicy(Qt::NoFocus);
        chatList->addItem(new QListWidgetItem(QStringLiteral("  ◉   Welcome to AERO7 GRAM")));
        chatList->addItem(new QListWidgetItem(QStringLiteral("  ◉   Saved Messages")));
        chatList->addItem(new QListWidgetItem(QStringLiteral("  ◉   Telegram")));
        chatList->setCurrentRow(0);

        sideLayout->addWidget(brand);
        sideLayout->addWidget(subtitle);
        sideLayout->addSpacing(10);
        sideLayout->addWidget(search);
        sideLayout->addWidget(chatsLabel);
        sideLayout->addWidget(chatList, 1);

        auto *conversation = new QWidget(body);
        auto *conversationLayout = new QVBoxLayout(conversation);
        conversationLayout->setContentsMargins(28, 26, 28, 24);
        conversationLayout->setSpacing(14);

        auto *conversationTitle = new QLabel(QStringLiteral("Welcome to AERO7 GRAM"), conversation);
        conversationTitle->setObjectName(QStringLiteral("conversationTitle"));
        auto *conversationSubtitle = new QLabel(
            QStringLiteral("A classic desktop feeling. Your Telegram, reimagined."),
            conversation);
        conversationSubtitle->setObjectName(QStringLiteral("muted"));

        auto *welcomeCard = new QFrame(conversation);
        welcomeCard->setObjectName(QStringLiteral("welcomeCard"));
        auto *welcomeLayout = new QVBoxLayout(welcomeCard);
        welcomeLayout->setContentsMargins(24, 22, 24, 22);
        welcomeLayout->setSpacing(10);

        auto *welcomeTitle = new QLabel(QStringLiteral("Aero is back."), welcomeCard);
        welcomeTitle->setObjectName(QStringLiteral("welcomeTitle"));
        auto *welcomeText = new QLabel(
            QStringLiteral("This is the first native application shell. The next milestone is "
                           "connecting TDLib, adding secure sign-in and loading real Telegram chats."),
            welcomeCard);
        welcomeText->setObjectName(QStringLiteral("welcomeText"));
        welcomeText->setWordWrap(true);

        auto *status = new QLabel(QStringLiteral("●  APPLICATION SHELL · TELEGRAM CONNECTION NOT YET CONFIGURED"), welcomeCard);
        status->setObjectName(QStringLiteral("status"));

        welcomeLayout->addWidget(welcomeTitle);
        welcomeLayout->addWidget(welcomeText);
        welcomeLayout->addSpacing(8);
        welcomeLayout->addWidget(status);

        auto *spacer = new QWidget(conversation);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto *composer = new QLineEdit(conversation);
        composer->setPlaceholderText(QStringLiteral("Messaging will be available after Telegram integration"));
        composer->setReadOnly(true);
        composer->setObjectName(QStringLiteral("composer"));

        conversationLayout->addWidget(conversationTitle);
        conversationLayout->addWidget(conversationSubtitle);
        conversationLayout->addSpacing(18);
        conversationLayout->addWidget(welcomeCard);
        conversationLayout->addWidget(spacer, 1);
        conversationLayout->addWidget(composer);

        bodyLayout->addWidget(sidebar);
        bodyLayout->addWidget(conversation, 1);
        root->addWidget(body, 1);
    }

    QWidget *createTitleBar() {
        auto *bar = new DragTitleBar(this);
        bar->setObjectName(QStringLiteral("titleBar"));
        bar->setFixedHeight(42);

        auto *layout = new QHBoxLayout(bar);
        layout->setContentsMargins(14, 0, 8, 0);
        layout->setSpacing(5);

        auto *appMark = new QLabel(QStringLiteral("✦"), bar);
        appMark->setObjectName(QStringLiteral("appMark"));
        appMark->setFixedSize(22, 22);
        appMark->setAlignment(Qt::AlignCenter);
        appMark->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        auto *title = new QLabel(QStringLiteral("AERO7 GRAM"), bar);
        title->setObjectName(QStringLiteral("windowTitle"));
        title->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        auto *version = new QLabel(QStringLiteral("PREVIEW"), bar);
        version->setObjectName(QStringLiteral("versionTag"));
        version->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        auto *stretch = new QWidget(bar);
        stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        stretch->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        auto *minimize = new QPushButton(QStringLiteral("—"), bar);
        auto *maximize = new QPushButton(QStringLiteral("□"), bar);
        auto *close = new QPushButton(QStringLiteral("×"), bar);
        minimize->setObjectName(QStringLiteral("windowButton"));
        maximize->setObjectName(QStringLiteral("windowButton"));
        close->setObjectName(QStringLiteral("closeButton"));
        minimize->setFixedSize(34, 23);
        maximize->setFixedSize(34, 23);
        close->setFixedSize(46, 23);

        connect(minimize, &QPushButton::clicked, this, &QWidget::showMinimized);
        connect(maximize, &QPushButton::clicked, this, [this]() {
            isMaximized() ? showNormal() : showMaximized();
        });
        connect(close, &QPushButton::clicked, this, &QWidget::close);

        layout->addWidget(appMark);
        layout->addWidget(title);
        layout->addWidget(version);
        layout->addWidget(stretch);
        layout->addWidget(minimize);
        layout->addWidget(maximize);
        layout->addWidget(close);
        return bar;
    }

    void applyFallbackStyle() {
        setStyleSheet(QStringLiteral(R"(
            QWidget { color: #173e59; font-family: "Segoe UI", Tahoma, sans-serif; font-size: 9pt; }
            #surface { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 rgba(220,246,255,72), stop:0.55 rgba(174,222,244,58), stop:1 rgba(190,244,220,68)); border: 1px solid rgba(255,255,255,245); }
            #titleBar { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 rgba(255,255,255,220), stop:0.48 rgba(196,230,246,175), stop:1 rgba(132,193,224,120)); border-bottom: 1px solid rgba(255,255,255,220); }
            #appMark { color: white; font-size: 14pt; font-weight: bold; border: 1px solid white; background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #8ce8dc, stop:1 #1688bd); }
            #windowTitle { font-weight: 600; color: #174f70; }
            #versionTag { color: #46748d; font-size: 7pt; padding-left: 5px; }
            QPushButton#windowButton, QPushButton#closeButton { color: #174c6b; border: 1px solid rgba(255,255,255,230); background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #faffff, stop:1 #a9d8ed); padding: 0; }
            QPushButton#windowButton:hover { background: #e8fbff; }
            QPushButton#closeButton { color: white; font-size: 13pt; font-weight: bold; background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #f5a99a, stop:1 #c92e22); }
            #sidebar { background: rgba(224,246,252,115); border-right: 1px solid rgba(255,255,255,210); }
            #brand { color: #155b83; font-size: 16pt; font-weight: 600; }
            #subtitle, #sectionLabel { color: #578096; font-size: 7pt; font-weight: 700; letter-spacing: 1px; }
            QLineEdit { min-height: 30px; padding: 0 9px; color: #204e68; border: 1px solid rgba(255,255,255,245); background: rgba(255,255,255,125); selection-background-color: #5db9df; }
            QLineEdit:focus { border: 1px solid #76c9e7; background: rgba(255,255,255,190); }
            #chatList { background: transparent; outline: 0; }
            #chatList::item { min-height: 38px; padding: 5px; border: 1px solid transparent; }
            #chatList::item:selected { color: #174e6d; border: 1px solid white; background: rgba(255,255,255,135); }
            #conversationTitle { color: #15577d; font-size: 17pt; font-weight: 600; }
            #muted { color: #5a7d91; }
            #welcomeCard { border: 1px solid rgba(255,255,255,235); background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 rgba(255,255,255,170), stop:1 rgba(190,232,241,145)); }
            #welcomeTitle { color: #14618a; font-size: 23pt; font-weight: 600; }
            #welcomeText { color: #365f76; font-size: 10pt; line-height: 1.5; }
            #status { color: #8a6537; font-size: 7pt; font-weight: 700; }
            #composer { min-height: 38px; }
        )"));
    }

#ifdef Q_OS_WIN
    void enableAeroGlass() {
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

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("AERO7 GRAM"));
    QApplication::setOrganizationName(QStringLiteral("German-GPD"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    AeroWindow window;
    window.show();
    return app.exec();
}
