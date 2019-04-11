#!/bin/bash
set -e

# Currently supported values for ${TARGET} are 'bionic' or 'trusty'
# pwd should be the repos root.
#
# Example:
#
# /home/user/git/fritzing-app:$ TARGET=trusty ./docker/docker-build.sh
#
# Step 1/7 : FROM registry.gitlab.com/fritzing/fritzing-app_playground/build:trusty
# ---> 2c4428f29501
# ....

docker build -t fritzing_${TARGET} docker/fritzing_${TARGET} --build-arg user_id=`id -u`
docker run -v "$(pwd):/fritzing" fritzing_${TARGET} qmake phoenix.pro
docker run -v "$(pwd):/fritzing" fritzing_${TARGET} make -j16

