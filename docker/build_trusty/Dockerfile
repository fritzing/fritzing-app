# A new image must to be pushed, to make changes to this file valid:
# docker build -t registry.gitlab.com/fritzing/fritzing-app_playground/build:trusty docker/build_trusty
# docker push registry.gitlab.com/fritzing/fritzing-app_playground/build:trusty

FROM ubuntu:trusty-20190305

MAINTAINER Kjell kjell@gmx.de

RUN apt-get update -y \
    && apt-get install -y \
        bison \
        build-essential \
        cmake \
        flex \
        genparse \
        git \
        libjpeg-dev \
        libncurses5-dev \
        libpng-dev \
        libqt5serialport5-dev \
        libqt5svg5-dev \
        libssl-dev \
        libudev-dev \
        libz-dev \
        python-dev \
        qt5-default \
        wget

ENV BOOST_ROOT /usr/local

RUN cd /home && wget -q -O - https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.bz2 | tar xj

RUN cd /home/boost_1_70_0 \
  && ./bootstrap.sh --prefix=${BOOST_ROOT} \
  && ./b2 -j 16 install \
  && cd /home \
  && rm -rf boost_1_70_0

RUN cd / && wget -q -O - https://github.com/libgit2/libgit2/archive/v0.28.1.tar.gz | tar xz && mv libgit2-0.28.1 libgit2

RUN cd /libgit2 \
  && mkdir build \
  && cd build \
  && cmake -D BUILD_SHARED_LIBS=OFF .. \
  && cmake --build . -- -j16 
