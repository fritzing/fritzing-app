#ifndef SVGPATHPARSER_H
#define SVGPATHPARSER_H

#include <QRegularExpression>

static const QRegularExpression AaCc("[aAcCqQtTsS]");
static const QRegularExpression MFinder("([mM])\\s*([-+\\.\\d]+)[\\s,]+([-+\\.\\d]+)");
static const QRegularExpression MultipleZs("[zZ]\\s*[^\\s]");

#endif
