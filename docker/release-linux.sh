#!/bin/bash
set -xe

docker run -v "$(pwd):/home/conan/fritzing" -w /home/conan/fritzing fritzing/build:"$1" docker/xvfb-release-helper.sh "$2"
