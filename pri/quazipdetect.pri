# Copyright (c) 2021 Fritzing GmbH
# https://aur.archlinux.org/cgit/aur.git/tree/0001-Quick-Dirty-patch-to-allow-finding-quazip-qt6-on-Arc.patch?h=fritzing

#message("Using fritzing quazip detect script.")
message("Using custom quazip detect script.")

SOURCES += \
    src/zlibdummy.c \

message("including quazip library on linux")
INCLUDEPATH += /usr/local/include/QuaZip-Qt6-1.4/quazip/
LIBS += -lquazip1-qt6
