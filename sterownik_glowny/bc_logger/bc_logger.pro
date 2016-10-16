QT += core
QT -= gui

CONFIG += c++11

TARGET = bc_logger
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    cbclogger.cpp

HEADERS += \
    cbclogger.h
