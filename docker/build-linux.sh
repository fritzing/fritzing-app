#!/bin/bash
set -xe

mkdir build
docker run -v "$(pwd):/home/conan/fritzing" -w /home/conan/fritzing/build fritzing/build:xenial qmake ../phoenix.pro
docker run -v "$(pwd):/home/conan/fritzing" -w /home/conan/fritzing/build fritzing/build:xenial make -j16
