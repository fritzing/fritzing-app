# /*******************************************************************
#
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2007-19 Fritzing
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

lessThan(QT_MAJOR_VERSION, 5) {
    error(Fritzing does not build with Qt 4 or earlier)
}

CONFIG += debug_and_release

unix:!macx {
    CONFIG += link_pkgconfig
}

load(configure)

win32 {
# release build using msvc 2010 needs to use Multi-threaded (/MT) for the code generation/runtime library option
# release build using msvc 2010 needs to add msvcrt.lib;%(IgnoreSpecificDefaultLibraries) to the linker/no default libraries option
    CONFIG -= embed_manifest_exe
    INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtZlib
    DEFINES += _CRT_SECURE_NO_DEPRECATE
    DEFINES += _WINDOWS
    RELEASE_SCRIPT = $$(RELEASE_SCRIPT)    # environment variable set from release script

    message("target arch: $${QMAKE_TARGET.arch}")
    contains(QMAKE_TARGET.arch, x86_64) {
        RELDIR = ../release64
        DEBDIR = ../debug64
        DEFINES += WIN64
    } else {
        RELDIR = ../release32
        DEBDIR = ../debug32
    }

    Release:DESTDIR = $${RELDIR}
    Release:OBJECTS_DIR = $${RELDIR}
    Release:MOC_DIR = $${RELDIR}
    Release:RCC_DIR = $${RELDIR}
    Release:UI_DIR = $${RELDIR}

    Debug:DESTDIR = $${DEBDIR}
    Debug:OBJECTS_DIR = $${DEBDIR}
    Debug:MOC_DIR = $${DEBDIR}
    Debug:RCC_DIR = $${DEBDIR}
    Debug:UI_DIR = $${DEBDIR}
}
macx {
    RELDIR = ../release64
    DEBDIR = ../debug64
    Release:DESTDIR = $${RELDIR}
    Release:OBJECTS_DIR = $${RELDIR}
    Release:MOC_DIR = $${RELDIR}
    Release:RCC_DIR = $${RELDIR}
    Release:UI_DIR = $${RELDIR}

    Debug:DESTDIR = $${DEBDIR}
    Debug:OBJECTS_DIR = $${DEBDIR}
    Debug:MOC_DIR = $${DEBDIR}
    Debug:RCC_DIR = $${DEBDIR}
    Debug:UI_DIR = $${DEBDIR}

    #QMAKE_MAC_SDK = macosx10.11            # uncomment/adapt for your version of OSX
    CONFIG += x86_64 # x86 ppc
    QMAKE_INFO_PLIST = FritzingInfo.plist
    #DEFINES += QT_NO_DEBUG                # uncomment this for xcode
    LIBS += -lz
    LIBS += /usr/lib/libz.dylib
    LIBS += /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation
    LIBS += /System/Library/Frameworks/Carbon.framework/Carbon
    LIBS += /System/Library/Frameworks/IOKit.framework/Versions/A/IOKit
    LIBS += -liconv
}
unix {
    !macx { # unix is defined on mac
        HARDWARE_PLATFORM = $$system(uname -m)
        contains(HARDWARE_PLATFORM, x86_64) {
            DEFINES += LINUX_64
        } else {
            DEFINES += LINUX_32
        }
        !contains(DEFINES, QUAZIP_INSTALLED) {
            LIBS += -lz
        }
    }

    isEmpty(PREFIX) {
        PREFIX = /usr
    }
    BINDIR = $$PREFIX/bin
    DATADIR = $$PREFIX/share
    PKGDATADIR = $$DATADIR/fritzing

    DEFINES += DATADIR=\\\"$$DATADIR\\\" PKGDATADIR=\\\"$$PKGDATADIR\\\"

    target.path = $$BINDIR

    desktop.path = $$DATADIR/applications
    desktop.files += org.fritzing.Fritzing.desktop

    appdata.path = $$DATADIR/metainfo
    appdata.files += org.fritzing.Fritzing.appdata.xml

    mimedb.path = $$DATADIR/mime/packages
    mimedb.files += resources/system_icons/linux/fritzing.xml

    manpage.path = $$DATADIR/man/man1
    manpage.files += Fritzing.1

    icon.path = $$DATADIR/pixmaps
    icon.extra = install -D -m 0644 $$PWD/resources/images/fritzing_icon.png $(INSTALL_ROOT)$$DATADIR/pixmaps/fritzing.png

    parts.path = $$PKGDATADIR
    parts.files += parts

    help.path = $$PKGDATADIR
    help.files += help

    sketches.path = $$PKGDATADIR
    sketches.files += sketches

    bins.path = $$PKGDATADIR
    bins.files += bins

    translations.path = $$PKGDATADIR/translations
    isEmpty(LINGUAS) {
        translations.files += $$system(find $$PWD/translations -name "*.qm" -size +128c)
    } else {
        for(lang, LINGUAS):translations.files += $$system(find $$PWD/translations -name "fritzing_$${lang}.qm" -size +128c)
        isEmpty(translations.files):error("No translations found for $$LINGUAS")
    }

    syntax.path = $$PKGDATADIR/translations/syntax
    syntax.files += translations/syntax/*.xml

    INSTALLS += target desktop appdata mimedb manpage icon parts sketches bins translations syntax help
}

ICON = resources/system_icons/macosx/fritzing_icon.icns

macx {
    FILE_ICONS.files = resources/system_icons/macosx/mac_fz_icon.icns resources/system_icons/macosx/mac_fzz_icon.icns resources/system_icons/macosx/mac_fzb_icon.icns resources/system_icons/macosx/mac_fzp_icon.icns resources/system_icons/macosx/mac_fzm_icon.icns resources/system_icons/macosx/mac_fzpz_icon.icns
    FILE_ICONS.path = Contents/Resources
    QMAKE_BUNDLE_DATA += FILE_ICONS
}

QT += concurrent core gui network printsupport serialport sql svg widgets xml

RC_FILE = fritzing.rc
RESOURCES += phoenixresources.qrc

# Fritzing is using libgit2 since version 0.9.3
packagesExist(libgit2) {
    message("always true on win32. leads to build problems")

    PKGCONFIG += libgit2
    win32 {
        include(pri/libgit2detect.pri)
        message($$PKGCONFIG)
    }
} else {
    include(pri/libgit2detect.pri)
}

include(pri/boostdetect.pri)

include(pri/kitchensink.pri)
include(pri/mainwindow.pri)
include(pri/partsbinpalette.pri)
include(pri/partseditor.pri)
include(pri/referencemodel.pri)
include(pri/svg.pri)
include(pri/help.pri)
include(pri/version.pri)
include(pri/eagle.pri)
include(pri/utils.pri)
include(pri/dock.pri)
include(pri/items.pri)
include(pri/autoroute.pri)
include(pri/dialogs.pri)
include(pri/connectors.pri)
include(pri/infoview.pri)
include(pri/model.pri)
include(pri/sketch.pri)
include(pri/translations.pri)
include(pri/program.pri)
include(pri/qtsysteminfo.pri)

contains(DEFINES, QUAZIP_INSTALLED) {
    INCLUDEPATH += /usr/include/quazip
    LIBS += -lquazip
} else {
    include(pri/quazip.pri)
}

TARGET = Fritzing
TEMPLATE = app

message("libs $$LIBS")
