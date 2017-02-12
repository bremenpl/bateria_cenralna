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

INCLUDEPATH += ../bc_logger/ \
    keyboard/ \
    ../bc_core/

SOURCES += main.cpp\
    ../bc_logger/cbclogger.cpp \
    ../bc_core/cbclc.cpp \
    ../bc_core/cbcrc.cpp \
    ../bc_core/cbcslavedevice.cpp \
    ../bc_core/cbctcpserver.cpp \
    ../bc_core/cbcclientthread.cpp \
    mainwindow.cpp \
    cdevicedialog.cpp \
    cmainmenu.cpp \
    csettingsmenu.cpp \
    cabstractmenu.cpp \
    cbatteriesmenu.cpp \
    keyboard/keyboard.cpp \
    citemsmenu.cpp \
    citemslcmenu.cpp \
    cprelcpanel.cpp \
    citemsrcmenu.cpp \
    cabstractdevice.cpp \
    crcdevice.cpp \
    cpadevice.cpp \
    citemsbatmenu.cpp \
    cbatdevice.cpp \
    citemscharmenu.cpp \
    cchardevice.cpp \
    ctcpparser.cpp

HEADERS  += mainwindow.h \
    ../bc_logger/cbclogger.h \
    ../bc_core/cbclc.h \
    ../bc_core/cbcrc.h \
    ../bc_core/cbcslavedevice.h \
    ../bc_core/cbctcpserver.h \
    ../bc_core/cbcclientthread.h \
    cdevicedialog.h \
    cmainmenu.h \
    csettingsmenu.h \
    types.h \
    cabstractmenu.h \
    cbatteriesmenu.h \
    keyboard/keyboard.h \
    keyboard/ui_keyboard.h \
    citemsmenu.h \
    citemslcmenu.h \
    cprelcpanel.h \
    citemsrcmenu.h \
    cabstractdevice.h \
    crcdevice.h \
    cpadevice.h \
    citemsbatmenu.h \
    cbatdevice.h \
    citemscharmenu.h \
    cchardevice.h \
    ctcpparser.h

FORMS    += mainwindow.ui \
    cdevicedialog.ui \
    cmainmenu.ui \
    csettingsmenu.ui \
    cabstractmenu.ui \
    cbatteriesmenu.ui \
    keyboard/keyboard.ui \
    citemsmenu.ui \
    cprelcpanel.ui \
    cabstractdevice.ui

TRANSLATIONS    +=  bcclient_en.ts \
                    bcclient_pl.ts

RESOURCES += \
    resources.qrc






