# /*******************************************************************
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2019 Fritzing
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

LATESTBOOST=0
# First look for an installed boost package

# Loads mkspecs/features/configure.prf file from https://github.com/qt/qtbase.
# This feature file defines test function qtCompileTest() that is used below.
# For details see
# https://doc.qt.io/qt-5/qmake-test-function-reference.html#qtcompiletest-test
load(configure)
# This command tries to build config.tests/boost_exists/main.cpp file that just
# includes one of boost headers. If the build succeeds, config_boost_exists is
# set to true.
qtCompileTest(boost_exists)
# This command tries to build config.tests/boost_not_1_54/main.cpp file that only
# succeeds to compile if boost version is not 1.5.4. If the build succeeds,
# config_boost_not_1_54 is set to true.
qtCompileTest(boost_not_1_54)

config_boost_exists:config_boost_not_1_54 {
    LATESTBOOST = installed
    message("Using installed Boost library")
} else:config_boost_exists:!config_boost_not_1_54 {
    error("Boost 1.54 has a bug in a function that Fritzing uses, so download or install some other version")
} else:!config_boost_exists:!config_boost_not_1_54 {
    message("Boost library is not installed, will look in fritzing-app sibling directories...")
} else {
    error("It seems there is a bug: boost is reported to have version 1.54, though it is not installed")
}


!contains(LATESTBOOST, installed) {
    # Then try to check for existence of each of the
    # boost_1_xx_0 directories located in the same
    # dir as fritzing-app
    BOOSTS =          43 44 45 46 47 48 49 \
             50 51 52 53 54 55 56 57 58 59 \
             60 61 62 63 64 65 66 67 68 69 \
             70 71 72 73 74 75 76 77 78 79 \
             80 81 82 83 84 85 86 87 88 89 \
             90 91 92 93 94 95 96 97 98 99
    for (boost, BOOSTS) {
        exists(../../boost_1_$${boost}_0) {
            LATESTBOOST = $$boost
        }
    }
    contains(LATESTBOOST, 0) {
        message("Boost not found in any of boost_1_xx_0 fritzing-app sibling directories.")
        error("Take a look in the https://github.com/fritzing/fritzing-app/wiki to find out how to install boost properly.")
    } else {
        message("Using boost_1_$${LATESTBOOST}_0")
        INCLUDEPATH += ../boost_1_$${LATESTBOOST}_0
    }
}
