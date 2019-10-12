#!/bin/bash
set -xe

mkdir build
apt-get install libssh2-1-dev libcurl4-openssl-dev libhttp-parser-dev
docker run -v "$(pwd):/home/conan/fritzing" -w /home/conan/fritzing/build fritzing/build:bionic qmake ../phoenix.pro
docker run -v "$(pwd):/home/conan/fritzing" -w /home/conan/fritzing/build fritzing/build:bionic make -j16
