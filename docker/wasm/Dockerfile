# syntax=docker/dockerfile:1
#
# Builds Fritzing for the browser (Qt for WebAssembly) and serves it with nginx.
#
#   docker build -f docker/wasm/Dockerfile -t fritzing-wasm .
#   docker run --rm -p 8080:80 fritzing-wasm      # then open http://localhost:8080
#
# For Arduino upload, start it together with the compile service: docker/wasm/compose.yaml
#
# Stages:
#   sources  downloads and verifies the third-party sources and the parts library
#   native   builds the regular desktop app, only to generate parts.db (as tools/linux_release_script does)
#   wasm     builds the WebAssembly app with the parts library preloaded
#   (final)  nginx serving the static files

ARG QT_VERSION=6.8.3
# The Emscripten version the Qt release was built with (see QT_EMCC_VERSION in Qt's mkspecs/qconfig.pri)
ARG EMSDK_VERSION=3.1.56
# A fritzing-parts commit, or "latest" for the newest one (parts are updated by rebuilding;
# add --no-cache-filter sources to fetch again)
ARG PARTS_COMMIT=27535f2fd02097be9bed229b75aa8e9be282a4a0

############################################################################
FROM --platform=$BUILDPLATFORM ubuntu:24.04 AS sources

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
       bzip2 ca-certificates curl git unzip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

RUN curl -fsSL -o boost.tar.bz2 https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.bz2 \
    && echo "7009fe1faa1697476bdc7027703a2badb84e849b7b0baad5086b087b971f8617  boost.tar.bz2" | sha256sum -c - \
    && tar xjf boost.tar.bz2 boost_1_85_0/boost \
    && rm boost.tar.bz2

RUN curl -fsSL -o svgpp.tar.gz https://github.com/svgpp/svgpp/archive/refs/tags/v1.3.1.tar.gz \
    && echo "be8a89df72d01cf062cc9815dd64c9576b4d20910d6d7aee7f0ea26484dc5e76  svgpp.tar.gz" | sha256sum -c - \
    && tar xzf svgpp.tar.gz \
    && rm svgpp.tar.gz

RUN curl -fsSL -o clipper.zip "https://sourceforge.net/projects/polyclipping/files/clipper_ver6.4.2.zip/download" \
    && echo "a14320d82194807c4480ce59c98aa71cd4175a5156645c4e2b3edd330b930627  clipper.zip" | sha256sum -c - \
    && unzip -q clipper.zip 'cpp/clipper.*' -d clipper \
    && rm clipper.zip

RUN curl -fsSL -o libgit2.tar.gz https://github.com/libgit2/libgit2/archive/refs/tags/v1.7.1.tar.gz \
    && echo "17d2b292f21be3892b704dddff29327b3564f96099a1c53b00edc23160c71327  libgit2.tar.gz" | sha256sum -c - \
    && tar xzf libgit2.tar.gz \
    && mv libgit2-1.7.1 libgit2 \
    && rm libgit2.tar.gz

# QuaZip with Qt 6 support from https://github.com/stachenov/quazip/pull/199 (see pri/quazipdetect.pri)
RUN curl -fsSL -o quazip.tar.gz https://github.com/stachenov/quazip/archive/b6943c314188873f410dfaf8a21fc765adf17825.tar.gz \
    && echo "ed34abb9732eb7f8ac97db38ff74553f9ef63f80c3866ff4127727a98d02c64b  quazip.tar.gz" | sha256sum -c - \
    && tar xzf quazip.tar.gz \
    && mv quazip-b6943c314188873f410dfaf8a21fc765adf17825 quazip \
    && rm quazip.tar.gz

RUN curl -fsSL -o ngspice.tar.gz "https://sourceforge.net/projects/ngspice/files/ng-spice-rework/old-releases/42/ngspice-42.tar.gz/download" \
    && echo "737fe3846ab2333a250dfadf1ed6ebe1860af1d8a5ff5e7803c772cc4256e50a  ngspice.tar.gz" | sha256sum -c - \
    && tar xzf ngspice.tar.gz \
    && rm ngspice.tar.gz

# A git checkout, because generating parts.db reads the commit hash with libgit2
ARG PARTS_COMMIT
RUN ref="$PARTS_COMMIT" \
    && if [ "$ref" = latest ]; then ref=HEAD; fi \
    && git init -q fritzing-parts \
    && git -C fritzing-parts fetch -q --depth 1 https://github.com/fritzing/fritzing-parts.git "$ref" \
    && git -C fritzing-parts checkout -q FETCH_HEAD \
    && echo "fritzing-parts $(git -C fritzing-parts rev-parse HEAD)"

############################################################################
FROM --platform=$BUILDPLATFORM ubuntu:24.04 AS native

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
       build-essential ca-certificates cmake fonts-dejavu-core git libdbus-1-3 libegl1 libfontconfig1 \
       libgl-dev libglib2.0-0 libngspice0-dev libssl-dev libxkbcommon0 pkg-config python3-venv zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

ARG QT_VERSION
RUN python3 -m venv /opt/aqt \
    && /opt/aqt/bin/pip install -q aqtinstall==3.2.1 \
    && case "$(uname -m)" in \
         aarch64) /opt/aqt/bin/aqt install-qt linux_arm64 desktop "$QT_VERSION" linux_gcc_arm64 -m qtserialport qtimageformats -O /opt/Qt ;; \
         *)       /opt/aqt/bin/aqt install-qt linux desktop "$QT_VERSION" linux_gcc_64 -m qtserialport qtimageformats -O /opt/Qt ;; \
       esac \
    && ln -s /opt/Qt/"$QT_VERSION"/gcc_* /opt/Qt/host
ENV PATH=/opt/Qt/host/bin:$PATH

# Dependencies go next to the source tree, where pri/*detect.pri look for them
WORKDIR /build
COPY --from=sources /src/boost_1_85_0 boost_1_85_0
COPY --from=sources /src/svgpp-1.3.1 svgpp-1.3.1
COPY --from=sources /src/clipper /src/clipper
COPY --from=sources /src/libgit2 /src/libgit2
COPY --from=sources /src/quazip /src/quazip

RUN mkdir -p ngspice-42/include && cp -r /usr/include/ngspice ngspice-42/include/

RUN mkdir -p Clipper1-6.4.2/include/polyclipping Clipper1-6.4.2/lib \
    && cp /src/clipper/cpp/clipper.hpp Clipper1-6.4.2/include/polyclipping/ \
    && g++ -O2 -fPIC -c /src/clipper/cpp/clipper.cpp -o /tmp/clipper.o \
    && ar rcs Clipper1-6.4.2/lib/libpolyclipping.a /tmp/clipper.o

RUN cmake -S /src/libgit2 -B /tmp/libgit2-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DBUILD_CLI=OFF \
       -DCMAKE_INSTALL_PREFIX=/build/libgit2-1.7.1 -DCMAKE_INSTALL_LIBDIR=lib \
    && cmake --build /tmp/libgit2-build -j"$(nproc)" \
    && cmake --install /tmp/libgit2-build \
    && rm -rf /tmp/libgit2-build

RUN cmake -S /src/quazip -B /tmp/quazip-build -DCMAKE_BUILD_TYPE=Release -DQUAZIP_QT_MAJOR_VERSION=6 \
       -DCMAKE_PREFIX_PATH=/opt/Qt/host -DQUAZIP_ENABLE_TESTS=OFF \
       -DCMAKE_INSTALL_PREFIX="/build/quazip-${QT_VERSION}-1.4intuisphere" -DCMAKE_INSTALL_LIBDIR=lib \
    && cmake --build /tmp/quazip-build -j"$(nproc)" \
    && cmake --install /tmp/quazip-build \
    && rm -rf /tmp/quazip-build

COPY --exclude=docker/wasm . /build/fritzing-app
WORKDIR /build/fritzing-app/build-native
RUN qmake ../phoenix.pro CONFIG+=release \
    && make -j"$(nproc)"

COPY --from=sources /src/fritzing-parts /build/fritzing-parts
RUN QT_QPA_PLATFORM=offscreen "$(find . -maxdepth 2 -type f -name Fritzing -perm -u+x | head -n 1)" \
       -db /build/fritzing-parts/parts.db -pp /build/fritzing-parts -f /build/fritzing-app \
    && test -s /build/fritzing-parts/parts.db

############################################################################
# emsdk publishes Linux binaries for x86_64 only, so this stage is emulated on arm64 hosts
FROM --platform=linux/amd64 ubuntu:24.04 AS wasm

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
       ca-certificates cmake git libglib2.0-0 make python3 python3-venv xz-utils \
    && rm -rf /var/lib/apt/lists/*

ARG EMSDK_VERSION
RUN git clone -q --depth 1 --branch "$EMSDK_VERSION" https://github.com/emscripten-core/emsdk.git /opt/emsdk \
    && /opt/emsdk/emsdk install "$EMSDK_VERSION" \
    && /opt/emsdk/emsdk activate "$EMSDK_VERSION"
ENV EMSDK=/opt/emsdk
ENV PATH=/opt/emsdk:/opt/emsdk/upstream/emscripten:$PATH

ARG QT_VERSION
RUN python3 -m venv /opt/aqt \
    && /opt/aqt/bin/pip install -q aqtinstall==3.2.1 \
    && /opt/aqt/bin/aqt install-qt all_os wasm "$QT_VERSION" wasm_singlethread -m qtimageformats --autodesktop -O /opt/Qt \
    && ln -s /opt/Qt/"$QT_VERSION"/wasm_singlethread /opt/Qt/wasm \
    && ln -s /opt/Qt/"$QT_VERSION"/gcc_64 /opt/Qt/host \
    && chmod +x /opt/Qt/wasm/bin/*    # the all_os package ships its scripts without the executable bit

WORKDIR /deps
COPY --from=sources /src/boost_1_85_0 boost_1_85_0
COPY --from=sources /src/svgpp-1.3.1 svgpp-1.3.1
COPY --from=sources /src/clipper /src/clipper
COPY --from=sources /src/quazip /src/quazip
COPY --from=sources /src/ngspice-42 /src/ngspice-42

# ngspice as a static library with its shared-library API (sharedspice.h), linked into the
# app. Overriding STATIC and the libngspice_la flags stops the build from adding -shared,
# which libtool refuses for Emscripten (gcc builds the XSPICE code model preprocessor, which
# runs during the build). getrusage() is switched off: Emscripten 3.1.56 clears more memory
# than its struct rusage has, which overwrites ngspice's stack (operating point results got
# lost); ngspice then uses times().
# getpwuid() returns NULL in the browser, which ngspice does not check when it looks for
# ~/.spiceinit, so that read gets a guard.
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends bison flex gcc libc6-dev \
    && rm -rf /var/lib/apt/lists/* \
    && sed -i 's|pw->pw_dir, INITSTR|pw ? pw->pw_dir : ".", INITSTR|' /src/ngspice-42/src/sharedspice.c \
    && grep -q 'pw ? pw->pw_dir : "."' /src/ngspice-42/src/sharedspice.c \
    && mkdir /tmp/ngspice-build && cd /tmp/ngspice-build \
    && emconfigure /src/ngspice-42/configure --host=wasm32-unknown-emscripten --build=x86_64-linux-gnu \
       --with-ngshared --disable-shared --enable-static --disable-openmp --disable-debug --without-x \
       --enable-xspice --disable-osdi --prefix=/deps/ngspice-42 CFLAGS="-O2" \
       ac_cv_func_getrusage=no > configure.log \
    && emmake make -j"$(nproc)" STATIC= libngspice_la_CFLAGS= libngspice_la_LDFLAGS= > make.log \
    && emmake make install STATIC= libngspice_la_CFLAGS= libngspice_la_LDFLAGS= > install.log \
    && test -f /deps/ngspice-42/lib/libngspice.a \
    && cd / && rm -rf /tmp/ngspice-build

RUN mkdir -p Clipper1-6.4.2/include/polyclipping Clipper1-6.4.2/lib \
    && cp /src/clipper/cpp/clipper.hpp Clipper1-6.4.2/include/polyclipping/ \
    && em++ -O2 -fexceptions -c /src/clipper/cpp/clipper.cpp -o /tmp/clipper.o \
    && emar rcs Clipper1-6.4.2/lib/libpolyclipping.a /tmp/clipper.o

RUN embuilder build zlib \
    && SYSROOT=/opt/emsdk/upstream/emscripten/cache/sysroot \
    && /opt/Qt/wasm/bin/qt-cmake -S /src/quazip -B /tmp/quazip-build -DCMAKE_BUILD_TYPE=Release -DQT_HOST_PATH=/opt/Qt/host \
       -DQUAZIP_QT_MAJOR_VERSION=6 -DBUILD_SHARED_LIBS=OFF -DQUAZIP_ENABLE_TESTS=OFF -DQUAZIP_BZIP2=OFF \
       -DZLIB_INCLUDE_DIR="$SYSROOT/include" -DZLIB_LIBRARY="$SYSROOT/lib/wasm32-emscripten/libz.a" \
       -DCMAKE_CXX_FLAGS="-fexceptions" \
       -DQUAZIP_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/deps/quazip -DCMAKE_INSTALL_LIBDIR=lib \
    && cmake --build /tmp/quazip-build -j"$(nproc)" \
    && cmake --install /tmp/quazip-build \
    && test -f /deps/quazip/include/QuaZip-Qt6-1.4/quazip/quazip.h \
    && rm -rf /tmp/quazip-build

COPY --exclude=docker/wasm . /build/fritzing-app

# Application data, preloaded into the in-memory file system as /fritzing
RUN mkdir -p /data/fritzing \
    && cd /build/fritzing-app \
    && /opt/Qt/host/bin/lrelease -silent phoenix.pro \
    && cp -r help sketches translations /data/fritzing/ \
    && find /data/fritzing/translations -name '*.ts' -delete \
    && find /data/fritzing/translations -name '*.qm' -size -128c -delete
COPY --from=native /build/fritzing-parts /data/fritzing/fritzing-parts
RUN rm -rf /data/fritzing/fritzing-parts/.git

ENV FRITZING_WASM_DEPS=/deps
ENV FRITZING_WASM_DATA=/data/fritzing
WORKDIR /build/fritzing-app/build-wasm
RUN /opt/Qt/wasm/bin/qmake ../phoenix.pro \
    && make -j"$(nproc)"

# index.html also loads the browser services (Web Serial, Arduino upload)
COPY docker/wasm/web/fritzing-web.js /out/
RUN cp Fritzing.html Fritzing.js Fritzing.wasm Fritzing.data qtloader.js qtlogo.svg /out/ \
    && sed 's|<script type="text/javascript" src="qtloader.js"></script>|<script type="text/javascript" src="fritzing-web.js"></script>\n&|' Fritzing.html > /out/index.html \
    && grep -q 'src="fritzing-web.js"' /out/index.html \
    && gzip -9 -k /out/Fritzing.js /out/Fritzing.wasm /out/Fritzing.data /out/qtloader.js /out/fritzing-web.js

############################################################################
FROM nginx:1.27-alpine

# the entrypoint renders templates/ into conf.d, with the container's DNS servers as resolver
ENV NGINX_ENTRYPOINT_LOCAL_RESOLVERS=1
COPY docker/wasm/templates/ /etc/nginx/templates/
COPY --from=wasm /out/ /usr/share/nginx/html/

EXPOSE 80
