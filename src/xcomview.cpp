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

#include "xcomview.h"
#include "xcompage.h"
#include "xcomprofile.h"
#include "mainwindow.h"

#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QMenu>
#include <QWebEngineContextMenuRequest>
#include <QWebEngineHistory>
#include <QWheelEvent>

static const QString MENU_STYLE = QStringLiteral(
    "QMenu {"
    "  background-color: #0f0f24;"
    "  border: 1px solid #252550;"
    "  border-radius: 8px;"
    "  padding: 4px 0px;"
    "  color: #c0c0f0;"
    "}"
    "QMenu::item {"
    "  padding: 7px 20px 7px 14px;"
    "  border-radius: 5px;"
    "  margin: 1px 4px;"
    "  font-size: 13px;"
    "}"
    "QMenu::item:selected {"
    "  background-color: #1e1e42;"
    "  color: #ffffff;"
    "}"
    "QMenu::item:disabled {"
    "  color: #404070;"
    "}"
    "QMenu::separator {"
    "  height: 1px;"
    "  background: #252550;"
    "  margin: 3px 8px;"
    "}"
);

XComView::XComView(MainWindow* mainWindow, QWidget* parent)
    : QWebEngineView(parent)
    , m_mainWindow(mainWindow)
{
    auto* p = new XComPage(mainWindow, XComProfile::instance(), this);
    setPage(p);
}

bool XComView::event(QEvent* e)
{
    if (!m_wheelFilterInstalled) {
        if (QWidget* proxy = focusProxy()) {
            proxy->installEventFilter(this);
            m_wheelFilterInstalled = true;
        }
    }
    return QWebEngineView::event(e);
}

bool XComView::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Wheel && !m_forwardingWheel) {
        auto* wheel = static_cast<QWheelEvent*>(event);

        if (wheel->modifiers() & Qt::ControlModifier) {
            const int delta = wheel->angleDelta().y();
            if (delta != 0)
                setZoomFactor(qBound(0.25, zoomFactor() + (delta > 0 ? 0.1 : -0.1), 5.0));
            return true;
        }

        const qreal speed = m_mainWindow->scrollSpeed();
        if (!qFuzzyCompare(speed, 1.0)) {
            QWheelEvent scaled(wheel->position(), wheel->globalPosition(),
                                wheel->pixelDelta() * speed, wheel->angleDelta() * speed,
                                wheel->buttons(), wheel->modifiers(), wheel->phase(),
                                wheel->inverted(), wheel->source());
            m_forwardingWheel = true;
            QCoreApplication::sendEvent(watched, &scaled);
            m_forwardingWheel = false;
            return true;
        }
    }
    return QWebEngineView::eventFilter(watched, event);
}

void XComView::contextMenuEvent(QContextMenuEvent* event)
{
    const auto* req = lastContextMenuRequest();
    if (!req) return;

    const QUrl linkUrl = req->linkUrl();
    const QPoint pos = event->pos();
    const QPoint globalPos = event->globalPos();

    const QString js = QStringLiteral(
        "(function(){"
        "var el=document.elementFromPoint(%1,%2);"
        "while(el&&el.tagName!=='ARTICLE'){el=el.parentElement;}"
        "if(!el)return '';"
        "var links=el.querySelectorAll('a[href]');"
        "for(var i=0;i<links.length;i++){"
        "var h=links[i].getAttribute('href');"
        "if(/\\/[^\\/]+\\/status\\/\\d+/.test(h))return h;"
        "}return '';})()").arg(pos.x()).arg(pos.y());

    page()->runJavaScript(js, 0, [this, linkUrl, globalPos](const QVariant& result) {
        auto* menu = new QMenu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->setStyleSheet(MENU_STYLE);

        bool hasCustom = false;

        const QString postHref = result.toString().trimmed();
        if (!postHref.isEmpty()) {
            const QUrl postUrl = postHref.startsWith('/')
                ? QUrl(QStringLiteral("https://x.com") + postHref)
                : QUrl(postHref);
            QAction* act = menu->addAction(tr("Open Post in New Tab"));
            connect(act, &QAction::triggered, this, [this, postUrl]() {
                m_mainWindow->createTab(postUrl);
            });
            hasCustom = true;
        }

        if (linkUrl.isValid() && !linkUrl.isEmpty()) {
            QAction* act = menu->addAction(tr("Open Link in New Tab"));
            connect(act, &QAction::triggered, this, [this, linkUrl]() {
                m_mainWindow->createTab(linkUrl);
            });
            hasCustom = true;
        }

        if (hasCustom)
            menu->addSeparator();

        if (history()->canGoBack())
            menu->addAction(page()->action(QWebEnginePage::Back));
        if (history()->canGoForward())
            menu->addAction(page()->action(QWebEnginePage::Forward));
        menu->addAction(page()->action(QWebEnginePage::Reload));

        menu->exec(globalPos);
    });
}
