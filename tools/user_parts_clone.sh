#!/bin/sh
#
# enable user to override installed parts library with GitHub clone

REPODIR="${HOME}/.local/share/fritzing/fritzing-parts"
mkdir -p "${REPODIR}"
git clone --branch master --single-branch https://github.com/fritzing/fritzing-parts.git "${REPODIR}" || exit 1
Fritzing -db "${REPODIR}/parts.db"
