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

$Revision: 6912 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-09 08:18:59 +0100 (Sa, 09. Mrz 2013) $

********************************************************************/

#include "infographicsview.h"
#include "../debugdialog.h"
#include "../infoview/htmlinfoview.h"

#include <QMessageBox>

static LayerHash ViewLayers;

InfoGraphicsView::InfoGraphicsView( QWidget * parent )
	: ZoomableGraphicsView(parent)
{
	m_infoView = NULL;
	m_boardLayers = 1;
	m_hoverEnterMode = m_hoverEnterConnectorMode = false;
    m_smdOrientation = Qt::Vertical;
}

void InfoGraphicsView::viewItemInfo(ItemBase * item) {
	if (m_infoView == NULL) return;

	m_infoView->viewItemInfo(this, item ? item->layerKinChief() : item, swappingEnabled(item));
}

void InfoGraphicsView::hoverEnterItem(QGraphicsSceneHoverEvent * event, ItemBase * itemBase) {
	if (m_infoView == NULL) return;

	if (event->modifiers() & Qt::ShiftModifier || itemBase->viewID() == ViewLayer::IconView) {
		m_hoverEnterMode = true;
		m_infoView->hoverEnterItem(this, event, itemBase ? itemBase->layerKinChief() : itemBase, swappingEnabled(itemBase));
	}
}

void InfoGraphicsView::hoverLeaveItem(QGraphicsSceneHoverEvent * event, ItemBase * itemBase) {
	if (m_infoView == NULL) return;

	if (m_hoverEnterMode) {
		m_hoverEnterMode = false;
		m_infoView->hoverLeaveItem(this, event, itemBase ? itemBase->layerKinChief() : itemBase);
	}
}

void InfoGraphicsView::hoverEnterConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem * item) {
	if (m_infoView == NULL) return;

	if (event->modifiers() & Qt::ShiftModifier) {
		m_hoverEnterConnectorMode = true;
		m_infoView->hoverEnterConnectorItem(this, event, item, swappingEnabled(item->attachedTo()));
	}
}

void InfoGraphicsView::hoverLeaveConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem * item){
	if (m_infoView == NULL) return;

	if (m_hoverEnterConnectorMode) {
		m_hoverEnterConnectorMode = false;
		m_infoView->hoverLeaveConnectorItem(this, event, item);
	}
}

void InfoGraphicsView::setInfoView(HtmlInfoView * infoView) {
	m_infoView = infoView;
}

HtmlInfoView * InfoGraphicsView::infoView() {
	return m_infoView;
}

void InfoGraphicsView::mousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
}

void InfoGraphicsView::hidePartLabel(ItemBase * item) {
	Q_UNUSED(item);
}

void InfoGraphicsView::partLabelChanged(ItemBase * item, const QString &oldText, const QString & newText) {
	Q_UNUSED(item);
	Q_UNUSED(oldText);
	Q_UNUSED(newText);
}

void InfoGraphicsView::noteChanged(ItemBase * item, const QString &oldText, const QString & newText, QSizeF oldSize, QSizeF newSize) {
	Q_UNUSED(item);
	Q_UNUSED(oldText);
	Q_UNUSED(newText);
	Q_UNUSED(oldSize);
	Q_UNUSED(newSize);
}

QGraphicsItem *InfoGraphicsView::selectedAux() {
	QList<QGraphicsItem*> selItems = scene()->selectedItems();
	if(selItems.size() != 1) {
		return NULL;
	} else {
		return selItems[0];
	}
}

void InfoGraphicsView::partLabelMoved(ItemBase * itemBase, QPointF oldPos, QPointF oldOffset, QPointF newPos, QPointF newOffset) 
{
	Q_UNUSED(itemBase);
	Q_UNUSED(oldPos);
	Q_UNUSED(oldOffset);
	Q_UNUSED(newPos);
	Q_UNUSED(newOffset);
}

void InfoGraphicsView::rotateFlipPartLabel(ItemBase * itemBase, double degrees, Qt::Orientations flipDirection) {
	Q_UNUSED(itemBase);
	Q_UNUSED(degrees);
	Q_UNUSED(flipDirection);
}

void InfoGraphicsView::noteSizeChanged(ItemBase * itemBase, const QSizeF & oldSize, const QSizeF & newSize) {
	Q_UNUSED(itemBase);
    Q_UNUSED(oldSize);
    Q_UNUSED(newSize);
}

bool InfoGraphicsView::spaceBarIsPressed() {
	return false;
}

InfoGraphicsView * InfoGraphicsView::getInfoGraphicsView(QGraphicsItem * item)
{
	if (item == NULL) return NULL;

	QGraphicsScene * scene = item->scene();
	if (scene == NULL) return NULL;

	return dynamic_cast<InfoGraphicsView *>(scene->parent());
}

void InfoGraphicsView::initWire(Wire *, int penWidth) {
	Q_UNUSED(penWidth);
}


void InfoGraphicsView::getBendpointWidths(class Wire *, double w, double & w1, double & w2, bool & negativeOffsetRect) {
	Q_UNUSED(w);
	Q_UNUSED(w1);
	Q_UNUSED(w2);
	Q_UNUSED(negativeOffsetRect);
}

void InfoGraphicsView::getLabelFont(QFont &, QColor &, ItemBase *) {
}

double InfoGraphicsView::getLabelFontSizeTiny() {
        return 5;
}

double InfoGraphicsView::getLabelFontSizeSmall() {
	return 7;
}

double InfoGraphicsView::getLabelFontSizeMedium() {
	return 9;
}

double InfoGraphicsView::getLabelFontSizeLarge() {
	return 14;
}

bool InfoGraphicsView::hasBigDots() {
	return false;
}

void InfoGraphicsView::setVoltage(double v, bool doEmit) {
	if (doEmit) {
		emit setVoltageSignal(v, false);
	}
}

void InfoGraphicsView::resizeBoard(double w, double h, bool doEmit) {
	Q_UNUSED(w);
	Q_UNUSED(h);
	Q_UNUSED(doEmit);
}

void InfoGraphicsView::setResistance(QString resistance, QString pinSpacing)
{
	Q_UNUSED(resistance);
	Q_UNUSED(pinSpacing);
}

void InfoGraphicsView::setProp(ItemBase * item, const QString & prop, const QString & trProp, const QString & oldValue, const QString & newValue, bool redraw)
{
	Q_UNUSED(item);
	Q_UNUSED(prop);
	Q_UNUSED(trProp);
	Q_UNUSED(oldValue);
	Q_UNUSED(newValue);
	Q_UNUSED(redraw);
}

void InfoGraphicsView::setHoleSize(ItemBase * item, const QString & prop, const QString & trProp, const QString & oldValue, const QString & newValue, QRectF & oldRect, QRectF & newRect, bool redraw)
{
	Q_UNUSED(item);
	Q_UNUSED(prop);
	Q_UNUSED(trProp);
	Q_UNUSED(oldValue);
	Q_UNUSED(newValue);
	Q_UNUSED(oldRect);
	Q_UNUSED(newRect);
	Q_UNUSED(redraw);
}

void InfoGraphicsView::changeWireWidthMils(const QString newWidth) {
	Q_UNUSED(newWidth);
}

void InfoGraphicsView::changeWireColor(const QString newColor) {
	Q_UNUSED(newColor);
}

void InfoGraphicsView::swap(const QString & family, const QString & prop, QMap<QString, QString> & propsMap, ItemBase * itemBase) {
	emit swapSignal(family, prop, propsMap, itemBase);
}

void InfoGraphicsView::setInstanceTitle(long id, const QString & oldTitle, const QString & newTitle, bool isUndoable, bool doEmit) {
	Q_UNUSED(id);
	Q_UNUSED(newTitle);
	Q_UNUSED(oldTitle);
	Q_UNUSED(isUndoable);
	Q_UNUSED(doEmit);
}

LayerHash & InfoGraphicsView::viewLayers() {
	return ViewLayers;
}

void InfoGraphicsView::loadLogoImage(ItemBase *, const QString & oldSvg, const QSizeF oldAspectRatio, const QString & oldFilename, const QString & newFilename, bool addName) {
	Q_UNUSED(oldSvg);
	Q_UNUSED(oldAspectRatio);
	Q_UNUSED(oldFilename);
	Q_UNUSED(newFilename);
	Q_UNUSED(addName);
}

void InfoGraphicsView::setNoteFocus(QGraphicsItem * item, bool inFocus) {
	Q_UNUSED(item);
	Q_UNUSED(inFocus);
}

void InfoGraphicsView::setBoardLayers(int layers, bool redraw) {
	Q_UNUSED(redraw);
	m_boardLayers = layers;
}

int InfoGraphicsView::boardLayers() {
	return m_boardLayers;
}

VirtualWire * InfoGraphicsView::makeOneRatsnestWire(ConnectorItem * source, ConnectorItem * dest, bool routed, QColor color, bool force) {
	Q_UNUSED(source);
	Q_UNUSED(dest);
	Q_UNUSED(routed);
	Q_UNUSED(color);
	Q_UNUSED(force);
	return NULL;
}

void InfoGraphicsView::getRatsnestColor(QColor & color) 
{
	Q_UNUSED(color);
}

void InfoGraphicsView::changeBus(ItemBase *, bool connect, const QString & oldBus, const QString & newBus, QList<ConnectorItem *> &, const QString & message, const QString & oldLayout, const QString & newLayout)
{
	Q_UNUSED(connect);
	Q_UNUSED(oldBus);
	Q_UNUSED(newBus);
	Q_UNUSED(message);
    Q_UNUSED(oldLayout);
    Q_UNUSED(newLayout);
}

const QString & InfoGraphicsView::filenameIf()
{
	return ___emptyString___;
}

QString InfoGraphicsView::generateCopperFillUnit(ItemBase * itemBase, QPointF whereToStart)
{
	Q_UNUSED(itemBase);
	Q_UNUSED(whereToStart);
	return "";
}

void InfoGraphicsView::prepLegSelection(ItemBase *) 
{
}

void InfoGraphicsView::prepLegBendpointMove(ConnectorItem * from, int index, QPointF oldPos, QPointF newPos, ConnectorItem * to, bool changeConnections)
{
	Q_UNUSED(from);	
	Q_UNUSED(index);
	Q_UNUSED(oldPos);	
	Q_UNUSED(newPos);	
	Q_UNUSED(to);
	Q_UNUSED(changeConnections);
}

void InfoGraphicsView::prepLegCurveChange(ConnectorItem * from, int index, const class Bezier *, const class Bezier *, bool triggerFirstTime) 
{
	Q_UNUSED(from);	
	Q_UNUSED(index);
	Q_UNUSED(triggerFirstTime);
}

void InfoGraphicsView::prepLegBendpointChange(ConnectorItem *, int oldCount, int newCount, int index, QPointF, const class Bezier *, const class Bezier *, const class Bezier *, bool triggerFirstTime)
{
	Q_UNUSED(triggerFirstTime);
	Q_UNUSED(index);
	Q_UNUSED(oldCount);
	Q_UNUSED(newCount);
}

double InfoGraphicsView::getWireStrokeWidth(Wire *, double wireWidth)
{
	return wireWidth;
}

bool InfoGraphicsView::curvyWiresIndicated(Qt::KeyboardModifiers)
{
	return true;
}

void InfoGraphicsView::triggerRotate(ItemBase *, double degrees)
{
	Q_UNUSED(degrees);
}

void InfoGraphicsView::changePinLabels(ItemBase * itemBase, bool singleRow)
{
	Q_UNUSED(itemBase);
	Q_UNUSED(singleRow);
}

void InfoGraphicsView::renamePins(ItemBase *, const QStringList & oldLabels, const QStringList & newLabels, bool singleRow)
{
	Q_UNUSED(oldLabels);
	Q_UNUSED(newLabels);
	Q_UNUSED(singleRow);
}

ViewGeometry::WireFlag InfoGraphicsView::getTraceFlag()
{
	return ViewGeometry::NoFlag;
}

void InfoGraphicsView::setAnyInRotation()
{
}

void InfoGraphicsView::setActiveWire(Wire * wire)
{
	emit setActiveWireSignal(wire);
}

void InfoGraphicsView::setActiveConnectorItem(ConnectorItem * connectorItem)
{
	emit setActiveConnectorItemSignal(connectorItem);
}

void InfoGraphicsView::resolveTemporary(bool, ItemBase *)
{
}
void InfoGraphicsView::newWire(Wire * wire) 
{
	bool succeeded = connect(wire, SIGNAL(wireChangedSignal(Wire*, const QLineF & , const QLineF & , QPointF, QPointF, ConnectorItem *, ConnectorItem *)	),
			this, SLOT(wireChangedSlot(Wire*, const QLineF & , const QLineF & , QPointF, QPointF, ConnectorItem *, ConnectorItem *)),
			Qt::DirectConnection);		// DirectConnection means call the slot directly like a subroutine, without waiting for a thread or queue
	succeeded = connect(wire, SIGNAL(wireChangedCurveSignal(Wire*, const Bezier *, const Bezier *, bool)),
			this, SLOT(wireChangedCurveSlot(Wire*, const Bezier *, const Bezier *, bool)),
			Qt::DirectConnection);		// DirectConnection means call the slot directly like a subroutine, without waiting for a thread or queue
	succeeded = succeeded && connect(wire, SIGNAL(wireSplitSignal(Wire*, QPointF, QPointF, const QLineF & )),
			this, SLOT(wireSplitSlot(Wire*, QPointF, QPointF, const QLineF & )));
	succeeded = succeeded && connect(wire, SIGNAL(wireJoinSignal(Wire*, ConnectorItem *)),
			this, SLOT(wireJoinSlot(Wire*, ConnectorItem*)));
	if (!succeeded) {
		DebugDialog::debug("wire signal connect failed");
	}

	emit newWireSignal(wire);
}

void InfoGraphicsView::setSMDOrientation(Qt::Orientations orientation) {
    m_smdOrientation = orientation;
}

Qt::Orientations InfoGraphicsView::smdOrientation() {
    return m_smdOrientation;
}

void InfoGraphicsView::moveItem(ItemBase *, double x, double y) {
    Q_UNUSED(x);
    Q_UNUSED(y);
}

void InfoGraphicsView::updateRotation(ItemBase * itemBase) {
    if (m_infoView) m_infoView->updateRotation(itemBase);
}

void InfoGraphicsView::rotateX(double degrees, bool rubberBandLegEnabled, ItemBase * originatingItem) 
{
    Q_UNUSED(degrees);
    Q_UNUSED(rubberBandLegEnabled);
    Q_UNUSED(originatingItem);
}

