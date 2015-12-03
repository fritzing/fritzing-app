#!/bin/bash
arch_aux=`uname -m`

echo
script_path="$(readlink -f "$0")"
script_folder=$(dirname $script_path)
echo "script_folder $script_folder"

current_dir=$(pwd)

echo ""
echo "NOTE: Don't forget to set this script's QT_HOME variable"
echo "      Execute this script from outside the fritzing-app folder"
echo "      But script now operates from fritzing-app folder, rather than cloning a new one and operating from there"
echo ""

QT_HOME="/home/jonathan/Qt/5.5/gcc_64"
#QT_HOME="/home/vuser/Qt5.2.1/5.2.1/gcc_64" # 64bit version
#QT_HOME="~/Qt5.2.1/5.2.1/gcc" # doesn't work for some reason

if [ "$1" = "" ]
then
  echo "Usage: $0 <need a version string such as '0.6.4b' (without the quotes)>"
  exit
fi

PKG_OK=$(dpkg-query -W --showformat='${Status}\n' libquazip-dev)
quazip='QUAZIP_LIB'
if [ "`expr index "$PKG_OK" installed`" -gt 0 ]
then
  quazip='QUAZIP_INSTALLED'
  echo "using installed quazip"
else
  echo "using src/lib/quazip"
fi

#let's define some variables that we'll need to use in the future
relname=$1  #`date +%Y.%m.%d`

if [ "$arch_aux" == 'x86_64' ] ; then
	arch='AMD64'
	else arch='i386'
fi

app_folder=$(dirname $script_folder)
app_folder=$(dirname $app_folder)
cd $app_folder
echo "appfolder $app_folder"

echo "compiling... if this is not taking a long time, something is probably wrong"
$QT_HOME/bin/qmake CONFIG+=release DEFINES+=$quazip
make

release_name=fritzing-$relname.linux.$arch
release_folder="$current_dir/$release_name"
echo "release folder $release_folder"

echo "making release folder: $release_folder"
mkdir $release_folder

echo "copying release files"
cp -rf  sketches/ help/ translations/ Fritzing.sh Fritzing.1 fritzing.desktop fritzing.rc fritzing.appdata.xml readme.md LICENSE.CC-BY-SA LICENSE.GPL2 LICENSE.GPL3 $release_folder/
mv Fritzing $release_folder/
cd $release_folder
echo "cleaning translations"

rm ./translations/*.ts  			# remove translation xml files, since we only need the binaries in the release
find ./translations -name "*.qm" -size -128c -delete   # delete empty translation binaries

git clone --depth 1 https://github.com/fritzing/fritzing-parts.git

echo "making library folders"
mkdir lib
mkdir lib/imageformats
mkdir lib/sqldrivers
mkdir lib/platforms

cd lib
echo "copying qt libraries"

cp $QT_HOME/lib/libicudata.so.* $QT_HOME/lib/libicui18n.so.* $QT_HOME/lib/libicuuc.so.* $QT_HOME/lib/libQt5Concurrent.so.5 $QT_HOME/lib/libQt5Core.so.5 $QT_HOME/lib/libQt5DBus.so.5 $QT_HOME/lib/libQt5Gui.so.5 $QT_HOME/lib/libQt5Network.so.5 $QT_HOME/lib/libQt5SerialPort.so.5 $QT_HOME/lib/libQt5PrintSupport.so.5 $QT_HOME/lib/libQt5Sql.so.5 $QT_HOME/lib/libQt5Svg.so.5  $QT_HOME/lib/libQt5Xml.so.5 $QT_HOME/lib/libQt5Widgets.so.5 $QT_HOME/lib/libQt5XmlPatterns.so.5 .

echo "copying qt plugins"
cp $QT_HOME/plugins/imageformats/libqjpeg.so imageformats
cp $QT_HOME/plugins/sqldrivers/libqsqlite.so sqldrivers
cp $QT_HOME/plugins/platforms/libqxcb.so platforms

echo "copying libgit2"
cp $app_folder/../libgit2/build/libgit2.so* .

mv ../Fritzing .  				# hide the executable in the lib folder
mv ../Fritzing.sh ../Fritzing   		# rename Fritzing.sh to Fritzing
chmod +x ../Fritzing

LD_LIBRARY_PATH=$(pwd)
export LD_LIBRARY_PATH
./Fritzing -db "$release_folder/fritzing-parts/parts.db" -pp "$release_folder/fritzing-parts" -f "$release_folder"

cd $current_dir

echo "compressing...."
tar -cjf ./$release_name.tar.bz2 $release_name

echo "cleaning up"
rm -rf $release_folder

echo "done!"

