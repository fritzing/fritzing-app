# -------------------------------------------------
# Project created by QtCreator 2011-02-16T09:56:30
# -------------------------------------------------

FRITZING_SRC = ../../src


INCLUDEPATH += $$FRITZING_SRC

QT += xml \
    xmlpatterns \    
    network \
    gui \
    # script \

TARGET = brd2svg
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += main.cpp \
    brdapplication.cpp \
    miscutils.cpp \
    $$FRITZING_SRC/utils/textutils.cpp \
    $$FRITZING_SRC/utils/graphicsutils.cpp  \
    $$FRITZING_SRC/utils/schematicrectconstants.cpp  \
    $$FRITZING_SRC/svg/svgfilesplitter.cpp  \
    $$FRITZING_SRC/svg/svgpathlexer.cpp  \    
    $$FRITZING_SRC/svg/svgpathparser.cpp \
    $$FRITZING_SRC/svg/svgpathgrammar.cpp \
    $$FRITZING_SRC/svg/svgpathrunner.cpp  
   
HEADERS += brdapplication.h \
    miscutils.h \
    $$FRITZING_SRC/utils/textutils.h \
    $$FRITZING_SRC/utils/misc.h \
    $$FRITZING_SRC/utils/graphicsutils.h  \
    $$FRITZING_SRC/utils/schematicrectconstants.h  \
    $$FRITZING_SRC/svg/svgfilesplitter.h  \
    $$FRITZING_SRC/svg/svgpathlexer.h  \    
    $$FRITZING_SRC/svg/svgpathparser.h  \
    $$FRITZING_SRC/svg/svgpathgrammar_p.h  \
    $$FRITZING_SRC/svg/svgpathrunner.h  
     
RESOURCES +=  $$FRITZING_SRC/../phoenixresources.qrc

win32 {
	DEFINES += _CRT_SECURE_NO_DEPRECATE
}
