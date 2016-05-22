#!/bin/sh
#
# this is a rought begin of a linux install script for fritzing
# with icon setup for *.fzz files and assignment to fritzing application
# the script needs to be improved and need some more file-copy in the release-script for linux
# to serve the mime-files and the icons files. 
#
# the script needs to sit in the fritzing home folder 
# and needs an icon folder with the following files:
# linux_fzz_icon.png
# linux_fzz_icon128.png
# linux_fzz_icon.icon
# x-fritzing.xml
#
#!/bin/bash
APPDIR=$(dirname "$0")

# check if user .mime.types fill exists, otherwise
# create it

if [ ! -f ~/.mime.types ]; then
	echo "file does not exists and is going to be created"
	touch ~/.mime.types
	echo "mimetypes/x-fritzing-fzz 	fritzing" >> ~/.mime.types
fi


echo $APPDIR
cd $APPDIR
pwd

# install fritzing into mime user directory
xdg-mime install --mode user 'icons/x-fritzing-fzz.xml'

# set the mime default programm to the desktop-file
xdg-mime default 'fritzing.desktop' mimetypes/x-fritzing-fzz

# install the image-fiels into the user mime system with specified size
# ~/.local/share/icons/hicolor/*size*
# 128x128
xdg-icon-resource install --mode user --context mimetypes --size 128 'icons/linux_fzz_icon128.png' mimetypes-x-fritzing-fzz
# 256x256
xdg-icon-resource install --mode user --context mimetypes --size 256 'icons/linux_fzz_icon.png' mimetypes-x-fritzing-fzz
# install fritzing into mime user directory
xdg-mime install --mode user 'icons/x-fritzing-fzz.xml'

# update the user mime-database to enable the new icons
update-mime-database ~/.local/share/mime

echo " installing MIME THEME DONE fzz should look nice now"
