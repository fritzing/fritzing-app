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

LATESTBOOST = 0

# reduce warning messages by checking if variables are defined before using them
defined(boost_root, var) {
    LATESTBOOST = user
    exists($$boost_root) {
        # force absolute: relative would break due to detection versus use folder nest level
        BOOSTPATH = $$absolute_path($$boost_root)
        # prevent possibly existing absolute_boost flag from breaking already absolute path
        absolute_boost = 1
        unset(absolute_boost)
    } else {
        error("requested boost_root $$boost_root does not exist")
    }
} else {
    # message("Boost auto detect is deprecated, please set boost root variable.")
    message("Using fritzing boost detect script.")

    # boost_1_54_0 is buggy
    BOOSTS = 43 44 45 46 47 48 49 50 51 52 53 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99
    for(boost, BOOSTS) {
        exists(../../boost_1_$${boost}_0) {
            LATESTBOOST = $$boost
            BOOSTPATH = ../boost_1_$${boost}_0
        }
    }
}

# do the common configuration after all detection is finished
contains(LATESTBOOST, 0) {
    boost = 99
    qtCompileTest(boost)
    config_boost {
        !build_pass:message("using installed Boost library")
    } else {
        message("Boost 1.54 has a bug in a function that Fritzing uses, so download or install some other version")
        error("Easiest to copy the Boost library to ..., so that you have .../boost_1_xx_0")
    }
} else {
    defined(BOOSTPATH, var) {
        defined(absolute_boost, var) {
            BOOSTPATH = $$absolute_path(../$${BOOSTPATH})
        }
        message("using $${BOOSTPATH} boost environment")
        INCLUDEPATH += $$BOOSTPATH
    } else {
        error("unknown boost environment")
    }
}