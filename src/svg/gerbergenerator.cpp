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

$Revision: 6976 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-21 09:50:09 +0200 (So, 21. Apr 2013) $

********************************************************************/

#include <QMessageBox>
#include <QFileDialog>
#include <QSvgRenderer>
#include <qmath.h>

#include "gerbergenerator.h"
#include "../debugdialog.h"
#include "../fsvgrenderer.h"
#include "../sketch/pcbsketchwidget.h"
#include "../connectors/connectoritem.h"
#include "../connectors/svgidlayer.h"
#include "svgfilesplitter.h"
#include "groundplanegenerator.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../utils/folderutils.h"
#include "../version/version.h"

static const QRegExp AaCc("[aAcCqQtTsS]");
static const QRegExp MFinder("([mM])\\s*([0-9.]*)[\\s,]*([0-9.]*)");
const QRegExp GerberGenerator::MultipleZs("z\\s*[^\\s]");

const QString GerberGenerator::SilkTopSuffix = "_silkTop.gto";
const QString GerberGenerator::SilkBottomSuffix = "_silkBottom.gbo";
const QString GerberGenerator::CopperTopSuffix = "_copperTop.gtl";
const QString GerberGenerator::CopperBottomSuffix = "_copperBottom.gbl";
const QString GerberGenerator::MaskTopSuffix = "_maskTop.gts";
const QString GerberGenerator::MaskBottomSuffix = "_maskBottom.gbs";
const QString GerberGenerator::PasteMaskTopSuffix = "_pasteMaskTop.gtp";
const QString GerberGenerator::PasteMaskBottomSuffix = "_pasteMaskBottom.gbp";
const QString GerberGenerator::DrillSuffix = "_drill.txt";
const QString GerberGenerator::OutlineSuffix = "_contour.gm1";
const QString GerberGenerator::MagicBoardOutlineID = "boardoutline";

const double GerberGenerator::MaskClearanceMils = 5;	

////////////////////////////////////////////

bool pixelsCollide(QImage * image1, QImage * image2, int x1, int y1, int x2, int y2) {
	for (int y = y1; y < y2; y++) {
		for (int x = x1; x < x2; x++) {
			QRgb p1 = image1->pixel(x, y);
			if (p1 == 0xffffffff) continue;

			QRgb p2 = image2->pixel(x, y);
			if (p2 == 0xffffffff) continue;

			//DebugDialog::debug(QString("p1:%1 p2:%2").arg(p1, 0, 16).arg(p2, 0, 16));

			return true;
		}
	}

	return false;
}

////////////////////////////////////////////

void GerberGenerator::exportToGerber(const QString & prefix, const QString & exportDir, ItemBase * board, PCBSketchWidget * sketchWidget, bool displayMessageBoxes) 
{
	if (board == NULL) {
        int boardCount;
		board = sketchWidget->findSelectedBoard(boardCount);
		if (boardCount == 0) {
			DebugDialog::debug("board not found");
			return;
		}
		if (board == NULL) {
			DebugDialog::debug("multiple boards found");
			return;
		}
	}

    exportPickAndPlace(prefix, exportDir, board, sketchWidget, displayMessageBoxes);

	LayerList viewLayerIDs = ViewLayer::copperLayers(ViewLayer::NewBottom);
	int copperInvalidCount = doCopper(board, sketchWidget, viewLayerIDs, "Copper0", CopperBottomSuffix, prefix, exportDir, displayMessageBoxes);
 
    if (sketchWidget->boardLayers() == 2) {
		viewLayerIDs = ViewLayer::copperLayers(ViewLayer::NewTop);
		copperInvalidCount += doCopper(board, sketchWidget, viewLayerIDs, "Copper1", CopperTopSuffix, prefix, exportDir, displayMessageBoxes);
	}

	LayerList maskLayerIDs = ViewLayer::maskLayers(ViewLayer::NewBottom);
	QString maskBottom, maskTop;
	int maskInvalidCount = doMask(maskLayerIDs, "Mask0", MaskBottomSuffix, board, sketchWidget, prefix, exportDir, displayMessageBoxes, maskBottom);

	if (sketchWidget->boardLayers() == 2) {
		maskLayerIDs = ViewLayer::maskLayers(ViewLayer::NewTop);
		maskInvalidCount += doMask(maskLayerIDs, "Mask1", MaskTopSuffix, board, sketchWidget, prefix, exportDir, displayMessageBoxes, maskTop);
	}

	maskLayerIDs = ViewLayer::maskLayers(ViewLayer::NewBottom);
	int pasteMaskInvalidCount = doPasteMask(maskLayerIDs, "PasteMask0", PasteMaskBottomSuffix, board, sketchWidget, prefix, exportDir, displayMessageBoxes);

	if (sketchWidget->boardLayers() == 2) {
		maskLayerIDs = ViewLayer::maskLayers(ViewLayer::NewTop);
		pasteMaskInvalidCount += doPasteMask(maskLayerIDs, "PasteMask1", PasteMaskTopSuffix, board, sketchWidget, prefix, exportDir, displayMessageBoxes);
	}

    LayerList silkLayerIDs = ViewLayer::silkLayers(ViewLayer::NewTop);
	int silkInvalidCount = doSilk(silkLayerIDs, "Silk1", SilkTopSuffix, board, sketchWidget, prefix, exportDir, displayMessageBoxes, maskTop);
    silkLayerIDs = ViewLayer::silkLayers(ViewLayer::NewBottom);
	silkInvalidCount += doSilk(silkLayerIDs, "Silk0", SilkBottomSuffix, board, sketchWidget, prefix, exportDir, displayMessageBoxes, maskBottom);

    // now do it for the outline/contour
    LayerList outlineLayerIDs = ViewLayer::outlineLayers();
    bool empty;
    QString svgOutline = renderTo(outlineLayerIDs, board, sketchWidget, empty);
    if (empty || svgOutline.isEmpty()) {
        displayMessage(QObject::tr("outline is empty"), displayMessageBoxes);
        return;
    }

	svgOutline = cleanOutline(svgOutline);
    // at this point svgOutline must be a single element; a path element may contain cutouts
    QMultiHash<long, ConnectorItem *> treatAsCircle;
	svgOutline = clipToBoard(svgOutline, board, "board", SVG2gerber::ForOutline, "", displayMessageBoxes, treatAsCircle);
	QSizeF svgSize = TextUtils::parseForWidthAndHeight(svgOutline);

    // create outline gerber from svg
    SVG2gerber outlineGerber;
	int outlineInvalidCount = outlineGerber.convert(svgOutline, sketchWidget->boardLayers() == 2, "contour", SVG2gerber::ForOutline, svgSize * GraphicsUtils::StandardFritzingDPI);
	
	//DebugDialog::debug(QString("outline output: %1").arg(outlineGerber.getGerber()));
	saveEnd("contour", exportDir, prefix, OutlineSuffix, displayMessageBoxes, outlineGerber);

	doDrill(board, sketchWidget, prefix, exportDir, displayMessageBoxes);

	if (outlineInvalidCount > 0 || silkInvalidCount > 0 || copperInvalidCount > 0 || maskInvalidCount || pasteMaskInvalidCount) {
		QString s;
		if (outlineInvalidCount > 0) s += QObject::tr("the board outline layer, ");
		if (silkInvalidCount > 0) s += QObject::tr("silkscreen layer(s), ");
		if (copperInvalidCount > 0) s += QObject::tr("copper layer(s), ");
		if (maskInvalidCount > 0) s += QObject::tr("mask layer(s), ");
		if (pasteMaskInvalidCount > 0) s += QObject::tr("paste mask layer(s), ");
		s.chop(2);
		displayMessage(QObject::tr("Unable to translate svg curves in %1").arg(s), displayMessageBoxes);
	}

}

int GerberGenerator::doCopper(ItemBase * board, PCBSketchWidget * sketchWidget, LayerList & viewLayerIDs, const QString & copperName, const QString & copperSuffix, const QString & filename, const QString & exportDir, bool displayMessageBoxes) 
{
    bool empty;
	QString svg = renderTo(viewLayerIDs, board, sketchWidget, empty);
	if (empty || svg.isEmpty()) {
		displayMessage(QObject::tr("%1 layer export is empty.").arg(copperName), displayMessageBoxes);
		return 0;
	}
    
    QMultiHash<long, ConnectorItem *> treatAsCircle;
    foreach (QGraphicsItem * item, sketchWidget->scene()->collidingItems(board)) {
        ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
        if (connectorItem == NULL) continue;
        if (!connectorItem->isPath()) continue;
        if (connectorItem->radius() == 0) continue;

        treatAsCircle.insert(connectorItem->attachedToID(), connectorItem);
    }

	QSizeF svgSize = TextUtils::parseForWidthAndHeight(svg);

	svg = clipToBoard(svg, board, copperName, SVG2gerber::ForCopper, "", displayMessageBoxes, treatAsCircle);
	if (svg.isEmpty()) {
		displayMessage(QObject::tr("%1 layer export is empty (case 2).").arg(copperName), displayMessageBoxes);
		return 0;
	}

	return doEnd(svg, sketchWidget->boardLayers(), copperName, SVG2gerber::ForCopper, svgSize * GraphicsUtils::StandardFritzingDPI, exportDir, filename, copperSuffix, displayMessageBoxes);
}


int GerberGenerator::doSilk(LayerList silkLayerIDs, const QString & silkName, const QString & gerberSuffix, ItemBase * board, PCBSketchWidget * sketchWidget, const QString & filename, const QString & exportDir, bool displayMessageBoxes, const QString & clipString) 
{

	bool empty;
	QString svgSilk = renderTo(silkLayerIDs, board, sketchWidget, empty);
    if (empty || svgSilk.isEmpty()) {
        if (silkLayerIDs.contains(ViewLayer::Silkscreen1)) {
		    displayMessage(QObject::tr("silk layer %1 export is empty").arg(silkName), displayMessageBoxes);
        }
        return 0;
    }

	//QFile f(silkName + "original.svg");
	//f.open(QFile::WriteOnly);
	//QTextStream fs(&f);
	//fs << svgSilk;
	//f.close();

	QSizeF svgSize = TextUtils::parseForWidthAndHeight(svgSilk);

    QMultiHash<long, ConnectorItem *> treatAsCircle;
	svgSilk = clipToBoard(svgSilk, board, silkName, SVG2gerber::ForSilk, clipString, displayMessageBoxes, treatAsCircle);
	if (svgSilk.isEmpty()) {
		displayMessage(QObject::tr("silk export failure"), displayMessageBoxes);
		return 0;
	}

	//QFile f2(silkName + "clipped.svg");
	//f2.open(QFile::WriteOnly);
	//QTextStream fs2(&f2);
	//fs2 << svgSilk;
	//f2.close();

	return doEnd(svgSilk, sketchWidget->boardLayers(), silkName, SVG2gerber::ForSilk, svgSize * GraphicsUtils::StandardFritzingDPI, exportDir, filename, gerberSuffix, displayMessageBoxes);
}


int GerberGenerator::doDrill(ItemBase * board, PCBSketchWidget * sketchWidget, const QString & filename, const QString & exportDir, bool displayMessageBoxes) 
{
    LayerList drillLayerIDs;
    drillLayerIDs << ViewLayer::drillLayers();

	bool empty;
	QString svgDrill = renderTo(drillLayerIDs, board, sketchWidget, empty);
    if (empty || svgDrill.isEmpty()) {
		displayMessage(QObject::tr("exported drill file is empty"), displayMessageBoxes);
        return 0;
    }

	QSizeF svgSize = TextUtils::parseForWidthAndHeight(svgDrill);
    QMultiHash<long, ConnectorItem *> treatAsCircle;
    foreach (QGraphicsItem * item, sketchWidget->scene()->collidingItems(board)) {
        ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
        if (connectorItem == NULL) continue;
        if (!connectorItem->isPath()) continue;
        if (connectorItem->radius() == 0) continue;

        treatAsCircle.insert(connectorItem->attachedToID(), connectorItem);
    }

	svgDrill = clipToBoard(svgDrill, board, "Copper0", SVG2gerber::ForDrill, "", displayMessageBoxes, treatAsCircle);
	if (svgDrill.isEmpty()) {
		displayMessage(QObject::tr("drill export failure"), displayMessageBoxes);
		return 0;
	}

	return doEnd(svgDrill, sketchWidget->boardLayers(), "drill", SVG2gerber::ForDrill, svgSize * GraphicsUtils::StandardFritzingDPI, exportDir, filename, DrillSuffix, displayMessageBoxes);
}

int GerberGenerator::doMask(LayerList maskLayerIDs, const QString &maskName, const QString & gerberSuffix, ItemBase * board, PCBSketchWidget * sketchWidget, const QString & filename, const QString & exportDir, bool displayMessageBoxes, QString & clipString) 
{
	// don't want these in the mask laqyer
	QList<ItemBase *> copperLogoItems;
	sketchWidget->hideCopperLogoItems(copperLogoItems);

	bool empty;
	QString svgMask = renderTo(maskLayerIDs, board, sketchWidget, empty);
	sketchWidget->restoreCopperLogoItems(copperLogoItems);

    if (empty || svgMask.isEmpty()) {
		displayMessage(QObject::tr("exported mask layer %1 is empty").arg(maskName), displayMessageBoxes);
        return 0;
    }

	svgMask = TextUtils::expandAndFill(svgMask, "black", MaskClearanceMils * 2);
	if (svgMask.isEmpty()) {
		displayMessage(QObject::tr("%1 mask export failure (2)").arg(maskName), displayMessageBoxes);
		return 0;
	}

	QSizeF svgSize = TextUtils::parseForWidthAndHeight(svgMask);
    QMultiHash<long, ConnectorItem *> treatAsCircle;
	svgMask = clipToBoard(svgMask, board, maskName, SVG2gerber::ForCopper, "", displayMessageBoxes, treatAsCircle);
	if (svgMask.isEmpty()) {
		displayMessage(QObject::tr("mask export failure"), displayMessageBoxes);
		return 0;
	}

	clipString = svgMask;

	return doEnd(svgMask, sketchWidget->boardLayers(), maskName, SVG2gerber::ForCopper, svgSize * GraphicsUtils::StandardFritzingDPI, exportDir, filename, gerberSuffix, displayMessageBoxes);
}

int GerberGenerator::doPasteMask(LayerList maskLayerIDs, const QString &maskName, const QString & gerberSuffix, ItemBase * board, PCBSketchWidget * sketchWidget, const QString & filename, const QString & exportDir, bool displayMessageBoxes) 
{
	// don't want these in the mask laqyer
	QList<ItemBase *> copperLogoItems;
	sketchWidget->hideCopperLogoItems(copperLogoItems);
	QList<ItemBase *> holes;
	sketchWidget->hideHoles(holes);

	bool empty;
	QString svgMask = renderTo(maskLayerIDs, board, sketchWidget, empty);
	sketchWidget->restoreCopperLogoItems(copperLogoItems);
	sketchWidget->restoreCopperLogoItems(holes);

    if (empty || svgMask.isEmpty()) {
		displayMessage(QObject::tr("exported paste mask layer is empty"), displayMessageBoxes);
        return 0;
    }

    svgMask = sketchWidget->makePasteMask(svgMask, board, GraphicsUtils::StandardFritzingDPI, maskLayerIDs);
    if (svgMask.isEmpty()) return 0;

	QSizeF svgSize = TextUtils::parseForWidthAndHeight(svgMask);
    QMultiHash<long, ConnectorItem *> treatAsCircle;
	svgMask = clipToBoard(svgMask, board, maskName, SVG2gerber::ForCopper, "", displayMessageBoxes, treatAsCircle);
	if (svgMask.isEmpty()) {
		displayMessage(QObject::tr("mask export failure"), displayMessageBoxes);
		return 0;
	}

	return doEnd(svgMask, sketchWidget->boardLayers(), maskName, SVG2gerber::ForCopper, svgSize * GraphicsUtils::StandardFritzingDPI, exportDir, filename, gerberSuffix, displayMessageBoxes);
}

int GerberGenerator::doEnd(const QString & svg, int boardLayers, const QString & layerName, SVG2gerber::ForWhy forWhy, QSizeF svgSize, 
							const QString & exportDir, const QString & prefix, const QString & suffix, bool displayMessageBoxes)
{
    // create mask gerber from svg
    SVG2gerber gerber;
	int invalidCount = gerber.convert(svg, boardLayers == 2, layerName, forWhy, svgSize);

	saveEnd(layerName, exportDir, prefix, suffix, displayMessageBoxes, gerber);

	return invalidCount;
}

bool GerberGenerator::saveEnd(const QString & layerName, const QString & exportDir, const QString & prefix, const QString & suffix, bool displayMessageBoxes, SVG2gerber & gerber)
{

    QString outname = exportDir + "/" +  prefix + suffix;
    QFile out(outname);
	if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
		displayMessage(QObject::tr("%1 layer: unable to save to '%2'").arg(layerName).arg(outname), displayMessageBoxes);
		return false;
	}

    QTextStream stream(&out);
    stream << gerber.getGerber();
	stream.flush();
	out.close();
	return true;

}

void GerberGenerator::displayMessage(const QString & message, bool displayMessageBoxes) {
	// don't use QMessageBox if running conversion as a service
	if (displayMessageBoxes) {
		QMessageBox::warning(NULL, QObject::tr("Fritzing"), message);
		return;
	}

	DebugDialog::debug(message);
}

QString GerberGenerator::clipToBoard(QString svgString, ItemBase * board, const QString & layerName, SVG2gerber::ForWhy forWhy, const QString & clipString, bool displayMessageBoxes, QMultiHash<long, ConnectorItem *> & treatAsCircle) {
	QRectF source = board->sceneBoundingRect();
	source.moveTo(0, 0);
	return clipToBoard(svgString, source, layerName, forWhy, clipString, displayMessageBoxes, treatAsCircle);
}

QString GerberGenerator::clipToBoard(QString svgString, QRectF & boardRect, const QString & layerName, SVG2gerber::ForWhy forWhy, const QString & clipString, bool displayMessageBoxes, QMultiHash<long, ConnectorItem *> & treatAsCircle) {
	// document 1 will contain svg that is easy to convert to gerber
	QDomDocument domDocument1;
	QString errorStr;
	int errorLine;
	int errorColumn;
	bool result = domDocument1.setContent(svgString, &errorStr, &errorLine, &errorColumn);
	if (!result) {
		return "";
	}

	QDomElement root1 = domDocument1.documentElement();
	if (root1.firstChildElement().isNull()) {
		return "";
	}

    if (forWhy != SVG2gerber::ForDrill) {
        QDomNodeList nodeList = root1.elementsByTagName("circle"); 
        QList<QDomElement> justHoles;
        for (int i = 0; i < nodeList.count(); i++) {
            QDomElement circle = nodeList.at(i).toElement();
            if (circle.attribute("id").contains(FSvgRenderer::NonConnectorName)) {
                double sw = circle.attribute("stroke-width").toDouble();
                if (sw == 0) {
                    justHoles << circle;
                }
            }
        }
        foreach (QDomElement circle, justHoles) {
            circle.setTagName("g");
        }
    }

    handleDonuts(root1, treatAsCircle);

    bool multipleContours = false;
    if (forWhy == SVG2gerber::ForOutline) { 
        multipleContours = dealWithMultipleContours(root1, displayMessageBoxes);
    }

	// document 2 will contain svg that must be rasterized for gerber conversion
	QDomDocument domDocument2 = domDocument1.cloneNode(true).toDocument();

	bool anyConverted = false;
    if (TextUtils::squashElement(domDocument1, "text", "", QRegExp())) {
        anyConverted = true; 
	}

	// gerber can't handle ellipses that are rotated, so cull them all
    if (TextUtils::squashElement(domDocument1, "ellipse", "", QRegExp())) {
		anyConverted = true;
    }

    if (TextUtils::squashElement(domDocument1, "rect", "rx", QRegExp())) {
		anyConverted = true;
    }

    if (TextUtils::squashElement(domDocument1, "rect", "ry", QRegExp())) {
		anyConverted = true;
    }

	// gerber can't handle paths with curves
    if (TextUtils::squashElement(domDocument1, "path", "d", AaCc)) {
		anyConverted = true;
    }

	// gerber can't handle multiple subpaths if there are intersections
    if (TextUtils::squashElement(domDocument1, "path", "d", MultipleZs)) {
		anyConverted = true;
    }

    if (TextUtils::squashElement(domDocument1, "image", "", QRegExp())) {
		anyConverted = true;
    }

    // can't handle scaled paths very well. There is probably a deeper bug that needs to be chased down.
    // is this only necessary for contour view?
    QDomNodeList nodeList = root1.elementsByTagName("path"); 
    for (int i = 0; i < nodeList.count(); i++) {
        QDomNode parent = nodeList.at(i);
        while (!parent.isNull()) {
            QString transformString = parent.toElement().attribute("transform");
            if (!transformString.isNull()) {
                QMatrix matrix = TextUtils::transformStringToMatrix(transformString);
                QTransform transform(matrix);
                if (transform.isScaling()) {
                    nodeList.at(i).toElement().setTagName("g");
                    anyConverted = true;
                    break;
                }

            }

            parent = parent.parentNode();
        }
    }

	QVector <QDomElement> leaves1;
	int transformCount1 = 0;
    QDomElement e1 = domDocument1.documentElement();
    TextUtils::collectLeaves(e1, transformCount1, leaves1);

	QVector <QDomElement> leaves2;
	int transformCount2 = 0;
    QDomElement e2 = domDocument2.documentElement();
    TextUtils::collectLeaves(e2, transformCount2, leaves2);

	double res = GraphicsUtils::StandardFritzingDPI;
	// convert from pixel dpi to StandardFritzingDPI
	QRectF sourceRes(boardRect.left() * res / GraphicsUtils::SVGDPI, boardRect.top() * res / GraphicsUtils::SVGDPI, 
					 boardRect.width() * res / GraphicsUtils::SVGDPI, boardRect.height() * res / GraphicsUtils::SVGDPI);
	int twidth = sourceRes.width();
	int theight = sourceRes.height();
	QSize imgSize(twidth + 2, theight + 2);
	QRectF target(0, 0, twidth, theight);

	QImage * clipImage = NULL;
	if (!clipString.isEmpty()) {
		clipImage = new QImage(imgSize, QImage::Format_Mono);
		clipImage->fill(0xffffffff);
		clipImage->setDotsPerMeterX(res * GraphicsUtils::InchesPerMeter);
		clipImage->setDotsPerMeterY(res * GraphicsUtils::InchesPerMeter);

		QXmlStreamReader reader(clipString);
		QSvgRenderer renderer(&reader);		
		QPainter painter;
		painter.begin(clipImage);
		renderer.render(&painter, target);
		painter.end();

#ifndef QT_NO_DEBUG
        clipImage->save(FolderUtils::getTopLevelUserDataStorePath() + "/clip.png");
#endif

	}

	svgString = TextUtils::removeXMLEntities(domDocument1.toString());

    QList<QDomElement> possibleHoles;
	QXmlStreamReader reader(svgString);
	QSvgRenderer renderer(&reader);
	bool anyClipped = false;
    if (forWhy != SVG2gerber::ForOutline) { 
	    for (int i = 0; i < transformCount1; i++) {
		    QString n = QString::number(i);
		    QRectF bounds = renderer.boundsOnElement(n);
		    QMatrix m = renderer.matrixForElement(n);
		    QDomElement element = leaves1.at(i);
		    QRectF mBounds = m.mapRect(bounds);
		    if (mBounds.left() < sourceRes.left() - 0.1|| mBounds.top() < sourceRes.top() - 0.1 || mBounds.right() > sourceRes.right() + 0.1 || mBounds.bottom() > sourceRes.bottom() + 0.1) {
			    if (element.tagName() == "circle") {
                    possibleHoles.append(element);
                }               
                // element is outside of bounds--squash it so it will be clipped
			    // we don't care if the board shape is irregular
			    // since anything printed between the shape and the bounding rectangle 
			    // will be physically clipped when the board is cut out
			    element.setTagName("g");
			    anyClipped = anyConverted = true;
		    }	
	    }
    }


    if (possibleHoles.count() > 0) {
        QList<QDomElement> newHoles;
        int ix = 0;
        foreach (QDomElement element, possibleHoles) {
            QDomElement newElement = element.cloneNode(false).toElement();
            double radius = element.attribute("r").toDouble();
            double sw = element.attribute("stroke-width").toDouble();
            element.parentNode().insertAfter(newElement, element);
            newElement.setAttribute("id", QString("__%1__").arg(ix++));
            newElement.setAttribute("stroke-width", 0);
            newElement.setAttribute("r", QString::number(radius - (sw / 2)));
            newElement.setTagName("circle");
            newHoles.append(newElement);
        }

	    QSvgRenderer renderer(domDocument1.toByteArray());
        for (int i = newHoles.count() - 1; i >= 0; i--) {
            QString id = QString("__%1__").arg(i);
		    QRectF bounds = renderer.boundsOnElement(id);
		    QMatrix m = renderer.matrixForElement(id);
		    QDomElement newElement = newHoles.at(i);
		    QRectF mBounds = m.mapRect(bounds);
		    if (mBounds.left() < sourceRes.left() - 0.1 || mBounds.top() < sourceRes.top() - 0.1 || mBounds.right() > sourceRes.right() + 0.1 || mBounds.bottom() > sourceRes.bottom() + 0.1) {
                // hole is still clipped
                newHoles.removeAt(i);
                newElement.parentNode().removeChild(newElement);
            }
            else {
                // enlarge it a little due to aliasing when the clipped portion is converted to raster and back
                double radius = newElement.attribute("r").toDouble();
                radius += 4;
                newElement.setAttribute("r", QString::number(radius));
                newElement.setAttribute("stroke-width", 2);
            }
        }
    }

	if (clipImage) {
		QImage another(imgSize, QImage::Format_Mono);
		another.fill(0xffffffff);
		another.setDotsPerMeterX(res * GraphicsUtils::InchesPerMeter);
		another.setDotsPerMeterY(res * GraphicsUtils::InchesPerMeter);

		svgString = TextUtils::removeXMLEntities(domDocument1.toString());
		QXmlStreamReader reader(svgString);
		QSvgRenderer renderer(&reader);
		QPainter painter;
		painter.begin(&another);
		renderer.render(&painter, target);
		painter.end();

		for (int i = 0; i < transformCount1; i++) {
			QDomElement element = leaves1.at(i);
			if (element.tagName().compare("g") == 0) {
				// element is already converted to raster space, we'll clip it later
				continue;
			}

			QString n = QString::number(i);
			QRectF bounds = renderer.boundsOnElement(n);
			QMatrix m = renderer.matrixForElement(n);
			QRectF mBounds = m.mapRect(bounds);

			int x1 = qFloor(qMax(0.0, mBounds.left() - sourceRes.left()));          // atmel compiler fails without cast
			int x2 = qCeil(qMin(sourceRes.width(), mBounds.right() - sourceRes.left()));
			int y1 = qFloor(qMax(0.0, mBounds.top() - sourceRes.top()));            // atmel compiler fails without cast
			int y2 = qCeil(qMin(sourceRes.height(), mBounds.bottom() - sourceRes.top()));
			
			if (pixelsCollide(&another, clipImage, x1, y1, x2, y2)) {
				element.setTagName("g");
				anyClipped = anyConverted = true;
			}
		}
	}

	if (anyClipped) {
		// svg has been changed by clipping process so get the string again
		svgString = TextUtils::removeXMLEntities(domDocument1.toString());
	}

    if (anyConverted) {
		for (int i = 0; i < transformCount1; i++) {
			QDomElement element1 = leaves1.at(i);
			if (element1.tagName().compare("g") != 0) {
				// document 1 element svg can be directly converted to gerber
				// so remove it from document 2
				QDomElement element2 = leaves2.at(i);
				element2.setTagName("g");
			}
		}
		

		// expand the svg to fill the space of the image
		QDomElement root2 = domDocument2.documentElement();
		root2.setAttribute("width", QString("%1px").arg(twidth));
		root2.setAttribute("height", QString("%1px").arg(theight));
		if (boardRect.x() != 0 || boardRect.y() != 0) {
			QString viewBox = root2.attribute("viewBox");
			QStringList coords = viewBox.split(" ", QString::SkipEmptyParts);
			coords[0] = QString::number(sourceRes.left());
			coords[1] = QString::number(sourceRes.top());
			root2.setAttribute("viewBox", coords.join(" "));
		}

		QStringList exceptions;
		exceptions << "none" << "";
		QString toColor("#000000");
		SvgFileSplitter::changeColors(root2, toColor, exceptions);

        QImage image(imgSize, QImage::Format_Mono);
		image.setDotsPerMeterX(res * GraphicsUtils::InchesPerMeter);
		image.setDotsPerMeterY(res * GraphicsUtils::InchesPerMeter);

        if (forWhy == SVG2gerber::ForOutline) {		
            QDomNodeList paths = root2.elementsByTagName("path");
            if (paths.count() == 0) {
                // some non-path element makes up the outline
                mergeOutlineElement(image, target, res, domDocument2, svgString, 0, layerName);
            }
            else {
                for (int p = 0; p < paths.count(); p++) {
                    QDomElement path = paths.at(p).toElement();
                    path.setTagName("g");
                }
                for (int p = 0; p < paths.count(); p++) {
                    QDomElement path = paths.at(p).toElement();
                    path.setTagName("path");
                    if (p > 0) {
                        paths.at(p - 1).toElement().setTagName("g");
                    }
                    mergeOutlineElement(image, target, res, domDocument2, svgString, p, layerName);
                }
            }
		}
        else {
		    image.fill(0xffffffff);
		    QByteArray svg = TextUtils::removeXMLEntities(domDocument2.toString()).toUtf8();
		    QSvgRenderer renderer(svg);
		    QPainter painter;
		    painter.begin(&image);
		    renderer.render(&painter, target);
		    painter.end();
		    image.invertPixels();				// need white pixels on a black background for GroundPlaneGenerator

    #ifndef QT_NO_DEBUG
            image.save(FolderUtils::getTopLevelUserDataStorePath() + "/preclip_output.png");
    #endif

		    if (clipImage != NULL) {
			    // can this be done with a single blt using composition mode
			    // if not, grab a scanline instead of testing every pixel
			    for (int y = 0; y < theight; y++) {
				    for (int x = 0; x < twidth; x++) {
					    if (clipImage->pixel(x, y) != 0xffffffff) {
						    image.setPixel(x, y, 0);
					    }
				    }
			    }
		    }

    #ifndef QT_NO_DEBUG
            image.save(FolderUtils::getTopLevelUserDataStorePath() + "/output.png");
    #endif

            QString path = makePath(image, res / GraphicsUtils::StandardFritzingDPI, "#000000");
            svgString.replace("</svg>", path + "</svg>");

            /*

		    GroundPlaneGenerator gpg;
		    gpg.setLayerName(layerName);
		    gpg.setMinRunSize(1, 1);
			gpg.scanImage(image, image.width(), image.height(), GraphicsUtils::StandardFritzingDPI / res, GraphicsUtils::StandardFritzingDPI, "#000000", false, false, QSizeF(0, 0), 0, sourceRes.topLeft());
		    if (gpg.newSVGs().count() > 0) {
                svgString = gpg.mergeSVGs(svgString, "");
		    }

            */
		}
	}

	if (clipImage) delete clipImage;

    return QString(svgString);
}

QString GerberGenerator::cleanOutline(const QString & outlineSvg)
{
	QDomDocument doc;
	doc.setContent(outlineSvg);
	QList<QDomElement> leaves;
    QDomElement root = doc.documentElement();
    TextUtils::collectLeaves(root, leaves);
	QDomNodeList textNodes = root.elementsByTagName("text");
	for (int t = 0; t < textNodes.count(); t++) {
		leaves << textNodes.at(t).toElement();
	}

	if (leaves.count() == 0) return "";
	if (leaves.count() == 1) return outlineSvg;

	if (leaves.count() > 1) {
		for (int i = 0; i < leaves.count(); i++) {
			QDomElement leaf = leaves.at(i);
			if (leaf.attribute("id", "").compare(MagicBoardOutlineID) == 0) {
				for (int j = 0; j < leaves.count(); j++) {
					if (i != j) {
						QDomElement jleaf = leaves.at(j);
						jleaf.parentNode().removeChild(jleaf);
					}
				}

				return doc.toString();
			}
		}
	}

	if (leaves.count() == 0) return "";

	return outlineSvg;
}

void GerberGenerator::mergeOutlineElement(QImage & image, QRectF & target, double res, QDomDocument & document, QString & svgString, int ix, const QString & layerName) {
    
    image.fill(0xffffffff);
	QByteArray svg = TextUtils::removeXMLEntities(document.toString()).toUtf8();

	QSvgRenderer renderer(svg);
	QPainter painter;
	painter.begin(&image);
	renderer.render(&painter, target);
	painter.end();
	image.invertPixels();				// need white pixels on a black background for GroundPlaneGenerator

    #ifndef QT_NO_DEBUG
        image.save(QString("%2/output%1.png").arg(ix).arg(FolderUtils::getTopLevelUserDataStorePath()));
    #else
        Q_UNUSED(ix);
    #endif

	GroundPlaneGenerator gpg;
	gpg.setLayerName(layerName);
	gpg.setMinRunSize(1, 1);
    gpg.scanOutline(image, image.width(), image.height(), GraphicsUtils::StandardFritzingDPI / res, GraphicsUtils::StandardFritzingDPI, "#000000", false, false, QSizeF(0, 0), 0);
	if (gpg.newSVGs().count() > 0) {
        svgString = gpg.mergeSVGs(svgString, "");
	}
}

QString GerberGenerator::makePath(QImage & image, double unit, const QString & colorString) 
{
    double halfUnit = unit / 2;
    QString paths;
    int lineCount = 0;
    for (int y = 0; y < image.height(); y++) {
		bool inWhite = false;
		int whiteStart = 0;
		for (int x = 0; x < image.width(); x++) {
			QRgb current = image.pixel(x, y);
			if (inWhite) {
				if (current == 0xffffffff) {			
					// another white pixel, keep moving
					continue;
				}

				// got black: close up this segment;
				inWhite = false;
                paths += QString("M%1,%2L%3,%2 ").arg(whiteStart + halfUnit).arg(y + halfUnit).arg(x - 1 + halfUnit);
                if (++lineCount == 10) {
                    lineCount = 0;
                    paths += "\n";
                }
			}
			else {
				if (current != 0xffffffff) {				
					// another black pixel, keep moving
					continue;
				}

				inWhite = true;
				whiteStart = x;
			}
		}
	}

    QString path = QString("<path fill='none' stroke='%1' stroke-width='%2' stroke-linecap='square' d='").arg(colorString).arg(unit);
    return path + paths + "' />\n";
}

bool GerberGenerator::dealWithMultipleContours(QDomElement & root, bool displayMessageBoxes) {
    bool multipleContours = false;
    bool contoursOK = true;

    // split path into multiple contours
    QDomNodeList paths = root.elementsByTagName("path");
    // should only be one
    for (int p = 0; p < paths.count() && contoursOK; p++) {
        QDomElement path = paths.at(p).toElement();
        QString originalPath = path.attribute("d", "").trimmed();
        if (MultipleZs.indexIn(originalPath) < 0) continue;

        multipleContours = true;
        QStringList subpaths = path.attribute("d").split("z", QString::SkipEmptyParts);
        foreach (QString subpath, subpaths) {
            if (!subpath.trimmed().startsWith("m", Qt::CaseInsensitive)) {
                contoursOK = false;
                break;
            }
        }
    }

    if (!multipleContours) return false;

    if (!contoursOK) {
        QString msg =
            QObject::tr("Fritzing is unable to process the cutouts in this custom PCB shape. ") +
            QObject::tr("You may need to reload the shape SVG. ") +
            QObject::tr("Fritzing requires that you make cutouts using a shape 'subtraction' or 'difference' operation in your vector graphics editor.");
        displayMessage(msg, displayMessageBoxes);
        return false;
    }

    for (int p = 0; p < paths.count(); p++) {
        QDomElement path = paths.at(p).toElement();
        QString originalPath = path.attribute("d", "").trimmed();
        if (MultipleZs.indexIn(originalPath) >= 0) {
            QStringList subpaths = path.attribute("d").split("z", QString::SkipEmptyParts);
            QString priorM;
            MFinder.indexIn(subpaths.at(0).trimmed());
            priorM += MFinder.cap(1) + MFinder.cap(2) + "," + MFinder.cap(3) + " ";
            for (int i = 1; i < subpaths.count(); i++) {
                QDomElement newPath = path.cloneNode(true).toElement();
                QString z = ((i < subpaths.count() - 1) || originalPath.endsWith("z", Qt::CaseInsensitive)) ? "z" : "";
                QString d = subpaths.at(i).trimmed() + z;
                MFinder.indexIn(d);
                if (d.startsWith("m", Qt::CaseSensitive)) {
                    d = priorM + d;    
                }
                priorM += MFinder.cap(1) + MFinder.cap(2) + "," + MFinder.cap(3) + " ";
                newPath.setAttribute("d",  d);
                path.parentNode().appendChild(newPath);
            }
            path.setAttribute("d", subpaths.at(0) + "z");
        }
    }

    return true;
}

void GerberGenerator::exportPickAndPlace(const QString & prefix, const QString & exportDir, ItemBase * board, PCBSketchWidget * sketchWidget, bool displayMessageBoxes) 
{
    QPointF bottomLeft = board->sceneBoundingRect().bottomLeft();
    QSet<ItemBase *> itemBases;
    foreach (QGraphicsItem * item, sketchWidget->scene()->collidingItems(board)) {
        ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
        if (itemBase == NULL) continue;
        if (itemBase == board) continue;
        if (itemBase->itemType() == ModelPart::Wire) continue;

        itemBase = itemBase->layerKinChief();
        if (!itemBase->isEverVisible()) continue;
        if (itemBase == board) continue;

        itemBases.insert(itemBase->layerKinChief());
    }
    
    QString outname = exportDir + "/" + prefix + "_pnp.txt";
    QFile out(outname);
	if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
		displayMessage(QObject::tr("Unable to save pick and place file: %2").arg(outname), displayMessageBoxes);
		return;
	}

    QTextStream stream(&out);
    stream << "*Pick And Place List\n"
        << "*Company=\n"
        << "*Author=\n"
        //*Tel=
        //*Fax=
        << "*eMail=\n"
        << "*\n"
        << QString("*Project=%1\n").arg(prefix)
        // *Variant=<alle Bauteile>
        << QString("*Date=%1\n").arg(QTime::currentTime().toString())
        << QString("*CreatedBy=Fritzing %1\n").arg(Version::versionString())
        << "*\n"
        << "*\n*Coordinates in mm, always center of component\n"
        << "*Origin 0/0=Lower left corner of PCB\n"
        << "*Rotation in degree (0-360, math. pos.)\n"
        << "*\n"
        << "*No;Value;Package;X;Y;Rotation;Side;Name\n"
        ;

    QStringList valueKeys;
    valueKeys << "resistance" << "capacitance" << "inductance" << "voltage"  << "current" << "power";

    int ix = 1;
    foreach (ItemBase * itemBase, itemBases) {
        QString value;
        foreach (QString valueKey, valueKeys) {
            value = itemBase->modelPart()->localProp(valueKey).toString();
            if (!value.isEmpty()) break;

            value = itemBase->modelPart()->properties().value(valueKey);
            if (!value.isEmpty()) break;
        }

        QPointF loc = itemBase->sceneBoundingRect().center();
        QTransform transform = itemBase->transform();
        // doesn't account for scaling
        double angle = atan2(transform.m12(), transform.m11()) * 180 / M_PI;
        // No;Value;Package;X;Y;Rotation;Side;Name
        QString string = QString("%1;%2;%3;%4;%5;%6;%7;%8\n")
            .arg(ix++)
            .arg(value)
            .arg(itemBase->modelPart()->properties().value("package"))
            .arg(GraphicsUtils::pixels2mm(loc.x() - bottomLeft.x(), GraphicsUtils::SVGDPI))
            .arg(GraphicsUtils::pixels2mm(loc.y() - bottomLeft.y(), GraphicsUtils::SVGDPI))
            .arg(angle)
            .arg(itemBase->viewLayerID() == ViewLayer::Copper1 ? "Top" : "Bottom")
            .arg(itemBase->instanceTitle())
        ;
        stream << string;
        stream.flush();
    }

    out.close();
}

void GerberGenerator::handleDonuts(QDomElement & root1, QMultiHash<long, ConnectorItem *> & treatAsCircle) {
    // most of this would not be necessary if we cached cleaned SVGs

    static const QString unique("%%%%%%%%%%%%%%%%%%%%%%%%_________________________________%%%%%%%%%%%%%%%%%%%%%%%%%%%%%");

    QDomNodeList nodeList = root1.elementsByTagName("path"); 
    if (treatAsCircle.count() > 0) {
        QStringList ids;
        foreach (ConnectorItem * connectorItem, treatAsCircle.values()) {
            ItemBase * itemBase = connectorItem->attachedTo();
            SvgIdLayer * svgIdLayer = connectorItem->connector()->fullPinInfo(itemBase->viewID(), itemBase->viewLayerID());
            DebugDialog::debug(QString("treat as circle %1").arg(svgIdLayer->m_svgId));
            ids << svgIdLayer->m_svgId;
        }

        for (int n = 0; n < nodeList.count(); n++) {
            QDomElement path = nodeList.at(n).toElement();
            QString id = path.attribute("id");
            if (id.isEmpty()) continue;

            DebugDialog::debug(QString("checking for %1").arg(id));
            if (!ids.contains(id)) continue;

            QString pid;
            ConnectorItem * connectorItem = NULL;
            for (QDomElement parent = path.parentNode().toElement(); !parent.isNull(); parent = parent.parentNode().toElement()) {
                pid = parent.attribute("partID");
                if (pid.isEmpty()) continue;

                QList<ConnectorItem *> connectorItems = treatAsCircle.values(pid.toLong());
                if (connectorItems.count() == 0) break;

                foreach (ConnectorItem * candidate, connectorItems) {
                    ItemBase * itemBase = candidate->attachedTo();
                    SvgIdLayer * svgIdLayer = candidate->connector()->fullPinInfo(itemBase->viewID(), itemBase->viewLayerID());
                    if (svgIdLayer->m_svgId == id) {
                        connectorItem = candidate;
                        break;
                    }
                }

                if (connectorItem) break;
            }
            if (connectorItem == NULL) continue;

            //QString string;
            //QTextStream stream(&string);
            //path.save(stream, 0);
            //DebugDialog::debug("path " + string);

            connectorItem->debugInfo("make path");
            path.setAttribute("id", unique);
            QSvgRenderer renderer;
            renderer.load(root1.ownerDocument().toByteArray());
            QRectF bounds = renderer.boundsOnElement(unique);
            path.removeAttribute("id");

            QDomElement circle = root1.ownerDocument().createElement("circle");
            path.parentNode().insertBefore(circle, path);
            circle.setAttribute("id", id);
            QPointF p = bounds.center();
            circle.setAttribute("cx", QString::number(p.x()));
            circle.setAttribute("cy", QString::number(p.y()));
            circle.setAttribute("r", QString::number(connectorItem->radius() * GraphicsUtils::StandardFritzingDPI / GraphicsUtils::SVGDPI));
            circle.setAttribute("stroke-width", QString::number(connectorItem->strokeWidth() * GraphicsUtils::StandardFritzingDPI / GraphicsUtils::SVGDPI));

        }
    }
}

QString GerberGenerator::renderTo(const LayerList & layers, ItemBase * board, PCBSketchWidget * sketchWidget, bool & empty) {
    RenderThing renderThing;
    renderThing.printerScale = GraphicsUtils::SVGDPI;
    renderThing.blackOnly = true;
    renderThing.dpi = GraphicsUtils::StandardFritzingDPI;
    renderThing.hideTerminalPoints = true;
    renderThing.selectedItems = renderThing.renderBlocker = false;
	QString svg = sketchWidget->renderToSVG(renderThing, board, layers);
    empty = renderThing.empty;
    return svg;
}
