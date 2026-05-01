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
