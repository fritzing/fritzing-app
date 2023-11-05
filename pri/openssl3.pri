# Qt 6.5.3 is linked against openssl3
# If you want to use older versions of openssl,
# you'll have to build a Qt 6.5 version with support for that version.

unix { !macx {
	OPENSSL_REQUIRED_MAJOR_VERSION = 3
	OPENSSL_ACTUAL_VERSION = $$system(pkg-config --modversion openssl 2>/dev/null)
	
	isEmpty(OPENSSL_ACTUAL_VERSION) {
		message("pkg-config could not find OpenSSL.")
	} else {
		message("pkg-config found OpenSSL version: " $${OPENSSL_ACTUAL_VERSION})
		greaterThan(OPENSSL_ACTUAL_VERSION, 2.99) {
			message("OpenSSL version is adequate: " $${OPENSSL_ACTUAL_VERSION})
			CONFIG += link_pkgconfig
			PKGCONFIG += openssl
			OPENSSL_LIBDIRS = $$system(pkg-config --libs-only-L openssl 2>/dev/null | sed 's/-L//g')
			for(DIR, OPENSSL_LIBDIRS) {
				QMAKE_RPATHDIR += $$DIR
			}
			return() # No need for fallback if version is adequate
		} else {
			message("OpenSSL version found by pkg-config is too old: " $${OPENSSL_ACTUAL_VERSION})
		}
	}
	
	# Use hardcoded fallback path if pkg-config didn't find OpenSSL or the version was too old
	# Define the directory where OpenSSL 3 is installed (all dependencies go to Fritzing siblings directories)
	OPENSSL_PREFERRED_VERSION=3.0.12
	OPENSSL_DIR = $$absolute_path(../../openssl-$${OPENSSL_PREFERRED_VERSION})
	OPENSSL_BIN = $$absolute_path($${OPENSSL_DIR}/bin/openssl)
	OPENSSL_VERSION_STR = $$system("$${OPENSSL_BIN} version")
	OPENSSL_VERSION = $$system("echo '$${OPENSSL_VERSION_STR}' | awk '{print $2}'")
	message("OpenSSL version from sibling directory:" $${OPENSSL_VERSION})
	isEmpty(OPENSSL_VERSION) {
		error("Cannot determine fallback OpenSSL version. Please verify the installation of OpenSSL.")
	} else {
		OPENSSL_LIB_DIR = $${OPENSSL_DIR}/lib64
		OPENSSL_INCLUDE_DIR = $${OPENSSL_DIR}/include
		versionAtLeast(OPENSSL_VERSION, $${OPENSSL_REQUIRED_MAJOR_VERSION}) {
			message("Fallback OpenSSL version is adequate: " $${OPENSSL_VERSION})
	
			INCLUDEPATH += $${OPENSSL_INCLUDE_DIR}
			LIBS += -L$${OPENSSL_LIB_DIR} -lssl -lcrypto
			QMAKE_RPATHDIR += $${OPENSSL_LIB_DIR}
		} else {
			error("Fallback OpenSSL version " $${OPENSSL_REQUIRED_MAJOR_VERSION} "or higher is required. Found: " $${OPENSSL_VERSION_STR})
		}
	}
}}
