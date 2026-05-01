#pragma once

#include <QWebEngineView>

class MainWindow;

class XComView : public QWebEngineView
{
    Q_OBJECT
public:
    explicit XComView(MainWindow* mainWindow, QWidget* parent = nullptr);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    MainWindow* m_mainWindow;
};
