# Copyright (c) 2021,2024 Fritzing GmbH

message("Using Fritzing quazip detect script.")

# We are currently using a quazip version from this PR:
# https://github.com/stachenov/quazip/pull/199
# Important:
# Quazip is looking for additional maintainers and code reviewers. While the
# library is # mostly feature complete, there are a couple of big ticket
# issues that need # work. Code review is welcome for all pull requests.
# If you are well versed in # Qt and C/C++ please start helping around
# and check https://github.com/stachenov/quazip/issues/185
QUAZIP_VERSION=1.4
QUAZIP_PATH=$$absolute_path($$PWD/../../quazip-$$QT_VERSION-$$QUAZIP_VERSION)intuisphere
QUAZIP_INCLUDE_PATH=$$QUAZIP_PATH/include/QuaZip-Qt6-$$QUAZIP_VERSION
QUAZIP_LIB_PATH=$$QUAZIP_PATH/lib

SOURCES += \
	src/zlibdummy.c \

packagesExist(quazip1-qt$$QT_MAJOR_VERSION) {
    PKGCONFIG += quazip1-qt$$QT_MAJOR_VERSION
    message("found quazip using pkg-config")
    return()
}

exists($$QUAZIP_PATH) {
		message("found quazip in $${QUAZIP_PATH}")
	} else {
		error("quazip could not be found at $$QUAZIP_PATH")
	}

INCLUDEPATH += $$QUAZIP_INCLUDE_PATH
LIBS += -L$$QUAZIP_LIB_PATH -lquazip1-qt$$QT_MAJOR_VERSION

unix {
	message("set rpath for quazip")
	QMAKE_RPATHDIR += $$QUAZIP_LIB_PATH
}

macx {
	LIBS += -lz
}
