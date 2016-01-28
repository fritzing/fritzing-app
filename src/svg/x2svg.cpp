/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2016 Fritzing

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#include "x2svg.h"
#include "../utils/textutils.h"
#include "svgfilesplitter.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QObject>
#include <limits>
#include <QDomDocument>
#include <QDomElement>
#include <QDateTime>
#include <qmath.h>

static const int MAX_INT = std::numeric_limits<int>::max();
static const int MIN_INT = std::numeric_limits<int>::min();

X2Svg::X2Svg() {
	m_attribute = "<fz:attr name='%1'>%2</fz:attr>\n";
	m_comment = "<fz:comment>%2</fz:comment>\n";

}

void X2Svg::initLimits() {
	m_maxX = MIN_INT;
	m_maxY = MIN_INT;
	m_minX = MAX_INT;
	m_minY = MAX_INT;
}

void X2Svg::checkXLimit(double x) {
	if (x < m_minX) m_minX = x;
	if (x > m_maxX) m_maxX = x;
}

void X2Svg::checkYLimit(double y) {
	if (y < m_minY) m_minY = y;
	if (y > m_maxY) m_maxY = y;
}

QString X2Svg::offsetMin(const QString & svg) {
	if (m_minX == 0 && m_minY == 0) return svg;

	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;
	if (!domDocument.setContent(svg, true, &errorStr, &errorLine, &errorColumn)) {
		throw QObject::tr("failure in svg conversion 1: %1 %2 %3").arg(errorStr).arg(errorLine).arg(errorColumn);
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull()) {
		throw QObject::tr("failure in svg conversion 2: %1 %2 %3").arg(errorStr).arg(errorLine).arg(errorColumn);
	}

	SvgFileSplitter splitter;
	splitter.shiftChild(root, -m_minX, -m_minY, true);
	return TextUtils::removeXMLEntities(domDocument.toString());
}

QString X2Svg::unquote(const QString & string) {
	QString s = string;
	if (s.endsWith('"')) {
		s.chop(1);
	}
	if (s.startsWith('"')) {
		s = s.remove(0, 1);
	}

	return s;
}
