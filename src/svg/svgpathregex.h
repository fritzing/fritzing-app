#ifndef SVGPATHPARSER_H
#define SVGPATHPARSER_H

#include <QRegExp>

static const QRegExp AaCc("[aAcCqQtTsS]");
static const QRegExp MFinder("([mM])\\s*([-+\\.\\d]+)[\\s,]+([-+\\.\\d]+)");
static const QRegExp MultipleZs("[zZ]\\s*[^\\s]");

#endif
