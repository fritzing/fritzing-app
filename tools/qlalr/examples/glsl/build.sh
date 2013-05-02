#!/bin/sh

${FLEX-flex} -oglsl-lex.incl glsl-lex.l
${QLALR-qlalr} glsl.g

qmake
make
