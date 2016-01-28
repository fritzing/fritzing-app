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

#include "mysterypart.h"
#include "../utils/graphicsutils.h"
#include "../utils/familypropertycombobox.h"
#include "../utils/schematicrectconstants.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../commands.h"
#include "../utils/textutils.h"
#include "../layerattributes.h"
#include "partlabel.h"
#include "pinheader.h"
#include "partfactory.h"
#include "../connectors/connectoritem.h"
#include "../svg/svgfilesplitter.h"

#include <QDomNodeList>
#include <QDomDocument>
#include <QDomElement>
#include <QLineEdit>

static QStringList Spacings;
static QRegExp Digits("(\\d)+");
static QRegExp DigitsMil("(\\d)+mil");

static const int MinSipPins = 1;
static const int MaxSipPins = 128;
static const int MinDipPins = 4;
static const int MaxDipPins = 128;

static HoleClassThing TheHoleThing;

// TODO
//	save into parts bin

MysteryPart::MysteryPart( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: PaletteItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	m_chipLabel = modelPart->localProp("chip label").toString();
	if (m_chipLabel.isEmpty()) {
		m_chipLabel = modelPart->properties().value("chip label", "?");
		modelPart->setLocalProp("chip label", m_chipLabel);
	}

    setUpHoleSizes("mystery", TheHoleThing);

}

MysteryPart::~MysteryPart() {
}

void MysteryPart::setProp(const QString & prop, const QString & value) {
	if (prop.compare("chip label", Qt::CaseInsensitive) == 0) {
		setChipLabel(value, false);
		return;
	}

	PaletteItem::setProp(prop, value);
}

void MysteryPart::setChipLabel(QString chipLabel, bool force) {

	if (!force && m_chipLabel.compare(chipLabel) == 0) return;

	m_chipLabel = chipLabel;

	QString svg;
	switch (this->m_viewID) {
		case ViewLayer::BreadboardView:
			svg = makeSvg(chipLabel, true);
            if (!svg.isEmpty()) {
	            resetRenderer(svg);
            }			
            break;
		case ViewLayer::SchematicView:
            {
                QTransform  transform = untransform();
			    svg = makeSvg(chipLabel, false);
			    svg = retrieveSchematicSvg(svg);
                resetLayerKin(svg);
                retransform(transform);
            }
			break;
		default:
			break;
	}

	modelPart()->setLocalProp("chip label", chipLabel);

    if (m_partLabel) m_partLabel->displayTextsIf();
}

QString MysteryPart::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor) 
{
	QString svg = PaletteItem::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
	switch (viewLayerID) {
		case ViewLayer::Breadboard:
		case ViewLayer::Icon:
			return TextUtils::replaceTextElement(svg, "label", m_chipLabel);

		case ViewLayer::Schematic:
			svg = retrieveSchematicSvg(svg);
			return TextUtils::removeSVGHeader(svg);

		default:
			break;
	}

	return svg; 
}

QString MysteryPart::retrieveSchematicSvg(QString & svg) {

	bool hasLocal = false;
	QStringList labels = getPinLabels(hasLocal);
		
	if (hasLocal) {
		svg = makeSchematicSvg(labels, false);
	}

	return TextUtils::replaceTextElement(svg, "label", m_chipLabel);
}


QString MysteryPart::makeSvg(const QString & chipLabel, bool replace) {
	QString path = filename();
	QFile file(filename());
	QString svg;
	if (file.open(QFile::ReadOnly)) {
		svg = file.readAll();
		file.close();
		if (!replace) return svg;

		return TextUtils::replaceTextElement(svg, "label", chipLabel);
	}

	return "";
}

QStringList MysteryPart::collectValues(const QString & family, const QString & prop, QString & value) {
	if (prop.compare("layout", Qt::CaseInsensitive) == 0) {
        // TODO: translate these
        QStringList values;
        values << "Single Row" << "Double Row";
        value = values.at(moduleID().contains("sip", Qt::CaseInsensitive) ? 0 : 1);
        return values;
    }

	if (prop.compare("pin spacing", Qt::CaseInsensitive) == 0) {
		QStringList values;
        QString spacing;
        TextUtils::getPinsAndSpacing(moduleID(), spacing);
		if (isDIP()) {
			foreach (QString f, spacings()) {
				values.append(f);
			}
		}
		else {
			values.append(spacing);
		}

		value = spacing;
		return values;
	}

	if (prop.compare("pins", Qt::CaseInsensitive) == 0) {
		QStringList values;
		value = modelPart()->properties().value("pins");

		if (moduleID().contains("dip")) {
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


	return PaletteItem::collectValues(family, prop, value);
}

bool MysteryPart::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide)
{
	if (prop.compare("chip label", Qt::CaseInsensitive) == 0) {
		returnProp = tr("label");
		returnValue = m_chipLabel;

		QLineEdit * e1 = new QLineEdit(parent);
		e1->setEnabled(swappingEnabled);
		e1->setText(m_chipLabel);
		connect(e1, SIGNAL(editingFinished()), this, SLOT(chipLabelEntry()));
		e1->setObjectName("infoViewLineEdit");


		returnWidget = e1;

		return true;
	}

    if (prop.compare("hole size", Qt::CaseInsensitive) == 0) {
        return collectHoleSizeInfo(TheHoleThing.holeSizeValue, parent, swappingEnabled, returnProp, returnValue, returnWidget);
	}

	return PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}

QString MysteryPart::getProperty(const QString & key) {
	if (key.compare("chip label", Qt::CaseInsensitive) == 0) {
		return m_chipLabel;
	}

	return PaletteItem::getProperty(key);
}

QString MysteryPart::chipLabel() {
	return m_chipLabel;
}

void MysteryPart::addedToScene(bool temporary)
{
	if (this->scene()) {
		setChipLabel(m_chipLabel, true);
	}

    PaletteItem::addedToScene(temporary);
}

const QString & MysteryPart::title() {
	return m_chipLabel;
}

bool MysteryPart::hasCustomSVG() {
	switch (m_viewID) {
		case ViewLayer::BreadboardView:
		case ViewLayer::SchematicView:
		case ViewLayer::IconView:
		case ViewLayer::PCBView:
			return true;
		default:
			return ItemBase::hasCustomSVG();
	}
}

void MysteryPart::chipLabelEntry() {
	QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == NULL) return;

	if (edit->text().compare(this->chipLabel()) == 0) return;

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setProp(this, "chip label", tr("chip label"), this->chipLabel(), edit->text(), true);
	}
}

bool MysteryPart::isDIP() {
	QString layout = modelPart()->properties().value("layout", "");
	return (layout.indexOf("double", 0, Qt::CaseInsensitive) >= 0);
}

bool MysteryPart::otherPropsChange(const QMap<QString, QString> & propsMap) {
	QString layout = modelPart()->properties().value("layout", "");
	return (layout.compare(propsMap.value("layout", "")) != 0);
}

const QStringList & MysteryPart::spacings() {
	if (Spacings.count() == 0) {
		Spacings << "100mil" << "200mil" << "300mil" << "400mil" << "500mil" << "600mil" << "700mil" << "800mil" << "900mil";
	}
	return Spacings;
}

ItemBase::PluralType MysteryPart::isPlural() {
	return Plural;
}

QString MysteryPart::genSipFZP(const QString & moduleid)
{
    return genxFZP(moduleid, "mystery_part_sipFzpTemplate", MinSipPins, MaxSipPins, 1);
}

QString MysteryPart::genDipFZP(const QString & moduleid)
{
    return genxFZP(moduleid, "mystery_part_dipFzpTemplate", MinDipPins, MaxDipPins, 2);

}

QString MysteryPart::genxFZP(const QString & moduleid, const QString & templateName, int minPins, int maxPins, int step) {
    QString spacingString;
    TextUtils::getPinsAndSpacing(moduleid, spacingString);
    QString result = PaletteItem::genFZP(moduleid, templateName, minPins, maxPins, step, false);
   	result.replace(".percent.", "%");
	result = result.arg(spacingString);
	return hackFzpHoleSize(result, moduleid);
}

QString MysteryPart::hackFzpHoleSize(const QString & fzp, const QString & moduleid) {
    int hsix = moduleid.lastIndexOf(HoleSizePrefix);
    if (hsix >= 0) {
        return PaletteItem::hackFzpHoleSize(fzp, moduleid, hsix);
    }

    return fzp;
}

QString MysteryPart::genModuleID(QMap<QString, QString> & currPropsMap)
{
	QString value = currPropsMap.value("layout");
    bool single = value.contains("single", Qt::CaseInsensitive);
	QString pins = currPropsMap.value("pins");
    QString spacing = currPropsMap.value("pin spacing", "300mil");
	if (single) {
		return QString("mystery_part_sip_%1_100mil").arg(pins);
	}
	else {
		int p = pins.toInt();
		if (p < 4) p = 4;
		if (p % 2 == 1) p--;
		return QString("mystery_part_dip_%1_%2").arg(p).arg(spacing);
	}
}

QString MysteryPart::makeSchematicSvg(const QString & expectedFileName) 
{
	bool sip = expectedFileName.contains("sip", Qt::CaseInsensitive);

	QStringList pieces = expectedFileName.split("_");

	int pins = pieces.at(2).toInt();
	QStringList labels;
	for (int i = 0; i < pins; i++) {
		labels << QString::number(i + 1);
	}

    if (expectedFileName.contains(PartFactory::OldSchematicPrefix)) {
        return obsoleteMakeSchematicSvg(labels, sip);
    }     

	return makeSchematicSvg(labels, sip);
}

QString MysteryPart::makeSchematicSvg(const QStringList & labels, bool sip) 
{	
    QDomDocument fakeDoc;

    QList<QDomElement> lefts;
    for (int i = 0; i < labels.count(); i++) {
        QDomElement element = fakeDoc.createElement("contact");
        element.setAttribute("connectorIndex", i);
        element.setAttribute("name", labels.at(i));
        lefts.append(element);
    }
    QList<QDomElement> empty;
    QStringList busNames;

    QString titleText = sip ? "IC" : "?";
    QString svg = SchematicRectConstants::genSchematicDIP(empty, empty, lefts, empty, empty, busNames, titleText, false, false, SchematicRectConstants::simpleGetConnectorName);
    if (sip) return svg;
    
    // add the mystery part graphic
    QDomDocument doc;
    if (!doc.setContent(svg)) return svg;

    QRectF viewBox;
    double w, h;
    TextUtils::ensureViewBox(doc, GraphicsUtils::StandardFritzingDPI, viewBox, false, w, h, false);

    double newUnit = 1000 * SchematicRectConstants::NewUnit / 25.4;
    double rectStroke = 2 * 1000 * SchematicRectConstants::RectStrokeWidth / 25.4;
    QString circle = QString("<circle cx='%1' cy='%2' r='%3' fill='black' stroke-width='0' stroke='none' />\n");
	circle += QString("<text x='%1' fill='#FFFFFF' y='%4' font-family=\"%5\" text-anchor='middle' font-weight='bold' stroke='none' stroke-width='0' font-size='%6' >?</text>\n");
    circle = circle
                .arg(viewBox.width() - rectStroke - (newUnit / 2))
                .arg(rectStroke + (newUnit / 2))
                .arg(newUnit / 2)

                .arg(rectStroke + (newUnit / 2) + (newUnit / 3))   // offset so the text appears in the center of the circle
                .arg(SchematicRectConstants::FontFamily)
                .arg(newUnit)
                ;

    QDomDocument temp;
    temp.setContent(QString("<g>" + circle + "</g>"));

    QDomElement root = doc.documentElement();
    QDomElement schematic = TextUtils::findElementWithAttribute(root, "id", "schematic");
    if (schematic.isNull()) return svg;

    QDomElement tempRoot = temp.documentElement();
    schematic.appendChild(tempRoot);

    return doc.toString(1);
}

QString MysteryPart::obsoleteMakeSchematicSvg(const QStringList & labels, bool sip) 
{	
	int increment = GraphicsUtils::StandardSchematicSeparationMils;   // 7.5mm;
	int border = 30;
	double totalHeight = (labels.count() * increment) + increment + border;
	int textOffset = 50;
	int repeatTextOffset = 50;
	int fontSize = 255;
	int labelFontSize = 130;
	int defaultLabelWidth = 30;
	QString labelText = "?";
	double textMax = defaultLabelWidth;
	QFont font("Droid Sans", labelFontSize * 72 / GraphicsUtils::StandardFritzingDPI, QFont::Normal);
	QFontMetricsF fm(font);
	for (int i = 0; i < labels.count(); i++) {
		double w = fm.width(labels.at(i));
		if (w > textMax) textMax = w;
	}
	textMax = textMax * GraphicsUtils::StandardFritzingDPI / 72;

	int totalWidth = (5 * increment) + border;
	int innerWidth = 4 * increment;
	if (textMax > defaultLabelWidth) {
		totalWidth += (textMax - defaultLabelWidth);
		innerWidth += (textMax - defaultLabelWidth);
	}

	QString header("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
					"<svg xmlns:svg='http://www.w3.org/2000/svg' \n"
					"xmlns='http://www.w3.org/2000/svg' \n"
					"version='1.2' baseProfile='tiny' \n"
					"width='%7in' height='%1in' viewBox='0 0 %8 %2'>\n"
					"<g id='schematic'>\n"
					"<rect x='315' y='15' fill='none' width='%9' height='%3' stroke='#000000' stroke-linejoin='round' stroke-linecap='round' stroke-width='30' />\n"
					"<text id='label' x='%11' y='%4' fill='#000000' stroke='none' font-family='Droid Sans' text-anchor='middle' font-size='%5' >%6</text>\n");

	if (!sip) {
		header +=	"<circle fill='#000000' cx='%10' cy='200' r='150' stroke-width='0' />\n"
					"<text x='%10' fill='#FFFFFF' y='305' font-family='Droid Sans' text-anchor='middle' font-weight='bold' stroke-width='0' font-size='275' >?</text>\n";
	}
	else {
		labelText = "IC";
		fontSize = 235;
		textOffset = 0;
	}

	QString svg = header
		.arg(totalHeight / GraphicsUtils::StandardFritzingDPI)
		.arg(totalHeight)
		.arg(totalHeight - border)
		.arg((totalHeight / 2) + textOffset)
		.arg(fontSize)
		.arg(labelText)
		.arg(totalWidth / 1000.0)
		.arg(totalWidth)
		.arg(innerWidth)
		.arg(totalWidth - 200)
		.arg(increment + textMax + ((totalWidth - increment - textMax) / 2.0))
		;


	QString repeat("<line fill='none' stroke='#000000' stroke-linejoin='round' stroke-linecap='round' stroke-width='30' x1='15' y1='%1' x2='300' y2='%1'  />\n"
					"<rect x='0' y='%2' fill='none' width='300' height='30' id='connector%3pin' stroke-width='0' />\n"
					"<rect x='0' y='%2' fill='none' width='30' height='30' id='connector%3terminal' stroke-width='0' />\n"
					"<text id='label%3' x='390' y='%4' font-family='Droid Sans' stroke='none' fill='#000000' text-anchor='start' font-size='%6' >%5</text>\n");
  
	for (int i = 0; i < labels.count(); i++) {
		svg += repeat
			.arg(increment + (border / 2) + (i * increment))
			.arg(increment + (i * increment))
			.arg(i)
			.arg(increment + repeatTextOffset + (i * increment))
			.arg(labels.at(i))
			.arg(labelFontSize);
	}

	svg += "</g>\n";
	svg += "</svg>\n";
	return svg;
}


QString MysteryPart::makeBreadboardSvg(const QString & expectedFileName) 
{
	if (expectedFileName.contains("_sip_")) return makeBreadboardSipSvg(expectedFileName);
	if (expectedFileName.contains("_dip_")) return makeBreadboardDipSvg(expectedFileName);

	return "";
}

QString MysteryPart::makeBreadboardDipSvg(const QString & expectedFileName) 
{
    QString spacingString;
    int pins = TextUtils::getPinsAndSpacing(expectedFileName, spacingString);
	double spacing = TextUtils::convertToInches(spacingString) * 100;

	int increment = 10;

	QString repeatT("<rect id='connector%1terminal' x='[1.87]' y='1' fill='#8C8C8C' stroke='none' stroke-width='0' width='2.3' height='0'/>\n"
					"<rect id='connector%1pin' x='[1.87]' y='0' fill='#8C8C8C' stroke='none' stroke-width='0' width='2.3' height='3.5'/>\n");

	QString repeatB("<rect id='connector%1terminal' x='{1.87}' y='[11.0]' fill='#8C8C8C' stroke='none' stroke-width='0' width='2.3' height='0'/>\n"
					"<rect id='connector%1pin' x='{1.87}' y='[7.75]' fill='#8C8C8C' stroke='none' stroke-width='0' width='2.3' height='4.25'/>\n");

	QString header("<?xml version='1.0' encoding='utf-8'?>\n"
					"<svg version='1.2' baseProfile='tiny' xmlns='http://www.w3.org/2000/svg' \n"
					"width='.percent.1in' height='%1in' viewBox='0 0 {16.0022} [12.0]'>\n"
					"<g id='breadboard'>\n"
					".percent.2\n"
					"<rect width='{16.0022}' x='0' y='2.5' height='[6.5]' fill='#000000' id='upper' stroke-width='0' />\n"
					"<rect width='{16.0022}' x='0' y='[6.5]' fill='#404040' height='3.096' id='lower' stroke-width='0' />\n"
					"<text id='label' x='2.5894' y='{{6.0}}' fill='#e6e6e6' stroke='none' font-family='Droid Sans' text-anchor='start' font-size='7.3' >?</text>\n"
					"<circle fill='#8C8C8C' cx='11.0022' cy='{{7.5}}' r='3' stroke-width='0' />\n"
					"<text x='11.0022' y='{{9.2}}' font-family='Droid Sans' text-anchor='middle' font-weight='bold' stroke-width='0' font-size='5.5' >?</text>\n"
					".percent.3\n"
					"</g>\n"
					"</svg>\n");


	header = TextUtils::incrementTemplateString(header, 1, spacing - increment, TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);
	header = header.arg(TextUtils::getViewBoxCoord(header, 3) / 100.0);
	if (spacing == 10) {
		header.replace("{{6.0}}", "8.0");
		header.replace("{{7.5}}", "5.5");
		header.replace("{{9.2}}", "7.2");
	}
	else {
		header.replace("{{6.0}}", QString::number(6.0 + ((spacing - increment) / 2)));
		header.replace("{{7.5}}", "7.5");
		header.replace("{{9.2}}", "9.2");
	}
	header.replace(".percent.", "%");
	header.replace("{", "[");
	header.replace("}", "]");

	QString svg = TextUtils::incrementTemplateString(header, 1, increment * ((pins - 4) / 2), TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);

	repeatB = TextUtils::incrementTemplateString(repeatB, 1, spacing - increment, TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);
	repeatB.replace("{", "[");
	repeatB.replace("}", "]");

	int userData[2];
	userData[0] = pins;
	userData[1] = 1;
	QString repeatTs = TextUtils::incrementTemplateString(repeatT, pins / 2, increment, TextUtils::standardMultiplyPinFunction, TextUtils::negIncCopyPinFunction, userData);
	QString repeatBs = TextUtils::incrementTemplateString(repeatB, pins / 2, increment, TextUtils::standardMultiplyPinFunction, TextUtils::standardCopyPinFunction, NULL);

	return svg.arg(TextUtils::getViewBoxCoord(svg, 2) / 100).arg(repeatTs).arg(repeatBs);
}

QString MysteryPart::makeBreadboardSipSvg(const QString & expectedFileName) 
{
	QStringList pieces = expectedFileName.split("_");
	if (pieces.count() != 6) return "";

	int pins = pieces.at(2).toInt();
	int increment = 10;

	QString header("<?xml version='1.0' encoding='utf-8'?>\n"
					"<svg version='1.2' baseProfile='tiny' id='svg2' xmlns='http://www.w3.org/2000/svg' \n"
					"width='%1in' height='0.27586in' viewBox='0 0 [6.0022] 27.586'>\n"
					"<g id='breadboard'>\n"
					"<rect width='[6.0022]' x='0' y='0' height='24.17675' fill='#000000' id='upper' stroke-width='0' />\n"
					"<rect width='[6.0022]' x='0' y='22' fill='#404040' height='3.096' id='lower' stroke-width='0' />\n"
					"<text id='label' x='2.5894' y='13' fill='#e6e6e6' stroke='none' font-family='Droid Sans' text-anchor='start' font-size='7.3' >?</text>\n"
					"<circle fill='#8C8C8C' cx='[1.0022]' cy='5' r='3' stroke-width='0' />\n"
					"<text x='[1.0022]' y='6.7' font-family='Droid Sans' text-anchor='middle' font-weight='bold' stroke-width='0' font-size='5.5' >?</text>\n"      
					"%2\n"
					"</g>\n"
					"</svg>\n");

	QString svg = TextUtils::incrementTemplateString(header, 1, increment * (pins - 1), TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);

	QString repeat("<rect id='connector%1terminal' stroke='none' stroke-width='0' x='[1.87]' y='25.586' fill='#8C8C8C' width='2.3' height='2.0'/>\n"
					"<rect id='connector%1pin' stroke='none' stroke-width='0' x='[1.87]' y='23.336' fill='#8C8C8C' width='2.3' height='4.25'/>\n");

	QString repeats = TextUtils::incrementTemplateString(repeat, pins, increment, TextUtils::standardMultiplyPinFunction, TextUtils::standardCopyPinFunction, NULL);

	return svg.arg(TextUtils::getViewBoxCoord(svg, 2) / 100).arg(repeats);
}

bool MysteryPart::changePinLabels(bool singleRow, bool sip) {
	Q_UNUSED(singleRow);

	if (m_viewID != ViewLayer::SchematicView) return true;

	bool hasLocal = false;
	QStringList labels = getPinLabels(hasLocal);
	if (labels.count() == 0) return true;

    QTransform  transform = untransform();

	QString svg = MysteryPart::makeSchematicSvg(labels, sip);

    QString chipLabel = modelPart()->localProp("chip label").toString();
    if (!chipLabel.isEmpty()) {
        svg =TextUtils::replaceTextElement(svg, "label", chipLabel);
    }

    resetLayerKin(svg);

    retransform(transform);

	return true;
}

void MysteryPart::swapEntry(const QString & text) {

    FamilyPropertyComboBox * comboBox = qobject_cast<FamilyPropertyComboBox *>(sender());
    if (comboBox == NULL) return;

    QString layout = m_propsMap.value("layout");

    if (comboBox->prop().contains("layout", Qt::CaseInsensitive)) {
        layout = text;
    }

    if (layout.isEmpty()) {
        if (moduleID().contains("sip", Qt::CaseInsensitive)) {
            layout = "single";
        }
    }

    if (layout.contains("single", Qt::CaseInsensitive)) {
        generateSwap(text, genModuleID, genSipFZP, makeBreadboardSvg, makeSchematicSvg, PinHeader::makePcbSvg);
    }
    else {
        generateSwap(text, genModuleID, genDipFZP, makeBreadboardSvg, makeSchematicSvg, makePcbDipSvg);
    }
    PaletteItem::swapEntry(text);
}

QString MysteryPart::makePcbDipSvg(const QString & expectedFileName) 
{
    QString spacingString;
	int pins = TextUtils::getPinsAndSpacing(expectedFileName, spacingString);
    if (pins == 0) return "";  

	QString header("<?xml version='1.0' encoding='UTF-8'?>\n"
				    "<svg baseProfile='tiny' version='1.2' width='%1in' height='%2in' viewBox='0 0 %3 %4' xmlns='http://www.w3.org/2000/svg'>\n"
				    "<desc>Fritzing footprint SVG</desc>\n"
					"<g id='silkscreen'>\n"
					"<line stroke='white' stroke-width='10' x1='10' x2='10' y1='10' y2='%5'/>\n"
					"<line stroke='white' stroke-width='10' x1='10' x2='%6' y1='%5' y2='%5'/>\n"
					"<line stroke='white' stroke-width='10' x1='%6' x2='%6' y1='%5' y2='10'/>\n"
					"<line stroke='white' stroke-width='10' x1='10' x2='%7' y1='10' y2='10'/>\n"
					"<line stroke='white' stroke-width='10' x1='%8' x2='%6' y1='10' y2='10'/>\n"
					"</g>\n"
					"<g id='copper1'><g id='copper0'>\n"
					"<rect id='square' fill='none' height='55' stroke='rgb(255, 191, 0)' stroke-width='20' width='55' x='32.5' y='32.5'/>\n");

	double outerBorder = 10;
	double silkSplitTop = 100;
	double offsetX = 60;
	double offsetY = 60;
	double spacing = TextUtils::convertToInches(spacingString) * GraphicsUtils::StandardFritzingDPI; 
	double totalWidth = 120 + spacing;
	double totalHeight = (100 * pins / 2) + (outerBorder * 2);
	double center = totalWidth / 2;

	QString svg = header.arg(totalWidth / GraphicsUtils::StandardFritzingDPI).arg(totalHeight / GraphicsUtils::StandardFritzingDPI).arg(totalWidth).arg(totalHeight)
							.arg(totalHeight - outerBorder).arg(totalWidth - outerBorder)
							.arg(center - (silkSplitTop / 2)).arg(center + (silkSplitTop / 2));

	QString circle("<circle cx='%1' cy='%2' fill='none' id='connector%3pin' r='27.5' stroke='rgb(255, 191, 0)' stroke-width='20'/>\n");

	int y = offsetY;
	for (int i = 0; i < pins / 2; i++) {
		svg += circle.arg(offsetX).arg(y).arg(i);
		svg += circle.arg(totalWidth - offsetX).arg(y).arg(pins - 1 - i);
		y += 100;
	}

	svg += "</g></g>\n";
	svg += "</svg>\n";

    int hsix = expectedFileName.indexOf(HoleSizePrefix);
    if (hsix >= 0) {
        return hackSvgHoleSizeAux(svg, expectedFileName);
    }

	return svg;
}
