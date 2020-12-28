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


#include "itembase.h"
#include "partfactory.h"
#include "../debugdialog.h"
#include "../model/modelpart.h"
#include "../connectors/connectoritem.h"
#include "../connectors/connectorshared.h"
#include "../sketch/infographicsview.h"
#include "../connectors/connector.h"
#include "../connectors/bus.h"
#include "partlabel.h"
#include "../layerattributes.h"
#include "../fsvgrenderer.h"
#include "../svg/svgfilesplitter.h"
#include "../svg/svgflattener.h"
#include "../utils/folderutils.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"
#include "../utils/cursormaster.h"
#include "../utils/clickablelabel.h"
#include "../utils/familypropertycombobox.h"
#include "../referencemodel/referencemodel.h"

#include <QScrollBar>
#include <QTimer>
#include <QVector>
#include <QSet>
#include <QSettings>
#include <QComboBox>
#include <QBitmap>
#include <QApplication>
#include <QClipboard>
#include <qmath.h>

/////////////////////////////////

static QRegExp NumberMatcher;
static QHash<QString, double> NumberMatcherValues;

static constexpr double InactiveOpacity = 0.4;

bool numberValueLessThan(QString v1, QString v2)
{
	return NumberMatcherValues.value(v1, 0) <= NumberMatcherValues.value(v2, 0);
}

static QSvgRenderer MoveLockRenderer;
static QSvgRenderer StickyRenderer;

/////////////////////////////////

const QString ItemBase::ITEMBASE_FONT_PREFIX = "<font size='2'>";
const QString ItemBase::ITEMBASE_FONT_SUFFIX = "</font>";

QHash<QString, QString> ItemBase::TranslatedPropertyNames;

QPointer<ReferenceModel> ItemBase::TheReferenceModel = nullptr;

QString ItemBase::PartInstanceDefaultTitle;
const QList<ItemBase *> ItemBase::EmptyList;

const QColor ItemBase::HoverColor(0,0,0);
const double ItemBase::HoverOpacity = .20;
const QColor ItemBase::ConnectorHoverColor(0,0,255);
const double ItemBase::ConnectorHoverOpacity = .40;

const QColor StandardConnectedColor(0, 255, 0);
const QColor StandardUnconnectedColor(255, 0, 0);

QPen ItemBase::NormalPen(QColor(255,0,0));
QPen ItemBase::HoverPen(QColor(0, 0, 255));
QPen ItemBase::ConnectedPen(StandardConnectedColor);
QPen ItemBase::UnconnectedPen(StandardUnconnectedColor);
QPen ItemBase::ChosenPen(QColor(255,0,0));
QPen ItemBase::EqualPotentialPen(QColor(255,255,0));

QBrush ItemBase::NormalBrush(QColor(255,0,0));
QBrush ItemBase::HoverBrush(QColor(0,0,255));
QBrush ItemBase::ConnectedBrush(StandardConnectedColor);
QBrush ItemBase::UnconnectedBrush(StandardUnconnectedColor);
QBrush ItemBase::ChosenBrush(QColor(255,0,0));
QBrush ItemBase::EqualPotentialBrush(QColor(255,255,0));

static QHash<QString, QStringList> CachedValues;

///////////////////////////////////////////////////

ItemBase::ItemBase( ModelPart* modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu )
	: QGraphicsSvgItem(),
	  m_id(id),
	  m_viewGeometry(viewGeometry),
	  m_modelPart(modelPart),
	  m_viewID(viewID),
	  m_itemMenu(itemMenu)
{
	//DebugDialog::debug(QString("itembase %1 %2").arg(id).arg((long) static_cast<QGraphicsItem *>(this), 0, 16));
	if (m_modelPart) {
		m_modelPart->addViewItem(this);
	}
	setCursor(*CursorMaster::MoveCursor);

	setAcceptHoverEvents ( true );
}

ItemBase::~ItemBase() {
	//DebugDialog::debug(QString("deleting itembase %1 %2 %3").arg((long) this, 0, 16).arg(m_id).arg((long) m_modelPart, 0, 16));
	if (m_partLabel) {
		delete m_partLabel;
		m_partLabel = nullptr;
	}

	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		foreach (ConnectorItem * toConnectorItem, connectorItem->connectedToItems()) {
			toConnectorItem->tempRemove(connectorItem, true);
		}
	}

	foreach (ItemBase * itemBase, m_stickyList) {
		itemBase->addSticky(this, false);
	}

	if (m_modelPart) {
		m_modelPart->removeViewItem(this);
	}

	if (m_fsvgRenderer) {
		delete m_fsvgRenderer;
	}

}

void ItemBase::setTooltip() {
	if(m_modelPart) {
		QString title = instanceTitle();
		if(!title.isNull() && !title.isEmpty()) {
			setInstanceTitleTooltip(title);
		} else {
			setDefaultTooltip();
		}
	} else {
		setDefaultTooltip();
	}
}

void ItemBase::removeTooltip() {
	this->setToolTip(___emptyString___);
}

bool ItemBase::zLessThan(ItemBase * & p1, ItemBase * & p2)
{
	return p1->z() < p2->z();
}

qint64 ItemBase::getNextID() {
	return ModelPart::nextIndex() * ModelPart::indexMultiplier;								// make sure we leave room for layerkin inbetween
}

qint64 ItemBase::getNextID(qint64 index) {

	qint64 temp = index * ModelPart::indexMultiplier;						// make sure we leave room for layerkin inbetween
	ModelPart::updateIndex(index);
	return temp;
}

void ItemBase::resetID() {
	m_id = m_modelPart->modelIndex() * ModelPart::indexMultiplier;
}

double ItemBase::z() {
	return getViewGeometry().z();
}

ModelPart * ItemBase::modelPart() {
	return m_modelPart;
}

void ItemBase::setModelPart(ModelPart * modelPart) {
	m_modelPart = modelPart;
}

ModelPartShared * ItemBase::modelPartShared() {
	if (!m_modelPart) return nullptr;

	return m_modelPart->modelPartShared();
}

void ItemBase::initNames() {
	if (NumberMatcher.isEmpty()) {
		NumberMatcher.setPattern(QString("(([0-9]+(\\.[0-9]*)?)|\\.[0-9]+)([\\s]*([") + TextUtils::PowerPrefixesString + "]))?");
	}

	if (TranslatedPropertyNames.count() == 0) {
		TranslatedPropertyNames.insert("family", tr("family"));
		TranslatedPropertyNames.insert("type", tr("type"));
		TranslatedPropertyNames.insert("model", tr("model"));
		TranslatedPropertyNames.insert("size", tr("size"));
		TranslatedPropertyNames.insert("color", tr("color"));
		TranslatedPropertyNames.insert("resistance", tr("resistance"));
		TranslatedPropertyNames.insert("capacitance", tr("capacitance"));
		TranslatedPropertyNames.insert("inductance", tr("inductance"));
		TranslatedPropertyNames.insert("voltage", tr("voltage"));
		TranslatedPropertyNames.insert("current", tr("current"));
		TranslatedPropertyNames.insert("power", tr("power"));
		TranslatedPropertyNames.insert("pin spacing", tr("pin spacing"));
		TranslatedPropertyNames.insert("rated power", tr("rated power"));
		TranslatedPropertyNames.insert("rated voltage", tr("rated voltage"));
		TranslatedPropertyNames.insert("rated current", tr("rated current"));
		TranslatedPropertyNames.insert("version", tr("version"));
		TranslatedPropertyNames.insert("package", tr("package"));
		TranslatedPropertyNames.insert("shape", tr("shape"));
		TranslatedPropertyNames.insert("form", tr("form"));
		TranslatedPropertyNames.insert("part number", tr("part number"));
		TranslatedPropertyNames.insert("maximum resistance", tr("maximum resistance"));
		TranslatedPropertyNames.insert("pins", tr("pins"));
		TranslatedPropertyNames.insert("spacing", tr("spacing"));
		TranslatedPropertyNames.insert("pin spacing", tr("pin spacing"));
		TranslatedPropertyNames.insert("frequency", tr("frequency"));
		TranslatedPropertyNames.insert("processor", tr("processor"));
		TranslatedPropertyNames.insert("variant", tr("variant"));
		TranslatedPropertyNames.insert("layers", tr("layers"));
		TranslatedPropertyNames.insert("tolerance", tr("tolerance"));
		TranslatedPropertyNames.insert("descr", tr("descr"));
		TranslatedPropertyNames.insert("filename", tr("filename"));
		TranslatedPropertyNames.insert("title", tr("title"));
		TranslatedPropertyNames.insert("date", tr("date"));
		TranslatedPropertyNames.insert("rev", tr("rev"));
		TranslatedPropertyNames.insert("sheet", tr("sheet"));
		TranslatedPropertyNames.insert("project", tr("project"));
		TranslatedPropertyNames.insert("banded", tr("banded"));
		TranslatedPropertyNames.insert("top", tr("top"));
		TranslatedPropertyNames.insert("bottom", tr("bottom"));
		TranslatedPropertyNames.insert("copper bottom", tr("copper bottom"));
		TranslatedPropertyNames.insert("copper top", tr("copper top"));
		TranslatedPropertyNames.insert("silkscreen bottom", tr("silkscreen bottom"));
		TranslatedPropertyNames.insert("silkscreen top", tr("silkscreen top"));

		// TODO: translate more known property names from fzp files and resource xml files

	}

	PartInstanceDefaultTitle = tr("Part");

	QSettings settings;
	QString colorName = settings.value("ConnectedColor").toString();
	if (!colorName.isEmpty()) {
		QColor color;
		color.setNamedColor(colorName);
		setConnectedColor(color);
	}

	colorName = settings.value("UnconnectedColor").toString();
	if (!colorName.isEmpty()) {
		QColor color;
		color.setNamedColor(colorName);
		setUnconnectedColor(color);
	}

}

void ItemBase::saveInstance(QXmlStreamWriter & streamWriter) {
	streamWriter.writeStartElement(ViewLayer::viewIDXmlName(m_viewID));
	streamWriter.writeAttribute("layer", ViewLayer::viewLayerXmlNameFromID(m_viewLayerID));
	if (m_moveLock) {
		streamWriter.writeAttribute("locked", "true");
	}
	if (m_superpart) {
		streamWriter.writeAttribute("superpart", QString::number(m_superpart->modelPart()->modelIndex()));
	}
	if (m_viewLayerPlacement == ViewLayer::NewBottom && m_viewID == ViewLayer::PCBView) {
		streamWriter.writeAttribute("bottom", "true");
	}

	this->saveGeometry();
	writeGeometry(streamWriter);
	if (m_partLabel) {
		m_partLabel->saveInstance(streamWriter);
	}

	QList<ItemBase *> itemBases;
	itemBases.append(this);
	itemBases.append(layerKinChief()->layerKin());
	foreach (ItemBase * itemBase, itemBases) {
		if (itemBase->layerHidden()) {
			streamWriter.writeStartElement("layerHidden");
			streamWriter.writeAttribute("layer", ViewLayer::viewLayerXmlNameFromID(itemBase->viewLayerID()));
			streamWriter.writeEndElement();
		}
	}


	bool saveConnectorItems = false;
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		if (connectorItem->connectionsCount() > 0 || connectorItem->hasRubberBandLeg() || connectorItem->isGroundFillSeed()) {
			saveConnectorItems = true;
			break;
		}
	}

	if (saveConnectorItems) {
		streamWriter.writeStartElement("connectors");
		foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
			connectorItem->saveInstance(streamWriter);
		}
		streamWriter.writeEndElement();
	}


	streamWriter.writeEndElement();
}

void ItemBase::writeGeometry(QXmlStreamWriter & streamWriter) {
	streamWriter.writeStartElement("geometry");
	streamWriter.writeAttribute("z", QString::number(z()));
	this->saveInstanceLocation(streamWriter);
	// do not write attributes here
	streamWriter.writeEndElement();
}

ViewGeometry & ItemBase::getViewGeometry() {
	return m_viewGeometry;
}

ViewLayer::ViewID ItemBase::viewID() {
	return m_viewID;
}

QString & ItemBase::viewIDName() {
	return ViewLayer::viewIDName(m_viewID);
}

ViewLayer::ViewLayerID ItemBase::viewLayerID() const {
	return m_viewLayerID;
}

void ItemBase::setViewLayerID(const QString & layerName, const LayerHash & viewLayers) {
	//DebugDialog::debug(QString("using z %1").arg(layerName));
	setViewLayerID(ViewLayer::viewLayerIDFromXmlString(layerName), viewLayers);
}

void ItemBase::setViewLayerID(ViewLayer::ViewLayerID viewLayerID, const LayerHash & viewLayers) {
	m_viewLayerID = viewLayerID;
	if (m_zUninitialized) {
		ViewLayer * viewLayer = viewLayers.value(m_viewLayerID);
		if (viewLayer) {
			m_zUninitialized = false;
			if (!viewLayer->alreadyInLayer(m_viewGeometry.z())) {
				m_viewGeometry.setZ(viewLayer->nextZ());
			}
		}
	}

	//DebugDialog::debug(QString("using z: %1 z:%2 lid:%3").arg(title()).arg(m_viewGeometry.z()).arg(m_viewLayerID) );
}

void ItemBase::removeLayerKin() {
}

void ItemBase::hoverEnterConnectorItem(QGraphicsSceneHoverEvent *, ConnectorItem * ) {
	//DebugDialog::debug(QString("hover enter c %1").arg(instanceTitle()));
	hoverEnterConnectorItem();
}

void ItemBase::hoverEnterConnectorItem() {
	//DebugDialog::debug(QString("hover enter c %1").arg(instanceTitle()));
	m_connectorHoverCount++;
	hoverUpdate();
}

void ItemBase::hoverLeaveConnectorItem(QGraphicsSceneHoverEvent *, ConnectorItem * ) {
	hoverLeaveConnectorItem();
}

void ItemBase::hoverMoveConnectorItem(QGraphicsSceneHoverEvent *, ConnectorItem * ) {
}

void ItemBase::hoverLeaveConnectorItem() {
	//DebugDialog::debug(QString("hover leave c %1").arg(instanceTitle()));
	m_connectorHoverCount--;
	hoverUpdate();
}

void ItemBase::clearConnectorHover()
{
	m_connectorHoverCount2 = 0;
	hoverUpdate();
}

void ItemBase::connectorHover(ConnectorItem *, ItemBase *, bool hovering) {
	//DebugDialog::debug(QString("hover c %1 %2").arg(hovering).arg(instanceTitle()));

	if (hovering) {
		m_connectorHoverCount2++;
	}
	else {
		m_connectorHoverCount2--;
	}
	// DebugDialog::debug(QString("m_connectorHoverCount2 %1 %2").arg(instanceTitle()).arg(m_connectorHoverCount2));
	hoverUpdate();
}

void ItemBase::hoverUpdate() {
	this->update();
}

void ItemBase::mousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
}

void ItemBase::mouseDoubleClickConnectorEvent(ConnectorItem *) {
}

void ItemBase::mouseMoveConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
}

void ItemBase::mouseReleaseConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
}

bool ItemBase::filterMousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
	return false;
}

bool ItemBase::acceptsMousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
	return true;
}

bool ItemBase::acceptsMousePressLegEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
	return m_acceptsMousePressLegEvent;
}

void ItemBase::setAcceptsMousePressLegEvent(bool b) {
	m_acceptsMousePressLegEvent = b;
}

bool ItemBase::acceptsMouseReleaseConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
	return false;
}

bool ItemBase::acceptsMouseDoubleClickConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
	return false;
}

bool ItemBase::acceptsMouseMoveConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
	return false;
}

void ItemBase::connectionChange(ConnectorItem * /*onMe*/, ConnectorItem * /*onIt*/, bool /*connect*/) { }

void ItemBase::connectedMoved(ConnectorItem * /*from*/, ConnectorItem * /*to*/,  QList<ConnectorItem *> & /*already*/) { }

ItemBase * ItemBase::extractTopLevelItemBase(QGraphicsItem * item) {
	ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
	if (!itemBase) return nullptr;

	if (itemBase->topLevel()) return itemBase;

	return nullptr;
}

bool ItemBase::topLevel() {
	return (this == this->layerKinChief());
}

void ItemBase::setHidden(bool hide) {

	m_hidden = hide;
	updateHidden();
	foreach (QGraphicsItem * item, childItems()) {
		NonConnectorItem * nonconnectorItem = dynamic_cast<NonConnectorItem *>(item);
		if (!nonconnectorItem) continue;

		nonconnectorItem->setHidden(hide);
	}
}

void ItemBase::setInactive(bool inactivate) {

	m_inactive = inactivate;
	updateHidden();
	foreach (QGraphicsItem * item, childItems()) {
		NonConnectorItem * nonconnectorItem = dynamic_cast<NonConnectorItem *>(item);
		if (!nonconnectorItem) continue;

		nonconnectorItem->setInactive(inactivate);
	}
}

void ItemBase::setLayerHidden(bool layerHidden) {

	m_layerHidden = layerHidden;
	updateHidden();
	foreach (QGraphicsItem * item, childItems()) {
		NonConnectorItem * nonconnectorItem = dynamic_cast<NonConnectorItem *>(item);
		if (!nonconnectorItem) continue;

		nonconnectorItem->setLayerHidden(layerHidden);
	}
}

void ItemBase::updateHidden() {
	setAcceptedMouseButtons(m_hidden || m_inactive || m_layerHidden ? Qt::NoButton : ALLMOUSEBUTTONS);
	setAcceptHoverEvents(!(m_hidden || m_inactive || m_layerHidden));
	update();
}

void ItemBase::collectConnectors(ConnectorPairHash & connectorHash, SkipCheckFunction skipCheckFunction) {
	// Is this modelpart check obsolete?
	ModelPart * modelPart = this->modelPart();
	if (!modelPart) return;

	// collect all the connectorItem pairs

	foreach (ConnectorItem * fromConnectorItem, cachedConnectorItems()) {
		foreach (ConnectorItem * toConnectorItem, fromConnectorItem->connectedToItems()) {
			if (skipCheckFunction && skipCheckFunction(toConnectorItem)) continue;

			connectorHash.insert(fromConnectorItem, toConnectorItem);
		}

		ConnectorItem * crossConnectorItem = fromConnectorItem->getCrossLayerConnectorItem();
		if (!crossConnectorItem) continue;

		foreach (ConnectorItem * toConnectorItem, crossConnectorItem->connectedToItems()) {
			if (skipCheckFunction && skipCheckFunction(toConnectorItem)) continue;

			connectorHash.insert(crossConnectorItem, toConnectorItem);
		}
	}
}

ConnectorItem * ItemBase::findConnectorItemWithSharedID(const QString & connectorID)  {
	Connector * connector = modelPart()->getConnector(connectorID);
	if (connector) {
		return connector->connectorItem(m_viewID);
	}

	return nullptr;
}

ConnectorItem * ItemBase::findConnectorItemWithSharedID(const QString & connectorID, ViewLayer::ViewLayerPlacement viewLayerPlacement)  {
	ConnectorItem * connectorItem = findConnectorItemWithSharedID(connectorID);
	if (connectorItem) {
		return connectorItem->chooseFromSpec(viewLayerPlacement);
	}

	return nullptr;
}

void ItemBase::hoverEnterEvent ( QGraphicsSceneHoverEvent * event ) {
	// debugInfo("itembase hover enter");
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView && infoGraphicsView->spaceBarIsPressed()) {
		m_hoverEnterSpaceBarWasPressed = true;
		event->ignore();
		return;
	}

	m_hoverEnterSpaceBarWasPressed = false;
	m_hoverCount++;
	//debugInfo(QString("inc hover %1").arg(m_hoverCount));
	hoverUpdate();
	if (infoGraphicsView) {
		infoGraphicsView->hoverEnterItem(event, this);
	}
}

void ItemBase::hoverLeaveEvent ( QGraphicsSceneHoverEvent * event ) {
	//DebugDialog::debug(QString("hover leave %1").arg(instanceTitle()));
	if (m_hoverEnterSpaceBarWasPressed) {
		event->ignore();
		return;
	}

	m_hoverCount--;
	//debugInfo(QString("dec hover %1").arg(m_hoverCount));
	hoverUpdate();


	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView) {
		infoGraphicsView->hoverLeaveItem(event, this);
	}
}

void ItemBase::updateConnections(bool /* includeRatsnest */, QList<ConnectorItem *> & /* already */) { } 

void ItemBase::updateConnections(ConnectorItem * connectorItem, bool includeRatsnest, QList<ConnectorItem *> & already) {
	if (!already.contains(connectorItem)) {
		already << connectorItem;
		connectorItem->attachedMoved(includeRatsnest, already);
	}
	else {
		connectorItem->debugInfo("already");
	}
}

const QString & ItemBase::title() {
	if (!m_modelPart) return ___emptyString___;

	return m_modelPart->title();
}

const QString & ItemBase::constTitle() const {
	if (!m_modelPart) return ___emptyString___;

	return m_modelPart->title();
}

const QString & ItemBase::spice() const {
	if (!m_modelPart) return ___emptyString___;

	return m_modelPart->spice();
}

const QString & ItemBase::spiceModel() const {
	if (!m_modelPart) return ___emptyString___;

	return m_modelPart->spiceModel();
}

bool ItemBase::getRatsnest() {
	return m_viewGeometry.getRatsnest();
}

QList<Bus *> ItemBase::buses() {
	QList<Bus *> busList;
	if (!m_modelPart) return busList;

	foreach (Bus * bus, m_modelPart->buses().values()) {
		busList.append(bus);
	}

	return busList;
}

void ItemBase::busConnectorItems(class Bus * bus, ConnectorItem * /* fromConnectorItem */, QList<class ConnectorItem *> & items) {

	if (!bus) return;

	foreach (Connector * connector, bus->connectors()) {
		foreach (ConnectorItem * connectorItem, connector->viewItems()) {
			if (connectorItem) {
				//connectorItem->debugInfo(QString("on the bus %1").arg((long) connector, 0, 16));
				if (connectorItem->attachedTo() == this) {
					items.append(connectorItem);
				}
			}
		}
	}

	if (m_superpart || m_subparts.count() > 0) {
		Connector * connector = bus->subConnector();
		if (connector) {
			foreach (ConnectorItem * connectorItem, connector->viewItems()) {
				if (connectorItem) {
					//connectorItem->debugInfo(QString("on the bus %1").arg((long) connector, 0, 16));
					if (connectorItem->attachedToViewID() == m_viewID) {
						items.append(connectorItem);
					}
				}
			}
		}
	}


	/*
	if (items.count() > 0) {
	    fromConnectorItem->debugInfo("bus");
	    foreach (ConnectorItem * ci, items) {
	        ci->debugInfo("\t");
	    }
	}
	*/
}

int ItemBase::itemType() const
{
	if (!m_modelPart) return ModelPart::Unknown;

	return m_modelPart->itemType();
}

bool ItemBase::inHover() {
	return (!m_inactive && (m_connectorHoverCount > 0 || m_hoverCount > 0 || m_connectorHoverCount2 > 0));
}

void ItemBase::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	if (inHover()) {
		//DebugDialog::debug(QString("chc:%1 hc:%2 chc2:%3").arg(m_connectorHoverCount).arg(m_hoverCount).arg(m_connectorHoverCount2));
		layerKinChief()->paintHover(painter, option, widget);
	}
	//else {
	//DebugDialog::debug("no hover");
	//}

	if (m_inactive) {
		painter->save();
		painter->setOpacity(InactiveOpacity);
	}

	paintBody(painter, option, widget);

	if (option->state & QStyle::State_Selected) {
		layerKinChief()->paintSelected(painter, option, widget);
	}

	if (m_inactive) {
		painter->restore();
	}
}

void ItemBase::paintBody(QPainter *painter, const QStyleOptionGraphicsItem * /* option */, QWidget * /* widget */)
{
	// Qt's SVG renderer's defaultSize is not correct when the svg has a fractional pixel size
	fsvgRenderer()->render(painter, boundingRectWithoutLegs());
}

void ItemBase::paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	paintHover(painter, option, widget, hoverShape());
}

void ItemBase::paintSelected(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * /* widget */)
{
	GraphicsUtils::qt_graphicsItem_highlightSelected(painter, option, boundingRect(), hoverShape());
}

void ItemBase::paintHover(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/, const QPainterPath & shape)
{
	painter->save();
	if (m_connectorHoverCount > 0 || m_connectorHoverCount2 > 0) {
		painter->setOpacity(ConnectorHoverOpacity);
		painter->fillPath(shape, QBrush(ConnectorHoverColor));
	}
	else {
		painter->setOpacity(HoverOpacity);
		painter->fillPath(shape, QBrush(HoverColor));
	}
	painter->restore();
}

void ItemBase::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	InfoGraphicsView *infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView && infoGraphicsView->spaceBarIsPressed()) {
		event->ignore();
		return;
	}

	//scene()->setItemIndexMethod(QGraphicsScene::NoIndex);
	//setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	QGraphicsSvgItem::mousePressEvent(event);
}

void ItemBase::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	m_rightClickedConnector = nullptr;
	// calling parent class so that multiple selection will work
	// haven't yet discovered any nasty side-effect
	QGraphicsSvgItem::mouseReleaseEvent(event);

	//scene()->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
	// setCacheMode(QGraphicsItem::NoCache);

}

void ItemBase::mouseMoveEvent(QGraphicsSceneMouseEvent *)
{
}

void ItemBase::setItemPos(QPointF & loc) {
	setPos(loc);
}

bool ItemBase::stickyEnabled() {
	return true;
}

bool ItemBase::isSticky() {
	return isBaseSticky() && isLocalSticky();
}


bool ItemBase::isBaseSticky() {
	return m_sticky;   // to cancel sticky return false;
}

void ItemBase::setSticky(bool s)
{
	m_sticky = s;
}

bool ItemBase::isLocalSticky() {
	if (layerKinChief() != this) {
		return layerKinChief()->isLocalSticky();
	}
	QString stickyVal = modelPart()->localProp("sticky").toString();
	// return (stickyVal.compare("false") != 0);       // defaults to true
	return (stickyVal.compare("true") == 0);           // defaults to false
}

void ItemBase::setLocalSticky(bool s)
{
	// dirty the window?
	// undo command?
	if (layerKinChief() != this) {
		layerKinChief()->setLocalSticky(s);
		return;
	}

	modelPart()->setLocalProp("sticky", s ? "true" : "false");

	if (s) {
		if (!m_stickyItem) {
			if (!StickyRenderer.isValid()) {
				QString fn(":resources/images/part_sticky.svg");
				/* bool success = */ (void)StickyRenderer.load(fn);
				//DebugDialog::debug(QString("sticky load success %1").arg(success));
			}

			m_stickyItem = new QGraphicsSvgItem();
			m_stickyItem->setAcceptHoverEvents(false);
			m_stickyItem->setAcceptedMouseButtons(Qt::NoButton);
			m_stickyItem->setSharedRenderer(&StickyRenderer);
			m_stickyItem->setPos(!m_moveLockItem ? 0 : m_moveLockItem->boundingRect().width() + 1, 0);
			m_stickyItem->setZValue(-99999);
			m_stickyItem->setParentItem(this);
			m_stickyItem->setVisible(true);
		}
	}
	else {
		if (m_stickyItem) {
			delete m_stickyItem;
			m_stickyItem = nullptr;
		}
	}

	update();
}

void ItemBase::addSticky(ItemBase * stickyBase, bool stickem) {
	stickyBase = stickyBase->layerKinChief();
	//this->debugInfo(QString("add sticky %1:").arg(stickem));
	//sticky->debugInfo(QString("  to"));
	if (stickem) {
		if (!isBaseSticky()) {
			foreach (ItemBase * oldstickingTo, m_stickyList.values()) {
				if (oldstickingTo == stickyBase) continue;

				oldstickingTo->addSticky(this, false);
			}
			m_stickyList.clear();
		}
		m_stickyList.insert(stickyBase->id(), stickyBase);
	}
	else {
		m_stickyList.remove(stickyBase->id());
	}
}


ItemBase * ItemBase::stickingTo() {
	if (isBaseSticky()) return nullptr;

	if (m_stickyList.count() < 1) return nullptr;

	if (m_stickyList.count() > 1) {
		DebugDialog::debug(QString("error: sticky list > 1 %1").arg(title()));
	}

	return *m_stickyList.begin();
}

QList< QPointer<ItemBase> > ItemBase::stickyList() {
	return m_stickyList.values();
}

bool ItemBase::alreadySticking(ItemBase * itemBase) {
	return m_stickyList.value(itemBase->layerKinChief()->id(), nullptr);
}

ConnectorItem* ItemBase::newConnectorItem(Connector *connector)
{
	return newConnectorItem(this, connector);
}

ConnectorItem* ItemBase::newConnectorItem(ItemBase * layerKin, Connector *connector)
{
	return new ConnectorItem(connector, layerKin);
}

ConnectorItem * ItemBase::anyConnectorItem() {
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		return connectorItem;
	}

	return nullptr;
}


const QString & ItemBase::instanceTitle() const {
	if (m_modelPart) {
		return m_modelPart->instanceTitle();
	}
	return ___emptyString___;
}

void ItemBase::setInstanceTitle(const QString &title, bool initial) {
	setInstanceTitleAux(title, initial);
	if (m_partLabel) {
		m_partLabel->setPlainText(title);
	}
}

void ItemBase::updatePartLabelInstanceTitle() {
	if (m_partLabel) {
		m_partLabel->setPlainText(instanceTitle());
	}
}

void ItemBase::setInstanceTitleAux(const QString &title, bool initial)
{
	if (m_modelPart) {
		m_modelPart->setInstanceTitle(title, initial);
	}
	setInstanceTitleTooltip(title);

//	InfoGraphicsView *infographics = InfoGraphicsView::getInfoGraphicsView(this);
//	if (infographics ) {
//		infographics->setItemTooltip(this, title);
//	}
}

QString ItemBase::label() {
	if(m_modelPart) {
		return m_modelPart->label();
	}
	return ___emptyString___;
}

void ItemBase::updateTooltip() {
	setInstanceTitleTooltip(instanceTitle());
}

void ItemBase::setInstanceTitleTooltip(const QString &text) {
	setToolTip("<b>"+text+"</b><br></br>" + ITEMBASE_FONT_PREFIX + title()+ ITEMBASE_FONT_SUFFIX);
}

void ItemBase::setDefaultTooltip() {
	if (m_modelPart) {
		if (m_viewID == ViewLayer::IconView) {
			QString base = ITEMBASE_FONT_PREFIX + "%1" + ITEMBASE_FONT_SUFFIX;
			if(m_modelPart->itemType() != ModelPart::Wire) {
				this->setToolTip(base.arg(m_modelPart->title()));
			} else {
				this->setToolTip(base.arg(m_modelPart->title() + " (" + m_modelPart->moduleID() + ")"));
			}
			return;
		}

		QString title = ItemBase::PartInstanceDefaultTitle;
		QString inst = instanceTitle();
		if(!inst.isNull() && !inst.isEmpty()) {
			title = inst;
		} else {
			QString defaultTitle = label();
			if(!defaultTitle.isNull() && !defaultTitle.isEmpty()) {
				title = defaultTitle;
			}
		}
		ensureUniqueTitle(title, false);
		setInstanceTitleTooltip(instanceTitle());
	}
}

void ItemBase::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	if ((acceptedMouseButtons() & Qt::RightButton) == 0) {
		event->ignore();
		return;
	}

	if (m_hidden || m_inactive || m_layerHidden) {
		event->ignore();
		return;
	}

	scene()->clearSelection();
	setSelected(true);

	if (m_itemMenu) {
		m_rightClickedConnector = nullptr;
		foreach (QGraphicsItem * item, scene()->items(event->scenePos())) {
			ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
			if (!connectorItem) continue;

			if (connectorItem->attachedTo() == this) {
				m_rightClickedConnector = connectorItem;
				break;
			}
		}

		m_itemMenu->exec(event->screenPos());
	}
}

bool ItemBase::hasConnectors() {
	return cachedConnectorItems().count() > 0;
}

bool ItemBase::hasNonConnectors() {
	foreach (QGraphicsItem * childItem, childItems()) {
		if (dynamic_cast<NonConnectorItem *>(childItem)) return true;
	}

	return false;
}

bool ItemBase::canFlip(Qt::Orientations orientations) {
	bool result = true;
	if (orientations & Qt::Horizontal) {
		result = result && m_canFlipHorizontal;
	}
	if (orientations & Qt::Vertical) {
		result = result && m_canFlipVertical;
	}
	return result;
}

bool ItemBase::canFlipHorizontal() {
	return m_canFlipHorizontal && !m_moveLock;
}

void ItemBase::setCanFlipHorizontal(bool cf) {
	m_canFlipHorizontal = cf;
}

bool ItemBase::canFlipVertical() {
	return m_canFlipVertical && !m_moveLock;
}

void ItemBase::setCanFlipVertical(bool cf) {
	m_canFlipVertical = cf;
}

bool ItemBase::rotationAllowed() {
	return !m_moveLock;
}

bool ItemBase::rotation45Allowed() {
	return !m_moveLock;
}

bool ItemBase::freeRotationAllowed() {
	return false;
}

void ItemBase::clearModelPart() {
	m_modelPart = nullptr;
}

void ItemBase::hidePartLabel()
{
	InfoGraphicsView *infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView) infoGraphicsView->hidePartLabel(this);
}

void ItemBase::showPartLabel(bool showIt, ViewLayer* viewLayer) {
	if (m_partLabel) {
		m_partLabel->showLabel(showIt, viewLayer);
	}
}

void ItemBase::partLabelChanged(const QString & newText) {
	// sent from part label after inline edit
	InfoGraphicsView *infographics = InfoGraphicsView::getInfoGraphicsView(this);
	QString oldText = modelPart()->instanceTitle();
	setInstanceTitleAux(newText, false);
	if (infographics) {
		infographics->partLabelChanged(this, oldText, newText);
	}
}

bool ItemBase::isPartLabelVisible() {
	if (!m_partLabel) return false;
	if (!hasPartLabel()) return false;
	if (!m_partLabel->initialized()) return false;

	return m_partLabel->isVisible();
}

void ItemBase::clearPartLabel() {
	m_partLabel = nullptr;
}

void ItemBase::restorePartLabel(QDomElement & labelGeometry, ViewLayer::ViewLayerID viewLayerID)
{
	if (m_partLabel) {
		m_partLabel->setPlainText(instanceTitle());
		if (!labelGeometry.isNull()) {
			m_partLabel->restoreLabel(labelGeometry, viewLayerID);
			//m_partLabel->setPlainText(instanceTitle());
		}
	}
}

void ItemBase::movePartLabel(QPointF newPos, QPointF newOffset) {
	if (m_partLabel) {
		m_partLabel->moveLabel(newPos, newOffset);
	}
}

void ItemBase::partLabelSetHidden(bool hide) {
	if (m_partLabel) {
		m_partLabel->setHidden(hide);
	}
}

void ItemBase::partLabelMoved(QPointF oldPos, QPointF oldOffset, QPointF newPos, QPointF newOffset) {
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView) {
		infoGraphicsView->partLabelMoved(this, oldPos, oldOffset, newPos, newOffset);
	}
}

void ItemBase::rotateFlipPartLabel(double degrees, Qt::Orientations orientation)
{
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView) {
		infoGraphicsView->rotateFlipPartLabel(this, degrees, orientation);
	}
}


void ItemBase::doRotateFlipPartLabel(double degrees, Qt::Orientations orientation)
{
	if (m_partLabel) {
		m_partLabel->rotateFlipLabel(degrees, orientation);
	}
}

bool ItemBase::isSwappable() {
	return m_swappable;
}

void ItemBase::setSwappable(bool swappable) {
	m_swappable = swappable;
}

void ItemBase::ensureUniqueTitle(const QString & title, bool force) {
	if (force || instanceTitle().isEmpty() || instanceTitle().isNull()) {
		setInstanceTitle(modelPart()->getNextTitle(title), true);
	}
}

QVariant ItemBase::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value)
{
	switch (change) {
	case QGraphicsItem::ItemSelectedChange:
		if (m_partLabel) {
			m_partLabel->ownerSelected(value.toBool());
		}

		break;
	default:
		break;
	}

	return QGraphicsSvgItem::itemChange(change, value);
}

void ItemBase::cleanup() {
}

const QList<ItemBase *> & ItemBase::layerKin() {
	return EmptyList;
}

ItemBase * ItemBase::layerKinChief() {
	return this;
}

void ItemBase::rotateItem(double degrees, bool includeRatsnest) {
	//this->debugInfo(QString("\trotating item %1").arg(degrees));
	transformItem(QTransform().rotate(degrees), includeRatsnest);
}

void ItemBase::flipItem(Qt::Orientations orientation) {
	int xScale;
	int yScale;
	if(orientation == Qt::Vertical) {
		xScale = 1;
		yScale = -1;
	} else if(orientation == Qt::Horizontal) {
		xScale = -1;
		yScale = 1;
	}
	else {
		return;
	}

	transformItem(QTransform().scale(xScale,yScale), false);
}

void ItemBase::transformItem(const QTransform & currTransf, bool includeRatsnest) {
	//debugInfo("transform item " + TextUtils::svgMatrix(currTransf));

	QTransform trns = getViewGeometry().transform();
	//debugInfo("\t" + TextUtils::svgMatrix(trns));



	if (m_hasRubberBandLeg) {
		prepareGeometryChange();
	}
	QRectF rect = this->boundingRectWithoutLegs();

	//debugInfo(QString("\t bounding rect w:%1, h:%2").arg(rect.width()).arg(rect.height()));
	double x = rect.width() / 2.0;
	double y = rect.height() / 2.0;
	QTransform transf = QTransform().translate(-x, -y) * currTransf * QTransform().translate(x, y);
	getViewGeometry().setTransform(getViewGeometry().transform()*transf);
	this->setTransform(getViewGeometry().transform());
	if (!m_hasRubberBandLeg) {
		QList<ConnectorItem *> already;
		updateConnections(includeRatsnest, already);
	}

	trns = getViewGeometry().transform();
	//debugInfo("\t" + TextUtils::svgMatrix(trns));

	update();
}

void ItemBase::transformItem2(const QMatrix & matrix) {
	QTransform transform(matrix);
	transformItem(transform, false);
}

void ItemBase::collectWireConnectees(QSet<Wire *> & /* wires */) { } 

bool ItemBase::collectFemaleConnectees(QSet<ItemBase *> & /* items */) {
	return false;			// means no male connectors
}

void ItemBase::prepareGeometryChange() {
	// made public so it can be invoked from outside ItemBase class

	//debugInfo("itembase prepare geometry change");
	QGraphicsSvgItem::prepareGeometryChange();
}

void ItemBase::saveLocAndTransform(QXmlStreamWriter & streamWriter)
{
	streamWriter.writeAttribute("x", QString::number(m_viewGeometry.loc().x()));
	streamWriter.writeAttribute("y", QString::number(m_viewGeometry.loc().y()));
	GraphicsUtils::saveTransform(streamWriter, m_viewGeometry.transform());
}

FSvgRenderer * ItemBase::setUpImage(ModelPart * modelPart, LayerAttributes & layerAttributes)
{
	// at this point "this" has not yet been added to the scene, so one cannot get back to the InfoGraphicsView

	ModelPartShared * modelPartShared = modelPart->modelPartShared();

	if (!modelPartShared) {
		layerAttributes.error = tr("model part problem");
		return nullptr;
	}

	//if (modelPartShared->moduleID() == "df9d072afa2b594ac67b60b4153ff57b_29" && viewID == ViewLayer::PCBView) {
	//    DebugDialog::debug("here i am now");
	//}

	//DebugDialog::debug(QString("setting z %1 %2")
	//.arg(this->z())
	//.arg(ViewLayer::viewLayerNameFromID(viewLayerID))  );


	//DebugDialog::debug(QString("set up image elapsed (1) %1").arg(t.elapsed()) );
	QString filename = PartFactory::getSvgFilename(modelPart, modelPartShared->imageFileName(layerAttributes.viewID, layerAttributes.viewLayerID), true, true);

//#ifndef QT_NO_DEBUG
	//DebugDialog::debug(QString("set up image elapsed (2) %1").arg(t.elapsed()) );
//#endif

	if (filename.isEmpty()) {
		//QString deleteme = modelPartShared->domDocument()->toString();
		layerAttributes.error = tr("file for %1 %2 not found").arg(modelPartShared->title()).arg(modelPartShared->moduleID());
		return nullptr;
	}

	LoadInfo loadInfo;
	switch (layerAttributes.viewID) {
	case ViewLayer::PCBView:
		loadInfo.colorElementID = ViewLayer::viewLayerXmlNameFromID(layerAttributes.viewLayerID);
		switch (layerAttributes.viewLayerID) {
		case ViewLayer::Copper0:
			modelPartShared->connectorIDs(layerAttributes.viewID, layerAttributes.viewLayerID, loadInfo.connectorIDs, loadInfo.terminalIDs, loadInfo.legIDs);
			loadInfo.setColor = ViewLayer::Copper0Color;
			loadInfo.findNonConnectors = loadInfo.parsePaths = true;
			break;
		case ViewLayer::Copper1:
			modelPartShared->connectorIDs(layerAttributes.viewID, layerAttributes.viewLayerID, loadInfo.connectorIDs, loadInfo.terminalIDs, loadInfo.legIDs);
			loadInfo.setColor = ViewLayer::Copper1Color;
			loadInfo.findNonConnectors = loadInfo.parsePaths = true;
			break;
		case ViewLayer::Silkscreen1:
			loadInfo.setColor = ViewLayer::Silkscreen1Color;
			break;
		case ViewLayer::Silkscreen0:
			loadInfo.setColor = ViewLayer::Silkscreen0Color;
			break;
		default:
			break;
		}
		break;
	case ViewLayer::BreadboardView:
		modelPartShared->connectorIDs(layerAttributes.viewID, layerAttributes.viewLayerID, loadInfo.connectorIDs, loadInfo.terminalIDs, loadInfo.legIDs);
		break;
	default:
		// don't need connectorIDs() for schematic view since these parts do not have bendable legs or connectors with drill holes
		break;
	}

	FSvgRenderer * newRenderer = new FSvgRenderer();
	QDomDocument flipDoc;
	getFlipDoc(modelPart, filename, layerAttributes.viewLayerID, layerAttributes.viewLayerPlacement, flipDoc, layerAttributes.orientation);
	QByteArray bytesToLoad;
	if (layerAttributes.viewLayerID == ViewLayer::Schematic) {
		bytesToLoad = SvgFileSplitter::hideText(filename);
	}
	else if (layerAttributes.viewLayerID == ViewLayer::SchematicText) {
		bool hasText = false;
		bytesToLoad = SvgFileSplitter::showText(filename, hasText);
		if (!hasText) {
			return nullptr;
		}
	}
	else if ((layerAttributes.viewID != ViewLayer::IconView) && modelPartShared->hasMultipleLayers(layerAttributes.viewID)) {
		QString layerName = ViewLayer::viewLayerXmlNameFromID(layerAttributes.viewLayerID);
		// need to treat create "virtual" svg file for each layer
		SvgFileSplitter svgFileSplitter;
		bool result;
		if (flipDoc.isNull()) {
			result = svgFileSplitter.split(filename, layerName);
		}
		else {
			QString f = flipDoc.toString();
			result = svgFileSplitter.splitString(f, layerName);
		}
		if (result) {
			bytesToLoad = svgFileSplitter.byteArray();
		}
	}
	else {
		// only one layer, just load it directly
		if (flipDoc.isNull()) {
			QFile file(filename);
			file.open(QFile::ReadOnly);
			bytesToLoad = file.readAll();
		}
		else {
			bytesToLoad = flipDoc.toByteArray();
		}
	}

	QByteArray resultBytes;
	if (!bytesToLoad.isEmpty()) {
		if (makeLocalModifications(bytesToLoad, filename)) {
			if (layerAttributes.viewLayerID == ViewLayer::Schematic) {
				bytesToLoad = SvgFileSplitter::hideText2(bytesToLoad);
			}
			else if (layerAttributes.viewLayerID == ViewLayer::SchematicText) {
				bool hasText;
				bytesToLoad = SvgFileSplitter::showText2(bytesToLoad, hasText);
			}
		}

		loadInfo.filename = filename;
		resultBytes = newRenderer->loadSvg(bytesToLoad, loadInfo);
	}

	layerAttributes.setLoaded(resultBytes);

#ifndef QT_NO_DEBUG
//	DebugDialog::debug(QString("set up image elapsed (2.3) %1").arg(t.elapsed()) );
#endif

	if (resultBytes.isEmpty()) {
		delete newRenderer;
		layerAttributes.error = tr("unable to create renderer for svg %1").arg(filename);
		newRenderer = nullptr;
	}
	//DebugDialog::debug(QString("set up image elapsed (3) %1").arg(t.elapsed()) );

	if (newRenderer) {
		layerAttributes.setFilename(newRenderer->filename());
		if (layerAttributes.createShape) {
			createShape(layerAttributes);
		}
	}

	return newRenderer;
}

void ItemBase::updateConnectionsAux(bool includeRatsnest, QList<ConnectorItem *> & already) {
	//DebugDialog::debug("update connections");
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		updateConnections(connectorItem, includeRatsnest, already);
	}
}

void ItemBase::figureHover() {
}

QString ItemBase::retrieveSvg(ViewLayer::ViewLayerID /* viewLayerID */,  QHash<QString, QString> & /* svgHash */, bool /* blackOnly */, double /* dpi */, double & factor)
{
	factor = 1;
	return "";
}

bool ItemBase::hasConnections()
{
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		if (connectorItem->connectionsCount() > 0) return true;
	}

	return false;
}

void ItemBase::getConnectedColor(ConnectorItem *, QBrush &brush, QPen &pen, double & opacity, double & negativePenWidth, bool & negativeOffsetRect) {
	brush = ConnectedBrush;
	pen = ConnectedPen;
	opacity = 0.2;
	negativePenWidth = 0;
	negativeOffsetRect = true;
}

void ItemBase::getNormalColor(ConnectorItem *, QBrush &brush, QPen &pen, double & opacity, double & negativePenWidth, bool & negativeOffsetRect) {
	brush = NormalBrush;
	pen = NormalPen;
	opacity = NormalConnectorOpacity;
	negativePenWidth = 0;
	negativeOffsetRect = true;
}

void ItemBase::getUnconnectedColor(ConnectorItem *, QBrush &brush, QPen &pen, double & opacity, double & negativePenWidth, bool & negativeOffsetRect) {
	brush = UnconnectedBrush;
	pen = UnconnectedPen;
	opacity = 0.3;
	negativePenWidth = 0;
	negativeOffsetRect = true;
}

void ItemBase::getHoverColor(ConnectorItem *, QBrush &brush, QPen &pen, double & opacity, double & negativePenWidth, bool & negativeOffsetRect) {
	brush = HoverBrush;
	pen = HoverPen;
	opacity = NormalConnectorOpacity;
	negativePenWidth = 0;
	negativeOffsetRect = true;
}

void ItemBase::getEqualPotentialColor(ConnectorItem *, QBrush &brush, QPen &pen, double & opacity, double & negativePenWidth, bool & negativeOffsetRect) {
	brush = EqualPotentialBrush;
	pen = EqualPotentialPen;
	opacity = 1.0;
	negativePenWidth = 0;
	negativeOffsetRect = true;
}

void ItemBase::slamZ(double newZ) {
	double z = qFloor(m_viewGeometry.z()) + newZ;
	m_viewGeometry.setZ(z);
	setZValue(z);
}

bool ItemBase::isEverVisible() {
	return m_everVisible;
}

void ItemBase::setEverVisible(bool v) {
	m_everVisible = v;
}

bool ItemBase::connectionIsAllowed(ConnectorItem * other) {
	return ViewLayer::canConnect(this->viewLayerID(), other->attachedToViewLayerID());
}

QString ItemBase::getProperty(const QString & key) {
	if (!m_modelPart) return "";

	QString result = m_modelPart->localProp(key).toString();
	if (!result.isEmpty()) return result;

	return m_modelPart->properties().value(key, "");
}

ConnectorItem * ItemBase::rightClickedConnector() {
	return m_rightClickedConnector;
}

QColor ItemBase::connectedColor() {
	return ConnectedPen.color();
}

QColor ItemBase::unconnectedColor() {
	return UnconnectedPen.color();
}

QColor ItemBase::standardConnectedColor() {
	return StandardConnectedColor;
}

QColor ItemBase::standardUnconnectedColor() {
	return StandardUnconnectedColor;
}

void ItemBase::setConnectedColor(QColor & c) {
	ConnectedPen.setColor(c);
	ConnectedBrush.setColor(c);
}

void ItemBase::setUnconnectedColor(QColor & c) {
	UnconnectedPen.setColor(c);
	UnconnectedBrush.setColor(c);
}

QString ItemBase::translatePropertyName(const QString & key) {
	return TranslatedPropertyNames.value(key.toLower(), key);
}

bool ItemBase::canEditPart() {
	return false;
}

bool ItemBase::hasCustomSVG() {
	return false;
}

void ItemBase::setProp(const QString & prop, const QString & value) {
	if (!m_modelPart) return;

	//DebugDialog::debug(QString("setting prop %1 %2").arg(prop).arg(value));
	m_modelPart->setLocalProp(prop, value);
}

QString ItemBase::prop(const QString & p)
{
	if (!m_modelPart) return "";

	return m_modelPart->localProp(p).toString();
}

bool ItemBase::isObsolete() {
	if (!modelPart()) return false;

	return modelPart()->isObsolete();
}

bool ItemBase::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide)
{
	Q_UNUSED(hide);                 // assume this is set by the caller (HtmlInfoView)
	returnWidget = nullptr;
	returnProp = ItemBase::translatePropertyName(prop);
	returnValue = value;

	if (prop.compare("family", Qt::CaseInsensitive) == 0) {
		return true;
	}
	if (prop.compare("id", Qt::CaseInsensitive) == 0) {
		return true;
	}
#ifndef QT_NO_DEBUG
	if (prop.compare("svg", Qt::CaseInsensitive) == 0 || prop.compare("fzp", Qt::CaseInsensitive) == 0) {
		QFileInfo fileInfo(value);
		if (fileInfo.exists()) {
			ClickableLabel * label = new ClickableLabel(fileInfo.fileName(), parent);
			label->setProperty("path", value);
			label->setToolTip(value);
			connect(label, SIGNAL(clicked()), this, SLOT(showInFolder()));
			returnWidget = label;
		}
		return true;
	}
#endif

	QString tempValue = value;
	QStringList values = collectValues(family, prop, tempValue);
	if (values.count() > 1) {
		FamilyPropertyComboBox * comboBox = new FamilyPropertyComboBox(family, prop, parent);
		comboBox->setObjectName("infoViewComboBox");

		comboBox->addItems(values);
		comboBox->setCurrentIndex(comboBox->findText(tempValue));
		comboBox->setEnabled(swappingEnabled);
		comboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
		connect(comboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(swapEntry(const QString &)));

		returnWidget = comboBox;
		m_propsMap.insert(prop, tempValue);
		return true;
	}

	return true;
}

void ItemBase::swapEntry(const QString & text) {
	FamilyPropertyComboBox * comboBox = qobject_cast<FamilyPropertyComboBox *>(sender());
	if (!comboBox) return;

	m_propsMap.insert(comboBox->prop(), text);

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView) {
		infoGraphicsView->swap(comboBox->family(), comboBox->prop(), m_propsMap, this);
	}
}

void ItemBase::setReferenceModel(ReferenceModel * rm) {
	TheReferenceModel = rm;
}

QStringList ItemBase::collectValues(const QString & family, const QString & prop, QString & /* value */) {

	if (!TheReferenceModel) return ___emptyStringList___;

	QStringList values = CachedValues.value(family + prop, QStringList());
	if (values.count() > 0) return values;

	values = TheReferenceModel->propValues(family, prop, true);

	// sort values numerically
	NumberMatcherValues.clear();
	bool ok = true;
	foreach(QString opt, values) {
		int ix = NumberMatcher.indexIn(opt);
		if (ix < 0) {
			ok = false;
			break;
		}

		double n = TextUtils::convertFromPowerPrefix(NumberMatcher.cap(1) + NumberMatcher.cap(5), "");
		NumberMatcherValues.insert(opt, n);
	}
	if (ok) {
		qSort(values.begin(), values.end(), numberValueLessThan);
	}

	CachedValues.insert(family + prop, values);
	//debugInfo("cached " + prop);
	//foreach(QString v, values) {
	//    DebugDialog::debug("\t" + v);
	//}
	return values;
}

void ItemBase::resetValues(const QString & family, const QString & prop) {
	CachedValues.remove(family + prop);
}

bool ItemBase::hasPartLabel() {
	return true;
}

const QString & ItemBase::filename() {
	return m_filename;
}

void ItemBase::setFilename(const QString & fn) {
	m_filename = fn;
}

ItemBase::PluralType ItemBase::isPlural() {
	return ItemBase::NotSure;
}

ViewLayer::ViewLayerPlacement ItemBase::viewLayerPlacement() const {
	return m_viewLayerPlacement;
}

void ItemBase::setViewLayerPlacement(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	m_viewLayerPlacement = viewLayerPlacement;
}

ViewLayer::ViewLayerID ItemBase::partLabelViewLayerID() {
	if (!m_partLabel) return ViewLayer::UnknownLayer;
	if (!m_partLabel->initialized()) return ViewLayer::UnknownLayer;
	return m_partLabel->viewLayerID();
}

QString ItemBase::makePartLabelSvg(bool blackOnly, double dpi, double printerScale) {
	if (!m_partLabel) return "";
	if (!m_partLabel->initialized()) return "";
	return m_partLabel->makeSvg(blackOnly, dpi, printerScale, true);
}

QPointF ItemBase::partLabelScenePos() {
	if (!m_partLabel) return QPointF();
	if (!m_partLabel->initialized()) return QPointF();
	return m_partLabel->scenePos();
}

QRectF ItemBase::partLabelSceneBoundingRect() {
	if (!m_partLabel) return QRectF();
	if (!m_partLabel->initialized()) return QRectF();
	return m_partLabel->sceneBoundingRect();
}

bool ItemBase::getFlipDoc(ModelPart * modelPart, const QString & filename, ViewLayer::ViewLayerID viewLayerID, ViewLayer::ViewLayerPlacement viewLayerPlacement, QDomDocument & flipDoc, Qt::Orientations orientation)
{
	if (!modelPart->flippedSMD()) {
		// add copper1 layer to THT if it is missing
		fixCopper1(modelPart, filename, viewLayerID, viewLayerPlacement, flipDoc);
	}

	if (viewLayerPlacement == ViewLayer::NewBottom) {
		if (modelPart->flippedSMD()) {
			if (viewLayerID == ViewLayer::Copper0) {
				SvgFlattener::flipSMDSvg(filename, "", flipDoc, ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper1), ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper0), GraphicsUtils::SVGDPI, orientation);
				return true;
			}
			else if (viewLayerID == ViewLayer::Silkscreen0) {
				SvgFlattener::flipSMDSvg(filename, "", flipDoc, ViewLayer::viewLayerXmlNameFromID(ViewLayer::Silkscreen1), ViewLayer::viewLayerXmlNameFromID(ViewLayer::Silkscreen0), GraphicsUtils::SVGDPI, orientation);
				return true;
			}
			return false;
		}

		if (modelPart->itemType() == ModelPart::Part) {
			if (viewLayerID == ViewLayer::Copper0) {
				SvgFlattener::replaceElementID(filename, "", flipDoc, ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper0), "");
				//QString t1 = flipDoc.toString();
				SvgFlattener::flipSMDSvg(filename, "", flipDoc, ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper1), ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper0), GraphicsUtils::SVGDPI, orientation);
				//QString t2 = flipDoc.toString();
				return true;
			}
			if (viewLayerID == ViewLayer::Copper1) {
				SvgFlattener::replaceElementID(filename, "", flipDoc, ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper1), "");
				//QString t1 = flipDoc.toString();
				SvgFlattener::flipSMDSvg(filename, "", flipDoc, ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper0), ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper1), GraphicsUtils::SVGDPI, orientation);
				//QString t2 = flipDoc.toString();
				return true;
			}
			else if (viewLayerID == ViewLayer::Silkscreen0) {
				SvgFlattener::flipSMDSvg(filename, "", flipDoc, ViewLayer::viewLayerXmlNameFromID(ViewLayer::Silkscreen1), ViewLayer::viewLayerXmlNameFromID(ViewLayer::Silkscreen0), GraphicsUtils::SVGDPI, orientation);
				return true;
			}
		}
	}

	return false;
}

bool ItemBase::fixCopper1(ModelPart * modelPart, const QString & filename, ViewLayer::ViewLayerID viewLayerID, ViewLayer::ViewLayerPlacement /* viewLayerPlacement */, QDomDocument & doc)
{
	if (viewLayerID != ViewLayer::Copper1) return false;
	if (!modelPart->needsCopper1()) return false;

	return TextUtils::addCopper1(filename, doc, ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper0), ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper1));
}

void ItemBase::calcRotation(QTransform & rotation, QPointF center, ViewGeometry & vg2)
{
	vg2.setLoc(GraphicsUtils::calcRotation(rotation, center, pos(), boundingRectWithoutLegs().center()));
}

void ItemBase::updateConnectors()
{
	if (!isEverVisible()) return;

	QList<ConnectorItem *> visited;
	foreach(ConnectorItem * connectorItem, cachedConnectorItems()) {
		connectorItem->restoreColor(visited);
	}
	//DebugDialog::debug(QString("set up connectors restore:%1").arg(count));
}

const QString & ItemBase::moduleID() {
	if (m_modelPart) return m_modelPart->moduleID();

	return ___emptyString___;
}

bool ItemBase::moveLock() {
	return m_moveLock;
}

void ItemBase::setMoveLock(bool moveLock)
{
	m_moveLock = moveLock;
	if (moveLock) {
		if (!m_moveLockItem) {
			if (!MoveLockRenderer.isValid()) {
				QString fn(":resources/images/part_lock.svg");
				bool success = MoveLockRenderer.load(fn);
				DebugDialog::debug(QString("movelock load success %1").arg(success));
			}

			m_moveLockItem = new QGraphicsSvgItem();
			m_moveLockItem->setAcceptHoverEvents(false);
			m_moveLockItem->setAcceptedMouseButtons(Qt::NoButton);
			m_moveLockItem->setSharedRenderer(&MoveLockRenderer);
			m_moveLockItem->setPos(0,0);
			m_moveLockItem->setZValue(-99999);
			m_moveLockItem->setParentItem(this);
			m_moveLockItem->setVisible(true);
		}

	}
	else {
		if (m_moveLockItem) {
			delete m_moveLockItem;
			m_moveLockItem = nullptr;
		}
	}

	if (m_stickyItem) {
		m_stickyItem->setPos(!m_moveLockItem ? 0 : m_moveLockItem->boundingRect().width() + 1, 0);
	}

	update();
}

void ItemBase::debugInfo(const QString & msg) const
{

#ifndef QT_NO_DEBUG
	debugInfo2(msg);
#else
	Q_UNUSED(msg);
#endif
}

void ItemBase::debugInfo2(const QString & msg) const
{
	DebugDialog::debug(QString("%1 ti:'%2' id:%3 it:'%4' vid:%9 vlid:%5 spec:%6 x:%11 y:%12 z:%10 flg:%7 gi:%8")
	                   .arg(msg)
	                   .arg(this->constTitle())
	                   .arg(this->id())
	                   .arg(this->instanceTitle())
	                   .arg(this->viewLayerID())
	                   .arg(this->viewLayerPlacement())
	                   .arg(this->wireFlags())
	                   .arg((long) dynamic_cast<const QGraphicsItem *const>(this), 0, 16)
	                   .arg(m_viewID)
	                   .arg(this->zValue())
	                   .arg(this->pos().x())
	                   .arg(this->pos().y())
	                  );

	/*
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		if (connectorItem) connectorItem->debugInfo("\t");
	}
	*/
}

void ItemBase::addedToScene(bool temporary) {
	if (this->scene() && instanceTitle().isEmpty() && !temporary) {
		setTooltip();
		if (isBaseSticky() && isLocalSticky()) {
			// ensure icon is visible
			setLocalSticky(true);
		}
	}
}

bool ItemBase::hasPartNumberProperty()
{
	return true;
}

void ItemBase::collectPropsMap(QString & family, QMap<QString, QString> & propsMap) {
	QHash<QString, QString> properties;
	properties = m_modelPart->properties();
	family = properties.value("family", "");
	foreach (QString key, properties.keys()) {
		if (key.compare("family") == 0) continue;
		if (key.compare("id") == 0) continue;

		QString value = properties.value(key,"");
		QString tempValue = value;
		QStringList values = collectValues(family, key, tempValue);
		propsMap.insert(key, tempValue);
		DebugDialog::debug(QString("props map %1 %2").arg(key).arg(tempValue));
	}
}

void ItemBase::setDropOffset(QPointF)
{
}

bool ItemBase::hasRubberBandLeg() const
{
	return m_hasRubberBandLeg;
}

bool ItemBase::sceneEvent(QEvent *event)
{
	return QGraphicsSvgItem::sceneEvent(event);
}

const QList<ConnectorItem *> & ItemBase::cachedConnectorItems()
{
	if (m_cachedConnectorItems.isEmpty()) {
		foreach (QGraphicsItem * childItem, childItems()) {
			ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(childItem);
			if (connectorItem) m_cachedConnectorItems.append(connectorItem);
		}
	}

	return m_cachedConnectorItems;
}

const QList<ConnectorItem *> & ItemBase::cachedConnectorItemsConst() const
{
	return m_cachedConnectorItems;
}

void ItemBase::clearConnectorItemCache()
{
	m_cachedConnectorItems.clear();
}

void ItemBase::killRubberBandLeg() {
	if (!hasRubberBandLeg()) return;

	prepareGeometryChange();

	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		connectorItem->killRubberBandLeg();
	}
}

ViewGeometry::WireFlags ItemBase::wireFlags() const {
	return m_viewGeometry.wireFlags();
}

QRectF ItemBase::boundingRectWithoutLegs() const
{
	return boundingRect();
}

QRectF ItemBase::boundingRect() const
{
	FSvgRenderer * frenderer = fsvgRenderer();
	if (!frenderer) {
		return QGraphicsSvgItem::boundingRect();
	}

	QSizeF s = frenderer->defaultSizeF();
	QRectF r(0,0, s.width(), s.height());
	return r;
}

QPainterPath ItemBase::hoverShape() const
{
	return shape();
}

const QCursor * ItemBase::getCursor(Qt::KeyboardModifiers)
{
	return CursorMaster::MoveCursor;
}

PartLabel * ItemBase::partLabel() {
	return m_partLabel;
}

void ItemBase::doneLoading() {
}

QString ItemBase::family() {
	return modelPart()->family();
}

QPixmap * ItemBase::getPixmap(QSize size) {
	return FSvgRenderer::getPixmap(renderer(), size);
}

FSvgRenderer * ItemBase::fsvgRenderer() const {
	if (m_fsvgRenderer) return m_fsvgRenderer;

	FSvgRenderer * f = qobject_cast<FSvgRenderer *>(renderer());
	if (!f) {
		DebugDialog::debug("shouldn't happen: missing fsvgRenderer");
	}
	return f;
}

void ItemBase::setSharedRendererEx(FSvgRenderer * newRenderer) {
	if (newRenderer != m_fsvgRenderer) {
		setSharedRenderer(newRenderer);  // original renderer is deleted if it is not shared
		if (m_fsvgRenderer) delete m_fsvgRenderer;
		m_fsvgRenderer = newRenderer;
	}
	else {
		update();
	}
	m_size = newRenderer->defaultSizeF();
	//debugInfo(QString("set size %1, %2").arg(m_size.width()).arg(m_size.height()));
}

bool ItemBase::reloadRenderer(const QString & svg, bool fastLoad) {
	if (!svg.isEmpty()) {
		//DebugDialog::debug(svg);
		prepareGeometryChange();
		bool result = fastLoad ? fsvgRenderer()->fastLoad(svg.toUtf8()) : fsvgRenderer()->loadSvgString(svg.toUtf8());
		if (result) {
			update();
		}

		return result;
	}

	return false;
}

bool ItemBase::resetRenderer(const QString & svg) {
	// use resetRenderer instead of reloadRender because if the svg size changes, with reloadRenderer the new image seems to be scaled to the old bounds
	// what I don't understand is why the old renderer causes a crash if it is deleted here

	QString nothing;
	return resetRenderer(svg, nothing);
}

bool ItemBase::resetRenderer(const QString & svg, QString & newSvg) {
	// use resetRenderer instead of reloadRender because if the svg size changes, with reloadRenderer the new image seems to be scaled to the old bounds
	// what I don't understand is why the old renderer causes a crash if it is deleted here

	FSvgRenderer * newRenderer = new FSvgRenderer();
	bool result = newRenderer->loadSvgString(svg, newSvg);
	if (result) {
		//DebugDialog::debug("reloaded");
		//DebugDialog::debug(newSvg);
		setSharedRendererEx(newRenderer);
	}
	else {
		delete newRenderer;
	}
	return result;
}

void ItemBase::getPixmaps(QPixmap * & pixmap1, QPixmap * & pixmap2, QPixmap * & pixmap3, bool swappingEnabled, QSize size)
{
	pixmap1 = getPixmap(ViewLayer::BreadboardView, swappingEnabled, size);
	pixmap2 = getPixmap(ViewLayer::SchematicView, swappingEnabled, size);
	pixmap3 = getPixmap(ViewLayer::PCBView, swappingEnabled, size);
}

QPixmap * ItemBase::getPixmap(ViewLayer::ViewID vid, bool swappingEnabled, QSize size)
{
	ItemBase * vItemBase = nullptr;

	if (viewID() == vid) {
		if (!isEverVisible()) return nullptr;
	}
	else {
		vItemBase = modelPart()->viewItem(vid);
		if (vItemBase && !vItemBase->isEverVisible()) return nullptr;
	}

	vid = useViewIDForPixmap(vid, swappingEnabled);
	if (vid == ViewLayer::UnknownView) return nullptr;

	if (viewID() == vid) {
		return getPixmap(size);
	}

	if (vItemBase) {
		return vItemBase->getPixmap(size);
	}


	if (!modelPart()->hasViewFor(vid)) return nullptr;

	QString baseName = modelPart()->hasBaseNameFor(vid);
	if (baseName.isEmpty()) return nullptr;

	QString filename = PartFactory::getSvgFilename(modelPart(), baseName, true, true);
	if (filename.isEmpty()) {
		return nullptr;
	}

	QSvgRenderer renderer(filename);

	QPixmap * pixmap = new QPixmap(size);
	pixmap->fill(Qt::transparent);
	QPainter painter(pixmap);
	// preserve aspect ratio
	QSize def = renderer.defaultSize();
	double newW = size.width();
	double newH = newW * def.height() / def.width();
	if (newH > size.height()) {
		newH = size.height();
		newW = newH * def.width() / def.height();
	}
	QRectF bounds((size.width() - newW) / 2.0, (size.height() - newH) / 2.0, newW, newH);
	renderer.render(&painter, bounds);
	painter.end();

	return pixmap;
}

ViewLayer::ViewID ItemBase::useViewIDForPixmap(ViewLayer::ViewID vid, bool)
{
	if (vid == ViewLayer::BreadboardView) {
		return ViewLayer::IconView;
	}

	return vid;
}

bool ItemBase::makeLocalModifications(QByteArray &, const QString & ) {
	// a bottleneck for modifying part svg xml at setupImage time
	return false;
}

void ItemBase::showConnectors(const QStringList & connectorIDs) {
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		if (connectorIDs.contains(connectorItem->connectorSharedID())) {
			connectorItem->setVisible(true);
		}
	}
}

void ItemBase::setItemIsSelectable(bool selectable) {
	setFlag(QGraphicsItem::ItemIsSelectable, selectable);
}

bool ItemBase::inRotation() {
	return m_inRotation;
}

void ItemBase::setInRotation(bool val) {
	m_inRotation = val;
}

void ItemBase::addSubpart(ItemBase * sub)
{
	this->debugInfo("super");
	sub->debugInfo("\t");
	m_subparts.append(sub);
	sub->setSuperpart(this);
	foreach (ConnectorItem * connectorItem, sub->cachedConnectorItems()) {
		Bus * subbus = connectorItem->bus();
		Connector * subconnector = nullptr;
		if (!subbus) {
			subconnector = connectorItem->connector();
			if (subconnector) {
				subbus = new Bus(nullptr, nullptr);
				subconnector->setBus(subbus);
			}
		}

		Connector * connector = modelPart()->getConnector(connectorItem->connectorSharedID());
		if (connector) {
			if (subbus) subbus->addSubConnector(connector);
			if (subconnector) {
				Bus * bus = connector->bus();
				if (!bus) {
					bus = new Bus(nullptr, nullptr);
					connector->setBus(bus);
				}

				bus->addSubConnector(subconnector);
			}
		}
	}
}

void ItemBase::setSuperpart(ItemBase * super) {
	m_superpart = super;
}

ItemBase * ItemBase::superpart() {
	return m_superpart;
}

ItemBase * ItemBase::findSubpart(const QString & connectorID, ViewLayer::ViewLayerPlacement spec) {
	foreach (ItemBase * itemBase, m_subparts) {
		ConnectorItem * connectorItem = itemBase->findConnectorItemWithSharedID(connectorID, spec);
		if (connectorItem) return itemBase;
	}

	return nullptr;
}

const QList< QPointer<ItemBase> > & ItemBase::subparts()
{
	return m_subparts;
}

QHash<QString, QString> ItemBase::prepareProps(ModelPart * modelPart, bool wantDebug, QStringList & keys)
{
	m_propsMap.clear();

	// TODO: someday get local props
	QHash<QString, QString> props = modelPart->properties();
	QString family = props.value("family", "").toLower();

	// ensure family is first;
	keys = props.keys();
	keys.removeOne("family");
	keys.push_front("family");

	// ensure part number  is last
	QString partNumber = props.value(ModelPartShared::PartNumberPropertyName, "").toLower();
	keys.removeOne(ModelPartShared::PartNumberPropertyName);

	if (wantDebug) {
		props.insert("id", QString("%1 %2 %3")
		             .arg(QString::number(id()))
		             .arg(modelPart->moduleID())
		             .arg(ViewLayer::viewLayerNameFromID(viewLayerID()))
		            );
		keys.insert(1, "id");

		int insertAt = 2;
		PaletteItemBase * paletteItemBase = qobject_cast<PaletteItemBase *>(this);
		if (paletteItemBase) {
			props.insert("svg", paletteItemBase->filename());
			keys.insert(insertAt++, "svg");
		}
		props.insert("class", this->metaObject()->className());
		keys.insert(insertAt++, "class");

		if (modelPart->modelPartShared()) {
			props.insert("fzp",  modelPart->path());
			keys.insert(insertAt++, "fzp");
		}
	}

	// ensure part number is last
	if (hasPartNumberProperty()) {
		keys.append(ModelPartShared::PartNumberPropertyName);
	}

	return props;
}

void ItemBase::setSquashShape(bool squashShape) {
	m_squashShape = squashShape;
}

void ItemBase::createShape(LayerAttributes & layerAttributes) {
	switch (layerAttributes.viewID) {
	case ViewLayer::SchematicView:
	case ViewLayer::PCBView:
		break;
	default:
		return;
	}

	if (!isEverVisible()) return;

	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument doc;
	if (!doc.setContent(layerAttributes.loaded(), &errorStr, &errorLine, &errorColumn)) {
		return;
	}

	QDomElement root = doc.documentElement();

	QRectF viewBox;
	double w, h;
	TextUtils::ensureViewBox(doc, 1, viewBox, true, w, h, true);
	double svgDPI = viewBox.width() / w;
	int selectionExtra = layerAttributes.viewID == ViewLayer::SchematicView ? 20 : 10;
	SvgFileSplitter::forceStrokeWidth(root, svgDPI * selectionExtra / GraphicsUtils::SVGDPI, "#000000", true, false);

	double imageDPI = GraphicsUtils::SVGDPI;
	QRectF sourceRes(0, 0, w * imageDPI, h * imageDPI);
	QSize imgSize(qCeil(sourceRes.width()), qCeil(sourceRes.height()));
	QImage image(imgSize, QImage::Format_Mono);
	image.fill(0xffffffff);
	renderOne(&doc, &image, sourceRes);
	QBitmap bitmap = QBitmap::fromImage(image);
	QRegion region(bitmap);
	m_selectionShape.addRegion(region);

#ifndef QT_NODEBUG
	//QFileInfo info(layerAttributes.filename());
	//bitmap.save(FolderUtils::getUserDataStorePath("") + "/bitmap." + info.completeBaseName() + "." + QString::number(layerAttributes.viewLayerID) + ".png");
	//image.save(FolderUtils::getUserDataStorePath("") + "/image." + info.completeBaseName() + "." + QString::number(layerAttributes.viewLayerID) + ".png");
#endif
}

const QPainterPath & ItemBase::selectionShape() {
	return m_selectionShape;
}

void ItemBase::setTransform2(const QTransform & transform)
{
	setTransform(transform);
}

void ItemBase::renderOne(QDomDocument * masterDoc, QImage * image, const QRectF & renderRect) {
	QByteArray byteArray = masterDoc->toByteArray();
	QSvgRenderer renderer(byteArray);
	QPainter painter;
	painter.begin(image);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
	renderer.render(&painter, renderRect);
	painter.end();
}


void ItemBase::initLayerAttributes(LayerAttributes & layerAttributes, ViewLayer::ViewID viewID, ViewLayer::ViewLayerID viewLayerID, ViewLayer::ViewLayerPlacement viewLayerPlacement, bool doConnectors, bool doCreateShape) {
	layerAttributes.viewID = viewID;
	layerAttributes.viewLayerID = viewLayerID;
	layerAttributes.viewLayerPlacement = viewLayerPlacement;
	layerAttributes.doConnectors = doConnectors;
	layerAttributes.createShape = doCreateShape;
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView) {
		layerAttributes.orientation = infoGraphicsView->smdOrientation();
	}
}

void ItemBase::showInFolder() {
	QString path = sender()->property("path").toString();
	if (!path.isEmpty()) {
		FolderUtils::showInFolder(path);
		QClipboard *clipboard = QApplication::clipboard();
		if (clipboard) {
			clipboard->setText(path);
		}
	}
}

QString ItemBase::getInspectorTitle() {
	QString t = instanceTitle();
	if (!t.isEmpty()) return t;

	return title();
}

void ItemBase::setInspectorTitle(const QString & oldText, const QString & newText) {
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (!infoGraphicsView) return;

	DebugDialog::debug(QString("set instance title to %1").arg(newText));
	infoGraphicsView->setInstanceTitle(id(), oldText, newText, true, false);
}
