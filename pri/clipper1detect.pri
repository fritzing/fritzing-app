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

win32 {
    CONFIG(debug, debug|release):exists($$absolute_path($${CLIPPER1}/lib/debug/polyclipping.lib)) {
        LIBS += $$absolute_path($${CLIPPER1}/lib/debug/polyclipping.lib)
        QMAKE_RPATHDIR += $$absolute_path($${CLIPPER1}/lib/debug)
    } else {
        LIBS += $$absolute_path($${CLIPPER1}/lib/polyclipping.lib)
        QMAKE_RPATHDIR += $$absolute_path($${CLIPPER1}/lib)
    }
} else {
    LIBS += -L$$absolute_path($${CLIPPER1}/lib) -lpolyclipping
    QMAKE_RPATHDIR += $$absolute_path($${CLIPPER1}/lib)
}
