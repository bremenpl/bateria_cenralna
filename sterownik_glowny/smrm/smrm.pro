#-------------------------------------------------
#
# Project created by QtCreator 2016-11-30T20:06:20
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = smrm

target.path = /home/pi/
INSTALLS += target

TEMPLATE = app

INCLUDEPATH += ../bc_logger/

SOURCES += main.cpp\
        ../bc_logger/cbclogger.cpp \
        mainwindow.cpp \
        csmrm.cpp

HEADERS  += mainwindow.h \
        ../bc_logger/cbclogger.h \
        csmrm.h

FORMS    += mainwindow.ui
