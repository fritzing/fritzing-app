# Copyright (c) 2021 Fritzing GmbH

message("Using fritzing quazip detect script.")

SOURCES += \
    src/zlibdummy.c \

exists($$absolute_path($$PWD/../../quazip_qt5)) {
        QUAZIPPATH = $$absolute_path($$PWD/../../quazip_qt5)
        message("found quazip in $${QUAZIPPATH}")
    } else {
        error("quazip could not be found.")
    }

message("including $$absolute_path($${QUAZIPPATH}/include/quazip)")

unix:!macx {
    message("including quazip library on linux")
    INCLUDEPATH += $$absolute_path($${QUAZIPPATH}/include/quazip)
    LIBS += -L$$absolute_path($${QUAZIPPATH}/lib) -lquazip1-qt5
    QMAKE_RPATHDIR += $$absolute_path($${QUAZIPPATH}/lib)
}

macx {
    message("including quazip library on mac os")
    INCLUDEPATH += $$absolute_path($${QUAZIPPATH}/include/quazip)
    LIBS += -L$$absolute_path($${QUAZIPPATH}/lib) -lquazip1-qt5
    QMAKE_RPATHDIR += $$absolute_path($${QUAZIPPATH}/lib)
    LIBS += -lz
}

win32 {

    message("including quazip library on windows")

    QUAZIPINCLUDE = $$absolute_path($${QUAZIPPATH}/include/quazip)
    exists($$QUAZIPINCLUDE/quazip.h) {
        message("found quazip include path at $$QUAZIPINCLUDE")
    } else {
        message("Fritzing requires quazip")
        error("quazip include path not found in $$QUAZIPINCLUDE")
    }

    INCLUDEPATH += $$QUAZIPINCLUDE

    contains(QMAKE_TARGET.arch, x86_64) {
        QUAZIPLIB = $$absolute_path($$QUAZIPPATH/build64/Release)
    } else {
        QUAZIPLIB = $$absolute_path($$QUAZIPPATH/build32/Release)
    }

    exists($$QUAZIPLIB/quazip1-qt5.lib) {
        message("found quazip library in $$QUAZIPLIB")
    } else {
        error("quazip library not found in $$QUAZIPLIB")
    }

    LIBS += -L$$QUAZIPLIB -lquazip1-qt5
}