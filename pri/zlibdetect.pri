# Copyright (c) 2021,2023 Fritzing GmbH

message("Using Fritzing Zlib detect script.")

Zlib_VERSION=1.3.1
Zlib_PATH=$$absolute_path($$PWD/../zlib-$$Zlib_VERSION)
Zlib_INCLUDE_PATH=$$Zlib_PATH
Zlib_LIB_PATH=$$Zlib_PATH/build/release


exists($$Zlib_PATH) {
		message("found zlib in $${Zlib_PATH}")
	} else {
		error("Zlib could not be found at $$Zlib_PATH")
	}

INCLUDEPATH += $$Zlib_INCLUDE_PATH
LIBS += -L$$Zlib_LIB_PATH -lzlibstatic
#Swap for Linux
#LIBS += -L$$Zlib_LIB_PATH -l:libz.so

unix {
	message("set rpath for zlib")
	QMAKE_RPATHDIR += $$Zlib_LIB_PATH
}

macx {
	LIBS += -lz
}
