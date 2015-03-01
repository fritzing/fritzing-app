#!/bin/bash
arch_aux=`uname -m`

current_dir=$(pwd)

echo ""
echo "NOTE: Don't forget to set this script's QT_HOME variable"
echo "NOTE: Execute this script from outside the fritzing-app folder"
echo ""

QT_HOME="/home/ubuntu/Qt5.2.1/5.2.1/gcc"
#QT_HOME="/home/vuser/Qt5.2.1/5.2.1/gcc_64" # 64bit version
#QT_HOME="~/Qt5.2.1/5.2.1/gcc" # doesn't work for some reason

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
git clone --recursive https://github.com/fritzing/fritzing-app $compile_folder
cd $compile_folder/parts
git submodule update --init --recursive
cd ..
cd src/lib
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
	echo "making source tarball: $tarball_folder"
	cp -rf $compile_folder $tarball_folder
	rm -rf $tarball_folder/FritzingInfo.plist
	rm -rf $tarball_folder/tools/fixfz
	tar -cjf ./$tarball_folder.tar.bz2 $tarball_folder
	rm -rf $tarball_folder
	echo "done with source tarball: $tarball_folder"

	else arch='i386'
fi

cd $compile_folder
echo "compliling... if this is not taking a long time, something is probably wrong"
$QT_HOME/bin/qmake CONFIG+=release DEFINES+=$quazip
make

release_folder="fritzing-$relname.linux.$arch"

echo "making release folder: $release_folder"
mkdir ../$release_folder

echo "copying release files"
cp -rf bins/ parts/ sketches/ help/ pdb/ Fritzing Fritzing.sh Fritzing.1 fritzing.desktop fritzing.rc fritzing.appdata.xml readme.md LICENSE.CC-BY-SA LICENSE.GPL2 LICENSE.GPL3 ../$release_folder/
cd ../$release_folder

echo "move parts into pdb folder - TEMPORARY WORKAROUND"
mv parts/contrib/* pdb/contrib/
mv parts/core/* pdb/core/
mv parts/obsolete/* pdb/obsolete/
mv parts/user/* pdb/user/

echo "making library folders"
mkdir lib
mkdir lib/imageformats
mkdir lib/sqldrivers
mkdir translations
mkdir lib/platforms

cd lib
echo "copying libraries"

cp $QT_HOME/lib/libicudata.so.51 $QT_HOME/lib/libicui18n.so.51 $QT_HOME/lib/libicuuc.so.51 $QT_HOME/lib/libicudata.so.5 $QT_HOME/lib/libQt5Concurrent.so.5 $QT_HOME/lib/libQt5Core.so.5 $QT_HOME/lib/libQt5DBus.so.5 $QT_HOME/lib/libQt5Gui.so.5 $QT_HOME/lib/libQt5Network.so.5 $QT_HOME/lib/libQt5SerialPort.so.5 $QT_HOME/lib/libQt5PrintSupport.so.5 $QT_HOME/lib/libQt5Sql.so.5 $QT_HOME/lib/libQt5Svg.so.5  $QT_HOME/lib/libQt5Xml.so.5 $QT_HOME/lib/libQt5Widgets.so.5 $QT_HOME/lib/libQt5XmlPatterns.so.5 .

mv ../Fritzing .  				     # hide the executable in the lib folder
mv ../Fritzing.sh ../Fritzing   		# rename Fritzing.sh to Fritzing
chmod +x ../Fritzing

echo "copying plugins"
cp $QT_HOME/plugins/imageformats/libqjpeg.so imageformats
cp $QT_HOME/plugins/sqldrivers/libqsqlite.so sqldrivers
cp $QT_HOME/plugins/platforms/libqxcb.so platforms

echo "copying translations"
cp ../../$compile_folder/translations/ -r ../
rm ../translations/*.ts  			# remove translation xml files, since we only need the binaries in the release
find ../translations -name "*.qm" -size -128c -delete   # delete empty translation binaries

cd ../../

echo "compressing...."
tar -cjf ./$release_folder.tar.bz2 $release_folder

echo "cleaning up"
rm -rf $release_folder
rm -rf $compile_folder

echo "done!"

