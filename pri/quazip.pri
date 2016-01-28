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
# $Revision: 5721 $:
# $Author: cohen@irascible.com $:
# $Date: 2012-01-03 16:53:58 +0100 (Di, 03. Jan 2012) $
#
#********************************************************************/

HEADERS += \
	src/lib/quazip/crypt.h \
	src/lib/quazip/ioapi.h \
	src/lib/quazip/quazip.h \
	src/lib/quazip/quazipfile.h \
	src/lib/quazip/quazipfileinfo.h \
	src/lib/quazip/quazipnewinfo.h \
	src/lib/quazip/unzip.h \
	src/lib/quazip/zip.h

SOURCES += \
	src/lib/quazip/ioapi.c \
	src/lib/quazip/quazip.cpp \
	src/lib/quazip/quazipfile.cpp \
	src/lib/quazip/quazipnewinfo.cpp \
	src/lib/quazip/unzip.c \
	src/lib/quazip/zip.c 

