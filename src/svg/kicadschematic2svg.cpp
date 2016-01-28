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

#include "kicadschematic2svg.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"
#include "../version/version.h"
#include "../debugdialog.h"
#include "../viewlayer.h"
#include "../fsvgrenderer.h"
#include "../utils/misc.h"

#include <QFile>
#include <QTextStream>
#include <QObject>
#include <QDomDocument>
#include <QDomElement>
#include <qmath.h>
#include <limits>

// TODO:
//		pin shape: invert, etc.

KicadSchematic2Svg::KicadSchematic2Svg() : Kicad2Svg() {
}

QStringList KicadSchematic2Svg::listDefs(const QString & filename) {
	QStringList defs;

	QFile file(filename);
	if (!file.open(QFile::ReadOnly)) return defs;

	QTextStream textStream(&file);
	while (true) {
		QString line = textStream.readLine();
		if (line.isNull()) break;

		if (line.startsWith("DEF")) {
			QStringList linedefs = line.split(" ", QString::SkipEmptyParts);
			if (linedefs.count() > 1) {
				defs.append(linedefs[1]);
			}
		}
	}

	return defs;
}

QString KicadSchematic2Svg::convert(const QString & filename, const QString & defName) 
{
	initLimits();

	QFile file(filename);
	if (!file.open(QFile::ReadOnly)) {
		throw QObject::tr("unable to open %1").arg(filename);
	}

	QTextStream textStream(&file);

	QString metadata = makeMetadata(filename, "schematic part", defName);
	metadata += endMetadata();

	QString reference;
        int textOffset = 0;
        bool drawPinNumber = true;
        bool drawPinName = true;
	bool gotDef = false;
	while (true) {
		QString line = textStream.readLine();
		if (line.isNull()) {
			break;
		}

		if (line.startsWith("DEF") && line.contains(defName, Qt::CaseInsensitive)) {
			QStringList defs = splitLine(line);
			if (defs.count() < 8) {
				throw QObject::tr("bad schematic definition %1").arg(filename);
			}
			reference = defs[2];
			textOffset = defs[4].toInt();
			drawPinName = defs[6] == "Y";
			drawPinNumber = defs[5] == "Y";
			gotDef = true;
			break;
		}
	}

	if (!gotDef) {
		throw QObject::tr("schematic part %1 not found in %2").arg(defName).arg(filename);
	}

	QString contents = "<g id='schematic'>\n";
	bool inFPLIST = false;
	while (true) {
		QString fline = textStream.readLine();
		if (fline.isNull()) {
			throw QObject::tr("schematic %1 unexpectedly ends (1) in %2").arg(defName).arg(filename);
		}

		if (fline.contains("ENDDEF")) {
			throw QObject::tr("schematic %1 unexpectedly ends (2) in %2").arg(defName).arg(filename);
		}

		if (fline.startsWith("DRAW")) {
			break;
		}

		if (fline.startsWith("ALIAS")) continue;

		if (fline.startsWith("F")) {
			contents += convertField(fline);
			continue;
		}

		if (fline.startsWith("$FPLIST")) {
			inFPLIST = true;
			break;
		}
	}

	while (inFPLIST) {
		QString fline = textStream.readLine();
		if (fline.isNull()) {
			throw QObject::tr("schematic %1 unexpectedly ends (1) in %2").arg(defName).arg(filename);
		}

		if (fline.startsWith("$ENDFPLIST")) {
			inFPLIST = false;
			break;
		}

		if (fline.contains("ENDDEF")) {
			throw QObject::tr("schematic %1 unexpectedly ends (2) in %2").arg(defName).arg(filename);
		}
	}

	int pinIndex = 0;
	while (true) {
		QString line = textStream.readLine();
		if (line.isNull()) {
			throw QObject::tr("schematic %1 unexpectedly ends (3) in %2").arg(defName).arg(filename);
		}

		if (line.startsWith("DRAW")) {
			continue;
		}

		if (line.contains("ENDDEF")) {
			break;
		}
		if (line.contains("ENDDRAW")) {
			break;
		}

		if (line.startsWith("S")) {
			contents += convertRect(line);
		}
		else if (line.startsWith("X")) {
			// need to look at them all before formatting (I think)
			contents += convertPin(line, textOffset, drawPinName, drawPinNumber, pinIndex++);
		}
		else if (line.startsWith("C")) {
			contents += convertCircle(line);
		}
		else if (line.startsWith("P")) {
			contents += convertPoly(line);
		}
		else if (line.startsWith("A")) {
			contents += convertArc(line);
		}
		else if (line.startsWith("T")) {
			contents += convertText(line);
		}
		else {
			DebugDialog::debug("Unknown line " + line);
		}
	}

	contents += "</g>\n";

	QString svg = TextUtils::makeSVGHeader(GraphicsUtils::StandardFritzingDPI, GraphicsUtils::StandardFritzingDPI, m_maxX - m_minX, m_maxY - m_minY) 
					+ m_title + m_description + metadata + offsetMin(contents) + "</svg>";

	return svg;
}

QString KicadSchematic2Svg::convertText(const QString & line) {
	QStringList fs = splitLine(line);
	if (fs.count() < 8) {
		DebugDialog::debug("bad text " + line);
		return "";
	}

	return convertField(fs[2], fs[3], fs[4], fs[1], "C", "C", fs[8]);
}

QString KicadSchematic2Svg::convertField(const QString & line) {
	QStringList fs = splitLine(line);
	if (fs.count() < 7) {
		DebugDialog::debug("bad field " + line);
		return "";
	}

	if (fs[6] == "I") {
		// invisible
		return "";
	}

	while (fs.count() < 9) {
		fs.append("");
	}

	return convertField(fs[2], fs[3], fs[4], fs[5], fs[7], fs[8], fs[1]);
}

QString KicadSchematic2Svg::convertField(const QString & xString, const QString & yString, const QString & fontSizeString, const QString &orientation, 
					 const QString & hjustify, const QString & vjustify, const QString & t)
{
	QString text = t;
	bool notName = false;
	if (text.startsWith("~")) {
		notName = true;
		text.remove(0, 1);
	}

	int x = xString.toInt();
	int y = -yString.toInt();						// KiCad flips y-axis w.r.t. svg
	int fontSize = fontSizeString.toInt();

	bool rotate = (orientation == "V");
	QString rotation;
	QMatrix m;
	if (rotate) {
		m = QMatrix().translate(-x, -y) * QMatrix().rotate(-90) * QMatrix().translate(x, y);
		// store x, y, and r so they can be shifted correctly later
		rotation = QString("transform='%1' _x='%2' _y='%3' _r='-90'").arg(TextUtils::svgMatrix(m)).arg(x).arg(y);
	}

	QFont font;
	font.setFamily(OCRAFontName);
	font.setWeight(QFont::Normal);
	font.setPointSizeF(72.0 * fontSize / GraphicsUtils::StandardFritzingDPI);

	QString style;
	if (vjustify.contains("I")) {
		style += "font-style='italic' ";
		font.setStyle(QFont::StyleItalic);
	}
	if (vjustify.endsWith("B")) {
		style += "font-weight='bold' ";
		font.setWeight(QFont::Bold);
	}
	QString anchor = "middle";
	if (vjustify.startsWith("T")) {
		anchor = "end";
	}
	else if (vjustify.startsWith("B")) {
		anchor = "start";
	}
	if (hjustify.contains("L")) {
		anchor = "start";
	}
	else if (hjustify.contains("R")) {
		anchor = "end";
	}

	QFontMetricsF metrics(font);
	QRectF bri = metrics.boundingRect(text);

	// convert back to 1000 dpi
	QRectF brf(0, 0,
			   bri.width() * GraphicsUtils::StandardFritzingDPI / GraphicsUtils::SVGDPI, 
			   bri.height() * GraphicsUtils::StandardFritzingDPI / GraphicsUtils::SVGDPI);

	if (anchor == "start") {
		brf.translate(x, y - (brf.height() / 2));
	}
	else if (anchor == "end") {
		brf.translate(x - brf.width(), y - (brf.height() / 2));
	}
	else if (anchor == "middle") {
		brf.translate(x - (brf.width() / 2), y - (brf.height() / 2));
	}

	if (rotate) {
		brf = m.map(QPolygonF(brf)).boundingRect();
	}

	checkXLimit(brf.left());
	checkXLimit(brf.right());
	checkYLimit(brf.top());
	checkYLimit(brf.bottom());

	QString s = QString("<text x='%1' y='%2' font-size='%3' font-family='%8' stroke='none' fill='#000000' text-anchor='%4' %5 %6>%7</text>\n")
					.arg(x)		
					.arg(y + (fontSize / 3))		
					.arg(fontSize)		
					.arg(anchor)
					.arg(style)
					.arg(rotation)
					.arg(TextUtils::escapeAnd(unquote(text)))
                    .arg(OCRAFontName)
                    ;		
	if (notName) {
		s += QString("<line fill='none' stroke='#000000' x1='%1' y1='%2' x2='%3' y2='%4' stroke-width='2' />\n")
			.arg(brf.left())
			.arg(brf.top())
			.arg(rotate ? brf.left() : brf.right())
			.arg(rotate ? brf.bottom() : brf.top());
	}
	return s;
}

QString KicadSchematic2Svg::convertRect(const QString & line) 
{
	QStringList s = splitLine(line);
	if (s.count() < 8) {
		DebugDialog::debug(QString("bad rectangle %1").arg(line));
		return "";
	}

	if (s.count() < 9) {
		s.append("N");				// assume it's unfilled
	}

	int x = s[1].toInt();
	int y = -s[2].toInt();					// KiCad flips y-axis w.r.t. svg
	int x2 = s[3].toInt();
	int y2 = -s[4].toInt();					// KiCad flips y-axis w.r.t. svg

	checkXLimit(x);
	checkXLimit(x2);
	checkYLimit(y);
	checkYLimit(y2);

	QString rect = QString("<rect x='%1' y='%2' width='%3' height='%4' ")
			.arg(qMin(x, x2))
			.arg(qMin(y, y2))
			.arg(qAbs(x2 - x))
			.arg(qAbs(y2 - y));

	rect += addFill(line, s[8], s[7]);
	rect += " />\n";
	return rect;
}

QString KicadSchematic2Svg::convertPin(const QString & line, int textOffset, bool drawPinName, bool drawPinNumber, int pinIndex) 
{
	QStringList l = splitLine(line);
	if (l.count() < 12) {
		DebugDialog::debug(QString("bad line %1").arg(line));
		return "";
	}

	if (l[6].length() != 1) {
		DebugDialog::debug(QString("bad orientation %1").arg(line));
		return "";
	}

	if (l.count() > 12 && l[12] == "N") {
		// don't draw this
		return "";
	}

	int unit = l[9].toInt();
	if (unit > 1) {
		// don't draw this
		return "";
	}

	QChar orientation = l[6].at(0);
	QString name = l[1];
	if (name == "~") {
		name = "";
	}
	bool pinNumberOK;
	int pinNumber = l[2].toInt(&pinNumberOK);
	if (!pinNumberOK) {
		pinNumber = pinIndex;
	}
	int nFontSize = l[7].toInt();
	int x1 = l[3].toInt();
	int y1 = -l[4].toInt();							// KiCad flips y-axis w.r.t. svg
	int length = l[5].toInt();
	int x2 = x1;
	int y2 = y1;
	int x3 = x1;
	int y3 = y1;
	int x4 = x1;
	int y4 = y1;
	QString justify = "C";
	bool rotate = false;
	switch (orientation.toLatin1()) {
		case 'D':
			y2 = y1 + length;
			y3 = y1 + (length / 2);
			if (textOffset == 0) {
				x3 += nFontSize / 2;
				x4 -= nFontSize / 2;
				y4 = y3;
				justify = "C";
			}
			else {
				x3 -= nFontSize / 2;
				y4 = y2;
				justify = "R";			}
			rotate = true;
			break;
		case 'U':
			y2 = y1 - length;
			y3 = y1 - (length / 2);
			if (textOffset == 0) {
				x3 += nFontSize / 2;
				x4 -= nFontSize / 2;
				y4 = y3;
				justify = "C";
			}
			else {
				x3 -= nFontSize / 2;
				y4 = y2;
				justify = "L";
			}
			rotate = true;
			break;
		case 'L':
			x2 = x1 - length;
			x3 = x1 - (length / 2);
			if (textOffset == 0) {
				y3 += nFontSize / 2;
				y4 -= nFontSize / 2;
				x4 = x3;
				justify = "C";
			}
			else {
				y3 -= nFontSize / 2;
				x4 = x2;
				justify = "R";
			}
			break;
		case 'R':
			x2 = x1 + length;
			x3 = x1 + (length / 2);
			if (textOffset == 0) {
				y3 += nFontSize / 2;
				y4 -= nFontSize / 2;
				x4 = x3;
				justify = "C";
			}
			else {
				y3 -= nFontSize / 2;
				x4 = x2;
				justify = "L";
			}
			break;
		default:
			DebugDialog::debug(QString("bad orientation %1").arg(line));
			break;
	}

	checkXLimit(x1);
	checkXLimit(x2);
	checkYLimit(y1);
	checkYLimit(y2);

	int thickness = 1;
	
	QString pin = QString("<line fill='none' stroke='#000000' x1='%1' y1='%2' x2='%3' y2='%4' stroke-width='%5' id='connector%6pin' connectorname='%7' />\n")
			.arg(x1)
			.arg(y1)
			.arg(x2)
			.arg(y2)
			.arg(thickness)
			.arg(pinNumber)
			.arg(TextUtils::escapeAnd(name));

	pin += QString("<rect fill='none' x='%1' y='%2' width='0' height='0' stroke-width='0' id='connector%3terminal'  />\n")
			.arg(x1)
			.arg(y1)
			.arg(pinNumber);


	if (drawPinNumber) {
		pin += convertField(QString::number(x3), QString::number(-y3), l[7], rotate ? "V" : "H", "C", "C", l[2]);
	}
	if (drawPinName && !name.isEmpty()) {
		pin += convertField(QString::number(x4), QString::number(-y4), l[8], rotate ? "V" : "H", justify, "C", name);
	}

	return pin;
}

QString KicadSchematic2Svg::convertCircle(const QString & line) 
{
	QStringList s = splitLine(line);
	if (s.count() < 8) {
		DebugDialog::debug(QString("bad circle %1").arg(line));
		return "";
	}

	int x = s[1].toInt();
	int y = -s[2].toInt();					// KiCad flips y-axis w.r.t. svg
	int r = s[3].toInt();

	checkXLimit(x + r);
	checkXLimit(x - r);
	checkYLimit(y + r);
	checkYLimit(y - r);

	QString circle = QString("<circle cx='%1' cy='%2' r='%3' ")
			.arg(x)
			.arg(y)
			.arg(r);

	circle += addFill(line, s[7], s[6]);
	circle += " />\n";
	return circle;
}

QString KicadSchematic2Svg::convertArc(const QString & line) 
{
	QStringList s = splitLine(line);
	if (s.count() == 9) {
		s.append("N");			// assume unfilled
	}

	bool calcPoints = false;
	if (s.count() == 10) {
		s.append("N");
		s.append("N");
		s.append("N");
		s.append("N");
		calcPoints = true;
	}

	if (s.count() < 14) {
		DebugDialog::debug("bad arc " + line);
		return "";
	}


	int x = s[1].toInt();
	int y = -s[2].toInt();					// KiCad flips y-axis w.r.t. svg
	int r = s[3].toInt();
	double startAngle = (s[4].toInt() % 3600) / 10.0;
	double endAngle = (s[5].toInt() % 3600) / 10.0;

	double x1 = s[10].toInt();
	double y1 = -s[11].toInt();					// KiCad flips y-axis w.r.t. svg
	double x2 = s[12].toInt();
	double y2 = -s[13].toInt();					// KiCad flips y-axis w.r.t. svg

	if (calcPoints) {
		x1 = x + (r * cos(startAngle * M_PI / 180.0));
		y1 = y - (r * sin(startAngle * M_PI / 180.0));
		x2 = x + (r * cos(endAngle * M_PI / 180.0));
		y2 = y - (r * sin(endAngle * M_PI / 180.0));
	}

	// kicad arcs will always sweep < 180, kicad uses multiple arcs for > 180 sweeps
	double diffAngle = endAngle - startAngle;
	if (diffAngle > 180) diffAngle -= 360;
	else if (diffAngle < -180) diffAngle += 360;

	// TODO: use actual bounding box of arc for clipping
	checkXLimit(x + r);
	checkXLimit(x - r);
	checkYLimit(y + r);
	checkYLimit(y - r);

	QString arc = QString("<path d='M%1,%2a%3,%4 0 %5,%6 %7,%8' ")
			.arg(x1)
			.arg(y1)
			.arg(r)
			.arg(r)
			.arg(qAbs(diffAngle) >= 180 ? 1 : 0)  
			.arg(diffAngle > 0 ? 0 : 1)
			.arg(x2 - x1)
			.arg(y2 - y1);

	arc += addFill(line, s[9], s[8]);
	arc += " />\n";
	return arc;
}

QString KicadSchematic2Svg::convertPoly(const QString & line) 
{
	QStringList s = splitLine(line);
	if (s.count() < 6) {
		DebugDialog::debug(QString("bad poly %1").arg(line));
		return "";
	}

	int np = s[1].toInt();
	if (np < 2) {
		DebugDialog::debug(QString("degenerate poly %1").arg(line));
		return "";
	}
	if (s.count() < (np * 2) + 5) {
		DebugDialog::debug(QString("bad poly (2) %1").arg(line));
		return "";
	}

	if (s.count() < (np * 2) + 6) {
		s.append("N");				// assume unfilled
	}

	int ix = 5;
	if (np == 2) {
		int x1 = s[ix++].toInt();
		int y1 = -s[ix++].toInt();						// KiCad flips y-axis w.r.t. svg
		int x2 = s[ix++].toInt();
		int y2 = -s[ix++].toInt();						// KiCad flips y-axis w.r.t. svg
		checkXLimit(x1);
		checkYLimit(y1);
		checkXLimit(x2);
		checkYLimit(y2);
		QString line = QString("<line x1='%1' y1='%2' x2='%3' y2='%4' ").arg(x1).arg(y1).arg(x2).arg(y2);
		line += addFill(line, s[ix], s[4]);
		line += " />\n";
		return line;
	}

	QString poly = "<polyline points='";
	for (int i = 0; i < np; i++) {
		int x = s[ix++].toInt();
		int y = -s[ix++].toInt();						// KiCad flips y-axis w.r.t. svg
		checkXLimit(x);
		checkYLimit(y);
		poly += QString("%1,%2,").arg(x).arg(y);
	}
	poly.chop(1);
	poly += "' ";
	poly += addFill(line, s[ix], s[4]);
	poly += " />\n";
	return poly;
}

QString KicadSchematic2Svg::addFill(const QString & line, const QString & NF, const QString & strokeString) {
	int stroke = strokeString.toInt();
	if (stroke <= 0) stroke = 1;

	if (NF == "F") {
		return "fill='#000000' ";
	}
	else if (NF == "f") {
		return "fill='#000000' fill-opacity='0.3' ";
	}
	else if (NF == "N") {
		return QString("fill='none' stroke='#000000' stroke-width='%1' ").arg(stroke);
	}


	DebugDialog::debug(QString("bad NF param: %1").arg(line));
	return "";
}

QStringList KicadSchematic2Svg::splitLine(const QString & line) {
	// doesn't handle escaped quotes next to spaces
	QStringList strs = line.split(" ", QString::SkipEmptyParts);
	for (int i = strs.count() - 1; i > 0; i--) {
		QString s = strs[i];
		if (s[s.length() - 1] != '"') continue;
		
		if (s[0] == '"' && s.length() > 1) continue;

		// space in a quoted string: combine
		strs[i - 1] += strs[i];
		strs.removeAt(i);
	}

	return strs;
}
