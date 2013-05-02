#!/bin/bash
arch_aux=`uname -m`

 
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
svn export http://fritzing.googlecode.com/svn/trunk/fritzing $compile_folder

# get rid of user folder contents and bins
current_dir=$(pwd)
cd $compile_folder/parts/user
rm -rf *
cd ../svg/user/breadboard
rm -rf *
cd ../schematic
rm -rf *
cd "../new schematic"
rm -rf *
cd ../pcb
rm -rf *
cd ../icon
rm -rf *
cd ..
rmdir "new schematic"
cd $current_dir
cd $compile_folder/pdb/user
rm -rf *
cd $current_dir
cd $compile_folder/bins/more
rm -rf sparkfun-*.fzb
cd $current_dir
cd $compile_folder/src/lib
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
	cp -rf $compile_folder $tarball_folder
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
#QT_HOME="/home/jonathan/qtsdk-2010.05/qt"
#QT_HOME="/usr"
#QT_HOME="/home/jonathan/QtSDK/Desktop/Qt/473/gcc"
QT_HOME="/usr/local/Trolltech/Qt-4.8.3"


$QT_HOME/bin/qmake CONFIG+=release DEFINES+=$quazip
make

release_folder="fritzing-$relname.linux.$arch"

echo "making release folder: $release_folder"
mkdir ../$release_folder

echo "copying release files"
cp -rf bins/ parts/ sketches/ help/ pdb/ Fritzing Fritzing.sh README.txt LICENSE.GPL2 LICENSE.GPL3 ../$release_folder/
cd ../$release_folder

echo "making library folders"
mkdir lib
mkdir lib/imageformats
mkdir lib/sqldrivers
mkdir translations

cd lib
echo "copying libraries"

cp $QT_HOME/lib/libQtCore.so.4 $QT_HOME/lib/libQtGui.so.4 $QT_HOME/lib/libQtNetwork.so.4 $QT_HOME/lib/libQtSql.so.4 $QT_HOME/lib/libQtSvg.so.4  $QT_HOME/lib/libQtXml.so.4 $QT_HOME/lib/libQtXmlPatterns.so.4 .

mv ../Fritzing .  				     # hide the executable in the lib folder
mv ../Fritzing.sh ../Fritzing   		# rename Fritzing.sh to Fritzing
chmod +x ../Fritzing

# libaudio seems not to be needed anymore
# if is i368 copy the libaudio
#if [ $arch == 'i386' ]
#    then
#       cp /usr/lib/libaudio.so /usr/lib/libaudio.so.2 /usr/lib/libaudio.so.2.4 .
#       echo "copying libaudio files"
#    else
#        echo "skipping libaudio files"
#fi

echo "copying plugins"
cp $QT_HOME/plugins/imageformats/libqjpeg.so imageformats
cp $QT_HOME/plugins/sqldrivers/libqsqlite.so sqldrivers

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

#echo "done!"

