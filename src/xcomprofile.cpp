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
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QRegularExpression>

namespace
{

    const char ES2023_ARRAY_POLYFILLS_JS[] = R"js(
(function(){
    if (typeof Array.prototype.toSorted !== 'function') {
        Array.prototype.toSorted = function(compareFn) {
            return this.slice().sort(compareFn);
        };
    }
    if (typeof Array.prototype.toReversed !== 'function') {
        Array.prototype.toReversed = function() {
            return this.slice().reverse();
        };
    }
    if (typeof Array.prototype.toSpliced !== 'function') {
        Array.prototype.toSpliced = function(start, deleteCount) {
            var items = Array.prototype.slice.call(arguments, 2);
            var copy = this.slice();
            copy.splice.apply(copy, [start, deleteCount].concat(items));
            return copy;
        };
    }
    if (typeof Array.prototype.with !== 'function') {
        Array.prototype.with = function(index, value) {
            var copy = this.slice();
            copy[index < 0 ? copy.length + index : index] = value;
            return copy;
        };
    }
})();
)js";

    const char VIEWPORT_UNIT_FALLBACK_CSS[] = R"js(
(function(){
    if(document.getElementById('xcom-viewport-unit-fallback'))return;
    var s=document.createElement('style');
    s.id='xcom-viewport-unit-fallback';
    s.textContent=
        '.min-h-dvh{min-height:100vh!important}'+
        '.h-dvh{height:100vh!important}'+
        '.max-h-dvh{max-height:100vh!important}'+
        '.min-h-svh{min-height:100vh!important}'+
        '.h-svh{height:100vh!important}'+
        '.max-h-svh{max-height:100vh!important}'+
        '.min-h-lvh{min-height:100vh!important}'+
        '.h-lvh{height:100vh!important}'+
        '.max-h-lvh{max-height:100vh!important}'+
        '.min-w-dvw{min-width:100vw!important}'+
        '.w-dvw{width:100vw!important}'+
        '.max-w-dvw{max-width:100vw!important}';
    if(document.head)document.head.appendChild(s);
})();
)js";

    const char VIEW_TRANSITION_FALLBACK_JS[] = R"js(
(function(){
    if (typeof document.startViewTransition === 'function') return;
    document.startViewTransition = function(callback) {
        var updateResult;
        try {
            updateResult = typeof callback === 'function' ? callback() : undefined;
        } catch (e) {
            updateResult = Promise.reject(e);
        }
        var done = Promise.resolve(updateResult);
        return {
            ready: done,
            updateCallbackDone: done,
            finished: done,
            skipTransition: function() {}
        };
    };
})();
)js";

    const char CONTAINER_QUERY_FALLBACK_CSS[] = R"js(
(function(){
    if(document.getElementById('xcom-container-query-fallback'))return;
    var s=document.createElement('style');
    s.id='xcom-container-query-fallback';
    s.textContent=
        '.narrow\\:inset-0{inset:0!important}'+
        '.narrow\\:m-auto{margin:auto!important}'+
        '.narrow\\:h-fit{height:fit-content!important}'+
        '.narrow\\:w-\\[700px\\]{width:700px!important}'+
        '.narrow\\:max-w-full{max-width:100%!important}'+
        '.narrow\\:overflow-hidden{overflow:hidden!important}'+
        '.narrow\\:rounded-lg{border-radius:0.5rem!important}'+
        '.narrow\\:border{border-width:1px!important;border-style:solid!important}';
    if(document.head)document.head.appendChild(s);
})();
)js";

    const char CHROME_STRIP_JS[] = R"js(
(function(){
    if(document.getElementById('xcom-chrome-strip'))return;
    var style=document.createElement('style');
    style.id='xcom-chrome-strip';
    style.textContent=
        'nav[aria-label="Primary"]{display:none!important}'+
        'div[aria-label="Trending"]{display:none!important}'+
        'div[aria-label="Who to follow"]{display:none!important}';
    if(document.head)document.head.appendChild(style);

    function hideCard(el){
        var cur=el, depth=0;
        while(cur && depth<6){
            if(cur.hasAttribute && cur.hasAttribute('aria-label')){ cur.style.display='none'; return; }
            cur=cur.parentElement; depth++;
        }
    }

    function sweepSidebar(){
        var sidebar=document.querySelector('[data-testid="sidebarColumn"]');
        if(!sidebar) return;
        sidebar.querySelectorAll('a[href*="premium_sign_up"]').forEach(hideCard);
    }

    function attachObserver(){
        var sidebar=document.querySelector('[data-testid="sidebarColumn"]');
        if(!sidebar){ setTimeout(attachObserver,500); return; }
        sweepSidebar();
        new MutationObserver(sweepSidebar).observe(sidebar,{childList:true,subtree:true});
    }
    attachObserver();
})();
)js";
}

XComProfile *XComProfile::s_instance = nullptr;

XComProfile *XComProfile::instance()
{
    if (!s_instance)
        s_instance = new XComProfile();
    return s_instance;
}

void XComProfile::shutdown()
{
    delete s_instance;
    s_instance = nullptr;
}

XComProfile::XComProfile(QObject *parent)
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

    installEs2023Polyfills();
    installViewportUnitFallback();
    installViewTransitionFallback();
    installContainerQueryFallback();
    installChromeStripping();
}

void XComProfile::installEs2023Polyfills()
{
    QWebEngineScript script;
    script.setName(QStringLiteral("xcom-es2023-array-polyfills"));
    script.setSourceCode(QString::fromUtf8(ES2023_ARRAY_POLYFILLS_JS));
    script.setInjectionPoint(QWebEngineScript::DocumentCreation);
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setRunsOnSubFrames(true);
    scripts()->insert(script);
}

void XComProfile::installViewportUnitFallback()
{
    QWebEngineScript script;
    script.setName(QStringLiteral("xcom-viewport-unit-fallback"));
    script.setSourceCode(QString::fromUtf8(VIEWPORT_UNIT_FALLBACK_CSS));
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setRunsOnSubFrames(true);
    scripts()->insert(script);
}

void XComProfile::installViewTransitionFallback()
{
    QWebEngineScript script;
    script.setName(QStringLiteral("xcom-view-transition-fallback"));
    script.setSourceCode(QString::fromUtf8(VIEW_TRANSITION_FALLBACK_JS));
    script.setInjectionPoint(QWebEngineScript::DocumentCreation);
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setRunsOnSubFrames(true);
    scripts()->insert(script);
}

void XComProfile::installContainerQueryFallback()
{
    QWebEngineScript script;
    script.setName(QStringLiteral("xcom-container-query-fallback"));
    script.setSourceCode(QString::fromUtf8(CONTAINER_QUERY_FALLBACK_CSS));
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setRunsOnSubFrames(true);
    scripts()->insert(script);
}

void XComProfile::installChromeStripping()
{
    QWebEngineScript script;
    script.setName(QStringLiteral("xcom-chrome-strip"));
    script.setSourceCode(QString::fromUtf8(CHROME_STRIP_JS));
    script.setInjectionPoint(QWebEngineScript::DocumentReady);
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setRunsOnSubFrames(false);
    scripts()->insert(script);
}

void XComProfile::logout()
{
    cookieStore()->deleteAllCookies();
    clearAllVisitedLinks();
    clearHttpCache();
}
