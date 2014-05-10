#!/bin/bash
arch_aux=`uname -m`

current_dir=$(pwd)

echo "don't forget to set this script's QT_HOME variable"
echo ""

if [ "$1" = "" ]
then
  echo "Usage: $0 <need a version string such as '0.6.4b' (without the quotes)>"
  exit
fi


PKG_OK=$(dpkg-query -W --showformat='${Status}\n' libboost-dev)
if [ "`expr index "$PKG_OK" installed`" -gt 0 ]
then
  echo "using installed boost library"
else
  echo "please install libboost-dev"
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

compile_folder="build-$arch_aux"
#svn export http://fritzing.googlecode.com/svn/trunk/fritzing $compile_folder
git clone https://code.google.com/p/fritzing/ $compile_folder

echo "this git checkout is only temporary until the code is pushed to the master"
cd $compile_folder/fritzing
git checkout -b qt5 origin/qt5

cd $current_dir
cd $compile_folder/fritzing/src/lib
rm -rf boost*				# depend on linux boost installation 
if [ "$quazip" == 'QUAZIP_INSTALLED' ]
then
  rm -rf quazip*
fi

cd $current_dir

#let's define some variables that we'll need to in the future
relname=$1  #`date +%Y.%m.%d`

if [ "$arch_aux" == 'x86_64' ] ; then
	arch='AMD64'
	# only creates the source tarball, when running on the 64 platform
	tarball_folder="fritzing-$relname.source"
	cp -rf $compile_folder/fritzing $tarball_folder
	rm -rf $tarball_folder/SetupAPI.Lib
	rm -rf $tarball_folder/deploy_fritzing.bat
	rm -rf $tarball_folder/FritzingInfo.plist
	rm -rf $tarball_folder/datasheets
	rm -rf $tarball_folder/not_quite_ready
	rm -rf $tarball_folder/part-gen-scripts
	rm -rf $tarball_folder/tools/artreeno
	rm -rf $tarball_folder/tools/fixfz
	rm -rf $tarball_folder/tools/gerb-merge
	rm -rf $tarball_folder/tools/qlalr
	echo "making source tarball: $tarball_folder"
	tar -cjf ./$tarball_folder.tar.bz2 $tarball_folder
	rm -rf $tarball_folder
	echo "done with source tarball: $tarball_folder"

	else arch='i386'
fi

cd $compile_folder
QT_HOME="/home/jonathan/Qt/5.2.1/gcc"
#QT_HOME="/usr"
#QT_HOME="/home/jonathan/QtSDK/Desktop/Qt/473/gcc"
#QT_HOME="/usr/local/Trolltech/Qt-4.8.3"

cd fritzing
$QT_HOME/bin/qmake CONFIG+=release DEFINES+=$quazip
make
cd ..

release_folder="fritzing-$relname.linux.$arch"

echo "making release folder: $release_folder"
mkdir ../$release_folder

echo "copying release files"
cp -rf fritzing/bins/ fritzing/parts/ fritzing/sketches/ fritzing/help/ fritzing/pdb/ fritzing/Fritzing fritzing/Fritzing.sh fritzing/README.txt fritzing/LICENSE.GPL2 fritzing/LICENSE.GPL3 ../$release_folder/
cd ../$release_folder

echo "making library folders"
mkdir lib
mkdir lib/imageformats
mkdir lib/sqldrivers
mkdir translations
mkdir lib/platforms

cd lib
echo "copying libraries"

cp $QT_HOME/lib/libicudata.so.51 $QT_HOME/lib/libicui18n.so.51 $QT_HOME/lib/libicuuc.so.51 $QT_HOME/lib/libicudata.so.5 $QT_HOME/lib/libQt5Concurrent.so.5 $QT_HOME/lib/libQt5Core.so.5 $QT_HOME/lib/libQt5DBus.so.5 $QT_HOME/lib/libQt5Gui.so.5 $QT_HOME/lib/libQt5Network.so.5 $QT_HOME/lib/libQt5PrintSupport.so.5 $QT_HOME/lib/libQt5Sql.so.5 $QT_HOME/lib/libQt5Svg.so.5  $QT_HOME/lib/libQt5Xml.so.5 $QT_HOME/lib/libQt5Widgets.so.5 $QT_HOME/lib/libQt5XmlPatterns.so.5 .

mv ../Fritzing .  				     # hide the executable in the lib folder
mv ../Fritzing.sh ../Fritzing   		# rename Fritzing.sh to Fritzing
chmod +x ../Fritzing

echo "copying plugins"
cp $QT_HOME/plugins/imageformats/libqjpeg.so imageformats
cp $QT_HOME/plugins/sqldrivers/libqsqlite.so sqldrivers
cp $QT_HOME/plugins/platforms/libqxcb.so platforms

echo "copying translations"
cp ../../$compile_folder/fritzing/translations/ -r ../
rm ../translations/*.ts  			# remove translation xml files, since we only need the binaries in the release
find ../translations -name "*.qm" -size -128c -delete   # delete empty translation binaries
cd ../../

echo "compressing...."
tar -cjf ./$release_folder.tar.bz2 $release_folder

echo "cleaning up"
rm -rf $release_folder
rm -rf $compile_folder

#echo "done!"

