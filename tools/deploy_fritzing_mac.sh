#!/bin/bash
QTBIN=/Users/andre/Qt5.2.1/5.2.1/clang_64/bin

tdir=`dirname $BASH_SOURCE`
cd $tdir
cd ..
currentdir=$(pwd)

echo "current directory"
echo $currentdir

echo building fritzing
$QTBIN/qmake -o Makefile phoenix.pro
make release  # release is the type of build
releasedir=$currentdir/../release64

deploydir=$currentdir/../deploy-src
echo "deploy directory"
echo $deploydir
echo $releasedir

rm -rf $deploydir
mkdir $deploydir

git clone --recursive https://github.com/fritzing/fritzing-app/ $deploydir

fritzingdir=$currentdir/../deploy-app
echo "fritzing directory"
echo $fritzingdir


echo "removing translations"
rm $fritzingdir/translations/*.ts
find $fritzingdir/translations -name "*.qm" -size -128c -delete

echo "still more cleaning"
rm $fritzingdir/control
rm -rf $fritzingdir/deploy*
rm -rf $fritzingdir/fritzing.*
rm $fritzingdir/Fritzing.1
rm $fritzingdir/Fritzing.sh
rm -rf $fritzingdir/Fritzing*.plist
rm -rf $fritzingdir/linguist*
rm -rf $fritzingdir/phoenix*
rm -rf $fritzingdir/pri/*
rmdir $fritzingdir/pri
rm -rf $fritzingdir/resources/*
rmdir $fritzingdir/resources
rm -rf $fritzingdir/src/*
rmdir $fritzingdir/src
rm -rf $fritzingdir/tools/*
rmdir $fritzingdir/tools

echo mac deploy qt
cp -r $releasedir/Fritzing.app $deploydir
$QTBIN/macdeployqt $deploydir/Fritzing.app
cp -r $fritzingdir/* $deploydir/Fritzing.app/Contents/MacOS
