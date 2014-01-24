#!/bin/bash

echo please check phoenix.pro to make sure that none of the platforms are commented out in the line:
echo "    CONFIG += x86_64 x86 ppc"

QTBIN=/usr/local/Trolltech/Qt-4.8.5/bin

tdir=`dirname $BASH_SOURCE`
cd $tdir
cd ..
currentdir=$(pwd)

echo "current directory"
echo $currentdir

echo building fritzing
QTBIN/qmake -o Makefile phoenix.pro
make release

deploydir=$currentdir/../deploy
echo "deploy directory"
echo $deploydir

rm -rf $deploydir
mkdir $deploydir

git clone https://code.google.com/p/fritzing/ $deploydir

fritzingdir=$deploydir/fritzing
echo "fritzing directory"
echo $fritzingdir


echo "removing translations"
rm $fritzingdir/translations/*.ts
find $fritzingdir/translations -name "*.qm" -size -128c -delete

echo "still more cleaning"
cd $fritzingdir
rm control
cd datasheets
rm -rf *
cd ..
rmdir datasheets
rm -rf deploy*
rm -rf fritzing.*
rm Fritzing.1
rm Fritzing.sh
rm -rf Fritzing*.plist
rm -rf linguist*
cd part-gen-scripts
rm -rf *
cd ..
rmdir part-gen-scripts
cd not_quite_ready
rm -rf *
cd ..
rmdir not_quite_ready
rm -rf phoenix*
cd pri
rm -rf *
cd ..
rmdir pri
cd resources
rm -rf *
cd ..
rmdir resources
rm Setup*
cd src
rm -rf *
cd ..
rmdir src
cd tools
rm -rf *
cd ..
rmdir tools


cd $currentdir

echo mac deploy qt
cp -r ./Fritzing.app $deploydir
$QTBIN/macdeployqt $deploydir/Fritzing.app 
cp -r $fritzingdir/* $deploydir/Fritzing.app/Contents/MacOS


#/Developer/Tools/Qt/macdeployqt /Users/jonathancohen/fritzing/fritzing/deploy/fritzing/Fritzing.app 

# may need this symlink to get carbon to build
#sudo ln -s /Developer/Platforms/iPhoneOS.platform/Developer/usr/libexec/gcc/darwin/ppc /usr/libexec/gcc/darwin/ppc
