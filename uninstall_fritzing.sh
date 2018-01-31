#!/bin/bash

# Uninstall script for Fritzing. <WORKING ON IT!>
# Self-destructing.
# Added in by: @johnDaH4x0r

# Variables
PROGPATH="$(realpath "$0")"
PROG="$(basename "$0")"
ID="$(id -u)"
ARGS="$*"

# TODO: remove marker when finished
DEVSTAT=""
export DEVSTAT

# Variables [template]
MODE='{mode}'
FRITZ_DIR='{fritz-dir}'
MIMEDIR='{mimedir}'
MIMES='{mimes}'
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
            help_scr
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
    exit 1
fi

if [[ "$MODE" == "system" && "$ID" -ne 0 ]]; then
    echo "(exit 1) Fritzing was installed in system mode. Must be root!" >&2
    exit 1
fi

# ---- UNINSTALL START ---- #
grep -q 'application/x-fritzing' "$MIMES"
if [ $? -eq 0 ]; then
    TOWRITE="$(grep -v 'application/x-fritzing' "$MIMES")"
    echo "$TOWRITE" > "$MIMES"
else
    echo "Fritzing MIME types are not registered. (or are deregistered)"
fi

echo "Entering $(basename "$FRITZ_DIR")/" | tr -s "/"
cd "$FRITZ_DIR"

# Uninstall Fritzing MIME packages
echo "Uninstalling Fritzing MIME types..."
{
    xdg-mime uninstall --mode "$MODE" 'icons/x-fritzing-fz.xml' && \
    xdg-mime uninstall --mode "$MODE" 'icons/x-fritzing-fzz.xml' && \
    xdg-mime uninstall --mode "$MODE" 'icons/x-fritzing-fzp.xml' && \
    xdg-mime uninstall --mode "$MODE" 'icons/x-fritzing-fzpz.xml' && \
    xdg-mime uninstall --mode "$MODE" 'icons/x-fritzing-fzb.xml' && \
    xdg-mime uninstall --mode "$MODE" 'icons/x-fritzing-fzbz.xml' && \
    xdg-mime uninstall --mode "$MODE" 'icons/x-fritzing-fzm.xml' ;
}
case $? in
    0)
        echo "-- TASK ENDED SUCCESSFULLY! --"
        ;;
    1)
        echo "AN ERROR OCCURED! PLEASE FIX THE PROBLEMS ABOVE, THEN TRY AGAIN" >&2
        echo -e "-*-*-*-*-*-*-*-\n"
        exit 1
        ;;
    esac
echo -e "-*-*-*-*-*-*-*-\n"

# Uninstall image-files into mime system with specified size
echo "Uninstalling Fritzing system icons..."
echo "-*-*-*-*-*-*-*-"

{
    xdg-icon-resource uninstall --mode "$MODE" --context mimetypes --size 128 'icons/fz_icon128.png' application-x-fritzing-fz && \
    xdg-icon-resource uninstall --mode "$MODE" --context mimetypes --size 256 'icons/fz_icon256.png' application-x-fritzing-fz && \
    xdg-icon-resource uninstall --mode "$MODE" --context mimetypes --size 128 'icons/fzz_icon128.png' application-x-fritzing-fzz && \
    xdg-icon-resource uninstall --mode "$MODE" --context mimetypes --size 256 'icons/fzz_icon256.png' application-x-fritzing-fzz && \
    xdg-icon-resource uninstall --mode "$MODE" --context mimetypes --size 128 'icons/fzp_icon128.png' application-x-fritzing-fzp && \
    xdg-icon-resource uninstall --mode "$MODE" --context mimetypes --size 256 'icons/fzp_icon256.png' application-x-fritzing-fzp && \
    xdg-icon-resource uninstall --mode "$MODE" --context mimetypes --size 128 'icons/fzpz_icon128.png' application-x-fritzing-fzpz && \
    xdg-icon-resource uninstall --mode "$MODE" --context mimetypes --size 256 'icons/fzpz_icon256.png' application-x-fritzing-fzpz && \
    xdg-icon-resource uninstall --mode "$MODE" --context mimetypes --size 128 'icons/fzb_icon128.png' application-x-fritzing-fzb && \
    xdg-icon-resource uninstall --mode "$MODE" --context mimetypes --size 256 'icons/fzb_icon256.png' application-x-fritzing-fzb && \
    xdg-icon-resource uninstall --mode "$MODE" --context mimetypes --size 128 'icons/fzbz_icon128.png' application-x-fritzing-fzbz && \
    xdg-icon-resource uninstall --mode "$MODE" --context mimetypes --size 256 'icons/fzbz_icon256.png' application-x-fritzing-fzbz && \
    xdg-icon-resource uninstall --mode "$MODE" --context mimetypes --size 128 'icons/fzm_icon128.png' application-x-fritzing-fzm && \
    xdg-icon-resource uninstall --mode "$MODE" --context mimetypes --size 256 'icons/fzm_icon256.png' application-x-fritzing-fzm ;
}
case $? in
    0)
        echo "-- TASK ENDED SUCCESSFULLY! --"
        ;;
    1)
        echo "AN ERROR OCCURED! PLEASE FIX THE PROBLEMS ABOVE, THEN TRY AGAIN" >&2
        echo -e "-*-*-*-*-*-*-*-\n"
        exit 1
        ;;
    esac
echo -e "-*-*-*-*-*-*-*-\n"

# Remove 'Fritzing' symlink
echo "Removing symlinks..."
rm -f "$BIN"/Fritzing 2>/dev/null

echo "Leaving $(basename "$FRITZ_DIR")/" | tr -s "/"
cd -

echo "Deleting $(basename "$FRITZ_DIR")/" | tr -s "/"
rm -rf "$FRITZ_DIR"

echo "Removing desktop file..."
rm -f "$APPS"/fritzing.desktop

echo "Updating databases..."
update-desktop-database "$APPS"
update-mime-database "$MIMEDIR"

echo "-*-*-*-*-*-*-"
echo "UNINSTALLED!"
echo "-*-*-*-*-*-*-"
echo

# -- SCRIPT END-OF-LIFE <you know, self-destruction> -- #
echo "Removing residues..."
rm -f "$PROGPATH"
