# /*******************************************************************
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2017 Fritzing
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

win32 {
    LIBZINCLUDE = "$$_PRO_FILE_PWD_/src/lib/zlib"
    exists($$LIBZINCLUDE/zlib.h) {
        message("found zlib include path at $$LIBZINCLUDE")
    } else {
        message("Fritzing requires zlib")

        error("zlib include path not found in $$LIBZINCLUDE")
    }

    INCLUDEPATH += $$LIBZINCLUDE

    contains(QMAKE_TARGET.arch, x86_64) {
        LIBZLIB = "$$_PRO_FILE_PWD_/src/lib/zlib/build64"
    } else {
        LIBZLIB = "$$_PRO_FILE_PWD_/src/lib/zlib/build32"
    }

    exists($$LIBZLIB/zlib.lib) {
        message("found zlib library in $$LIBZLIB")
    } else {
        error("zlib library not found in $$LIBZLIB")
    }

    LIBS += -L$$LIBZLIB -lzlib
}
unix {
    LIBS += -lz
}
macx {
    LIBS += /usr/lib/libz.dylib
}
