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

#include "layerkinpaletteitem.h"
#include "../debugdialog.h"
#include "../layerattributes.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../svg/svgtext.h"

#include <qmath.h>

////////////////////////////////////////////////

LayerKinPaletteItem::LayerKinPaletteItem(PaletteItemBase * chief, ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu* itemMenu)
	: PaletteItemBase(modelPart, viewID, viewGeometry, id, itemMenu), 
    m_layerKinChief(chief), m_ok(false), m_passMouseEvents(false)
{
	setFlag(QGraphicsItem::ItemIsSelectable, true);
	m_modelPart->removeViewItem(this);  // we don't need to save layerkin
}

void LayerKinPaletteItem::initLKPI(LayerAttributes & layerAttributes, const LayerHash & viewLayers) {
	setViewLayerPlacement(layerAttributes.viewLayerPlacement);
	m_ok = setUpImage(m_modelPart, viewLayers, layerAttributes);
	//DebugDialog::debug(QString("lk accepts hover %1 %2 %3 %4 %5").arg(title()).arg(m_viewID).arg(m_id).arg(viewLayerID).arg(this->acceptHoverEvents()));
}

QVariant LayerKinPaletteItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
	//DebugDialog::debug(QString("lk item change %1 %2").arg(this->id()).arg(change));
	if (m_layerKinChief != nullptr) {
		if (change == ItemSelectedChange) {
			bool selected = value.toBool();
			if (m_blockItemSelectedChange && m_blockItemSelectedValue == selected) {
				m_blockItemSelectedChange = false;
			}
			else {
				m_layerKinChief->syncKinSelection(selected, this);
			}
		}
		//else if (change == ItemVisibleHasChanged && value.toBool()) {
		//this->setSelected(m_layerKinChief->syncSelected());
		//this->setPos(m_offset + m_layerKinChief->syncMoved());
		//}
		else if (change == ItemPositionHasChanged) {
			m_layerKinChief->syncKinMoved(this->m_offset, value.toPointF());
		}
	}
	return PaletteItemBase::itemChange(change, value);
}

void LayerKinPaletteItem::setOffset(double x, double y) {
	m_offset.setX(x);
	m_offset.setY(y);
	this->setPos(this->pos() + m_offset);
}

ItemBase * LayerKinPaletteItem::layerKinChief() {
	return m_layerKinChief;
}

bool LayerKinPaletteItem::ok() {
	return m_ok;
}

void LayerKinPaletteItem::updateConnections(bool includeRatsnest, QList<ConnectorItem *> & already) {
	if (m_layerKinChief != nullptr) {
		m_layerKinChief->updateConnections(includeRatsnest, already);
	}
	else {
		DebugDialog::debug("chief deleted before layerkin");
	}
}

void LayerKinPaletteItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	m_passMouseEvents = m_layerKinChief->mousePressEventK(this, event);
	return;

	//ItemBase::mousePressEvent(event);
}

void LayerKinPaletteItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	if (m_passMouseEvents) {
		m_layerKinChief->mouseMoveEventK(this, event);
		return;
	}

	PaletteItemBase::mouseMoveEvent(event);
}

void LayerKinPaletteItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	if (m_passMouseEvents) {
		m_layerKinChief->mouseReleaseEventK(this, event);
		return;
	}

	PaletteItemBase::mouseReleaseEvent(event);
}

void LayerKinPaletteItem::setHidden(bool hide) {
	ItemBase::setHidden(hide);
	m_layerKinChief->figureHover();
}

void LayerKinPaletteItem::setInactive(bool inactivate) {
	ItemBase::setInactive(inactivate);
	m_layerKinChief->figureHover();
}

void LayerKinPaletteItem::clearModelPart() {
	m_layerKinChief->clearModelPart();
}

bool LayerKinPaletteItem::stickyEnabled() {
	return m_layerKinChief->stickyEnabled();
}

bool LayerKinPaletteItem::isSticky() {
	return m_layerKinChief->isSticky();
}

bool LayerKinPaletteItem::isBaseSticky() {
	return m_layerKinChief->isBaseSticky();
}

void LayerKinPaletteItem::setSticky(bool s)
{
	m_layerKinChief->setSticky(s);
}

void LayerKinPaletteItem::addSticky(ItemBase * sticky, bool stickem) {
	m_layerKinChief->addSticky(sticky, stickem);
}

ItemBase * LayerKinPaletteItem::stickingTo() {
	return m_layerKinChief->stickingTo();
}

QList< QPointer<ItemBase> > LayerKinPaletteItem::stickyList() {
	return m_layerKinChief->stickyList();
}

bool LayerKinPaletteItem::alreadySticking(ItemBase * itemBase) {
	return m_layerKinChief->alreadySticking(itemBase);
}

void LayerKinPaletteItem::resetID() {
	long offset = m_id % ModelPart::indexMultiplier;
	ItemBase::resetID();
	m_id += offset;
}

QString LayerKinPaletteItem::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor)
{
	return m_layerKinChief->retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
}

ConnectorItem* LayerKinPaletteItem::newConnectorItem(Connector *connector)
{
	return m_layerKinChief->newConnectorItem(this, connector);
}


bool LayerKinPaletteItem::isSwappable() {
	return m_layerKinChief->isSwappable();
}

void LayerKinPaletteItem::setSwappable(bool swappable) {
	m_layerKinChief->setSwappable(swappable);
}

bool LayerKinPaletteItem::inRotation() {
	return layerKinChief()->inRotation();
}

void LayerKinPaletteItem::setInRotation(bool val) {
	layerKinChief()->setInRotation(val);
}

//////////////////////////////////////////////////

SchematicTextLayerKinPaletteItem::SchematicTextLayerKinPaletteItem(PaletteItemBase * chief, ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu* itemMenu)
	: LayerKinPaletteItem(chief, modelPart, viewID, viewGeometry, id, itemMenu)

{
}

void SchematicTextLayerKinPaletteItem::cacheLoaded(const LayerAttributes & layerAttributes)
{
	this->setProperty("textSvg", layerAttributes.loaded());
}

void SchematicTextLayerKinPaletteItem::setTransform2(const QTransform & currTransf) {
	transformItem(currTransf, false);
}

QString SchematicTextLayerKinPaletteItem::getTransformedSvg(const QString & svgToTransform, double & rotation) {
	QTransform chiefTransform = layerKinChief()->transform();      // assume chief already has rotation
	bool isFlipped = GraphicsUtils::isFlipped(chiefTransform, rotation);
	QString svg = svgToTransform;
	if (isFlipped) {
		svg = flipTextSvg(svg);
	}

	if (rotation >= 135 && rotation <= 225) {
		svg = rotate(svg, isFlipped);
	}
	return svg;
}

void SchematicTextLayerKinPaletteItem::transformItem(const QTransform & currTransf, bool includeRatsnest) {
	Q_UNUSED(currTransf);
	Q_UNUSED(includeRatsnest);

	QTransform chiefTransform = layerKinChief()->transform();

	if (m_textThings.count() == 0) {
		initTextThings();
	}

	double rotation;
	QString svg = this->property("textSvg").toString();
	svg = getTransformedSvg(svg, rotation);
	reloadRenderer(svg, true);

	setTransform(chiefTransform);
}

void SchematicTextLayerKinPaletteItem::initTextThings() {
	QByteArray textSvg = this->property("textSvg").toByteArray();

	QDomDocument doc;
	QString errorStr;
	int errorLine;
	int errorColumn;
	if (!doc.setContent(textSvg, &errorStr, &errorLine, &errorColumn)) {
		DebugDialog::debug(QString("unable to parse schematic text: %1 %2 %3:\n%4").arg(errorStr).arg(errorLine).arg(errorColumn).arg(QString(textSvg)));
		return;
	}

	QDomElement root = doc.documentElement();
	QDomNodeList nodeList = root.elementsByTagName("text");
	QList<QDomElement> texts;
	for (int i = 0; i < nodeList.count(); i++) {
		texts.append(nodeList.at(i).toElement());
	}

	positionTexts(texts);
}

QString SchematicTextLayerKinPaletteItem::flipTextSvg(const QString & textSvg) {
	QDomDocument doc;
	QString errorStr;
	int errorLine;
	int errorColumn;
	if (!doc.setContent(textSvg, &errorStr, &errorLine, &errorColumn)) {
		DebugDialog::debug(QString("unable to parse schematic text: %1 %2 %3:\n%4").arg(errorStr).arg(errorLine).arg(errorColumn).arg(QString(textSvg)));
		return "";
	}

	QDomElement root = doc.documentElement();
	QDomNodeList nodeList = root.elementsByTagName("text");
	QList<QDomElement> texts;
	for (int i = 0; i < nodeList.count(); i++) {
		texts.append(nodeList.at(i).toElement());
	}

	int ix = 0;
	Q_FOREACH (QDomElement text, texts) {
		QDomElement g = text.ownerDocument().createElement("g");
		text.parentNode().insertAfter(g, text);
		g.appendChild(text);
		QTransform m = m_textThings[ix++].flipMatrix;
		TextUtils::setSVGTransform(g, m);
	}

	return doc.toString();
}

void SchematicTextLayerKinPaletteItem::positionTexts(QList<QDomElement> &texts)
{
	// TODO: Render all texts at once
	// TODO: Reuse values if labels did not change. We might even hash them
	// by ModuleID + ID attribute globally.
	m_textThings.clear();

	for (QDomElement &text : texts) {
		TextThing textThing;

		QString id = text.attribute("id");
		if (id.isEmpty()) {
			id = "123";
			text.setAttribute("id", id);
		}

		QGraphicsSvgItem tempSvgItem;
		tempSvgItem.setSharedRenderer(new QSvgRenderer(text.toElement().ownerDocument().toByteArray()));
		QRectF boundingBox = tempSvgItem.renderer()->boundsOnElement(id);

		QTransform flipHorizontal;
		flipHorizontal.translate(boundingBox.center().x(), 0);
		flipHorizontal.scale(-1, 1);
		flipHorizontal.translate(-boundingBox.center().x(), 0);

		textThing.flipMatrix = flipHorizontal;
		textThing.newRect = boundingBox;
		m_textThings.append(textThing);
	}
}

void SchematicTextLayerKinPaletteItem::clearTextThings() {
	m_textThings.clear();
}

QString SchematicTextLayerKinPaletteItem::rotate(const QString & svg, bool isFlipped) {
	Q_UNUSED(isFlipped);

	QDomDocument doc;
	QString errorStr;
	int errorLine;
	int errorColumn;
	if (!doc.setContent(svg, &errorStr, &errorLine, &errorColumn)) {
		DebugDialog::debug(QString("unable to parse schematic text: %1 %2 %3:\n%4").arg(errorStr).arg(errorLine).arg(errorColumn).arg(QString(svg)));
		return svg;
	}

	QDomElement root = doc.documentElement();
	QDomNodeList nodeList = root.elementsByTagName("text");
	QList<QDomElement> texts;
	for (int i = 0; i < nodeList.count(); i++) {
		texts.append(nodeList.at(i).toElement());
	}

	int ix = 0;
	Q_FOREACH (QDomElement text, texts) {
		QDomElement g = text.ownerDocument().createElement("g");
		text.parentNode().insertAfter(g, text);
		g.appendChild(text);
		QRectF r = m_textThings[ix].newRect;
		ix++;
		QTransform m;
		m.translate(r.center().x(), r.center().y());
		QTransform inv = m.inverted();
		QTransform matrix = inv * QTransform().rotate(180) * m;
		TextUtils::setSVGTransform(g, matrix);
	}

	return doc.toString();
}

void SchematicTextLayerKinPaletteItem::setInitialTransform(const QTransform & matrix) {
	setTransform2(matrix);
}
