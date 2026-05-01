#include "xcompage.h"
#include "mainwindow.h"
#include "xcomview.h"

#include <QDesktopServices>
#include <QWebEngineSettings>

XComPage::XComPage(MainWindow* mainWindow, QWebEngineProfile* profile,
                   QObject* parent)
    : QWebEnginePage(profile, parent)
    , m_mainWindow(mainWindow)
{
    settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
}

QWebEnginePage* XComPage::createWindow(WebWindowType)
{
    XComView* view = m_mainWindow->createTab(QUrl());
    return view->page();
}

bool XComPage::acceptNavigationRequest(const QUrl& url, NavigationType type,
                                       bool isMainFrame)
{
    if (isMainFrame
        && type == NavigationTypeLinkClicked
        && !isXUrl(url)
        && isXDomain(this->url()))
    {
        QDesktopServices::openUrl(url);
        return false;
    }
    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

bool XComPage::isXDomain(const QUrl& url)
{
    const QString host = url.host();
    return host == QLatin1String("x.com")
        || host.endsWith(QLatin1String(".x.com"))
        || host == QLatin1String("twitter.com")
        || host.endsWith(QLatin1String(".twitter.com"))
        || host == QLatin1String("twimg.com")
        || host.endsWith(QLatin1String(".twimg.com"));
}

bool XComPage::isXUrl(const QUrl& url)
{
    const QString scheme = url.scheme();
    if (scheme == QLatin1String("about") ||
        scheme == QLatin1String("data")  ||
        scheme == QLatin1String("blob"))
        return true;
    return isXDomain(url);
}
