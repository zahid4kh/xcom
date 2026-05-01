#include <QApplication>
#include <QCoreApplication>

#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("XCOM"));
    app.setOrganizationName(QStringLiteral("XCOM"));
    app.setApplicationVersion(QStringLiteral("1.0"));

    MainWindow w;
    w.show();

    return app.exec();
}
