#!/bin/sh
#
# this is a rough beginning of a linux install script for fritzing
# sets up document icons and file associations using mime types

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

# install fritzing into mime user directory
xdg-mime install --mode user 'icons/x-fritzing-fz.xml'
xdg-mime install --mode user 'icons/x-fritzing-fzz.xml'
xdg-mime install --mode user 'icons/x-fritzing-fzp.xml'
xdg-mime install --mode user 'icons/x-fritzing-fzpz.xml'
xdg-mime install --mode user 'icons/x-fritzing-fzb.xml'
xdg-mime install --mode user 'icons/x-fritzing-fzbz.xml'
xdg-mime install --mode user 'icons/x-fritzing-fzm.xml'

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
xdg-icon-resource install --mode user --context mimetypes --size 128 'icons/fz_icon128.png' application-x-fritzing-fz
xdg-icon-resource install --mode user --context mimetypes --size 256 'icons/fz_icon256.png' application-x-fritzing-fz
xdg-icon-resource install --mode user --context mimetypes --size 128 'icons/fzz_icon128.png' application-x-fritzing-fzz
xdg-icon-resource install --mode user --context mimetypes --size 256 'icons/fzz_icon256.png' application-x-fritzing-fzz
xdg-icon-resource install --mode user --context mimetypes --size 128 'icons/fzp_icon128.png' application-x-fritzing-fzp
xdg-icon-resource install --mode user --context mimetypes --size 256 'icons/fzp_icon256.png' application-x-fritzing-fzp
xdg-icon-resource install --mode user --context mimetypes --size 128 'icons/fzpz_icon128.png' application-x-fritzing-fzpz
xdg-icon-resource install --mode user --context mimetypes --size 256 'icons/fzpz_icon256.png' application-x-fritzing-fzpz
xdg-icon-resource install --mode user --context mimetypes --size 128 'icons/fzb_icon128.png' application-x-fritzing-fzb
xdg-icon-resource install --mode user --context mimetypes --size 256 'icons/fzb_icon256.png' application-x-fritzing-fzb
xdg-icon-resource install --mode user --context mimetypes --size 128 'icons/fzbz_icon128.png' application-x-fritzing-fzbz
xdg-icon-resource install --mode user --context mimetypes --size 256 'icons/fzbz_icon256.png' application-x-fritzing-fzbz
xdg-icon-resource install --mode user --context mimetypes --size 128 'icons/fzm_icon128.png' application-x-fritzing-fzm
xdg-icon-resource install --mode user --context mimetypes --size 256 'icons/fzm_icon256.png' application-x-fritzing-fzm

# update user databases
update-desktop-database ~/.local/share/applications
update-mime-database ~/.local/share/mime

echo "installed fritzing system icons"
