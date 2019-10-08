# /*******************************************************************
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2007-08 Fritzing
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

include(gitversion.pri)

HEADERS += \
	src/version/modfiledialog.h \
	src/version/updatedialog.h \
	src/version/version.h \
	src/version/versionchecker.h \
	src/version/partschecker.h

SOURCES += \
	src/version/modfiledialog.cpp \
	src/version/updatedialog.cpp \
	src/version/version.cpp \
	src/version/versionchecker.cpp \
	src/version/partschecker.cpp

FORMS += \
	src/version/modfiledialog.ui
