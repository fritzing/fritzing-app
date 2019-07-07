#!/bin/bash
set -xe

xvfb-run ./tools/linux_release_script/release.sh "$1"
