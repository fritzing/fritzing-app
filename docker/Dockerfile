FROM conanio/gcc5
# Docherfile for ^ https://github.com/conan-io/conan-docker-tools/blob/master/gcc_5/Dockerfile

ENV BOOST_ROOT /usr/local

USER root

RUN apt-get update \
    && apt-get install -y \
    libjpeg-dev \
    libncurses5-dev \
    libpng-dev \
    libqt5serialport5-dev \
    libqt5svg5-dev \
    libssl-dev \
    libudev-dev \
    libz-dev \
    python-dev \
    qt5-default

# No SHA verification for now. Will be automated by Conan in the future.
RUN set -xe \
    && wget -q -O - https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.bz2 | tar xj \
    && cd boost_1_70_0 \
    && ./bootstrap.sh --prefix=${BOOST_ROOT} --without-libraries=python \
    && ./b2 -j4 install \
    && cd .. \
    && rm -rf boost_1_70_0

RUN set -xe \
    && wget -q -O - https://github.com/libgit2/libgit2/archive/v0.28.1.tar.gz | tar xz \
    && mv libgit2-0.28.1 libgit2 \
    && mkdir libgit2/build \
    && cd libgit2/build \
    && cmake -D BUILD_SHARED_LIBS=OFF .. \
    && cmake --build . -- -j4 \
    && cd ../../


USER conan
