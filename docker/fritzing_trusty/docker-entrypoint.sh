#!/bin/bash
set -e

ls
cd /fritzing
qmake phoenix.pro
make

# make test
# echo "Tests ran successful!"
