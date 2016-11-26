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

#Defining new Install Path
install_path=~/.fritzing-0.9.3b/

#Check if Installation exists
if [ ! -f $install_path ]
then
	echo "Installing"
	mkdir $install_path
	cp -r ./* $install_path
	echo "Copied all files"
else
	echo "Already Installed"
fi

#Put fritzing.desktop to right place and place file for command line startup in /bin/
sudo cp ./fritzing.desktop /usr/share/applications/fritzing.desktop
sudo mv  ./fritzing /bin/fritzing
sudo cp ./icons/fritzing_icon.png /usr/share/icons/fritzing_icon.png
#Making it all executable
sudo chmod a+x /bin/fritzing
sudo chmod a+x /usr/share/applications/fritzing.desktop
echo "Set up Starter icons"

#Removing unneccesary Directory
working_dir=${PWD##*/}
echo "Removing $working_dir"
cd ..
sudo rm -r ./$working_dir
echo "Cleaned"
echo "Fritzing is now located at $install_path"
