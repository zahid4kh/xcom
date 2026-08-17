// Copyright 2026 Zahid Khalilov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mainwindow.h"
#include "widgetheader.h"
#include "xcomview.h"
#include "xcomprofile.h"
#include "resourcepanel.h"
#include "svgicon.h"

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QKeySequence>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QTabBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineView>
#include <QWebEngineFullScreenRequest>

static const QUrl HOME_URL = QUrl(QStringLiteral("https://x.com"));
static const QColor ICON_COLOR(180, 180, 210);

static const char NOTIFICATIONS_COUNT_JS[] = R"js(
(function(){
    var e=document.querySelector('[data-testid="AppTabBar_Notifications_Link"]');
    if(!e)return 0;
    var a=e.getAttribute('aria-label')||'';
    var m=a.match(/\((\d+)\s*unread/);
    return m?parseInt(m[1],10):0;
})()
)js";

static constexpr int WIDGET_W = 400;
static constexpr int WIDGET_H = 700;

static const QColor WIDGET_BG_COLOR(0x09, 0x09, 0x0b);
static constexpr int WIDGET_RADIUS = 22;

static const char WIDGET_CSS_JS[] = R"js(
(function(){
    if(document.getElementById('xcom-widget-css'))return;
    var s=document.createElement('style');
    s.id='xcom-widget-css';
    s.textContent=
        '[data-testid="sidebarColumn"]{display:none!important}'+
        'nav[aria-label="Primary"]{display:none!important}'+
        'header[role="banner"]{display:none!important}'+
        '[data-testid="primaryColumn"]{max-width:100%!important;border:none!important}';
    if(document.head)document.head.appendChild(s);
})();
)js";

static const char REMOVE_CSS_JS[] = R"js(
(function(){var e=document.getElementById('xcom-widget-css');if(e)e.remove();})();
)js";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("XCOM"));
    resize(1280, 800);

    m_scrollSpeed = QSettings().value(QStringLiteral("scrollSpeed"), 1.0).toReal();

    setupMenu();
    setupToolbar();
    setupNavRail();
    setupZoomControls();
    setupTabs();
    setupTray();

    auto *newTabShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this);
    connect(newTabShortcut, &QShortcut::activated, this, &MainWindow::onNewTab);

    auto *modeShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W), this);
    connect(modeShortcut, &QShortcut::activated, this, [this]()
            { m_mode == WindowMode::Widget ? exitWidgetMode() : enterWidgetMode(); });

    m_resourcePanel = new ResourcePanel(this);
    m_resourcePanel->raise();
    connect(m_resourcePanel, &ResourcePanel::cpuStatsChanged, m_cpuMiniLabel, &QLabel::setText);
    connect(m_resourcePanel, &ResourcePanel::memStatsChanged, m_memMiniLabel, &QLabel::setText);
    connect(m_resourcePanel, &ResourcePanel::diskStatsChanged, m_diskMiniLabel, &QLabel::setText);

    createTab(HOME_URL);
    loadSettings();
}

void MainWindow::setupMenu()
{
    QMenu *menu = menuBar()->addMenu(QStringLiteral("XCOM"));

    QAction *logoutAct = menu->addAction(
        svgIcon(QStringLiteral(":/icons/log-out.svg"), ICON_COLOR, 16),
        QStringLiteral("Log Out"));
    connect(logoutAct, &QAction::triggered, this, &MainWindow::onLogout);

    menu->addSeparator();

    QMenu *scrollMenu = menu->addMenu(QStringLiteral("Scroll Speed"));
    auto *scrollGroup = new QActionGroup(this);
    scrollGroup->setExclusive(true);

    struct SpeedOption
    {
        QString label;
        qreal value;
    };
    const QList<SpeedOption> speeds = {
        {QStringLiteral("0.5x"), 0.5},
        {QStringLiteral("0.75x"), 0.75},
        {QStringLiteral("1.0x (Default)"), 1.0},
        {QStringLiteral("1.5x"), 1.5},
        {QStringLiteral("2.0x"), 2.0},
        {QStringLiteral("3.0x"), 3.0},
    };
    for (const SpeedOption &opt : speeds)
    {
        QAction *act = scrollMenu->addAction(opt.label);
        act->setCheckable(true);
        act->setChecked(qFuzzyCompare(m_scrollSpeed, opt.value));
        scrollGroup->addAction(act);
        connect(act, &QAction::triggered, this, [this, value = opt.value]()
                {
            m_scrollSpeed = value;
            QSettings().setValue(QStringLiteral("scrollSpeed"), value); });
    }

    menu->addSeparator();

    m_widgetModeAction = menu->addAction(QStringLiteral("Enter Widget Mode"));
    m_widgetModeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W));
    connect(m_widgetModeAction, &QAction::triggered, this, [this]()
            { m_mode == WindowMode::Widget ? exitWidgetMode() : enterWidgetMode(); });

    menu->addSeparator();

    QAction *quitAct = menu->addAction(QStringLiteral("Quit"));
    quitAct->setShortcut(QKeySequence::Quit);
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);
}

void MainWindow::setupToolbar()
{
    m_toolbar = addToolBar(QStringLiteral("Navigation"));
    m_toolbar->setMovable(false);
    m_toolbar->setFloatable(false);
    m_toolbar->setIconSize(QSize(20, 20));

    m_back = m_toolbar->addAction(svgIcon(QStringLiteral(":/icons/arrow-left.svg"), ICON_COLOR), QString());
    m_forward = m_toolbar->addAction(svgIcon(QStringLiteral(":/icons/arrow-right.svg"), ICON_COLOR), QString());
    QAction *refresh = m_toolbar->addAction(svgIcon(QStringLiteral(":/icons/rotate-cw.svg"), ICON_COLOR), QString());
    QAction *home = m_toolbar->addAction(svgIcon(QStringLiteral(":/icons/home.svg"), ICON_COLOR), QString());
    m_toolbar->addSeparator();
    QAction *newTab = m_toolbar->addAction(svgIcon(QStringLiteral(":/icons/x.svg"), ICON_COLOR), QString());
    m_toolbar->addSeparator();
    QAction *statsAct = m_toolbar->addAction(svgIcon(QStringLiteral(":/icons/activity.svg"), ICON_COLOR), QString());

    auto *miniStats = new QWidget(m_toolbar);
    auto *miniLayout = new QHBoxLayout(miniStats);
    miniLayout->setContentsMargins(6, 0, 4, 0);
    miniLayout->setSpacing(6);

    const auto addMiniLabel = [&](const QString &color)
    {
        auto *lbl = new QLabel(QStringLiteral("—"), miniStats);
        lbl->setStyleSheet(QStringLiteral(
                               "color:%1; font-size:11px; font-weight:600;")
                               .arg(color));
        miniLayout->addWidget(lbl);
        return lbl;
    };

    m_cpuMiniLabel = addMiniLabel(QStringLiteral("#00e5ff"));
    m_memMiniLabel = addMiniLabel(QStringLiteral("#ff2bd6"));
    m_diskMiniLabel = addMiniLabel(QStringLiteral("#39ff88"));
    m_toolbar->addWidget(miniStats);

    auto *versionLabel = new QLabel(
        QStringLiteral("v%1").arg(QApplication::applicationVersion()), m_toolbar);
    versionLabel->setStyleSheet(QStringLiteral(
        "color:#52525b; font-size:10px; font-weight:600; padding:0 8px;"));
    m_toolbar->addWidget(versionLabel);

    m_back->setToolTip(QStringLiteral("Back"));
    m_forward->setToolTip(QStringLiteral("Forward"));
    refresh->setToolTip(QStringLiteral("Refresh"));
    home->setToolTip(QStringLiteral("Go to x.com home"));
    newTab->setToolTip(QStringLiteral("Open new tab (Ctrl+T)"));
    statsAct->setToolTip(QStringLiteral("Resource Monitor"));

    connect(m_back, &QAction::triggered, this, [this]()
            {
        if (auto* v = currentView()) v->back(); });
    connect(m_forward, &QAction::triggered, this, [this]()
            {
        if (auto* v = currentView()) v->forward(); });
    connect(refresh, &QAction::triggered, this, [this]()
            {
        if (auto* v = currentView()) v->reload(); });
    connect(home, &QAction::triggered, this, &MainWindow::onHome);
    connect(newTab, &QAction::triggered, this, &MainWindow::onNewTab);
    connect(statsAct, &QAction::triggered, this, [this]()
            { m_resourcePanel->toggle(); });
}

void MainWindow::setupNavRail()
{
    m_navRail = new QToolBar(QStringLiteral("Navigate"), this);
    m_navRail->setMovable(false);
    m_navRail->setFloatable(false);
    m_navRail->setIconSize(QSize(22, 22));
    m_navRail->setToolButtonStyle(Qt::ToolButtonIconOnly);
    addToolBar(Qt::LeftToolBarArea, m_navRail);

    const auto addLinkAction = [this](const QString &icon, const QString &tooltip, const QString &selector)
    {
        QAction *act = m_navRail->addAction(
            svgIcon(QStringLiteral(":/icons/%1.svg").arg(icon), ICON_COLOR, 22), QString());
        act->setToolTip(tooltip);
        connect(act, &QAction::triggered, this, [this, selector]()
                { triggerNavLink(selector); });
        return act;
    };

    const auto addUrlAction = [this](const QString &icon, const QString &tooltip, const QUrl &url)
    {
        QAction *act = m_navRail->addAction(
            svgIcon(QStringLiteral(":/icons/%1.svg").arg(icon), ICON_COLOR, 22), QString());
        act->setToolTip(tooltip);
        connect(act, &QAction::triggered, this, [this, url]()
                { navigateTo(url); });
    };

    addLinkAction(QStringLiteral("square-pen"), QStringLiteral("Post"),
                  QStringLiteral("[data-testid=\"SideNav_NewTweet_Button\"]"));
    m_navRail->addSeparator();
    addLinkAction(QStringLiteral("home"), QStringLiteral("Home"),
                  QStringLiteral("[data-testid=\"AppTabBar_Home_Link\"]"));
    addLinkAction(QStringLiteral("search"), QStringLiteral("Explore"),
                  QStringLiteral("[data-testid=\"AppTabBar_Explore_Link\"]"));
    m_notificationsAction = addLinkAction(QStringLiteral("bell"), QStringLiteral("Notifications"),
                                          QStringLiteral("[data-testid=\"AppTabBar_Notifications_Link\"]"));
    m_bellBaseIcon = m_notificationsAction->icon();

    auto *notifTimer = new QTimer(this);
    connect(notifTimer, &QTimer::timeout, this, &MainWindow::pollNotificationsBadge);
    notifTimer->start(5000);
    addLinkAction(QStringLiteral("message-circle"), QStringLiteral("Chat"),
                  QStringLiteral("[data-testid=\"AppTabBar_DirectMessage_Link\"]"));
    addLinkAction(QStringLiteral("grok"), QStringLiteral("Grok"),
                  QStringLiteral("nav[aria-label=\"Primary\"] a[aria-label=\"Grok\"]"));
    addLinkAction(QStringLiteral("badge-check"), QStringLiteral("Premium"),
                  QStringLiteral("[data-testid=\"premium-hub-tab\"]"));
    addLinkAction(QStringLiteral("bookmark"), QStringLiteral("Bookmarks"),
                  QStringLiteral("nav[aria-label=\"Primary\"] a[aria-label=\"Bookmarks\"]"));
    addUrlAction(QStringLiteral("clapperboard"), QStringLiteral("Creator Studio"),
                 QUrl(QStringLiteral("https://x.com/i/jf/creators/studio")));
    addUrlAction(QStringLiteral("newspaper"), QStringLiteral("Articles"),
                 QUrl(QStringLiteral("https://x.com/compose/articles")));
    addLinkAction(QStringLiteral("circle-user"), QStringLiteral("Profile"),
                  QStringLiteral("[data-testid=\"AppTabBar_Profile_Link\"]"));
    addUrlAction(QStringLiteral("settings"), QStringLiteral("Settings"),
                 QUrl(QStringLiteral("https://x.com/settings")));
}

void MainWindow::triggerNavLink(const QString &selector)
{
    auto *v = currentView();
    if (!v)
        return;
    const QString js = QStringLiteral(
                           "(function(){var e=document.querySelector('%1');if(e)e.click();})();")
                           .arg(selector);
    v->page()->runJavaScript(js);
}

void MainWindow::navigateTo(const QUrl &url)
{
    if (auto *v = currentView())
        v->load(url);
}

void MainWindow::setupZoomControls()
{
    m_zoomLabel = new QLabel(QStringLiteral("100%"), m_toolbar);
    m_zoomLabel->setStyleSheet(QStringLiteral(
        "color:#b4b4d2; font-size:11px; font-weight:600; padding:0 4px;"));
    m_toolbar->addWidget(m_zoomLabel);

    auto *zoomTimer = new QTimer(this);
    connect(zoomTimer, &QTimer::timeout, this, [this]()
            {
        if (auto *v = currentView()) {
            const QString text = QStringLiteral("%1%").arg(qRound(v->zoomFactor() * 100));
            if (m_zoomLabel->text() != text)
                m_zoomLabel->setText(text);
        } });
    zoomTimer->start(250);

    auto *zoomInShortcut = new QShortcut(QKeySequence::ZoomIn, this);
    connect(zoomInShortcut, &QShortcut::activated, this, [this]()
            {
        if (auto *v = currentView())
            v->setZoomFactor(qBound(0.25, v->zoomFactor() + 0.1, 5.0)); });

    auto *zoomOutShortcut = new QShortcut(QKeySequence::ZoomOut, this);
    connect(zoomOutShortcut, &QShortcut::activated, this, [this]()
            {
        if (auto *v = currentView())
            v->setZoomFactor(qBound(0.25, v->zoomFactor() - 0.1, 5.0)); });

    auto *zoomResetShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_0), this);
    connect(zoomResetShortcut, &QShortcut::activated, this, [this]()
            {
        if (auto *v = currentView())
            v->setZoomFactor(1.0); });
}

void MainWindow::setupTabs()
{
    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(false);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    m_tabs->setElideMode(Qt::ElideRight);

    connect(m_tabs, &QTabWidget::currentChanged,
            this, &MainWindow::onCurrentTabChanged);

    setCentralWidget(m_tabs);
}

void MainWindow::setupTray()
{
    QApplication::setQuitOnLastWindowClosed(false);

    m_tray = new QSystemTrayIcon(windowIcon(), this);
    m_tray->setToolTip(QStringLiteral("XCOM"));

    auto *trayMenu = new QMenu(this);
    trayMenu->setStyleSheet(QStringLiteral(
        "QMenu {"
        "  background:#0f0f24; border:1px solid #252550;"
        "  border-radius:8px; padding:4px 0; color:#c0c0f0;}"
        "QMenu::item { padding:7px 20px 7px 14px; border-radius:5px; margin:1px 4px; font-size:13px;}"
        "QMenu::item:selected { background:#1e1e42; color:#ffffff;}"
        "QMenu::separator { height:1px; background:#252550; margin:3px 8px;}"));

    m_trayModeAction = trayMenu->addAction(QStringLiteral("Enter Widget Mode"));
    connect(m_trayModeAction, &QAction::triggered, this, [this]()
            { m_mode == WindowMode::Widget ? exitWidgetMode() : enterWidgetMode(); });

    trayMenu->addSeparator();

    QAction *showAct = trayMenu->addAction(QStringLiteral("Show XCOM"));
    connect(showAct, &QAction::triggered, this, [this]()
            {
        show();
        raise();
        activateWindow(); });

    trayMenu->addSeparator();

    QAction *quitAct = trayMenu->addAction(QStringLiteral("Quit"));
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);

    m_tray->setContextMenu(trayMenu);

    connect(m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason)
            {
                if (reason == QSystemTrayIcon::Trigger)
                {
                    if (isVisible())
                    {
                        raise();
                        activateWindow();
                    }
                    else
                    {
                        show();
                        raise();
                        activateWindow();
                    }
                }
            });

    m_tray->show();

    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::saveSettings);
}

XComView *MainWindow::createTab(const QUrl &url)
{
    auto *view = new XComView(this);
    const int idx = m_tabs->addTab(view, QStringLiteral("Loading…"));
    m_tabs->setCurrentIndex(idx);

    if (m_mode == WindowMode::Widget)
        view->page()->setBackgroundColor(WIDGET_BG_COLOR);

    auto *closeBtn = new QToolButton(m_tabs->tabBar());
    closeBtn->setIcon(svgIcon(QStringLiteral(":/icons/circle-x.svg"), QColor(100, 100, 160), 14));
    closeBtn->setFixedSize(18, 18);
    closeBtn->setAutoRaise(true);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(QStringLiteral(
        "QToolButton { background: transparent; border: none; padding: 0; }"
        "QToolButton:hover { background: rgba(255,255,255,15); border-radius: 4px; }"));
    m_tabs->tabBar()->setTabButton(idx, QTabBar::RightSide, closeBtn);
    connect(closeBtn, &QToolButton::clicked, this, [this, view]()
            { onTabCloseRequested(m_tabs->indexOf(view)); });

    connect(view, &QWebEngineView::titleChanged, this,
            [this, view](const QString &title)
            {
                const int i = m_tabs->indexOf(view);
                if (i >= 0)
                    m_tabs->setTabText(i, title.isEmpty() ? QStringLiteral("X") : title);
                if (view == currentView())
                    pollNotificationsBadge();
            });

    connect(view, &QWebEngineView::loadStarted, this,
            [this, view]()
            {
                const int i = m_tabs->indexOf(view);
                if (i >= 0)
                    m_tabs->setTabText(i, QStringLiteral("Loading…"));
            });

    connect(view, &QWebEngineView::urlChanged, this,
            [this]()
            { updateNavActions(); });

    connect(view->page(), &QWebEnginePage::windowCloseRequested, this,
            [this, view]()
            {
                const int i = m_tabs->indexOf(view);
                if (i >= 0 && m_tabs->count() > 1)
                    onTabCloseRequested(i);
            });

    connect(view->page(), &QWebEnginePage::fullScreenRequested, this,
            [this](QWebEngineFullScreenRequest request)
            {
                request.accept();
                if (request.toggleOn())
                {
                    m_prevWindowState = windowState();
                    m_inFullscreen = true;

                    if (m_mode == WindowMode::Widget)
                    {
                        clearMask();
                        setWindowFlags(Qt::Window);
                        if (m_widgetHeader)
                            m_widgetHeader->hide();
                    }

                    menuBar()->hide();
                    m_toolbar->hide();
                    m_navRail->hide();
                    m_tabs->tabBar()->hide();
                    showFullScreen();
                }
                else
                {
                    m_inFullscreen = false;

                    if (m_mode == WindowMode::Widget)
                    {
                        Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::Tool;
                        if (QGuiApplication::platformName() == QLatin1String("xcb"))
                            flags |= Qt::WindowStaysOnBottomHint;
                        setWindowFlags(flags);
                        resize(WIDGET_W, WIDGET_H);
                        applyWidgetMask();
                        if (m_hasWidgetPos)
                            move(m_widgetPos);
                        if (m_widgetHeader)
                            m_widgetHeader->show();
                        show();
                    }
                    else
                    {
                        setWindowState(m_prevWindowState);
                        menuBar()->show();
                        m_toolbar->show();
                        m_navRail->show();
                        m_tabs->tabBar()->show();
                    }
                }
            });

    if (url.isValid() && !url.isEmpty())
        view->load(url);

    updateNavActions();
    return view;
}

XComView *MainWindow::currentView() const
{
    return qobject_cast<XComView *>(m_tabs->currentWidget());
}

void MainWindow::onTabCloseRequested(int index)
{
    if (m_tabs->count() == 1)
    {
        if (auto *v = currentView())
            v->load(HOME_URL);
        return;
    }
    QWidget *w = m_tabs->widget(index);
    m_tabs->removeTab(index);
    w->deleteLater();
}

void MainWindow::onCurrentTabChanged(int)
{
    updateNavActions();
    pollNotificationsBadge();
}

void MainWindow::pollNotificationsBadge()
{
    auto *v = currentView();
    if (!v)
        return;
    v->page()->runJavaScript(QString::fromUtf8(NOTIFICATIONS_COUNT_JS),
                             [this](const QVariant &result)
                             { updateNotificationsBadge(result.toInt()); });
}

void MainWindow::updateNotificationsBadge(int count)
{
    if (!m_notificationsAction)
        return;

    if (count <= 0)
    {
        m_notificationsAction->setIcon(m_bellBaseIcon);
        return;
    }

    QPixmap pix = m_bellBaseIcon.pixmap(22, 22);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    const QString text = count > 99 ? QStringLiteral("99+") : QString::number(count);
    QFont f = p.font();
    f.setPixelSize(9);
    f.setBold(true);
    p.setFont(f);
    QFontMetrics fm(f);
    const int textW = fm.horizontalAdvance(text);
    const int badgeW = qMax(14, textW + 6);
    const int badgeH = 13;
    const QRectF badgeRect(pix.width() - badgeW, 0, badgeW, badgeH);

    p.setBrush(QColor(244, 33, 46));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(badgeRect, badgeH / 2.0, badgeH / 2.0);

    p.setPen(Qt::white);
    p.drawText(badgeRect, Qt::AlignCenter, text);
    p.end();

    m_notificationsAction->setIcon(QIcon(pix));
}

void MainWindow::onNewTab()
{
    if (m_mode == WindowMode::Widget)
        return;
    createTab(HOME_URL);
}

void MainWindow::onHome()
{
    if (auto *v = currentView())
        v->load(HOME_URL);
}

void MainWindow::onLogout()
{
    XComProfile::instance()->logout();
    for (int i = 0; i < m_tabs->count(); ++i)
    {
        if (auto *v = qobject_cast<XComView *>(m_tabs->widget(i)))
            v->load(HOME_URL);
    }
}

void MainWindow::enterWidgetMode()
{
    if (m_mode == WindowMode::Widget)
        return;

    m_normalGeometry = saveGeometry();

    hide();

    Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::Tool;
    // WindowStaysOnBottomHint is X11-only; unreliable on Wayland.
    if (QGuiApplication::platformName() == QLatin1String("xcb"))
        flags |= Qt::WindowStaysOnBottomHint;
    setWindowFlags(flags);

    m_mode = WindowMode::Widget;

    takeCentralWidget();

    auto *container = new QWidget(this);
    container->setObjectName(QStringLiteral("WidgetFrame"));
    container->setAttribute(Qt::WA_StyledBackground);
    container->setStyleSheet(QStringLiteral(
        "#WidgetFrame {"
        "  background: #09090b;"
        "  border-radius: 22px;"
        "  border: 1px solid #27272a;"
        "}"));

    auto *vl = new QVBoxLayout(container);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    m_widgetHeader = new WidgetHeader(container);
    connect(m_widgetHeader, &WidgetHeader::exitRequested,
            this, &MainWindow::exitWidgetMode);
    connect(m_widgetHeader, &WidgetHeader::hideRequested,
            this, [this]
            { hide(); });
    connect(m_widgetHeader, &WidgetHeader::reloadRequested,
            this, [this]
            { if (auto* v = currentView()) v->reload(); });
    vl->addWidget(m_widgetHeader);

    auto *webWrapper = new QWidget(container);
    webWrapper->setAutoFillBackground(false);
    auto *wl = new QVBoxLayout(webWrapper);
    wl->setContentsMargins(5, 0, 5, 5);
    wl->setSpacing(0);
    wl->addWidget(m_tabs);
    vl->addWidget(webWrapper);

    setCentralWidget(container);

    menuBar()->hide();
    m_toolbar->hide();
    m_navRail->hide();
    m_tabs->tabBar()->hide();
    if (m_resourcePanel->isVisible())
        m_resourcePanel->hide();

    setAllViewsBackground(WIDGET_BG_COLOR);

    resize(WIDGET_W, WIDGET_H);
    applyWidgetMask();
    if (m_hasWidgetPos)
    {
        move(m_widgetPos);
    }
    else
    {
        const QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
        move(avail.right() - WIDGET_W - 20,
             avail.bottom() - WIDGET_H - 20);
    }

    show();

    injectWidgetCss();

    updateModeUi();
    saveSettings();
}

void MainWindow::exitWidgetMode()
{
    if (m_mode == WindowMode::Normal)
        return;

    m_widgetPos = pos();
    m_hasWidgetPos = true;

    removeWidgetCss();

    hide();

    QWidget *container = takeCentralWidget();
    m_tabs->setParent(this);
    m_widgetHeader = nullptr;
    delete container;
    setCentralWidget(m_tabs);

    clearMask();
    setWindowFlags(Qt::Window);

    m_mode = WindowMode::Normal;

    setAllViewsBackground(Qt::white);

    menuBar()->show();
    m_toolbar->show();
    m_navRail->show();
    m_tabs->tabBar()->show();
    m_resourcePanel->reanchor();

    if (!m_normalGeometry.isEmpty())
        restoreGeometry(m_normalGeometry);

    show();

    updateModeUi();
    saveSettings();
}

void MainWindow::injectWidgetCss()
{
    QWebEngineScriptCollection *scripts = XComProfile::instance()->scripts();
    if (scripts->find(QStringLiteral("xcom-widget-css")).isEmpty())
    {
        QWebEngineScript script;
        script.setName(QStringLiteral("xcom-widget-css"));
        script.setSourceCode(QString::fromUtf8(WIDGET_CSS_JS));
        script.setInjectionPoint(QWebEngineScript::DocumentReady);
        script.setWorldId(QWebEngineScript::MainWorld);
        script.setRunsOnSubFrames(false);
        scripts->insert(script);
    }

    const QString js = QString::fromUtf8(WIDGET_CSS_JS);
    for (int i = 0; i < m_tabs->count(); ++i)
    {
        if (auto *v = qobject_cast<XComView *>(m_tabs->widget(i)))
            v->page()->runJavaScript(js);
    }
}

void MainWindow::removeWidgetCss()
{
    QWebEngineScriptCollection *scripts = XComProfile::instance()->scripts();
    const QList<QWebEngineScript> found = scripts->find(QStringLiteral("xcom-widget-css"));
    for (const QWebEngineScript &s : found)
        scripts->remove(s);

    const QString js = QString::fromUtf8(REMOVE_CSS_JS);
    for (int i = 0; i < m_tabs->count(); ++i)
    {
        if (auto *v = qobject_cast<XComView *>(m_tabs->widget(i)))
            v->page()->runJavaScript(js);
    }
}

void MainWindow::applyWidgetMask()
{

    QPainterPath path;
    path.addRoundedRect(rect(), WIDGET_RADIUS, WIDGET_RADIUS);
    setMask(QRegion(path.toFillPolygon().toPolygon()));
}

void MainWindow::setAllViewsBackground(const QColor &color)
{
    for (int i = 0; i < m_tabs->count(); ++i)
    {
        if (auto *v = qobject_cast<XComView *>(m_tabs->widget(i)))
            v->page()->setBackgroundColor(color);
    }
}

void MainWindow::updateModeUi()
{
    const bool widget = (m_mode == WindowMode::Widget);
    const QString label = widget
                              ? QStringLiteral("Exit Widget Mode")
                              : QStringLiteral("Enter Widget Mode");

    if (m_widgetModeAction)
        m_widgetModeAction->setText(label);
    if (m_trayModeAction)
        m_trayModeAction->setText(label);
}

void MainWindow::updateNavActions()
{
    auto *v = currentView();
    m_back->setEnabled(v && v->history()->canGoBack());
    m_forward->setEnabled(v && v->history()->canGoForward());
}

void MainWindow::saveSettings()
{
    QSettings s;
    s.setValue(QStringLiteral("mode"), m_mode == WindowMode::Widget ? 1 : 0);

    if (m_mode == WindowMode::Normal)
    {
        s.setValue(QStringLiteral("geometry/normal"), saveGeometry());
    }
    else
    {
        s.setValue(QStringLiteral("geometry/widgetPos"), pos());
        if (!m_normalGeometry.isEmpty())
            s.setValue(QStringLiteral("geometry/normal"), m_normalGeometry);
    }
    if (m_hasWidgetPos)
        s.setValue(QStringLiteral("geometry/widgetPos"), m_widgetPos);
}

void MainWindow::loadSettings()
{
    QSettings s;

    const QByteArray geo = s.value(QStringLiteral("geometry/normal")).toByteArray();
    if (!geo.isEmpty())
    {
        m_normalGeometry = geo;
        restoreGeometry(geo);
    }

    if (s.contains(QStringLiteral("geometry/widgetPos")))
    {
        m_widgetPos = s.value(QStringLiteral("geometry/widgetPos")).toPoint();
        m_hasWidgetPos = true;
    }

    if (s.value(QStringLiteral("mode"), 0).toInt() == 1)
        QTimer::singleShot(0, this, &MainWindow::enterWidgetMode);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (m_resourcePanel)
        m_resourcePanel->reanchor();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_mode == WindowMode::Widget)
    {
        event->ignore();
        hide();
        if (m_tray && QSystemTrayIcon::isSystemTrayAvailable())
        {
            m_tray->showMessage(
                QStringLiteral("XCOM"),
                QStringLiteral("XCOM is running in widget mode. "
                               "Right-click the tray icon to quit or switch modes."),
                QSystemTrayIcon::Information,
                2500);
        }
    }
    else
    {
        saveSettings();
        event->accept();
    }
}
