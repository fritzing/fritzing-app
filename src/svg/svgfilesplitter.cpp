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

$Revision: 6980 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-22 01:45:43 +0200 (Mo, 22. Apr 2013) $

********************************************************************/

#include "svgfilesplitter.h"

#include "../utils/misc.h"
#include "../utils/textutils.h"
//#include "../debugdialog.h"
#include "svgpathparser.h"
#include "svgpathlexer.h"
#include "svgpathrunner.h"

#include <QDomDocument>
#include <QFile>
#include <QtDebug>
#include <QXmlStreamReader>

struct HVConvertData {
	double x;
	double y;
	double subX;
	double subY;
	QString path;
};

void appendPair(QString & path, double a1, double a2) {
	path.append(QString::number(a1));
	path.append(',');
	path.append(QString::number(a2));
	path.append(',');
}

//////////////////////////////////////////////////

SvgFileSplitter::SvgFileSplitter()
{
}

bool SvgFileSplitter::split(const QString & filename, const QString & elementID)
{
	m_byteArray.clear();

	QFile file(filename);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		return false;
	}

	QString contents = file.readAll();
	file.close();

	return splitString(contents, elementID);
}

bool SvgFileSplitter::splitString(QString & contents, const QString & elementID)
{
	m_byteArray.clear();

	// gets rid of some crap inserted by illustrator which can screw up polygons and paths
	contents.replace('\t', ' ');
	contents.replace('\n', ' ');
	contents.replace('\r', ' ');

	// get rid of inkscape stuff too
	TextUtils::cleanSodipodi(contents);

	QString errorStr;
	int errorLine;
	int errorColumn;

	if (!m_domDocument.setContent(contents, true, &errorStr, &errorLine, &errorColumn)) {
        //DebugDialog::debug(QString("parse error: %1 l:%2 c:%3\n\n%4").arg(errorStr).arg(errorLine).arg(errorColumn).arg(contents));
		return false;
	}

	QDomElement root = m_domDocument.documentElement();
	if (root.isNull()) {
		return false;
	}

	if (root.tagName() != "svg") {
		return false;
	}

	root.removeAttribute("space");

	QDomElement element = TextUtils::findElementWithAttribute(root, "id", elementID);
	if (element.isNull()) {
		return false;
	}

	QStringList superTransforms;
	QDomNode parent = element.parentNode();
	while (!parent.isNull()) {
		QDomElement e = parent.toElement();
		if (!e.isNull()) {
			QString transform = e.attribute("transform");
			if (!transform.isEmpty()) {
				superTransforms.append(transform);
			}
		}
		parent = parent.parentNode();
	}

	if (!superTransforms.isEmpty()) {
		element.removeAttribute("id");
	}

	QDomDocument document;
	QDomNode node = document.importNode(element, true);
	document.appendChild(node);
	QString elementText = document.toString();

	if (!superTransforms.isEmpty()) {
		for (int i = 0; i < superTransforms.count() - 1; i++) {
			elementText = QString("<g transform='%1'>%2</g>").arg(superTransforms[i]).arg(elementText);
		}
		elementText = QString("<g id='%1' transform='%2'>%3</g>")
			.arg(elementID)
			.arg(superTransforms[superTransforms.count() - 1])
			.arg(elementText);
	}

	while (!root.firstChild().isNull()) {
		root.removeChild(root.firstChild());
	}

	// at this point the document should contain only the <svg> element and possibly svg info strings
	QString svgOnly = m_domDocument.toString();
	int ix = svgOnly.lastIndexOf("/>");
	if (ix < 0) return false;

	svgOnly[ix] = ' ';
	svgOnly += elementText;
	svgOnly += "</svg>";

	if (!m_domDocument.setContent(svgOnly, true, &errorStr, &errorLine, &errorColumn)) {
		return false;
	}

	m_byteArray = m_domDocument.toByteArray();

	//QString s = m_domDocument.toString();
	//DebugDialog::debug(s);

	return true;
}

const QByteArray & SvgFileSplitter::byteArray() {
	return m_byteArray;
}

const QDomDocument & SvgFileSplitter::domDocument() {
	return m_domDocument;
}

QString SvgFileSplitter::elementString(const QString & elementID) {
	QDomElement root = m_domDocument.documentElement();

	QDomElement mainElement = TextUtils::findElementWithAttribute(root, "id", elementID);
	if (mainElement.isNull()) return ___emptyString___;

	QDomDocument document;
	QDomNode node = document.importNode(mainElement, true);
	document.appendChild(node);

	return document.toString();
}

bool SvgFileSplitter::normalize(double dpi, const QString & elementID, bool blackOnly, double & factor)
{
	// get the viewbox and the width and height
	// then normalize them
	// then normalize all the internal stuff
	// if there are translations, we're probably ok

    factor = 1;

	QDomElement root = m_domDocument.documentElement();
	if (root.isNull()) return false;

	double sWidth, sHeight, vbWidth, vbHeight;
	if (!TextUtils::getSvgSizes(m_domDocument, sWidth, sHeight, vbWidth, vbHeight)) return false;

	root.setAttribute("width", QString("%1in").arg(sWidth));
	root.setAttribute("height", QString("%1in").arg(sHeight));

	root.setAttribute("viewBox", QString("%1 %2 %3 %4").arg(0).arg(0).arg(sWidth * dpi).arg(sHeight * dpi) );

	QDomElement mainElement = root;
	if (!elementID.isEmpty()) {
		mainElement = TextUtils::findElementWithAttribute(root, "id", elementID);
		if (mainElement.isNull()) return false;
	}

	normalizeChild(mainElement, sWidth * dpi, sHeight * dpi, vbWidth, vbHeight, blackOnly);

    factor = sWidth * dpi / vbWidth;

	return true;
}

QPainterPath SvgFileSplitter::painterPath(double dpi, const QString & elementID)
{
	// only partially implemented because so far we only use this for polygons generated for the groundplane

	QPainterPath ppath;

    double factor;
	bool result = normalize(dpi, elementID, false, factor);
	if (!result) return ppath;

	QDomElement root = m_domDocument.documentElement();
	if (root.isNull()) return ppath;

	QDomElement mainElement = TextUtils::findElementWithAttribute(root, "id", elementID);
	if (mainElement.isNull()) return ppath;

	QDomElement childElement = mainElement.firstChildElement();
	while (!childElement.isNull()) {
		painterPathChild(childElement, ppath);
		childElement = childElement.nextSiblingElement();
	}

	return ppath;
}

void SvgFileSplitter::painterPathChild(QDomElement & element, QPainterPath & ppath)
{
	// only partially implemented

	if (element.nodeName().compare("circle") == 0) {
		double cx = element.attribute("cx").toDouble();
		double cy = element.attribute("cy").toDouble();
		double r = element.attribute("r").toDouble();
		double stroke = element.attribute("stroke-width").toDouble();
		ppath.addEllipse(QRectF(cx - r - (stroke / 2), cy - r - (stroke / 2), (r * 2) + stroke, (r * 2) + stroke));
	}
	else if (element.nodeName().compare("line") == 0) {

		/*
		double x1 = element.attribute("x1").toDouble();
		double y1 = element.attribute("y1").toDouble();
		double x2 = element.attribute("x2").toDouble();
		double y2 = element.attribute("y2").toDouble();
		double stroke = element.attribute("stroke-width").toDouble();

		// treat as parallel lines stroke width apart?
		*/
	}
	else if (element.nodeName().compare("rect") == 0) {
		double width = element.attribute("width").toDouble();
		double height = element.attribute("height").toDouble();
		double x = element.attribute("x").toDouble();
		double y = element.attribute("y").toDouble();
		double stroke = element.attribute("stroke-width").toDouble();
		double rx = element.attribute("rx").toDouble();
		double ry = element.attribute("ry").toDouble();
		if (rx != 0 || ry != 0) { 
			ppath.addRoundedRect(x - (stroke / 2), y - (stroke / 2), width + stroke, height + stroke, rx, ry);
		}
		else {
			ppath.addRect(x - (stroke / 2), y - (stroke / 2), width + stroke, height + stroke);
		}
	}
	else if (element.nodeName().compare("ellipse") == 0) {
		double cx = element.attribute("cx").toDouble();
		double cy = element.attribute("cy").toDouble();
		double rx = element.attribute("rx").toDouble();
		double ry = element.attribute("ry").toDouble();
		double stroke = element.attribute("stroke-width").toDouble();
		ppath.addEllipse(QRectF(cx - rx - (stroke / 2), cy - ry - (stroke / 2), (rx * 2) + stroke, (ry * 2) + stroke));
	}
	else if (element.nodeName().compare("polygon") == 0 || element.nodeName().compare("polyline") == 0) {
		QString data = element.attribute("points");
		if (!data.isEmpty()) {
			const char * slot = SLOT(painterPathCommandSlot(QChar, bool, QList<double> &, void *));
			PathUserData pathUserData;
			pathUserData.pathStarting = true;
			pathUserData.painterPath = &ppath;
            if (parsePath(data, slot, pathUserData, this, false)) {
			}
		}
	}
	else if (element.nodeName().compare("path") == 0) {
		/*
		QString data = element.attribute("d").trimmed();
		if (!data.isEmpty()) {
			const char * slot = SLOT(normalizeCommandSlot(QChar, bool, QList<double> &, void *));
			PathUserData pathUserData;
			pathUserData.pathStarting = true;
			pathUserData.sNewHeight = sNewHeight;
			pathUserData.sNewWidth = sNewWidth;
			pathUserData.vbHeight = vbHeight;
			pathUserData.vbWidth = vbWidth;
            if (parsePath(data, slot, pathUserData, this, true)) {
				element.setAttribute("d", pathUserData.string);
			}
		}
		*/
	}
	else {
		QDomElement childElement = element.firstChildElement();
		while (!childElement.isNull()) {
			painterPathChild(childElement, ppath);
			childElement = childElement.nextSiblingElement();
		}
	}
}

void SvgFileSplitter::normalizeTranslation(QDomElement & element,
											double sNewWidth, double sNewHeight,
											double vbWidth, double vbHeight)
{
	QString attr = element.attribute("transform");
	if (attr.isEmpty()) return;

	QMatrix matrix = TextUtils::elementToMatrix(element);
	if (matrix.dx() == 0 && matrix.dy() == 0) return;

	double dx = matrix.dx() * sNewWidth / vbWidth;
	double dy = matrix.dy() * sNewHeight / vbHeight;
	if (dx == 0 && dy == 0) return;

	matrix.setMatrix(matrix.m11(), matrix.m12(), matrix.m21(), matrix.m22(), dx, dy);

	TextUtils::setSVGTransform(element, matrix);
}


void SvgFileSplitter::normalizeChild(QDomElement & element,
									 double sNewWidth, double sNewHeight,
									 double vbWidth, double vbHeight, bool blackOnly)
{
	normalizeTranslation(element, sNewWidth, sNewHeight, vbWidth, vbHeight);

	bool doChildren = false;
	QString nodeName = element.nodeName();
	if (nodeName.compare("g") == 0) {
		TextUtils::fixStyleAttribute(element);
		normalizeAttribute(element, "stroke-width", sNewWidth, vbWidth);
		normalizeAttribute(element, "font-size", sNewWidth, vbWidth);
		setStrokeOrFill(element, blackOnly, "black", false);
		doChildren = true;
	}
	else if (nodeName.compare("circle") == 0) {
		TextUtils::fixStyleAttribute(element);
		normalizeAttribute(element, "cx", sNewWidth, vbWidth);
		normalizeAttribute(element, "cy", sNewHeight, vbHeight);
		normalizeAttribute(element, "r", sNewWidth, vbWidth);
		normalizeAttribute(element, "stroke-width", sNewWidth, vbWidth);
		setStrokeOrFill(element, blackOnly, "black", false);
	}
	else if (nodeName.compare("line") == 0) {
		TextUtils::fixStyleAttribute(element);
		normalizeAttribute(element, "x1", sNewWidth, vbWidth);
		normalizeAttribute(element, "y1", sNewHeight, vbHeight);
		normalizeAttribute(element, "x2", sNewWidth, vbWidth);
		normalizeAttribute(element, "y2", sNewHeight, vbHeight);
		normalizeAttribute(element, "stroke-width", sNewWidth, vbWidth);
		setStrokeOrFill(element, blackOnly, "black", false);
	}
	else if (nodeName.compare("rect") == 0) {
		TextUtils::fixStyleAttribute(element);
		normalizeAttribute(element, "width", sNewWidth, vbWidth);
		normalizeAttribute(element, "height", sNewHeight, vbHeight);
		normalizeAttribute(element, "x", sNewWidth, vbWidth);
		normalizeAttribute(element, "y", sNewHeight, vbHeight);
		normalizeAttribute(element, "stroke-width", sNewWidth, vbWidth);

		// rx, ry for rounded rects
		if (!element.attribute("rx").isEmpty()) {
			normalizeAttribute(element, "rx", sNewWidth, vbWidth);
		}
		if (!element.attribute("ry").isEmpty()) {
			normalizeAttribute(element, "ry", sNewHeight, vbHeight);
		}
		setStrokeOrFill(element, blackOnly, "black", false);
	}
	else if (nodeName.compare("ellipse") == 0) {
		TextUtils::fixStyleAttribute(element);
		normalizeAttribute(element, "cx", sNewWidth, vbWidth);
		normalizeAttribute(element, "cy", sNewHeight, vbHeight);
		normalizeAttribute(element, "rx", sNewWidth, vbWidth);
		normalizeAttribute(element, "ry", sNewHeight, vbHeight);
		normalizeAttribute(element, "stroke-width", sNewWidth, vbWidth);
		setStrokeOrFill(element, blackOnly, "black", false);
	}
	else if (nodeName.compare("polygon") == 0 || nodeName.compare("polyline") == 0) {
		TextUtils::fixStyleAttribute(element);
		normalizeAttribute(element, "stroke-width", sNewWidth, vbWidth);
		QString data = element.attribute("points");
		if (!data.isEmpty()) {
			const char * slot = SLOT(normalizeCommandSlot(QChar, bool, QList<double> &, void *));
			PathUserData pathUserData;
			pathUserData.pathStarting = true;
			pathUserData.sNewHeight = sNewHeight;
			pathUserData.sNewWidth = sNewWidth;
			pathUserData.vbHeight = vbHeight;
			pathUserData.vbWidth = vbWidth;
            if (parsePath(data, slot, pathUserData, this, false)) {
				pathUserData.string.remove(0, 1);			// get rid of the "M"
				element.setAttribute("points", pathUserData.string);
			}
		}
		setStrokeOrFill(element, blackOnly, "black", false);
	}
	else if (nodeName.compare("path") == 0) {
		TextUtils::fixStyleAttribute(element);
		normalizeAttribute(element, "stroke-width", sNewWidth, vbWidth);
		setStrokeOrFill(element, blackOnly, "black", false);
		QString data = element.attribute("d").trimmed();
		if (!data.isEmpty()) {
			const char * slot = SLOT(normalizeCommandSlot(QChar, bool, QList<double> &, void *));
			PathUserData pathUserData;
			pathUserData.pathStarting = true;
			pathUserData.sNewHeight = sNewHeight;
			pathUserData.sNewWidth = sNewWidth;
			pathUserData.vbHeight = vbHeight;
			pathUserData.vbWidth = vbWidth;
            if (parsePath(data, slot, pathUserData, this, true)) {
				element.setAttribute("d", pathUserData.string);
			}
		}
	}
	else if (nodeName.compare("text") == 0) {
		TextUtils::fixStyleAttribute(element);
		normalizeAttribute(element, "x", sNewWidth, vbWidth);
		normalizeAttribute(element, "y", sNewHeight, vbHeight);
		normalizeAttribute(element, "stroke-width", sNewWidth, vbWidth);
		normalizeAttribute(element, "font-size", sNewWidth, vbWidth);
		setStrokeOrFill(element, blackOnly, "black", false);
	}
	else if (nodeName.compare("linearGradient") == 0) {
		if (element.attribute("gradientUnits").compare("userSpaceOnUse") == 0) {
			normalizeAttribute(element, "x1", sNewWidth, vbWidth);
			normalizeAttribute(element, "y1", sNewWidth, vbWidth);
			normalizeAttribute(element, "x2", sNewWidth, vbWidth);
			normalizeAttribute(element, "y2", sNewWidth, vbWidth);
		}
		else {
			//DebugDialog::debug(QString("unable to handle linearGradient with gradientUnits=%1").arg(element.attribute("gradientUnits")));
		}
	}
	else if (nodeName.compare("radialGradient") == 0) {
		if (element.attribute("gradientUnits").compare("userSpaceOnUse") == 0) {
			normalizeAttribute(element, "cx", sNewWidth, vbWidth);
			normalizeAttribute(element, "cy", sNewWidth, vbWidth);
			normalizeAttribute(element, "fx", sNewWidth, vbWidth);
			normalizeAttribute(element, "fy", sNewWidth, vbWidth);
			normalizeAttribute(element, "r", sNewWidth, vbWidth);
		}
		else {
			//DebugDialog::debug(QString("unable to handle radialGradient with gradientUnits=%1").arg(element.attribute("gradientUnits")));
		}
	}
	else {
		doChildren = true;
	}

	if (doChildren) {
		QDomElement childElement = element.firstChildElement();
		while (!childElement.isNull()) {
			normalizeChild(childElement, sNewWidth, sNewHeight, vbWidth, vbHeight, blackOnly);
			childElement = childElement.nextSiblingElement();
		}
	}
}

bool SvgFileSplitter::normalizeAttribute(QDomElement & element, const char * attributeName, double num, double denom)
{
    QString attributeValue = element.attribute(attributeName);
    if (attributeValue.isEmpty()) return true;

    bool ok;
	double n = attributeValue.toDouble(&ok) * num / denom;
    if (!ok) {
        QString string;
        QTextStream stream(&string);
        element.save(stream, 0);
        //DebugDialog::debug("bad attribute " + string);
    }

	element.setAttribute(attributeName, QString::number(n));
	return ok;
}

QString SvgFileSplitter::shift(double x, double y, const QString & elementID, bool shiftTransforms)
{
	QDomElement root = m_domDocument.documentElement();

	QDomElement mainElement = TextUtils::findElementWithAttribute(root, "id", elementID);
	if (mainElement.isNull()) return "";

	QDomElement childElement = mainElement.firstChildElement();
	while (!childElement.isNull()) {
		shiftChild(childElement, x, y, shiftTransforms);
		childElement = childElement.nextSiblingElement();
	}

	QDomDocument document;
	QDomNode node = document.importNode(mainElement, true);
	document.appendChild(node);

	return document.toString();

}

bool SvgFileSplitter::shiftTranslation(QDomElement & element, double x, double y)
{
	QString attr = element.attribute("transform");
	if (attr.isEmpty()) return false;

	QMatrix m0 = TextUtils::elementToMatrix(element);

	bool ok1, ok2, ok3;
	double _x = element.attribute("_x").toDouble(&ok1);
	double _y = element.attribute("_y").toDouble(&ok2);
	double _r = element.attribute("_r").toDouble(&ok3);
	if (ok1 && ok2 && ok3) {
		QMatrix mx = QMatrix().translate(-_x - x, -_y - y) * QMatrix().rotate(_r) * QMatrix().translate(_x + x, _y + y);
		TextUtils::setSVGTransform(element, mx);
		element.removeAttribute("_x");
		element.removeAttribute("_y");
		element.removeAttribute("_r");
		return false;
	}

	// calculate the original cx and cy and retranslate (there must be an easier way)
	// and this doesn't seem to work...

	double sinTheta = m0.m21();
	double cosTheta = m0.m11();
	double cosThetaMinusOne = cosTheta - 1;

	double cx = (m0.dx() + (sinTheta * m0.dy() / cosThetaMinusOne)) / 
			    (cosThetaMinusOne + (sinTheta * sinTheta / cosThetaMinusOne));
	double cy = (m0.dy() - (sinTheta * cx)) / cosThetaMinusOne;

	cx += x;
	cy += y;

	QMatrix m1(m0.m11(), m0.m12(), m0.m21(), m0.m22(), 0, 0);
	QMatrix mx = QMatrix().translate(-cx, -cy) * m1 * QMatrix().translate(cx, cy);

	TextUtils::setSVGTransform(element, mx);
	return false;
}


void SvgFileSplitter::shiftChild(QDomElement & element, double x, double y, bool shiftTransforms)
{
	if (shiftTransforms) {
		shiftTranslation(element, x, y);
	}

	QString nodeName = element.nodeName();
	if (nodeName.compare("circle") == 0 || nodeName.compare("ellipse") == 0) {
		shiftAttribute(element, "cx", x);
		shiftAttribute(element, "cy", y);
	}
	else if (nodeName.compare("line") == 0) {
		shiftAttribute(element, "x1", x);
		shiftAttribute(element, "y1", y);
		shiftAttribute(element, "x2", x);
		shiftAttribute(element, "y2", y);
	}
	else if (nodeName.compare("rect") == 0) {
		shiftAttribute(element, "x", x);
		shiftAttribute(element, "y", y);
	}
	else if (nodeName.compare("text") == 0) {
		shiftAttribute(element, "x", x);
		shiftAttribute(element, "y", y);
	}
	else if (nodeName.compare("polygon") == 0 || nodeName.compare("polyline") == 0) {
		QString data = element.attribute("points");
		if (!data.isEmpty()) {
			const char * slot = SLOT(shiftCommandSlot(QChar, bool, QList<double> &, void *));
			PathUserData pathUserData;
			pathUserData.pathStarting = true;
			pathUserData.x = x;
			pathUserData.y = y;
            if (parsePath(data, slot, pathUserData, this, false)) {
				pathUserData.string.remove(0, 1);			// get rid of the "M"
				element.setAttribute("points", pathUserData.string);
			}
		}
	}
	else if (nodeName.compare("path") == 0) {
		QString data = element.attribute("d").trimmed();
		if (!data.isEmpty()) {
			const char * slot = SLOT(shiftCommandSlot(QChar, bool, QList<double> &, void *));
			PathUserData pathUserData;
			pathUserData.pathStarting = true;
			pathUserData.x = x;
			pathUserData.y = y;
            if (parsePath(data, slot, pathUserData, this, true)) {
				element.setAttribute("d", pathUserData.string);
			}
		}
	}
	else {
		QDomElement childElement = element.firstChildElement();
		while (!childElement.isNull()) {
			shiftChild(childElement, x, y, shiftTransforms);
			childElement = childElement.nextSiblingElement();
		}
	}
}

void SvgFileSplitter::normalizeCommandSlot(QChar command, bool relative, QList<double> & args, void * userData) {

	Q_UNUSED(relative);			// just normalizing here, so relative is not used

	PathUserData * pathUserData = (PathUserData *) userData;

	double d;
	pathUserData->string.append(command);
	switch(command.toLatin1()) {
		case 'v':
		case 'V':
			//DebugDialog::debug("'v' and 'V' are now removed by preprocessing; shouldn't be here");
			/*
			for (int i = 0; i < args.count(); i++) {
				d = args[i] * pathUserData->sNewHeight / pathUserData->vbHeight;
				pathUserData->string.append(QString::number(d));
				if (i < args.count() - 1) {
					pathUserData->string.append(',');
				}
			}
			*/
			break;
		case 'h':
		case 'H':
			//DebugDialog::debug("'h' and 'H' are now removed by preprocessing; shouldn't be here");
			/*
			for (int i = 0; i < args.count(); i++) {
				d = args[i] * pathUserData->sNewWidth / pathUserData->vbWidth;
				pathUserData->string.append(QString::number(d));
				if (i < args.count() - 1) {
					pathUserData->string.append(',');
				}
			}
			*/
			break;
		case 'a':
		case 'A':
			for (int i = 0; i < args.count(); i++) {
				switch (i % 7) {
					case 0:
					case 5:
						d = args[i] * pathUserData->sNewWidth / pathUserData->vbWidth;
						break;
					case 1:
					case 6:
						d = args[i] * pathUserData->sNewHeight / pathUserData->vbHeight;
						break;
					default:
						d = args[i];
						break;
				}
				pathUserData->string.append(QString::number(d));
				if (i < args.count() - 1) {
					pathUserData->string.append(',');
				}
			}
			break;
		case SVGPathLexer::FakeClosePathChar:
			break;
		default:
			for (int i = 0; i < args.count(); i++) {
				if (i % 2 == 0) {
					d = args[i] * pathUserData->sNewWidth / pathUserData->vbWidth;
				}
				else {
					d = args[i] * pathUserData->sNewHeight / pathUserData->vbHeight;
				}
				pathUserData->string.append(QString::number(d));
				if (i < args.count() - 1) {
					pathUserData->string.append(',');
				}
			}
			break;
	}
}

void SvgFileSplitter::painterPathCommandSlot(QChar command, bool relative, QList<double> & args, void * userData) {

	Q_UNUSED(relative);			// just normalizing here, so relative is not used
	Q_UNUSED(command)			// note: painterPathCommandSlot is only partially implemented

	PathUserData * pathUserData = (PathUserData *) userData;

	double dx, dy;
	for (int i = 0; i < args.count(); i += 2) {
		dx = args[i];
		dy = args[i + 1];
		if (i == 0) {
			pathUserData->painterPath->moveTo(dx, dy);
		}
		else {
			pathUserData->painterPath->lineTo(dx, dy);
		}
	}
	pathUserData->painterPath->closeSubpath();


}

void SvgFileSplitter::shiftCommandSlot(QChar command, bool relative, QList<double> & args, void * userData) {

	Q_UNUSED(relative);			// just normalizing here, so relative is not used

	PathUserData * pathUserData = (PathUserData *) userData;

	double d;
	pathUserData->string.append(command);
	switch(command.toLatin1()) {
		case 'v':
		case 'V':
			//DebugDialog::debug("'v' and 'V' are now removed by preprocessing; shouldn't be here");
			/*
			d = args[0];
			if (!relative) {
				 d += pathUserData->y;
			}
			pathUserData->string.append(QString::number(d));
			*/
			break;
		case 'h':
		case 'H':
			//DebugDialog::debug("'h' and 'H' are now removed by preprocessing; shouldn't be here");
			/*
			d = args[0];
			if (!relative) {
				 d += pathUserData->x;
			}
			pathUserData->string.append(QString::number(d));
			*/
			break;
		case 'z':
		case 'Z':
			pathUserData->pathStarting = true;
			break;
		case SVGPathLexer::FakeClosePathChar:
			pathUserData->pathStarting = true;
			break;
		case 'a':
		case 'A':
			for (int i = 0; i < args.count(); i++) {
				d = args[i];
				if (i % 7 == 5) {
					if (!relative) {
						d += pathUserData->x;
					}
				}
				else if (i % 7 == 6) {
					if (!relative) {
						d += pathUserData->y;
					}
				}
				pathUserData->string.append(QString::number(d));
				if (i < args.count() - 1) {
					pathUserData->string.append(',');
				}
			}
			break;
		case 'm':
		case'M':
			standardArgs(relative, pathUserData->pathStarting, args, pathUserData);
			pathUserData->pathStarting = false;
			break;
		default:
			standardArgs(relative, false, args, pathUserData);
			break;
	}
}

void SvgFileSplitter::standardArgs(bool relative, bool starting, QList<double> & args, PathUserData * pathUserData) {
	for (int i = 0; i < args.count(); i++) {
		double d = args[i];
		if (i % 2 == 0) {
			if (!relative || (starting && i == 0)) {
				d += pathUserData->x;
			}
		}
		else {
			if (!relative || (starting && i == 1)) {
				d += pathUserData->y;
			}
		}
		pathUserData->string.append(QString::number(d));
		if (i < args.count() - 1) {
			pathUserData->string.append(',');
		}
	}
}

QVector<QVariant> SvgFileSplitter::simpleParsePath(const QString & data) {
	static QVector<QVariant> emptyStack;
	
	QString dataCopy(data);

	if (!dataCopy.startsWith('M', Qt::CaseInsensitive)) {
		dataCopy.prepend('M');
	}
	while (dataCopy.at(dataCopy.length() - 1).isSpace()) {
		dataCopy.remove(dataCopy.length() - 1, 1);
	}
	QChar last = dataCopy.at(dataCopy.length() - 1);
	if (last != 'z' && last != 'Z' && last != SVGPathLexer::FakeClosePathChar) {
		dataCopy.append(SVGPathLexer::FakeClosePathChar);
	}

	SVGPathLexer lexer(dataCopy);
	SVGPathParser parser;
	bool result = parser.parse(&lexer);
	if (!result) {
		//DebugDialog::debug(QString("svg path parse failed %1").arg(dataCopy));
		return emptyStack;
	}

	return parser.symStack();
}


bool SvgFileSplitter::parsePath(const QString & dataString, const char * slot, PathUserData & pathUserData, QObject * slotTarget, bool convertHV) {
	QVector<QVariant> symStack = simpleParsePath(dataString);

	if (convertHV && (dataString.contains("h", Qt::CaseInsensitive) || dataString.contains("v",  Qt::CaseInsensitive))) 
	{  
		SVGPathRunner svgPathRunner;
		HVConvertData data;
		data.x = data.y = data.subX = data.subY = 0;
		data.path = "";
		connect(&svgPathRunner, SIGNAL(commandSignal(QChar, bool, QList<double> &, void *)), 
				this, SLOT(convertHVSlot(QChar, bool, QList<double> &, void *)), 
				Qt::DirectConnection);
		svgPathRunner.runPath(symStack, &data);
		return parsePath(data.path, slot, pathUserData, slotTarget, false);
	}

	SVGPathRunner svgPathRunner;
    connect(&svgPathRunner, SIGNAL(commandSignal(QChar, bool, QList<double> &, void *)), slotTarget, slot, Qt::DirectConnection);
	return svgPathRunner.runPath(symStack, &pathUserData);
}

void SvgFileSplitter::convertHVSlot(QChar command, bool relative, QList<double> & args, void * userData) {
	Q_UNUSED(relative);
	HVConvertData * data = (HVConvertData *) userData;

	double x, y;
	switch(command.toLatin1()) {
		case 'M':
			data->path.append(command);
			for (int i = 0; i < args.count(); i += 2) {
				data->x = data->subX = args[i];
				data->y = data->subY = args[i + 1];
				appendPair(data->path, args[i], args[i + 1]);
			}
			data->path.chop(1);
			break;
		case 'm':
			data->path.append(command);
			x = data->x;
			y = data->y;
			for (int i = 0; i < args.count(); i += 2) {
				data->subX = data->x = (x + args[i]);
				data->subY = data->y = (y + args[i + 1]);
				appendPair(data->path, args[i], args[i + 1]);
			}
			data->path.chop(1);
			break;
		case 'L':
		case 'T':
			data->path.append(command);
			for (int i = 0; i < args.count(); i += 2) {
				data->x = args[i];
				data->y = args[i + 1];
				appendPair(data->path, args[i], args[i + 1]);
			}
			data->path.chop(1);
			break;
		case 'l':
		case 't':
			data->path.append(command);
			x = data->x;
			y = data->y;
			for (int i = 0; i < args.count(); i += 2) {
				data->x = x + args[i];
				data->y = y + args[i + 1];
				appendPair(data->path, args[i], args[i + 1]);
			}
			data->path.chop(1);
			break;
		case 'C':
			data->path.append(command);
			for (int i = 0; i < args.count(); i += 6) {
				data->x = args[i + 4];
				data->y = args[i + 5];
				appendPair(data->path, args[i], args[i + 1]);
				appendPair(data->path, args[i + 2], args[i + 3]);
				appendPair(data->path, args[i + 4], args[i + 5]);
			}
			data->path.chop(1);
			break;
		case 'c':
			data->path.append(command);
			x = data->x;
			y = data->y;
			for (int i = 0; i < args.count(); i += 6) {
				data->x = x + args[i + 4];
				data->y = y + args[i + 5];
				appendPair(data->path, args[i], args[i + 1]);
				appendPair(data->path, args[i + 2], args[i + 3]);
				appendPair(data->path, args[i + 4], args[i + 5]);
			}
			data->path.chop(1);
			break;
		case 'S':
		case 'Q':
			data->path.append(command);
			for (int i = 0; i < args.count(); i += 4) {
				data->x = args[i + 2];
				data->y = args[i + 3];
				appendPair(data->path, args[i], args[i + 1]);
				appendPair(data->path, args[i + 2], args[i + 3]);
			}
			data->path.chop(1);
			break;
		case 's':
		case 'q':
			data->path.append(command);
			x = data->x;
			y = data->y;
			for (int i = 0; i < args.count(); i += 4) {
				data->x = x + args[i + 2];
				data->y = y + args[i + 3];
				appendPair(data->path, args[i], args[i + 1]);
				appendPair(data->path, args[i + 2], args[i + 3]);
			}
			data->path.chop(1);
			break;
		case 'Z':
		case 'z':
			data->path.append(command);
			data->x = data->subX;
			data->y = data->subY;
			break;
		case SVGPathLexer::FakeClosePathChar:
			data->path.append(command);
			break;
		case 'A':
			data->path.append(command);
			for (int i = 0; i < args.count(); i += 7) {
				data->x = args[i + 5];
				data->y = args[i + 6];
				appendPair(data->path, args[i], args[i + 1]);
				appendPair(data->path, args[i + 2], args[i + 3]);
				appendPair(data->path, args[i + 4], args[i + 5]);
				data->path.append(QString::number(args[i + 6]));
				data->path.append(',');
			}
			data->path.chop(1);
			break;
		case 'a':
			data->path.append(command);
			x = data->x;
			y = data->y;
			for (int i = 0; i < args.count(); i += 7) {
				data->x = x + args[i + 5];
				data->y = y + args[i + 6];
				appendPair(data->path, args[i], args[i + 1]);
				appendPair(data->path, args[i + 2], args[i + 3]);
				appendPair(data->path, args[i + 4], args[i + 5]);
				data->path.append(QString::number(args[i + 6]));
				data->path.append(',');
			}
			data->path.chop(1);
			break;
		case 'v':
			data->path.append('l');
			y = data->y;
			for (int i = 0; i < args.count(); i++) {
				data->y = y + args[i];
				appendPair(data->path, 0, args[i]);
			}
			data->path.chop(1);
			break;
		case 'h':
			data->path.append('l');
			x = data->x;
			for (int i = 0; i < args.count(); i++) {
				data->x = x + args[i];
				appendPair(data->path, args[i], 0);
			}
			data->path.chop(1);
			break;
		case 'H':
			data->path.append('L');
			for (int i = 0; i < args.count(); i++) {
				data->x = args[i];
				appendPair(data->path, args[i], data->y);
			}
			data->path.chop(1);
			break;
		case 'V':
			data->path.append('L');
			for (int i = 0; i < args.count(); i++) {
				data->y = args[i];
				appendPair(data->path, data->x, args[i]);
			}
			data->path.chop(1);
			break;
		default:
			//DebugDialog::debug(QString("unknown path command %1").arg(command));
			data->path.append(command);
			for (int i = 0; i < args.count(); i++) {
				data->path.append(QString::number(args[i]));
				data->path.append(',');
			}
			data->path.chop(1);
			break;
	}
}

void SvgFileSplitter::setStrokeOrFill(QDomElement & element, bool blackOnly, const QString & color, bool force)
{
	if (!blackOnly) return;

	// if stroke attribute is not empty make it black
	// if fill attribute is not empty and not "none" make it black
	QString stroke = element.attribute("stroke");
	if (!stroke.isEmpty() || force) {
		if (stroke.compare("none") != 0) {
			element.setAttribute("stroke", color);
		}
	}
	QString fill = element.attribute("fill");
	if (!fill.isEmpty()) {
		if (fill.compare("none") != 0) {
			element.setAttribute("fill", color);
		}
	}
}

void SvgFileSplitter::fixStyleAttributeRecurse(QDomElement & element) {
	TextUtils::fixStyleAttribute(element);
	QDomElement childElement = element.firstChildElement();
	while (!childElement.isNull()) {
		fixStyleAttributeRecurse(childElement);
		childElement = childElement.nextSiblingElement();
	}
}

void SvgFileSplitter::fixColorRecurse(QDomElement & element, const QString & newColor, const QStringList & exceptions) {
	TextUtils::fixStyleAttribute(element);
	bool gotException = false;
	QString s = element.attribute("stroke");
	QString f = element.attribute("fill");
	QString id = element.attribute("id");
	foreach (QString e, exceptions) {
		if (s.isEmpty()) {
			if (f.isEmpty()) {
			}
			else {
				if (exceptions.contains(f)) {
					gotException = true;
					break;
				}
			}
		}
		else {		
			if (f.isEmpty()) {
				if (exceptions.contains(s)) {
					gotException = true;
					break;
				}
			}
			else {
				if (exceptions.contains(s) && exceptions.contains(f)) {
					gotException = true;
					break;
				}
			}
		}
	}
	if (!gotException) {
		setStrokeOrFill(element, true, newColor, !id.isEmpty()); 
	}

	QDomElement childElement = element.firstChildElement();
	while (!childElement.isNull()) {
		fixColorRecurse(childElement, newColor, exceptions);
		childElement = childElement.nextSiblingElement();
	}
}

bool SvgFileSplitter::getSvgSizeAttributes(const QString & svg, QString & width, QString & height, QString & viewBox)
{
	QXmlStreamReader xml(svg);
	xml.setNamespaceProcessing(false);

	while (!xml.atEnd()) {
        switch (xml.readNext()) {
        case QXmlStreamReader::StartElement:
			if (xml.name().toString().compare("svg") == 0) {
				width = xml.attributes().value("width").toString();
				height = xml.attributes().value("height").toString();
				viewBox = xml.attributes().value("viewBox").toString();
				return true;	
			}
		default:
			break;
		}
	}

	return false;
}

bool SvgFileSplitter::changeStrokeWidth(const QString & svg, double delta, bool absolute, bool changeOpacity, QByteArray & byteArray) {
	QString errorStr;
	int errorLine;
	int errorColumn;

	QDomDocument domDocument;

	if (!domDocument.setContent(svg, true, &errorStr, &errorLine, &errorColumn)) {
		return false;
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull()) {
		return false;
	}

	if (root.tagName() != "svg") {
		return false;
	}

	changeStrokeWidth(root, delta, absolute, changeOpacity);
	byteArray = domDocument.toByteArray();
	return true;
}

void SvgFileSplitter::changeStrokeWidth(QDomElement & element, double delta, bool absolute, bool changeOpacity) {
	bool ok;
	double sw = element.attribute("stroke-width").toDouble(&ok);
	if (ok) {
		element.setAttribute("stroke-width", QString::number((absolute) ? delta : sw + delta));
	}

    if (changeOpacity) {
	    QString fillo = element.attribute("fill-opacity");
	    if (!fillo.isEmpty()) {
		    element.setAttribute("fill-opacity", "1.0");
	    }

	    QString sillo = element.attribute("stroke-opacity");
	    if (!sillo.isEmpty()) {
		    element.setAttribute("stroke-opacity", "1.0");
	    }
    }

	QDomElement child = element.firstChildElement();
	while (!child.isNull()) {
		changeStrokeWidth(child, delta, absolute, changeOpacity);
		child = child.nextSiblingElement();
	}
}

void SvgFileSplitter::forceStrokeWidth(QDomElement & element, double delta, const QString & stroke, bool recurse, bool fill) {
	bool ok;
	double sw = element.attribute("stroke-width").toDouble(&ok);
    if (!ok) sw = 0;

	element.setAttribute("stroke-width", QString::number(qMax(0, sw + delta)));
	element.setAttribute("stroke", stroke);
    if (fill) element.setAttribute("fill", stroke);

    if (recurse) {
	    QDomElement child = element.firstChildElement();
	    while (!child.isNull()) {
		    forceStrokeWidth(child, delta, stroke, recurse, fill);
		    child = child.nextSiblingElement();
	    }
    }
}

bool SvgFileSplitter::changeColors(const QString & svg, QString & toColor, QStringList & exceptions, QByteArray & byteArray) {
	QString errorStr;
	int errorLine;
	int errorColumn;

	QDomDocument domDocument;

	if (!domDocument.setContent(svg, true, &errorStr, &errorLine, &errorColumn)) {
		return false;
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull()) {
		return false;
	}

	if (root.tagName() != "svg") {
		return false;
	}

	changeColors(root, toColor, exceptions);
	byteArray = domDocument.toByteArray();
	return true;
}

void SvgFileSplitter::changeColors(QDomElement & element, QString & toColor, QStringList & exceptions) {
	QString c = element.attribute("stroke");
	if (!exceptions.contains(c)) {
		element.setAttribute("stroke", toColor);
	}
	c = element.attribute("fill");
	if (!exceptions.contains(c)) {
		element.setAttribute("fill", toColor);
	}

	QString fillo = element.attribute("fill-opacity");
	if (!fillo.isEmpty()) {
		element.setAttribute("fill-opacity", "1.0");
	}

	QString sillo = element.attribute("stroke-opacity");
	if (!sillo.isEmpty()) {
		element.setAttribute("stroke-opacity", "1.0");
	}

	QDomElement child = element.firstChildElement();
	while (!child.isNull()) {
		changeColors(child, toColor, exceptions);
		child = child.nextSiblingElement();
	}
}

bool SvgFileSplitter::shiftAttribute(QDomElement & element, const char * attributeName, double d)
{
	double n = element.attribute(attributeName).toDouble() + d;
	element.setAttribute(attributeName, QString::number(n));
	return true;
}

bool SvgFileSplitter::load(const QString string)
{
	QString errorStr;
	int errorLine;
	int errorColumn;

	return m_domDocument.setContent(string, true, &errorStr, &errorLine, &errorColumn);
}

bool SvgFileSplitter::load(const QString * filename)
{
	QFile file(*filename);

	return load(&file);
}

bool SvgFileSplitter::load(QFile * file)
{
	QString errorStr;
	int errorLine;
	int errorColumn;

	return m_domDocument.setContent(file, true, &errorStr, &errorLine, &errorColumn);
}

QString SvgFileSplitter::toString() {
	return TextUtils::removeXMLEntities(m_domDocument.toString());
}

void SvgFileSplitter::gWrap(const QHash<QString, QString> & attributes)
{
	TextUtils::gWrap(m_domDocument, attributes);
}

void SvgFileSplitter::gReplace(const QString & id)
{
	QDomElement element = TextUtils::findElementWithAttribute(m_domDocument.documentElement(), "id", id);
	if (element.isNull()) return;

	element.setTagName("g");
}

QByteArray SvgFileSplitter::hideText(const QString & filename) {
    QString errorStr;
	int errorLine;
	int errorColumn;
    QDomDocument doc;

    QFile file(filename);
	if (!doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
        return QByteArray();
    }

    QDomElement root = doc.documentElement();
    hideTextAux(root, false);

    return TextUtils::removeXMLEntities(doc.toString()).toUtf8();
}

QByteArray SvgFileSplitter::hideText2(const QByteArray & svg) {
    QString errorStr;
	int errorLine;
	int errorColumn;
    QDomDocument doc;

	if (!doc.setContent(svg, &errorStr, &errorLine, &errorColumn)) {
        return QByteArray();
    }

    QDomElement root = doc.documentElement();
    hideTextAux(root, false);

    return TextUtils::removeXMLEntities(doc.toString()).toUtf8();
}

QString SvgFileSplitter::hideText3(const QString & svg) {
    QString errorStr;
	int errorLine;
	int errorColumn;
    QDomDocument doc;

	if (!doc.setContent(svg, &errorStr, &errorLine, &errorColumn)) {
        return "";
    }

    QDomElement root = doc.documentElement();
    hideTextAux(root, false);

    return TextUtils::removeXMLEntities(doc.toString());
}


void SvgFileSplitter::hideTextAux(QDomElement & parent, bool hideChildren) {
    if (hideChildren || parent.tagName() == "text") {
        parent.setTagName("g");
        hideChildren = true;
    }

    QDomElement child = parent.firstChildElement();
    while (!child.isNull()) {
        hideTextAux(child, hideChildren);
        child = child.nextSiblingElement();
    }
}

QByteArray SvgFileSplitter::showText(const QString & filename, bool & hasText) {
    QString errorStr;
	int errorLine;
	int errorColumn;
    QDomDocument doc;

    QFile file(filename);
	if (!doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
        return QByteArray();
    }

    QDomElement root = doc.documentElement();
    showTextAux(root, hasText, true);
    if (!hasText) {
        return QByteArray();
    }

    return TextUtils::removeXMLEntities(doc.toString()).toUtf8();
}

QByteArray SvgFileSplitter::showText2(const QByteArray & svg, bool & hasText) {
    QString errorStr;
	int errorLine;
	int errorColumn;
    QDomDocument doc;

	if (!doc.setContent(svg, &errorStr, &errorLine, &errorColumn)) {
        return QByteArray();
    }

    QDomElement root = doc.documentElement();
    showTextAux(root, hasText, true);
    if (!hasText) {
        return QByteArray();
    }

    return TextUtils::removeXMLEntities(doc.toString()).toUtf8();
}

QString SvgFileSplitter::showText3(const QString & svg, bool & hasText) {
    QString errorStr;
	int errorLine;
	int errorColumn;
    QDomDocument doc;

	if (!doc.setContent(svg, &errorStr, &errorLine, &errorColumn)) {
        return "";
    }

    QDomElement root = doc.documentElement();
    showTextAux(root, hasText, true);
    if (!hasText) {
        return "";
    }

    return TextUtils::removeXMLEntities(doc.toString());
}

void SvgFileSplitter::showTextAux(QDomElement & parent, bool & hasText, bool root) {
    if (parent.tagName() == "text") {
        hasText = true;
        return;
    }

    if (!root) {
        parent.setTagName("g");
    }

    QDomElement child = parent.firstChildElement();
    while (!child.isNull()) {
        showTextAux(child, hasText, false);
        child = child.nextSiblingElement();
    }
}

