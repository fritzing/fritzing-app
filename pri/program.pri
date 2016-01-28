# /*******************************************************************
#
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2007-08 Fritzing
#
# Fritzing is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Fritzing is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.
#
# ********************************************************************
#
# $Revision: 3854 $:
# $Author: cohen@irascible.com $:
# $Date: 2009-12-10 17:42:28 +0100 (Thu, 10 Dec 2009) $
#
#********************************************************************/

HEADERS += \
    src/program/highlighter.h \
    src/program/programtab.h \
    src/program/programwindow.h \
    src/program/syntaxer.h \
    src/program/trienode.h \ 
    src/program/console.h \
    src/program/consolewindow.h \
    src/program/consolesettings.h \
    src/program/platform.h \
    src/program/platformarduino.h \
    src/program/platformpicaxe.h \
    src/program/platformlaunchpad.h
 
SOURCES += \
    src/program/highlighter.cpp \
    src/program/programtab.cpp \
    src/program/programwindow.cpp \
    src/program/syntaxer.cpp \
    src/program/trienode.cpp \
    src/program/console.cpp \
    src/program/consolewindow.cpp \
    src/program/consolesettings.cpp \
    src/program/platform.cpp \
    src/program/platformarduino.cpp \
    src/program/platformpicaxe.cpp \
    src/program/platformlaunchpad.cpp

FORMS += \
    src/program/consolewindow.ui \
    src/program/consolesettings.ui
