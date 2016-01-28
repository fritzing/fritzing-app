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

$Revision: 6988 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-26 12:05:48 +0200 (Fr, 26. Apr 2013) $

********************************************************************/

#include "fsvgrenderer.h"
#include "debugdialog.h"
#include "svg/svgfilesplitter.h"
#include "utils/textutils.h"
#include "utils/graphicsutils.h"
#include "utils/folderutils.h"
#include "connectors/svgidlayer.h"

#include <QRegExp>
#include <QTextStream>
#include <QPainter>
#include <QCoreApplication>
#include <QGraphicsSvgItem>
#include <qnumeric.h>

/////////////////////////////////////////////

QString FSvgRenderer::NonConnectorName("nonconn");

static ConnectorInfo VanillaConnectorInfo;

FSvgRenderer::FSvgRenderer(QObject * parent) : QSvgRenderer(parent)
{
	m_defaultSizeF = QSizeF(0,0);
}

FSvgRenderer::~FSvgRenderer()
{
	clearConnectorInfoHash(m_connectorInfoHash);
	clearConnectorInfoHash(m_nonConnectorInfoHash);
}

void FSvgRenderer::initNames() {
    VanillaConnectorInfo.gotPath = VanillaConnectorInfo.gotCircle = false;	
}

void FSvgRenderer::clearConnectorInfoHash(QHash<QString, ConnectorInfo *> & hash) {
	foreach (ConnectorInfo * connectorInfo, hash.values()) {
		delete connectorInfo;
	}
	hash.clear();
}

void FSvgRenderer::cleanup() {

}

QByteArray FSvgRenderer::loadSvg(const QString & filename) {
    LoadInfo loadInfo;
    loadInfo.filename = filename;
	return loadSvg(loadInfo);
}

QByteArray FSvgRenderer::loadSvg(const LoadInfo & loadInfo) {
	if (!QFileInfo(loadInfo.filename).exists() || !QFileInfo(loadInfo.filename).isFile()) {
		return QByteArray();
	}

    QFile file(loadInfo.filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
		return QByteArray();
	}

	QByteArray contents = file.readAll();
	file.close();

	if (contents.length() <= 0) return QByteArray();

	return loadAux(contents, loadInfo);

}

bool FSvgRenderer::loadSvgString(const QString & svg) {
	QByteArray byteArray(svg.toUtf8());
	QByteArray result = loadSvg(byteArray, "", true);
    return (!result.isEmpty());
}

bool FSvgRenderer::loadSvgString(const QString & svg, QString & newSvg) {
	QByteArray byteArray(svg.toUtf8());
	QByteArray result = loadSvg(byteArray, "", true);
    newSvg = QString(result);
    return (!result.isEmpty());
}

QByteArray FSvgRenderer::loadSvg(const QByteArray & contents, const QString & filename, bool findNonConnectors) {
    LoadInfo loadInfo;
    loadInfo.filename = filename;
    loadInfo.findNonConnectors = findNonConnectors;
	return loadAux(contents, loadInfo);
}

QByteArray FSvgRenderer::loadSvg(const QByteArray & contents, const LoadInfo & loadInfo) {
	return loadAux(contents, loadInfo);
}

QByteArray FSvgRenderer::loadAux(const QByteArray & theContents, const LoadInfo & loadInfo) 
{
	QByteArray cleanContents(theContents);
	bool cleaned = false;

    QString string(cleanContents);
    if (TextUtils::fixMuch(string, false)) {
		cleaned = true;
	}
	if (TextUtils::fixPixelDimensionsIn(string)) {
        cleaned = true;
    }
    if (cleaned) {
        cleanContents = string.toUtf8();
    }

	if (loadInfo.connectorIDs.count() > 0 || !loadInfo.setColor.isEmpty() || loadInfo.findNonConnectors) {
		QString errorStr;
		int errorLine;
		int errorColumn;
		QDomDocument doc;
		if (!doc.setContent(cleanContents, &errorStr, &errorLine, &errorColumn)) {
			DebugDialog::debug(QString("renderer loadAux failed %1 %2 %3 %4").arg(loadInfo.filename).arg(errorStr).arg(errorLine).arg(errorColumn));
		}

		bool resetContents = false;

		QDomElement root = doc.documentElement();
		if (!loadInfo.setColor.isEmpty()) {
			QDomElement element = TextUtils::findElementWithAttribute(root, "id", loadInfo.colorElementID);
			if (!element.isNull()) {
				QStringList exceptions;
				exceptions << "black" << "#000000";
				SvgFileSplitter::fixColorRecurse(element, loadInfo.setColor, exceptions);
				resetContents = true;
			}
		}
		if (loadInfo.connectorIDs.count() > 0) {
			bool init =  initConnectorInfo(doc, loadInfo);
			resetContents = resetContents || init;
		}
		if (loadInfo.findNonConnectors) {
			initNonConnectorInfo(doc, loadInfo.filename);
		}

		if (resetContents) {
			cleanContents = TextUtils::removeXMLEntities(doc.toString()).toUtf8();
		}
	}


	/*
	cleanContents = doc.toByteArray();

	//QFile file("all.txt");
	//if (file.open(QIODevice::Append)) {
		//QTextStream t(&file);
		//t << cleanContents;
		//file.close();
	//}

	*/

	//DebugDialog::debug(cleanContents.data());

    return finalLoad(cleanContents, loadInfo.filename);
}

QByteArray FSvgRenderer::finalLoad(QByteArray & cleanContents, const QString & filename) {

	QXmlStreamReader xml(cleanContents);
	bool result = determineDefaultSize(xml);
	if (!result) {
		return QByteArray();
	}

	result = QSvgRenderer::load(cleanContents);
	if (result) {
		m_filename = filename;
		return cleanContents;
	}

	return QByteArray();
}

bool FSvgRenderer::fastLoad(const QByteArray & contents) {
	return QSvgRenderer::load(contents);
}

const QString & FSvgRenderer::filename() {
	return m_filename;
}

QPixmap * FSvgRenderer::getPixmap(QSvgRenderer * renderer, QSize size) 
{
	QPixmap *pixmap = new QPixmap(size);
	pixmap->fill(Qt::transparent);
	QPainter painter(pixmap);
	// preserve aspect ratio
	QSizeF def = renderer->defaultSize();
    FSvgRenderer * frenderer = qobject_cast<FSvgRenderer *>(renderer);
    if (frenderer) {
        def = frenderer->defaultSizeF();
    }
	double newW = size.width();
	double newH = newW * def.height() / def.width();
	if (newH > size.height()) {
		newH = size.height();
		newW = newH * def.width() / def.height();
	}
	QRectF bounds((size.width() - newW) / 2.0, (size.height() - newH) / 2.0, newW, newH);
	renderer->render(&painter, bounds);
	painter.end();

    return pixmap;
}

bool FSvgRenderer::determineDefaultSize(QXmlStreamReader & xml)
{
	QSizeF size = parseForWidthAndHeight(xml);

	m_defaultSizeF = QSizeF(size.width() * GraphicsUtils::SVGDPI, size.height() * GraphicsUtils::SVGDPI);
	return (size.width() != 0 && size.height() != 0);
}

QSizeF FSvgRenderer::parseForWidthAndHeight(QXmlStreamReader & xml)
{
    QSizeF size = TextUtils::parseForWidthAndHeight(xml);
    if (size.width() != 0 && size.height() != 0) return size;

	QIODevice * device = xml.device();
	DebugDialog::debug("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	DebugDialog::debug("bad width and/or bad height in svg:");
	if (device) {
		device->reset();
		QString string(device->readAll());
		DebugDialog::debug(string);
	}
	DebugDialog::debug("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

	return size;
}

QSizeF FSvgRenderer::defaultSizeF() {
	if (m_defaultSizeF.width() == 0 && m_defaultSizeF.height() == 0) {
		return defaultSize();
	}

	return m_defaultSizeF;
}


void FSvgRenderer::initNonConnectorInfo(QDomDocument & domDocument, const QString & filename)
{
	clearConnectorInfoHash(m_nonConnectorInfoHash);
	QDomElement root = domDocument.documentElement();
	initNonConnectorInfoAux(root, filename);
}

void FSvgRenderer::initNonConnectorInfoAux(QDomElement & element, const QString & filename)
{
	QString id = element.attribute("id");
	if (id.startsWith(NonConnectorName, Qt::CaseInsensitive)) {
		ConnectorInfo * connectorInfo = initConnectorInfoStruct(element, filename, false);
		m_nonConnectorInfoHash.insert(id, connectorInfo);
	}
	QDomElement child = element.firstChildElement();
	while (!child.isNull()) {
		initNonConnectorInfoAux(child, filename);
		child = child.nextSiblingElement();
	}
}

bool FSvgRenderer::initConnectorInfo(QDomDocument & domDocument, const LoadInfo & loadInfo)
{
	bool result = false;
	clearConnectorInfoHash(m_connectorInfoHash);
	QDomElement root = domDocument.documentElement();
	initConnectorInfoAux(root, loadInfo);
	if (loadInfo.terminalIDs.count() > 0) {
		initTerminalInfoAux(root, loadInfo);
	}
	if (loadInfo.legIDs.count() > 0) {
		initLegInfoAux(root, loadInfo, result);
	}

	return result;
}

void FSvgRenderer::initLegInfoAux(QDomElement & element, const LoadInfo & loadInfo, bool & gotOne)
{
	QString id = element.attribute("id");
	if (!id.isEmpty()) {
		int ix = loadInfo.legIDs.indexOf(id);
		if (ix >= 0) {
			//DebugDialog::debug("init leg info " + id);
			//foreach (QString lid, legIDs) {
			//	DebugDialog::debug("\tleg id:" + lid);
			//}

			element.setTagName("g");			// don't want this element to actually be drawn
			gotOne = true;
			ConnectorInfo * connectorInfo = m_connectorInfoHash.value(loadInfo.connectorIDs.at(ix), NULL);
			if (connectorInfo) {
				//QString temp;
				//QTextStream stream(&temp);
				//element.save(stream, 0);
				//DebugDialog::debug("\t matched " + connectorIDs.at(ix) + " " + temp);
				connectorInfo->legMatrix = TextUtils::elementToMatrix(element);
				connectorInfo->legColor = element.attribute("stroke");
				connectorInfo->legLine = QLineF();
				connectorInfo->legStrokeWidth = 0;
				initLegInfoAux(element, connectorInfo);
			}
			// don't return here, might miss other legs
		}
	}

	QDomElement child = element.firstChildElement();
	while (!child.isNull()) {
		initLegInfoAux(child, loadInfo, gotOne);
		child = child.nextSiblingElement();
	}
}

bool FSvgRenderer::initLegInfoAux(QDomElement & element, ConnectorInfo * connectorInfo) 
{
	bool ok;
	double sw = element.attribute("stroke-width").toDouble(&ok);
	if (!ok) return false;

	double x1 = element.attribute("x1").toDouble(&ok);
	if (!ok) return false;

	double y1 = element.attribute("y1").toDouble(&ok);
	if (!ok) return false;

	double x2 = element.attribute("x2").toDouble(&ok);
	if (!ok) return false;

	double y2 = element.attribute("y2").toDouble(&ok);
	if (!ok) return false;

	connectorInfo->legStrokeWidth = sw;
	connectorInfo->legLine = QLineF(x1, y1, x2, y2);
	return true;
}

void FSvgRenderer::initTerminalInfoAux(QDomElement & element, const LoadInfo & loadInfo)
{
	QString id = element.attribute("id");
	if (!id.isEmpty()) {
		int ix = loadInfo.terminalIDs.indexOf(id);
		if (ix >= 0) {
			ConnectorInfo * connectorInfo = m_connectorInfoHash.value(loadInfo.connectorIDs.at(ix), NULL);
			if (connectorInfo) {
				connectorInfo->terminalMatrix = TextUtils::elementToMatrix(element);
			}
			// don't return here, might miss other terminal ids
		}
	}

	QDomElement child = element.firstChildElement();
	while (!child.isNull()) {
		initTerminalInfoAux(child, loadInfo);
		child = child.nextSiblingElement();
	}
}

void FSvgRenderer::initConnectorInfoAux(QDomElement & element, const LoadInfo & loadInfo)
{
	QString id = element.attribute("id");
	if (!id.isEmpty()) {
		if (loadInfo.connectorIDs.contains(id)) {
			ConnectorInfo * connectorInfo = initConnectorInfoStruct(element, loadInfo.filename, loadInfo.parsePaths);
			m_connectorInfoHash.insert(id, connectorInfo);
		}
		// don't return here, might miss other connectors
	}

	QDomElement child = element.firstChildElement();
	while (!child.isNull()) {
		initConnectorInfoAux(child, loadInfo);
		child = child.nextSiblingElement();
	}
}

ConnectorInfo * FSvgRenderer::initConnectorInfoStruct(QDomElement & connectorElement, const QString & filename, bool parsePaths) {
	ConnectorInfo * connectorInfo = new ConnectorInfo();
	connectorInfo->radius = connectorInfo->strokeWidth = 0;
	connectorInfo->gotPath = connectorInfo->gotCircle = false;

	if (connectorElement.isNull()) return connectorInfo;

	connectorInfo->matrix = TextUtils::elementToMatrix(connectorElement);
	initConnectorInfoStructAux(connectorElement, connectorInfo, filename, parsePaths);
	return connectorInfo;
}

bool FSvgRenderer::initConnectorInfoStructAux(QDomElement & element, ConnectorInfo * connectorInfo, const QString & filename, bool parsePaths) 
{
    if (element.nodeName().compare("circle") == 0) {
        return initConnectorInfoCircle(element, connectorInfo, filename);
    }

    if (element.nodeName().compare("path") == 0) {
        if (!parsePaths) return false;
        return initConnectorInfoPath(element, connectorInfo, filename);
    }

	QDomElement child = element.firstChildElement();
	while (!child.isNull()) {
		if (initConnectorInfoStructAux(child, connectorInfo, filename, parsePaths)) return true;

		child = child.nextSiblingElement();
	}

    return false;
}

bool FSvgRenderer::initConnectorInfoPath(QDomElement & element, ConnectorInfo * connectorInfo, const QString & filename) 
{
    QString id = element.attribute("id");
    if (id.isEmpty()) return false;         // shouldn't be here

    QString stroke = element.attribute("stroke");
    if (stroke == "none") return false;     // cannot be a circle with a hole in the center

    connectorInfo->gotPath = true;
    double sw = TextUtils::getStrokeWidth(element, 1);

    if (!stroke.isEmpty()) element.setAttribute("stroke", "black");

    QString fill = element.attribute("fill");
    if (!fill.isEmpty() && (fill != "none")) element.setAttribute("fill", "black");

    QDomDocument doc = element.ownerDocument();
    FSvgRenderer renderer;
    QByteArray byteArray = doc.toByteArray();
    renderer.finalLoad(byteArray, filename);
    QRectF bounds = renderer.boundsOnElement(id);	

    static const int dim = 101;
    int width = dim;
    int height = dim;
    if (bounds.width() - bounds.height() / (bounds.width() + bounds.height()) > .01) {
        height = (int) (float(dim) * bounds.height() / bounds.width());
    }
    else if (bounds.height() - bounds.width() / (bounds.width() + bounds.height()) > .01) {
        width = (int) (float(dim) * bounds.width() / bounds.height());
    }

    QImage image(width, height, QImage::Format_Mono);
    image.fill(0xffffffff);
    QPainter painter;
    painter.begin(&image);
    renderer.render(&painter, id);
    painter.end();

#ifndef QT_NO_DEBUG
    //image.save(FolderUtils::getUserDataStorePath("") + "/donutcheck.png");
#endif

    if (!fill.isEmpty()) element.setAttribute("fill", fill);
    if (!stroke.isEmpty()) element.setAttribute("stroke", stroke);

    //DebugDialog::debug(QString("checking connector path %1").arg(id));

    int lxStart = -1, lxEnd = -1, rxStart = -1, rxEnd = -1;
    for (int x = 0; x < width; x++) {
        if (image.pixel(x, height / 2) == 0xff000000) {
            lxStart = x;
            break;
        }
    }
    if (lxStart < 0) return false;
    if (lxStart >= width / 2) return false;        // not an ellipse
    for (int x = lxStart + 1; x < width; x++) {
        if (image.pixel(x, height / 2) == 0xff000000) {
            lxEnd = x;
        }
        else break;
    }
    if (lxEnd < 0) return false;         
    if (lxEnd > width / 2) return false;       // not an ellipse

    for (int x = lxEnd + 1; x < width; x++) {
        if (image.pixel(x, height / 2) == 0xff000000) {
            rxStart = x;
            break;
        }
    }
    if (rxStart < 0) return false;
    if (rxStart < height / 2) return false;        // not an ellipse;
    for (int x = rxStart + 1; x < width; x++) {
        if (image.pixel(x, height / 2) == 0xff000000) {
            rxEnd = x;
        }
        else break;
    }
    if (rxEnd < 0) return false; 
    if (qAbs(rxEnd - rxStart - (lxEnd - lxStart)) > 1) return false;   // sides are not symmetric


    int lyStart = -1, lyEnd = -1, ryStart = -1, ryEnd = -1;
    for (int y = 0; y < height; y++) {
        if (image.pixel(width / 2, y) == 0xff000000) {
            lyStart = y;
            break;
        }
    }
    if (lyStart < 0) return false;
    if (lyStart >= height / 2) return false;        // not an ellipse
    for (int y = lyStart + 1; y < height; y++) {
        if (image.pixel(width / 2, y) == 0xff000000) {
            lyEnd = y;
        }
        else break;
    }
    if (lyEnd < 0) return false;         
    if (lyEnd > height / 2) return false;       // not an ellipse

    for (int y = lyEnd + 1; y < height; y++) {
        if (image.pixel(width / 2, y) == 0xff000000) {
            ryStart = y;
            break;
        }
    }
    if (ryStart < 0) return false;
    if (ryStart < height / 2) return false;        // not an ellipse
    for (int y = ryStart + 1; y < height; y++) {
        if (image.pixel(width / 2, y) == 0xff000000) {
            ryEnd = y;
        }
        else break;
    }
    if (ryEnd < 0) return false;         
    if (qAbs((ryEnd - ryStart) - (lyEnd - lyStart)) > 1) return false;  // tops not symmetric

    if (qAbs((rxStart - lxEnd) - (ryStart - lyEnd)) > 1) return false;  // inner drill hole is not circular

    double r = (qMin(bounds.width(), bounds.height()) -  sw) / 2;

	QMatrix matrix = TextUtils::elementToMatrix(element);
	if (!matrix.isIdentity()) {
		QRectF r1(0,0,r,r);
		QRectF r2 = matrix.mapRect(r1);
		if (r2.width() != r1.width()) {
			r = r2.width();
			sw = sw * r2.width() / r1.width();
		}
	}

	connectorInfo->radius = r;
	connectorInfo->strokeWidth = sw;
    connectorInfo->gotCircle = true;

    //DebugDialog::debug(QString("got connector path %1").arg(id));

    return true;
}

bool FSvgRenderer::initConnectorInfoCircle(QDomElement & element, ConnectorInfo * connectorInfo, const QString & filename) 
{
    Q_UNUSED(filename);

	bool ok;
	element.attribute("cx").toDouble(&ok);
	if (!ok) return false;

	element.attribute("cy").toDouble(&ok);
	if (!ok) return false;

	double r = element.attribute("r").toDouble(&ok);
	if (!ok) return false;

    double sw = TextUtils::getStrokeWidth(element, 1);

	QMatrix matrix = TextUtils::elementToMatrix(element);
	if (!matrix.isIdentity()) {
		QRectF r1(0,0,r,r);
		QRectF r2 = matrix.mapRect(r1);
		if (r2.width() != r1.width()) {
			r = r2.width();
			sw = sw * r2.width() / r1.width();
		}
	}

	//DebugDialog::debug("got a circle");
	connectorInfo->gotCircle = true;
	//connectorInfo->cbounds.setRect(cx - r - (sw / 2.0), cy - r - (sw / 2.0), (r * 2) + sw, (r * 2) + sw);
	connectorInfo->radius = r;
	connectorInfo->strokeWidth = sw;
	return true;
}

ConnectorInfo * FSvgRenderer::getConnectorInfo(const QString & connectorID) {
	return m_connectorInfoHash.value(connectorID, &VanillaConnectorInfo);
}

bool FSvgRenderer::setUpConnector(SvgIdLayer * svgIdLayer, bool ignoreTerminalPoint, ViewLayer::ViewLayerPlacement viewLayerPlacement) {

	if (svgIdLayer == NULL) return false;

    //if (svgIdLayer->m_viewID = ViewLayer::SchematicView) {
    //    DebugDialog::debug("delete me please");
    //}

	if (svgIdLayer->processed(viewLayerPlacement)) {
		// hybrids are not visible in some views
		return svgIdLayer->svgVisible(viewLayerPlacement) || svgIdLayer->m_hybrid;
	}

	QString connectorID = svgIdLayer->m_svgId;

	// boundsOnElement seems to include any matrix on the element itself.
	// I would swear this wasn't true before Qt4.7, but maybe I am crazy
	QRectF bounds = this->boundsOnElement(connectorID);	

	if (bounds.isNull() && !svgIdLayer->m_hybrid) {		// hybrids can have zero size
		svgIdLayer->setInvisible(viewLayerPlacement);		
		DebugDialog::debug("renderer::setupconnector: null bounds");
		return false;
	}

	QSizeF defaultSizeF = this->defaultSizeF();
	QRectF viewBox = this->viewBoxF();

	ConnectorInfo * connectorInfo = getConnectorInfo(connectorID);	

	/*
	DebugDialog::debug(QString("connectorid:%1 m11:%2 m12:%3 m21:%4 m22:%5 dx:%6 dy:%7")
						.arg(connectorID)
						.arg(connectorInfo->matrix.m11())
						.arg(connectorInfo->matrix.m12())
						.arg(connectorInfo->matrix.m21())
						.arg(connectorInfo->matrix.m22())
						.arg(connectorInfo->matrix.dx())
						.arg(connectorInfo->matrix.dy()),
				bounds);
	*/

	
	/*DebugDialog::debug(QString("identity matrix %11 %1 %2, viewbox: %3 %4 %5 %6, bounds: %7 %8 %9 %10, size: %12 %13").arg(m_modelPart->title()).arg(connectorSharedID())
					   .arg(viewBox.x()).arg(viewBox.y()).arg(viewBox.width()).arg(viewBox.height())
					   .arg(bounds.x()).arg(bounds.y()).arg(bounds.width()).arg(bounds.height())
					   .arg(viewID)
					   .arg(defaultSizeF.width()).arg(defaultSizeF.height())
	);
	*/

	// some strangeness in the way that svg items and non-svg items map to screen space
	// might be a qt problem.
	//QMatrix matrix0 = connectorInfo->matrix * this->matrixForElement(connectorID);  
	//QRectF r1 = matrix0.mapRect(bounds);

    QMatrix elementMatrix = this->matrixForElement(connectorID);
	QRectF r1 = elementMatrix.mapRect(bounds);

	if (connectorInfo != NULL) {
        if (connectorInfo->gotCircle) {
            QLineF l(0,0,connectorInfo->radius, 0);
            QLineF lm = elementMatrix.map(l);
		    svgIdLayer->m_radius = lm.length() * defaultSizeF.width() / viewBox.width();

            QLineF k(0,0,connectorInfo->strokeWidth, 0);
            QLineF km = elementMatrix.map(k);
		    svgIdLayer->m_strokeWidth = km.length() * defaultSizeF.width() / viewBox.width();
		    //bounds = connectorInfo->cbounds;
        }
        if (connectorInfo->gotPath) {
            svgIdLayer->m_path = true;
        }
	}


	/*
	svgIdLayer->m_rect.setRect(r1.x() * defaultSize.width() / viewBox.width(), 
							   r1.y() * defaultSize.height() / viewBox.height(), 
							   r1.width() * defaultSize.width() / viewBox.width(), 
							   r1.height() * defaultSize.height() / viewBox.height());
	*/

    QRectF svgRect(r1.x() * defaultSizeF.width() / viewBox.width(), 
							   r1.y() * defaultSizeF.height() / viewBox.height(), 
							   r1.width() * defaultSizeF.width() / viewBox.width(), 
							   r1.height() * defaultSizeF.height() / viewBox.height());

	//if (!svgIdLayer->m_svgVisible) {
		//DebugDialog::debug("not vis");
	//}
	QPointF terminal = calcTerminalPoint(svgIdLayer->m_terminalId, svgRect, ignoreTerminalPoint, viewBox, connectorInfo->terminalMatrix);

    svgIdLayer->setPointRect(viewLayerPlacement, terminal, svgRect, !bounds.isNull());
	calcLeg(svgIdLayer, viewBox, connectorInfo);
	
	return true;
}

void FSvgRenderer::calcLeg(SvgIdLayer * svgIdLayer, const QRectF & viewBox, ConnectorInfo * connectorInfo)
{
	if (svgIdLayer->m_legId.isEmpty()) return;

	svgIdLayer->m_legColor = connectorInfo->legColor;

	QSizeF defaultSizeF = this->defaultSizeF();
	svgIdLayer->m_legStrokeWidth = connectorInfo->legStrokeWidth * defaultSizeF.width() / viewBox.width();

	/*
	DebugDialog::debug(	QString("calcleg leg %1 %2 %3 %4")
		.arg(connectorInfo->legLine.p1().x())
		.arg(connectorInfo->legLine.p1().y())
		.arg(connectorInfo->legLine.p2().x())
		.arg(connectorInfo->legLine.p2().y())
		);
	*/

	QMatrix matrix = this->matrixForElement(svgIdLayer->m_legId) * connectorInfo->legMatrix;
	QPointF p1 = matrix.map(connectorInfo->legLine.p1());
	QPointF p2 = matrix.map(connectorInfo->legLine.p2());

	double x1 = p1.x() * defaultSizeF.width() / viewBox.width();
	double y1 = p1.y() * defaultSizeF.height() / viewBox.height();
	double x2 = p2.x() * defaultSizeF.width() / viewBox.width();
	double y2 = p2.y() * defaultSizeF.height() / viewBox.height();
	QPointF center(defaultSizeF.width() / 2, defaultSizeF.height() / 2);
	double d1 = GraphicsUtils::distanceSqd(QPointF(x1, y1), center);
	double d2 = GraphicsUtils::distanceSqd(QPointF(x2, y2), center);

	// find the end which is closer to the center of the viewBox (which shouldn't include the leg)
	if (d1 <= d2) {
		svgIdLayer->m_legLine = QLineF(x1, y1, x2, y2);
	}
	else {
		svgIdLayer->m_legLine = QLineF(x2, y2, x1, y1);
	}
}

QPointF FSvgRenderer::calcTerminalPoint(const QString & terminalId, const QRectF & connectorRect, bool ignoreTerminalPoint, const QRectF & viewBox, QMatrix & terminalMatrix)
{
	Q_UNUSED(terminalMatrix);
	QPointF terminalPoint = connectorRect.center() - connectorRect.topLeft();    // default spot is centered
	if (ignoreTerminalPoint) {
		return terminalPoint;
	}
	if (terminalId.isNull() || terminalId.isEmpty()) {
		return terminalPoint;
	}

    if (!this->elementExists(terminalId)) {
        DebugDialog::debug(QString("missing expected terminal point element %1").arg(terminalId));
        return terminalPoint;
    }

	QRectF tBounds = this->boundsOnElement(terminalId);
	if (tBounds.isNull()) {
		return terminalPoint;
	}

	QSizeF defaultSizeF = this->defaultSizeF();
	//DebugDialog::debug(	QString("terminal %5 rect %1,%2,%3,%4").arg(tBounds.x()).
										//arg(tBounds.y()).
										//arg(tBounds.width()).
										//arg(tBounds.height()).
										//arg(terminalID) );


	// matrixForElement only grabs parent matrices, not any transforms in the element itself
	//QMatrix tMatrix = this->matrixForElement(terminalId) ; //* terminalMatrix;
	//QRectF terminalRect = tMatrix.mapRect(tBounds);
	QRectF terminalRect = this->matrixForElement(terminalId).mapRect(tBounds);
	QPointF c = terminalRect.center();
	QPointF q(c.x() * defaultSizeF.width() / viewBox.width(), c.y() * defaultSizeF.height() / viewBox.height());
	terminalPoint = q - connectorRect.topLeft();
	//DebugDialog::debug(	QString("terminalagain %3 rect %1,%2 ").arg(terminalPoint.x()).
										//arg(terminalPoint.y()).
										//arg(terminalID) );

	return terminalPoint;
}

QList<SvgIdLayer *> FSvgRenderer::setUpNonConnectors(ViewLayer::ViewLayerPlacement viewLayerPlacement) {

	QList<SvgIdLayer *> list;
	if (m_nonConnectorInfoHash.count() == 0) return list;

	foreach (QString nonConnectorID, m_nonConnectorInfoHash.keys()) {
		SvgIdLayer * svgIdLayer = new SvgIdLayer(ViewLayer::PCBView);
		svgIdLayer->m_svgId = nonConnectorID;
		QRectF bounds = this->boundsOnElement(nonConnectorID);
		if (bounds.isNull()) {
			delete svgIdLayer;
			continue;
		}

		QSizeF defaultSizeF = this->defaultSizeF();
		QSize defaultSize = this->defaultSize();
		if ((bounds.width()) == defaultSizeF.width() && (bounds.height()) == defaultSizeF.height()) {
			delete svgIdLayer;
			continue;
		}

		QRectF viewBox = this->viewBoxF();

		ConnectorInfo * connectorInfo = m_nonConnectorInfoHash.value(nonConnectorID, NULL);		
		if (connectorInfo && connectorInfo->gotCircle) {
			svgIdLayer->m_radius = connectorInfo->radius * defaultSizeF.width() / viewBox.width();
			svgIdLayer->m_strokeWidth = connectorInfo->strokeWidth * defaultSizeF.width() / viewBox.width();
			//bounds = connectorInfo->cbounds;
		}

		// matrixForElement only grabs parent matrices, not any transforms in the element itself
		//QMatrix matrix0 = connectorInfo->matrix * this->matrixForElement(nonConnectorID);  
		//QRectF r1 = matrix0.mapRect(bounds);
		QRectF r1 = this->matrixForElement(nonConnectorID).mapRect(bounds);
        QRectF svgRect(r1.x() * defaultSize.width() / viewBox.width(), r1.y() * defaultSize.height() / viewBox.height(), r1.width() * defaultSize.width() / viewBox.width(), r1.height() * defaultSize.height() / viewBox.height());
        QPointF center = svgRect.center() - svgRect.topLeft();
        svgIdLayer->setPointRect(viewLayerPlacement, center, svgRect, true);
		list.append(svgIdLayer);
	}

	return list;

}


