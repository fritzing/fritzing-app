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

#include "breadboardsketchwidget.h"
#include "../debugdialog.h"
#include "../items/virtualwire.h"
#include "../items/resizableboard.h"
#include "../connectors/connectoritem.h"
#include "../items/moduleidnames.h"
#include "../waitpushundostack.h"

#include <QScrollBar>
#include <QSettings>

static const double WireHoverStrokeFactor = 4.0;

BreadboardSketchWidget::BreadboardSketchWidget(ViewLayer::ViewID viewID, QWidget *parent)
    : SketchWidget(viewID, parent)
{
	m_shortName = QObject::tr("bb");
	m_viewName = QObject::tr("Breadboard View");
	initBackgroundColor();

    m_colorWiresByLength = false;
    QSettings settings;
    QString colorWiresByLength = settings.value(QString("%1ColorWiresByLength").arg(getShortName())).toString();
    if (!colorWiresByLength.isEmpty()) {
        m_colorWiresByLength = (colorWiresByLength.compare("1") == 0);
    }
}

void BreadboardSketchWidget::setWireVisible(Wire * wire)
{
	bool visible = !(wire->getTrace());
	wire->setVisible(visible);
	wire->setEverVisible(visible);
	//wire->setVisible(true);					// for debugging
}

bool BreadboardSketchWidget::collectFemaleConnectees(ItemBase * itemBase, QSet<ItemBase *> & items) {
	return itemBase->collectFemaleConnectees(items);
}

bool BreadboardSketchWidget::checkUnder() {
	return true;
};

void BreadboardSketchWidget::findConnectorsUnder(ItemBase * item) {
	item->findConnectorsUnder();
}

void BreadboardSketchWidget::addViewLayers() {
	setViewLayerIDs(ViewLayer::Breadboard, ViewLayer::BreadboardWire, ViewLayer::Breadboard, ViewLayer::BreadboardRuler, ViewLayer::BreadboardNote);
	addViewLayersAux(ViewLayer::layersForView(ViewLayer::BreadboardView), ViewLayer::layersForViewFromBelow(ViewLayer::BreadboardView));
}

bool BreadboardSketchWidget::disconnectFromFemale(ItemBase * item, QHash<long, ItemBase *> & savedItems, ConnectorPairHash & connectorHash, bool doCommand, bool rubberBandLegEnabled, QUndoCommand * parentCommand)
{
	// if item is attached to a virtual wire or a female connector in breadboard view
	// then disconnect it
	// at the moment, I think this doesn't apply to other views

	bool result = false;
	foreach (ConnectorItem * fromConnectorItem, item->cachedConnectorItems()) {
		if (rubberBandLegEnabled && fromConnectorItem->hasRubberBandLeg()) continue;

		foreach (ConnectorItem * toConnectorItem, fromConnectorItem->connectedToItems())  {
			if (rubberBandLegEnabled && toConnectorItem->hasRubberBandLeg()) continue;

			if (toConnectorItem->connectorType() == Connector::Female) {
				if (savedItems.keys().contains(toConnectorItem->attachedTo()->layerKinChief()->id())) {
					// the thing we're connected to is also moving, so don't disconnect
					continue;
				}

				result = true;
				fromConnectorItem->tempRemove(toConnectorItem, true);
				toConnectorItem->tempRemove(fromConnectorItem, true);
				if (doCommand) {
					extendChangeConnectionCommand(BaseCommand::CrossView, fromConnectorItem, toConnectorItem, ViewLayer::NewBottom, false, parentCommand);
				}
				connectorHash.insert(fromConnectorItem, toConnectorItem);

			}
		}
	}

	return result;
}


BaseCommand::CrossViewType BreadboardSketchWidget::wireSplitCrossView()
{
	return BaseCommand::CrossView;
}

bool BreadboardSketchWidget::canDropModelPart(ModelPart * modelPart) {	
	if (!SketchWidget::canDropModelPart(modelPart)) return false;

    if (Board::isBoard(modelPart)) {
        return matchesLayer(modelPart);
    }

	switch (modelPart->itemType()) {
		case ModelPart::Logo:
             if (modelPart->moduleID().contains("breadboard", Qt::CaseInsensitive)) return true;
		case ModelPart::Symbol:
		case ModelPart::SchematicSubpart:
		case ModelPart::Jumper:
		case ModelPart::CopperFill:
		case ModelPart::Hole:
		case ModelPart::Via:
			return false;
		default:
			if (modelPart->moduleID().endsWith(ModuleIDNames::SchematicFrameModuleIDName)) return false;
			if (modelPart->moduleID().endsWith(ModuleIDNames::PadModuleIDName)) return false;
			if (modelPart->moduleID().endsWith(ModuleIDNames::CopperBlockerModuleIDName)) return false;
			return true;
	}
}

void BreadboardSketchWidget::initWire(Wire * wire, int penWidth) {
	if (wire->getRatsnest()) {
		// handle elsewhere
		return;
	}
	wire->setPenWidth(penWidth - 2, this, (penWidth - 2) * WireHoverStrokeFactor);
	wire->setColorString("blue", 1.0, false);
    wire->colorByLength(m_colorWiresByLength);
}

const QString & BreadboardSketchWidget::traceColor(ViewLayer::ViewLayerPlacement) {
	if (!m_lastColorSelected.isEmpty()) return m_lastColorSelected;

	static QString color = "blue";
	return color;
}

double BreadboardSketchWidget::getTraceWidth() {
	// TODO: dig this constant out of wire.svg or somewhere else...
	return 2.0;
}

void BreadboardSketchWidget::getLabelFont(QFont & font, QColor & color, ItemBase *) {
	font.setFamily("Droid Sans");
	font.setPointSize(9);
	font.setBold(false);
	font.setItalic(false);
	color.setAlpha(255);
	color.setRgb(0);
}

void BreadboardSketchWidget::setNewPartVisible(ItemBase * itemBase) {
	switch (itemBase->itemType()) {
		case ModelPart::Symbol:
		case ModelPart::SchematicSubpart:
		case ModelPart::Jumper:
		case ModelPart::CopperFill:
		case ModelPart::Logo:
            if (itemBase->moduleID().contains("breadboard", Qt::CaseInsensitive)) return;
		case ModelPart::Hole:
		case ModelPart::Via:
			itemBase->setVisible(false);
			itemBase->setEverVisible(false);
			return;
		default:
			if (itemBase->moduleID().endsWith(ModuleIDNames::SchematicFrameModuleIDName) || 
				itemBase->moduleID().endsWith(ModuleIDNames::PadModuleIDName) ||
                itemBase->moduleID().endsWith(ModuleIDNames::PowerModuleIDName)) 
			{
				itemBase->setVisible(false);
				itemBase->setEverVisible(false);
				return;
			}
			break;
	}
}

bool BreadboardSketchWidget::canDisconnectAll() {
	return false;
}

bool BreadboardSketchWidget::ignoreFemale() {
	return false;
}

double BreadboardSketchWidget::defaultGridSizeInches() {
	return 0.1;
}

ViewLayer::ViewLayerID BreadboardSketchWidget::getLabelViewLayerID(ItemBase *) {
	return ViewLayer::BreadboardLabel;
}

void BreadboardSketchWidget::addDefaultParts() {
	long newID = ItemBase::getNextID();
	ViewGeometry viewGeometry;
	viewGeometry.setLoc(QPointF(0, 0));
    ModelPart * modelPart = referenceModel()->retrieveModelPart(ModuleIDNames::FullPlusBreadboardModuleIDName);
	m_addedDefaultPart = addItem(modelPart, defaultViewLayerPlacement(modelPart), BaseCommand::CrossView, viewGeometry, newID, -1, NULL);
	m_addDefaultParts = true;
	// have to put this off until later, because positioning the item doesn't work correctly until the view is visible
}

void BreadboardSketchWidget::showEvent(QShowEvent * event) {
	SketchWidget::showEvent(event);
	if (m_addDefaultParts && (m_addedDefaultPart != NULL)) {
		m_addDefaultParts = false;
		QSizeF partSize = m_addedDefaultPart->size();
		QSizeF vpSize = this->viewport()->size();
		//if (vpSize.height() < helpSize.height() + 50 + partSize.height()) {
			//vpSize.setWidth(vpSize.width() - verticalScrollBar()->width());
		//}

		QPointF p;
		p.setX((int) ((vpSize.width() - partSize.width()) / 2.0));
		p.setY((int) ((vpSize.height() - partSize.height()) / 2.0));
		QPointF q = mapToScene(p.toPoint());
		m_addedDefaultPart->setPos(q);
		alignOneToGrid(m_addedDefaultPart);
		QTimer::singleShot(10, this, SLOT(vScrollToZero()));
	}
}

double BreadboardSketchWidget::getWireStrokeWidth(Wire *, double wireWidth)
{
	return wireWidth * WireHoverStrokeFactor;
}

void BreadboardSketchWidget::getBendpointWidths(Wire * wire, double width, double & bendpointWidth, double & bendpoint2Width, bool & negativeOffsetRect) {
	Q_UNUSED(wire);
	Q_UNUSED(width);
	bendpoint2Width = bendpointWidth = -1;
	negativeOffsetRect = true;
}

void BreadboardSketchWidget::colorWiresByLength(bool colorByLength) {
    m_colorWiresByLength = colorByLength;
    QSettings settings;
    settings.setValue(QString("%1ColorWiresByLength").arg(viewName()), colorByLength);

    QList<Wire *> wires;
    QList<Wire *> visited;
    foreach (QGraphicsItem * item, scene()->items()) {
        Wire * wire = dynamic_cast<Wire *>(item);
        if (wire == NULL) continue;
        wire->colorByLength(colorByLength);
    }
}

bool BreadboardSketchWidget::coloringWiresByLength() {
    return m_colorWiresByLength;
}

