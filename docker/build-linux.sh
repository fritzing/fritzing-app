#!/bin/bash
set -xe

mkdir build
docker run -v "$(pwd):/home/conan/fritzing" -w /home/conan/fritzing/build fritzing/build:bionic qmake ../phoenix.pro
docker run -v "$(pwd):/home/conan/fritzing" -w /home/conan/fritzing/build fritzing/build:bionic make -j16
