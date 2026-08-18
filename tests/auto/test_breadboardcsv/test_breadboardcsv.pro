# /*******************************************************************
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2026 Fritzing
# Fritzing is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# Fritzing is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with Fritzing. If not, see <http://www.gnu.org/licenses/>.
# ********************************************************************/

TEMPLATE = app
TARGET = test_breadboardcsv

CONFIG += c++17 console testcase
CONFIG -= app_bundle

absolute_boost = 1
include($$absolute_path(../../../pri/boostdetect.pri))

QT += core sql
QT -= gui

macx:QMAKE_CXXFLAGS += -include $$PWD/qt_apple_arm_compat.h

HEADERS += qt_apple_arm_compat.h

INCLUDEPATH += $$absolute_path(../../../src)

DEFINES += BREADBOARDCSV_TEST_LIBRARIES_ROOT=\\\"$$absolute_path(../../../resources/breadboardcsv/librepcb)\\\"

SOURCES += test_breadboardcsv.cpp \
    ../../../src/mainwindow/breadboardcoordinate.cpp \
    ../../../src/mainwindow/breadboardcsvdipfootprintresolver.cpp \
    ../../../src/mainwindow/breadboardcsvdipphysicalresolver.cpp \
    ../../../src/mainwindow/breadboardcsvlibrepcbprovider.cpp
