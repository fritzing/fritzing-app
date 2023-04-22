/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

********************************************************************/

#include "svg2gerber.h"
#include "../debugdialog.h"
#include "svgflattener.h"
#include <QTextStream>
#include <QSettings>
#include <QSet>
#include <QtDebug>
#include <QRegularExpression>
#include <qmath.h>

constexpr double MaskClearance = 0.0;  // 5 mils clearance
constexpr double milsPerInch = 1000;  // used to convert mils (standard fritzing resolution) to inches

bool hasFill(QDomElement & element) {
	QString fill = element.attribute("fill");
	if (fill.isEmpty()) return true; // svg spec: default to black fill
	if (fill.compare("none") == 0) return false;
	return true;
}

bool hasStroke(QDomElement & element) {
	QString stroke = element.attribute("stroke");
	if (stroke.isEmpty()) return false;
	if (stroke.compare("none") == 0) return false;
	return true;
}

//TODO: currently only supports one board per sketch (i.e. multiple board outlines will mess you up)

int SVG2gerber::convert(const QString & svgStr, bool doubleSided, const QString & mainLayerName, ForWhy forWhy, QSizeF boardSize)
{
	m_boardSize = boardSize;
	m_SVGDom = QDomDocument("svg");
	QString errorStr;
	int errorLine;
	int errorColumn;
	bool result = m_SVGDom.setContent(svgStr, &errorStr, &errorLine, &errorColumn);
	if (!result) {
		DebugDialog::debug(QString("gerber svg failed %2 %3 %4 %1").arg(svgStr).arg(errorStr).arg(errorLine).arg(errorColumn));
	}

#ifndef QT_NO_DEBUG
	QString temp = m_SVGDom.toString();
#endif

	normalizeSVG();

#ifndef QT_NO_DEBUG
	temp = m_SVGDom.toString();
#endif

	return renderGerber(doubleSided, mainLayerName, forWhy);
}

QString SVG2gerber::getGerber() {
	return m_gerber_header + m_gerber_paths;
}

int SVG2gerber::renderGerber(bool doubleSided, const QString & mainLayerName, ForWhy forWhy) {
	bool gerberExportImprovementsEnabled = QSettings().value("gerberExportImprovementsEnabled").toBool();
	if (forWhy != ForDrill) {
		// human readable description comments
		m_gerber_header = "G04 MADE WITH FRITZING*\n";
		m_gerber_header += "G04 WWW.FRITZING.ORG*\n";
		m_gerber_header += QString("G04 %1 SIDED*\n").arg(doubleSided ? "DOUBLE" : "SINGLE");
		m_gerber_header += QString("G04 HOLES%1PLATED*\n").arg(doubleSided ? " " : " NOT ");
		m_gerber_header += "G04 CONTOUR ON CENTER OF CONTOUR VECTOR*\n";

		if (gerberExportImprovementsEnabled) {

			m_gerber_header += "%FSLAX26Y26*%\n";
			// set units to inches
			m_gerber_header += "%MOIN*%\n";

			m_f2g = 1000.0;
			m_G54 = ""; // Deprecated since 2012 - omit G54 and simply write Dnn* to select aperture nn

		} else {
			// initialize axes
			m_gerber_header += "%ASAXBY*%\n";

			// NOTE: this currently forces a 1 mil grid
			// format coordinates to drop leading zeros with 2,3 digits
			m_gerber_header += "%FSLAX23Y23*%\n";

			// set units to inches
			m_gerber_header += "%MOIN*%\n";

			// no offset
			m_gerber_header += "%OFA0B0*%\n";

			// scale factor 1x1
			m_gerber_header += "%SFA1.0B1.0*%\n";
		}
	}
	else {
		// deal with header at the end
	}

	// define apertures and draw them
	int invalidCount = allPaths2gerber(forWhy);

	if (forWhy == ForDrill) {
		static constexpr int initialHoleIndex = 1;
		static constexpr int offset = 100;
		int initialPlatedIndex = (((m_holeApertures.uniqueKeys().count() + initialHoleIndex - 1) / offset) + 1) * offset;
		m_gerber_header = "";
		m_gerber_header += QString("; NON-PLATED HOLES START AT T%1\n").arg(initialHoleIndex);
		m_gerber_header += QString("; THROUGH (PLATED) HOLES START AT T%1\n").arg(initialPlatedIndex);

		// setup drill file header
		m_gerber_header += "M48\n";
		// set to english (inches) units, with trailing zeros
		m_gerber_header += "INCH\n";

		int ix = initialHoleIndex;
		Q_FOREACH (QString aperture, m_holeApertures.uniqueKeys()) {
			m_gerber_header += QString("T%1%2\n").arg(ix).arg(aperture);
			m_gerber_paths += QString("T%1\n").arg(ix);
			auto values = m_holeApertures.values(aperture);
			Q_FOREACH (QString loc, QSet<QString>(values.begin(), values.end())) {
				m_gerber_paths += loc + "\n";
			}
			ix++;
		}

		ix = initialPlatedIndex;
		Q_FOREACH (QString aperture, m_platedApertures.uniqueKeys()) {
			m_gerber_header += QString("T%1%2\n").arg(ix).arg(aperture);
			m_gerber_paths += QString("T%1\n").arg(ix);
			auto values = m_platedApertures.values(aperture);
			Q_FOREACH (QString loc, QSet<QString>(values.begin(), values.end())) {
				m_gerber_paths += loc + "\n";
			}
			ix++;
		}

		m_gerber_header += "%\n";    // closes the header


		//m_gerber_paths += m_drill_slots;   // from handleOblong, not up to date

		// drill file unload tool and end of program
		m_gerber_paths += "T00\n";
		m_gerber_paths += "M30\n";

	}
	else {
		if (gerberExportImprovementsEnabled) {
			// label our layers
			m_gerber_header += QString("%G04%1*%\n").arg(mainLayerName.toUpper());

			// Not sure why we configure this at the end of the job again.
			// Assuming the old "just to be safe" comment was intended to leave a
			// reasonable default for the next job.
			m_gerber_header += "%FSLAX26Y26*%\n";
			// set units to inches
			m_gerber_header += "%MOIN*%\n";

		} else {
			// label our layers
			m_gerber_header += QString("%LN%1*%\n").arg(mainLayerName.toUpper());

			//just to be safe: G90 (absolute coords) and G70 (inches)
			m_gerber_header += "G90*\nG70*\n";
		}

		// now write the footer
		// comment to indicate end-of-sketch
		m_gerber_paths += QString("G04 End of %1*\n").arg(mainLayerName);

		// write gerber end-of-program
		m_gerber_paths += "M02*";
	}

	return invalidCount;
}

void SVG2gerber::normalizeSVG() {
	QDomElement root = m_SVGDom.documentElement();

	//  convert to paths
	convertShapes2paths(root);

	//  get rid of transforms
	SvgFlattener flattener;
	flattener.flattenChildren(root, SvgAttributesMap());

}

void SVG2gerber::convertShapes2paths(QDomNode node) {
	// I'm a leaf node.  make me a path
	//TODO: this should strip svg: namespaces
	if(!node.hasChildNodes()) {
		QString tag = node.nodeName().toLower();
		QDomElement element = node.toElement();
		QDomElement path;

		//DebugDialog::debug("converting child to path: " + tag);

		if(tag=="polygon") {
			path = element;
		}
		else if(tag=="polyline") {
			path = element;
		}
		else if(tag=="rect") {
			path = element;
		}
		else if(tag=="circle") {
			path = element;
		}
		else if(tag=="line") {
			path = element;
		}
		else if(tag=="ellipse") {
			path = ellipse2path(element);
		}
		else if((tag=="path") || (tag=="svg:path")) {
			path = element;
		}
		else if (tag == "g") {
			// no op
		}
		else if (tag == "#comment") {
			// no op
		}
		else if (tag == "#text") {
			// no op
		}
		else {
			DebugDialog::debug("svg2gerber ignoring SVG element: " + tag);
		}

		copyStyles(element, path);

		// add the path and delete the primitive element (is this ok for paths?)
		QDomNode parent = node.parentNode();
		parent.replaceChild(path, node);

		return;
	}

	// recurse the children
	QDomNodeList tagList = node.childNodes();

	//DebugDialog::debug("child nodes: " + QString::number(tagList.length()));
	for(int i = 0; i < tagList.length(); i++) {
		convertShapes2paths(tagList.item(i));
	}
}

void SVG2gerber::copyStyles(QDomElement source, QDomElement dest) {
	QStringList attrList;
	attrList << "stroke" << "fill" << "stroke-width" << "style";

	for (int i = 0; i < attrList.size(); ++i) {
		if (source.hasAttribute(attrList.at(i)))
			dest.setAttribute(attrList.at(i), source.attribute(attrList.at(i)));
	}
}

QTransform SVG2gerber::parseTransform(QDomElement element) {
	QTransform transform = QTransform();

	QString svgTransform = element.attribute("transform");

	return transform;
}

int SVG2gerber::allPaths2gerber(ForWhy forWhy) {
	int invalidPathsCount = 0;
	QHash<QString, QString> apertureMap;
	QString current_dcode;
	int dcode_index = 10;
	bool light_on = false;
	int currentx = -1;
	int currenty = -1;

	m_holeApertures.clear();
	m_platedApertures.clear();

	// iterates through all circles, rects, lines and paths
	//  1. check if we already have an aperture
	//      if aperture does not exist, add it to the header
	//  2. switch to this aperture
	//  3. draw it at the correct path/location

	QDomNodeList circleList = m_SVGDom.elementsByTagName("circle");
	//DebugDialog::debug("circles to gerber: " + QString::number(circleList.length()));

	QDomNodeList rectList = m_SVGDom.elementsByTagName("rect");
	//DebugDialog::debug("rects to gerber: " + QString::number(rectList.length()));

	QDomNodeList polyList = m_SVGDom.elementsByTagName("polygon");
	//DebugDialog::debug("polygons to gerber: " + QString::number(polyList.length()));

	QDomNodeList polyLineList = m_SVGDom.elementsByTagName("polyline");
	//DebugDialog::debug("polylines to gerber: " + QString::number(polyLineList.length()));

	QDomNodeList lineList = m_SVGDom.elementsByTagName("line");
	//DebugDialog::debug("lines to gerber: " + QString::number(lineList.length()));

	QDomNodeList pathList = m_SVGDom.elementsByTagName("path");
	//DebugDialog::debug("paths to gerber: " + QString::number(pathList.length()));

	// if this is the board outline, use it as the contour
	if (forWhy == ForOutline) {
		//DebugDialog::debug("drawing board outline");

		// switch aperture to the only one used for contour: note this is the last one on the list: the aperture is added at the end of this function
		m_gerber_paths += m_G54 + "D10*\n";
	}

	// circles
	for(int i = 0; i < circleList.length(); i++) {
		QDomElement circle = circleList.item(i).toElement();

		double centerx = circle.attribute("cx").toDouble();
		double centery = circle.attribute("cy").toDouble();
		double r = circle.attribute("r").toDouble();
		if (fabs(r) < 0.001) continue; // Ignore circles smaller then 1 micro inch

		QString drillAttribute = circle.attribute("drill", "");
		bool noDrill = (drillAttribute.compare("0") == 0 || drillAttribute.compare("no", Qt::CaseInsensitive) == 0 || drillAttribute.compare("false", Qt::CaseInsensitive) == 0);

		double stroke_width = circle.attribute("stroke-width").toDouble();
		double hole = ((2*r) - stroke_width) / milsPerInch;  // convert mils (standard fritzing resolution) to inches
		noDrill |= (qFuzzyIsNull(hole) || hole < 0); // Don't drill holes with a radius <= 0

		if (forWhy == ForDrill) {
			if (noDrill) continue;

			QString drill_cx = QString("%1").arg((int) (centerx * 10), 6, 10, QChar('0'));				// drill file is in inches 00.0000, converting mils to 10000ths
			QString drill_cy = QString("%1").arg((int) (flipy(centery) * 10), 6, 10, QChar('0'));				// drill file is in inches 00.0000, converting mils to 10000ths
			QString aperture = QString("C%1").arg(hole, 0, 'f');
			QString loc = "X" + drill_cx + "Y" + drill_cy;
			if (stroke_width == 0) m_holeApertures.insert(aperture, loc);
			else m_platedApertures.insert(aperture, loc);
			continue;
		}

		QString aperture;

		QString cx = f2gerber(centerx);
		QString cy = f2gerber(flipy(centery));

		QString fill = circle.attribute("fill");

		double diam = ((2*r) + stroke_width)/milsPerInch;
		if (forWhy == ForMask) {
			diam += 2 * MaskClearance;
		}

		if ((forWhy != ForCopper && fill=="none" && forWhy != ForMask) || (forWhy == ForCopper && noDrill)) {
			aperture = QString("C,%1X%2").arg(diam, 0, 'f').arg(hole);
		}
		else {
			aperture = QString("C,%1").arg(diam, 0, 'f');
		}


		// add aperture to defs if we don't have it yet
		if(!apertureMap.contains(aperture)) {
			apertureMap[aperture] = QString::number(dcode_index);
			m_gerber_header += "%ADD" + QString::number(dcode_index) + aperture + "*%\n";
			dcode_index++;
		}

		if (forWhy != ForOutline) {
			QString dcode = apertureMap[aperture];
			if(current_dcode != dcode) {
				//switch to correct aperture
				m_gerber_paths += m_G54 + "D" + dcode + "*\n";
				current_dcode = dcode;
			}
			//flash
			m_gerber_paths += "X" + cx + "Y" + cy + "D03*\n";
		}
		else {
			standardAperture(circle, apertureMap, current_dcode, dcode_index, 0);

			// create circle outline
			m_gerber_paths += QString(
					"G01X%1Y%2D02*\n"
					"G75*\n"
					"G03X%1Y%2I%3J0D01*\n"
				)
				.arg(f2gerber(centerx + r)
					,f2gerber(flipy(centery))
					,f2gerber(-r)
				);
			m_gerber_paths += "G01*\n";
		}
	}

	if (forWhy != ForDrill) {
		// rects
		for(int j = 0; j < rectList.length(); j++) {
			QDomElement rect = rectList.item(j).toElement();

			QString aperture;

			double width = rect.attribute("width").toDouble();
			double height = rect.attribute("height").toDouble();

			double rx = rect.attribute("rx", "0").toDouble();
			double ry = rect.attribute("ry", "0").toDouble();
			if (!(qFuzzyIsNull(rx) && qFuzzyIsNull(ry))) {
				// not sure how to do rounded rects in gerber
				invalidPathsCount++;
				continue;
			}

			if (qFuzzyIsNull(width)) continue;
			if (qFuzzyIsNull(height)) continue;

			double x = rect.attribute("x").toDouble();
			double y = rect.attribute("y").toDouble();
			double centerx = x + (width/2.0);
			double centery = y + (height/2.0);
			QString cx = f2gerber(centerx);
			QString cy = f2gerber(flipy(centery));

			QString fill = rect.attribute("fill");
			double stroke_width = rect.attribute("stroke-width").toDouble();

			double totalx = (width + stroke_width)/milsPerInch;
			double totaly = (height + stroke_width)/milsPerInch;
			double holex = (width - stroke_width)/milsPerInch;
			double holey = (height - stroke_width)/milsPerInch;

			if (forWhy == ForMask) {
				totalx += 2.0 * MaskClearance;
				totaly += 2.0 * MaskClearance;
			}


			if(forWhy != ForCopper && fill=="none" && forWhy != ForMask) {
				aperture = QString("R,%1X%2X%3X%4").arg(totalx, 0, 'f').arg(totaly, 0, 'f').arg(holex, 0, 'f').arg(holey, 0, 'f');
			}
			else {
				aperture = QString("R,%1X%2").arg(totalx, 0, 'f').arg(totaly, 0, 'f');
			}

			// add aperture to defs if we don't have it yet
			if(!apertureMap.contains(aperture)) {
				apertureMap[aperture] = QString::number(dcode_index);
				m_gerber_header += "%ADD" + QString::number(dcode_index) + aperture + "*%\n";
				dcode_index++;
			}

			bool doLines = false;
			if (forWhy == ForOutline) doLines = true;
			else if (forWhy == ForSilk && fill == "none") doLines = true;

			if (!doLines) {
				QString dcode = apertureMap[aperture];
				if(current_dcode != dcode) {
					//switch to correct aperture
					m_gerber_paths += m_G54 + "D" + dcode + "*\n";
					current_dcode = dcode;
				}
				//flash
				m_gerber_paths += "X" + cx + "Y" + cy + "D03*\n";
			}
			else {
				// draw 4 lines

				standardAperture(rect, apertureMap, current_dcode, dcode_index, 0);
				m_gerber_paths += "X" + f2gerber(x) + "Y" + f2gerber(flipy(y)) + "D02*\n";
				m_gerber_paths += "X" + f2gerber(x+width) + "Y" + f2gerber(flipy(y)) + "D01*\n";
				m_gerber_paths += "X" + f2gerber(x+width) + "Y" + f2gerber(flipy(y+height)) + "D01*\n";
				m_gerber_paths += "X" + f2gerber(x) + "Y" + f2gerber(flipy(y+height)) + "D01*\n";
				m_gerber_paths += "X" + f2gerber(x) + "Y" + f2gerber(flipy(y)) + "D01*\n";
				m_gerber_paths += "D02*\n";
			}
		}

		// lines - NOTE: this assumes a circular aperture
		for(int k = 0; k < lineList.length(); k++) {
			QDomElement line = lineList.item(k).toElement();

			// Note: should be no forWhy == ForMask cases

			double x1 = line.attribute("x1").toDouble();
			double y1 = line.attribute("y1").toDouble();
			double x2 = line.attribute("x2").toDouble();
			double y2 = line.attribute("y2").toDouble();

			standardAperture(line, apertureMap, current_dcode, dcode_index, 0);

			// turn off light if we are not continuing along a path
			if ((y1 != currenty) || (x1 != currentx)) {
				if (light_on) {
					m_gerber_paths += "D02*\n";
					// Assignment of light_on to false was removed from this line because it is overwritten to true below.
				}
			}

			//go to start - light off
			m_gerber_paths += "X" + f2gerber(x1) + "Y" + f2gerber(flipy(y1)) + "D02*\n";
			//go to end point - light on
			m_gerber_paths += "X" + f2gerber(x2) + "Y" + f2gerber(flipy(y2)) + "D01*\n";
			light_on = true;
			currentx = x2;
			currenty = y2;
		}

		// polys - NOTE: assumes comma- or space- separated formatting
		for(int p = 0; p < polyList.length(); p++) {
			QDomElement polygon = polyList.item(p).toElement();
			doPoly(polygon, forWhy, true, apertureMap, current_dcode, dcode_index);
		}
		for(int p = 0; p < polyLineList.length(); p++) {
			QDomElement polygon = polyLineList.item(p).toElement();
			doPoly(polygon, forWhy, false, apertureMap, current_dcode, dcode_index);
		}
	}

	// paths - NOTE: this assumes circular aperture
	for(int n = 0; n < pathList.length(); n++) {
		QDomElement path = pathList.item(n).toElement();

		if (forWhy == ForDrill) {
			handleOblongPath(path, dcode_index);  // this is currently a no-op
			continue;
		}

		QString data = path.attribute("d").trimmed();

		const char * slot = SLOT(path2gerbCommandSlot(QChar, bool, QList<double> &, void *));

		PathUserData pathUserData;
		pathUserData.x = 0;
		pathUserData.y = 0;
		pathUserData.pathStarting = true;
		pathUserData.string = "";

		SvgFlattener flattener;
		bool invalid = false;
		try {
			flattener.parsePath(data, slot, pathUserData, this, true);
		}
		catch (const QString & msg) {
			DebugDialog::debug("flattener.parsePath failed " + msg);
			invalid = true;
		}
		catch (char const *str) {
			DebugDialog::debug("flattener.parsePath failed " + QString(str));
			invalid = true;
		}
		catch (...) {
			DebugDialog::debug("flattener.parsePath failed");
			invalid = true;
		}


		// only add paths if they contained gerber-izable path commands (NO CURVES!)
		if (invalid || pathUserData.string.contains("INVALID")) {
			invalidPathsCount++;
			continue;
		}

		// set poly fill if this is actually a filled in shape
		if (hasFill(path) && (forWhy != ForOutline)) {
			// use a minimal aperture. gerbv seems to use the last used aperture for image size calculation
			// the aperture should not matter for the fill, though
			standardAperture(path, apertureMap, current_dcode, dcode_index,  0.1);
			// start poly fill
			m_gerber_paths += "G36*\n";
			m_gerber_paths += pathUserData.string;
			//DebugDialog::debug("path id: " + path.attribute("id"));
			// stop poly fill
			m_gerber_paths += "G37*\n";
		}

		// draw the outline, G36 only does the fill
		if (hasStroke(path) || (forWhy == ForMask) || (forWhy == ForOutline)) {
			double stroke_width = path.attribute("stroke-width").toDouble();
			if (forWhy == ForMask) {
				stroke_width += MaskClearance * 2 * milsPerInch;
			}

			if (path.attribute("stroke-linecap") == "square") {

				if (stroke_width != 0) {
					QString aperture = QString("R,%1X%1").arg(stroke_width/milsPerInch, 0, 'f');

					// add aperture to defs if we don't have it yet
					if (!apertureMap.contains(aperture)) {
						apertureMap[aperture] = QString::number(dcode_index);
						m_gerber_header += "%ADD" + QString::number(dcode_index) + aperture + "*%\n";
						dcode_index++;
					}

					QString dcode = apertureMap[aperture];
					if (current_dcode != dcode) {
						//switch to correct aperture
					m_gerber_paths += m_G54 + "D" + dcode + "*\n";
						current_dcode = dcode;
					}
				}
			}
			else {
				standardAperture(path, apertureMap, current_dcode, dcode_index,  stroke_width);
			}

			m_gerber_paths += pathUserData.string;
		}

		// light off
		m_gerber_paths += "D02*\n";
	}


	if (forWhy == ForOutline) {
		// add circular aperture with 0 width
		m_gerber_header += "%ADD10C,0.008*%\n";
	}

	return invalidPathsCount;
}

void SVG2gerber::doPoly(QDomElement & polygon, ForWhy forWhy, bool closedCurve,
						QHash<QString, QString> & apertureMap, QString & current_dcode, int & dcode_index)
{
	QString points = polygon.attribute("points");
	QStringList pointList = points.split(QRegularExpression("\\s+|,"), Qt::SkipEmptyParts);

	if (pointList.length() < 4) {
		qDebug() << QString("Empty polyline %1").arg(points);
		DebugDialog::debug(QString("Empty polyline %1").arg(points), DebugDialog::Error);
		return;
	}

	QString aperture;

	QString pointString;

	double startx = pointList.at(0).toDouble();
	double starty = pointList.at(1).toDouble();
	// move to start - light off
	pointString += "X" + f2gerber(startx) + "Y" + f2gerber(flipy(starty)) + "D02*\n";

	// iterate through all other points - light on
	for(int pt = 2; pt < pointList.length(); pt +=2) {
		double ptx = pointList.at(pt).toDouble();
		double pty = pointList.at(pt+1).toDouble();
		pointString += "X" + f2gerber(ptx) + "Y" + f2gerber(flipy(pty)) + "D01*\n";
	}

	if (closedCurve) {
		// move back to start point
		pointString += "X" + f2gerber(startx) + "Y" + f2gerber(flipy(starty)) + "D01*\n";
	}

	double stroke_width = polygon.attribute("stroke-width").toDouble();

	// add poly fill if this is actually a filled in shape
	if (hasFill(polygon) && (forWhy != ForOutline)) {
		// use a minimal aperture. gerbv seems to use the last used aperture for image size calculation
		standardAperture(polygon, apertureMap, current_dcode, dcode_index,  0.1);
		// start poly fill
		m_gerber_paths += "G36*\n";
		m_gerber_paths += pointString;
		// stop poly fill
		m_gerber_paths += "G37*\n";
	}

	if (hasStroke(polygon) || (forWhy == ForMask) || (forWhy == ForOutline)) {
		// Some elements are missing a stroke-width
		// TinySVG 1.2 does not specify a default stroke-width, while SVG 2.0 specifies "1".
		stroke_width = fmax(stroke_width, 0.005 * milsPerInch);

		if (forWhy == ForMask) {
			 stroke_width += (MaskClearance * 2 * milsPerInch);
		}
		// draw the outline, G36 only does the fill
		standardAperture(polygon, apertureMap, current_dcode, dcode_index,  stroke_width);
		m_gerber_paths += pointString;
	}

	// light off
	m_gerber_paths += "D02*\n";
}

QString SVG2gerber::standardAperture(QDomElement & element, QHash<QString, QString> & apertureMap, QString & current_dcode, int & dcode_index, double stroke_width) {
	if (stroke_width == 0) {
		stroke_width = element.attribute("stroke-width").toDouble();
	}
	if (stroke_width == 0) return "";

	QString aperture = QString("C,%1").arg(stroke_width/milsPerInch, 0, 'f');

	// add aperture to defs if we don't have it yet
	if (!apertureMap.contains(aperture)) {
		apertureMap[aperture] = QString::number(dcode_index);
		m_gerber_header += "%ADD" + QString::number(dcode_index) + aperture + "*%\n";
		dcode_index++;
	}

	QString dcode = apertureMap[aperture];
	if (current_dcode != dcode) {
		//switch to correct aperture
		m_gerber_paths += m_G54 + "D" + dcode + "*\n";
		current_dcode = dcode;
	}

	return aperture;

}

void SVG2gerber::handleOblongPath(QDomElement & path, int & dcode_index) {
	// this code has not been tested in a long time and is probably obsolete
	return;

	// look for oblong paths
	QDomElement parent = path.parentNode().toElement();

	if (parent.attribute("id").compare("oblong") != 0) return;

	QDomElement nextPath = path.nextSiblingElement("path");
	if (nextPath.isNull()) return;

	QDomElement nextLine = nextPath.nextSiblingElement("line");
	if (nextLine.isNull()) return;

	double diameter = parent.attribute("stroke-width").toDouble();
	double cx1 = nextLine.attribute("x1").toDouble();
	double cy1 = nextLine.attribute("y1").toDouble();
	double cx2 = nextLine.attribute("x2").toDouble();
	double cy2 = nextLine.attribute("y2").toDouble();

	QString drill_aperture = QString("C%1").arg(diameter / milsPerInch, 0, 'f') + "\n";   // convert mils to inches
	if (!m_gerber_header.contains(drill_aperture)) {
		m_gerber_header += "T" + QString::number(dcode_index++) + drill_aperture;
	}
	int ix = m_gerber_header.indexOf(drill_aperture);
	int it = m_gerber_header.lastIndexOf("T", ix);
	m_drill_slots += QString("%1\nX%2Y%3G85X%4Y%5\nG05\n")
					 .arg(m_gerber_header.mid(it, ix - it), 0, 'f')
					 .arg((int) (cx1 * 10), 6, 10, QChar('0'))
					 .arg((int) (flipy(cy1) * 10), 6, 10, QChar('0'))
					 .arg((int) (cx2 * 10), 6, 10, QChar('0'))
					 .arg((int) (flipy(cy2) * 10), 6, 10, QChar('0'));
}

QDomElement SVG2gerber::ellipse2path(QDomElement ellipseElement) {
	// TODO
	return ellipseElement;
}

QString SVG2gerber::path2gerber(QDomElement pathElement) {
	Q_UNUSED(pathElement);
	QString d;

	return d;
}

void SVG2gerber::path2gerbCommandSlot(QChar command, bool relative, QList<double> & args, void * userData) {
	QString gerb_path;
	double x, y;

	auto * pathUserData = (PathUserData *) userData;

	if (command.toLatin1() == 'z' || command.toLatin1() == 'Z') {
		gerb_path = "X" + f2gerber(m_pathstart_x) + "Y" + f2gerber(flipy(m_pathstart_y)) + "D01*\n";
		gerb_path += "D02*\n";
		pathUserData->x = m_pathstart_x;
		pathUserData->y = m_pathstart_y;
		pathUserData->string.append(gerb_path);
		return;
	}

	int argIndex = 0;
	while (argIndex < args.count()) {
		switch(command.toLatin1()) {
		case 'a':
		case 'A':
		case 'c':
		case 'C':
		case 'q':
		case 'Q':
		case 's':
		case 'S':
		case 't':
		case 'T':
			// TODO: implement elliptical arc, etc.
			pathUserData->string.append("INVALID");
			argIndex = args.count();
			break;
		case 'm':
		case 'M':
			if (relative && !pathUserData->pathStarting) {
				pathUserData->x += args[argIndex];
				pathUserData->y += args[argIndex + 1];
			} else {
				pathUserData->x = args[argIndex];
				pathUserData->y = args[argIndex + 1];
			}
			x = pathUserData->x;
			y = pathUserData->y;

			if (argIndex == 0) {
				// treat first 'm' arg pair as a move to
				gerb_path = "X" + f2gerber(x) + "Y" + f2gerber(flipy(y)) + "D02*\n";
				m_pathstart_x = x;
				m_pathstart_y = y;
			} else {
				// treat subsequent 'm' arg pair as line to
				gerb_path = "X" + f2gerber(x) + "Y" + f2gerber(flipy(y)) + "D01*\n";
			}
			pathUserData->pathStarting = false;
			pathUserData->string.append(gerb_path);
			argIndex += 2;
			break;
		case 'v':
		case 'V':
			DebugDialog::debug("'v' and 'V' are now removed by preprocessing; shouldn't be here");
			break;
		case 'h':
		case 'H':
			DebugDialog::debug("'h' and 'H' are now removed by preprocessing; shouldn't be here");
			break;
		case 'l':
		case 'L':
			if (relative) {
				pathUserData->x += args[argIndex];
				pathUserData->y += args[argIndex+1];
			} else {
				pathUserData->x = args[argIndex];
				pathUserData->y = args[argIndex+1];
			}
			gerb_path = "X" + f2gerber(pathUserData->x) + "Y" + f2gerber(flipy(pathUserData->y)) + "D01*\n";
			pathUserData->string.append(gerb_path);
			argIndex += 2;
			break;
		default:
			argIndex = args.count();
			pathUserData->string.append("INVALID");
			break;
		}
	}
}

QString SVG2gerber::f2gerber(double value)
{
	return QString::number(qRound(value * m_f2g));
}

double SVG2gerber::flipy(double y)
{
	return m_boardSize.height() - y;
}
