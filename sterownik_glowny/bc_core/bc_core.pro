QT += core serialport serialbus network
QT -= gui

CONFIG += c++11

TARGET = bc_core

target.path = /home/pi/
INSTALLS += target

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../bc_logger/

SOURCES += main.cpp \
    ../bc_logger/cbclogger.cpp \
    cbcserialthread.cpp

HEADERS += \
    ../bc_logger/cbclogger.h \
    literals.h \
    cbcserialthread.h
