#include "xcomprofile.h"

#include <QStandardPaths>
#include <QWebEngineCookieStore>
#include <QRegularExpression>

XComProfile* XComProfile::s_instance = nullptr;

XComProfile* XComProfile::instance()
{
    if (!s_instance)
        s_instance = new XComProfile();
    return s_instance;
}

XComProfile::XComProfile(QObject* parent)
    : QWebEngineProfile(QStringLiteral("XCOM"), parent)
{
    const QString dataPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    setPersistentStoragePath(dataPath);
    setCachePath(dataPath + QStringLiteral("/cache"));
    setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

    QString userAgent = httpUserAgent();
    userAgent.remove(QRegularExpression(QStringLiteral("\\s+QtWebEngine/\\S+")));
    setHttpUserAgent(userAgent);
}

void XComProfile::logout()
{
    cookieStore()->deleteAllCookies();
    clearAllVisitedLinks();
    clearHttpCache();
}
