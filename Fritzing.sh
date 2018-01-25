#!/bin/sh
appname="Fritzing"
# dirname=`dirname "${0}"`

# new dirname procedure from Volker Kuhlmann
# use -f to make the readlink path absolute
dirname="$(dirname -- "$(readlink -f -- "${0}")" )"

if [ "$dirname" = "." ]; then
	dirname="$PWD/$dirname"
fi

# Initialize interpreter path
export LD_LIBRARY_PATH="${dirname}/lib"

# Run Fritzing binary
"$dirname/lib/$appname" "$@"
