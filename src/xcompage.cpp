#include "xcompage.h"
#include "mainwindow.h"
#include "xcomview.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QWebEngineSettings>

XComPage::XComPage(MainWindow* mainWindow, QWebEngineProfile* profile,
                   QObject* parent)
    : QWebEnginePage(profile, parent)
    , m_mainWindow(mainWindow)
{
    settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    settings()->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
}

QWebEnginePage* XComPage::createWindow(WebWindowType)
{
    XComView* view = m_mainWindow->createTab(QUrl());
    return view->page();
}

bool XComPage::acceptNavigationRequest(const QUrl& url, NavigationType type,
                                       bool isMainFrame)
{
    if (isMainFrame && isGoogleAuthUrl(url))
        return openGoogleAuthExternally(url);

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

bool XComPage::openGoogleAuthExternally(const QUrl& url)
{
    QDesktopServices::openUrl(url);

    if (!m_googleAuthWarningShown) {
        m_googleAuthWarningShown = true;
        QMessageBox::information(
            m_mainWindow,
            QStringLiteral("Google sign-in"),
            QStringLiteral(
                "Google blocks this sign-in flow inside embedded browsers such as "
                "Qt WebEngine. The Google page was opened in your default browser. "
                "To sign in inside XCOM, use X's email, username, or phone sign-in "
                "flow instead of Google sign-in."));
    }

    return false;
}

bool XComPage::isGoogleAuthUrl(const QUrl& url)
{
    const QString host = url.host();
    if (host == QLatin1String("accounts.google.com")
        || host.endsWith(QLatin1String(".accounts.google.com")))
        return true;

    return host == QLatin1String("google.com")
        || host.endsWith(QLatin1String(".google.com"));
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
