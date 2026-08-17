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

#pragma once

#include <QByteArray>
#include <QIcon>
#include <QMainWindow>
#include <QPoint>

class QTabWidget;
class QToolBar;
class QAction;
class QCloseEvent;
class QSystemTrayIcon;
class QLabel;
class XComView;
class ResourcePanel;
class WidgetHeader;

enum class WindowMode
{
    Normal,
    Widget
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    XComView *createTab(const QUrl &url);
    XComView *currentView() const;
    WindowMode mode() const { return m_mode; }
    qreal scrollSpeed() const { return m_scrollSpeed; }

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onTabCloseRequested(int index);
    void onCurrentTabChanged(int index);
    void onNewTab();
    void onHome();
    void onLogout();
    void enterWidgetMode();
    void exitWidgetMode();

private:
    void setupMenu();
    void setupToolbar();
    void setupNavRail();
    void setupZoomControls();
    void setupTabs();
    void setupTray();
    void triggerNavLink(const QString &selector);
    void navigateTo(const QUrl &url);
    void updateNavActions();
    void updateModeUi();
    void injectWidgetCss();
    void removeWidgetCss();
    void applyWidgetMask();
    void setAllViewsBackground(const QColor &color);
    void saveSettings();
    void loadSettings();
    void updateNotificationsBadge(int count);
    void pollNotificationsBadge();

    // Widgets
    QTabWidget *m_tabs = nullptr;
    QToolBar *m_toolbar = nullptr;
    QToolBar *m_navRail = nullptr;
    QAction *m_back = nullptr;
    QAction *m_forward = nullptr;
    ResourcePanel *m_resourcePanel = nullptr;
    QLabel *m_cpuMiniLabel = nullptr;
    QLabel *m_memMiniLabel = nullptr;
    QLabel *m_diskMiniLabel = nullptr;
    QLabel *m_zoomLabel = nullptr;
    QAction *m_notificationsAction = nullptr;
    QIcon m_bellBaseIcon;
    WidgetHeader *m_widgetHeader = nullptr;
    QSystemTrayIcon *m_tray = nullptr;
    QAction *m_widgetModeAction = nullptr;
    QAction *m_trayModeAction = nullptr;

    qreal m_scrollSpeed = 1.0;

    WindowMode m_mode = WindowMode::Normal;
    QByteArray m_normalGeometry;
    QPoint m_widgetPos;
    bool m_hasWidgetPos = false;
    bool m_inFullscreen = false;
    Qt::WindowStates m_prevWindowState = Qt::WindowNoState;
};
