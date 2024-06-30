# Copyright (c) 2023 Fritzing GmbH

message("Using fritzing Clipper 1 detect script.")

unix {
    message("including Clipper1 library on linux or mac")

    exists($$absolute_path($$PWD/../../Clipper1)) {
	            CLIPPER1 = $$absolute_path($$PWD/../../Clipper1/6.4.2)
				message("found Clipper1 in $${CLIPPER1}")
			}
}

win32 {
    message("including Clipper1 library on windows")

    exists($$absolute_path($$PWD/../../Clipper1-6.4.2)) {
        CLIPPER1 = $$absolute_path($$PWD/../../Clipper1-6.4.2)
                    message("found Clipper1 in $${CLIPPER1}")
            }
}

message("including $$absolute_path($${CLIPPER1}/include)")
INCLUDEPATH += $$absolute_path($${CLIPPER1}/include/polyclipping)

LIBS += -L$$absolute_path($${CLIPPER1}/lib) -lpolyclipping
QMAKE_RPATHDIR += $$absolute_path($${CLIPPER1}/lib)
