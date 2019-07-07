# ********************************************************************
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

# This is QT project(.pro) file that is used by qmake to convert
# QT project to XCode, Visual studio projects
# (see https://doc.qt.io/qt-5/qmake-platform-notes.html for details)
# or to generate Makefile
# (see https://doc.qt.io/qt-5/qmake-running.html for details).

# The manual for qmake, that includes the list of built-in functions
# (lessThan(), load(), include()) as well as of a list of built-in
# variables (CONFIG, RC_FILE) and the notes on how they are used by
# qmake can be found here:
#   https://doc.qt.io/qt-5/qmake-manual.html

message(Qt version $$[QT_VERSION])

# Starting from 0.9.4 Fritzing requires at least 5.9
# We use equal(QT_MAJOR_VERSION, ...) instead of lessThan(), because
# change of major version number means the change in API.
!equals(QT_MAJOR_VERSION, 5) | !greaterThan(QT_MINOR_VERSION, 9) {
    error("Unsupported Qt version, 5.9+ is required")
}

# The project will be processed three times: one time to produce a
# "meta" Makefile, and two more times to produce a Makefile.Debug and
# a Makefile.Release. Also the build mode could be chosen in VS and
# XCode if this is the case.
CONFIG += debug_and_release

TARGET   = Fritzing
TEMPLATE = app

# Specify the name of resource collection files
# See https://doc.qt.io/qt-5/resources.html for details
RESOURCES += fritzingresources.qrc

# Use pkg-config on UNIXes. See the link below for details.
# This is used to link libgit2.
# http://qt.shoutwiki.com/wiki/Using_pkg-config_with_qmake
unix:CONFIG += link_pkgconfig

# Set directories for Windows and OS X to put different
# files to(see comments for the variables)
win32 {
    message("target arch: $${QMAKE_TARGET.arch}")
    contains(QMAKE_TARGET.arch, x86_64) {
        RELDIR = ../release64
        DEBDIR = ../debug64
        DEFINES += WIN64
    } else {
        RELDIR = ../release32
        DEBDIR = ../debug32
    }
}
macx {
    RELDIR = ../release64
    DEBDIR = ../debug64
}
win32 | macx {
    Release {
        DESTDIR     = $${RELDIR} # Executable(target) file
        OBJECTS_DIR = $${RELDIR} # Object files
        MOC_DIR     = $${RELDIR} # Intermediate MOC files
        RCC_DIR     = $${RELDIR} # Resource compiler output files
        UI_DIR      = $${RELDIR} # Intermediate files from UI compiler
    }
    Debug {
        DESTDIR     = $${DEBDIR} # Executable(target) file
        OBJECTS_DIR = $${DEBDIR} # Object files
        MOC_DIR     = $${DEBDIR} # Intermediate MOC files
        RCC_DIR     = $${DEBDIR} # Resource compiler output files
        UI_DIR      = $${DEBDIR} # Intermediate files from UI compiler
    }
}

# Some magic os-specific libs linking and defines
win32 {
    CONFIG -= embed_manifest_exe
    INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtZlib
    DEFINES += _CRT_SECURE_NO_DEPRECATE
    DEFINES += _WINDOWS
    RELEASE_SCRIPT = $$(RELEASE_SCRIPT)    # environment variable set from release script
}
macx {
    # Only set this for deployment. By default qmake will use sdk version installed on
    # building machine.
    # QMAKE_MAC_SDK = macosx10.11

    # DEFINES += QT_NO_DEBUG # uncomment this for xcode
    CONFIG += x86_64 # x86 ppc
    QMAKE_INFO_PLIST = tools/mac/FritzingInfo.plist
    LIBS += -lz
    LIBS += /usr/lib/libz.dylib
    LIBS += /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation
    LIBS += /System/Library/Frameworks/Carbon.framework/Carbon
    LIBS += /System/Library/Frameworks/IOKit.framework/Versions/A/IOKit
    LIBS += -liconv
}
unix : !macx {
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

# Install application-related files to the
# (by default) /usr/{bin,share,share/fritzing}
# directories on UNIX and Mac OS
unix {
    isEmpty(PREFIX):PREFIX = /usr

    BINDIR = $$PREFIX/bin
    DEFINES += BINDIR=\\\"$$BINDIR\\\"

    DATADIR = $$PREFIX/share
    DEFINES += DATADIR=\\\"$$DATADIR\\\"

    PKGDATADIR = $$PREFIX/share/fritzing
    DEFINES += PKGDATADIR=\\\"$$PKGDATADIR\\\"

    # See the following link to understand the way INSTALLS variable is used:
    # https://doc.qt.io/qt-5/qmake-advanced-usage.html#installing-files
    target.path = $$BINDIR
    INSTALLS += target

    desktop.path = $$DATADIR/applications
    desktop.files += tools/linux/org.fritzing.Fritzing.desktop
    INSTALLS += desktop

    appdata.path = $$DATADIR/metainfo
    appdata.files += tools/linux/org.fritzing.Fritzing.appdata.xml
    INSTALLS += appdata

    mimedb.path = $$DATADIR/mime/packages
    mimedb.files += resources/system_icons/linux/fritzing.xml
    INSTALLS += mimedb

    manpage.path = $$DATADIR/man/man1
    manpage.files += tools/man/Fritzing.1
    INSTALLS += manpage

    icon.path = $$DATADIR/pixmaps
    icon.extra = install -D -m 0644 $$PWD/resources/images/fritzing_icon.png \
                               $(INSTALL_ROOT)$$DATADIR/pixmaps/fritzing.png
    INSTALLS += icon

    parts.path = $$PKGDATADIR
    parts.files += parts
    INSTALLS += parts

    help.path = $$PKGDATADIR
    help.files += help
    INSTALLS += help

    sketches.path = $$PKGDATADIR
    sketches.files += sketches
    INSTALLS += sketches

    bins.path = $$PKGDATADIR
    bins.files += bins
    INSTALLS += bins

    translations.path = $$PKGDATADIR/translations
    isEmpty(LINGUAS) {
        translations.files += $$system(find $$PWD/translations -name "*.qm" -size +128c)
    } else {
        for(lang, LINGUAS):translations.files += $$system(find $$PWD/translations -name "fritzing_$${lang}.qm" -size +128c)
        isEmpty(translations.files):error("No translations found for $$LINGUAS")
    }
    INSTALLS += translations

    syntax.path = $$PKGDATADIR/translations/syntax
    syntax.files += translations/syntax/*.xml
    INSTALLS += syntax
}

# Sets application icon for macos and windows.
# For linux it is set above.
# See https://doc.qt.io/qt-5/appicon.html
macx {
    ICON = resources/system_icons/macosx/fritzing_icon.icns
    FILE_ICONS.files = resources/system_icons/macosx/mac_fz_icon.icns \
                       resources/system_icons/macosx/mac_fzz_icon.icns \
                       resources/system_icons/macosx/mac_fzb_icon.icns \
                       resources/system_icons/macosx/mac_fzp_icon.icns \
                       resources/system_icons/macosx/mac_fzm_icon.icns \
                       resources/system_icons/macosx/mac_fzpz_icon.icns
    FILE_ICONS.path = Contents/Resources
    QMAKE_BUNDLE_DATA += FILE_ICONS
}
win32 {
    RC_FILE = fritzing.rc
}

# Specify QT modules that are in use
QT += concurrent   \
      core         \
      gui          \
      network      \
      printsupport \
      serialport   \
      sql          \
      svg          \
      widgets      \
      xml

# packageExist() is always true on win32
# (lookup link_pkgconfig above), so we
# need to use libgit2detect.pri script to
# add it to include path if either OS
# is Windows or package wasn't found by
# pkg-config.
!win32:packagesExist(libgit2) {
    PKGCONFIG += libgit2
} else {
    include(pri/libgit2detect.pri)
}

# Check if we have boost
include(pri/boostdetect.pri)

# These pri files mostly just
# append to HEADERS and SOURCES
# variables
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

message("libs $$LIBS")
