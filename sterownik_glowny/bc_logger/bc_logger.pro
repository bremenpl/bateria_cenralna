QT += core
QT -= gui

CONFIG += c++11

TARGET = bc_logger

target.path = /home/pi/
INSTALLS += target

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    cbclogger.cpp

HEADERS += \
    cbclogger.h

