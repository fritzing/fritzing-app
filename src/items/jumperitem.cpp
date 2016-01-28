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

$Revision: 6998 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-28 13:51:10 +0200 (So, 28. Apr 2013) $

********************************************************************/

#include "jumperitem.h"
#include "../connectors/connectoritem.h"
#include "../fsvgrenderer.h"
#include "../model/modelpart.h"
#include "../utils/graphicsutils.h"
#include "../svg/svgfilesplitter.h"
#include "../sketch/infographicsview.h"
#include "../debugdialog.h"
#include "../layerattributes.h"

static QString Copper0LayerTemplate = "";
static QString JumperWireLayerTemplate = "";
static const QString ShadowColor ="#1b5bb3";
static const QString JumperColor = "#418dd9";

static QHash<ViewLayer::ViewLayerID, QString> Colors;

QString makeWireImage(double w, double h, double r0x, double r0y, double r1x, double r1y, const QString & layerName, const QString & color, double thickness) {
	return JumperWireLayerTemplate
				.arg(w).arg(h)
				.arg(w * 1000).arg(h * 1000)			
				.arg(r0x).arg(r0y).arg(r1x).arg(r1y)
				.arg(layerName)
				.arg(color)
				.arg(thickness);
}


void shorten(QRectF r0, QPointF r0c, QPointF r1c, double & r0x, double & r0y, double & r1x, double & r1y) 
{
	double radius = r0.width() / 2.0;
	GraphicsUtils::shortenLine(r0c, r1c, radius, radius);
	r0x = GraphicsUtils::pixels2mils(r0c.x(), GraphicsUtils::SVGDPI);
	r0y = GraphicsUtils::pixels2mils(r0c.y(), GraphicsUtils::SVGDPI);
	r1x = GraphicsUtils::pixels2mils(r1c.x(), GraphicsUtils::SVGDPI);
	r1y = GraphicsUtils::pixels2mils(r1c.y(), GraphicsUtils::SVGDPI);
}


// TODO: 
//	ignore during autoroute?
//	ignore during other connections?
//	don't let footprints overlap during dragging

/////////////////////////////////////////////////////////

JumperItem::JumperItem( ModelPart * modelPart, ViewLayer::ViewID viewID,  const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel) 
	: PaletteItem(modelPart, viewID,  viewGeometry,  id, itemMenu, doLabel)
{
    m_originalClickItem = NULL;
    if (Colors.isEmpty()) {
		Colors.insert(ViewLayer::Copper0, ViewLayer::Copper0Color);
		Colors.insert(ViewLayer::Copper1, ViewLayer::Copper1Color);
		Colors.insert(ViewLayer::PartImage, JumperColor);
		Colors.insert(ViewLayer::Silkscreen1, ViewLayer::Silkscreen1Color);
	}

	m_otherItem = m_connector0 = m_connector1 = m_dragItem = NULL;
	if (Copper0LayerTemplate.isEmpty()) {
		QFile file(":/resources/templates/jumper_copper0LayerTemplate.txt");
		if (file.open(QFile::ReadOnly)) {
			Copper0LayerTemplate = file.readAll();
			file.close();
		}
	}
	if (JumperWireLayerTemplate.isEmpty()) {
		QFile file(":/resources/templates/jumper_jumperwiresLayerTemplate.txt");
		if (file.open(QFile::ReadOnly)) {
			JumperWireLayerTemplate = file.readAll();
			file.close();
		}
	}
}

JumperItem::~JumperItem() {
}

QRectF JumperItem::boundingRect() const
{
    if (m_viewID != ViewLayer::PCBView) {
        return PaletteItem::boundingRect();
    }

	return shape().controlPointRect();
}

void JumperItem::paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (m_viewID != ViewLayer::PCBView) {
       PaletteItem::paintHover(painter, option, widget);
	   return;
    }

	ItemBase::paintHover(painter, option, widget, hoverShape());
}

void JumperItem::paintSelected(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (m_viewID != ViewLayer::PCBView) {
       PaletteItem::paintSelected(painter, option, widget);
	   return;
    }

	GraphicsUtils::qt_graphicsItem_highlightSelected(painter, option, boundingRect(), hoverShape());
}

QPainterPath JumperItem::hoverShape() const
{
    if (m_viewID != ViewLayer::PCBView) {
        return PaletteItem::hoverShape();
     }

	QPainterPath path; 
    QPointF c0 = m_connector0->rect().center();
    QPointF c1 = m_connector1->rect().center();
    path.moveTo(c0);
    path.lineTo(c1);

	QRectF rect = m_connector0->rect();
	double dx = m_connectorTL.x();
	double dy = m_connectorTL.y();
	rect.adjust(-dx, -dy, dx, dy);

	QPen pen;
	pen.setCapStyle(Qt::RoundCap);
	return GraphicsUtils::shapeFromPath(path, pen, rect.width(), false);
}

QPainterPath JumperItem::shape() const
{
	return hoverShape();
}

bool JumperItem::setUpImage(ModelPart * modelPart, const LayerHash & viewLayers, LayerAttributes & layerAttributes)
{
	bool result = PaletteItem::setUpImage(modelPart, viewLayers, layerAttributes);

	if (layerAttributes.doConnectors) {
		foreach (ConnectorItem * item, cachedConnectorItems()) {
			item->setCircular(true);
			if (item->connectorSharedName().contains('0')) {
				m_connector0 = item;
			}
			else if (item->connectorSharedName().contains('1')) {
				m_connector1 = item;
			}
		}

        m_connectorTL = m_connector0->rect().topLeft();			
		m_connectorBR = boundingRect().bottomRight() - m_connector1->rect().bottomRight();

		initialResize(layerAttributes.viewID);
	}

	return result;
}

void JumperItem::initialResize(ViewLayer::ViewID viewID) {
	if (viewID != ViewLayer::PCBView) return;

	bool ok;
	double r0x = m_modelPart->localProp("r0x").toDouble(&ok);
	if (!ok) return;

	double r0y = m_modelPart->localProp("r0y").toDouble(&ok);
	if (!ok) return;
					
	double r1x = m_modelPart->localProp("r1x").toDouble(&ok);
	if (!ok) return;
						
	double r1y = m_modelPart->localProp("r1y").toDouble(&ok);
	if (!ok) return;
							
	resizeAux(GraphicsUtils::mils2pixels(r0x, GraphicsUtils::SVGDPI), 
				GraphicsUtils::mils2pixels(r0y, GraphicsUtils::SVGDPI),
				GraphicsUtils::mils2pixels(r1x, GraphicsUtils::SVGDPI), 
				GraphicsUtils::mils2pixels(r1y, GraphicsUtils::SVGDPI));

}

bool JumperItem::mousePressEventK(PaletteItemBase * originalItem, QGraphicsSceneMouseEvent *event)
{
    m_originalClickItem = originalItem;
    mousePressEvent(event);
    return (m_dragItem != NULL);
}

void JumperItem::mouseMoveEventK(PaletteItemBase * originalItem, QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(originalItem);
    mouseMoveEvent(event);
}

void JumperItem::mouseReleaseEventK(PaletteItemBase * originalItem, QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(originalItem);
    mouseReleaseEvent(event);
}

void JumperItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    PaletteItemBase * originalItem = m_originalClickItem;
    m_originalClickItem = NULL;
	InfoGraphicsView *infographics = InfoGraphicsView::getInfoGraphicsView(this);
	if (infographics != NULL && infographics->spaceBarIsPressed()) { 
		event->ignore();
		return;
	}

	m_dragItem = NULL;
	QRectF rect = m_connector0->rect();
	double dx = m_connectorTL.x();
	double dy = m_connectorTL.y();
	rect.adjust(-dx, -dy, dx, dy);
	if (rect.contains(event->pos())) {
		m_dragItem = m_connector0;
		m_otherItem = m_connector1;
	}
	else {
		rect = m_connector1->rect();
		dx = m_connectorBR.x();
		dy = m_connectorBR.y();
		rect.adjust(-dx, -dy, dx, dy);
		if (rect.contains(event->pos())) {
			m_dragItem = m_connector1;
			m_otherItem = m_connector0;
		}
		else {
            if (originalItem) return ItemBase::mousePressEvent(event);
			return PaletteItem::mousePressEvent(event);
		}
	}

	m_dragStartScenePos = event->scenePos();
	m_dragStartThisPos = this->pos();
	m_dragStartConnectorPos = this->mapToScene(m_dragItem->rect().topLeft());
	m_dragStartCenterPos = this->mapToScene(m_dragItem->rect().center());
	m_otherPos = this->mapToScene(m_otherItem->rect().topLeft());
}

void JumperItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_dragItem == NULL) return;

	// TODO: make sure the two connectors don't overlap

	QPointF d = event->scenePos() - m_dragStartScenePos;
	QPointF p = m_dragStartConnectorPos + d;
	emit alignMe(this, p);
	QPointF myPos(qMin(p.x(), m_otherPos.x()) - m_connectorTL.x(), 
				  qMin(p.y(), m_otherPos.y()) - m_connectorTL.y());
	this->setPos(myPos);
	QRectF r = m_otherItem->rect();
	r.moveTo(mapFromScene(m_otherPos));
	m_otherItem->setRect(r);
	ConnectorItem * cross = m_otherItem->getCrossLayerConnectorItem();
	if (cross) cross->setRect(r);

	r = m_dragItem->rect();
	r.moveTo(mapFromScene(p));
	m_dragItem->setRect(r);

	cross = m_dragItem->getCrossLayerConnectorItem();
	if (cross) cross->setRect(r);

	resize();
    QList<ConnectorItem *> already;
	ItemBase::updateConnections(m_dragItem, true, already);	
}

void JumperItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	m_dragItem = NULL;
	PaletteItem::mouseReleaseEvent(event);
}


QString JumperItem::makeSvg(ViewLayer::ViewLayerID viewLayerID) 
{
	QRectF r0 = m_connector0->rect();
	QRectF r1 = m_connector1->rect();
	QRectF r = r0.united(r1);
	double w = GraphicsUtils::pixels2ins(r.width() + m_connectorTL.x() + m_connectorBR.x(), GraphicsUtils::SVGDPI);
	double h = GraphicsUtils::pixels2ins(r.height() + m_connectorTL.y() + m_connectorBR.y(), GraphicsUtils::SVGDPI);

	QPointF r0c = r0.center();
	QPointF r1c = r1.center();
	double r0x = GraphicsUtils::pixels2mils(r0c.x(), GraphicsUtils::SVGDPI);
	double r0y = GraphicsUtils::pixels2mils(r0c.y(), GraphicsUtils::SVGDPI);
	double r1x = GraphicsUtils::pixels2mils(r1c.x(), GraphicsUtils::SVGDPI);
	double r1y = GraphicsUtils::pixels2mils(r1c.y(), GraphicsUtils::SVGDPI);

	modelPart()->setLocalProp("r0x", r0x);
	modelPart()->setLocalProp("r0y", r0y);
	modelPart()->setLocalProp("r1x", r1x);
	modelPart()->setLocalProp("r1y", r1y);

	switch (viewLayerID) {
		case ViewLayer::Copper0:
		case ViewLayer::Copper1:
			return Copper0LayerTemplate
				.arg(w).arg(h)
				.arg(w * GraphicsUtils::StandardFritzingDPI).arg(h * GraphicsUtils::StandardFritzingDPI)			
				.arg(r0x).arg(r0y).arg(r1x).arg(r1y)
				.arg(ViewLayer::viewLayerXmlNameFromID(viewLayerID))
				.arg(Colors.value(viewLayerID));

		//case ViewLayer::Silkscreen0:
		case ViewLayer::Silkscreen1:
			shorten(r0, r0c, r1c, r0x, r0y, r1x, r1y);
			return makeWireImage(w, h, r0x, r0y, r1x, r1y, ViewLayer::viewLayerXmlNameFromID(viewLayerID), Colors.value(viewLayerID), 21);
			
		case ViewLayer::PartImage:
			{
				shorten(r0, r0c, r1c, r0x, r0y, r1x, r1y);
				QString svg = makeWireImage(w, h, r0x, r0y, r1x, r1y, ViewLayer::viewLayerXmlNameFromID(viewLayerID), ShadowColor, 40);
				QString svg2 = makeWireImage(w, h, r0x, r0y, r1x, r1y, "", Colors.value(viewLayerID), 20);
				int ix1 = svg.indexOf("<line");
				int ix2 = svg.indexOf(">", ix1);
				int ix3 = svg2.indexOf("<line");
				int ix4 = svg2.indexOf(">", ix3);
				svg.insert(ix2 + 1, svg2.mid(ix3, ix4 - ix3 + 1));
				return svg;
			}
			
        default:
			break;
	}

	return ___emptyString___;
}

void JumperItem::resize(QPointF p0, QPointF p1) {
	QPointF p = calcPos(p0, p1);
	resize(p, p0 - p, p1 - p);
}

void JumperItem::resize() {
	if (m_viewID != ViewLayer::PCBView) return;

	if (m_connector0 == NULL) return;
	if (m_connector1 == NULL) return;

	prepareGeometryChange();

	QString s = makeSvg(ViewLayer::Copper0);
	//DebugDialog::debug(s);
	resetRenderer(s);

	foreach (ItemBase * itemBase, m_layerKin) {
		switch(itemBase->viewLayerID()) {
			case ViewLayer::PartImage:
			case ViewLayer::Copper1:
			case ViewLayer::Silkscreen1:
			//case ViewLayer::Silkscreen0:
                {
					s = makeSvg(itemBase->viewLayerID());
					itemBase->resetRenderer(s);
                }
				break;
            default:
				break;
		}
	}

	//	DebugDialog::debug(QString("fast load result %1 %2").arg(result).arg(s));
}

void JumperItem::saveParams() {
	m_itemPos = pos();
	m_itemC0 = m_connector0->rect().center();
	m_itemC1 = m_connector1->rect().center();
}

void JumperItem::getParams(QPointF & p, QPointF & c0, QPointF & c1) {
	p = m_itemPos;
	c0 = m_itemC0;
	c1 = m_itemC1;
}

void JumperItem::resize(QPointF p, QPointF nc0, QPointF nc1) {
	resizeAux(nc0.x(), nc0.y(), nc1.x(), nc1.y());	

	DebugDialog::debug(QString("jumper item set pos %1 %2, %3").arg(this->id()).arg(p.x()).arg(p.y()) );
	setPos(p);
}

void JumperItem::resizeAux(double r0x, double r0y, double r1x, double r1y) {
	prepareGeometryChange();

	QRectF r0 = m_connector0->rect();
	QRectF r1 = m_connector1->rect();
	QPointF c0 = r0.center();
	QPointF c1 = r1.center();
	r0.translate(r0x - c0.x(), r0y - c0.y());
	r1.translate(r1x - c1.x(), r1y - c1.y());
	m_connector0->setRect(r0);
	m_connector1->setRect(r1);
	ConnectorItem * cc0 = m_connector0->getCrossLayerConnectorItem();
	if (cc0 != NULL) {
		cc0->setRect(r0);
	}
	ConnectorItem * cc1 = m_connector1->getCrossLayerConnectorItem();
	if (cc1 != NULL) {
		cc1->setRect(r1);
	}
	resize();
}

QSizeF JumperItem::footprintSize() {
	QRectF r0 = m_connector0->rect();
	return r0.size();
}

QString JumperItem::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor) 
{
	QString xml = "";
	switch (viewLayerID) {
		case ViewLayer::Copper0:
		case ViewLayer::Copper1:
		case ViewLayer::PartImage:
		//case ViewLayer::Silkscreen0:
		case ViewLayer::Silkscreen1:
			xml = makeSvg(viewLayerID);
        default:
			break;
	}

	if (!xml.isEmpty()) {
        return PaletteItemBase::normalizeSvg(xml, viewLayerID, blackOnly, dpi, factor);
	}

	return PaletteItemBase::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
}

void JumperItem::setAutoroutable(bool ar) {
	m_viewGeometry.setAutoroutable(ar);
}

bool JumperItem::getAutoroutable() {
	return m_viewGeometry.getAutoroutable();
}

ConnectorItem * JumperItem::connector0() {
	return m_connector0;
}

ConnectorItem * JumperItem::connector1() {
	return m_connector1;
}

bool JumperItem::hasCustomSVG() {
	switch (m_viewID) {
		case ViewLayer::PCBView:
			return true;
		default:
			return ItemBase::hasCustomSVG();
	}
}

bool JumperItem::inDrag() {
	return m_dragItem != NULL;
}

void JumperItem::loadLayerKin( const LayerHash & viewLayers, ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	PaletteItem::loadLayerKin(viewLayers, viewLayerPlacement);
	resize();
}

ItemBase::PluralType JumperItem::isPlural() {
	return Singular;
}

void JumperItem::addedToScene(bool temporary) {

	if (m_connector0 == NULL) return;
	if (m_connector1 == NULL) return;

	ConnectorItem * cc0 = m_connector0->getCrossLayerConnectorItem();
	if (cc0 != NULL) {
		cc0->setRect(m_connector0->rect());
	}
	ConnectorItem * cc1 = m_connector1->getCrossLayerConnectorItem();
	if (cc1 != NULL) {
		cc1->setRect(m_connector1->rect());
	}

	PaletteItem::addedToScene(temporary);
}

void JumperItem::rotateItem(double degrees, bool updateRatsnest) {
    Q_UNUSED(updateRatsnest);
	QPointF tc0, tc1;
	QTransform rotation;
	rotation.rotate(degrees);
	rotateEnds(rotation, tc0, tc1);
	resize(tc0, tc1);	
}

void JumperItem::calcRotation(QTransform & rotation, QPointF center, ViewGeometry & vg2) 
{
	QPointF tc0, tc1;
	rotateEnds(rotation, tc0, tc1);
	QPointF p = calcPos(tc0, tc1);
	QPointF myCenter = mapToScene(boundingRect().center());
	QTransform transf = QTransform().translate(-center.x(), -center.y()) * rotation * QTransform().translate(center.x(), center.y());
	QPointF q = transf.map(myCenter);
	vg2.setLoc(p + q - myCenter);
}

void JumperItem::rotateEnds(QTransform & rotation, QPointF & tc0, QPointF & tc1) 
{
	ConnectorItem * cc0 = m_connector0;
	QRectF r0 = cc0->rect();
	QPointF c0 = cc0->mapToScene(r0.center());
	ConnectorItem * cc1 = m_connector1;
	QRectF r1 = cc1->rect();
	QPointF c1 = cc1->mapToScene(r1.center());
	QPointF c((c0.x() + c1.x()) / 2, (c0.y() + c1.y()) / 2);
	QTransform transf = QTransform().translate(-c.x(), -c.y()) * rotation * QTransform().translate(c.x(), c.y());
	tc0 = transf.map(c0);
	tc1 = transf.map(c1);
}

QPointF JumperItem::calcPos(QPointF p0, QPointF p1) {
	QRectF r0 = m_connector0->rect();
	QPointF p(qMin(p0.x(), p1.x()) - (r0.width() / 2) - m_connectorTL.x(), 
			  qMin(p0.y(), p1.y()) - (r0.height() / 2) - m_connectorTL.y());
	return p;
}

QPointF JumperItem::dragOffset() {
	return m_dragStartConnectorPos - m_dragStartCenterPos;
}

void JumperItem::saveInstanceLocation(QXmlStreamWriter & streamWriter)
{
	streamWriter.writeAttribute("x", QString::number(m_viewGeometry.loc().x()));
	streamWriter.writeAttribute("y", QString::number(m_viewGeometry.loc().y()));
	streamWriter.writeAttribute("wireFlags", QString::number(m_viewGeometry.flagsAsInt()));
	GraphicsUtils::saveTransform(streamWriter, m_viewGeometry.transform());
}

bool JumperItem::hasPartNumberProperty()
{
	return false;
}

ViewLayer::ViewID JumperItem::useViewIDForPixmap(ViewLayer::ViewID vid, bool) 
{
    if (vid == ViewLayer::PCBView) {
        return ViewLayer::IconView;
    }

    return ViewLayer::UnknownView;
}
