#include <QApplication>
#include <QCoreApplication>
#include <QIcon>

#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("XCOM"));
    app.setOrganizationName(QStringLiteral("XCOM"));
    app.setApplicationVersion(QStringLiteral("1.0"));

    QIcon icon;
    icon.addFile(QStringLiteral(":/icons/appicon_512x512.png"));
    icon.addFile(QStringLiteral(":/icons/appicon_1024x1024.png"));
    app.setWindowIcon(icon);

    MainWindow w;
    w.show();

    return app.exec();
}
