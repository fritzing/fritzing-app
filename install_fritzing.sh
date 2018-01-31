#!/bin/bash
#
# This is a rough beginning of a Linux install script for Fritzing
#
# Sets up document icons and file associations (MIME types),
# and installs Fritzing to target.
#

# Initial variables
APPDIR="$(dirname "$0")"
PROG="$(basename "$0")"
APPVER="0.9.3b"
ID="$(id -u)"
ARGS="$*"

# Mode toggles
MODE=""
SYSMODE="false"
USERMODE="false"

# Functions
show_help() {
    cat >&2 <<END
$PROG - Usage:
    $PROG [-h | --help] [-s | --system] [-u | --user]

    -h/--help - Show this message, then exit.
 *  -s/--system - Install Fritzing in system mode (for global use)
 *  -u/--user - Install Fritzing in user mode (recommended, optimal)

*Mutually exclusive, cannot be mixed!

Installer program for Fritzing.
Fritzing version: $APPVER
END
}

arg_err() {
    show_help
    echo "(exit 1) Unknown argument: $*" >&2
    exit 1
}

esc_path() {
    if [[ ! "$*" ]]; then
        sed 's/\//\\\//g'
        return
    fi

    echo "$*" | sed 's/\//\\\//g'
}

# Parse arguments
for i in $ARGS; do
    case $i in
        '-h' | '--help')
            show_help
            exit 0
            ;;
        '-s' | '--system')
            SYSMODE="true"
            ;;
        '-u' | '--user')
            USERMODE="true"
            ;;
        -* | --*)
            arg_err "$i"
            exit 1
            ;;
    esac
done

if [[ "$SYSMODE" == 'true' && "$USERMODE" == 'true' ]]; then
    show_help
    echo "Cannot be mixed: '-u/--user' and '-s/--system'" >&2
    exit 1
elif [[ "$ID" -eq 0 || "$SYSMODE" == "true" ]]; then
    MODE="system"
elif [[ "$ID" -ne 0 || "$USERMODE" = "true" ]]; then
    MODE="user"
fi

if [[ "$MODE" == "system" && "$ID" -ne 0 ]]; then
    echo "(exit 1) Cannot install in system mode. Must be root!" >&2
    exit 1
fi

# Mode interpreter
case $MODE in
    'user')
        MIMEDIR=~/.local/share/mime
        APPS=~/.local/share/applications
        APP_ICON=~/.local/share/icons/hicolor/128x128/apps/fritzing.png
        MIMES=~/.mime.types
        FRITZ_DIR=~/.local/share/fritzing
        BIN=~/.local/bin

        # Back-up ~/.mime.types
        echo " Copying ~/.mime.types -> ~/.mime.types.bak"
        cp "$MIMES" "${MIMES}.bak";;

    'system')
        MIMEDIR=/usr/share/mime
        APPS=/usr/share/applications
        MIMES=/etc/mime.types
        FRITZ_DIR=/usr/share/fritzing
        APP_ICON=/usr/share/icons/hicolor/128x128/apps/fritzing.png
        BIN=/usr/bin

        # Back-up /etc/mime.types
        echo "Copying /etc/mime.types -> /etc/mime.types.bak"
        cp "$MIMES" "${MIMES}.bak"
        ;;
    '')
        echo "Sorry, but you might have a bug!" >&2
        exit 1
        ;;
    esac


# check if user .mime.types file exists, otherwise create it
if [[ ! -f ~/.mime.types && "$MODE" == "user" ]]; then
    echo "Creating ~/.mime.types ..."
    touch ~/.mime.types
fi

# Fritzing target directory
if [[ ! -d "$FRITZ_DIR" ]]; then
    rm -rf "$FRITZ_DIR"
    mkdir -p "$FRITZ_DIR"
fi

# Executable(s) folder
if [[ ! -d "$BIN" ]]; then
    mkdir -p "$BIN"
fi

# add mime types for fritzing file formats
grep -q 'application/x-fritzing' "$MIMES"
if [ $? -eq 0 ]; then
    echo "Fritzing MIME types already registered!"
else
    echo "Registering Fritzing MIME types..."

    # This is the "correct" syntax
    {
        echo -e "application/x-fritzing-fz\t\t\tfritzing" ;
        echo -e "application/x-fritzing-fzz\t\t\tfritzing" ;
        echo -e "application/x-fritzing-fzp\t\t\tfritzing" ;
        echo -e "application/x-fritzing-fzpz\t\t\tfritzing" ;
        echo -e "application/x-fritzing-fzb\t\t\tfritzing" ;
        echo -e "application/x-fritzing-fzbz\t\t\tfritzing" ;
        echo -e "application/x-fritzing-fzm\t\t\tfritzing" ;
    } >> "$MIMES"
fi

# Install / copy Fritzing to target
echo "Copying Fritzing data..."
cp -Rp "$APPDIR" "$FRITZ_DIR"

# Enter installation/Fritzing directory
echo "Entering $(basename "$FRITZ_DIR")/" | tr -s "/"
cd "$FRITZ_DIR"

# If installing in system mode, notify user that
# it will take a long time.
if [[ "$MODE" == "system" ]]; then
    echo -e "\n"
    echo ":: INSTALLING IN SYSTEM MODE. IT WILL TAKE A LONG TIME!"
    echo "PLEASE BE PATIENT..."
    echo -e "\n"
fi

# install Fritzing into MIME directory
echo "Installing Fritzing MIME types..."
echo "-*-*-*-*-*-*-*-"
{
    xdg-mime install --mode "$MODE" 'icons/x-fritzing-fz.xml' && \
    xdg-mime install --mode "$MODE" 'icons/x-fritzing-fzz.xml' && \
    xdg-mime install --mode "$MODE" 'icons/x-fritzing-fzp.xml' && \
    xdg-mime install --mode "$MODE" 'icons/x-fritzing-fzpz.xml' && \
    xdg-mime install --mode "$MODE" 'icons/x-fritzing-fzb.xml' && \
    xdg-mime install --mode "$MODE" 'icons/x-fritzing-fzbz.xml' && \
    xdg-mime install --mode "$MODE" 'icons/x-fritzing-fzm.xml' ;
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

# set the default application to fritzing.desktop
echo "Setting up default application -> fritzing.desktop"
echo "-*-*-*-*-*-*-*-"
{
    xdg-mime default 'fritzing.desktop' application/x-fritzing-fz && \
    xdg-mime default 'fritzing.desktop' application/x-fritzing-fzz && \
    xdg-mime default 'fritzing.desktop' application/x-fritzing-fzp && \
    xdg-mime default 'fritzing.desktop' application/x-fritzing-fzpz && \
    xdg-mime default 'fritzing.desktop' application/x-fritzing-fzb && \
    xdg-mime default 'fritzing.desktop' application/x-fritzing-fzbz && \
    xdg-mime default 'fritzing.desktop' application/x-fritzing-fzm ;
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

# install image-files into mime system with specified size
echo "Installing Fritzing system icons..."
echo "-*-*-*-*-*-*-*-"

{
    xdg-icon-resource install --mode "$MODE" --context mimetypes --size 128 'icons/fz_icon128.png' application-x-fritzing-fz && \
    xdg-icon-resource install --mode "$MODE" --context mimetypes --size 256 'icons/fz_icon256.png' application-x-fritzing-fz && \
    xdg-icon-resource install --mode "$MODE" --context mimetypes --size 128 'icons/fzz_icon128.png' application-x-fritzing-fzz && \
    xdg-icon-resource install --mode "$MODE" --context mimetypes --size 256 'icons/fzz_icon256.png' application-x-fritzing-fzz && \
    xdg-icon-resource install --mode "$MODE" --context mimetypes --size 128 'icons/fzp_icon128.png' application-x-fritzing-fzp && \
    xdg-icon-resource install --mode "$MODE" --context mimetypes --size 256 'icons/fzp_icon256.png' application-x-fritzing-fzp && \
    xdg-icon-resource install --mode "$MODE" --context mimetypes --size 128 'icons/fzpz_icon128.png' application-x-fritzing-fzpz && \
    xdg-icon-resource install --mode "$MODE" --context mimetypes --size 256 'icons/fzpz_icon256.png' application-x-fritzing-fzpz && \
    xdg-icon-resource install --mode "$MODE" --context mimetypes --size 128 'icons/fzb_icon128.png' application-x-fritzing-fzb && \
    xdg-icon-resource install --mode "$MODE" --context mimetypes --size 256 'icons/fzb_icon256.png' application-x-fritzing-fzb && \
    xdg-icon-resource install --mode "$MODE" --context mimetypes --size 128 'icons/fzbz_icon128.png' application-x-fritzing-fzbz && \
    xdg-icon-resource install --mode "$MODE" --context mimetypes --size 256 'icons/fzbz_icon256.png' application-x-fritzing-fzbz && \
    xdg-icon-resource install --mode "$MODE" --context mimetypes --size 128 'icons/fzm_icon128.png' application-x-fritzing-fzm && \
    xdg-icon-resource install --mode "$MODE" --context mimetypes --size 256 'icons/fzm_icon256.png' application-x-fritzing-fzm ;
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

echo "Making symlinks..."
ln -s "$(realpath ./Fritzing)" "$BIN"/Fritzing 2>/dev/null

if [ $? -ne 0 ]; then
    echo "Symlinks already exists! (or linking failed...)" >&2
fi

echo "Doing final touch..."

# Escape all forward slashes to make `sed` stable
APP_ICON_ESC="$(esc_path "$APP_ICON")"

# Modify Fritzing desktop executable [keep original]
sed -i.use "s/icons\/fritzing_icon.png/${APP_ICON_ESC}/" fritzing.desktop

# Install Fritzing desktop application
cp fritzing.desktop.use "$APPS"/fritzing.desktop
cp icons/fritzing_icon.png "$APP_ICON"
rm -f fritzing.desktop.use

# Escape all paths [forward slashes] to make `sed` stable
MIMES_ESC="$(esc_path "$MIMES")"
MIMEDIR_ESC="$(esc_path "$MIMEDIR")"
FRITZ_DIR_ESC="$(esc_path "$FRITZ_DIR")"
APPS_ESC="$(esc_path "$APPS")"
BIN_ESC="$(esc_path "$BIN")"

DEVSTAT="$(grep 'DEVSTAT' uninstall_fritzing.sh | cut -d '=' -f 2 | tr -d '"')"

# Substitute all variables
cp uninstall_fritzing.sh uninstall_fritzing.use

# NOTE: Do not put a '$' before the placeholder brackets!
# How I did it: sed -i "s/{placeholder}/$VARIABLE/(g if neccessary)" <file>
#
sed -i "s/{mode}/$MODE/" uninstall_fritzing.use
sed -i "s/{fritz-dir}/${FRITZ_DIR_ESC}/" uninstall_fritzing.use
sed -i "s/{mimedir}/${MIMEDIR_ESC}/" uninstall_fritzing.use
sed -i "s/{mimes}/${MIMES_ESC}/" uninstall_fritzing.use
sed -i "s/{app-dir}/${APPS_ESC}/" uninstall_fritzing.use
sed -i "s/{bin}/${BIN_ESC}/" uninstall_fritzing.use

# Activate/validate uninstall script
if [[ "$DEVSTAT" == '<in_dev>' ]]; then
    echo "(warn) Uninstall script still in development!" >&2

    # No-op (do not activate!)
else
    echo "Activating uninstall script..."
    sed -i "s/{stat}/true/" uninstall_fritzing.use
    echo "Activated!"
    echo
fi

cp uninstall_fritzing.use "$BIN"/uninstall_fritzing.sh

# update databases
echo "Updating databases..."
update-desktop-database "$APPS"
update-mime-database "$MIMEDIR"

echo "Done!"
