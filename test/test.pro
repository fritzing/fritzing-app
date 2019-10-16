include(../pri/boostdetect.pri)
include(../pri/common.pri)

TEMPLATE = app

CONFIG   += console
#CONFIG   -= app_bundle
#CONFIG   -= qt

QT += concurrent core gui network printsupport serialport sql svg widgets xml

SOURCES += main.cpp

#LIBS += -lunittest++ -L$$TOP_OUT_PWD/src/libs -lmyapp

