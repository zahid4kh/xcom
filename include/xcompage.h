#pragma once

#include <QWebEnginePage>

class MainWindow;

class XComPage : public QWebEnginePage
{
    Q_OBJECT
public:
    explicit XComPage(MainWindow* mainWindow, QWebEngineProfile* profile,
                      QObject* parent = nullptr);

protected:
    QWebEnginePage* createWindow(WebWindowType type) override;
    bool acceptNavigationRequest(const QUrl& url, NavigationType type,
                                 bool isMainFrame) override;

private:
    MainWindow* m_mainWindow;

    static bool isXDomain(const QUrl& url);
    static bool isXUrl(const QUrl& url);
};
