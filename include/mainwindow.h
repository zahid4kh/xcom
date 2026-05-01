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

#include <QMainWindow>

class QTabWidget;
class QToolBar;
class QAction;
class XComView;
class ResourcePanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    XComView* createTab(const QUrl& url);
    XComView* currentView() const;

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onTabCloseRequested(int index);
    void onCurrentTabChanged(int index);
    void onNewTab();
    void onHome();
    void onLogout();

private:
    void setupMenu();
    void setupToolbar();
    void setupTabs();
    void updateNavActions();

    QTabWidget*      m_tabs              = nullptr;
    QToolBar*        m_toolbar           = nullptr;
    QAction*         m_back              = nullptr;
    QAction*         m_forward           = nullptr;
    ResourcePanel*   m_resourcePanel     = nullptr;
    Qt::WindowStates m_prevWindowState   = Qt::WindowNoState;
};
