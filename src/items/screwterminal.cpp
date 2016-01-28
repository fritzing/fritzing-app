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

#include "screwterminal.h"
#include "../utils/graphicsutils.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../commands.h"
#include "../utils/textutils.h"
#include "../utils/schematicrectconstants.h"
#include "partlabel.h"
#include "partfactory.h"

#include <QDomNodeList>
#include <QDomDocument>
#include <QDomElement>
#include <QLineEdit>


static const int MinPins = 2;
static const int MaxPins = 20;
static QHash<QString, QString> Spacings;

static HoleClassThing TheHoleThing;

ScrewTerminal::ScrewTerminal( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: PaletteItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
    setUpHoleSizes("screwterminal", TheHoleThing);
}

ScrewTerminal::~ScrewTerminal() {
}

QStringList ScrewTerminal::collectValues(const QString & family, const QString & prop, QString & value) {
	if (prop.compare("pins", Qt::CaseInsensitive) == 0) {
		QStringList values;
		value = modelPart()->properties().value("pins");

		for (int i = MinPins; i <= MaxPins; i++) {
			values << QString::number(i);
		}
		
		return values;
	}

	if (prop.compare("pin spacing", Qt::CaseInsensitive) == 0) {
		QStringList values;
		value = modelPart()->properties().value("pin spacing");

		initSpacings();		
		return Spacings.values();
	}

	return PaletteItem::collectValues(family, prop, value);
}

ItemBase::PluralType ScrewTerminal::isPlural() {
	return Plural;
}

QString ScrewTerminal::genFZP(const QString & moduleid)
{
	initSpacings();

    QString useModuleID = moduleid;
    int hsix = useModuleID.lastIndexOf(HoleSizePrefix);
    if (hsix >= 0) useModuleID.truncate(hsix);

	QStringList pieces = useModuleID.split("_");

	QString result = PaletteItem::genFZP(useModuleID, "screw_terminal_fzpTemplate", MinPins, MaxPins, 1, false);
	result.replace(".percent.", "%");
	QString spacing = pieces.at(3);
	result = result.arg(spacing).arg(Spacings.value(spacing, ""));
    if (hsix >= 0) {
        return hackFzpHoleSize(result, moduleid, hsix);
    }

    return result;
}

QString ScrewTerminal::genModuleID(QMap<QString, QString> & currPropsMap)
{
	initSpacings();

	QString spacing = currPropsMap.value("pin spacing");
	QString pins = currPropsMap.value("pins");

	foreach (QString key, Spacings.keys()) {
		if (Spacings.value(key).compare(spacing, Qt::CaseInsensitive) == 0) {
			return QString("screw_terminal_%1_%2").arg(pins).arg(key);
		}
	}

	return "";
}

void ScrewTerminal::initSpacings() {
	if (Spacings.count() == 0) {
		Spacings.insert("3.5mm", "0.137in (3.5mm)");
		Spacings.insert("5.0mm", "0.197in (5.0mm)");
		Spacings.insert("100mil", "0.1in (2.54mm)");
		Spacings.insert("200mil", "0.2in (5.08mm)");
		Spacings.insert("300mil", "0.3in (7.62mm)");
	}
}

QString ScrewTerminal::makeBreadboardSvg(const QString & expectedFileName) 
{
	QStringList pieces = expectedFileName.split("_");

	int pins = pieces.at(2).toInt();
	double increment = 0.1;  // inches
	double incrementPoints = 100;

	QString header("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
					"<svg xmlns:svg='http://www.w3.org/2000/svg' xmlns='http://www.w3.org/2000/svg' version='1.2' baseProfile='tiny' width='%1in' height='0.17in' viewBox='0 0 [100] 170' >\n"
					"<g id='breadboard' >\n"
					"<polygon id='background' fill='#254E89' points='[96.012],39.9487 [96.012],0 0,0 0,39.9487 3.98829,39.9487 3.98829,72.4061 0,72.4061 0,157.607 [96.012],157.607 [96.012],72.4061 [100],72.4061 [100],39.9487' />\n"
					"<rect id='upperstripe' width='[92.745]' x='1.75594' y='2.38169' fill='#456DAB' height='5.76036' stroke-width='0' />\n"
					"<rect id='lowerrect' width='[96.012]' x='0' y='86.1147' fill='#123C66' height='71.4784' stroke-width='0' />\n");


	QString svg = TextUtils::incrementTemplateString(header.arg(increment * pins), 1, incrementPoints * (pins - 1), TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);
	svg += TextUtils::incrementTemplate(":/resources/templates/screw_terminal_bread_template.txt",
										pins, incrementPoints, TextUtils::standardMultiplyPinFunction, TextUtils::standardCopyPinFunction, NULL);
	svg += "</g>\n</svg>";

	return svg;
}

QString ScrewTerminal::makeSchematicSvg(const QString & expectedFileName) 
{
    if (expectedFileName.contains(PartFactory::OldSchematicPrefix)) {
        return obsoleteMakeSchematicSvg(expectedFileName);
    }     

	QStringList pieces = expectedFileName.split("_");

	int pins = pieces.at(2).toInt();			
	double increment = SchematicRectConstants::NewUnit / 25.4;
    double incrementPoints = 72 * increment;		// 72 dpi
    double pinWidthPoints = 72 * SchematicRectConstants::PinWidth / 25.4;
    double pinLength = 2 * increment;
    double pinLengthPoints = 2 * incrementPoints;

	QString header("<?xml version='1.0' encoding='utf-8'?>\n"
					"<svg version='1.1' baseProfile='basic' id='svg2' xmlns:svg='http://www.w3.org/2000/svg'\n"
					"xmlns='http://www.w3.org/2000/svg'  x='0px' y='0px' width='%1in'\n"
					"height='percent1in' viewBox='0 0 %2 [percent2]' xml:space='preserve'>\n"
					"<g id='schematic'>\n");
    header = header.arg(increment + pinLength).arg(incrementPoints + pinLengthPoints);
    header.replace("percent", "%");

	QString repeat("<line id='connectorpercent1pin' fill='none' stroke='%1' stroke-width='%2' stroke-linecap='round' stroke-linejoin='round' x1='%3' y1='[%5]' x2='%4' y2='[%5]'/>\n"
					"<rect id='connectorpercent1terminal' x='0' y='[%6]' width='0' height='%2'/>\n"
					"<circle fill='none' stroke-width='%2' stroke='%1' cx='%7' cy='[%5]' r='%8' />\n"
					);
    
    repeat = repeat
                .arg(SchematicRectConstants::PinColor)
                .arg(pinWidthPoints)
                .arg(pinWidthPoints / 2)
                .arg(pinLengthPoints - pinWidthPoints)
                .arg(incrementPoints / 2)
                .arg((incrementPoints - pinWidthPoints) / 2)
                .arg(pinLengthPoints + (incrementPoints / 2) - pinWidthPoints)
                .arg((incrementPoints / 2) - pinWidthPoints)
                ;
    repeat.replace("percent", "%");

	QString svg = TextUtils::incrementTemplateString(header.arg(increment * pins).arg(incrementPoints), 1, incrementPoints * (pins - 1), TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);
	svg += TextUtils::incrementTemplateString(repeat, pins, incrementPoints, TextUtils::standardMultiplyPinFunction, TextUtils::standardCopyPinFunction, NULL);
	svg += "</g>\n</svg>";

	return svg;
}
QString ScrewTerminal::obsoleteMakeSchematicSvg(const QString & expectedFileName) 
{
	QStringList pieces = expectedFileName.split("_");

	int pins = pieces.at(2).toInt();
	double increment = GraphicsUtils::StandardSchematicSeparationMils / 1000;				
	double incrementPoints = increment * 72;		// 72 dpi


	QString header("<?xml version='1.0' encoding='utf-8'?>\n"
					"<svg version='1.1' baseProfile='basic' id='svg2' xmlns:svg='http://www.w3.org/2000/svg'\n"
					"xmlns='http://www.w3.org/2000/svg'  x='0px' y='0px' width='0.87in'\n"
					"height='%1in' viewBox='0 0 62.641 [%2]' xml:space='preserve'>\n"
					"<g id='schematic'>\n");

	QString repeat("<line id='connector%1pin' fill='none' stroke='#000000' stroke-width='2' stroke-linecap='round' stroke-linejoin='round' x1='0.998' y1='[9.723]' x2='17.845' y2='[9.723]'/>\n"
					"<rect id='connector%1terminal' x='0' y='[8.725]' width='0.998' height='1.997'/>\n"
					"<circle fill='none' stroke-width='2' stroke='#000000' cx='52.9215' cy='[9.723]' r='8.7195' />\n"
					"<line id='line' fill='none' stroke='#000000' stroke-width='2' stroke-linecap='round' stroke-linejoin='round' x1='43.202' y1='[9.723]' x2='16.452' y2='[9.723]'/>\n");

	QString svg = TextUtils::incrementTemplateString(header.arg(increment * pins).arg(incrementPoints), 1, incrementPoints * (pins - 1), TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);
	svg += TextUtils::incrementTemplateString(repeat, pins, incrementPoints, TextUtils::standardMultiplyPinFunction, TextUtils::standardCopyPinFunction, NULL);
	svg += "</g>\n</svg>";

	return svg;
}

QString ScrewTerminal::makePcbSvg(const QString & originalExpectedFileName) 
{
    QString expectedFileName = originalExpectedFileName;
    int hsix = expectedFileName.indexOf(HoleSizePrefix);
    if (hsix >= 0) {
        expectedFileName.truncate(hsix);
    }

	QStringList pieces = expectedFileName.split("_");

	int pins = pieces.at(2).toInt();
	bool ok;
	double spacing = TextUtils::convertToInches(pieces.at(3), &ok, false);
	if (!ok) return "";

	double dpi  = 1000;
	double width = spacing < 0.15 ? 0.256 : 0.48;
	double verticalX = spacing < 0.15 ? 0.196 : 0.38;
	double centerX = (verticalX * dpi / 2) + 10;
	double initialY = (spacing * dpi / 2) + 25;

	QString header("<?xml version='1.0' encoding='UTF-8'?>\n"
					"<svg baseProfile='tiny' version='1.2' height='%3in' width='%1in' viewBox='0 0 %2 [40.0]' >\n"
					"<g id='silkscreen'>\n"
					"<line id='vertical-left' stroke='white' stroke-width='10' x1='20' x2='20' y1='20' y2='[30.0]'/>\n"
					"<line id='bottom' stroke='white' stroke-width='10' x1='20' x2='%4' y1='[30.0]' y2='[30.0]'/>\n"
					"<line id='vertical-right' stroke='white' stroke-width='10' x1='%4' x2='%4' y1='[30.0]' y2='20'/>\n"
					"<line id='top' stroke='white' stroke-width='10' x1='%4' x2='20' y1='20' y2='20'/>\n"
					"<line id='mid-vertical' stroke='white' stroke-width='5' x1='%5' x2='%5' y1='[30.0]' y2='20'/>\n"
					"</g>\n"
					"<g id='copper1'>\n"
					"<g id='copper0'>\n"
					"<rect id='square' width='60' height='60' x='%6' y='%7' fill='none' stroke='rgb(255, 191, 0)' stroke-width='20' />\n");

	QString repeat("<circle cx='%1' cy='%2' fill='none' id='connector%3pin' r='30' stroke='rgb(255, 191, 0)' stroke-width='20'/>\n");

	header = header.arg(width)
					.arg(width * dpi)
					.arg(pins * spacing + 0.04)
					.arg((width - 0.02) * dpi)
					.arg(verticalX * dpi)
					.arg(centerX - 30)
					.arg(initialY - 30);
	QString svg = TextUtils::incrementTemplateString(header, 1, pins * dpi * spacing, TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);
	for (int i = 0; i < pins; i++) {
		svg += repeat.arg(centerX).arg(initialY + (i * dpi * spacing)).arg(i);
	}

	svg += "</g>\n</g>\n</svg>\n";

    if (hsix >= 0) {
        return hackSvgHoleSizeAux(svg, originalExpectedFileName);
    }

	return svg;
}

bool ScrewTerminal::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide) 
{
	if (prop.compare("hole size", Qt::CaseInsensitive) == 0) {
        return collectHoleSizeInfo(TheHoleThing.holeSizeValue, parent, swappingEnabled, returnProp, returnValue, returnWidget);
	}

	return PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}

void ScrewTerminal::swapEntry(const QString & text) {
    generateSwap(text, genModuleID, genFZP, makeBreadboardSvg, makeSchematicSvg, makePcbSvg);
    PaletteItem::swapEntry(text);
}
