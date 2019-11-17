lessThan(QT_MAJOR_VERSION, 5) {
    error(Fritzing does not build with Qt 4 or earlier. 5.12 is recommended.)
}

lessThan(QT_MINOR_VERSION, 9) {
    error(Fritzing does not build with Qt 5.8 or earlier. 5.12 is recommended.)
}

CONFIG += debug
#CONFIG += debug_and_release
CONFIG += c++14

QT += concurrent core gui network printsupport serialport sql svg widgets xml

DESTDIR=generated
OBJECTS_DIR=generated
MOC_DIR=generated
RCC_DIR=generated
UI_DIR=generated
Debug:DESTDIR = debug
Debug:OBJECTS_DIR = debug
Debug:MOC_DIR = debug
Debug:RCC_DIR = debug
Debug:UI_DIR = debug
Release:DESTDIR = release
Release:OBJECTS_DIR = release
Release:MOC_DIR = release
Release:RCC_DIR = release
Release:UI_DIR = release

load(configure)

win32 {
    DEFINES += WIN64
}
macx {
}
unix {
    !macx { # unix is defined on mac
        HARDWARE_PLATFORM = $$system(uname -m)
        contains(HARDWARE_PLATFORM, x86_64) {
            DEFINES += LINUX_64
        } else {
            DEFINES += LINUX_32
        }
    }
}
