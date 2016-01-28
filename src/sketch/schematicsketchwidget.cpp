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

#include "schematicsketchwidget.h"
#include "../debugdialog.h"
#include "../items/virtualwire.h"
#include "../items/symbolpaletteitem.h"
#include "../items/tracewire.h"
#include "../items/partlabel.h"
#include "../connectors/connectoritem.h"
#include "../waitpushundostack.h"
#include "../items/moduleidnames.h"
#include "../fsvgrenderer.h"
#include "../utils/graphicsutils.h"
#include "../version/version.h"

#include <limits>

QSizeF SchematicSketchWidget::m_jumperItemSize = QSizeF(0, 0);

static QString SchematicTraceColor = "black";
static const double TraceHoverStrokeFactor = 3;
static const double TraceWidthMils = 9.7222;
static const double TraceWidthMilsOld = 33.3333;

bool sameGround(ConnectorItem * c1, ConnectorItem * c2) 
{
	bool c1Grounded = c1->isGrounded();
	bool c2Grounded = c2->isGrounded();
			
	return (c1Grounded == c2Grounded);
}

///////////////////////////////////////////////////

SchematicSketchWidget::SchematicSketchWidget(ViewLayer::ViewID viewID, QWidget *parent)
    : PCBSketchWidget(viewID, parent)
{
    m_oldSchematic = m_convertSchematic = false;
	m_shortName = QObject::tr("schem");
	m_viewName = QObject::tr("Schematic View");
	initBackgroundColor();

	m_cleanType = ninetyClean;

	m_updateDotsTimer.setInterval(20);
	m_updateDotsTimer.setSingleShot(true);
	connect(&m_updateDotsTimer, SIGNAL(timeout()), this, SLOT(updateBigDots()));
}

void SchematicSketchWidget::addViewLayers() {
	setViewLayerIDs(ViewLayer::Schematic, ViewLayer::SchematicTrace, ViewLayer::Schematic, ViewLayer::SchematicRuler, ViewLayer::SchematicNote);
	addViewLayersAux(ViewLayer::layersForView(ViewLayer::SchematicView), ViewLayer::layersForViewFromBelow(ViewLayer::SchematicView));
}

ViewLayer::ViewLayerID SchematicSketchWidget::getDragWireViewLayerID(ConnectorItem *) {
	return ViewLayer::SchematicTrace;
}

ViewLayer::ViewLayerID SchematicSketchWidget::getWireViewLayerID(const ViewGeometry & viewGeometry, ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	if (viewGeometry.getAnyTrace()) {
		return ViewLayer::SchematicTrace;
	}

	if (viewGeometry.getRatsnest()) {
		return ViewLayer::SchematicWire;
	}

	return SketchWidget::getWireViewLayerID(viewGeometry, viewLayerPlacement);
}

void SchematicSketchWidget::initWire(Wire * wire, int penWidth) {
	Q_UNUSED(penWidth);
	if (wire->getRatsnest()) {
		return;
	}

	wire->setPenWidth(getTraceWidth(), this, getTraceWidth() * TraceHoverStrokeFactor);
	wire->setColorString("black", 1.0, false);
}

bool SchematicSketchWidget::autorouteTypePCB() {
	return false;
}

void SchematicSketchWidget::tidyWires() {
	QList<Wire *> wires;
	QList<Wire *> visited;
	foreach (QGraphicsItem * item, scene()->selectedItems()) {
		Wire * wire = dynamic_cast<Wire *>(item);
		if (wire == NULL) continue;
		if ((wire->getViewGeometry().wireFlags() & ViewGeometry::SchematicTraceFlag) == 0) continue;
		if (visited.contains(wire)) continue;
	}
}

void SchematicSketchWidget::ensureTraceLayersVisible() {
	ensureLayerVisible(ViewLayer::SchematicTrace);
}

void SchematicSketchWidget::ensureTraceLayerVisible() {
	ensureLayerVisible(ViewLayer::SchematicTrace);
}

void SchematicSketchWidget::setClipEnds(ClipableWire * vw, bool) {
	vw->setClipEnds(false);
}

void SchematicSketchWidget::getBendpointWidths(Wire * wire, double width, double & bendpointWidth, double & bendpoint2Width, bool & negativeOffsetRect) 
{
	Q_UNUSED(wire);
	bendpointWidth = -width - 1;
	bendpoint2Width = width + ((m_oldSchematic) ? 3 : 1);
	negativeOffsetRect = true;
}

void SchematicSketchWidget::getLabelFont(QFont & font, QColor & color, ItemBase *) {
	font.setFamily("Droid Sans");
	font.setPointSize(getLabelFontSizeSmall());
	font.setBold(false);
	font.setItalic(false);
	color.setAlpha(255);
	color.setRgb(0);
}

void SchematicSketchWidget::setNewPartVisible(ItemBase * itemBase) {
	switch (itemBase->itemType()) {
		case ModelPart::Logo:
            if (itemBase->moduleID().contains("schematic", Qt::CaseInsensitive)) break;
		case ModelPart::Breadboard:
		case ModelPart::Jumper:
		case ModelPart::CopperFill:
		case ModelPart::Via:
		case ModelPart::Hole:
			// don't need to see the breadboard in the other views
			// but it's there so connections can be more easily synched between views
			itemBase->setVisible(false);
			itemBase->setEverVisible(false);
			return;
		default:
		    if (itemBase->moduleID().endsWith(ModuleIDNames::PadModuleIDName)) {
				itemBase->setVisible(false);
				itemBase->setEverVisible(false);
				return;
			}

			break;
	}
}

bool SchematicSketchWidget::canDropModelPart(ModelPart * modelPart) {
	if (!SketchWidget::canDropModelPart(modelPart)) return false;

	switch (modelPart->itemType()) {
		case ModelPart::Logo:
            if (modelPart->moduleID().contains("schematic", Qt::CaseInsensitive)) return true;
		case ModelPart::Jumper:
		case ModelPart::CopperFill:
		case ModelPart::Board:
		case ModelPart::ResizableBoard:
		case ModelPart::Breadboard:
		case ModelPart::Via:
		case ModelPart::Hole:
			return false;
		case ModelPart::Symbol:
		case ModelPart::SchematicSubpart:
			return true;
		default:
			break;
	}

	if (modelPart->moduleID().endsWith(ModuleIDNames::SchematicFrameModuleIDName)) return true;
	if (modelPart->moduleID().endsWith(ModuleIDNames::PadModuleIDName)) return false;
	if (modelPart->moduleID().endsWith(ModuleIDNames::CopperBlockerModuleIDName)) return false;

	return PCBSketchWidget::canDropModelPart(modelPart);
}

bool SchematicSketchWidget::includeSymbols() {
	return true;
}

bool SchematicSketchWidget::hasBigDots() {
	return true;
}

void SchematicSketchWidget::updateBigDots() 
{	
	QList<ConnectorItem *> connectorItems;
	foreach (QGraphicsItem * item, scene()->items()) {
		ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
		if (connectorItem == NULL) continue;
		if (connectorItem->attachedToItemType() != ModelPart::Wire) continue;

		TraceWire * traceWire = qobject_cast<TraceWire *>(connectorItem->attachedTo());
		if (traceWire == NULL) continue;

		//DebugDialog::debug(QString("update big dot %1 %2").arg(traceWire->id()).arg(connectorItem->connectorSharedID()));

		connectorItems.append(connectorItem);
	}

    QList<ConnectorItem *> visited;
	foreach (ConnectorItem * connectorItem, connectorItems) {
	    connectorItem->restoreColor(visited);
	}
}

void SchematicSketchWidget::changeConnection(long fromID, const QString & fromConnectorID,
									long toID, const QString & toConnectorID,
									ViewLayer::ViewLayerPlacement viewLayerPlacement,
									bool connect, bool doEmit, bool updateConnections)
{
	m_updateDotsTimer.stop();
	SketchWidget::changeConnection(fromID, fromConnectorID, toID, toConnectorID, viewLayerPlacement, connect,  doEmit,  updateConnections);
	m_updateDotsTimer.start();
}

void SchematicSketchWidget::setInstanceTitle(long itemID, const QString & oldText, const QString & newText, bool isUndoable, bool doEmit) {
	// isUndoable is true when setInstanceTitle is called from the infoview 

    if (isUndoable) {
	    SymbolPaletteItem * sitem = qobject_cast<SymbolPaletteItem *>(findItem(itemID));
	    if (sitem && sitem->isOnlyNetLabel()) {
            setProp(sitem, "label", ItemBase::TranslatedPropertyNames.value("label"), oldText, newText, true);
            return;
        }
    }

    SketchWidget::setInstanceTitle(itemID, oldText, newText, isUndoable, doEmit);
}

void SchematicSketchWidget::setProp(ItemBase * itemBase, const QString & prop, const QString & trProp, const QString & oldValue, const QString & newValue, bool redraw)
{
    if (prop =="label") {
        SymbolPaletteItem * sitem = qobject_cast<SymbolPaletteItem *>(itemBase);
        if (sitem != NULL && sitem->isOnlyNetLabel()) {
            if (sitem->getLabel() == newValue) {
                return;
            }

            QUndoCommand * parentCommand =  new QUndoCommand();
	        parentCommand->setText(tr("Change label from %1 to %2").arg(oldValue).arg(newValue));

	        new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);
	        new CleanUpRatsnestsCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);

	        QList<Wire *> done;
	        foreach (ConnectorItem * toConnectorItem, sitem->connector0()->connectedToItems()) {
		        Wire * w = qobject_cast<Wire *>(toConnectorItem->attachedTo());
		        if (w == NULL) continue;
		        if (done.contains(w)) continue;

		        QList<ConnectorItem *> ends;
		        removeWire(w, ends, done, parentCommand);
	        }

	        new SetPropCommand(this, itemBase->id(), "label", oldValue, newValue, true, parentCommand);
            new ChangeLabelTextCommand(this, itemBase->id(), oldValue, newValue, parentCommand);

	        new CleanUpRatsnestsCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
	        new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);

	        m_undoStack->waitPush(parentCommand, PropChangeDelay);
            return;
        }
    }

    SketchWidget::setProp(itemBase, prop, trProp, oldValue, newValue, redraw);
}

void SchematicSketchWidget::setVoltage(double v, bool doEmit)
{
	Q_UNUSED(doEmit);

	PaletteItem * item = getSelectedPart();
	if (item == NULL) return;

	if (item->itemType() != ModelPart::Symbol) return;

	SymbolPaletteItem * sitem = qobject_cast<SymbolPaletteItem *>(item);
	if (sitem == NULL) return;

	if (sitem->moduleID().compare("ground symbol", Qt::CaseInsensitive) == 0) return;
	if (v == sitem->voltage()) return;

	QUndoCommand * parentCommand =  new QUndoCommand();
	parentCommand->setText(tr("Change voltage from %1 to %2").arg(sitem->voltage()).arg(v));

	new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);
	new CleanUpRatsnestsCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);

	QList<Wire *> done;
	foreach (ConnectorItem * toConnectorItem, sitem->connector0()->connectedToItems()) {
		Wire * w = qobject_cast<Wire *>(toConnectorItem->attachedTo());
		if (w == NULL) continue;
		if (done.contains(w)) continue;

		QList<ConnectorItem *> ends;
		removeWire(w, ends, done, parentCommand);
	}

	new SetPropCommand(this, item->id(), "voltage", QString::number(sitem->voltage()), QString::number(v), true, parentCommand);

	new CleanUpRatsnestsCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
	new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);

	m_undoStack->waitPush(parentCommand, PropChangeDelay);
}

double SchematicSketchWidget::defaultGridSizeInches() {
	return GraphicsUtils::StandardSchematicSeparation10thinMils / 1000;
}

ViewLayer::ViewLayerID SchematicSketchWidget::getLabelViewLayerID(ItemBase *) {
	return ViewLayer::SchematicLabel;
}

const QString & SchematicSketchWidget::traceColor(ConnectorItem *) {
	if (m_lastColorSelected.isEmpty()) return SchematicTraceColor;

	else return m_lastColorSelected;
}

const QString & SchematicSketchWidget::traceColor(ViewLayer::ViewLayerPlacement) {
	return SchematicTraceColor;
}

bool SchematicSketchWidget::isInLayers(ConnectorItem * connectorItem, ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	Q_UNUSED(connectorItem);
	Q_UNUSED(viewLayerPlacement);
	return true;
}

bool SchematicSketchWidget::routeBothSides() {
	return false;
}

void SchematicSketchWidget::addDefaultParts() {
	SketchWidget::addDefaultParts();
}

bool SchematicSketchWidget::sameElectricalLayer2(ViewLayer::ViewLayerID, ViewLayer::ViewLayerID) {
	// schematic is always one layer
	return true;
}

double SchematicSketchWidget::getKeepout() {
	return 0.1 * GraphicsUtils::SVGDPI;  // in pixels
}

bool SchematicSketchWidget::acceptsTrace(const ViewGeometry & viewGeometry) {
	return viewGeometry.getSchematicTrace();
}

ViewGeometry::WireFlag SchematicSketchWidget::getTraceFlag() {
	return ViewGeometry::SchematicTraceFlag;
}

double SchematicSketchWidget::getTraceWidth() {
	return GraphicsUtils::SVGDPI * ((m_oldSchematic ) ? TraceWidthMilsOld : TraceWidthMils) / 1000;
}

double SchematicSketchWidget::getAutorouterTraceWidth() {
	return getTraceWidth();
}

void SchematicSketchWidget::extraRenderSvgStep(ItemBase * itemBase, QPointF offset, double dpi, double printerScale, QString & outputSvg)
{
	TraceWire * traceWire = qobject_cast<TraceWire *>(itemBase);
	if (traceWire == NULL) return;

	if (traceWire->connector0()->isBigDot()) {
		double r = traceWire->connector0()->rect().width();
		outputSvg += makeCircleSVG(traceWire->connector0()->sceneAdjustedTerminalPoint(NULL), r, offset, dpi, printerScale);
	}
	if (traceWire->connector1()->isBigDot()) {
		double r = traceWire->connector0()->rect().width();
		outputSvg += makeCircleSVG(traceWire->connector1()->sceneAdjustedTerminalPoint(NULL), r, offset, dpi, printerScale);
	}

}

QString SchematicSketchWidget::makeCircleSVG(QPointF p, double r, QPointF offset, double dpi, double printerScale)
{
	double cx = (p.x() - offset.x()) * dpi / printerScale;
	double cy = (p.y() - offset.y()) * dpi / printerScale;
	double rr = r * dpi / printerScale;

	QString stroke = "black";
	return QString("<circle  fill=\"black\" cx=\"%1\" cy=\"%2\" r=\"%3\" stroke-width=\"0\" stroke=\"none\" />")
			.arg(cx)
			.arg(cy)
			.arg(rr);
}

QString SchematicSketchWidget::generateCopperFillUnit(ItemBase * itemBase, QPointF whereToStart)
{
	Q_UNUSED(itemBase);
	Q_UNUSED(whereToStart);
	return "";
}

ViewLayer::ViewLayerPlacement SchematicSketchWidget::createWireViewLayerPlacement(ConnectorItem * from, ConnectorItem * to) {
	return SketchWidget::createWireViewLayerPlacement(from, to);
}

double SchematicSketchWidget::getWireStrokeWidth(Wire *, double wireWidth)
{
	return wireWidth * TraceHoverStrokeFactor;
}

Wire * SchematicSketchWidget::createTempWireForDragging(Wire * fromWire, ModelPart * wireModel, ConnectorItem * connectorItem, ViewGeometry & viewGeometry, ViewLayer::ViewLayerPlacement spec) 
{
	viewGeometry.setSchematicTrace(true);
	Wire * wire =  SketchWidget::createTempWireForDragging(fromWire, wireModel, connectorItem, viewGeometry, spec);
	if (fromWire) {
		wire->setColorString(fromWire->colorString(), fromWire->opacity(), false);
	}
	else {
		wire->setProperty(PCBSketchWidget::FakeTraceProperty, true);
	    wire->setColorString(traceColor(connectorItem), 1.0, false);
	}
	return wire;
}

void SchematicSketchWidget::rotatePartLabels(double degrees, QTransform & transform, QPointF center, QUndoCommand * parentCommand)
{
	PCBSketchWidget::rotatePartLabels(degrees, transform, center, parentCommand);
}

void SchematicSketchWidget::loadFromModelParts(QList<ModelPart *> & modelParts, BaseCommand::CrossViewType crossViewType, QUndoCommand * parentCommand, 
						bool offsetPaste, const QRectF * boundingRect, bool seekOutsideConnections, QList<long> & newIDs)
{
	SketchWidget::loadFromModelParts(modelParts, crossViewType, parentCommand, offsetPaste, boundingRect, seekOutsideConnections, newIDs);
}

void SchematicSketchWidget::selectAllWires(ViewGeometry::WireFlag flag) 
{
   SketchWidget::selectAllWires(flag);
}

bool SchematicSketchWidget::canConnect(Wire *, ItemBase *) {
    return true;
}

QString SchematicSketchWidget::checkDroppedModuleID(const QString & moduleID) {
    return moduleID;
}

LayerList SchematicSketchWidget::routingLayers(ViewLayer::ViewLayerPlacement) {
    LayerList layerList;
    layerList << ViewLayer::Schematic;
    return layerList;
}

bool SchematicSketchWidget::attachedToBottomLayer(ConnectorItem * connectorItem) {
    return (connectorItem->attachedToViewLayerID() == ViewLayer::Schematic) ||
           (connectorItem->attachedToViewLayerID() == ViewLayer::SchematicTrace);
}

bool SchematicSketchWidget::attachedToTopLayer(ConnectorItem *) {
    return false;
}

QSizeF SchematicSketchWidget::jumperItemSize() {
    if (SchematicSketchWidget::m_jumperItemSize.width() == 0) {
	    long newID = ItemBase::getNextID();
	    ViewGeometry viewGeometry;
	    viewGeometry.setLoc(QPointF(0, 0));
	    ItemBase * itemBase = addItem(referenceModel()->retrieveModelPart(ModuleIDNames::NetLabelModuleIDName), defaultViewLayerPlacement(NULL), BaseCommand::SingleView, viewGeometry, newID, -1, NULL);
	    if (itemBase) {
		    SymbolPaletteItem * netLabel = qobject_cast<SymbolPaletteItem *>(itemBase);
            netLabel->setLabel("00");
            SchematicSketchWidget::m_jumperItemSize = netLabel->boundingRect().size();
            deleteItem(itemBase, true, false, false);
        }
    }

	return SchematicSketchWidget::m_jumperItemSize;
}

QHash<QString, QString> SchematicSketchWidget::getAutorouterSettings() {
    return SketchWidget::getAutorouterSettings();
}

void SchematicSketchWidget::setAutorouterSettings(QHash<QString, QString> & autorouterSettings) {
    SketchWidget::setAutorouterSettings(autorouterSettings);
}

void SchematicSketchWidget::getDroppedItemViewLayerPlacement(ModelPart * modelPart, ViewLayer::ViewLayerPlacement & viewLayerPlacement) {
    SketchWidget::getDroppedItemViewLayerPlacement(modelPart, viewLayerPlacement);
}

ViewLayer::ViewLayerPlacement SchematicSketchWidget::getViewLayerPlacement(ModelPart * modelPart, QDomElement & instance, QDomElement & view, ViewGeometry & viewGeometry) 
{
    return SketchWidget::getViewLayerPlacement(modelPart, instance, view, viewGeometry);
}


void SchematicSketchWidget::viewGeometryConversionHack(ViewGeometry & viewGeometry, ModelPart * modelPart) {
    if (!m_convertSchematic) return;
    if (modelPart->itemType() == ModelPart::Wire) return;
    if (viewGeometry.transform().isIdentity()) return;

	ViewGeometry vg;
    ItemBase * itemBase = addItemAuxTemp(modelPart, ViewLayer::NewTop, vg, 0, true, viewID(), true);
    double rotation;
    if (GraphicsUtils::isFlipped(viewGeometry.transform().toAffine(), rotation)) {
        itemBase->flipItem(Qt::Horizontal);
    }
    itemBase->rotateItem(rotation, false);
    itemBase->saveGeometry();
    viewGeometry.setTransform(itemBase->getViewGeometry().transform());

    foreach (ItemBase * kin, itemBase->layerKin()) delete kin;
    delete itemBase;
}

void SchematicSketchWidget::setOldSchematic(bool old) {
    m_oldSchematic = old;
}

bool SchematicSketchWidget::isOldSchematic() {
    return m_oldSchematic;
}

void SchematicSketchWidget::setConvertSchematic(bool convert) {
    m_convertSchematic = convert;
}

void SchematicSketchWidget::resizeWires() {
    double tw = getTraceWidth();
    double sw = getWireStrokeWidth(NULL, tw);
    foreach (QGraphicsItem * item, scene()->items()) {
        Wire * wire = dynamic_cast<Wire *>(item);
        if (wire == NULL) continue;
        if (!wire->isTraceType(getTraceFlag())) continue;

        wire->setWireWidth(tw, this, sw);
    }
}

void SchematicSketchWidget::resizeLabels() {

    double fontSize = getLabelFontSizeSmall();
    foreach (QGraphicsItem * item, scene()->items()) {
        ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
        if (itemBase == NULL) continue;

        if (itemBase->hasPartLabel() && itemBase->partLabel() != NULL) {
            itemBase->partLabel()->setFontPointSize(fontSize);
        }
    }
}
