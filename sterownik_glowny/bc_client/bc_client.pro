#-------------------------------------------------
#
# Project created by QtCreator 2016-12-01T11:24:06
#
#-------------------------------------------------

TARGET = bc_client
TEMPLATE = app

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4)
{
    QT += widgets
}

target.path = /home/pi/
INSTALLS += target

INCLUDEPATH += ../bc_logger/
    keyboard/

SOURCES += main.cpp\
    ../bc_logger/cbclogger.cpp \
    mainwindow.cpp \
    cdevicedialog.cpp \
    cmainmenu.cpp \
    csettingsmenu.cpp \
    cabstractmenu.cpp \
    cbatteriesmenu.cpp \
    keyboard/keyboard.cpp

HEADERS  += mainwindow.h \
    ../bc_logger/cbclogger.h \
    cdevicedialog.h \
    cmainmenu.h \
    csettingsmenu.h \
    types.h \
    cabstractmenu.h \
    cbatteriesmenu.h \
    keyboard/keyboard.h \
    keyboard/ui_keyboard.h

FORMS    += mainwindow.ui \
    cdevicedialog.ui \
    cmainmenu.ui \
    csettingsmenu.ui \
    cabstractmenu.ui \
    cbatteriesmenu.ui \
    keyboard/keyboard.ui

TRANSLATIONS    +=  bcclient_en.ts \
                    bcclient_pl.ts

RESOURCES += \
    resources.qrc






