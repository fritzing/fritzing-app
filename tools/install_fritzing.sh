#!/bin/sh
#
# this is a rough beginning of a linux install script for fritzing
# with icon setup for *.fzz files and assignment to fritzing application

APPDIR=$(dirname "$0")

# check if user .mime.types file exists, otherwise create it
if [ ! -f ~/.mime.types ]
then
	echo "creating user mime.types file"
	touch ~/.mime.types
fi

# add mime types for fritzing file formats
grep -q x-fritzing ~/.mime.types
if [ $? -eq 0 ]
then
	echo "fritzing mime types already registered"
else
	echo "mimetypes/x-fritzing-fz 	fritzing" >> ~/.mime.types
	echo "mimetypes/x-fritzing-fzz 	fritzing" >> ~/.mime.types
	echo "mimetypes/x-fritzing-fzp 	fritzing" >> ~/.mime.types
	echo "mimetypes/x-fritzing-fzpz 	fritzing" >> ~/.mime.types
	echo "mimetypes/x-fritzing-fzb 	fritzing" >> ~/.mime.types
	echo "mimetypes/x-fritzing-fzbz 	fritzing" >> ~/.mime.types
fi

cd $APPDIR

# install fritzing into mime user directory
xdg-mime install --mode user 'icons/x-fritzing-fz.xml'
xdg-mime install --mode user 'icons/x-fritzing-fzz.xml'
xdg-mime install --mode user 'icons/x-fritzing-fzp.xml'
xdg-mime install --mode user 'icons/x-fritzing-fzpz.xml'
xdg-mime install --mode user 'icons/x-fritzing-fzb.xml'
xdg-mime install --mode user 'icons/x-fritzing-fzbz.xml'

# set the default application to fritzing.desktop
xdg-mime default 'fritzing.desktop' mimetypes/x-fritzing-fz
xdg-mime default 'fritzing.desktop' mimetypes/x-fritzing-fzz
xdg-mime default 'fritzing.desktop' mimetypes/x-fritzing-fzp
xdg-mime default 'fritzing.desktop' mimetypes/x-fritzing-fzpz
xdg-mime default 'fritzing.desktop' mimetypes/x-fritzing-fzb
xdg-mime default 'fritzing.desktop' mimetypes/x-fritzing-fzbz

# install image-files into user mime system with specified size
# ~/.local/share/icons/hicolor/*size*
# 128x128
xdg-icon-resource install --mode user --context mimetypes --size 128 'icons/linux_fzz_icon128.png' mimetypes-x-fritzing-fzz
# 256x256
xdg-icon-resource install --mode user --context mimetypes --size 256 'icons/linux_fzz_icon.png' mimetypes-x-fritzing-fzz

# 256x256
xdg-icon-resource install --mode user --context mimetypes --size 256 'icons/linux_fzb_icon.png' mimetypes-x-fritzing-fzb

# update the user mime-database to enable the new icons
update-mime-database ~/.local/share/mime

echo "installed fritzing document icons -- ignore any warnings"
