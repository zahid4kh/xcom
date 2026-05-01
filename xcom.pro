QT += core gui widgets webenginewidgets concurrent

CONFIG += c++20

TARGET   = xcom
TEMPLATE = app

DESTDIR     = build
OBJECTS_DIR = build/.obj
MOC_DIR     = build/.moc
RCC_DIR     = build/.rcc
UI_DIR      = build/.ui

INCLUDEPATH += include

HEADERS += \
    include/mainwindow.h    \
    include/xcompage.h      \
    include/xcomprofile.h   \
    include/xcomview.h      \
    include/resourcepanel.h

SOURCES += \
    main.cpp                  \
    src/mainwindow.cpp        \
    src/xcompage.cpp          \
    src/xcomprofile.cpp       \
    src/xcomview.cpp          \
    src/resourcepanel.cpp

RESOURCES += resources.qrc
