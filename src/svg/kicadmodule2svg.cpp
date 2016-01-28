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

#include "kicadmodule2svg.h"
#include "../utils/textutils.h"
#include "../debugdialog.h"
#include "../viewlayer.h"
#include "../fsvgrenderer.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QObject>
#include <QDomDocument>
#include <QDomElement>
#include <QDateTime>
#include <qmath.h>
#include <limits>

#define KicadSilkscreenTop 21
#define KicadSilkscreenBottom 20

// TODO:
//		non-centered drill holes?
//		trapezoidal pads (shape delta may or may not be a separate issue)?
//		non-copper holes?
//		find true bounding box of arcs instead of using the whole circle

double checkStrokeWidth(double w) {
	if (w >= 0) return w;

	DebugDialog::debug("stroke width < 0");
	return 0;
}

KicadModule2Svg::KicadModule2Svg() : Kicad2Svg() {
}

QStringList KicadModule2Svg::listModules(const QString & filename) {
	QStringList modules;

	QFile file(filename);
	if (!file.open(QFile::ReadOnly)) return modules;

	QTextStream textStream(&file);
	bool gotIndex = false;
	while (true) {
		QString line = textStream.readLine();
		if (line.isNull()) break;

		if (line.compare("$INDEX") == 0) {
			gotIndex = true;
			break;
		}
	}

	if (!gotIndex) return modules;
	while (true) {
		QString line = textStream.readLine();
		if (line.isNull()) break;

		if (line.compare("$EndINDEX") == 0) {
			return modules;
		}

		modules.append(line);
	}

	modules.clear();
	return modules;
}

QString KicadModule2Svg::convert(const QString & filename, const QString & moduleName, bool allowPadsAndPins) 
{
	m_nonConnectorNumber = 0;
	initLimits();

	QFile file(filename);
	if (!file.open(QFile::ReadOnly)) {
		throw QObject::tr("unable to open %1").arg(filename);
	}

	QString text;
	QTextStream textStream(&file);

	QString metadata = makeMetadata(filename, "module", moduleName);


	bool gotModule = false;
	while (true) {
		QString line = textStream.readLine();
		if (line.isNull()) {
			break;
		}

		if (line.contains("$MODULE") && line.contains(moduleName, Qt::CaseInsensitive)) {
			gotModule = true;
			break;
		}
	}

	if (!gotModule) {
		throw QObject::tr("footprint %1 not found in %2").arg(moduleName).arg(filename);
	}

	bool gotT0;
	QString line;
	while (true) {
		line = textStream.readLine();
		if (line.isNull()) {
			throw QObject::tr("unexpected end of file in footprint %1 in file %2").arg(moduleName).arg(filename);
		}

		if (line.startsWith("T0") || line.startsWith("DS") || line.startsWith("DA") || line.startsWith("DC")) {
			gotT0 = true;
			break;
		}
		else if (line.startsWith("Cd")) {
			metadata += m_comment.arg(TextUtils::stripNonValidXMLCharacters(TextUtils::escapeAnd(line.remove(0,3))));
		}
		else if (line.startsWith("Kw")) {
			QStringList keywords = line.split(" ");
			for (int i = 1; i < keywords.count(); i++) {
				metadata += m_attribute.arg("keyword").arg(TextUtils::stripNonValidXMLCharacters(TextUtils::escapeAnd(keywords[i])));
			}
		}
	}

	metadata += endMetadata();

	if (!gotT0) {
		throw QObject::tr("unexpected format (1) in %1 from %2").arg(moduleName).arg(filename);
	}

	while (line.startsWith("T")) {
		line = textStream.readLine();
		if (line.isNull()) {
			throw QObject::tr("unexpected end of file in footprint %1 in file %2").arg(moduleName).arg(filename);
		}
	}

	bool done = false;
	QString copper0;
	QString copper1;
	QString silkscreen0;
	QString silkscreen1;

	while (true) {
		if (line.startsWith("$PAD")) break;
		if (line.startsWith("$EndMODULE")) {
			done = true;
			break;
		}

		int layer = 0;
		QString svgElement;
		if (line.startsWith("DS")) {
			layer = drawDSegment(line, svgElement);
		}
		else if (line.startsWith("DA")) {
			layer = drawDArc(line, svgElement);
		}
		else if (line.startsWith("DC")) {
			layer = drawDCircle(line, svgElement);
		}
		switch (layer) {
			case KicadSilkscreenTop:
				silkscreen1 += svgElement;
				break;
			case KicadSilkscreenBottom:
				silkscreen0 += svgElement;
				break;
			default:
				break;
		}
	
		line = textStream.readLine();
		if (line.isNull()) {
			throw QObject::tr("unexpected end of file in footprint %1 in file %2").arg(moduleName).arg(filename);
		}
	}

	if (!done) {
		QList<int> numbers;
		for (int i = 0; i < 512; i++) {
			numbers << i;
		}
		int pads = 0;
		int pins = 0;
		while (!done) {
			try {
				QString pad;
				PadLayer padLayer = convertPad(textStream, pad, numbers);
				switch (padLayer) {
					case ToCopper0:
						copper0 += pad;
						pins++;
						break;
					case ToCopper1:
						copper1 += pad;
						pads++;
						break;
					default:
						break;
				}
			}
			catch (const QString & msg) {
				DebugDialog::debug(QString("kicad pad %1 conversion failed in %2: %3").arg(moduleName).arg(filename).arg(msg));
			}

			while (true) {
				line = textStream.readLine();
				if (line.isNull()) {
					throw QObject::tr("unexpected end of file in footprint %1 in file %2").arg(moduleName).arg(filename);
				}

				if (line.contains("$SHAPE3D")) {
					done = true;
					break;
				}
				if (line.contains("$EndMODULE")) {
					done = true;
					break;
				}
				if (line.contains("$PAD")) {
					break;
				}
			}
		}

		if (!allowPadsAndPins && pins > 0 && pads > 0) {
			throw QObject::tr("Sorry, Fritzing can't yet handle both pins and pads together (in %1 in %2)").arg(moduleName).arg(filename);
		}

	}

	if (!copper0.isEmpty()) {
		copper0 = offsetMin("\n<g id='copper0'><g id='copper1'>" + copper0 + "</g></g>\n");
	}
	if (!copper1.isEmpty()) {
		copper1 = offsetMin("\n<g id='copper1'>" + copper1 + "</g>\n");
	}
	if (!silkscreen1.isEmpty()) {
		silkscreen1 = offsetMin("\n<g id='silkscreen'>" + silkscreen1 + "</g>\n");
	}
	if (!silkscreen0.isEmpty()) {
		silkscreen0 = offsetMin("\n<g id='silkscreen0'>" + silkscreen0 + "</g>\n");
	}

	QString svg = TextUtils::makeSVGHeader(10000, 10000, m_maxX - m_minX, m_maxY - m_minY) 
					+ m_title + m_description + metadata + copper0 + copper1 + silkscreen0 + silkscreen1 + "</svg>";

	return svg;
}

int KicadModule2Svg::drawDCircle(const QString & ds, QString & circle) {
	// DC Xcentre Ycentre Xend Yend width layer
	QStringList params = ds.split(" ");
	if (params.count() < 7) return -1;

	int cx = params.at(1).toInt();
	int cy = params.at(2).toInt();
	int x2 = params.at(3).toInt();
	int y2 = params.at(4).toInt();
	double radius = qSqrt((cx - x2) * (cx - x2) + (cy - y2) * (cy - y2));

	int w = params.at(5).toInt();
	double halfWidth = w / 2.0;

	checkXLimit(cx + radius + halfWidth);
	checkXLimit(cx - radius - halfWidth);
	checkYLimit(cy + radius + halfWidth);
	checkYLimit(cy - radius - halfWidth);


	int layer = params.at(6).toInt();

	circle = QString("<circle cx='%1' cy='%2' r='%3' stroke-width='%4' stroke='white' fill='none' />")
		.arg(cx)
		.arg(cy)
		.arg(radius)
		.arg(checkStrokeWidth(w));

	return layer;
}

int KicadModule2Svg::drawDSegment(const QString & ds, QString & line) {
	// DS Xstart Ystart Xend Yend Width Layer
	QStringList params = ds.split(" ");
	if (params.count() < 7) return -1;

	int x1 = params.at(1).toInt();
	int y1 = params.at(2).toInt();
	int x2 = params.at(3).toInt();
	int y2 = params.at(4).toInt();
	checkXLimit(x1);
	checkXLimit(x2);
	checkYLimit(y1);
	checkYLimit(y2);

	int layer = params.at(6).toInt();

	line = QString("<line x1='%1' y1='%2' x2='%3' y2='%4' stroke-width='%5' stroke='white' fill='none' />")
				.arg(x1)
				.arg(y1)
				.arg(x2)
				.arg(y2)
				.arg(checkStrokeWidth(params.at(5).toDouble()));

	return layer;
}

int KicadModule2Svg::drawDArc(const QString & ds, QString & arc) {
	//DA x0 y0 x1 y1 angle width layer

	QStringList params = ds.split(" ");
	if (params.count() < 8) return -1;

	int cx = params.at(1).toInt();
	int cy = params.at(2).toInt();
	int x2 = params.at(3).toInt();
	int y2 = params.at(4).toInt();
	int width = params.at(6).toInt();
	double diffAngle = (params.at(5).toInt() % 3600) / 10.0;
	double radius = qSqrt((cx - x2) * (cx - x2) + (cy - y2) * (cy - y2));
	double endAngle = asin((y2 - cy) / radius);
	if (x2 < cx) {
		endAngle += M_PI;
	}
	double startAngle = endAngle + (diffAngle * M_PI / 180.0);
	double x1 = (radius * cos(startAngle)) + cx;
	double y1 = (radius * sin(startAngle)) + cy;

	// TODO: figure out bounding box for circular arc and set min and max accordingly

/*
You have radius R, start angle S, end angle T, and I'll
assume that the arc is swept counterclockwise from S to T.

start.x = R * cos(S)
start.y = R * sin(S)
end.x = R * cos(T)
end.y = R * sin(T)

Determine the axis crossings by analyzing the start and
end angles. For discussion sake, I'll describe angles
using degrees. Provide a function, wrap(angle), that
returns an angle in the range [0 to 360).

cross0 = wrap(S) > wrap(T)
cross90 = wrap(S-90) > wrap(T-90)
cross180 = wrap(S-180) > wrap(T-180)
cross270 = wrap(S-270) > wrap(T-270)

Now the axis aligned bounding box is defined by:

right = cross0 ? +R : max(start.x, end.x)
top = cross90 ? +R : max(start.y, end.y)
left = cross180 ? -R : min(start.x, end.x)
bottom = cross270 ? -R : min(start.y, end.y)

*/


	checkXLimit(cx + radius);
	checkXLimit(cx - radius);
	checkYLimit(cy + radius);
	checkYLimit(cy - radius);

	int layer = params.at(7).toInt();

	arc = QString("<path stroke-width='%1' stroke='white' d='M%2,%3a%4,%5 0 %6,%7 %8,%9' fill='none' />")
			.arg(checkStrokeWidth(width / 2.0))
			.arg(x1)
			.arg(y1)
			.arg(radius)
			.arg(radius)
			.arg(qAbs(diffAngle) >= 180 ? 1 : 0)
			.arg(diffAngle > 0 ? 0 : 1)
			.arg(x2 - x1)
			.arg(y2 - y1);

	return layer;
}

KicadModule2Svg::PadLayer KicadModule2Svg::convertPad(QTextStream & stream, QString & pad, QList<int> & numbers) {
	PadLayer padLayer = UnableToTranslate;

	QStringList padStrings;
	while (true) {
		QString line = stream.readLine();
		if (line.isNull()) {
			throw QObject::tr("unexpected end of file");
		}
		if (line.contains("$EndPAD")) {
			break;
		}

		padStrings.append(line);
	}

	QString shape;
	QString drill;
	QString attributes;
	QString position;

	foreach (QString string, padStrings) {
		if (string.startsWith("Sh")) {
			shape = string;
		}
		else if (string.startsWith("Po")) {
			position = string;
		}
		else if (string.startsWith("At")) {
			attributes = string;
		}
		else if (string.startsWith("Dr")) {
			drill = string;
		}
	}

	if (drill.isEmpty()) {
		throw QObject::tr("pad missing drill");
	}
	if (attributes.isEmpty()) {
		throw QObject::tr("pad missing attributes");
	}
	if (position.isEmpty()) {
		throw QObject::tr("pad missing position");
	}
	if (shape.isEmpty()) {
		throw QObject::tr("pad missing shape");
	}

	QStringList positionStrings = position.split(" ");
	if (positionStrings.count() < 3) {
		throw QObject::tr("position missing params");
	}

	int posX = positionStrings.at(1).toInt();
	int posY = positionStrings.at(2).toInt();

	QStringList drillStrings = drill.split(" ");
	if (drillStrings.count() < 4) {
		throw QObject::tr("drill missing params");
	}

	int drillX = drillStrings.at(1).toInt();
	int drillXOffset = drillStrings.at(2).toInt();
	int drillYOffset = drillStrings.at(3).toInt();
	int drillY = drillX;

	if (drillXOffset != 0 || drillYOffset != 0) {
		throw QObject::tr("drill offset not implemented");
	}

	if (drillStrings.count() > 4) {
		if (drillStrings.at(4) == "O") {
			if (drillStrings.count() < 7) {
				throw QObject::tr("drill missing ellipse params");
			}
			drillY = drillStrings.at(6).toInt();
		}
	}

	QStringList attributeStrings = attributes.split(" ");
	if (attributeStrings.count() < 4) {
		throw QObject::tr("attributes missing params");
	}

	bool ok;
	int layerMask = attributeStrings.at(3).toInt(&ok, 16);
	if (!ok) {
		throw QObject::tr("bad layer mask parameter");
	}

	QString padType = attributeStrings.at(1);
	if (padType == "MECA") {
		// seems to be the same thing
		padType = "STD";
	}

	if (padType == "STD") {
		padLayer = ToCopper0;
	}
	else if (padType == "SMD") {
		padLayer = ToCopper1;
	}
	else if (padType == "CONN") {
		if (layerMask & 1) {
			padLayer = ToCopper0;
		}
		else {
			padLayer = ToCopper1;
		}
	}
	else if (padType == "HOLE") {
		padLayer = ToCopper0;
	}
	else {
		throw QObject::tr("Sorry, can't handle pad type %1").arg(padType);
	}

	QStringList shapeStrings = shape.split(" ");
	if (shapeStrings.count() < 8) {
		throw QObject::tr("pad shape missing params");
	}

	QString padName = unquote(shapeStrings.at(1));
	int padNumber = padName.toInt(&ok) - 1;
	if (!ok) {
		padNumber = padName.isEmpty() ? -1 : numbers.takeFirst();
		//DebugDialog::debug(QString("name:%1 padnumber %2").arg(padName).arg(padNumber));
	}
	else {
		numbers.removeOne(padNumber);
	}


	QString shapeIdentifier = shapeStrings.at(2);
	int xSize = shapeStrings.at(3).toInt();
	int ySize = shapeStrings.at(4).toInt();
	if (ySize <= 0) {
		DebugDialog::debug(QString("ySize is zero %1").arg(padName));
		ySize = xSize;
	}
	if (xSize <= 0) {
		throw QObject::tr("pad shape size is invalid");
	}

	int xDelta = shapeStrings.at(5).toInt();
	int yDelta = shapeStrings.at(6).toInt();
	int orientation = shapeStrings.at(7).toInt();

	if (shapeIdentifier == "T") {
		throw QObject::tr("trapezoidal pads not implemented");
		// eventually polygon?
	}

	if (xDelta != 0 || yDelta != 0) {
		// note: so far, all cases of non-zero delta go with shape "T"
		throw QObject::tr("shape delta not implemented");
	}

	if (padType == "HOLE") {
		if (shapeIdentifier != "C") {
			throw QObject::tr("non-circular holes not implemented");
		}

		if (drillX == xSize) {
			throw QObject::tr("non-copper holes not implemented");
		}
	}

	if (shapeIdentifier == "C") {
		checkLimits(posX, xSize, posY, ySize);
		pad += drawCPad(posX, posY, xSize, ySize, drillX, drillY, padName, padNumber, padType, padLayer);
	}
	else if (shapeIdentifier == "R") {
		checkLimits(posX, xSize, posY, ySize);
		pad += drawRPad(posX, posY, xSize, ySize, drillX, drillY, padName, padNumber, padType, padLayer);
	}
	else if (shapeIdentifier == "O") {
		checkLimits(posX, xSize, posY, ySize);
		QString id = getID(padNumber, padLayer);
		pad += QString("<g %1 connectorname='%2'>")
			.arg(id).arg(padName) 
			+ drawOblong(posX, posY, xSize, ySize, drillX, drillY, padType, padLayer) 
			+ "</g>";
	}
	else {
		throw QObject::tr("unable to handle pad shape %1").arg(shapeIdentifier);
	}
		
	if (orientation != 0) {
		if (orientation < 0) {
			orientation = (orientation % 3600) + 3600;
		}
		orientation = 3600 - (orientation % 3600);
		QTransform t = QTransform().translate(-posX, -posY) * 
						QTransform().rotate(orientation / 10.0) * 
						QTransform().translate(posX, posY);
		pad = TextUtils::svgTransform(pad, t, true, QString("_x='%1' _y='%2' _r='%3'").arg(posX).arg(posY).arg(orientation / 10.0));
	}

	return padLayer;
}

QString KicadModule2Svg::drawVerticalOblong(int posX, int posY, double xSize, double ySize, int drillX, int drillY, const QString & padType, KicadModule2Svg::PadLayer padLayer) {

	QString color = getColor(padLayer);
	double rad = xSize / 4.0;

	QString bot;

	if (drillX == drillY) {
		bot = QString("<path d='M%1,%2a%3,%3 0 0 1 %4,0' fill='%5' stroke-width='0' />")
						.arg(posX - rad - rad)
						.arg(posY - (ySize / 2.0) + (xSize / 2.0))
						.arg(rad * 2)
						.arg(rad * 4)
						.arg(color);
		bot += QString("<path d='M%1,%2a%3,%3 0 1 1 %4,0' fill='%5' stroke-width='0' />")
						.arg(posX + rad + rad)
						.arg(posY + (ySize / 2.0) - (xSize / 2.0))
						.arg(rad * 2)
						.arg(-rad * 4)
						.arg(color);
	}
	else {
		double w = (ySize - drillY) / 2.0;
		double newrad = rad - w / 4;
		bot = QString("<g id='oblong' stroke-width='%1'>").arg(checkStrokeWidth(drillX));
		bot += QString("<path d='M%1,%2a%3,%3 0 0 1 %4,0' fill='none' stroke='%5' stroke-width='%6' />")
						.arg(posX - rad - rad + (w / 2))
						.arg(posY - (ySize / 2.0) + (xSize / 2.0))
						.arg(newrad * 2)
						.arg(newrad * 4)
						.arg(color)
						.arg(checkStrokeWidth(w));
		bot += QString("<path d='M%1,%2a%3,%3 0 1 1 %4,0' fill='none' stroke='%5' stroke-width='%6' />")
						.arg(posX + rad + rad - (w / 2))
						.arg(posY + (ySize / 2.0) - (xSize / 2.0))
						.arg(newrad * 2)
						.arg(-newrad * 4)
						.arg(color)
						.arg(checkStrokeWidth(w));
		bot += QString("<line fill='none' stroke-width='0' x1='%1' y1='%2' x2='%3' y2='%4' />")
			.arg(posX).arg(posY - (ySize / 2.0) + (xSize / 2.0)).arg(posX).arg(posY + (ySize / 2.0) - (xSize / 2.0));
		bot += "</g>";
	}

	QString middle;

	if (padType == "SMD") {
		middle = QString("<rect x='%1' y='%2' width='%3' height='%4' stroke-width='0' fill='%5' />")
							.arg(posX - (xSize / 2.0))
							.arg(posY - (ySize / 2.0) + (xSize / 2.0))
							.arg(xSize)
							.arg(ySize - xSize)
							.arg(color);
	}
	else {
		if (drillX == drillY) {
			middle = QString("<circle fill='none' cx='%1' cy='%2' r='%3' stroke-width='%4' stroke='%5' />")
								.arg(posX)
								.arg(posY)
								.arg((qMin(xSize, ySize) / 2.0) - (drillX / 4.0))
								.arg(checkStrokeWidth(drillX / 2.0))
								.arg(color);
		}

		middle += QString("<line x1='%1' y1='%2' x2='%1' y2='%3' fill='none' stroke-width='%4' stroke='%5' />")
							.arg(posX - (xSize / 2.0) + (drillX / 4.0))
							.arg(posY - (ySize / 2.0) + (xSize / 2.0))
							.arg(posY + (ySize / 2.0) - (xSize / 2.0))
							.arg(checkStrokeWidth(drillX / 2.0))
							.arg(color);
		middle += QString("<line x1='%1' y1='%2' x2='%1' y2='%3' fill='none' stroke-width='%4' stroke='%5' />")
							.arg(posX + (xSize / 2.0) - (drillX / 4.0))
							.arg(posY - (ySize / 2.0) + (xSize / 2.0))
							.arg(posY + (ySize / 2.0) - (xSize / 2.0))
							.arg(checkStrokeWidth(drillX / 2.0))
							.arg(color);
	}

	return middle + bot;
}

QString KicadModule2Svg::drawHorizontalOblong(int posX, int posY, double xSize, double ySize, int drillX, int drillY, const QString & padType, KicadModule2Svg::PadLayer padLayer) {

	QString color = getColor(padLayer);
	double rad = ySize / 4.0;

	QString bot;

	if (drillX == drillY) {
		bot = QString("<path d='M%1,%2a%3,%3 0 0 0 0,%4' fill='%5' stroke-width='0' />")
					.arg(posX - (xSize / 2.0) + (ySize / 2.0))
					.arg(posY - rad - rad)
					.arg(rad * 2)
					.arg(rad * 4)
					.arg(color);
		bot += QString("<path d='M%1,%2a%3,%3 0 1 0 0,%4' fill='%5' stroke-width='0' />")
					.arg(posX + (xSize / 2.0) - (ySize / 2.0))
					.arg(posY + rad + rad)
					.arg(rad * 2)
					.arg(-rad * 4)
					.arg(color);
	}
	else {
		double w = (xSize - drillX) / 2.0;
		double newrad = rad - w / 4;
		bot = QString("<g id='oblong' stroke-width='%1'>").arg(checkStrokeWidth(drillY));
		bot += QString("<path d='M%1,%2a%3,%3 0 0 0 0,%4' fill='none' stroke='%5' stroke-width='%6' />")
					.arg(posX - (xSize / 2.0) + (ySize / 2.0))
					.arg(posY - rad - rad  + (w / 2))
					.arg(newrad * 2)
					.arg(newrad * 4)
					.arg(color)
					.arg(checkStrokeWidth(w));
		bot += QString("<path d='M%1,%2a%3,%3 0 1 0 0,%4' fill='none' stroke='%5' stroke-width='%6' />")
					.arg(posX + (xSize / 2.0) - (ySize / 2.0))
					.arg(posY + rad + rad - (w / 2))
					.arg(newrad * 2)
					.arg(-newrad * 4)
					.arg(color)
					.arg(checkStrokeWidth(w));
		bot += QString("<line fill='none' stroke-width='0' x1='%1' y1='%2' x2='%3' y2='%4' />")
			.arg(posX - (xSize / 2.0) + (ySize / 2.0)).arg(posY).arg(posX + (xSize / 2.0) - (ySize / 2.0)).arg(posY);
		bot += "</g>";
	}

	QString middle;
	bool gotID = false;

	if (padType == "SMD") {
		middle = QString("<rect x='%1' y='%2' width='%3' height='%4' stroke-width='0' fill='%5' />")
							.arg(posX - (xSize / 2.0) + (ySize / 2.0))
							.arg(posY - (ySize / 2.0))
							.arg(xSize - ySize)
							.arg(ySize)
							.arg(color);
	}
	else {
		if (drillX == drillY) {
			gotID = true;
			middle = QString("<circle fill='none' cx='%1' cy='%2' r='%3' stroke-width='%4' stroke='%5' />")
								.arg(posX)
								.arg(posY)
								.arg((qMin(xSize, ySize) / 2.0) - (drillY / 4.0))
								.arg(checkStrokeWidth(drillY / 2.0))
								.arg(color);
		}

		middle += QString("<line x1='%1' y1='%2' x2='%3' y2='%2' fill='none' stroke-width='%4' stroke='%5' />")
							.arg(posX - (xSize / 2.0) + (ySize / 2.0))
							.arg(posY - (ySize / 2.0) + (drillY / 4.0))
							.arg(posX + (xSize / 2.0) - (ySize / 2.0))
							.arg(checkStrokeWidth(drillY / 2.0))
							.arg(color);
		middle += QString("<line x1='%1' y1='%2' x2='%3' y2='%2' fill='none' stroke-width='%4' stroke='%5' />")
							.arg(posX - (xSize / 2.0) + (ySize / 2.0))
							.arg(posY + (ySize / 2.0) - (drillY / 4.0))
							.arg(posX + (xSize / 2.0) - (ySize / 2.0))
							.arg(checkStrokeWidth(drillY / 2.0))
							.arg(color);
	}

	return middle + bot;
}

void KicadModule2Svg::checkLimits(int posX, int xSize, int posY, int ySize) {
	checkXLimit(posX - (xSize / 2.0));
	checkXLimit(posX + (xSize / 2.0));
	checkYLimit(posY - (ySize / 2.0));
	checkYLimit(posY + (ySize / 2.0));
}

QString KicadModule2Svg::drawCPad(int posX, int posY, int xSize, int ySize, int drillX, int drillY, const QString & padName, int padNumber, const QString & padType, KicadModule2Svg::PadLayer padLayer) 
{
	QString color = getColor(padLayer);
	QString id = getID(padNumber, padLayer);

	Q_UNUSED(ySize);
	if (padType == "SMD") {
		return QString("<circle cx='%1' cy='%2' r='%3' %4 fill='%5' stroke-width='0' connectorname='%6'/>")
						.arg(posX)
						.arg(posY)
						.arg(xSize / 2.0)
						.arg(id)
						.arg(color)
						.arg(padName);
	}

	if (drillX == drillY) {
		double w = (xSize - drillX) / 2.0;
		QString pad = QString("<g %1 connectorname='%2'>").arg(id).arg(padName);
		pad += QString("<circle cx='%1' cy='%2' r='%3' stroke-width='%4' stroke='%5' fill='none' />")
							.arg(posX)
							.arg(posY)
							.arg((drillX / 2.0) + (w / 2))
							.arg(checkStrokeWidth(w))
							.arg(color);
		if (drillX > 500) {
			pad += QString("<circle cx='%1' cy='%2' r='%3' stroke-width='0' fill='black' drill='0' />")
										.arg(posX)
										.arg(posY)
										.arg(drillX / 2.0);
		}
		pad += "</g>";
		return pad;
	}


	QString pad = QString("<g %1>").arg(id);
	double w = (xSize - qMax(drillX, drillY)) / 2.0;
	pad += QString("<circle cx='%1' cy='%2' r='%3' stroke-width='%4' stroke='%5' fill='none' drill='0' />")
						.arg(posX)
						.arg(posY)
						.arg((xSize / 2.0) - (w / 2))
						.arg(checkStrokeWidth(w))
						.arg(color);
	pad += drawOblong(posX, posY, drillX + w, drillY + w, drillX, drillY, "", padLayer);

	// now fill the gaps between the oblong and the circle
	if (drillX >= drillY) {
		double angle = asin(((drillY + w) / 2) / (ySize / 2.0));
		double opp = (ySize / 2.0) * cos(angle);
		pad += QString("<polygon stroke-width='0' fill='%1' points='%2,%3,%4,%5,%6,%7' />")
			.arg(color)
			.arg(posX)
			.arg(posY - (ySize / 2.0))
			.arg(posX - opp)
			.arg(posY - (drillY / 2.0))
			.arg(posX + opp)
			.arg(posY - (drillY / 2.0));
		pad += QString("<polygon stroke-width='0' fill='%1' points='%2,%3,%4,%5,%6,%7' />")
			.arg(color)
			.arg(posX)
			.arg(posY + (ySize / 2.0))
			.arg(posX - opp)
			.arg(posY + (drillY / 2.0))
			.arg(posX + opp)
			.arg(posY + (drillY / 2.0));
	}
	else {
		double angle = acos(((drillX + w) / 2) / (xSize / 2.0));
		double adj = (xSize / 2.0) * sin(angle);
		pad += QString("<polygon stroke-width='0' fill='%1' points='%2,%3,%4,%5,%6,%7' />")
			.arg(color)
			.arg(posX - (xSize / 2.0))
			.arg(posY)
			.arg(posX - (drillX / 2.0))
			.arg(posY - adj)
			.arg(posX - (drillX / 2.0))
			.arg(posY + adj);
		pad += QString("<polygon stroke-width='0' fill='%1' points='%2,%3,%4,%5,%6,%7' />")
			.arg(color)
			.arg(posX + (xSize / 2.0))
			.arg(posY)
			.arg(posX + (drillX / 2.0))
			.arg(posY - adj)
			.arg(posX + (drillX / 2.0))
			.arg(posY + adj);
	}

	pad += "</g>";

	return pad;
}

QString KicadModule2Svg::drawRPad(int posX, int posY, int xSize, int ySize, int drillX, int drillY, const QString & padName, int padNumber, const QString & padType, KicadModule2Svg::PadLayer padLayer) 
{
	QString color = getColor(padLayer);
	QString id = getID(padNumber, padLayer);

	if (padType == "SMD") {
		return QString("<rect x='%1' y='%2' width='%3' height='%4' %5 stroke-width='0' fill='%6' connectorname='%7'/>")
						.arg(posX - (xSize / 2.0))
						.arg(posY - (ySize / 2.0))
						.arg(xSize)
						.arg(ySize)
						.arg(id)
						.arg(color)
						.arg(padName);
	}

	QString pad = QString("<g %1 connectorname='%2'>").arg(id).arg(padName);
	if (drillX == drillY) {
		double w = (qMin(xSize, ySize) - drillX) / 2.0;
		pad += QString("<circle fill='none' cx='%1' cy='%2' r='%3' stroke-width='%4' stroke='%5' />")
							.arg(posX)
							.arg(posY)
							.arg((w / 2) + (drillX / 2.0))
							.arg(checkStrokeWidth(w))
							.arg(color);
	}
	else {
		double w = (drillX >= drillY) ? (xSize - drillX) / 2.0 : (ySize - drillY) / 2.0 ;
		pad += QString("<circle fill='none' cx='%1' cy='%2' r='%3' stroke-width='%4' stroke='%5' />")
							.arg(posX)
							.arg(posY)
							.arg((w / 2) + (qMax(drillX, drillY) / 2.0))
							.arg(checkStrokeWidth(w))
							.arg(color);
		pad += drawOblong(posX, posY, drillX + w, drillY + w, drillX, drillY, "", padLayer);
	}
			
	// draw 4 lines otherwise there may be gaps if one pair of sides is much longer than the other pair of sides

	double w = (ySize - drillY) / 2.0;
	double tlx = posX - xSize / 2.0;
	double tly = posY - ySize / 2.0;
	pad += QString("<line x1='%1' y1='%2' x2='%3' y2='%2' fill='none' stroke-width='%4' stroke='%5' />")
					.arg(tlx)
					.arg(tly + w / 2)
					.arg(tlx + xSize)
					.arg(checkStrokeWidth(w))
					.arg(color);
	pad += QString("<line x1='%1' y1='%2' x2='%3' y2='%2' fill='none' stroke-width='%4' stroke='%5' />")
					.arg(tlx)
					.arg(tly + ySize - w / 2)
					.arg(tlx + xSize)
					.arg(checkStrokeWidth(w))
					.arg(color);

	w = (xSize - drillX) / 2.0;
	pad += QString("<line x1='%1' y1='%2' x2='%1' y2='%3' fill='none' stroke-width='%4' stroke='%5' />")
					.arg(tlx + w / 2)
					.arg(tly)
					.arg(tly + ySize)
					.arg(checkStrokeWidth(w))
					.arg(color);
	pad += QString("<line x1='%1' y1='%2' x2='%1' y2='%3' fill='none' stroke-width='%4' stroke='%5' />")
					.arg(tlx + xSize - w / 2)
					.arg(tly)
					.arg(tly + ySize)
					.arg(checkStrokeWidth(w))
					.arg(color);
	pad += "</g>";
	return pad;
}

QString KicadModule2Svg::drawOblong(int posX, int posY, double xSize, double ySize, int drillX, int drillY, const QString & padType, KicadModule2Svg::PadLayer padLayer) {
	if (xSize <= ySize) {
		return drawVerticalOblong(posX, posY, xSize, ySize, drillX, drillY, padType, padLayer);
	}
	else {
		return drawHorizontalOblong(posX, posY, xSize, ySize, drillX, drillY, padType, padLayer);
	}
}

QString KicadModule2Svg::getID(int padNumber, KicadModule2Svg::PadLayer padLayer) {	
	if (padNumber < 0) {
		return QString("id='%1%2'").arg(FSvgRenderer::NonConnectorName).arg(m_nonConnectorNumber++);
	}

	return QString("id='connector%1%2'").arg(padNumber).arg((padLayer == ToCopper1) ? "pad" : "pin");
}

QString KicadModule2Svg::getColor(KicadModule2Svg::PadLayer padLayer) {
	switch (padLayer) {
		case ToCopper0:
			return ViewLayer::Copper0Color;
			break;
		case ToCopper1:
			return ViewLayer::Copper1Color;
			break;
		default:
			DebugDialog::debug("kicad getcolor with unknown layer");
			return "#FF0000";
	}
}
