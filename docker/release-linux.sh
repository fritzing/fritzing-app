#!/bin/bash
set -xe

docker run --privileged -v "$(pwd):/home/conan/fritzing" \
    -e TRAVIS="${TRAVIS:-}" \
    -e TRAVIS_BUILD_NUMBER="${TRAVIS_BUILD_NUMBER:-}" \
    -w /home/conan/fritzing fritzing/build:"$1" docker/xvfb-release-helper.sh "$2"
