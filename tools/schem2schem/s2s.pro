# -------------------------------------------------
# Project created by QtCreator 2011-02-16T09:56:30
# -------------------------------------------------


QT += xml \  
    gui \
    svg \

TARGET = s2s
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += main.cpp \
    s2sapplication.cpp \
    ../../src/utils/textutils.cpp \
   
HEADERS += s2sapplication.h \
    ../../src/utils/textutils.h \

RESOURCES +=  ../../phoenixresources.qrc

win32 {
	DEFINES += _CRT_SECURE_NO_DEPRECATE
}
