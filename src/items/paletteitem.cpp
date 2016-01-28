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

#include "paletteitem.h"
#include "../debugdialog.h"
#include "../viewgeometry.h"
#include "../sketch/infographicsview.h"
#include "layerkinpaletteitem.h"
#include "../fsvgrenderer.h"
#include "partlabel.h"
#include "partfactory.h"
#include "../commands.h"
#include "../connectors/connectoritem.h"
#include "../connectors/connector.h"
#include "../connectors/svgidlayer.h"
#include "../layerattributes.h"
#include "../dialogs/pinlabeldialog.h"
#include "../utils/folderutils.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"
#include "../utils/familypropertycombobox.h"
#include "../svg/svgfilesplitter.h"
#include "wire.h"

#include <QGraphicsSceneMouseEvent>
#include <QSvgRenderer>
#include <QtDebug>
#include <QPainter>
#include <QDomElement>
#include <QDir>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QRegExp>
#include <QGroupBox>
#include <QLabel>
#include <limits>

/////////////////////////////////////////////////

QString HoleSettings::currentUnits() {
	if (mmRadioButton->isChecked()) return QObject::tr("mm");
	return QObject::tr("in");
}

QString HoleSettings::holeSize() {
	return QString("%1,%2").arg(holeDiameter).arg(ringThickness);
}

/////////////////////////////////////////////////

static QRegExp LabelFinder("id=['|\"]label['|\"]");

static bool ByIDParseSuccessful = true;

static QRegExp ConnectorFinder("connector\\d+pin");
const QString PaletteItem::HoleSizePrefix("_hs_");

int findNumber(const QString & string) {
	int ix = string.indexOf(IntegerFinder);
	if (ix < 0) {
		return -1;
	}

	int result = IntegerFinder.cap(0).toInt();
	int length = IntegerFinder.cap(0).length();

	int jx = string.lastIndexOf(IntegerFinder);
	if (jx >= ix + length) {
		return -1;
	}

	return result;
}

bool byID(Connector * c1, Connector * c2)
{
	int i1 = findNumber(c1->connectorSharedID());
	if (i1 < 0) {
		ByIDParseSuccessful = false;
		return true;
	}
	int i2 = findNumber(c2->connectorSharedID());
	if (i2 < 0) {
		ByIDParseSuccessful = false;
		return true;
	}

	if (i2 == i1 && c1 != c2) {
		// should not be two connectors with the same number
		ByIDParseSuccessful = false;
		return true;
	}

	return i1 <= i2;
}

/////////////////////////////////////////////////

PaletteItem::PaletteItem( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: PaletteItemBase(modelPart, viewID, viewGeometry, id, itemMenu)
{
    m_flipCount = 0;
	if(doLabel) {
		m_partLabel = new PartLabel(this, NULL);
		m_partLabel->setVisible(false);
	} else {
		m_partLabel = NULL;
	}
}

PaletteItem::~PaletteItem() {
	if (m_partLabel) {
		delete m_partLabel;
	}
}

bool PaletteItem::renderImage(ModelPart * modelPart, ViewLayer::ViewID viewID, const LayerHash & viewLayers, ViewLayer::ViewLayerID viewLayerID, bool doConnectors,  QString & error) {
	LayerAttributes layerAttributes; 
    initLayerAttributes(layerAttributes, viewID, viewLayerID, viewLayerPlacement(), doConnectors, true);
	bool result = setUpImage(modelPart, viewLayers, layerAttributes);
    error = layerAttributes.error;

	m_syncMoved = this->pos();
	return result;
}

void PaletteItem::loadLayerKin(const LayerHash & viewLayers, ViewLayer::ViewLayerPlacement viewLayerPlacement) {

	if (m_modelPart == NULL) return;

	ModelPartShared * modelPartShared = m_modelPart->modelPartShared();
	if (modelPartShared == NULL) return;

	qint64 id = m_id + 1;
	ViewGeometry viewGeometry = m_viewGeometry;
	viewGeometry.setZ(-1);


	foreach (ViewLayer::ViewLayerID viewLayerID, viewLayers.keys()) {
		if (viewLayerID == m_viewLayerID) continue;
		if (!m_modelPart->hasViewFor(m_viewID, viewLayerID)) continue;

        if (m_modelPart->itemType() == ModelPart::CopperFill) {
            if (viewLayerPlacement == ViewLayer::NewTop) {
                if (ViewLayer::bottomLayers().contains(viewLayerID)) continue;
            }
            else {
                if (ViewLayer::topLayers().contains(viewLayerID)) continue;
            }
        }
        else if (m_modelPart->flippedSMD()) {
            if (viewLayerPlacement == ViewLayer::NewTop) {
                if (ViewLayer::bottomLayers().contains(viewLayerID)) continue;
            }
            else {
                if (ViewLayer::topLayers().contains(viewLayerID)) continue;
            }
        }
        else if (m_modelPart->itemType() == ModelPart::Part) {
            // through hole part
            if (ViewLayer::silkLayers().contains(viewLayerID)) {
                if (viewLayerPlacement == ViewLayer::NewTop) {
                    if (ViewLayer::bottomLayers().contains(viewLayerID)) continue;
                }
                else {
                    if (ViewLayer::topLayers().contains(viewLayerID)) continue;
                }
            }

        }

        makeOneKin(id, viewLayerID, viewLayerPlacement, viewGeometry, viewLayers);
	}

}

void PaletteItem::makeOneKin(qint64 & id, ViewLayer::ViewLayerID viewLayerID, ViewLayer::ViewLayerPlacement viewLayerPlacement, ViewGeometry & viewGeometry, const LayerHash & viewLayers) {
    LayerAttributes layerAttributes;
    initLayerAttributes(layerAttributes, m_viewID, viewLayerID, viewLayerPlacement, true, true);

    LayerKinPaletteItem * lkpi = newLayerKinPaletteItem(this, m_modelPart, viewGeometry, id, m_itemMenu, viewLayers, layerAttributes);
	if (lkpi->ok()) {
		DebugDialog::debug(QString("adding layer kin %1 %2 %3 %4")
            .arg(id).arg(m_viewID).arg(viewLayerID) 
            .arg((long) lkpi, 0, 16)
        );
		addLayerKin(lkpi);
		id++;
	}
	else {
		delete lkpi;
	}
}


void PaletteItem::addLayerKin(LayerKinPaletteItem * lkpi) {
	m_layerKin.append(lkpi);
}

void PaletteItem::removeLayerKin() {
	// assumes paletteitem is still in scene
	for (int i = 0; i < m_layerKin.size(); i++) {
		//DebugDialog::debug(QString("removing kin %1 %2").arg(m_layerKin[i]->id()).arg(m_layerKin[i]->z()));
		this->scene()->removeItem(m_layerKin[i]);
		delete m_layerKin[i];
	}

	m_layerKin.clear();
}

void PaletteItem::syncKinSelection(bool selected, PaletteItemBase * originator) {
	PaletteItemBase::syncKinSelection(selected, originator);

	foreach (ItemBase * lkpi, m_layerKin) {
		if (lkpi != originator && lkpi->isSelected() != selected) {
				qobject_cast<LayerKinPaletteItem *>(lkpi)->blockItemSelectedChange(selected);
				lkpi->setSelected(selected);
		}
	}

	if (this != originator && this->isSelected() != selected) {
		this->blockItemSelectedChange(selected);
		this->setSelected(selected);
	}
}

QVariant PaletteItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
	//DebugDialog::debug(QString("chief item change %1 %2").arg(this->id()).arg(change));
	if (m_layerKin.count() > 0) {
	    if (change == ItemSelectedChange) {
	       	bool selected = value.toBool();
	    	if (m_blockItemSelectedChange && m_blockItemSelectedValue == selected) {
	    		m_blockItemSelectedChange = false;
	   		}
			else {
	       		syncKinSelection(selected, this);
			}
	    }
	    //else if (change == ItemVisibleHasChanged && value.toBool()) {
	    	//this->setSelected(syncSelected());
	    	//this->setPos(m_offset + syncMoved());
	    //}
	    else if (change == ItemPositionHasChanged) {
	    	this->syncKinMoved(this->m_offset, value.toPointF());
	   	}
   	}

	if (m_partLabel && m_partLabel->initialized()) {
		if (change == ItemPositionHasChanged) {
	    	m_partLabel->ownerMoved(value.toPointF());
	   	}
		else if (change == ItemSelectedChange) {
			m_partLabel->update();
		}
	}

    return PaletteItemBase::itemChange(change, value);
}

const QList<class ItemBase *> & PaletteItem::layerKin()
{
	return m_layerKin;
}

void PaletteItem::rotateItem(double degrees, bool includeRatsnest) {
	PaletteItemBase::rotateItem(degrees, includeRatsnest);
	for (int i = 0; i < m_layerKin.count(); i++) {
		m_layerKin[i]->rotateItem(degrees, includeRatsnest);
	}
}

void PaletteItem::flipItem(Qt::Orientations orientation) {
	PaletteItemBase::flipItem(orientation);
	foreach (ItemBase * lkpi, m_layerKin) {
        lkpi->flipItem(orientation);
	}
}

void PaletteItem::transformItem2(const QMatrix & matrix) {
	PaletteItemBase::transformItem2(matrix);
	foreach (ItemBase * lkpi, m_layerKin) {
		lkpi->transformItem2(matrix);
	}
}

void PaletteItem::setTransforms() {
    // only ever called when loading from file
    // jrc 14 july 2013: this call seems redundant--transforms have already been set up by now

    QTransform transform = getViewGeometry().transform();
    if (transform.isIdentity()) return;

    //debugInfo("set transforms " + TextUtils::svgMatrix(transform));
    //debugInfo("\t " + TextUtils::svgMatrix(this->transform()));


	setTransform(transform);
	for (int i = 0; i < m_layerKin.count(); i++) {
        //debugInfo("\t " + TextUtils::svgMatrix(m_layerKin[i]->getViewGeometry().transform()));
        //debugInfo("\t " + TextUtils::svgMatrix(m_layerKin[i]->transform()));
		m_layerKin[i]->setTransform2(m_layerKin[i]->getViewGeometry().transform());
	}
}

void PaletteItem::moveItem(ViewGeometry & viewGeometry) {
	PaletteItemBase::moveItem(viewGeometry);
	for (int i = 0; i < m_layerKin.count(); i++) {
		m_layerKin[i]->moveItem(viewGeometry);
	}
}

void PaletteItem::setItemPos(QPointF & loc) {
	PaletteItemBase::setItemPos(loc);
	for (int i = 0; i < m_layerKin.count(); i++) {
		m_layerKin[i]->setItemPos(loc);
	}
}

void PaletteItem::updateConnections(bool includeRatsnest, QList<ConnectorItem *> & already) {
	updateConnectionsAux(includeRatsnest, already);
	foreach (ItemBase * lkpi, m_layerKin) {
		lkpi->updateConnectionsAux(includeRatsnest, already);
	}
}

bool PaletteItem::collectFemaleConnectees(QSet<ItemBase *> & items) {
	bool hasMale = PaletteItemBase::collectFemaleConnectees(items);
	foreach (ItemBase * lkpi, m_layerKin) {
		if (lkpi->collectFemaleConnectees(items)) {
			hasMale = true;
		}
	}
	return hasMale;
}

void PaletteItem::collectWireConnectees(QSet<Wire *> & wires) {
	PaletteItemBase::collectWireConnectees(wires);
	foreach (ItemBase * lkpi, m_layerKin) {
		qobject_cast<LayerKinPaletteItem *>(lkpi)->collectWireConnectees(wires);
	}
}

bool PaletteItem::mousePressEventK(PaletteItemBase * originalItem, QGraphicsSceneMouseEvent *event) {
	//DebugDialog::debug("layerkinchief got mouse press event");
	/*

	if (acceptsMousePressConnectorEvent(NULL, event) && isBuriedConnectorHit(event)  ) return;
	foreach(LayerKinPaletteItem * lkpi, m_layerKin) {
		if (lkpi->isBuriedConnectorHit(event)) return;
	}
	*/

    return PaletteItemBase::mousePressEventK(originalItem, event);
}

void PaletteItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	InfoGraphicsView *infographics = InfoGraphicsView::getInfoGraphicsView(this);
	if (infographics != NULL && infographics->spaceBarIsPressed()) { 
		event->ignore();
		return;
	}

    mousePressEventK(this, event);
}


void PaletteItem::syncKinMoved(QPointF offset, QPointF newPos) {
	Q_UNUSED(offset);    // ignore offset--should all be zeros now

	//DebugDialog::debug(QString("sync kin moved %1 %2").arg(offset.x()).arg(offset.y()) );
	//m_syncMoved = pos - offset;
	//if (newPos != pos()) {
		setPos(newPos);
		foreach (ItemBase * lkpi, m_layerKin) {
			lkpi->setPos(newPos);
		}
	//}
}

void PaletteItem::setInstanceTitle(const QString& title, bool initial) {
	ItemBase::setInstanceTitle(title, initial);
	foreach (ItemBase * lkpi, m_layerKin) {
		lkpi->setInstanceTitle(title, initial);
	}
}

void PaletteItem::setHidden(bool hide) {
	ItemBase::setHidden(hide);
	figureHover();
}

void PaletteItem::setInactive(bool inactivate) {
	ItemBase::setInactive(inactivate);
	figureHover();
}

void PaletteItem::figureHover() {
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(ALLMOUSEBUTTONS);
	foreach(ItemBase * lkpi, m_layerKin) {
		lkpi->setAcceptHoverEvents(true);
		lkpi->setAcceptedMouseButtons(ALLMOUSEBUTTONS);
	}
}

void PaletteItem::clearModelPart() {
	foreach (ItemBase * lkpi, m_layerKin) {
		lkpi->setModelPart(NULL);
	}
	ItemBase::clearModelPart();
}

void PaletteItem::resetID() {
	ItemBase::resetID();
	foreach (ItemBase * lkpi, m_layerKin) {
		lkpi->resetID();
	}
}

void PaletteItem::slamZ(double z) {
	PaletteItemBase::slamZ(z);
	foreach (ItemBase * lkpi, m_layerKin) {
		lkpi->slamZ(z);
	}
}

void PaletteItem::resetImage(InfoGraphicsView * infoGraphicsView) {
	foreach (Connector * connector, modelPart()->connectors()) {
		connector->unprocess(this->viewID(), this->viewLayerID());
	}
    
	LayerAttributes layerAttributes;
    initLayerAttributes(layerAttributes, viewID(), viewLayerID(), viewLayerPlacement(), true, !m_selectionShape.isEmpty());
	this->setUpImage(modelPart(), infoGraphicsView->viewLayers(), layerAttributes);
	
	foreach (ItemBase * layerKin, m_layerKin) {
		resetKinImage(layerKin, infoGraphicsView);
	}
}

void PaletteItem::resetKinImage(ItemBase * layerKin, InfoGraphicsView * infoGraphicsView) 
{
	foreach (Connector * connector, modelPart()->connectors()) {
		connector->unprocess(layerKin->viewID(), layerKin->viewLayerID());
	}
	LayerAttributes layerAttributes;
    initLayerAttributes(layerAttributes, layerKin->viewID(), layerKin->viewLayerID(), layerKin->viewLayerPlacement(), true, !layerKin->selectionShape().isEmpty());
	qobject_cast<PaletteItemBase *>(layerKin)->setUpImage(modelPart(), infoGraphicsView->viewLayers(), layerAttributes);
}

QString PaletteItem::genFZP(const QString & moduleid, const QString & templateName, int minPins, int maxPins, int steps, bool smd)
{
	QString FzpTemplate = "";
	QString ConnectorFzpTemplate = "";


	QFile file1(QString(":/resources/templates/%1.txt").arg(templateName));
	file1.open(QFile::ReadOnly);
	FzpTemplate = file1.readAll();
	file1.close();
	if (smd) {
		FzpTemplate.replace("<layer layerId=\"copper0\"/>", "");
	}

	QFile file2(":/resources/templates/generic_sip_connectorFzpTemplate.txt");
	file2.open(QFile::ReadOnly);
	ConnectorFzpTemplate = file2.readAll();
	file2.close();
	if (smd) {
		ConnectorFzpTemplate.replace("<p layer=\"copper0\" svgId=\"connector%1pin\"/>", "");
	}

	QStringList ss = moduleid.split("_");
	int count = 0;
	foreach (QString s, ss) {
		bool ok;
		int c = s.toInt(&ok);
		if (ok) {
			count = c;
			break;
		}
	}

	if (count > maxPins || count < minPins) return "";
	if (count % steps != 0) return "";

	QString middle;

	for (int i = 0; i < count; i++) {
		middle += ConnectorFzpTemplate.arg(i).arg(i + 1);
	}

	return FzpTemplate.arg(count).arg(middle);
}

bool PaletteItem::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide)
{
	if (prop.compare("editable pin labels", Qt::CaseInsensitive) == 0 && value.compare("true") == 0) {
		returnProp = "";
		returnValue = value;

		QPushButton * button = new QPushButton(tr("Edit Pin Labels"));
		button->setObjectName("infoViewButton");
		connect(button, SIGNAL(pressed()), this, SLOT(openPinLabelDialog()));
		button->setEnabled(swappingEnabled);

		returnWidget = button;

		return true;
	}

	bool result = PaletteItemBase::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);

    if (prop.compare("layer") == 0 && modelPart()->flippedSMD() && returnWidget != NULL) {
        bool disabled = true;
        InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
        if (infoGraphicsView && infoGraphicsView->boardLayers() == 2) disabled = false;
        returnWidget->setDisabled(disabled);
    }

    return result;
}

QStringList PaletteItem::collectValues(const QString & family, const QString & prop, QString & value) {
    if (prop.compare("layer") == 0) {
        if (modelPart()->flippedSMD()) {
            QStringList values = PaletteItemBase::collectValues(family, prop, value);
            for (int ix = values.count() - 1; ix >= 0; ix--) {
                if (values.at(ix).isEmpty()) {
                    values.removeAt(ix);
                }
            }
            if (values.count() < 2) {
                ItemBase * itemBase = modelPart()->viewItem(ViewLayer::PCBView);
                if (itemBase) {
                    values.clear();
                    values << TranslatedPropertyNames.value("bottom") << TranslatedPropertyNames.value("top");
                    if (itemBase->viewLayerID() == ViewLayer::Copper0) {
                        value = values.at(0);
                    }
                    else {
                        value = values.at(1);
                    }
                }
            }
            return values;
        }

        if (modelPart()->itemType() == ModelPart::Part) {
            QStringList values = PaletteItemBase::collectValues(family, prop, value);
            if (values.count() == 0) {
                ItemBase * itemBase = modelPart()->viewItem(ViewLayer::PCBView);
                if (itemBase) {
                    values << TranslatedPropertyNames.value("bottom") << TranslatedPropertyNames.value("top");
                    if (itemBase->viewLayerPlacement() == ViewLayer::NewBottom) {
                        value = values.at(0);
                    }
                    else {
                        value = values.at(1);
                    }
                }
            }
            return values;
        }
    }

   return PaletteItemBase::collectValues(family, prop, value);
}


void PaletteItem::openPinLabelDialog() {
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView == NULL) {
		QMessageBox::warning(
			NULL,
			tr("Fritzing"),
			tr("Unable to proceed; unable to find top level view.")
		);		
		return;	
	}

	QStringList labels;
	QList<Connector *> sortedConnectors = sortConnectors();
	if (sortedConnectors.count() == 0) {
		QMessageBox::warning(
			NULL,
			tr("Fritzing"),
			tr("Unable to proceed; part connectors do no have standard IDs.")
		);
		return;
	}

	foreach (Connector * connector, sortedConnectors) {
		labels.append(connector->connectorSharedName());
	}

	QString chipLabel = modelPart()->localProp("chip label").toString();
	if (chipLabel.isEmpty()) {
		chipLabel = instanceTitle();
	}

	bool singleRow = isSingleRow(cachedConnectorItems());
	PinLabelDialog pinLabelDialog(labels, singleRow, chipLabel, modelPart()->isCore(), NULL);
	int result = pinLabelDialog.exec();
	if (result != QDialog::Accepted) return;

	QStringList newLabels = pinLabelDialog.labels();
	if (newLabels.count() != sortedConnectors.count()) {
		QMessageBox::warning(
			NULL,
			tr("Fritzing"),
			tr("Label mismatch.  Nothing was saved.")
		);	
		return;
	}

	infoGraphicsView->renamePins(this, labels, newLabels, singleRow);
}

void PaletteItem::renamePins(const QStringList & labels, bool singleRow)
{
	QList<Connector *> sortedConnectors = sortConnectors();
	for (int i = 0; i < labels.count(); i++) {
		Connector * connector = sortedConnectors.at(i);
		connector->setConnectorLocalName(labels.at(i));
	}

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	infoGraphicsView->changePinLabels(this, singleRow);
}

bool PaletteItem::isSingleRow(const QList<ConnectorItem *> & connectorItems) {
	if (connectorItems.count() == 2) {
		// no way to tell? so default to double
		return false;
	}
	else if (connectorItems.count() % 2 == 0) {
		QPointF p = connectorItems.at(0)->sceneAdjustedTerminalPoint(NULL);
		double slope = 0;
		for (int i = 1; i < connectorItems.count(); i++) { 
			QPointF q = connectorItems.at(i)->sceneAdjustedTerminalPoint(NULL);
			if (p == q) continue;
			
			double newSlope = q.x() == p.x() ? std::numeric_limits<double>::max() : (q.y()  - p.y()) / (q.x() - p.x());
			if (i == 1) {
				slope = newSlope;
			}
			else {
				double d = qAbs(newSlope - slope);
				if (d != 0 && d / qMax(qAbs(slope), qAbs(newSlope)) > 0.01) {
					return false;
				}
			}
		}
	}

	return true;
}

QList<Connector *> PaletteItem::sortConnectors() { 
	QList<Connector *> sortedConnectors;
    foreach (Connector * connector, modelPart()->connectors().values()) {
        sortedConnectors.append(connector);
    }
	ByIDParseSuccessful = true;
	qSort(sortedConnectors.begin(), sortedConnectors.end(), byID);
	if (!ByIDParseSuccessful || sortedConnectors.count() == 0) {		
		sortedConnectors.clear();
	}

	return sortedConnectors;
}

bool PaletteItem::changePinLabels(bool singleRow, bool sip) {
	Q_UNUSED(singleRow);
	Q_UNUSED(sip);
	if (m_viewID != ViewLayer::SchematicView) return true;

	return false;
}

QStringList PaletteItem::getPinLabels(bool & hasLocal) {
	hasLocal = false;
	QStringList labels;
	QList<Connector *> sortedConnectors = sortConnectors();
	if (sortedConnectors.count() == 0) return labels;

	foreach (Connector * connector, sortedConnectors) {
		labels.append(connector->connectorSharedName());
		if (!connector->connectorLocalName().isEmpty()) {
			hasLocal = true;
		}
	}

	return labels;
}

void PaletteItem::resetConnectors() {
	if (m_viewID != ViewLayer::SchematicView) return;

    FSvgRenderer * renderer = fsvgRenderer();
    if (renderer == NULL) return;

	QSizeF size = renderer->defaultSizeF();   // pixels
	QRectF viewBox = renderer->viewBoxF();
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		SvgIdLayer * svgIdLayer = connectorItem->connector()->fullPinInfo(m_viewID, m_viewLayerID);
		if (svgIdLayer == NULL) continue;

		QRectF bounds = renderer->boundsOnElement(svgIdLayer->m_svgId);
		QPointF p(bounds.left() * size.width() / viewBox.width(), bounds.top() * size.height() / viewBox.height());
		QRectF r = connectorItem->rect();
		r.moveTo(p.x(), p.y());
		connectorItem->setRect(r);
	}
}


void PaletteItem::resetConnectors(ItemBase * otherLayer, FSvgRenderer * otherLayerRenderer)
{
	// there's only one connector
	foreach (Connector * connector, m_modelPart->connectors().values()) {
		if (connector == NULL) continue;

		connector->unprocess(m_viewID, m_viewLayerID);
		SvgIdLayer * svgIdLayer = connector->fullPinInfo(m_viewID, m_viewLayerID);
		if (svgIdLayer == NULL) continue;

		bool result = fsvgRenderer()->setUpConnector(svgIdLayer, false, viewLayerPlacement());
		if (!result) continue;

		resetConnector(this, svgIdLayer);
	}

	if (otherLayer) {
		foreach (Connector * connector, m_modelPart->connectors().values()) {
			if (connector == NULL) continue;

			connector->unprocess(m_viewID, otherLayer->viewLayerID());
			SvgIdLayer * svgIdLayer = connector->fullPinInfo(m_viewID, otherLayer->viewLayerID());
			if (svgIdLayer == NULL) continue;

			bool result = otherLayerRenderer->setUpConnector(svgIdLayer, false, viewLayerPlacement());
			if (!result) continue;

			resetConnector(otherLayer, svgIdLayer);
		}
	}


}

void PaletteItem::resetConnector(ItemBase * itemBase, SvgIdLayer * svgIdLayer) 
{
    QList<ConnectorItem *> already;
	foreach (ConnectorItem * connectorItem, itemBase->cachedConnectorItems()) {
		//DebugDialog::debug(QString("via set rect %1").arg(itemBase->viewID()), svgIdLayer->m_rect);

		connectorItem->setRect(svgIdLayer->rect(viewLayerPlacement()));
		connectorItem->setTerminalPoint(svgIdLayer->point(viewLayerPlacement()));
		connectorItem->setRadius(svgIdLayer->m_radius, svgIdLayer->m_strokeWidth);
        connectorItem->setIsPath(svgIdLayer->m_path);
		connectorItem->attachedMoved(false, already);
		break;
	}
}

bool PaletteItem::collectHoleSizeInfo(const QString & defaultHoleSizeValue, QWidget * parent, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget) 
{
	returnProp = tr("hole size");

	returnValue = m_modelPart->localProp("hole size").toString();
    if (returnValue.isEmpty()) {
        returnValue = defaultHoleSizeValue;
    }
	QWidget * frame = createHoleSettings(parent, m_holeSettings, swappingEnabled, returnValue, true);

	connect(m_holeSettings.sizesComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(changeHoleSize(const QString &)));	
	connect(m_holeSettings.mmRadioButton, SIGNAL(toggled(bool)), this, SLOT(changeUnits(bool)));
	connect(m_holeSettings.inRadioButton, SIGNAL(toggled(bool)), this, SLOT(changeUnits(bool)));
	connect(m_holeSettings.diameterEdit, SIGNAL(editingFinished()), this, SLOT(changeDiameter()));
	connect(m_holeSettings.thicknessEdit, SIGNAL(editingFinished()), this, SLOT(changeThickness()));

	returnWidget = frame;
	return true;
}

void PaletteItem::setUpHoleSizes(const QString & type, HoleClassThing & holeThing) 
{
	if (holeThing.holeSizes.count() == 0) {       
		setUpHoleSizesAux(holeThing, type);
	}

    initHoleSettings(m_holeSettings, &holeThing);
    QStringList localHoleSize = modelPart()->localProp("hole size").toString().split(",");
    if (localHoleSize.count() == 2) {
        m_holeSettings.ringThickness = localHoleSize.at(1);
        m_holeSettings.holeDiameter = localHoleSize.at(0);
    }
    else {
        QString hs = modelPart()->properties().value("hole size");
        localHoleSize = hs.split(",");
        if (localHoleSize.count() == 2) {
            modelPart()->setLocalProp("hole size", hs);
            m_holeSettings.ringThickness = localHoleSize.at(1);
            m_holeSettings.holeDiameter = localHoleSize.at(0);
        }
        else {
            m_holeSettings.ringThickness = holeThing.ringThickness;
            m_holeSettings.holeDiameter = holeThing.holeSize;
        }
    }
}
void PaletteItem::setUpHoleSizesAux(HoleClassThing & holeThing, const QString & type) {
	QFile file(":/resources/vias.xml");

	QString errorStr;
	int errorLine;
	int errorColumn;

	QDomDocument domDocument;
	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		DebugDialog::debug(QString("failed loading properties %1 line:%2 col:%3").arg(errorStr).arg(errorLine).arg(errorColumn));
		return;
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull()) return;
	if (root.tagName() != "vias") return;

	QDomElement ve = root.firstChildElement("via");
	while (!ve.isNull()) {
		QString rt = ve.attribute("ringthickness");
		QString hs = ve.attribute("holesize");
		QString name = ve.attribute("name");
        QString isOK = ve.attribute(type);
        if (isOK.toInt() == 1) {
		    if (ve.attribute(type + "default").compare("yes") == 0) {
			    if (holeThing.ringThickness.isEmpty()) {
				    holeThing.ringThickness = rt;
			    }
			    if (holeThing.holeSize.isEmpty()) {
				    holeThing.holeSize = hs;
			    }
		    }

            bool ok;
            double val = TextUtils::convertToInches(ve.attribute(type + "ringthicknesslow"), &ok, false);
            if (ok) {
                holeThing.ringThicknessRange.setX(val);
            }
            val = TextUtils::convertToInches(ve.attribute(type + "ringthicknesshigh"), &ok, false);
            if (ok) {
                holeThing.ringThicknessRange.setY(val);
            }
            val = TextUtils::convertToInches(ve.attribute(type + "holediameterlow"), &ok, false);
            if (ok) {
                holeThing.holeDiameterRange.setX(val);
            }
            val = TextUtils::convertToInches(ve.attribute(type + "holediameterhigh"), &ok, false);
            if (ok) {
                holeThing.holeDiameterRange.setY(val);
            }

		    holeThing.holeSizes.insert(name, QString("%1,%2").arg(hs).arg(rt));
            holeThing.holeSizeKeys.append(name);
        }
		ve = ve.nextSiblingElement("via");
	}

    holeThing.holeSizeValue = QString("%1,%2").arg(holeThing.holeSize).arg(holeThing.ringThickness);

}

QWidget * PaletteItem::createHoleSettings(QWidget * parent, HoleSettings & holeSettings, bool swappingEnabled, const QString & currentHoleSize, bool advanced) {
    static const int RowHeight = 21;

    holeSettings.diameterEdit = NULL;
	holeSettings.thicknessEdit = NULL;
	holeSettings.mmRadioButton = NULL;
	holeSettings.inRadioButton = NULL;
	holeSettings.diameterValidator = NULL;
	holeSettings.thicknessValidator = NULL;

    QFrame * frame = new QFrame(parent);
	frame->setObjectName("infoViewPartFrame");

	QVBoxLayout * vBoxLayout = new QVBoxLayout(frame);
	vBoxLayout->setMargin(0);
	vBoxLayout->setContentsMargins(0, 0, 0, 0);
	vBoxLayout->setSpacing(0);

	holeSettings.sizesComboBox = new QComboBox(frame);
	holeSettings.sizesComboBox->setEditable(false);
	holeSettings.sizesComboBox->setObjectName("infoViewComboBox");
    foreach (QString key, holeSettings.holeThing->holeSizeKeys) {
	    holeSettings.sizesComboBox->addItem(key);
    }
	holeSettings.sizesComboBox->setEnabled(swappingEnabled);

	vBoxLayout->addWidget(holeSettings.sizesComboBox);

    if (advanced) {
        vBoxLayout->addSpacing(4);

        QFrame * hFrame = new QFrame(frame);
        QHBoxLayout * hLayout = new QHBoxLayout(hFrame);
	    hLayout->setMargin(0);

	    QGroupBox * subFrame = new QGroupBox(tr("advanced settings"), frame);
	    subFrame->setObjectName("infoViewGroupBox");

	    QGridLayout * gridLayout = new QGridLayout(subFrame);
	    gridLayout->setMargin(0);

	    QGroupBox * rbFrame = new QGroupBox("", subFrame);
	    rbFrame->setObjectName("infoViewGroupBox");
	    QVBoxLayout * vbl = new QVBoxLayout(rbFrame);
	    vbl->setMargin(0);

	    holeSettings.inRadioButton = new QRadioButton(tr("in"), subFrame);
	    gridLayout->addWidget(holeSettings.inRadioButton, 0, 2);
	    holeSettings.inRadioButton->setObjectName("infoViewRadioButton");

	    holeSettings.mmRadioButton = new QRadioButton(tr("mm"), subFrame);
	    gridLayout->addWidget(holeSettings.mmRadioButton, 1, 2);
	    holeSettings.inRadioButton->setObjectName("infoViewRadioButton");

	    vbl->addWidget(holeSettings.inRadioButton);
	    vbl->addWidget(holeSettings.mmRadioButton);
	    rbFrame->setLayout(vbl);

	    gridLayout->addWidget(rbFrame, 0, 2, 2, 1, Qt::AlignVCenter);

	    holeSettings.diameterEdit = new QLineEdit(subFrame);
	    holeSettings.diameterEdit->setMinimumHeight(RowHeight);
	    holeSettings.diameterValidator = new QDoubleValidator(holeSettings.diameterEdit);
	    holeSettings.diameterValidator->setNotation(QDoubleValidator::StandardNotation);
	    holeSettings.diameterEdit->setValidator(holeSettings.diameterValidator);
	    gridLayout->addWidget(holeSettings.diameterEdit, 0, 1);
	    holeSettings.diameterEdit->setObjectName("infoViewLineEdit");

	    QLabel * label = new QLabel(tr("Hole Diameter"));
	    label->setMinimumHeight(RowHeight);
        label->setObjectName("infoViewGroupBoxLabel");
	    gridLayout->addWidget(label, 0, 0);

	    holeSettings.thicknessEdit = new QLineEdit(subFrame);
	    holeSettings.thicknessEdit->setMinimumHeight(RowHeight);
	    holeSettings.thicknessValidator = new QDoubleValidator(holeSettings.thicknessEdit);
	    holeSettings.thicknessValidator->setNotation(QDoubleValidator::StandardNotation);
	    holeSettings.thicknessEdit->setValidator(holeSettings.thicknessValidator);
	    gridLayout->addWidget(holeSettings.thicknessEdit, 1, 1);
	    holeSettings.thicknessEdit->setObjectName("infoViewLineEdit");

	    label = new QLabel(tr("Ring Thickness"));
	    label->setMinimumHeight(RowHeight);
	    gridLayout->addWidget(label, 1, 0);
	    label->setObjectName("infoViewLabel");

	    gridLayout->setContentsMargins(10, 2, 0, 2);
	    gridLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 3);
	    gridLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 3);

        hLayout->addWidget(subFrame);
        hLayout->addSpacerItem(new QSpacerItem(1,1,QSizePolicy::Expanding,QSizePolicy::Minimum));
        vBoxLayout->addWidget(hFrame);


	    holeSettings.mmRadioButton->setEnabled(swappingEnabled);
	    holeSettings.inRadioButton->setEnabled(swappingEnabled);
	    holeSettings.diameterEdit->setEnabled(swappingEnabled);
	    holeSettings.thicknessEdit->setEnabled(swappingEnabled);


	    if (currentHoleSize.contains("mm")) {
		    holeSettings.mmRadioButton->setChecked(true);
	    }
	    else {
		    holeSettings.inRadioButton->setChecked(true);
	    }
    }

	updateEditTexts(holeSettings);
	updateValidators(holeSettings);
	updateSizes(holeSettings);

	return frame;
}

void PaletteItem::updateEditTexts(HoleSettings & holeSettings) {
	if (holeSettings.diameterEdit == NULL) return;
	if (holeSettings.thicknessEdit == NULL) return;
	if (holeSettings.mmRadioButton == NULL) return;

	double hd = TextUtils::convertToInches(holeSettings.holeDiameter);
	double rt = TextUtils::convertToInches(holeSettings.ringThickness);

	QString newVal;
	if (holeSettings.currentUnits() == "in") {
		newVal = QString("%1,%2").arg(hd).arg(rt);
	}
	else {
		newVal = QString("%1,%2").arg(hd * 25.4).arg(rt * 25.4);
	}

	QStringList sizes = newVal.split(",");
	holeSettings.diameterEdit->setText(sizes.at(0));
	holeSettings.thicknessEdit->setText(sizes.at(1));
}

void PaletteItem::updateSizes(HoleSettings &  holeSettings) {
	if (holeSettings.sizesComboBox == NULL) return;

	int newIndex = -1;

	QPointF current(TextUtils::convertToInches(holeSettings.holeDiameter), TextUtils::convertToInches(holeSettings.ringThickness));
	for (int ix = 0; ix < holeSettings.sizesComboBox->count(); ix++) {
		QString key = holeSettings.sizesComboBox->itemText(ix);
		QString value = holeSettings.holeThing->holeSizes.value(key, "");
		QStringList sizes;
		if (value.isEmpty()) {
			sizes = key.split(",");
		}
		else {
			sizes = value.split(",");
		}
		if (sizes.count() < 2) continue;

		QPointF p(TextUtils::convertToInches(sizes.at(0)), TextUtils::convertToInches(sizes.at(1)));
		if (p == current) {
			newIndex = ix;
			break;
		}
	}

	if (newIndex < 0) {
		QString newItem = holeSettings.holeDiameter + "," + holeSettings.ringThickness;
		holeSettings.sizesComboBox->addItem(newItem);
		newIndex = holeSettings.sizesComboBox->findText(newItem);

		holeSettings.holeThing->holeSizes.insert(newItem, newItem);
        holeSettings.holeThing->holeSizeKeys.prepend(newItem);
	}

	// don't want to trigger another undo command
	bool wasBlocked = holeSettings.sizesComboBox->blockSignals(true);
	holeSettings.sizesComboBox->setCurrentIndex(newIndex);
	holeSettings.sizesComboBox->blockSignals(wasBlocked);
}

void PaletteItem::updateValidators(HoleSettings & holeSettings)
{
	if (holeSettings.diameterValidator == NULL) return;
	if (holeSettings.thicknessValidator == NULL) return;
	if (holeSettings.mmRadioButton == NULL) return;

	QString units = holeSettings.currentUnits();
	double multiplier = (units == "mm") ? 25.4 : 1.0;
	holeSettings.diameterValidator->setRange(holeSettings.holeThing->holeDiameterRange.x() * multiplier, holeSettings.holeThing->holeDiameterRange.y() * multiplier, 3);
	holeSettings.thicknessValidator->setRange(holeSettings.holeThing->ringThicknessRange.x() * multiplier, holeSettings.holeThing->ringThicknessRange.y() * multiplier, 3);
}

void PaletteItem::initHoleSettings(HoleSettings & holeSettings, HoleClassThing * holeThing) 
{
    holeSettings.holeThing = holeThing;
	holeSettings.diameterEdit = holeSettings.thicknessEdit = NULL;
	holeSettings.diameterValidator = holeSettings.thicknessValidator = NULL;
	holeSettings.inRadioButton = holeSettings.mmRadioButton = NULL;
	holeSettings.sizesComboBox = NULL;
}


bool PaletteItem::setHoleSize(QString & holeSize, bool force, HoleSettings & holeSettings)
{
	QStringList sizes = getSizes(holeSize, holeSettings);
	if (sizes.count() < 2) return false;

	if (!force && (holeSettings.holeDiameter.compare(sizes.at(0)) == 0) && (holeSettings.ringThickness.compare(sizes.at(1)) == 0)) 
	{
		return false;
	}

	holeSettings.holeDiameter = sizes.at(0);
	holeSettings.ringThickness = sizes.at(1);
	updateEditTexts(holeSettings);
	updateValidators(holeSettings);
	updateSizes(holeSettings);
	return true;
}

QStringList PaletteItem::getSizes(QString & holeSize, HoleSettings & holeSettings)
{
	QStringList sizes;
	QString hashedHoleSize = holeSettings.holeThing->holeSizes.value(holeSize);
	if (hashedHoleSize.isEmpty()) {
		sizes = holeSize.split(",");
	}
	else {
		sizes = hashedHoleSize.split(",");
		holeSize = sizes[0] + "," + sizes[1];
	}
	return sizes;
}

void PaletteItem::changeHoleSize(const QString & newSize) {
    if (this->m_viewID != ViewLayer::PCBView) {
        PaletteItem * paletteItem = qobject_cast<PaletteItem *>(modelPart()->viewItem(ViewLayer::PCBView));
        if (paletteItem == NULL) return;

        paletteItem->changeHoleSize(newSize);
        return;
    }

    QString holeSize = newSize;
    QStringList sizes = getSizes(holeSize, m_holeSettings);
    if (sizes.count() != 2) return;

    QString svg = hackSvgHoleSize(sizes.at(0), sizes.at(1));
    if (svg.isEmpty()) return;

    // figure out the new filename
    QString newModuleID = appendHoleSize(moduleID(), sizes.at(0), sizes.at(1));
    QString newFzpFilename = newModuleID + ".fzp"; 
    QString newSvgFilename = "pcb/" + newModuleID + ".svg";

    QString fzp = hackFzpHoleSize(newModuleID, newSvgFilename, sizes.at(0) + "," + sizes.at(1));    
    if (fzp.isEmpty()) return;
   
    if (!TextUtils::writeUtf8(PartFactory::fzpPath() + newFzpFilename, fzp)) {
        return;
    }

    if (!TextUtils::writeUtf8(PartFactory::partPath() + newSvgFilename, svg)) {
        return;
    }

    m_propsMap.insert("hole size", newSize);
    m_propsMap.insert("moduleID", newModuleID);

    InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
    if (infoGraphicsView != NULL) {
        infoGraphicsView->swap(family(), newModuleID, m_propsMap, this);
    }
}

QString PaletteItem::hackFzpHoleSize(const QString & fzp, const QString & moduleid, int hsix) 
{
    QString errorStr;
    int errorLine;
    int errorColumn;
    QDomDocument document;
    bool result = document.setContent(fzp, &errorStr, &errorLine, &errorColumn);
    if (!result) {
        DebugDialog::debug(QString("bad fzp in %1:%2").arg(moduleid).arg(fzp));
    }
    QStringList strings = moduleid.mid(hsix).split("_");
    return hackFzpHoleSize(document, moduleid, "pcb/" + moduleid + ".svg", strings.at(2) + "," + strings.at(3)); 
}


QString PaletteItem::hackFzpHoleSize(const QString & newModuleID, const QString & pcbFilename, const QString & newSize) {
    QFile file(modelPart()->path());
    QString errorStr;
    int errorLine;
    int errorColumn;
    QDomDocument document;
    bool result = document.setContent(&file, &errorStr, &errorLine, &errorColumn);    
    if (!result) {
        DebugDialog::debug(QString("bad doc fzp in %1:%2 %3 %4").arg(newModuleID).arg(errorStr).arg(errorLine).arg(errorColumn));
    }

    return hackFzpHoleSize(document, newModuleID, pcbFilename, newSize);
}

QString PaletteItem::hackFzpHoleSize(QDomDocument & document, const QString & newModuleID, const QString & pcbFilename, const QString & newSize) 
{
    QDomElement root = document.documentElement();
    root.setAttribute("moduleId", newModuleID);

    QDomElement views = root.firstChildElement("views");
    QDomElement pcbView = views.firstChildElement("pcbView");
    QDomElement layers = pcbView.firstChildElement("layers");
    if (layers.isNull()) return "";

    layers.setAttribute("image", pcbFilename);

    QDomElement properties = root.firstChildElement("properties");
    QDomElement prop = properties.firstChildElement("property");
    bool gotProp = false;
    while (!prop.isNull()) {
        QString name = prop.attribute("name");
        if (name.compare("hole size", Qt::CaseInsensitive) == 0) {
            gotProp = true;
            TextUtils::replaceChildText(prop, newSize);
            break;
        }

        prop = prop.nextSiblingElement("property");
    }

    if (!gotProp) return "";


    return TextUtils::removeXMLEntities(document.toString());
}


QString PaletteItem::hackSvgHoleSizeAux(const QString & svg, const QString & expectedFileName) 
{
    QDomDocument document;
    document.setContent(svg);
    QFileInfo info(expectedFileName);
    QString baseName = info.completeBaseName();
    int hsix = baseName.indexOf(HoleSizePrefix);
    QStringList strings = baseName.mid(hsix).split("_");
    return hackSvgHoleSize(document, strings.at(2), strings.at(3));
}

QString PaletteItem::hackSvgHoleSize(const QString & holeDiameter, const QString & ringThickness) {
    QFile file(filename());
    QString errorStr;
    int errorLine;
    int errorColumn;

    QDomDocument domDocument;
    if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		DebugDialog::debug(QString("unable to parse pcb svg xml: %1 %2 %3").arg(errorStr).arg(errorLine).arg(errorColumn));
		return "";
	}

    return hackSvgHoleSize(domDocument, holeDiameter, ringThickness);
}

QString PaletteItem::hackSvgHoleSize(QDomDocument & domDocument, const QString & holeDiameter, const QString & ringThickness) 
{
    QDomElement root = domDocument.documentElement();
    double w = TextUtils::convertToInches(root.attribute("width"));
    QStringList vb = root.attribute("viewBox").split(" ");
    if (vb.count() != 4) return "";

    double wp = vb.at(2).toDouble();
    if (wp == 0) return "";

    double dpi = wp / w;
    double rt = TextUtils::convertToInches(ringThickness) * dpi;
    double hs = TextUtils::convertToInches(holeDiameter) * dpi;
    double rad = (hs + rt) / 2;

    QDomNodeList circles = root.elementsByTagName("circle");
    for (int i = 0; i < circles.count(); i++) {
        QDomElement circle = circles.at(i).toElement();
        QString id = circle.attribute("id");
        if (ConnectorFinder.indexIn(id) == 0) {
            circle.setAttribute("r", QString::number(rad));
            circle.setAttribute("stroke-width", QString::number(rt));
        }
    }

    QDomNodeList rects = root.elementsByTagName("rect");
    for (int i = 0; i < rects.count(); i++) {
        QDomElement rect = rects.at(i).toElement();
        QString id = rect.attribute("id");
        if (id.compare("square") == 0) {
            double oldWidth = rect.attribute("width").toDouble();
            double oldX = rect.attribute("x").toDouble();
            double oldY = rect.attribute("y").toDouble();

            rect.setAttribute("width", QString::number(rad * 2));
            rect.setAttribute("height", QString::number(rad * 2));
            rect.setAttribute("x", QString::number(oldX + (oldWidth / 2) - rad));
            rect.setAttribute("y", QString::number(oldY + (oldWidth / 2) - rad));
            rect.setAttribute("stroke-width", QString::number(rt));
        }
    }


    return TextUtils::removeXMLEntities(domDocument.toString());
}

QString PaletteItem::appendHoleSize(const QString & moduleid, const QString & holeSize, const QString & ringThickness)
{
    QString baseName = moduleid;
    int ix = baseName.lastIndexOf(HoleSizePrefix);
    if (ix >= 0) {
        baseName.truncate(ix);
    }

    return baseName + QString("%1%2_%3").arg(HoleSizePrefix).arg(holeSize).arg(ringThickness);
}

void PaletteItem::generateSwap(const QString & text, GenModuleID genModuleID, GenFzp genFzp, GenSvg makeBreadboardSvg, GenSvg makeSchematicSvg, GenSvg makePcbSvg) 
{
    FamilyPropertyComboBox * comboBox = qobject_cast<FamilyPropertyComboBox *>(sender());
    if (comboBox == NULL) return;

    QMap<QString, QString> propsMap(m_propsMap);
    propsMap.insert(comboBox->prop(), text);
    QString newModuleID = genModuleID(propsMap);
    if (!newModuleID.contains("smd", Qt::CaseInsensitive)) {
        // add hole size
        int ix = moduleID().indexOf(HoleSizePrefix);
        if (ix >= 0) {
            newModuleID.append(moduleID().mid(ix));
        }
    }

    QString path;
    if (!PartFactory::fzpFileExists(newModuleID, path)) {
        QString fzp = genFzp(newModuleID);
        TextUtils::writeUtf8(path, fzp);

        QDomDocument doc;
        doc.setContent(fzp);

        QHash<QString, QString> viewNames;

        QDomElement root = doc.documentElement();
	    QDomElement views = root.firstChildElement("views");
        QDomElement view = views.firstChildElement();
	    while (!view.isNull()) {
            viewNames.insert(view.tagName(), view.attribute("image", ""));
            view = view.nextSiblingElement();
        }

        QString name = viewNames.value("breadboardView", "");
        if (!PartFactory::svgFileExists(name, path)) {
            QString svg = makeBreadboardSvg(name);
	        TextUtils::writeUtf8(path, svg);
        }

        name = viewNames.value("schematicView", "");
        if (!PartFactory::svgFileExists(name, path)) {
            QString svg = makeSchematicSvg(name);
	        TextUtils::writeUtf8(path, svg);
        }

        name = viewNames.value("pcbView", "");
        if (!PartFactory::svgFileExists(name, path)) {
            QString svg = makePcbSvg(name);
	        TextUtils::writeUtf8(path, svg);
        } 
    }

    m_propsMap.insert("moduleID", newModuleID);

}

void PaletteItem::changeUnits(bool) 
{
	QString newVal = changeUnits(m_holeSettings);
	changeHoleSize(newVal);
}

QString PaletteItem::changeUnits(HoleSettings & holeSettings) 
{
	double hd = TextUtils::convertToInches(holeSettings.holeDiameter);
	double rt = TextUtils::convertToInches(holeSettings.ringThickness);
	QString newVal;
	if (holeSettings.currentUnits() == "in") {
		newVal = QString("%1in,%2in").arg(hd).arg(rt);
	}
	else {
		newVal = QString("%1mm,%2mm").arg(hd * 25.4).arg(rt * 25.4);
	}

	QStringList sizes = newVal.split(",");
	holeSettings.ringThickness = sizes.at(1);
	holeSettings.holeDiameter = sizes.at(0);

	updateValidators(holeSettings);
	updateSizes(holeSettings);
	updateEditTexts(holeSettings);

	return newVal;
}

void PaletteItem::changeThickness() 
{
	if (changeThickness(m_holeSettings, sender())) {
		QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
		changeHoleSize(m_holeSettings.holeDiameter + "," + edit->text() + m_holeSettings.currentUnits());
	}	
}

bool PaletteItem::changeThickness(HoleSettings & holeSettings, QObject * sender) 
{
	QLineEdit * edit = qobject_cast<QLineEdit *>(sender);
	if (edit == NULL) return false;

	double newValue = edit->text().toDouble();
	QString temp = holeSettings.ringThickness;
	temp.chop(2);
	double oldValue = temp.toDouble();
	return (newValue != oldValue);
}

void PaletteItem::changeDiameter() 
{
	if (changeDiameter(m_holeSettings, sender())) {
		QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
		changeHoleSize(edit->text() + m_holeSettings.currentUnits() + "," + m_holeSettings.ringThickness);
	}
}

bool PaletteItem::changeDiameter(HoleSettings & holeSettings, QObject * sender) 
{
	QLineEdit * edit = qobject_cast<QLineEdit *>(sender);
	if (edit == NULL) return false;

	double newValue = edit->text().toDouble();
	QString temp = holeSettings.holeDiameter;
	temp.chop(2);
	double oldValue = temp.toDouble();
	return (newValue != oldValue);
}

bool PaletteItem::makeLocalModifications(QByteArray & svg, const QString & filename) {
    // a bottleneck for modifying part svg xml at setupImage time

    // for saved-as-new-part parts (i.e. that are no longer MysteryParts) that still have a chip-label or custom pin names
    // also handles adding a title if there is a label id in the 
    switch (m_viewID) {
        case ViewLayer::PCBView:
            return false;
            
        default:
            if (itemType() != ModelPart::Part) return false;
            if (filename.startsWith("icon")) return false;

            break;
    }

    bool gotChipLabel = false;
    QString chipLabel = modelPart()->properties().value("chip label", ""); 
    if (!chipLabel.isEmpty()) {
        svg = TextUtils::replaceTextElement(svg, "label", chipLabel);
        gotChipLabel = true;
    }

    bool modified = false;
    if (m_viewID == ViewLayer::SchematicView) {
        QString value = modelPart()->properties().value("editable pin labels", "");
        if (value.compare("true") == 0) {
            bool hasLayout, sip;
            QStringList labels = sipOrDipOrLabels(hasLayout, sip);
            if (labels.count() > 0) {
                svg = PartFactory::makeSchematicSipOrDipOr(labels, hasLayout, sip).toUtf8();
                modified = true;
            }
        }
        gotChipLabel = true;
    }

    if (gotChipLabel) return modified;

    int rix = svg.indexOf("label");
    if (rix >= 0) {
        rix = qMax(0, rix - 4);     // backup for id="
        int ix = svg.indexOf("id=\"label\"", rix);
        if (ix < 0) {
            ix = svg.indexOf("id='label'", rix);
        }
        if (ix >= 0) {
            int tix = svg.lastIndexOf("<text", ix);
            int lix = svg.lastIndexOf("<", ix);
            if (tix == lix) {
                svg = TextUtils::replaceTextElement(svg, "label", modelPart()->title());
                modified = true;
            }
        }
    }

    return modified;
}

QStringList PaletteItem::sipOrDipOrLabels(bool & hasLayout, bool & sip) {
	hasLayout = sip = false;
    bool hasLocal = false;
	QStringList labels = getPinLabels(hasLocal);
	if (labels.count() == 0) return labels;

	// part was formerly a mystery part or generic ic ...
	QHash<QString, QString> properties = modelPart()->properties();
	foreach (QString key, properties.keys()) {
		QString value = properties.value(key);
		if (key.compare("layout", Qt::CaseInsensitive) == 0) {
			// was a mystery part
			hasLayout = true;
			break;
		}

		if (key.compare("package") == 0) {
			// was a generic ic
			sip = value.contains("sip", Qt::CaseInsensitive);		
		}
	}

    return labels;
}

void PaletteItem::resetLayerKin(const QString & svg) {
    QString svgNoText = SvgFileSplitter::hideText3(svg);

	resetRenderer(svgNoText);

    foreach (ItemBase * lkpi, layerKin()) {
        if (lkpi->viewLayerID() == ViewLayer::SchematicText) {
            bool hasText;
	        QString svgText = SvgFileSplitter::showText3(svg, hasText);
	        lkpi->resetRenderer(svgText);
            lkpi->setProperty("textSvg", svgText);
            qobject_cast<SchematicTextLayerKinPaletteItem *>(lkpi)->clearTextThings();
            break;
        }
    }
}

QTransform PaletteItem::untransform() {
    //DebugDialog::debug("untransform");
    QTransform chiefTransform = this->transform();
    chiefTransform.setMatrix(chiefTransform.m11(), chiefTransform.m12(), chiefTransform.m13(), chiefTransform.m21(), chiefTransform.m22(), chiefTransform.m23(), 0, 0, chiefTransform.m33()); 
    bool identity = chiefTransform.isIdentity();
    if (!identity) {    
        QTransform invert = chiefTransform.inverted();
        transformItem(invert, false);
        foreach (ItemBase * lkpi, layerKin()) {    
            lkpi->transformItem(invert, false);
        }
    }

    return chiefTransform;
}

void PaletteItem::retransform(const QTransform & chiefTransform) {
    //DebugDialog::debug("retransform");
    if (!chiefTransform.isIdentity()) {
        transformItem(chiefTransform, false);
        foreach (ItemBase * lkpi, layerKin()) {    
            lkpi->transformItem(chiefTransform, false);
        }
    }
}


