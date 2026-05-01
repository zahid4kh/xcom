#pragma once

#include <QMainWindow>

class QTabWidget;
class QToolBar;
class QAction;
class XComView;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    XComView* createTab(const QUrl& url);
    XComView* currentView() const;

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

    QTabWidget* m_tabs    = nullptr;
    QToolBar*   m_toolbar = nullptr;
    QAction*    m_back    = nullptr;
    QAction*    m_forward = nullptr;
};
