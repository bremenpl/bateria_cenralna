QT += core serialport serialbus network
QT -= gui

CONFIG += c++11

TARGET = bc_core

target.path = /home/pi/
INSTALLS += target

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../bc_logger/ \
               ../smrm/

SOURCES += main.cpp \
    ../bc_logger/cbclogger.cpp \
    ../smrm/csmrm.cpp \
    cbcserialthread.cpp \
    cbcmain.cpp \
    cbctcpserver.cpp \
    cbcclientthread.cpp \
    cbclcthread.cpp \
    cbclc.cpp \
    cbcslavedevice.cpp \
    cbcrc.cpp

HEADERS += \
    ../bc_logger/cbclogger.h \
    ../smrm/csmrm.h \
    literals.h \
    cbcserialthread.h \
    cbcmain.h \
    cbctcpserver.h \
    cbcclientthread.h \
    cbclcthread.h \
    cbclc.h \
    cbcslavedevice.h \
    cbcrc.h
