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

#include "dip.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"
#include "../utils/familypropertycombobox.h"
#include "../utils/schematicrectconstants.h"
#include "../connectors/connectoritem.h"
#include "../sketch/infographicsview.h"
#include "../fsvgrenderer.h"
#include "pinheader.h"
#include "partfactory.h"

#include <QFontMetricsF>

static QStringList Spacings;

static int MinSipPins = 2;
static int MaxSipPins = 128;
static int MinDipPins = 4;
static int MaxDipPins = 128;

//////////////////////////////////////////////////

Dip::Dip( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: MysteryPart(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
}

Dip::~Dip() {
}

bool Dip::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide)
{
	bool result = MysteryPart::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
	if (prop.compare("chip label", Qt::CaseInsensitive) == 0) {
		returnProp = tr("chip label");
	}

	return result;
}

bool Dip::isDIP() {
	QString package = modelPart()->properties().value("package", "");
	if (package.indexOf("dip", 0, Qt::CaseInsensitive) >= 0) return true;
	if (package.indexOf("sip", 0, Qt::CaseInsensitive) >= 0) return false;
	return family().contains("generic ic", Qt::CaseInsensitive);
}

bool Dip::otherPropsChange(const QMap<QString, QString> & propsMap) {
	QString layout = modelPart()->properties().value("package", "");
	return (layout.compare(propsMap.value("package", "")) != 0);
}

const QStringList & Dip::spacings() {
	if (Spacings.count() == 0) {
		Spacings << "300mil" << "400mil" << "600mil" << "900mil";
	}

	return Spacings;
}

QString Dip::genSipFZP(const QString & moduleid)
{
	return genxFZP(moduleid, "generic_sip_fzpTemplate", MinSipPins, MaxSipPins, 1);
}

QString Dip::genDipFZP(const QString & moduleid)
{
	return genxFZP(moduleid, "generic_dip_fzpTemplate", MinDipPins, MaxDipPins, 2);
}

QStringList Dip::collectValues(const QString & family, const QString & prop, QString & value) {
	if (prop.compare("pins", Qt::CaseInsensitive) == 0) {
		QStringList values;
		value = modelPart()->properties().value("pins");

		if (isDIP()) {
			for (int i = MinDipPins; i <= MaxDipPins; i += 2) {
				values << QString::number(i);
			}
		}
		else {
			for (int i = MinSipPins; i <= MaxSipPins; i++) {
				values << QString::number(i);
			}
		}
		
		return values;
	}

	return MysteryPart::collectValues(family, prop, value);
}

QString Dip::genModuleID(QMap<QString, QString> & currPropsMap)
{
    QString spacing = currPropsMap.value("pin spacing", "300mil");
	QString value = currPropsMap.value("package");
	QString pins = currPropsMap.value("pins");
	if (pins.isEmpty()) pins = "16";		// pick something safe
	QString moduleID;
	if (value.contains("sip", Qt::CaseInsensitive)) {
		return QString("generic_sip_%1_%2").arg(pins).arg(spacing);
	}
	else {
		int p = pins.toInt();
		if (p < 4) p = 4;
		if (p % 2 == 1) p--;
		return QString("generic_ic_dip_%1_%2").arg(p).arg(spacing);
	}
}

QString Dip::retrieveSchematicSvg(QString & svg) {
	bool hasLocal = false;
	QStringList labels = getPinLabels(hasLocal);

	if (hasLocal) {
		if (this->isDIP()) {
			svg = makeSchematicSvg(labels);
            //DebugDialog::debug("make dip " + svg);
		}
		else {
			svg = MysteryPart::makeSchematicSvg(labels, true);
		}
	}

	return TextUtils::replaceTextElement(svg, "label", m_chipLabel);
}

QString Dip::makeSchematicSvg(const QString & expectedFileName) 
{
	QStringList pieces = expectedFileName.split("_");
	if (pieces.count() != 5) return "";

    QString spacing;
    int pins = TextUtils::getPinsAndSpacing(expectedFileName, spacing);
	QStringList labels;
	for (int i = 0; i < pins; i++) {
		labels << QString::number(i + 1);
	}
    
    if (expectedFileName.contains("sip", Qt::CaseInsensitive)) {
        if (expectedFileName.contains(PartFactory::OldSchematicPrefix)) {
            return MysteryPart::obsoleteMakeSchematicSvg(labels, true);
        }       

        return MysteryPart::makeSchematicSvg(labels, true);
    }

    if (expectedFileName.contains(PartFactory::OldSchematicPrefix)) {
        return obsoleteMakeSchematicSvg(labels);
    }

	return makeSchematicSvg(labels);
}

QString Dip::makeSchematicSvg(const QStringList & labels) 
{
    QDomDocument fakeDoc;

    QList<QDomElement> lefts;
    QList<QDomElement> rights;
    for (int i = 0; i < labels.count() / 2; i++) {
        QDomElement element = fakeDoc.createElement("contact");
        element.setAttribute("connectorIndex", i);
        element.setAttribute("name", labels.at(i));
        lefts.append(element);
        int j = labels.count() - i - 1;
        element = fakeDoc.createElement("contact");
        element.setAttribute("connectorIndex", j);
        element.setAttribute("name", labels.at(j));
        rights.append(element);
    }
    QList<QDomElement> empty;
    QStringList busNames;
    QString ic("IC");
    return SchematicRectConstants::genSchematicDIP(empty, empty, lefts, rights, empty, busNames, ic, false, false, SchematicRectConstants::simpleGetConnectorName);
}

QString Dip::obsoleteMakeSchematicSvg(const QStringList & labels) 
{
	int pins = labels.count();
	int increment = GraphicsUtils::StandardSchematicSeparationMils;  
	int border = 30;
	double totalHeight = (pins * increment / 2) + increment + border;
	int pinWidth = increment + increment + border;
	int centralWidth = increment * 4;
	int pinTextWidth = 0;
	int defaultLabelWidth = 30;
	int labelFontSize = 130;

	double textMax = defaultLabelWidth;
	QFont font("Droid Sans", labelFontSize * 72 / GraphicsUtils::StandardFritzingDPI, QFont::Normal);
	QFontMetricsF fm(font);
	for (int i = 0; i < labels.count(); i++) {
		double w = fm.width(labels.at(i));
		if (w > textMax) textMax = w;
	}
	if (textMax > defaultLabelWidth) {
		pinTextWidth += (textMax - defaultLabelWidth) * 2 * 1000 / 72;
	}
	centralWidth += pinTextWidth;
	int totalWidth = centralWidth + pinWidth;


	QString header("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
					"<svg xmlns:svg='http://www.w3.org/2000/svg' xmlns='http://www.w3.org/2000/svg' version='1.2' baseProfile='tiny' \n"
					"width='%4in' height='%1in' viewBox='0 0 %5 %2' >\n"
					"<g id='schematic' >\n"
					"<rect x='315' y='15' fill='none' width='%6' height='%3' stroke='#000000' stroke-linejoin='round' stroke-linecap='round' stroke-width='30' />\n"
					"<text id='label' x='%7' y='465' font-family='Droid Sans' stroke='none' fill='#000000' text-anchor='middle' font-size='235' >IC</text>\n");

	QString svg = header
				.arg(totalHeight / 1000)
				.arg(totalHeight)
				.arg(totalHeight - border)
				.arg(totalWidth / 1000.0)
				.arg(totalWidth)
				.arg(centralWidth)
				.arg(totalWidth / 2.0)
				;
  
	QString repeatL("<line fill='none' stroke='#000000' stroke-linejoin='round' stroke-linecap='round' stroke-width='30' x1='15' y1='%1' x2='300' y2='%1'  />\n"
					"<rect x='0' y='%2' fill='none' width='300' height='30' stroke='none' id='connector%3pin' stroke-width='0' />\n"
					"<rect x='0' y='%2' fill='none' width='30' height='30' stroke='none' id='connector%3terminal' stroke-width='0' />\n"
					"<text id='label%3' x='390' y='%4' font-family='Droid Sans' stroke='none' fill='#000000' text-anchor='start' font-size='%6' >%5</text>\n");

	
	QString repeatR("<line fill='none' stroke='#000000' stroke-linejoin='round' stroke-linecap='round' stroke-width='30' x1='%7' y1='%1' x2='%8' y2='%1'  />\n"
					"<rect x='%9' y='%2' fill='none' width='300' height='30' id='connector%3pin' stroke='none' stroke-width='0' />\n"
					"<rect x='%10' y='%2' fill='none' width='30' height='30' id='connector%3terminal' stroke='none' stroke-width='0' />\n"
					"<text id='label%3' x='%11' y='%4' font-family='Droid Sans' stroke='none' fill='#000000' text-anchor='end' font-size='%6' >%5</text>\n");


	int y = 300;
	for (int i = 0; i < pins / 2; i++) {
		svg += repeatL.arg(15 + y).arg(y).arg(i).arg(y + 50).arg(labels.at(i)).arg(labelFontSize);
		svg += repeatR
				.arg(15 + y)
				.arg(y)
				.arg(pins - i - 1)
				.arg(y + 50)
				.arg(labels.at(pins - i - 1))
				.arg(labelFontSize)

				.arg(totalWidth - 300)
				.arg(totalWidth - 15)
				.arg(totalWidth - 300)
				.arg(totalWidth - 30)
				.arg(totalWidth - 390)								
				;
		y += increment;
	}

	svg += "</g>\n";
	svg += "</svg>\n";

	return svg;
}
 
QString Dip::makeBreadboardSvg(const QString & expectedFileName) 
{
	if (expectedFileName.contains("_sip_")) return makeBreadboardSipSvg(expectedFileName);
	if (expectedFileName.contains("_dip_")) return makeBreadboardDipSvg(expectedFileName);
	return "";
}

QString Dip::makeBreadboardSipSvg(const QString & expectedFileName) 
{
	QStringList pieces = expectedFileName.split("_");
	if (pieces.count() != 5) return "";

	int pins = pieces.at(2).toInt();
	int increment = 10;
	double totalWidth = (pins * increment);

	QString svg = TextUtils::incrementTemplate(":/resources/templates/generic_sip_bread_template.txt", 1, increment * (pins - 2), TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);

	QString repeat("<rect id='connector%1pin' x='[13.5]' y='25.66' fill='#8C8C8C' width='3' height='4.34'/>\n"
					"<rect id='connector%1terminal' x='[13.5]' y='27.0' fill='#8C8C8C' width='3' height='3'/>\n"
					"<polygon fill='#8C8C8C' points='[11.5],25.66 [11.5],26.74 [13.5],27.66 [16.5],27.66 [18.5],26.74 [18.5],25.66'/>\n"); 

	QString repeats;
	if (pins > 2) {
		repeats = TextUtils::incrementTemplateString(repeat, pins - 2, increment, TextUtils::standardMultiplyPinFunction, TextUtils::incCopyPinFunction, NULL);
	}

	return svg.arg(totalWidth / 100).arg(pins - 1).arg(repeats);

}

QString Dip::makeBreadboardDipSvg(const QString & expectedFileName) 
{
	QStringList pieces = expectedFileName.split("_");
	if (pieces.count() != 6) return "";

	int pins = pieces.at(3).toInt();
	int increment = 10;
	double spacing = TextUtils::convertToInches(pieces.at(4)) * 100;

	QString repeatB("<rect id='connector%1pin' x='{13.5}' y='[28.66]' fill='#8C8C8C' width='3' height='4.34'/>\n"
					"<rect id='connector%1terminal' x='{13.5}' y='[30.0]' fill='#8C8C8C' width='3' height='3'/>\n"
					"<polygon fill='#8C8C8C' points='{11.5},[28.66] {11.5},[29.74] {13.5},[30.66] {16.5},[30.66] {18.5},[29.74] {18.5},[28.66]'/> \n"); 

	QString repeatT("<rect id='connector%1pin' x='[13.5]' y='0' fill='#8C8C8C' width='3' height='4.34'/>\n"
					"<rect id='connector%1terminal' x='[13.5]' fill='#8C8C8C' width='3' height='3'/>\n"
					"<polygon fill='#8C8C8C' points='[18.5],4.34 [18.5],3.26 [16.5],2.34 [13.5],2.34 [11.5],3.26 [11.5],4.34'/>\n");

	QString header("<?xml version='1.0' encoding='utf-8'?>\n"
					"<svg version='1.2' baseProfile='tiny' id='svg2' xmlns='http://www.w3.org/2000/svg' \n"
					"width='.percent.1in' height='%1in' viewBox='0 0 {20.0} [33.0]' >\n"
					"<g id='breadboard'>\n"
					"<rect id='middle' x='0' y='3' fill='#303030' width='{20.0}' height='[27.0]' />\n"
					"<rect id='top' x='0' y='3.0' fill='#3D3D3D' width='{20.0}' height='2.46'/>\n"
					"<rect id='bottom' x='0' y='[27.54]' fill='#000000' width='{20.0}' height='2.46'/>\n"
					"<polygon id='right' fill='#141414' points='{20.0},3 {19.25},5.46 {19.25},[27.54] {20.0},[30.0]'/> \n"
					"<polygon id='left' fill='#1F1F1F' points='0,3 0.75,5.46 0.75,[27.54] 0,[30.0]'/>\n"
					"<polygon id='left-upper-rect' fill='#1C1C1C' points='5,{{11.5}} 0.75,{{11.46}} 0.56,{{13.58}} 0.56,{{16.5}} 5,{{16.5}}'/>\n"
					"<polygon id='left-lower-rect' fill='#383838' points='0.75,{{21.55}} 5,{{21.55}} 5,{{16.5}} 0.56,{{16.5}} 0.56,{{19.42}}'/>\n"
					"<path id='slot' fill='#262626' d='M0.56,{{13.58}}v5.83c1.47-0.17,2.62-1.4,2.62-2.92C3.18,{{14.97}},2.04,{{13.75}},0.56,{{13.58}}z'/>\n"
					"<path id='cover' fill='#303030' d='M0.75,5.46V{{11.45}}c2.38,0.45,4.19,2.53,4.19,5.04c0,2.51-1.8,4.6-4.19,5.05V{{21.55}}h5.0V5.46H0.75z'/>\n"
					"<circle fill='#212121' cx='6.5' cy='[23.47]' r='2.06'/>\n"
					"<text id='label' x='6.5' y='{{16.5}}' fill='#e6e6e6' stroke='none' font-family='%5' text-anchor='start' font-size='8' >IC</text>\n"

					"<rect id='connector0pin' x='3.5' y='[28.66]' fill='#8C8C8C' width='3' height='4.34'/>\n"
					"<rect id='connector0terminal' x='3.5' y='[30.0]' fill='#8C8C8C' width='3' height='3'/>\n"
					"<polygon fill='#8C8C8C' points='3.5,[28.66] 3.5,[30.66] 6.5,[30.66] 8.5,[29.74] 8.5,[28.66] '/>\n"

					"<rect id='connector%2pin' x='3.5' fill='#8C8C8C' width='3' height='4.34'/>\n"
					"<rect id='connector%2terminal' x='3.5' fill='#8C8C8C' width='3' height='3'/>\n"
					"<polygon fill='#8C8C8C' points='8.5,4.34 8.5,3.26 6.5,2.34 3.5,2.34 3.5,4.34'/>\n"

					"<rect id='connector%3pin' x='{13.5}' y='[28.66]' fill='#8C8C8C' width='3' height='4.34'/>\n"
					"<rect id='connector%3terminal' x='{13.5}' y='[30.0]' fill='#8C8C8C' width='3' height='3'/>\n"
					"<polygon fill='#8C8C8C' points='{11.5},[28.66] {11.5},[29.74] {13.5},[30.66] {16.5},[30.66] {16.5},[28.66]'/>\n"
					
					"<rect id='connector%4pin' x='{13.5}' y='0' fill='#8C8C8C' width='3' height='4.34'/>\n"
					"<rect id='connector%4terminal' x='{13.5}' y='0' fill='#8C8C8C' width='3' height='3'/>\n"
					"<polygon fill='#8C8C8C' points='{16.5},4.34 {16.5},2.34 {13.5},2.34 {11.5},3.26 {11.5},4.34 '/>\n"
 
					".percent.2\n"
					".percent.3\n"

					"</g>\n"
					"</svg>\n");

	// header came from a 300mil dip, so base case is spacing - (increment * 3)

	header = TextUtils::incrementTemplateString(header, 1, spacing - (increment * 3), TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);
	header = header
        .arg(TextUtils::getViewBoxCoord(header, 3) / 100.0)
        .arg(pins - 1)
        .arg((pins / 2) - 1)
        .arg(pins / 2)
        .arg(OCRAFontName);
	header.replace("{{", "[");
	header.replace("}}", "]");
	header = TextUtils::incrementTemplateString(header, 1, (spacing - (increment * 3)) / 2, TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);
	header.replace("{", "[");
	header.replace("}", "]");
	header.replace(".percent.", "%");

	QString svg = TextUtils::incrementTemplateString(header, 1, increment * ((pins - 4) / 2), TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);

	repeatB = TextUtils::incrementTemplateString(repeatB, 1, spacing - (increment * 3), TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);
	repeatB.replace("{", "[");
	repeatB.replace("}", "]");

	int userData[2];
	userData[0] = pins - 1;
	userData[1] = 1;
	QString repeatTs = TextUtils::incrementTemplateString(repeatT, (pins - 4) / 2, increment, TextUtils::standardMultiplyPinFunction, TextUtils::negIncCopyPinFunction, userData);
	QString repeatBs = TextUtils::incrementTemplateString(repeatB, (pins - 4) / 2, increment, TextUtils::standardMultiplyPinFunction, TextUtils::incCopyPinFunction, NULL);


	return svg.arg(TextUtils::getViewBoxCoord(svg, 2) / 100.0).arg(repeatTs).arg(repeatBs);
}

bool Dip::changePinLabels(bool singleRow, bool sip) {
	if (m_viewID != ViewLayer::SchematicView) return true;

	bool hasLocal = false;
	QStringList labels = getPinLabels(hasLocal);
	if (labels.count() == 0) return true;


	QString svg;
	if (singleRow) {
		svg = MysteryPart::makeSchematicSvg(labels, sip);
	}
	else {
		svg = Dip::makeSchematicSvg(labels);
	}

    QString chipLabel = modelPart()->localProp("chip label").toString();
    if (!chipLabel.isEmpty()) {
        svg =TextUtils::replaceTextElement(svg, "label", chipLabel);
    }

    QTransform  transform = untransform();

	resetLayerKin(svg);
    resetConnectors();

    retransform(transform);

	return true;
}

void Dip::addedToScene(bool temporary)
{
    MysteryPart::addedToScene(temporary);
	if (this->isDIP()) {
		resetConnectors();
	}
}

void Dip::swapEntry(const QString & text) {

    FamilyPropertyComboBox * comboBox = qobject_cast<FamilyPropertyComboBox *>(sender());
    if (comboBox == NULL) return;

    bool sip = moduleID().contains("sip");
    bool dip = moduleID().contains("dip");

    if (comboBox->prop().contains("package", Qt::CaseInsensitive)) {
        sip = text.contains("sip", Qt::CaseInsensitive);
        dip = text.contains("dip", Qt::CaseInsensitive);
        if (!dip && !sip) {
            // one of the generic smds
            PaletteItem::swapEntry(text);
            return;
        }
    }

    if (sip) {
        generateSwap(text, genModuleID, genSipFZP, makeBreadboardSvg, makeSchematicSvg, PinHeader::makePcbSvg);
    }
    else if (dip) {
        generateSwap(text, genModuleID, genDipFZP, makeBreadboardSvg, makeSchematicSvg, makePcbDipSvg);
    }
    PaletteItem::swapEntry(text);
}

