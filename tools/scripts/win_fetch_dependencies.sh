#!/bin/bash
set -xe

echo "Looking for Qt..."
if [ ! -d "/c/Qt/5.12.3/msvc2017/bin" ]; then
    echo "Qt not found in cache. Downloading..."
    curl -L \
        http://download.qt-project.org/official_releases/online_installers/qt-unified-windows-x86-online.exe \
        --output "${TRAVIS_BUILD_DIR}/qt-installer.exe"

    echo "Running Qt installer in headless mode..."
    "${TRAVIS_BUILD_DIR}/qt-installer.exe" \
        --script "${TRAVIS_BUILD_DIR}/tools/qt_installer_noninteractive.qs"
fi
ls -lah "/c/Qt/5.12.3/msvc2017/bin"

echo "Looking for LibGit2..."
LIBGIT2_DIR="${TRAVIS_BUILD_DIR}/../libgit2"
if [ ! -d "${LIBGIT2_DIR}/build64/Release/git2.lib" ]; then
    echo "LibGit2 not found in cache. Downloading..."
    curl -fsSL \
        https://github.com/libgit2/libgit2/archive/v0.28.1.zip \
        -o libgit2.zip
    7z x libgit2.zip
    mv libgit2-0.28.1 "${LIBGIT2_DIR}"

    echo "Building LibGit2..."
    mkdir "${LIBGIT2_DIR}/build64"
    cd "${LIBGIT2_DIR}/build64"
    cmake -G "Visual Studio 15 2017 Win64" "${LIBGIT2_DIR}"
    cmake --build . --config Release
    cd "${TRAVIS_BUILD_DIR}"
fi
ls -lah "${LIBGIT2_DIR}/build64/Release/"

# No point in putting Boost in cache since it would be downloading it from the cache server
# So there is no speed improvement.
echo "Fetching Boost..."
cd "${TRAVIS_BUILD_DIR}/src/lib"
curl -L \
    https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.bz2 \
    | tar xj

