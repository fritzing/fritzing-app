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

#include "resistor.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../utils/focusoutcombobox.h"
#include "../utils/boundedregexpvalidator.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../svg/svgfilesplitter.h"
#include "../commands.h"
#include "../layerattributes.h"
#include "moduleidnames.h"
#include "partlabel.h"
#include "../debugdialog.h"

#include <qmath.h>
#include <QRegExpValidator>

static QStringList Resistances;
static QHash<QString, QString> PinSpacings;
static QHash<int, QColor> ColorBands;
static QString OhmSymbol(QChar(0x03A9));
static QString PlusMinusSymbol(QChar(0x0B1));
static QHash<QString, QColor> Tolerances;

// TODO
//	save into parts bin
//	other manifestations of "220"?

Resistor::Resistor( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: Capacitor(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	m_changingPinSpacing = false;
	if (Resistances.count() == 0) {
		Resistances 
		 << QString("0") + OhmSymbol
		 << QString("1") + OhmSymbol << QString("1.5") + OhmSymbol << QString("2.2") + OhmSymbol << QString("3.3") + OhmSymbol << QString("4.7") + OhmSymbol << QString("6.8") + OhmSymbol
		 << QString("10") + OhmSymbol << QString("15") + OhmSymbol << QString("22") + OhmSymbol << QString("33") + OhmSymbol << QString("47") + OhmSymbol << QString("68") + OhmSymbol
		 << QString("100") + OhmSymbol << QString("150") + OhmSymbol << QString("220") + OhmSymbol << QString("330") + OhmSymbol << QString("470") + OhmSymbol << QString("680") + OhmSymbol
		 << QString("1k") + OhmSymbol << QString("1.5k") + OhmSymbol << QString("2.2k") + OhmSymbol << QString("3.3k") + OhmSymbol << QString("4.7k") + OhmSymbol << QString("6.8k") + OhmSymbol
		 << QString("10k") + OhmSymbol << QString("15k") + OhmSymbol << QString("22k") + OhmSymbol << QString("33k") + OhmSymbol << QString("47k") + OhmSymbol << QString("68k") + OhmSymbol
		 << QString("100k") + OhmSymbol << QString("150k") + OhmSymbol << QString("220k") + OhmSymbol << QString("330k") + OhmSymbol << QString("470k") + OhmSymbol << QString("680k") + OhmSymbol
		 << QString("1M") + OhmSymbol;
	}

	if (PinSpacings.count() == 0) {
		PinSpacings.insert("100 mil (stand-up right)", "pcb/axial_stand0_2_100mil_pcb.svg");
		PinSpacings.insert("100 mil (stand-up left)", "pcb/axial_stand1_2_100mil_pcb.svg");
		PinSpacings.insert("200 mil", "pcb/axial_lay_2_200mil_pcb.svg");
		PinSpacings.insert("300 mil", "pcb/axial_lay_2_300mil_pcb.svg");
		PinSpacings.insert("400 mil", "pcb/axial_lay_2_400mil_pcb.svg");
		PinSpacings.insert("500 mil", "pcb/axial_lay_2_500mil_pcb.svg");
		PinSpacings.insert("600 mil", "pcb/axial_lay_2_600mil_pcb.svg");
		PinSpacings.insert("800 mil", "pcb/axial_lay_2_800mil_pcb.svg");
	}

	if (ColorBands.count() == 0) {
		ColorBands.insert(0, QColor(0, 0, 0));
		ColorBands.insert(1, QColor(138, 61, 6));
		ColorBands.insert(2, QColor(196, 8, 8));
		ColorBands.insert(3, QColor(255, 77, 0));
		ColorBands.insert(4, QColor(255, 213, 0));
		ColorBands.insert(5, QColor(0, 163, 61));
		ColorBands.insert(6, QColor(0, 96, 182));
		ColorBands.insert(7, QColor(130, 16, 210));
		ColorBands.insert(8, QColor(140, 140, 140));
		ColorBands.insert(9, QColor(255, 255, 255));
		ColorBands.insert(-1, QColor(173, 159, 78));
		ColorBands.insert(-2, QColor(192, 192, 192));
	}

	if (Tolerances.count() == 0) {
		// TODO: move this into properties.xml
		Tolerances.insert(PlusMinusSymbol + "0.05%", QColor(140, 140, 140));
		Tolerances.insert(PlusMinusSymbol + "0.1%", QColor(130, 16, 210));
		Tolerances.insert(PlusMinusSymbol + "0.25%", QColor(0, 96, 182));
		Tolerances.insert(PlusMinusSymbol + "0.5%", QColor(0, 163, 61));
		Tolerances.insert(PlusMinusSymbol + "1%", QColor(138, 61, 6));
		Tolerances.insert(PlusMinusSymbol + "2%", QColor(196, 8, 8));
		Tolerances.insert(PlusMinusSymbol + "5%", QColor(173, 159, 78));
		Tolerances.insert(PlusMinusSymbol + "10%", QColor(192, 192, 192));
		Tolerances.insert(PlusMinusSymbol + "20%", QColor(0xdb, 0xb4, 0x77));
	}

	m_ohms = modelPart->localProp("resistance").toString();
	if (m_ohms.isEmpty()) {
		m_ohms = modelPart->properties().value("resistance", "220");
		modelPart->setLocalProp("resistance", m_ohms);
	}

	m_pinSpacing = modelPart->localProp("pin spacing").toString();
	if (m_pinSpacing.isEmpty()) {
		m_pinSpacing = modelPart->properties().value("pin spacing", "400 mil");
		modelPart->setLocalProp("pin spacing", m_pinSpacing);
	}

	updateResistances(m_ohms);
}

Resistor::~Resistor() {
}

void Resistor::setResistance(QString resistance, QString pinSpacing, bool force) {

	QString tolerance = prop("tolerance");

	if (resistance.endsWith(OhmSymbol)) {
		resistance.chop(1);
	}

	modelPart()->setLocalTitle(resistance + " " + OhmSymbol + " " + tr("Resistor"));

	switch (this->m_viewID) {
		case ViewLayer::BreadboardView:
			if (force || resistance.compare(m_ohms) != 0) {
				QString svg = makeSvg(resistance, m_viewLayerID);
				//DebugDialog::debug(svg);
				reloadRenderer(svg, false);
			}
			break;
		case ViewLayer::PCBView:
			if (force || pinSpacing.compare(m_pinSpacing) != 0) {

				InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
				if (infoGraphicsView == NULL) break;

				if (modelPart()->properties().value("package").compare("tht", Qt::CaseInsensitive) == 0) 
				{
                    // pinspacing is irrelevant for SMD resistors
					QString filename = PinSpacings.value(pinSpacing, "");
					if (filename.isEmpty()) break;

                    QString original = modelPart()->imageFileName(m_viewID);
                    modelPart()->setImageFileName(m_viewID, filename);
					m_changingPinSpacing = true;
					resetImage(infoGraphicsView);
					m_changingPinSpacing = false;
                    modelPart()->setImageFileName(m_viewID, original);
                    QList<ConnectorItem *> already;
					updateConnections(false, already);
				}

			}
			break;
		default:
			break;
	}

	m_ohms = resistance;
	m_pinSpacing = pinSpacing;
	modelPart()->setLocalProp("resistance", resistance);
	modelPart()->setLocalProp("pin spacing", pinSpacing);
	modelPart()->setLocalProp("tolerance", tolerance);

	updateResistances(m_ohms);
    if (m_partLabel) m_partLabel->displayTextsIf();
}

QString Resistor::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor) 
{
	switch (viewLayerID) {
		case ViewLayer::Breadboard:
		case ViewLayer::Icon:
			break;
		default:
			return Capacitor::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
	}

	QString svg = makeSvg(m_ohms, viewLayerID);
    return PaletteItemBase::normalizeSvg(svg, viewLayerID, blackOnly, dpi, factor);
}

QString Resistor::makeSvg(const QString & resistance, ViewLayer::ViewLayerID viewLayerID) {

	QString moduleID = this->moduleID();
	double ohms = TextUtils::convertFromPowerPrefix(resistance, OhmSymbol);
	QString sohms = QString::number(ohms, 'e', 3);
	int firstband = sohms.at(0).toLatin1() - '0';
	int secondband = sohms.at(2).toLatin1() - '0';
	int thirdband = sohms.at(3).toLatin1() - '0';
	int temp = (firstband * 10) + secondband;
	if (moduleID.contains("5Band")) {
		temp = (temp * 10) + thirdband;
	}
	int multiplier = (temp == 0) ? 0 : log10(ohms / temp);

	QString tolerance = prop("tolerance");

	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;
	QString fn = (viewLayerID == ViewLayer::Breadboard) ? m_breadboardSvgFile : m_iconSvgFile;
	QFile file(fn);
	if (!domDocument.setContent(&file, &errorStr, &errorLine, &errorColumn)) {
		DebugDialog::debug(QString("makesvg failed %1 %2 %3").arg(errorStr).arg(errorLine).arg(errorColumn));
		return "";
	}
	 
	QDomElement root = domDocument.documentElement();
	setBands(root, firstband, secondband, thirdband, multiplier, tolerance);
	return domDocument.toString();

}

void Resistor::setBands(QDomElement & element, int firstband, int secondband, int thirdband, int multiplier, const QString & tolerance)
{

	QString id = element.attribute("id");
	if (!id.isEmpty()) {
		if (id.compare("band_1_st") == 0) {
			element.setAttribute("fill", ColorBands.value(firstband, Qt::black).name());
		}
		else if (id.compare("band_2_nd") == 0) {
			element.setAttribute("fill", ColorBands.value(secondband, Qt::black).name());
		}
		else if (id.compare("band_3") == 0) {
			element.setAttribute("fill", ColorBands.value(thirdband, Qt::black).name());
		}
		else if (id.compare("band_rd_multiplier") == 0) {
			element.setAttribute("fill", ColorBands.value(multiplier, Qt::black).name());
		}
		else if (id.compare("gold_band") == 0) {
			element.setAttribute("fill", Tolerances.value(tolerance, QColor(173, 159, 78)).name());
		}		
	}

	QDomElement child = element.firstChildElement();
	while (!child.isNull()) {
		setBands(child, firstband, secondband, thirdband, multiplier, tolerance);
		child = child.nextSiblingElement();
	}
}

bool Resistor::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide)
{
	if (prop.compare("resistance", Qt::CaseInsensitive) == 0) {
		returnProp = tr("resistance");

		FocusOutComboBox * focusOutComboBox = new FocusOutComboBox();
		focusOutComboBox->setEnabled(swappingEnabled);
		focusOutComboBox->setEditable(true);
		QString current = m_ohms + OhmSymbol;
		focusOutComboBox->addItems(Resistances);
		focusOutComboBox->setCurrentIndex(focusOutComboBox->findText(current));
		BoundedRegExpValidator * validator = new BoundedRegExpValidator(focusOutComboBox);
		validator->setSymbol(OhmSymbol);
		validator->setConverter(TextUtils::convertFromPowerPrefix);
		validator->setBounds(0, 9900000000.0);
		validator->setRegExp(QRegExp(QString("((\\d{1,3})|(\\d{1,3}\\.)|(\\d{1,3}\\.\\d{1,2}))[\\x%1umkMG]{0,1}[\\x03A9]{0,1}").arg(TextUtils::MicroSymbolCode, 4, 16, QChar('0'))));
		focusOutComboBox->setValidator(validator);
		connect(focusOutComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(resistanceEntry(const QString &)));

		focusOutComboBox->setObjectName("infoViewComboBox");
        focusOutComboBox->setToolTip(tr("You can either type in a resistance value, or select one from the drop down. Format nnn.dP where P is one of 'umkMG'"));

		returnValue = current;			
		returnWidget = focusOutComboBox;	

		return true;
	}

	return Capacitor::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}

QString Resistor::getProperty(const QString & key) {
	if (key.compare("resistance", Qt::CaseInsensitive) == 0) {
		return m_ohms + OhmSymbol;
	}

	if (key.compare("pin spacing", Qt::CaseInsensitive) == 0) {
		return m_pinSpacing;
	}

	return Capacitor::getProperty(key);
}

QString Resistor::resistance() {
	return m_ohms;
}

QString Resistor::pinSpacing() {
	return m_pinSpacing;
}

void Resistor::addedToScene(bool temporary)
{
	if (this->scene()) {
		setResistance(m_ohms, m_pinSpacing, true);
	}

    return Capacitor::addedToScene(temporary);
}

const QString & Resistor::title() {
	m_title = QString("%1%2 Resistor").arg(m_ohms).arg(OhmSymbol);
	return m_title;
}

void Resistor::updateResistances(QString r) {
	if (!Resistances.contains(r + OhmSymbol)) {
		Resistances.append(r + OhmSymbol);
	}
}

ConnectorItem* Resistor::newConnectorItem(Connector *connector) {
	if (m_changingPinSpacing) {
		return connector->connectorItemByViewLayerID(viewID(), viewLayerID());
	}

	return Capacitor::newConnectorItem(connector);
}

ConnectorItem* Resistor::newConnectorItem(ItemBase * layerKin, Connector *connector) {
	if (m_changingPinSpacing) {
		return connector->connectorItemByViewLayerID(viewID(), layerKin->viewLayerID());
	}

	return Capacitor::newConnectorItem(layerKin, connector);
}

bool Resistor::hasCustomSVG() {
	switch (m_viewID) {
		case ViewLayer::BreadboardView:
		case ViewLayer::IconView:
			return true;
		default:
			return ItemBase::hasCustomSVG();
	}
}

bool Resistor::canEditPart() {
	return true;
}

QStringList Resistor::collectValues(const QString & family, const QString & prop, QString & value) {
	if (prop.compare("pin spacing", Qt::CaseInsensitive) == 0) {
		QStringList values;
		foreach (QString f, PinSpacings.keys()) {
			values.append(f);
		}
		value = m_pinSpacing;
		return values;
	}

	return Capacitor::collectValues(family, prop, value);
}

void Resistor::resistanceEntry(const QString & text) {
    //DebugDialog::debug(QString("resistance entry %1").arg(text));

    InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
    if (infoGraphicsView != NULL) {
            infoGraphicsView->setResistance(text, "");
    }
}

ItemBase::PluralType Resistor::isPlural() {
	return Plural;
}

void Resistor::setProp(const QString & prop, const QString & value) 
{
	Capacitor::setProp(prop, value);

	if (prop.compare("tolerance") == 0) {
		setResistance(m_ohms, m_pinSpacing, true);
	}
}

bool Resistor::setUpImage(ModelPart * modelPart, const LayerHash & viewLayers, LayerAttributes & layerAttributes)
{
	bool result = Capacitor::setUpImage(modelPart, viewLayers, layerAttributes);
	if (layerAttributes.viewLayerID == ViewLayer::Breadboard) {
		if (result && m_breadboardSvgFile.isEmpty()) m_breadboardSvgFile = layerAttributes.filename();
	}
	else if (layerAttributes.viewLayerID == ViewLayer::Icon) {
		if (result && m_iconSvgFile.isEmpty()) m_iconSvgFile = layerAttributes.filename();
	}
	return result;
}

ViewLayer::ViewID Resistor::useViewIDForPixmap(ViewLayer::ViewID vid, bool swappingEnabled) {
    if (swappingEnabled && vid == ViewLayer::BreadboardView) {
        return vid;
    }

    return ItemBase::useViewIDForPixmap(vid, swappingEnabled);
}
