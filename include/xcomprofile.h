#pragma once

#include <QWebEngineProfile>

class XComProfile : public QWebEngineProfile
{
    Q_OBJECT
public:
    static XComProfile* instance();
    void logout();

private:
    explicit XComProfile(QObject* parent = nullptr);

    static XComProfile* s_instance;
};
