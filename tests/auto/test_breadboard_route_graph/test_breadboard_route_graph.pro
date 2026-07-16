CONFIG += c++17 console
QT += core

absolute_boost = 1
include($$absolute_path(../../../pri/boostdetect.pri))

INCLUDEPATH += $$absolute_path(../../../src)

SOURCES += \
    test_breadboard_route_graph.cpp \
	../../../src/autoroute/breadboardplacementkernel.cpp \
    ../../../src/autoroute/breadboardroutegraphcore.cpp \
    ../../../src/autoroute/breadboardroutingscore.cpp

HEADERS += \
	../../../src/autoroute/breadboardplacementkernel.h \
    ../../../src/autoroute/breadboardroutegraphcore.h \
    ../../../src/autoroute/breadboardroutingscore.h
