#!/bin/bash
QTBIN=/Users/andre/Qt/5.6/clang_64/bin

toolsdir=`dirname $BASH_SOURCE`
cd $toolsdir
cd ..
workingdir=$(pwd)

echo ">> working directory"
echo $workingdir

deploydir=$workingdir/../deploy-app
echo ">> deploy directory"
echo $deploydir
rm -rf $deploydir
mkdir $deploydir

builddir=$workingdir/../release64  # this is pre-defined by Qt
echo ">> build directory"
echo $builddir

echo ">> building fritzing from working directory"
$QTBIN/qmake -o Makefile phoenix.pro
make "-j$(sysctl -n machdep.cpu.thread_count)" release  # release is the type of build
cp -r $builddir/Fritzing.app $deploydir

echo ">> add .app dependencies"
cd $deploydir
$QTBIN/macdeployqt Fritzing.app -verbose=2

supportdir=$deploydir/Fritzing.app/Contents/MacOS
echo ">> support directory"
echo $supportdir

echo ">> copy support files"
cd $workingdir
cp -rf sketches help translations readme.md LICENSE.CC-BY-SA LICENSE.GPL2 LICENSE.GPL3 $supportdir/

echo ">> clean translations"
cd $supportdir
rm ./translations/*.ts  			# remove translation xml files, since we only need the binaries in the release
find ./translations -name "*.qm" -size -128c -delete   # delete empty translation binaries

echo ">> clone parts repository"
git clone --branch master --single-branch https://github.com/fritzing/fritzing-parts.git
echo ">> build parts database and run fritzing"
./Fritzing -db "fritzing-parts/parts.db"  # -pp "fritzing-parts" -f "."

echo ">> launch Fritzing"
cd $deploydir
open Fritzing.app

echo ">> done!"
