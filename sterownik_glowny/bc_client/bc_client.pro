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
        mainwindow.cpp

HEADERS  += mainwindow.h \
        ../bc_logger/cbclogger.h \

FORMS    += mainwindow.ui

TRANSLATIONS    +=  bcclient_en.ts \
                    bcclient_pl.ts




