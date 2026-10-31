# Copyright (c) 2026 Fritzing GmbH

# Settings for the Qt for WebAssembly build (single-threaded Qt, linked with Asyncify).
# Only included by phoenix.pro when building with the wasm-emscripten mkspec;
# see docker/wasm/ for the build environment.
#
# FRITZING_WASM_DEPS: folder with boost, svgpp and the WebAssembly builds of
#                     Clipper1, QuaZip and ngspice
# FRITZING_WASM_DATA: folder preloaded as /fritzing (translations, help, sketches,
#                     fritzing-parts including a prebuilt parts.db)

message("Using Fritzing WebAssembly settings.")

WASM_DEPS = $$(FRITZING_WASM_DEPS)
isEmpty(WASM_DEPS):error("FRITZING_WASM_DEPS is not set")
WASM_DATA = $$(FRITZING_WASM_DATA)
isEmpty(WASM_DATA):error("FRITZING_WASM_DATA is not set")

CONFIG -= debug_and_release
CONFIG += release exceptions

INCLUDEPATH += \
	$$WASM_DEPS/boost_1_85_0 \
	$$WASM_DEPS/svgpp-1.3.1/include \
	$$WASM_DEPS/ngspice-42/include \
	$$WASM_DEPS/Clipper1-6.4.2/include/polyclipping \
	$$WASM_DEPS/quazip/include/QuaZip-Qt6-1.4 \
	$$PWD/../src/program/wasm

LIBS += \
	$$WASM_DEPS/quazip/lib/libquazip1-qt6.a \
	$$WASM_DEPS/Clipper1-6.4.2/lib/libpolyclipping.a \
	$$WASM_DEPS/ngspice-42/lib/libngspice.a

QTPLUGIN += qsqlite qsvg qsvgicon qjpeg qgif

SOURCES -= src/version/partschecker.cpp
SOURCES += src/version/partschecker_wasm.cpp

HEADERS += src/utils/wasmstorage.h
SOURCES += src/utils/wasmstorage.cpp

# No QtSerialPort and no QProcess: serial ports go through the browser's Web Serial API
# (src/program/wasm/QtSerialPort maps the QtSerialPort includes to webserialport_wasm.h),
# and Arduino code is compiled by docker/wasm/compile-service
HEADERS += src/program/webserialport_wasm.h
SOURCES += src/program/webserialport_wasm.cpp

# zlib comes from the Emscripten port (needed by QuaZip)
QMAKE_CFLAGS += -sUSE_ZLIB=1
QMAKE_CXXFLAGS += -sUSE_ZLIB=1 -fexceptions

# Asyncify lets the many modal exec() dialogs run nested event loops in the browser
QMAKE_LFLAGS += \
	-sUSE_ZLIB=1 \
	-fexceptions \
	-sASYNCIFY \
	-sASYNCIFY_STACK_SIZE=131072 \
	-sALLOW_MEMORY_GROWTH=1 \
	-sMAXIMUM_MEMORY=4GB \
	-lidbfs.js \
	--preload-file $$WASM_DATA@/fritzing \
	--use-preload-cache
