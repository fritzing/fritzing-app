#!/bin/bash
# don't forget to check phoenix.pro for the ppc/x86 config

tdir=`dirname $BASH_SOURCE`
cd $tdir
currentdir=$(pwd)

echo "current directory"
echo $currentdir

deploydir=$currentdir/deploy/fritzing
echo "deploy directory"
echo $deploydir

svn export https://fritzing.googlecode.com/svn/trunk/fritzing/LICENSE.GPL3 $deploydir/LICENSE.GPL3
svn export https://fritzing.googlecode.com/svn/trunk/fritzing/LICENSE.GPL2 $deploydir/LICENSE.GPL2
svn export https://fritzing.googlecode.com/svn/trunk/fritzing/LICENSE.CC-BY-SA $deploydir/LICENSE.CC-BY-SA
svn export https://fritzing.googlecode.com/svn/trunk/fritzing/README.txt $deploydir/README.txt
svn export https://fritzing.googlecode.com/svn/trunk/fritzing/translations $deploydir/translations
svn export https://fritzing.googlecode.com/svn/trunk/fritzing/bins $deploydir/bins
svn export https://fritzing.googlecode.com/svn/trunk/fritzing/sketches $deploydir/sketches
svn export https://fritzing.googlecode.com/svn/trunk/fritzing/parts $deploydir/parts
svn export https://fritzing.googlecode.com/svn/trunk/fritzing/help $deploydir/help
svn export https://fritzing.googlecode.com/svn/trunk/fritzing/pdb $deploydir/pdb
rm $deploydir/translations/*.ts
find $deploydir/translations -name "*.qm" -size -128c -delete

cd $deploydir/parts/user
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

cd $deploydir/pdb/user
rm -rf *

cd $deploydir/bins/more
rm -rf sparkfun-*.fzb

cd $currentdir


/usr/local/Trolltech/Qt-4.8.3/bin/macdeployqt $deploydir/Fritzing.app 


#/Users/jonathancohen/qt-everywhere-opensource-src-4.7.3/bin/macdeployqt /Users/jonathancohen/fritzing/fritzing/deploy/fritzing/Fritzing.app 
#/Users/jonathancohen/Downloads/qt-everywhere-opensource-src-4.8.0-tp/bin/macdeployqt /Users/jonathancohen/fritzing/fritzing/deploy/fritzing/Fritzing.app 
#/Developer/Tools/Qt/macdeployqt /Users/jonathancohen/fritzing/fritzing/deploy/fritzing/Fritzing.app 

# may need this symlink to get carbon to build
#sudo ln -s /Developer/Platforms/iPhoneOS.platform/Developer/usr/libexec/gcc/darwin/ppc /usr/libexec/gcc/darwin/ppc
