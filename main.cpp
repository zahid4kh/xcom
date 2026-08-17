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

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>

#include "mainwindow.h"
#include "xcomprofile.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("XCOM"));
    app.setOrganizationName(QStringLiteral("XCOM"));
    app.setApplicationVersion(QStringLiteral("1.3.1"));
    app.setDesktopFileName(QStringLiteral("xcom"));

    QIcon icon;
    icon.addFile(QStringLiteral(":/icons/appicon_512x512.png"));
    icon.addFile(QStringLiteral(":/icons/appicon_1024x1024.png"));
    app.setWindowIcon(icon);

    int result;
    {
        MainWindow w;
        w.setObjectName(QStringLiteral("xcom"));
        w.show();

        result = app.exec();
    }
    XComProfile::shutdown();

    return result;
}
