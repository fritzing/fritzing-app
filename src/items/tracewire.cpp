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

$Revision: 6984 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-22 23:44:56 +0200 (Mo, 22. Apr 2013) $

********************************************************************/

#include "tracewire.h"
#include "../sketch/infographicsview.h"
#include "../connectors/connectoritem.h"
#include "../utils/focusoutcombobox.h"


#include <QComboBox>

const int TraceWire::MinTraceWidthMils = 8;
const int TraceWire::MaxTraceWidthMils = 128;

/////////////////////////////////////////////////////////

TraceWire::TraceWire( ModelPart * modelPart, ViewLayer::ViewID viewID,  const ViewGeometry & viewGeometry, long id, QMenu * itemMenu  ) 
	: ClipableWire(modelPart, viewID,  viewGeometry,  id, itemMenu, true)
{
	m_canChainMultiple = true;
	m_wireDirection = TraceWire::NoDirection;
}


TraceWire::~TraceWire() 	
{
}

QComboBox * TraceWire::createWidthComboBox(double m, QWidget * parent) 
{
	QComboBox * comboBox = new FocusOutComboBox(parent);  // new QComboBox(parent);
	comboBox->setEditable(true);
	QIntValidator * intValidator = new QIntValidator(comboBox);
	intValidator->setRange(MinTraceWidthMils, MaxTraceWidthMils);
	comboBox->setValidator(intValidator);
    comboBox->setToolTip(tr("Select from the dropdown, or type in any value from %1 to %2").arg(MinTraceWidthMils).arg(MaxTraceWidthMils));

	int ix = 0;
	if (!Wire::widths.contains(m)) {
		Wire::widths.append(m);
		qSort(Wire::widths.begin(), Wire::widths.end());
	}
	foreach(long widthValue, Wire::widths) {
		QString widthName = Wire::widthTrans.value(widthValue, "");
		QVariant val((int) widthValue);
		comboBox->addItem(widthName.isEmpty() ? QString::number(widthValue) : widthName, val);
		if (qAbs(m - widthValue) < .01) {
			comboBox->setCurrentIndex(ix);
		}
		ix++;
	}

	return comboBox;

}


bool TraceWire::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide)
{
	if (prop.compare("width", Qt::CaseInsensitive) == 0) {
        if (viewID() != ViewLayer::PCBView) {
            // only in pcb view for now
            hide = true;
            return false;       
        }

		returnProp = tr("width");
		QComboBox * comboBox = createWidthComboBox(mils(), parent);
		comboBox->setEnabled(swappingEnabled);
		comboBox->setObjectName("infoViewComboBox");

		connect(comboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(widthEntry(const QString &)));
		returnWidget = comboBox;
		returnValue = comboBox->currentText();

		return true;
	}

	bool result =  ClipableWire::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
    if (prop.compare("layer") == 0 && returnWidget != NULL) {
        bool disabled = !canSwitchLayers();
        if (!disabled) {
            InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
            if (infoGraphicsView == NULL || infoGraphicsView->boardLayers() == 1) disabled = true;
        }
        returnWidget->setDisabled(disabled);
    }

    return result;


    return result;
}


void TraceWire::widthEntry(const QString & text) {

	int w = widthEntry(text, sender());
	if (w == 0) return;

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->changeWireWidthMils(QString::number(w));
	}
}

int TraceWire::widthEntry(const QString & text, QObject * sender) {

	QComboBox * comboBox = qobject_cast<QComboBox *>(sender);
	if (comboBox == NULL) return 0;

	int w = comboBox->itemData(comboBox->currentIndex()).toInt();
	if (w == 0) {
		// user typed in a number
		w = text.toInt();
	}
	if (!Wire::widths.contains(w)) {
		Wire::widths.append(w);
		qSort(Wire::widths.begin(), Wire::widths.end());
	}

	return w;
}

void TraceWire::setColorFromElement(QDomElement & element) {
	switch (m_viewLayerID) {
		case ViewLayer::Copper0Trace:
			element.setAttribute("color", ViewLayer::Copper0WireColor);
			break;
		case ViewLayer::Copper1Trace:
			element.setAttribute("color", ViewLayer::Copper1WireColor);
			break;
		case ViewLayer::SchematicTrace:
			//element.setAttribute("color", "#000000");
		default:
			break;
	}

	Wire::setColorFromElement(element);	
}

bool TraceWire::canSwitchLayers() {
	QList<Wire *> wires;
	QList<ConnectorItem *> ends;
	collectChained(wires, ends);

	foreach (ConnectorItem * end, ends) {
		if (end->getCrossLayerConnectorItem() == NULL) return false;
	}

	return true;
}

void TraceWire::setWireDirection(TraceWire::WireDirection wireDirection) {
	m_wireDirection = wireDirection;
}

TraceWire::WireDirection TraceWire::wireDirection() {
	return m_wireDirection;
}

TraceWire * TraceWire::getTrace(ConnectorItem * connectorItem)
{
	return qobject_cast<TraceWire *>(connectorItem->attachedTo());
}

void TraceWire::setSchematic(bool schematic) {
	m_viewGeometry.setSchematicTrace(schematic);
}

QHash<QString, QString> TraceWire::prepareProps(ModelPart * modelPart, bool wantDebug, QStringList & keys) 
{
    QHash<QString, QString> props = ClipableWire::prepareProps(modelPart, wantDebug, keys);

    if (m_viewID != ViewLayer::PCBView) return props;
    if (!m_viewGeometry.getPCBTrace()) return props;

    keys.append("layer");
    props.insert("layer", ViewLayer::topLayers().contains(m_viewLayerID) ? "top" : "bottom");
    return props;
}

QStringList TraceWire::collectValues(const QString & family, const QString & prop, QString & value) {
    if (prop.compare("layer") == 0) {
        QStringList values = ClipableWire::collectValues(family, prop, value);
        if (values.count() == 0) {
            values << TranslatedPropertyNames.value("bottom") << TranslatedPropertyNames.value("top");
            if (ViewLayer::bottomLayers().contains(m_viewLayerID)) {
                value = values.at(0);
            }
            else {
                value = values.at(1);
            }
        }
        return values;
    }

    return ClipableWire::collectValues(family, prop, value);
}
