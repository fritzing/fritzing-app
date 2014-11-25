#!/bin/bash
QTBIN=/Users/andre/Qt5.2.1/5.2.1/clang_64/bin

tdir=`dirname $BASH_SOURCE`
cd $tdir
cd ..
workingdir=$(pwd)

echo "current working directory"
echo $workingdir

echo "building fritzing from working directory"
$QTBIN/qmake -o Makefile phoenix.pro
make release  # release is the type of build
builddir=$workingdir/../release64
echo "build directory"
echo $builddir

deploysrcdir=$workingdir/../deploy-src
echo "deploy src directory"
echo $deploysrcdir

rm -rf $deploysrcdir
mkdir $deploysrcdir

git clone --recursive https://github.com/fritzing/fritzing-app/ $deploysrcdir

deployappdir=$workingdir/../deploy-app
echo "deploy app directory"
echo $deployappdir

rm -rf $deployappdir
mkdir $deployappdir

echo "moving parts to pdb - TEMPORARY WORKAROUND"
mv $deploysrcdir/parts/contrib/* $deploysrcdir/pdb/contrib/
mv $deploysrcdir/parts/core/* $deploysrcdir/pdb/core/
mv $deploysrcdir/parts/obsolete/* $deploysrcdir/pdb/obsolete/
mv $deploysrcdir/parts/user/* $deploysrcdir/pdb/user/

echo "removing translations"
rm $deploysrcdir/translations/*.ts
find $deploysrcdir/translations -name "*.qm" -size -128c -delete

echo "still more cleaning"
rm $deploysrcdir/control
rm -rf $deploysrcdir/deploy*
rm -rf $deploysrcdir/fritzing.*
rm $deploysrcdir/Fritzing.1
rm $deploysrcdir/Fritzing.sh
rm -rf $deploysrcdir/Fritzing*.plist
rm -rf $deploysrcdir/linguist*
rm -rf $deploysrcdir/phoenix*
rm -rf $deploysrcdir/pri/*
rmdir $deploysrcdir/pri
rm -rf $deploysrcdir/resources/*
rmdir $deploysrcdir/resources
rm -rf $deploysrcdir/src/*
rmdir $deploysrcdir/src
rm -rf $deploysrcdir/tools/*
rmdir $deploysrcdir/tools

echo "mac deploy qt"
cp -r $builddir/Fritzing.app $deployappdir
$QTBIN/macdeployqt $deployappdir/Fritzing.app
cp -r $deploysrcdir/* $deployappdir/Fritzing.app/Contents/MacOS
