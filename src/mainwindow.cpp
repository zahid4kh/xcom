#include "mainwindow.h"
#include "xcomview.h"
#include "xcomprofile.h"
#include "resourcepanel.h"
#include "svgicon.h"

#include <QApplication>
#include <QMenuBar>
#include <QResizeEvent>
#include <QToolBar>
#include <QToolButton>
#include <QTabWidget>
#include <QTabBar>
#include <QShortcut>
#include <QKeySequence>
#include <QWebEngineView>
#include <QWebEngineHistory>
#include <QWebEngineFullScreenRequest>

static const QUrl HOME_URL = QUrl(QStringLiteral("https://x.com"));
static const QColor ICON_COLOR(180, 180, 210);

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("XCOM"));
    resize(1280, 800);

    setupMenu();
    setupToolbar();
    setupTabs();

    auto* newTabShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this);
    connect(newTabShortcut, &QShortcut::activated, this, &MainWindow::onNewTab);

    m_resourcePanel = new ResourcePanel(this);
    m_resourcePanel->raise();

    createTab(HOME_URL);
}

void MainWindow::setupMenu()
{
    QMenu* menu = menuBar()->addMenu(QStringLiteral("XCOM"));

    QAction* logoutAct = menu->addAction(
        svgIcon(QStringLiteral(":/icons/log-out.svg"), ICON_COLOR, 16),
        QStringLiteral("Log Out"));
    connect(logoutAct, &QAction::triggered, this, &MainWindow::onLogout);

    menu->addSeparator();

    QAction* quitAct = menu->addAction(QStringLiteral("Quit"));
    quitAct->setShortcut(QKeySequence::Quit);
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);
}

void MainWindow::setupToolbar()
{
    m_toolbar = addToolBar(QStringLiteral("Navigation"));
    m_toolbar->setMovable(false);
    m_toolbar->setFloatable(false);
    m_toolbar->setIconSize(QSize(20, 20));

    m_back    = m_toolbar->addAction(svgIcon(QStringLiteral(":/icons/arrow-left.svg"),  ICON_COLOR), QString());
    m_forward = m_toolbar->addAction(svgIcon(QStringLiteral(":/icons/arrow-right.svg"), ICON_COLOR), QString());
    QAction* refresh = m_toolbar->addAction(svgIcon(QStringLiteral(":/icons/rotate-cw.svg"), ICON_COLOR), QString());
    QAction* home    = m_toolbar->addAction(svgIcon(QStringLiteral(":/icons/home.svg"),  ICON_COLOR), QString());
    m_toolbar->addSeparator();
    QAction* newTab   = m_toolbar->addAction(svgIcon(QStringLiteral(":/icons/x.svg"), ICON_COLOR), QString());
    m_toolbar->addSeparator();
    QAction* statsAct = m_toolbar->addAction(svgIcon(QStringLiteral(":/icons/activity.svg"), ICON_COLOR), QString());

    m_back->setToolTip(QStringLiteral("Back"));
    m_forward->setToolTip(QStringLiteral("Forward"));
    refresh->setToolTip(QStringLiteral("Refresh"));
    home->setToolTip(QStringLiteral("Go to x.com home"));
    newTab->setToolTip(QStringLiteral("Open new tab (Ctrl+T)"));
    statsAct->setToolTip(QStringLiteral("Resource Monitor"));

    connect(m_back,    &QAction::triggered, this, [this]() {
        if (auto* v = currentView()) v->back();
    });
    connect(m_forward, &QAction::triggered, this, [this]() {
        if (auto* v = currentView()) v->forward();
    });
    connect(refresh,   &QAction::triggered, this, [this]() {
        if (auto* v = currentView()) v->reload();
    });
    connect(home,     &QAction::triggered, this, &MainWindow::onHome);
    connect(newTab,   &QAction::triggered, this, &MainWindow::onNewTab);
    connect(statsAct, &QAction::triggered, this, [this]() {
        m_resourcePanel->toggle();
    });
}

void MainWindow::setupTabs()
{
    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(false);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    m_tabs->setElideMode(Qt::ElideRight);

    connect(m_tabs, &QTabWidget::currentChanged,
            this,   &MainWindow::onCurrentTabChanged);

    setCentralWidget(m_tabs);
}

XComView* MainWindow::createTab(const QUrl& url)
{
    auto* view = new XComView(this);
    const int idx = m_tabs->addTab(view, QStringLiteral("Loading…"));
    m_tabs->setCurrentIndex(idx);

    auto* closeBtn = new QToolButton(m_tabs->tabBar());
    closeBtn->setIcon(svgIcon(QStringLiteral(":/icons/circle-x.svg"), QColor(100, 100, 160), 14));
    closeBtn->setFixedSize(18, 18);
    closeBtn->setAutoRaise(true);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(QStringLiteral(
        "QToolButton { background: transparent; border: none; padding: 0; }"
        "QToolButton:hover { background: rgba(255,255,255,15); border-radius: 4px; }"
    ));
    m_tabs->tabBar()->setTabButton(idx, QTabBar::RightSide, closeBtn);
    connect(closeBtn, &QToolButton::clicked, this, [this, view]() {
        onTabCloseRequested(m_tabs->indexOf(view));
    });

    connect(view, &QWebEngineView::titleChanged, this,
            [this, view](const QString& title) {
        const int i = m_tabs->indexOf(view);
        if (i >= 0)
            m_tabs->setTabText(i, title.isEmpty() ? QStringLiteral("X") : title);
    });

    connect(view, &QWebEngineView::loadStarted, this,
            [this, view]() {
        const int i = m_tabs->indexOf(view);
        if (i >= 0)
            m_tabs->setTabText(i, QStringLiteral("Loading…"));
    });

    connect(view, &QWebEngineView::urlChanged, this,
            [this]() { updateNavActions(); });

    connect(view->page(), &QWebEnginePage::windowCloseRequested, this,
            [this, view]() {
        const int i = m_tabs->indexOf(view);
        if (i >= 0 && m_tabs->count() > 1)
            onTabCloseRequested(i);
    });

    connect(view->page(), &QWebEnginePage::fullScreenRequested, this,
            [this](QWebEngineFullScreenRequest request) {
        request.accept();
        if (request.toggleOn()) {
            m_prevWindowState = windowState();
            menuBar()->hide();
            m_toolbar->hide();
            m_tabs->tabBar()->hide();
            showFullScreen();
        } else {
            setWindowState(m_prevWindowState);
            menuBar()->show();
            m_toolbar->show();
            m_tabs->tabBar()->show();
        }
    });

    if (url.isValid() && !url.isEmpty())
        view->load(url);

    updateNavActions();
    return view;
}

XComView* MainWindow::currentView() const
{
    return qobject_cast<XComView*>(m_tabs->currentWidget());
}

void MainWindow::onTabCloseRequested(int index)
{
    if (m_tabs->count() == 1) {
        if (auto* v = currentView())
            v->load(HOME_URL);
        return;
    }
    QWidget* w = m_tabs->widget(index);
    m_tabs->removeTab(index);
    w->deleteLater();
}

void MainWindow::onCurrentTabChanged(int)
{
    updateNavActions();
}

void MainWindow::onNewTab()
{
    createTab(HOME_URL);
}

void MainWindow::onHome()
{
    if (auto* v = currentView())
        v->load(HOME_URL);
}

void MainWindow::onLogout()
{
    XComProfile::instance()->logout();
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (auto* v = qobject_cast<XComView*>(m_tabs->widget(i)))
            v->load(HOME_URL);
    }
}

void MainWindow::updateNavActions()
{
    auto* v = currentView();
    m_back->setEnabled(v && v->history()->canGoBack());
    m_forward->setEnabled(v && v->history()->canGoForward());
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if (m_resourcePanel)
        m_resourcePanel->reanchor();
}
