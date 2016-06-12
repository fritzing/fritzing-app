#!/bin/sh
#
# this is a rough beginning of a linux install script for fritzing
# sets up document icons and file associations using MIME types
#
# first ensure fritzing is unpacked in its final destination
# and then run this script

APPLICATIONSDIR="${HOME}/.local/share/applications"
MIMEDIR="${HOME}/.local/share/mime"
PACKAGESDIR="${MIMEDIR}/packages"

APPDIR=$(dirname "$0")

cd $APPDIR

# install fritzing into user MIME packages directory
mkdir -p "${PACKAGESDIR}"
cp icons/fritzing.xml "${PACKAGESDIR}" || exit 1

# install fritzing desktop entry for user (includes MIME associations)
desktop-file-edit --set-key=Exec --set-value="$(pwd)/Fritzing" fritzing.desktop
xdg-desktop-menu install --novendor --mode user fritzing.desktop

# install image-files into user hicolor theme with specified size
# ~/.local/share/icons/hicolor/*size*
#    /apps
xdg-icon-resource install --noupdate --novendor --mode user --context apps \
	--size 256 icons/fritzing_icon.png fritzing
#    /mimetypes
ICON_SIZES="128 256"
FILE_EXTENSIONS="fz fzz fzb fzbz fzp fzpz fzm"
for size in ${ICON_SIZES}; do
	for extension in ${FILE_EXTENSIONS}; do
		xdg-icon-resource install --noupdate --mode user --context mimetypes \
			--size ${size} "icons/${extension}_icon${size}.png" \
			"application-x-fritzing-${extension}"
	done
done

# update user databases
update-desktop-database "${APPLICATIONSDIR}"
update-mime-database "${MIMEDIR}"
xdg-icon-resource forceupdate --mode user

echo "installed fritzing system icons"
