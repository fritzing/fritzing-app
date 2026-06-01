#!/bin/bash
#
# Local Qt 6 build helper for Fritzing.
#
# Builds the application against a host Qt 6 installation. It does NOT
# build the sibling dependencies (libgit2, quazip_qt6) or clone the parts
# repository -- use docker/Dockerfile.qt6 for a fully self-contained build.
#
# The Qt 6 qmake binary is auto-detected; override it explicitly with:
#   QMAKE6=/path/to/qmake6 ./docker/build-linux-qt6.sh
#
set -euo pipefail

# Resolve the repository root regardless of where the script is invoked.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Locate a Qt 6 qmake. Distributions disagree on the name/path:
#   - Debian/Ubuntu ship "qmake6" on PATH
#   - openSUSE ships it as "qmake" under /usr/lib64/qt6/bin
find_qmake6() {
    if [[ -n "${QMAKE6:-}" ]]; then
        echo "${QMAKE6}"
        return 0
    fi
    for candidate in qmake6 /usr/lib64/qt6/bin/qmake /usr/lib/qt6/bin/qmake; do
        if command -v "${candidate}" >/dev/null 2>&1; then
            # Confirm it is actually Qt 6 before accepting it.
            if "${candidate}" -query QT_VERSION 2>/dev/null | grep -q '^6\.'; then
                echo "${candidate}"
                return 0
            fi
        fi
    done
    return 1
}

QMAKE="$(find_qmake6)" || {
    echo "error: could not find a Qt 6 qmake. Set QMAKE6=/path/to/qmake6" >&2
    exit 1
}

echo "Using Qt 6 qmake: ${QMAKE} ($(${QMAKE} -query QT_VERSION))"

BUILD_DIR="${REPO_ROOT}/build-qt6"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

"${QMAKE}" "${REPO_ROOT}/phoenix.pro" CONFIG+=debug
make -j"$(nproc)"

# On Linux qmake sets no DESTDIR, so the binary is left in the build dir.
echo "Build completed successfully."
echo "Binary location: ${BUILD_DIR}/Fritzing"
