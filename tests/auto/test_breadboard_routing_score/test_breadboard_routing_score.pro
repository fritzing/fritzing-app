CONFIG += c++17 console
QT += core

absolute_boost = 1
include($$absolute_path(../../../pri/boostdetect.pri))

INCLUDEPATH += $$absolute_path(../../../src)

SOURCES += \
    test_breadboard_routing_score.cpp \
    ../../../src/autoroute/breadboardroutingscore.cpp

HEADERS += ../../../src/autoroute/breadboardroutingscore.h
