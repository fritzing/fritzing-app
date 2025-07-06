# Copyright (c) 2023 Fritzing GmbH

message("Using fritzing Clipper 1 detect script.")

unix {
    message("including Clipper1 library on linux or mac")

    exists($$absolute_path($$PWD/../clipper-6.4.2)) {
                    CLIPPER1 = $$absolute_path($$PWD/../clipper-6.4.2)
				message("found Clipper1 in $${CLIPPER1}")
			}
}

win32 {
    message("including Clipper1 library on windows")

    exists($$absolute_path($$PWD/../Clipper-6.4.2)) {
        CLIPPER1 = $$absolute_path($$PWD/../Clipper-6.4.2)
                    message("found Clipper1 in $${CLIPPER1}")
            }
}

message("including $$absolute_path($$CLIPPER1/cpp)")
INCLUDEPATH += $$absolute_path($$CLIPPER1/cpp)

LIBS += -L$$absolute_path($$CLIPPER1/cpp/release) -lpolyclipping
#Swap for Linux
#LIBS += -L$$absolute_path($$CLIPPER1/cpp/Release) -l:libpolyclipping.a

QMAKE_RPATHDIR += $$absolute_path($$CLIPPER1/release)
