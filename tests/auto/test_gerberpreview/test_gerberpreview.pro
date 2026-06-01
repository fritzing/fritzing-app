# /*******************************************************************
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2007-2026 Fritzing
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

QT += core gui widgets testlib svg
CONFIG += testcase c++14
TARGET = test_gerberpreview

INCLUDEPATH += $$absolute_path(../../../src)

HEADERS += \
	../../../src/gerberpreview/excellonparser.h \
	../../../src/gerberpreview/gerberaperture.h \
	../../../src/gerberpreview/gerberdocument.h \
	../../../src/gerberpreview/gerberparser.h \
	../../../src/gerberpreview/gerberrenderer.h \

SOURCES += \
	test_gerberpreview.cpp \
	../../../src/gerberpreview/excellonparser.cpp \
	../../../src/gerberpreview/gerberaperture.cpp \
	../../../src/gerberpreview/gerberdocument.cpp \
	../../../src/gerberpreview/gerberparser.cpp \
	../../../src/gerberpreview/gerberrenderer.cpp \
