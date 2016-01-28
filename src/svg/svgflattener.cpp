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

$Revision: 6435 $:
$Author: cohen@irascible.com $:
$Date: 2012-09-16 23:31:00 +0200 (So, 16. Sep 2012) $

********************************************************************/

#include "svgflattener.h"
#include "svgpathlexer.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../debugdialog.h"
#include <QMatrix>
#include <QRegExp>
#include <QTextStream>   
#include <qmath.h>

SvgFlattener::SvgFlattener() : SvgFileSplitter()
{
}

void SvgFlattener::flattenChildren(QDomElement &element){

    // recurse the children
    QDomNodeList childList = element.childNodes();

    for(int i = 0; i < childList.length(); i++){
        QDomElement child = childList.item(i).toElement();
        flattenChildren(child);
    }

    //do translate
    if(hasTranslate(element)){
		QList<double> params = TextUtils::getTransformFloats(element);
		if (params.size() == 2) {
            shiftChild(element, params.at(0), params.at(1), false);
			//DebugDialog::debug(QString("translating %1 %2").arg(params.at(0)).arg(params.at(1)));
		}
		else if (params.size() == 6) {
            shiftChild(element, params.at(4), params.at(5), false);
		}
		else if (params.size() == 1) {
            shiftChild(element, params.at(0), 0, false);
			//DebugDialog::debug(QString("translating %1").arg(params.at(0)));
		}
		else {
			DebugDialog::debug("weird transform found");
		}
    }
    else if(hasOtherTransform(element)) {
        QMatrix transform = TextUtils::transformStringToMatrix(element.attribute("transform"));

        //DebugDialog::debug(QString("rotating %1 %2 %3 %4 %5 %6").arg(params.at(0)).arg(params.at(1)).arg(params.at(2)).arg(params.at(3)).arg(params.at(4)).arg(params.at(5)));
        unRotateChild(element, transform);
    }

    // remove transform
    element.removeAttribute("transform");
}

void SvgFlattener::unRotateChild(QDomElement & element, QMatrix transform) {

	// TODO: missing ellipse element

    if(!element.hasChildNodes()) {

		QString sw = element.attribute("stroke-width");
		if (!sw.isEmpty()) {
			bool ok;
			double strokeWidth = sw.toDouble(&ok);
			if (ok) {
                QLineF line(0, 0, strokeWidth, 0);
                QLineF newLine = transform.map(line);
                element.setAttribute("stroke-width", QString::number(newLine.length()));
			}
		}

		// I'm a leaf node.
		QString tag = element.nodeName().toLower();
		if(tag == "path"){
            QString data = element.attribute("d").trimmed();
            if (!data.isEmpty()) {
                const char * slot = SLOT(rotateCommandSlot(QChar, bool, QList<double> &, void *));
                PathUserData pathUserData;
                pathUserData.transform = transform;
                if (parsePath(data, slot, pathUserData, this, true)) {
                    element.setAttribute("d", pathUserData.string);
                }
            }
        }
		else if ((tag == "polygon") || (tag == "polyline")) {
			QString data = element.attribute("points");
			if (!data.isEmpty()) {
				const char * slot = SLOT(rotateCommandSlot(QChar, bool, QList<double> &, void *));
				PathUserData pathUserData;
				pathUserData.transform = transform;
				if (parsePath(data, slot, pathUserData, this, false)) {
					pathUserData.string.remove(0, 1);			// get rid of the "M"
					element.setAttribute("points", pathUserData.string);
				}
			}
		}
        else if(tag == "rect"){
			float x = element.attribute("x").toFloat();
			float y = element.attribute("y").toFloat();
			float width = element.attribute("width").toFloat();
			float height = element.attribute("height").toFloat();
			QRectF r(x, y, width, height);
			QPolygonF poly = transform.map(r);
			if (GraphicsUtils::isRect(poly)) {
				QRectF rect = GraphicsUtils::getRect(poly);
                element.setAttribute("x", QString::number(rect.left()));
                element.setAttribute("y", QString::number(rect.top()));
                element.setAttribute("width", QString::number(rect.width()));
                element.setAttribute("height", QString::number(rect.height()));
			}
			else {
				element.setTagName("polygon");
				QString pts;
				for (int i = 1; i < poly.count(); i++) {
					QPointF p = poly.at(i);
					pts += QString("%1,%2 ").arg(p.x()).arg(p.y());
				}
				pts.chop(1);
				element.setAttribute("points", pts);
			}
        }
        else if(tag == "circle"){
            float cx = element.attribute("cx").toFloat();
            float cy = element.attribute("cy").toFloat();
            float r = element.attribute("r").toFloat();
            QPointF point = transform.map(QPointF(cx,cy));
            element.setAttribute("cx", QString::number(point.x()));
            element.setAttribute("cy", QString::number(point.y()));
            QLineF line(0, 0, r, 0);
            QLineF newLine = transform.map(line);
            element.setAttribute("r", QString::number(newLine.length()));
        }
        else if(tag == "line") {
            float x1 = element.attribute("x1").toFloat();
            float y1 = element.attribute("y1").toFloat();
            QPointF p1 = transform.map(QPointF(x1,y1));
            element.setAttribute("x1", QString::number(p1.x()));
            element.setAttribute("y1", QString::number(p1.y()));

            float x2 = element.attribute("x2").toFloat();
            float y2 = element.attribute("y2").toFloat();
            QPointF p2 = transform.map(QPointF(x2,y2));
            element.setAttribute("x2", QString::number(p2.x()));
            element.setAttribute("y2", QString::number(p2.y()));
        }
		else if (tag == "g") {
			// no op
		}
		else if (tag.isEmpty()) {
		}
		else {
            DebugDialog::debug("Warning! Can't rotate element: " + tag);
		}
        return;
    }

    // recurse the children
    QDomNodeList childList = element.childNodes();

    for(int i = 0; i < childList.length(); i++){
        QDomElement child = childList.item(i).toElement();
        unRotateChild(child, transform);
    }

}

bool SvgFlattener::hasTranslate(QDomElement & element)
{
	QString transform = element.attribute("transform");
	if (transform.isEmpty()) return false;
    if (transform.startsWith("translate")) return true;

    if (transform.startsWith("matrix")) {
		QMatrix matrix = TextUtils::transformStringToMatrix(transform);
		matrix.translate(-matrix.dx(), -matrix.dy());
		if (matrix.isIdentity()) return true;
    }

    return false;
}

bool SvgFlattener::hasOtherTransform(QDomElement & element)
{
	QString transform = element.attribute("transform");
	if (transform.isEmpty()) return false;

	// NOTE: doesn't handle multiple transform attributes...
	return (!transform.contains("translate"));
}

void SvgFlattener::rotateCommandSlot(QChar command, bool relative, QList<double> & args, void * userData) {

    Q_UNUSED(relative);			// just normalizing here, so relative is not used

    PathUserData * pathUserData = (PathUserData *) userData;

    pathUserData->string.append(command);
    double x;
    double y;
	QPointF point;

	for (int i = 0; i < args.count(); ) {
		switch(command.toLatin1()) {
			case 'v':
			case 'V':
				DebugDialog::debug("'v' and 'V' are now removed by preprocessing; shouldn't be here");
				/*
				y = args[i];			
				x = 0; // what is x, really?
				DebugDialog::debug("Warning! Can't rotate path with V");
				*/
				i++;
				break;
			case 'h':
			case 'H':
				DebugDialog::debug("'h' and 'H' are now removed by preprocessing; shouldn't be here");
				/*
				x = args[i];
				y = 0; // what is y, really?
				DebugDialog::debug("Warning! Can't rotate path with H");
				*/
				i++;
				break;
			case SVGPathLexer::FakeClosePathChar:
				break;
			case 'a':
			case 'A':
				// TODO: test whether this is correct
				for (int j = 0; j < 5; j++) {
					pathUserData->string.append(QString::number(args[j]));
					pathUserData->string.append(',');
				}
				x = args[5];
				y = args[6];
				i += 7;
				point = pathUserData->transform.map(QPointF(x,y));
				pathUserData->string.append(QString::number(point.x()));
				pathUserData->string.append(',');
				pathUserData->string.append(QString::number(point.y()));
				if (i < args.count()) {
					pathUserData->string.append(',');
				}
				break;
			default:
				x = args[i];
				y = args[i+1];
				i += 2;
				point = pathUserData->transform.map(QPointF(x,y));
				pathUserData->string.append(QString::number(point.x()));
				pathUserData->string.append(',');
				pathUserData->string.append(QString::number(point.y()));
				if (i < args.count()) {
					pathUserData->string.append(',');
				}
		}
	}
}

void SvgFlattener::replaceElementID(const QString & filename, const QString & svg, QDomDocument & domDocument, const QString & elementID, const QString & altElementID) 
{
    if (!loadDocIf(filename, svg, domDocument)) return;

    QDomElement root = domDocument.documentElement();
	QDomElement element = TextUtils::findElementWithAttribute(root, "id", elementID);
	if (!element.isNull()) {
        element.setAttribute("id", altElementID);
    }
}

void SvgFlattener::flipSMDSvg(const QString & filename, const QString & svg, QDomDocument & flipDoc, const QString & elementID, const QString & altElementID, double printerScale, Qt::Orientations orientation) 
{
    QDomDocument domDocument;
    if (!loadDocIf(filename, svg, domDocument)) return;

    QDomElement root = domDocument.documentElement();
	QDomElement element = TextUtils::findElementWithAttribute(root, "id", elementID);
	if (!element.isNull()) {
		QDomElement altElement = TextUtils::findElementWithAttribute(root, "id", altElementID);
		QString svg = flipSMDElement(domDocument, element, elementID, altElement, altElementID, printerScale,  orientation);
        if (svg.length() > 0) {
            flipDoc.setContent(svg);
        }
	}

}

QString SvgFlattener::flipSMDElement(QDomDocument & domDocument, QDomElement & element, const QString & att, QDomElement altElement, const QString & altAtt, double printerScale, Qt::Orientations orientation) 
{
	Q_UNUSED(printerScale);
	Q_UNUSED(att);

	QMatrix m;
	//QRectF bounds = renderer.boundsOnElement(att);
    QRectF bounds;   // want part bounds, not layer bounds
    double w, h;
    TextUtils::ensureViewBox(domDocument, 1, bounds, false, w, h, false);
	m.translate(bounds.center().x(), bounds.center().y());
	QMatrix mMinus = m.inverted();
    QMatrix cm = mMinus * ((orientation & Qt::Vertical) ? QMatrix().scale(-1, 1) : QMatrix().scale(1, -1)) * m;
	QDomElement newElement = element.cloneNode(true).toElement();

#ifndef QT_NODEBUG
    QString string;
    QTextStream textStream(&string);
    newElement.save(textStream, 1);
#endif

	newElement.removeAttribute("id");
	QDomElement pElement = domDocument.createElement("g");
	pElement.setAttribute("id", altAtt);
	pElement.setAttribute("flipped", true);
    QDomElement mElement = domDocument.createElement("g");
	TextUtils::setSVGTransform(mElement, cm);
    pElement.appendChild(mElement);
    QDomElement tElementChild = domDocument.createElement("g");
    QDomElement tElementParent = tElementChild;

    QDomElement parent = element.parentNode().toElement();
    while (!parent.isNull()) {
        QString transform = parent.attribute("transform");
        if (!transform.isEmpty()) {
            QDomElement t = domDocument.createElement("g");
            t.setAttribute("transform", transform);
            t.appendChild(tElementParent);
            tElementParent = t;
        }
        parent = parent.parentNode().toElement();
    }
    mElement.appendChild(tElementParent);

    tElementChild.appendChild(newElement);
	if (!altElement.isNull()) {
		tElementChild.appendChild(altElement);
		altElement.removeAttribute("id");
	}
 
	domDocument.documentElement().appendChild(pElement);
    return domDocument.toString();
}

bool SvgFlattener::loadDocIf(const QString & filename, const QString & svg, QDomDocument & domDocument) {
	if (domDocument.isNull()) {
        QString errorStr;
	    int errorLine;
	    int errorColumn;
	    bool result;
	    if (filename.isEmpty()) {
		    result = domDocument.setContent(svg, &errorStr, &errorLine, &errorColumn);
	    }
	    else {
		    QFile file(filename);
		    result = domDocument.setContent(&file, &errorStr, &errorLine, &errorColumn);
	    }
	    if (!result) {
		    domDocument.clear();			// probably redundant
		    return false;
	    }
    }

    return true;
}
