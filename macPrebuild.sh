#!/bin/sh

BOOSTURL="https://dl.bintray.com/boostorg/release/1.76.0/source/boost_1_76_0.tar.gz"
BOOSTFNAME=${BOOSTURL##*/}
LIBGIT2URL="https://github.com/libgit2/libgit2/releases/download/v0.28.5/libgit2-0.28.5.tar.gz"
LIBGIT2FNAME=${LIBGIT2URL##*/}

echo Downloading boost and libgit2 sources and placing them in ../
cd ..
curl -L -O -# $BOOSTURL
tar xzf $BOOSTFNAME
# Nothing to do with boost - it'll get found by fritzing when Makefiles are built

# Next: libgit2 (v 0.28.5)
curl -L -O -# $LIBGIT2URL
tar xzf $LIBGIT2FNAME
mv ${LIBGIT2FNAME%.tar.gz} libgit2
mkdir libgit2/build
cd libgit2/build

cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" ..
# Only ARM64 - cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_OSX_ARCHITECTURES="arm64" ..
# Only x86_64 - cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_OSX_ARCHITECTURES="x86_64" ..
cmake --build .

# this will look at the output and see that it is arm64 based.
#echo Checking to see the final .a file is arm64 or arm64 and x86_64
#lipo -info libgit2.a

# Ready for qmake phoenix.pro / Qt Creator build.
cd ../../fritzing-app
echo Next step - Create Makefiles from phoenix.pro via 'qmake'. Default is x86_64 build.
echo "            Universal build not available due to Qt5 issues as of April 2021."
echo For arm64: Modify phoenix.pro 'QMAKE_APPLE_DEVICE_ARCHS=arm64'
echo "           Then run qmake from your arm64 Qt5 build - qtbase/bin/qmake phoenix.pro"
echo Finally build: /Applications/Xcode.app/Contents/Developer/usr/bin/make -f Makefile.Release

