#include "xcomview.h"
#include "xcompage.h"
#include "xcomprofile.h"
#include "mainwindow.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QWebEngineContextMenuRequest>
#include <QWebEngineHistory>

XComView::XComView(MainWindow* mainWindow, QWidget* parent)
    : QWebEngineView(parent)
    , m_mainWindow(mainWindow)
{
    auto* p = new XComPage(mainWindow, XComProfile::instance(), this);
    setPage(p);
}

void XComView::contextMenuEvent(QContextMenuEvent* event)
{
    const auto* req = lastContextMenuRequest();
    if (!req) return;

    QMenu menu(this);

    const QUrl linkUrl = req->linkUrl();
    if (linkUrl.isValid() && !linkUrl.isEmpty()) {
        QAction* openNewTab = menu.addAction(tr("Open Link in New Tab"));
        connect(openNewTab, &QAction::triggered, this, [this, linkUrl]() {
            m_mainWindow->createTab(linkUrl);
        });
        menu.addSeparator();
    }

    if (history()->canGoBack())
        menu.addAction(page()->action(QWebEnginePage::Back));
    if (history()->canGoForward())
        menu.addAction(page()->action(QWebEnginePage::Forward));
    menu.addAction(page()->action(QWebEnginePage::Reload));

    menu.exec(event->globalPos());
}
