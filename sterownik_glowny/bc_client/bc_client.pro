#-------------------------------------------------
#
# Project created by QtCreator 2016-12-01T11:24:06
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = bc_client

target.path = /home/pi/
INSTALLS += target

TEMPLATE = app

INCLUDEPATH += ../bc_logger/

SOURCES += main.cpp\
        ../bc_logger/cbclogger.cpp \
        mainwindow.cpp \
    cdevicedialog.cpp \
    cmainmenu.cpp \
    csettingsmenu.cpp \
    cabstractmenu.cpp \
    cbatteriesmenu.cpp

HEADERS  += mainwindow.h \
        ../bc_logger/cbclogger.h \
    cdevicedialog.h \
    cmainmenu.h \
    csettingsmenu.h \
    types.h \
    cabstractmenu.h \
    cbatteriesmenu.h

FORMS    += mainwindow.ui \
    cdevicedialog.ui \
    cmainmenu.ui \
    csettingsmenu.ui \
    cabstractmenu.ui \
    cbatteriesmenu.ui

TRANSLATIONS    +=  bcclient_en.ts \
                    bcclient_pl.ts

RESOURCES += \
    resources.qrc




