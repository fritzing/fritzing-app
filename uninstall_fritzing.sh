#!/bin/bash

# Uninstall script for Fritzing.
# Self-destructing.
# Added in by: @johnDaH4x0r

# Variables
PROGPATH="$(realpath "$0")"
PROG="$(basename "$0")"
ID="$(id -u)"
ARGS="$*"

# Variables [template]
MODE='{mode}'
FRITZ_DIR='{fritz-dir}'
MIMEDIR='{mimedir}'
APPS='{app-dir}'
BIN='{bin}'

ACTIVATE="{stat}"

# Functions

# Show help screen
#
help_scr() {
    cat >&2 <<END
$PROG - Usage:
    $PROG [-h | --help] [-m | --show-mode]

    -h/--help - Show this screen, then exit.
    -m/--show-mode - Show which installation mode was used, then exit.
                     Installation modes:
                        - System mode (system)
                        - User mode (user)

[Self-destructing] Uninstall script for Fritzing.

END
}

# Show help screen, then notify about an invalid argument
# $* - Argument
#
arg_err() {
    help_scr
    echo "Unknown argument: $*" >&2
    exit 1
}

# Parse arguments
for ARG in $ARGS; do
    case $ARG in
        '-h' | '--help')
            show_help
            ;;
        '-m' | '--show-mode')
            echo "Installation mode: $MODE" >&2
            exit
            ;;
        -* | --*)
            arg_err "$ARG"
            exit 1
            ;;
    esac
done

if [[ "$ACTIVATE" != "true" ]]; then
    echo "(exit 1) Uninstall script not yet activated!" >&2
fi

if [[ "$MODE" == "system" && "$ID" -ne 0 ]]; then
    echo "(exit 1) Fritzing was installed in system mode. Must be root!" >&2
    exit 1
fi

# -- SCRIPT END-OF-LIFE <you know, self-destruction> -- #
# echo "Removing residues..."
# rm -f "$PROGPATH"
