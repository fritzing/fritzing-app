# /*******************************************************************
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2007-16 Fritzing
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
# ********************************************************************
# $Revision: 6796 $:
# $Author: irascibl@gmail.com $:
# $Date: 2013-01-12 07:45:08 +0100 (Sa, 12. Jan 2013) $
# ********************************************************************/

# boost_1_54_0 is buggy
BOOSTS = 43 44 45 46 47 48 49 50 51 52 53 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99
LATESTBOOST = 0
for(boost, BOOSTS) {
    exists(../src/lib/boost_1_$${boost}_0) {
        LATESTBOOST = $$boost
    }
}

contains(LATESTBOOST, 0) {
    qtCompileTest(boost)
    config_boost {
        LATESTBOOST = installed
        message("using installed Boost library")
    } else {
        message("Boost 1.54 has a bug in a function that Fritzing uses, so download or install some other version")
        error("Easiest to copy the Boost library to .../src/lib/, so that you have .../src/lib/boost_1_xx_0")
    }
}

!contains(LATESTBOOST, installed) {
    message("using Boost from src/lib/boost_1_$${LATESTBOOST}_0")
    INCLUDEPATH += src/lib/boost_1_$${LATESTBOOST}_0
}

HEADERS += \
src/utils/abstractstatesbutton.h \
src/utils/autoclosemessagebox.h \
src/utils/bendpointaction.h \
src/utils/bezier.h \
src/utils/bezierdisplay.h \
src/utils/boundedregexpvalidator.h \
src/utils/bundler.h \
src/utils/clickablelabel.h \
src/utils/cursormaster.h \
src/utils/expandinglabel.h \
src/utils/familypropertycombobox.h \
src/utils/fileprogressdialog.h \
src/utils/flineedit.h \
src/utils/fmessagebox.h \
src/utils/focusoutcombobox.h \
src/utils/fsizegrip.h \
src/utils/lockmanager.h \
src/utils/misc.h \
src/utils/resizehandle.h \
src/utils/folderutils.h \
src/utils/graphicsutils.h \
src/utils/graphutils.h \
src/utils/ratsnestcolors.h \
src/utils/schematicrectconstants.h \
src/utils/s2s.h \
src/utils/textutils.h \
src/utils/zoomslider.h 
 
SOURCES += \
src/utils/autoclosemessagebox.cpp \
src/utils/bendpointaction.cpp \
src/utils/bezier.cpp \
src/utils/bezierdisplay.cpp \
src/utils/clickablelabel.cpp \
src/utils/cursormaster.cpp \
src/utils/expandinglabel.cpp \
src/utils/fileprogressdialog.cpp \
src/utils/flineedit.cpp \
src/utils/fmessagebox.cpp \
src/utils/focusoutcombobox.cpp \
src/utils/fsizegrip.cpp \
src/utils/lockmanager.cpp \
src/utils/misc.cpp \
src/utils/resizehandle.cpp \
src/utils/folderutils.cpp \
src/utils/graphicsutils.cpp \
src/utils/graphutils.cpp \
src/utils/ratsnestcolors.cpp \
src/utils/schematicrectconstants.cpp \
src/utils/s2s.cpp \
src/utils/textutils.cpp \
src/utils/zoomslider.cpp 
