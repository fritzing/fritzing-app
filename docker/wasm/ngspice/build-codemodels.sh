#!/bin/sh
# Fritzing web app (docker/wasm): builds ngspice's analog XSPICE code models (analog.cm,
# used by the DC Power Supply part) as a static library for the app, see static_codemodels.c.
#
#   build-codemodels.sh <ngspice source dir> <ngspice build dir> <output library>
#
# Run after ngspice itself was built in <ngspice build dir> with --enable-xspice. ngspice's
# own makefile (src/xspice/icm/GNUmakefile) generates and compiles the code model sources;
# the code model preprocessor (cmpp) is the copy built for the build machine.
set -eu

src=$1
build=$2
out=$3
here=$(cd "$(dirname "$0")" && pwd)
icm=$build/src/xspice/icm
cmpp=$build/src/xspice/cmpp/build/cmpp

test -x "$cmpp"
cd "$icm"

objects() {
	echo analog/dlmain.o
	for model in $("$cmpp" -p "$src/src/xspice/icm/analog/modpath.lst"); do
		echo "analog/$model/cfunc.o" "analog/$model/ifspec.o"
	done
}

build_objects() {
	make cm=analog cmpp="$cmpp" CMPP="$cmpp" CFLAGS="-O2 $1" $(objects) > /dev/null
}

# First pass: find the symbols dlmain.c defines. In analog.cm they call ngspice through
# its core interface table; the second pass prefixes them so that they do not clash with
# ngspice's own functions of the same names when linked together.
build_objects ""
emnm --defined-only --extern-only analog/dlmain.o | awk 'NF == 3 { print "#define " $3 " analogcm_" $3 }' > analogcm-prefix.h
grep -q "define CMdevs analogcm_CMdevs" analogcm-prefix.h

rm -f $(objects)
build_objects "-include $icm/analogcm-prefix.h"
emcc -O2 -c "$here/static_codemodels.c" -o static_codemodels.o
emar rcs "$out" static_codemodels.o $(objects)
