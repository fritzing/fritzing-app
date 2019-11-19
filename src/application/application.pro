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

include($$top_dir/pri/common.pri)

HEADERS += $$files(*.h)
SOURCES += $$files(*.cpp)

LIBGIT2LIB = $$_PRO_FILE_PWD_/../../../libgit2/build

INCLUDEPATH += ..

FLIBS = referencemodel help program infoview version mainwindow partsbinpalette eagle dock sketch autoroute partseditor svg connectors model items kitchensink dialogs utils
for(libname, FLIBS) {
    LIBS += -L../$${libname}/$$DESTDIR -l$${libname}
}
LIBS += -L../lib/qtsysteminfo/$$DESTDIR -lqtsysteminfo
include($$top_dir/pri/libquazip.pri)

# Linux specific
LIBS += $$LIBGIT2LIB/libgit2.a -lssl -lcrypto

RESOURCES += ../../fritzingresources.qrc

TEMPLATE = app
TARGET= Fritzing

