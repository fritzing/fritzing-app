#!/bin/bash
set -xe

docker run -v "$(pwd):/home/conan/fritzing" -w /home/conan/fritzing fritzing/build:xenial tools/linux/docker/xvfb-release-helper.sh "$1"

