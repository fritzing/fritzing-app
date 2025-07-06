# Copyright (c) 2021,2023 Fritzing GmbH

message("Using Fritzing quazip detect script.")

QUAZIP_VERSION=1.4
QUAZIP_PATH=$$absolute_path($$PWD/../quazip-$$QUAZIP_VERSION)
QUAZIP_INCLUDE_PATH=$$QUAZIP_PATH
QUAZIP_LIB_PATH=$$QUAZIP_PATH/quazip/release

SOURCES += \
	src/zlibdummy.c \

exists($$QUAZIP_PATH) {
		message("found quazip in $${QUAZIP_PATH}")
	} else {
		error("quazip could not be found at $$QUAZIP_PATH")
	}

INCLUDEPATH += $$QUAZIP_INCLUDE_PATH
LIBS += -L$$QUAZIP_LIB_PATH -lquazip1-qt$$QT_MAJOR_VERSION
#Swap for Linux
#LIBS += -L$$QUAZIP_LIB_PATH -l:libquazip1-qt6.so

unix {
	message("set rpath for quazip")
	QMAKE_RPATHDIR += $$QUAZIP_LIB_PATH
}

macx {
	LIBS += -lz
}
