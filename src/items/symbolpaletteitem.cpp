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

#include "symbolpaletteitem.h"
#include "../debugdialog.h"
#include "../connectors/connectoritem.h"
#include "../connectors/bus.h"
#include "moduleidnames.h"
#include "../fsvgrenderer.h"
#include "../utils/textutils.h"
#include "../utils/focusoutcombobox.h"
#include "../utils/graphicsutils.h"
#include "../sketch/infographicsview.h"
#include "partlabel.h"
#include "partfactory.h"
#include "layerkinpaletteitem.h"
#include "../svg/svgfilesplitter.h"

#include <QLineEdit>
#include <QMultiHash>
#include <QMessageBox>

#define VOLTAGE_HASH_CONVERSION 1000000
#define FROMVOLTAGE(v) ((long) (v * VOLTAGE_HASH_CONVERSION))

static QMultiHash<long, QPointer<ConnectorItem> > LocalVoltages;			// Qt doesn't do Hash keys with double
static QMultiHash<QString, QPointer<ConnectorItem> > LocalNetLabels;			
static QList< QPointer<ConnectorItem> > LocalGrounds;
static QList<double> Voltages;
double SymbolPaletteItem::DefaultVoltage = 5;

/////////////////////////////////////////////////////

/*
FocusBugLineEdit::FocusBugLineEdit(QWidget * parent) : QLineEdit(parent)
{
	connect(this, SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot()));
    m_lastEditingFinishedEmit = QTime::currentTime();
}

FocusBugLineEdit::~FocusBugLineEdit()
{
}

void FocusBugLineEdit::editingFinishedSlot() {
    QTime now = QTime::currentTime();
    int d = m_lastEditingFinishedEmit.msecsTo(now);
    DebugDialog::debug(QString("dtime %1").arg(d));
    if (d < 1000) {
        return;
    }

    m_lastEditingFinishedEmit = now;
    emit safeEditingFinished();
}
*/

////////////////////////////////////////

SymbolPaletteItem::SymbolPaletteItem( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: PaletteItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	if (Voltages.count() == 0) {
		Voltages.append(0.0);
		Voltages.append(3.3);
		Voltages.append(5.0);
		Voltages.append(12.0);
	}

	m_connector0 = m_connector1 = NULL;
	m_voltage = 0;
	m_voltageReference = (modelPart->properties().value("type").compare("voltage reference") == 0);

	if (modelPart->moduleID().endsWith(ModuleIDNames::NetLabelModuleIDName)) {
	    m_isNetLabel = true;
    }
	else {
		m_isNetLabel = modelPart->moduleID().endsWith(ModuleIDNames::PowerLabelModuleIDName);

		bool ok;
		double temp = modelPart->localProp("voltage").toDouble(&ok);
		if (ok) {
			m_voltage = temp;
		}
		else {
			temp = modelPart->properties().value("voltage").toDouble(&ok);
			if (ok) {
				m_voltage = SymbolPaletteItem::DefaultVoltage;
			}
			modelPart->setLocalProp("voltage", m_voltage);
		}
		if (!Voltages.contains(m_voltage)) {
			Voltages.append(m_voltage);
		}
	}
}

SymbolPaletteItem::~SymbolPaletteItem() {
	if (m_isNetLabel) {
		foreach (QString key, LocalNetLabels.uniqueKeys()) {
			if (m_connector0) {
				LocalNetLabels.remove(key, m_connector0);
			}
			if (m_connector1) {
				LocalNetLabels.remove(key, m_connector1);
			}
			LocalNetLabels.remove(key, NULL);		// cleans null QPointers
		}
	}
	else {
		if (m_connector0) LocalGrounds.removeOne(m_connector0);
		if (m_connector1) LocalGrounds.removeOne(m_connector1);
		LocalGrounds.removeOne(NULL);   // cleans null QPointers

		foreach (long key, LocalVoltages.uniqueKeys()) {
			if (m_connector0) {
				LocalVoltages.remove(key, m_connector0);
			}
			if (m_connector1) {
				LocalVoltages.remove(key, m_connector1);
			}
			LocalVoltages.remove(key, NULL);		// cleans null QPointers
		}
	}
}

void SymbolPaletteItem::removeMeFromBus(double v) {
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		if (m_isNetLabel) {
			LocalNetLabels.remove(getLabel(), connectorItem);
		}
		else {
			double nv = useVoltage(connectorItem);
			if (nv == v) {
				//connectorItem->debugInfo(QString("remove %1").arg(useVoltage(connectorItem)));

				bool gotOne = LocalGrounds.removeOne(connectorItem);
				int count = LocalVoltages.remove(FROMVOLTAGE(v), connectorItem);
				LocalVoltages.remove(FROMVOLTAGE(v), NULL);


				if (count == 0 && !gotOne) {
					DebugDialog::debug(QString("removeMeFromBus failed %1 %2 %3 %4")
						.arg(this->id())
						.arg(connectorItem->connectorSharedID())
						.arg(v).arg(nv));
				}
			}
		}
	}
	LocalGrounds.removeOne(NULL);  // keep cleaning these out
}

ConnectorItem* SymbolPaletteItem::newConnectorItem(Connector *connector) 
{
	ConnectorItem * connectorItem = PaletteItemBase::newConnectorItem(connector);

	if (connector->connectorSharedID().compare("connector0") == 0) {
		m_connector0 = connectorItem;
	}
	else if (connector->connectorSharedID().compare("connector1") == 0) {
		m_connector1 = connectorItem;
	}
	else {
		return connectorItem;
	}

	if (m_isNetLabel) {
		LocalNetLabels.insert(getLabel(), connectorItem);
	}
	else if (connectorItem->isGrounded()) {
		LocalGrounds.append(connectorItem);
		//connectorItem->debugInfo("new ground insert");
	}
	else {
		LocalVoltages.insert(FROMVOLTAGE(useVoltage(connectorItem)), connectorItem);
		//connectorItem->debugInfo(QString("new voltage insert %1").arg(useVoltage(connectorItem)));
	}
	return connectorItem;
}

void SymbolPaletteItem::busConnectorItems(Bus * bus, ConnectorItem * fromConnectorItem, QList<class ConnectorItem *> & items) {
	if (bus == NULL) return;

	PaletteItem::busConnectorItems(bus, fromConnectorItem, items);

	//foreach (ConnectorItem * bc, items) {
		//bc->debugInfo(QString("bc %1").arg(bus->id()));
	//}

	QList< QPointer<ConnectorItem> > mitems;
	if (m_isNetLabel) {
		mitems.append(LocalNetLabels.values(getLabel()));
	}
	else if (bus->id().compare("groundbus", Qt::CaseInsensitive) == 0) {
		mitems.append(LocalGrounds);
	}
	else {
		mitems.append(LocalVoltages.values(FROMVOLTAGE(m_voltage)));
	}
	foreach (ConnectorItem * connectorItem, mitems) {
		if (connectorItem == NULL) continue;

		if (connectorItem->scene() == this->scene()) {
			items.append(connectorItem);
			//connectorItem->debugInfo(QString("symbol bus %1").arg(bus->id()));
		}
	}
}

double SymbolPaletteItem::voltage() {
	return m_voltage;
}

void SymbolPaletteItem::setProp(const QString & prop, const QString & value) {
	if (prop.compare("voltage", Qt::CaseInsensitive) == 0) {
		setVoltage(value.toDouble());
		return;
	}
	if (prop.compare("label", Qt::CaseInsensitive) == 0 && m_isNetLabel) {
		setLabel(value);
		return;
	}

	PaletteItem::setProp(prop, value);
}

void SymbolPaletteItem::setLabel(const QString & label) {
	removeMeFromBus(0);

	m_modelPart->setLocalProp("label", label);

	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		LocalNetLabels.insert(label, connectorItem);
	}

    QTransform  transform = untransform();

	QString svg = makeSvg(this->viewLayerID());
	resetRenderer(svg);
    resetLayerKin();
    resetConnectors(NULL, NULL);

    retransform(transform);
}

void SymbolPaletteItem::setVoltage(double v) {
	removeMeFromBus(m_voltage);

	m_voltage = v;
	m_modelPart->setLocalProp("voltage", v);
	if (!Voltages.contains(v)) {
		Voltages.append(v);
	}

    if (m_isNetLabel) {
	    foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		    LocalNetLabels.insert(QString::number(m_voltage), connectorItem);
	    }
    }
    else {
	    foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		    if (connectorItem->isGrounded()) {
			    LocalGrounds.append(connectorItem);
			    //connectorItem->debugInfo("ground insert");

		    }
		    else {
			    LocalVoltages.insert(FROMVOLTAGE(v), connectorItem);
			    //connectorItem->debugInfo(QString("voltage insert %1").arg(useVoltage(connectorItem)));
		    }
	    }
    }

    if (m_viewID == ViewLayer::SchematicView) {
        if (m_voltageReference || m_isNetLabel) {
            QTransform transform = untransform();
	        QString svg = makeSvg(viewLayerID());
	        reloadRenderer(svg, false);

            resetLayerKin();

            retransform(transform);

            if (m_partLabel) m_partLabel->displayTextsIf();
        }
    }
}

QString SymbolPaletteItem::makeSvg(ViewLayer::ViewLayerID viewLayerID) {
	QString path = filename();
	QFile file(filename());
	QString svg;
	if (file.open(QFile::ReadOnly)) {
		svg = file.readAll();
		file.close();
		svg = replaceTextElement(svg);
        if (viewLayerID == ViewLayer::SchematicText) {
            bool hasText;
            return SvgFileSplitter::showText3(svg, hasText);
        }
        else {
            return SvgFileSplitter::hideText3(svg);
        }
	}

	return "";
}

QString SymbolPaletteItem::replaceTextElement(QString svg) {
	double v = ((int) (m_voltage * 1000)) / 1000.0;
	return TextUtils::replaceTextElement(svg, "label", QString::number(v) + "V");
}

QString SymbolPaletteItem::getProperty(const QString & key) {
	if (key.compare("voltage", Qt::CaseInsensitive) == 0) {
		return QString::number(m_voltage);
	}

	return PaletteItem::getProperty(key);
}

double SymbolPaletteItem::useVoltage(ConnectorItem * connectorItem) {
	return (connectorItem->connectorSharedName().compare("GND", Qt::CaseInsensitive) == 0) ? 0 : m_voltage;
}

ConnectorItem * SymbolPaletteItem::connector0() {
	return m_connector0;
}

ConnectorItem * SymbolPaletteItem::connector1() {
	return m_connector1;
}

void SymbolPaletteItem::addedToScene(bool temporary)
{
	if (this->scene()) {
		setVoltage(m_voltage);
	}

    return PaletteItem::addedToScene(temporary);
}

QString SymbolPaletteItem::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor) 
{
	QString svg = PaletteItem::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
	if (m_voltageReference) {
		switch (viewLayerID) {
			case ViewLayer::Schematic:
				return replaceTextElement(svg);
			default:
				break;
		}
	}

	return svg; 
}

bool SymbolPaletteItem::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide)
{
	if ((prop.compare("voltage", Qt::CaseInsensitive) == 0) && 
		(moduleID().compare(ModuleIDNames::GroundModuleIDName) != 0)) 
	{
		FocusOutComboBox * edit = new FocusOutComboBox(parent);
		edit->setEnabled(swappingEnabled);
		int ix = 0;
		foreach (double v, Voltages) {
			edit->addItem(QString::number(v));
			if (v == m_voltage) {
				edit->setCurrentIndex(ix);
			}
			ix++;
		}

		QDoubleValidator * validator = new QDoubleValidator(edit);
		validator->setRange(-9999.99, 9999.99, 2);
        validator->setLocale(QLocale::C);
        validator->setNotation(QDoubleValidator::StandardNotation);
        validator->setLocale(QLocale::C);
		edit->setValidator(validator);

		edit->setObjectName("infoViewComboBox");


		connect(edit, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(voltageEntry(const QString &)));
		returnWidget = edit;	

		returnValue = m_voltage;
		returnProp = tr("voltage");
		return true;
	}

	if (prop.compare("label", Qt::CaseInsensitive) == 0 && m_isNetLabel) 
	{
		QLineEdit * edit = new QLineEdit(parent);
		edit->setEnabled(swappingEnabled);
		edit->setText(getLabel());
		edit->setObjectName("infoViewLineEdit");

		connect(edit, SIGNAL(editingFinished()), this, SLOT(labelEntry()));
		returnWidget = edit;	

		returnValue = getLabel();
		returnProp = tr("label");
		return true;
	}

	return PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}

void SymbolPaletteItem::voltageEntry(const QString & text) {
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setVoltage(text.toDouble(), true);
	}
}

void SymbolPaletteItem::labelEntry() {
	QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == NULL) return;

	QString current = getLabel();
	if (edit->text().compare(current) == 0) return;

    if (edit->text().isEmpty()) {
		QMessageBox::warning(NULL, tr("Net labels"), tr("Net labels cannot be blank"));
        return;
    }

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setProp(this, "label", ItemBase::TranslatedPropertyNames.value("label"), current, edit->text(), true);
	}
}

ItemBase::PluralType SymbolPaletteItem::isPlural() {
	return Singular;
}

bool SymbolPaletteItem::hasPartNumberProperty()
{
	return false;
}

ViewLayer::ViewID SymbolPaletteItem::useViewIDForPixmap(ViewLayer::ViewID vid, bool) 
{
    if (vid == ViewLayer::SchematicView) {
        return ViewLayer::IconView;
    }

    return ViewLayer::UnknownView;
}

bool SymbolPaletteItem::hasPartLabel() {
	return !m_isNetLabel;
}

bool SymbolPaletteItem::isOnlyNetLabel() {
	return false;
}

QString SymbolPaletteItem::getLabel() {
    if (m_voltageReference) {
        return QString::number(m_voltage);
    }
    return  modelPart()->localProp("label").toString();
}

QString SymbolPaletteItem::getDirection() {
    return  modelPart()->localProp("direction").toString();
}

void SymbolPaletteItem::setAutoroutable(bool ar) {
	m_viewGeometry.setAutoroutable(ar);
}

bool SymbolPaletteItem::getAutoroutable() {
	return m_viewGeometry.getAutoroutable();
}

void SymbolPaletteItem::resetLayerKin() {

    foreach (ItemBase * lkpi, layerKin()) {
        if (lkpi->viewLayerID() == ViewLayer::SchematicText) {
	        QString svg = makeSvg(lkpi->viewLayerID());
	        lkpi->resetRenderer(svg);
            lkpi->setProperty("textSvg", svg);
            qobject_cast<SchematicTextLayerKinPaletteItem *>(lkpi)->clearTextThings();
            break;
        }
    }
}

///////////////////////////////////////////////////////////////

NetLabel::NetLabel( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: SymbolPaletteItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	QString label = getLabel();
	if (label.isEmpty()) {
		label = modelPart->properties().value("label");
		if (label.isEmpty()) {
			label = tr("net label");
		}
		modelPart->setLocalProp("label", label);
	}
    setInstanceTitle(label, true);

    // direction is now obsolete, new netlabels use flip
	QString direction = getDirection();
	if (direction.isEmpty()) {
		direction = modelPart->properties().value("direction");
		if (direction.isEmpty()) {
			direction = modelPart->moduleID().contains("left", Qt::CaseInsensitive) ? "left" : "right";
		}
		modelPart->setLocalProp("direction", direction);
	}
}

NetLabel::~NetLabel() {
}

QString NetLabel::makeSvg(ViewLayer::ViewLayerID viewLayerID) {

    DebugDialog::debug("moduleid " + this->moduleID());
    double divisor = moduleID().contains(PartFactory::OldSchematicPrefix) ? 1 : 3;

    double labelFontSize = 200 /divisor;
    double totalHeight = 300 / divisor;
    double arrowWidth = totalHeight / 2;
    double strokeWidth = 10 / divisor;
    double halfStrokeWidth = strokeWidth / 2;
    double labelOffset = 20 / divisor;
    double labelBaseLine = 220 / divisor;

    QFont font("Droid Sans", labelFontSize * 72 / GraphicsUtils::StandardFritzingDPI, QFont::Normal);
	QFontMetricsF fm(font);
    double textWidth = fm.width(getLabel()) * GraphicsUtils::StandardFritzingDPI / 72;
    double totalWidth = textWidth + arrowWidth + labelOffset;

	QString header("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
					"<svg xmlns:svg='http://www.w3.org/2000/svg' xmlns='http://www.w3.org/2000/svg' version='1.2' baseProfile='tiny' \n"
					"width='%1in' height='%2in' viewBox='0 0 %3 %4' >\n"
					"<g id='%5' >\n"
                    );

    bool goLeft = (getDirection() == "left");  // direction is now obsolete; this is left over from 0.7.12 and earlier
    double offset = goLeft ? arrowWidth : 0;

    QString svg = header.arg(totalWidth / 1000)
            .arg(totalHeight / 1000)
            .arg(totalWidth)
            .arg(totalHeight)
            .arg(ViewLayer::viewLayerXmlNameFromID(viewLayerID))
            ;

    if (viewLayerID == ViewLayer::SchematicText) {
        svg += QString("<text id='label' x='%1' y='%2' fill='#000000' font-family='Droid Sans' font-size='%3'>%4</text>\n")
                .arg(labelOffset + offset)
                .arg(labelBaseLine)
                .arg(labelFontSize)
                .arg(getLabel());
    }
    else {
        QString pin = QString("<rect id='connector0pin' x='%1' y='%2' width='%3' height='%4' fill='none' stroke='none' stroke-width='0' />\n");
        QString terminal = QString("<rect id='connector0terminal' x='%1' y='%2' width='0.1' height='0.1' fill='none' stroke='none' stroke-width='0' />\n");

        QString points = QString("%1,%2 %3,%4 %5,%4 %5,%6 %3,%6");
        if (goLeft) {
            points = points.arg(halfStrokeWidth).arg(totalHeight / 2)
                            .arg(arrowWidth).arg(halfStrokeWidth)
                            .arg(totalWidth - halfStrokeWidth).arg(totalHeight - halfStrokeWidth);
            terminal = terminal.arg(0).arg(totalHeight / 2);
            pin = pin.arg(0).arg(0).arg(arrowWidth).arg(totalHeight);
        }
        else {
            points = points.arg(totalWidth - halfStrokeWidth).arg(totalHeight / 2)
                            .arg(totalWidth - arrowWidth).arg(halfStrokeWidth)
                            .arg(halfStrokeWidth).arg(totalHeight - halfStrokeWidth);
            terminal = terminal.arg(totalWidth).arg(totalHeight / 2);
            pin = pin.arg(totalWidth - arrowWidth - 0.1).arg(0).arg(arrowWidth).arg(totalHeight);
        }

        svg += pin;
        svg += terminal;
        svg += QString("<polygon fill='white' stroke='#000000' stroke-width='%1' points='%2' />\n")
                        .arg(strokeWidth)
                        .arg(points);
    }

    svg += "</g>\n</svg>\n";
 
    return svg;
}

void NetLabel::addedToScene(bool temporary)
{
	if (this->scene() && m_viewID == ViewLayer::SchematicView) {
        // do not understand why plan setLabel() doesn't work the same as the Mystery Part setChipLabel() in addedToScene()

        if (!this->transform().isIdentity()) {
            // need to establish correct bounding rect here or setLabel will screw up alignment
            // this seems to solve half of the cases
	        QString svg = makeSvg(this->viewLayerID());
	        resetRenderer(svg);
            resetLayerKin();
            QRectF r = boundingRect();
            //debugInfo(QString("added to scene %1,%2").arg(r.width()).arg(r.height()));


            QTransform chiefTransform = this->transform();
            QTransform local;
            local.setMatrix(chiefTransform.m11(), chiefTransform.m12(), chiefTransform.m13(), chiefTransform.m21(), chiefTransform.m22(), chiefTransform.m23(), 0, 0, chiefTransform.m33()); 
            if (!local.isIdentity()) {    
                double x = r.width() / 2.0;
	            double y = r.height() / 2.0;
	            QTransform transf = QTransform().translate(-x, -y) * local * QTransform().translate(x, y);
                if (qAbs(chiefTransform.dx() - transf.dx()) > .01 || qAbs(chiefTransform.dy() - transf.dy()) > .01) {
                    DebugDialog::debug("got the translation bug here");
                    this->getViewGeometry().setTransform(transf);
                    this->setTransform2(transf);
                }
            }
        }

		setLabel(getLabel());

	}

    // deliberately skip SymbolPaletteItem::addedToScene()
    return PaletteItem::addedToScene(temporary);
}

QString NetLabel::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor) {
    Q_UNUSED(svgHash);
    QString svg = makeSvg(viewLayerID);
    return PaletteItemBase::normalizeSvg(svg, viewLayerID, blackOnly, dpi, factor);
}

ItemBase::PluralType NetLabel::isPlural() {
	return Plural;
}

bool NetLabel::isOnlyNetLabel() {
	return true;
}

QString NetLabel::getInspectorTitle() {
   return getLabel();

}

void NetLabel::setInspectorTitle(const QString & oldText, const QString & newText) {
    InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
    if (infoGraphicsView == NULL) return;

    infoGraphicsView->setProp(this, "label", ItemBase::TranslatedPropertyNames.value("label"), oldText, newText, true);
}

