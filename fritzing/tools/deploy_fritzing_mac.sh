#!/bin/bash

echo please check phoenix.pro to make sure that only x86_64 exists in the line:
echo "    CONFIG += x86_64 #x86 ppc"

QTBIN=/Users/jonathancohen/Qt/5.2.1/clang_64/bin

tdir=`dirname $BASH_SOURCE`
cd $tdir
cd ..
currentdir=$(pwd)

echo "current directory"
echo $currentdir

echo building fritzing
$QTBIN/qmake -o Makefile phoenix.pro
make release  # release is the type of build, not the 
releasedir=$currentdir/../../release64

deploydir=$currentdir/../../deploy
echo "deploy directory"
echo $deploydir
echo $releasedir

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
rm $fritzingdir/control
rm -rf $fritzingdir/datasheets/*
rmdir $fritzingdir/datasheets
rm -rf $fritzingdir/deploy*
rm -rf $fritzingdir/fritzing.*
rm $fritzingdir/Fritzing.1
rm $fritzingdir/Fritzing.sh
rm -rf $fritzingdir/Fritzing*.plist
rm -rf $fritzingdir/linguist*
rm -rf $fritzingdir/part-gen-scripts/*
rmdir $fritzingdir/part-gen-scripts
rm -rf $fritzingdir/not_quite_ready/*
rmdir $fritzingdir/not_quite_ready
rm -rf $fritzingdir/phoenix*
rm -rf $fritzingdir/pri/*
rmdir $fritzingdir/pri
rm -rf $fritzingdir/resources/*
rmdir $fritzingdir/resources
rm $fritzingdir/Setup*
rm -rf $fritzingdir/src/*
rmdir $fritzingdir/src
rm -rf $fritzingdir/tools/*
rmdir $fritzingdir/tools

echo mac deploy qt
cp -r $releasedir/Fritzing.app $deploydir
$QTBIN/macdeployqt $deploydir/Fritzing.app 
cp -r $fritzingdir/* $deploydir/Fritzing.app/Contents/MacOS


#/Developer/Tools/Qt/macdeployqt /Users/jonathancohen/fritzing/fritzing/deploy/fritzing/Fritzing.app 

# may need this symlink to get carbon to build
#sudo ln -s /Developer/Platforms/iPhoneOS.platform/Developer/usr/libexec/gcc/darwin/ppc /usr/libexec/gcc/darwin/ppc
