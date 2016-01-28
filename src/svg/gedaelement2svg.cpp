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

#include "gedaelement2svg.h"
#include "gedaelementparser.h"
#include "gedaelementlexer.h"
#include "../utils/textutils.h"
#include "../version/version.h"
#include "../items/wire.h"
#include "../debugdialog.h"
#include "../fsvgrenderer.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QObject>
#include <limits>
#include <QDomDocument>
#include <QDomElement>
#include <QDateTime>
#include <qmath.h>
#include <QTextDocument>

GedaElement2Svg::GedaElement2Svg() : X2Svg() {
}

QString GedaElement2Svg::convert(const QString & filename, bool allowPadsAndPins) 
{
	m_nonConnectorNumber = 0;
	initLimits();

	QFile file(filename);
	if (!file.open(QFile::ReadOnly)) {
		throw QObject::tr("unable to open %1").arg(filename);
	}

	QString text;
	QTextStream textStream(&file);
	text = textStream.readAll();
	file.close();

	GedaElementLexer lexer(text);
	GedaElementParser parser;

	if (!parser.parse(&lexer)) {
		throw QObject::tr("unable to parse %1").arg(filename);
	}

	QFileInfo fileInfo(filename);

	QDateTime now = QDateTime::currentDateTime();
	QString dt = now.toString("dd/MM/yyyy hh:mm:ss");

	QString title = QString("<title>%1</title>").arg(fileInfo.fileName());
	QString description = QString("<desc>Geda footprint file '%1' converted by Fritzing</desc>")
			.arg(TextUtils::stripNonValidXMLCharacters(TextUtils::escapeAnd(fileInfo.fileName())));


	QString metadata("<metadata xmlns:fz='http://fritzing.org/gedametadata/1.0/' xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>");
	metadata += "<rdf:RDF>";
	metadata += "<rdf:Description rdf:about=''>";
	metadata += m_attribute.arg("geda filename").arg(fileInfo.fileName());
	metadata += m_attribute.arg("fritzing version").arg(Version::versionString());
	metadata += m_attribute.arg("conversion date").arg(dt);

	// TODO: other layers
	QString silkscreen;

	QVector<QVariant> stack = parser.symStack();

	bool hasAuthor = false;
	QMultiHash<QString, QString> pads;
	QMultiHash<QString, QString> pins;
	QStringList pinIDs;
	QStringList padIDs;

	for (int ix = 0; ix < stack.size(); ) { 
		QVariant var = stack[ix];
		if (var.type() == QVariant::String) {
			QString thing = var.toString();
			int argCount = countArgs(stack, ix);
			bool mils = stack[ix + argCount + 1].toChar() == ')';
			if (thing.compare("element", Qt::CaseInsensitive) == 0) {
			}
			else if (thing.compare("pad", Qt::CaseInsensitive) == 0) {
				QString pid;
				QString s = convertPad(stack, ix, argCount, mils, pid);
				pads.insert(pid, s);
				if (!padIDs.contains(pid)) {
					padIDs.append(pid);
				}
			}
			else if (thing.compare("pin", Qt::CaseInsensitive) == 0) {
				QString pid;
				QString s = convertPin(stack, ix, argCount, mils, pid);
				pins.insert(pid, s);
				if (!pinIDs.contains(pid)) {
					pinIDs.append(pid);
				}
			}
			else if (thing.compare("elementline", Qt::CaseInsensitive) == 0) {
				QString unused;
				silkscreen += convertPad(stack, ix, argCount, mils, unused);
			}
			else if (thing.compare("elementarc", Qt::CaseInsensitive) == 0) {
				silkscreen += convertArc(stack, ix, argCount, mils);
			}
			else if (thing.compare("mark", Qt::CaseInsensitive) == 0) {
			}
			else if (thing.compare("attribute", Qt::CaseInsensitive) == 0) {
				QString aname = TextUtils::stripNonValidXMLCharacters(TextUtils::escapeAnd(unquote(stack[ix + 1].toString())));
				metadata += m_attribute.arg(aname, TextUtils::stripNonValidXMLCharacters(TextUtils::escapeAnd(unquote(stack[ix + 2].toString()))));
				if (aname.compare("author", Qt::CaseInsensitive) == 0) {
					hasAuthor = true;
				}
			}
			ix += argCount + 2;
		}
		else if (var.type() == QVariant::Char) {
			// will arrive here at the end of the element
			// TODO: shouldn't happen otherwise
			ix++;
		}
		else {
			throw QObject::tr("parse failure in %1").arg(filename);
		}
	}

	if (!allowPadsAndPins && pins.count() > 0 && pads.count() > 0) {
		throw QObject::tr("Sorry, Fritzing can't yet handle both pins and pads together (in %1)").arg(filename);
	}

	foreach (QString c, lexer.comments()) {
		metadata += m_comment.arg(TextUtils::stripNonValidXMLCharacters(TextUtils::escapeAnd(c)));
	}

	if (!hasAuthor) {
		metadata += m_attribute.arg("dist-license").arg("GPL");
		metadata += m_attribute.arg("use-license").arg("unlimited");
		metadata += m_attribute.arg("author").arg("gEDA project");
		metadata += m_attribute.arg("license-url").arg("http://www.gnu.org/licenses/gpl.html");
	}

	metadata += "</rdf:Description>";
	metadata += "</rdf:RDF>";
	metadata += "</metadata>";

	QString copper0 = makeCopper(pinIDs, pins, filename);
	QString copper1 = makeCopper(padIDs, pads, filename);

	if (!copper0.isEmpty()) {
		copper0 = offsetMin("\n<g id='copper0'><g id='copper1'>" + copper0 + "</g></g>\n");
	}
	if (!copper1.isEmpty()) {
		copper1 = offsetMin("\n<g id='copper1'>" + copper1 + "</g>\n");
	}
	if (!silkscreen.isEmpty()) {
		silkscreen = offsetMin("\n<g id='silkscreen'>" + silkscreen + "</g>\n");
	}

	QString svg = TextUtils::makeSVGHeader(100000, 100000, m_maxX - m_minX, m_maxY - m_minY) 
					+ title + description + metadata + copper0 + copper1 + silkscreen + "</svg>";

	return svg;
}

int GedaElement2Svg::countArgs(QVector<QVariant> & stack, int ix) {
	int argCount = 0;
	for (int i = ix + 1; i < stack.size(); i++) {
		QVariant var = stack[i];
		if (var.type() == QVariant::Char) {
			QChar ch = var.toChar();
			if (ch == ']' || ch == ')') {
				break;
			}
		}

		argCount++;
	}

	return argCount;
}

QString GedaElement2Svg::convertPin(QVector<QVariant> & stack, int ix, int argCount, bool mils, QString & pinID)
{
	double drill = 0;
	QString name;
	QString number;

	//int flags = stack[ix + argCount].toInt();
	//bool useNumber = (flags & 1) != 0;

	if (argCount == 9) {
		drill = stack[ix + 6].toInt();
		name = stack[ix + 7].toString();
		number = stack[ix + 8].toString();
	}
	else if (argCount == 7) {
		drill = stack[ix + 4].toInt();
		name = stack[ix + 5].toString();
		number = stack[ix + 6].toString();
	}
	else if (argCount == 6) {
		drill = stack[ix + 4].toInt();
		name = stack[ix + 5].toString();
	}
	else if (argCount == 5) {
		name = stack[ix + 4].toString();
	}
	else {
		throw QObject::tr("bad pin argument count");
	}


	pinID = getPinID(number, name, false);

	int cx = stack[ix + 1].toInt();
	int cy = stack[ix + 2].toInt();
	double r = stack[ix + 3].toInt() / 2.0;
	drill /= 2.0;

	if (mils) {
		// lo res
		cx *= 100;
		cy *= 100;
		r *= 100;
		drill *= 100;
	}

	checkXLimit(cx - r);
	checkXLimit(cx + r);
	checkYLimit(cy - r);
	checkYLimit(cy + r);

	double w = r - drill;

	// TODO: what if multiple pins have the same id--need to clear or increment the other ids. also put the pins on a bus?
	// TODO:  if the pin has a name, post it up to the fz as the connector name


	QString circle = QString("<circle fill='none' cx='%1' cy='%2' r='%3' id='%4' connectorname='%5' stroke-width='%6' stroke='%7' />")
					.arg(cx)
					.arg(cy)
					.arg(r - (w / 2))
					.arg(pinID)
					.arg(TextUtils::stripNonValidXMLCharacters(TextUtils::escapeAnd(name)))
					.arg(w)
					.arg(ViewLayer::Copper0Color);
	return circle;
}

QString GedaElement2Svg::convertPad(QVector<QVariant> & stack, int ix, int argCount, bool mils, QString & pinID)
{
	QString name; 
	QString number;

	int flags = (argCount > 5) ? stack[ix + argCount].toInt() : 0;
	bool square = (flags & 0x0100) != 0;
	int x1 = stack[ix + 1].toInt();
	int y1 = stack[ix + 2].toInt();
	int x2 = stack[ix + 3].toInt();
	int y2 = stack[ix + 4].toInt();
	int thickness = stack[ix + 5].toInt();

	bool isPad = true;
	if (argCount == 10) {
		name = stack[ix + 8].toString();
		number = stack[ix + 9].toString();
		QString sflags = stack[ix + argCount].toString();
		if (sflags.contains("square", Qt::CaseInsensitive)) {
			square = true;
		}
	}
	else if (argCount == 8) {
		name = stack[ix + 6].toString();
		number = stack[ix + 7].toString();
	}
	else if (argCount == 7) {
		name = stack[ix + 6].toString();
	}
	else if (argCount == 5) {
		// this is an elementline
		isPad = false;
	}
	else {
		throw QObject::tr("bad pad argument count");
	}

	if (isPad) {
		pinID = getPinID(number, name, true);
	}

	if (mils) {
		// lo res
		x1 *= 100;
		y1 *= 100;
		x2 *= 100;
		y2 *= 100;
		thickness *= 100;
	}

	double halft = thickness / 2.0;

	// don't know which of the coordinates is larger so check them all
	checkXLimit(x1 - halft);
	checkXLimit(x2 - halft);
	checkXLimit(x1 + halft);
	checkXLimit(x2 + halft);
	checkYLimit(y1 - halft);
	checkYLimit(y2 - halft);
	checkYLimit(y1 + halft);
	checkYLimit(y2 + halft);
	  
	QString line = QString("<line fill='none' x1='%1' y1='%2' x2='%3' y2='%4' stroke-width='%5' ")
					.arg(x1)
					.arg(y1)
					.arg(x2)
					.arg(y2)
					.arg(thickness);
	if (!isPad) {
		// elementline
		line += "stroke='white' ";
	}
	else {
		line += QString("stroke-linecap='%1' stroke-linejoin='%2' id='%3' connectorname='%4' stroke='%5' ")
					.arg(square ? "square" : "round")
					.arg(square ? "miter" : "round")
					.arg(pinID)
					.arg(TextUtils::stripNonValidXMLCharacters(TextUtils::escapeAnd(name)))
					.arg(ViewLayer::Copper1Color);
	}

	line += "/>";
	return line;
}

QString GedaElement2Svg::convertArc(QVector<QVariant> & stack, int ix, int argCount, bool mils)
{
	Q_UNUSED(argCount);

	int x = stack[ix + 1].toInt();
	int y = stack[ix + 2].toInt();
	double w = stack[ix + 3].toInt();
	double h = stack[ix + 4].toInt();

	// In PCB, an angle of zero points left (negative X direction), and 90 degrees points down (positive Y direction)
	int startAngle = (stack[ix + 5].toInt()) + 180;
	//  Positive angles sweep counterclockwise
	int deltaAngle = stack[ix + 6].toInt();

	int thickness = stack[ix + 7].toInt();

	if (mils) {
		// lo res
		x *= 100;
		y *= 100;
		w *= 100;
		h *= 100;
		thickness *= 100;
	}

	double halft = thickness / 2.0;
	checkXLimit(x - w - halft);
	checkXLimit(x + w + halft);
	checkYLimit(y - h - halft);
	checkYLimit(y + h + halft);

	if (deltaAngle == 360) {
		if (w == h) {
			QString circle = QString("<circle fill='none' cx='%1' cy='%2' stroke='white' r='%3' stroke-width='%4' />")
							.arg(x)
							.arg(y)
							.arg(w)
							.arg(thickness);

			return circle;
		}

		QString ellipse = QString("<ellipse fill='none' cx='%1' cy='%2' stroke='white' rx='%3' ry='%4' stroke-width='%5' />")
						.arg(x)
						.arg(y)
						.arg(w)
						.arg(h)
						.arg(thickness);

		return ellipse;
	}

	int quad = 0;
	int startAngleQ1 = reflectQuad(startAngle, quad);
	double q = atan(w * tan(2 * M_PI * startAngleQ1 / 360.0) / h);
	double px = w * cos(q);
	double py = -h * sin(q);
	fixQuad(quad, px, py);
	int endAngleQ1 = reflectQuad(startAngle + deltaAngle, quad);
	q = atan(w * tan(2 * M_PI * endAngleQ1 / 360.0) / h);
	double qx = w * cos(q);
	double qy = -h * sin(q);
	fixQuad(quad, qx, qy);

	QString arc = QString("<path fill='none' stroke-width='%1' stroke='white' d='M%2,%3a%4,%5 0 %6,%7 %8,%9' />")
			.arg(thickness)
			.arg(px + x)
			.arg(py + y)
			.arg(w)
			.arg(h)
			.arg(qAbs(deltaAngle) >= 180 ? 1 : 0)
			.arg(deltaAngle > 0 ? 0 : 1)
			.arg(qx - px)
			.arg(qy - py);

	return arc;
}

void GedaElement2Svg::fixQuad(int quad, double & px, double & py) {
	switch (quad) {
		case 0:
			break;
		case 1:
			px = -px;
			break;
		case 2:
			px = -px;
			py = -py;
			break;
		case 3:
			py = -py;
			break;
	}
}

int GedaElement2Svg::reflectQuad(int angle, int & quad) {
	angle = angle %360;
	if (angle < 0) angle += 360;
	quad = angle / 90;
	switch (quad) {
		case 0:
			return angle;
		case 1:
			return 180 - angle;
		case 2:
			return angle - 180;
		case 3:
			return 360 - angle;
	}

	// never gets here, but keeps compiler happy
	return angle;
}

QString GedaElement2Svg::getPinID(QString & number, QString & name, bool isPad) {

	if (!number.isEmpty()) {
		number = unquote(number);
	}
	if (!name.isEmpty()) {
		name = unquote(name);
	}

	QString suffix = isPad ? "pad" : "pin";

	if (!number.isEmpty()) {
		bool ok;
		int n = number.toInt(&ok);
		return ok ? QString("connector%1%2").arg(n - 1).arg(suffix) 
				  : QString("connector%1%2").arg(number).arg(suffix);
	}

	if (!name.isEmpty()) {
		if (number.isEmpty()) {
			bool ok;
			int n = name.toInt(&ok);
			if (ok) {
				return QString("connector%1%2").arg(n - 1).arg(suffix);
			}
		}
	}

	return QString("%1%2").arg(FSvgRenderer::NonConnectorName).arg(m_nonConnectorNumber++);
}


QString GedaElement2Svg::makeCopper(QStringList ids, QHash<QString, QString> & strings, const QString & filename) {
	QString copper;
	foreach (QString id, ids) {
		QStringList values = strings.values(id);
		if (id.isEmpty()) {
			DebugDialog::debug(QString("geda empty id %1").arg(filename));
			foreach(QString string, values) {
				copper.append(string);
			}
			continue;
		}

		if (values.count() == 1) {
			copper.append(values.at(0));
			continue;
		}

		if (values.count() == 0) {
			// shouldn't happen
			continue;
		}

		DebugDialog::debug(QString("geda multiple id %1").arg(filename));


		QString xml = "<g>";
		foreach (QString string, values) {
			xml.append(string);
		}
		xml.append("</g>");
		QString errorStr;
		int errorLine;
		int errorColumn;
		QDomDocument doc;
		if (!doc.setContent(xml, &errorStr, &errorLine, &errorColumn)) {
			throw QObject::tr("Unable to parse copper: %1 %2 %3").arg(errorStr).arg(errorLine).arg(errorColumn);
		}
		QDomElement root = doc.documentElement();
		QDomElement child = root.firstChildElement();
		while (!child.isNull()) {
			QString id = child.attribute("id");
			root.setAttribute("id", id);
			child.removeAttribute("id");
			QString name = child.attribute("connectorname");
			child.removeAttribute("connectorname");
			if (!name.isEmpty()) {
				root.setAttribute("connectorname", name);
			}
			child = child.nextSiblingElement();
		}

		copper += doc.toString();
	}

	return copper;
}
