#!/bin/sh
#
# this is a rough beginning of a linux install script for fritzing
# sets up document icons and file associations using mime types

APPLICATIONSDIR="${HOME}/.local/share/applications"
MIMEDIR="${HOME}/.local/share/mime"
PACKAGESDIR="${MIMEDIR}/packages"

APPDIR=$(dirname "$0")

# check if user .mime.types file exists, otherwise create it
if [ ! -f ~/.mime.types ]
then
	echo "creating user mime.types file"
	touch ~/.mime.types
fi

# add mime types for fritzing file formats
grep -q application/x-fritzing ~/.mime.types
if [ $? -eq 0 ]
then
	echo "fritzing mime types already registered"
else
	echo "application/x-fritzing-fz 	fritzing" >> ~/.mime.types
	echo "application/x-fritzing-fzz 	fritzing" >> ~/.mime.types
	echo "application/x-fritzing-fzp 	fritzing" >> ~/.mime.types
	echo "application/x-fritzing-fzpz 	fritzing" >> ~/.mime.types
	echo "application/x-fritzing-fzb 	fritzing" >> ~/.mime.types
	echo "application/x-fritzing-fzbz 	fritzing" >> ~/.mime.types
	echo "application/x-fritzing-fzm 	fritzing" >> ~/.mime.types
fi

cd $APPDIR

# install fritzing into user mime packages directory
mkdir -p "${PACKAGESDIR}"
cp icons/fritzing.xml "${PACKAGESDIR}" || exit 1

# set the default application to fritzing.desktop
xdg-mime default 'fritzing.desktop' application/x-fritzing-fz
xdg-mime default 'fritzing.desktop' application/x-fritzing-fzz
xdg-mime default 'fritzing.desktop' application/x-fritzing-fzp
xdg-mime default 'fritzing.desktop' application/x-fritzing-fzpz
xdg-mime default 'fritzing.desktop' application/x-fritzing-fzb
xdg-mime default 'fritzing.desktop' application/x-fritzing-fzbz
xdg-mime default 'fritzing.desktop' application/x-fritzing-fzm

# install image-files into user mime system with specified size
# ~/.local/share/icons/hicolor/*size*
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
