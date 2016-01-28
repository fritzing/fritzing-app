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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#include "schematicrectconstants.h"
#include "textutils.h"

// all measurements in millimeters

const double SchematicRectConstants::PinWidth = 0.246944;  // millimeters
const double SchematicRectConstants::PinSmallTextHeight = 0.881944444;
const double SchematicRectConstants::PinBigTextHeight = 1.23472222;
const double SchematicRectConstants::PinTextIndent = PinWidth * 2;   // was PinWidth * 3;
const double SchematicRectConstants::PinTextVert = PinWidth * 1;
const double SchematicRectConstants::PinSmallTextVert = -PinWidth / 2;
const double SchematicRectConstants::LabelTextHeight = 1.49930556;
const double SchematicRectConstants::LabelTextSpace = 0.1;
const double SchematicRectConstants::RectStrokeWidth = 0.3175;
const double SchematicRectConstants::NewUnit = 0.1 * 25.4;      // .1in in mm

const QString SchematicRectConstants::PinColor("#787878");
const QString SchematicRectConstants::PinTextColor("#8c8c8c");
const QString SchematicRectConstants::TitleColor("#000000");
const QString SchematicRectConstants::RectStrokeColor("#000000");
const QString SchematicRectConstants::RectFillColor("#FFFFFF");

const QString SchematicRectConstants::FontFamily("'Droid Sans'");


///////////////////////////////////////////

#include <QStringList>
#include <QFont>
#include <QFontMetricsF>
#include <qmath.h>

///////////////////////////////////////////


inline QString SW(qreal strokeWidth)
{
	return QString("%1").arg(strokeWidth);
}

QString schematicPinNumber(qreal x, qreal y, qreal pinSmallTextHeight, const QString & cid, bool rotate, bool gotZero, bool gotOne)
{
    if (!gotZero && !gotOne) return "";

    QString id = cid;
    if (gotZero) {
        id = QString::number(cid.toInt() + 1);
    }

    QString text;

    qreal useX = x;
    qreal offset = 1000 * (SchematicRectConstants::PinWidth - SchematicRectConstants::PinSmallTextVert) / 25.4;
    qreal useY = y - offset;

    if (rotate) {
		text += QString("<g transform='translate(%1,%2)'><g transform='rotate(%3)'>\n")
			.arg(useX - offset)
			.arg(useY + offset)
			.arg(270);
		useX = 0;
		useY = 0;
    }

    text += QString("<text class='text' font-family=%8 stroke='none' stroke-width='%6' fill='%7' font-size='%1' x='%2' y='%3' text-anchor='%4'>%5</text>\n")
					.arg(pinSmallTextHeight)
					.arg(useX)
					.arg(useY)
					.arg("middle")
					.arg(id)
					.arg(0)  // SW(width)
					.arg(SchematicRectConstants::PinTextColor) 
                    .arg(SchematicRectConstants::FontFamily)
                    ; 

    if (rotate) {
        text += "</g></g>\n";
    }

    return text;
}

QString schematicPinText(const QString & id, const QString & signal, qreal x, qreal y, qreal bigPinFontSize, const QString & anchor, bool rotate)
{
    QString text;
    if (rotate) {
        qreal yOffset = SchematicRectConstants::PinTextIndent + SchematicRectConstants::RectStrokeWidth;
        if (anchor == "start") yOffset = -yOffset;
        qreal xOffset = SchematicRectConstants::PinTextVert + SchematicRectConstants::PinWidth;
        xOffset = 1000 * xOffset / 25.4;
        yOffset = 1000 * yOffset / 25.4;
		text += QString("<g transform='translate(%1,%2)'><g transform='rotate(%3)'>\n")
			.arg(x + xOffset)
			.arg(y + yOffset)
			.arg(270);
		x = 0;
		y = 0;
        //xOffset = yOffset = 0;
    }

    text += QString("<text id='label%1' x='%5' y='%2' font-family=\"%7\" stroke='none' fill='%6' text-anchor='%8' font-size='%4' >%3</text>\n")
						.arg(id)
						.arg(y)
						.arg(TextUtils::escapeAnd(signal))
						.arg(bigPinFontSize)
						.arg(x)
                        .arg(SchematicRectConstants::PinTextColor)
                        .arg(SchematicRectConstants::FontFamily)
                        .arg(anchor)
                        ;

    if (rotate) {
        text += "</g></g>\n";
    }

    return text;
}


QString SchematicRectConstants::simpleGetConnectorName(const QDomElement & element) {
    return TextUtils::escapeAnd(element.attribute("name"));
}

QString SchematicRectConstants::genSchematicDIP(QList<QDomElement> & powers, QList<QDomElement> & grounds, QList<QDomElement> & lefts,
	            QList<QDomElement> & rights, QList<QDomElement> & vias, QStringList & busNames, 
                QString & boardName, bool usingParam, bool genericSMD, QString (*getConnectorName)(const QDomElement &)) 
{

	int powersBuses = 0, groundsBuses = 0, leftsBuses = 0, rightsBuses = 0;
	QList<QDomElement> busContacts;

    QList<QDomElement> all = powers;
    all.append(lefts);
    all.append(rights);
    all.append(grounds);
    bool gotZero = false;
    bool gotOne = false;
    foreach (QDomElement contact, all) {
        bool ok;
        if (contact.attribute("connectorIndex").toInt() == 1) gotOne = true;
        if (contact.attribute("connectorIndex").toInt(&ok) == 0 && ok) gotZero = true;
    }

	foreach (QDomElement contact, powers) {
		contact.setAttribute("ltrb", "power");
		if (contact.attribute("bus", 0).compare("1") == 0) {
			busContacts.append(contact);
			powersBuses++;
		}
	}
	foreach (QDomElement contact, grounds) {
		contact.setAttribute("ltrb", "ground");
		if (contact.attribute("bus", 0).compare("1") == 0) {
			busContacts.append(contact);
			groundsBuses++;
		}
	}
	foreach (QDomElement contact, lefts) {
		contact.setAttribute("ltrb", "left");
		if (contact.attribute("bus", 0).compare("1") == 0) {
			busContacts.append(contact);
			leftsBuses++;
		}
	}
	foreach (QDomElement contact, rights) {
		contact.setAttribute("ltrb", "right");
		if (contact.attribute("bus", 0).compare("1") == 0) {
			busContacts.append(contact);
			rightsBuses++;
		}
	}

	//foreach(QDomElement contact, busContacts) {
		//qDebug() << "sch bus" << contact.attribute("signal") << contact.attribute("name") << contact.attribute("connectorIndex");
	//}

	qreal bigFontSize = 1000 * SchematicRectConstants::PinBigTextHeight / 25.4;
	qreal bigPinFontSize = 1000 * SchematicRectConstants::PinBigTextHeight / 25.4;
	qreal smallPinFontSize = 1000 * SchematicRectConstants::PinSmallTextHeight / 25.4;
	qreal pinThickness = 1000 * SchematicRectConstants::PinWidth / 25.4;
	qreal halfPinThickness = pinThickness / 2;
	qreal unitLength = 1000 * SchematicRectConstants::NewUnit / 25.4;
	qreal pinLength = 2 * unitLength;
    qreal pinTextIndent = 1000 * SchematicRectConstants::PinTextIndent / 25.4;
    qreal pinTextVert = 1000 * SchematicRectConstants::PinTextVert / 25.4;
    qreal rectThickness = 1000 * SchematicRectConstants::RectStrokeWidth / 25.4;

	QStringList titles;
	if (genericSMD) {
		titles.append(boardName);
	}
	else {
		boardName.replace(" - ", " ");
		boardName.replace("_", " ");
		titles = boardName.split(" ");
		if (!usingParam) {
			QRegExp version("^[vV][\\d]+");
			if (titles.last().contains(version)) {
				titles.takeLast();
			}
		}
	}
	qreal titleWidth = 0;
	qreal leftWidth = 0;
	qreal rightWidth = 0;
    qreal topWidth = 0;
    qreal bottomWidth = 0;
	QFont bigFont("DroidSans");
	bigFont.setPointSizeF(bigFontSize * 72 / 1000.0);
	QFontMetricsF bigFontMetrics(bigFont);
	foreach (QString title, titles) {
		qreal w = bigFontMetrics.width(title);
		if (w > titleWidth) titleWidth = w;
	}
	QFont pinTextFont("DroidSans");
	pinTextFont.setPointSizeF(bigPinFontSize * 72 / 1000.0);
	QFontMetricsF smallFontMetrics(pinTextFont);
	foreach (QDomElement element, lefts) {
		qreal w = smallFontMetrics.width(getConnectorName(element));
		if (w > leftWidth) leftWidth = w;
	}
	foreach (QDomElement element, rights) {
		qreal w = smallFontMetrics.width(getConnectorName(element));
		if (w > rightWidth) rightWidth = w;
	}
	foreach (QDomElement element, powers) {
		qreal w = smallFontMetrics.width(getConnectorName(element));
		if (w > topWidth) topWidth = w;
	}
	foreach (QDomElement element, grounds) {
		qreal w = smallFontMetrics.width(getConnectorName(element));
		if (w > bottomWidth) bottomWidth = w;
	}

	// title is symmetric so leave max room on both sides
	if (leftWidth > rightWidth && rightWidth > 0) rightWidth = leftWidth;
	if (rightWidth > leftWidth && leftWidth > 0) leftWidth = rightWidth;

	// convert from pixels (which seem to be points) back to mils
	leftWidth = 1000 * leftWidth / 72.0;
	rightWidth = 1000 * rightWidth / 72.0;
	titleWidth = 1000 * titleWidth / 72.0;
	topWidth = 1000 * topWidth / 72.0;
	bottomWidth = 1000 * bottomWidth / 72.0;

	qreal textWidth = (leftWidth + rightWidth + titleWidth + (rightWidth > 0 ? pinTextIndent : 0) + (leftWidth > 0 ? pinTextIndent : 0)); 
    qreal width = (qCeil(textWidth / unitLength) * unitLength);
    if (lefts.count() > 0) width += pinLength;
    if (rights.count() > 0) width += pinLength;
	int room = qFloor(width / unitLength) - 4;
	if (powers.count() - powersBuses > room) width += (pinLength * (powers.count() - powersBuses - room));
	if (grounds.count() - groundsBuses > room) width += (pinLength * (grounds.count() - groundsBuses - room));

	qreal rectTop = rectThickness / 2;
	qreal height = 0;
    qreal startTitle = 0;
	if (powers.count() > 0) {
        startTitle = pinLength + qCeil(topWidth / unitLength) * unitLength;
		height += startTitle;
		rectTop += pinLength - rectThickness / 2;
	}
    else {
        height += unitLength;
        startTitle = unitLength;
    }
	qreal rectBottom = rectThickness / 2;
	if (grounds.count() > 0) {
        qreal dh = pinLength + qCeil(bottomWidth / unitLength) * unitLength;
		rectBottom += pinLength - (rectThickness / 2);
		height += dh;
	}
    else height += unitLength;
    int l1 = qMax(0, lefts.count() - leftsBuses - 1);
    l1 = qMax(l1,  rights.count() - rightsBuses - 1);
    l1 = qMax(l1, qCeil(titles.count() * bigFontSize / unitLength));
	height += l1 * unitLength;

	QString svg = TextUtils::makeSVGHeader(1000, 1000, width, height);
	svg += "<desc>Fritzing schematic generated by brd2svg</desc>\n";
	svg += "<g id='schematic'>\n";
    qreal rectLeft = rectThickness / 2;
    qreal rectMinus = 0;
    if (lefts.count()) {
        rectLeft = pinLength;
        rectMinus += pinLength;
    }
    if (rights.count()) {
        rectMinus += pinLength;
    }

	svg += QString("<rect x='%4' y='%3' fill='none' width='%1' height='%2' stroke='#000000' stroke-linejoin='round' stroke-linecap='round' stroke-width='%5' />\n")
						.arg(width - rectMinus - (rectThickness / 2))
						.arg(height - rectBottom - rectTop)
						.arg(rectTop)
						.arg(rectLeft)
						.arg(SW(rectThickness));
	
	qreal y = qFloor((height / 2) / unitLength) * unitLength;
    y -= bigFontSize * qCeil(titles.count() / 2.0);
    if (y < startTitle) y = startTitle;
	foreach (QString title, titles) {
		svg += QString("<text id='label' x='%1' y='%3' font-family=\"%5\" stroke='none' fill='#000000' text-anchor='middle' font-size='%4' >%2</text>\n")
						.arg(rectLeft + (width - rectMinus) / 2)
						.arg(TextUtils::escapeAnd(title))
						.arg(y)
						.arg(bigFontSize)
                        .arg(SchematicRectConstants::FontFamily)
                        ;
		y += bigFontSize;
	}

	QHash<QString, QString> busMids;


	// do vias first, so they are hidden beneath other connectors

	qreal viaX = 0;
	qreal viaY = 0;
	foreach (QDomElement contact, vias) {
		QString signal = getConnectorName(contact);
		svg += QString("<rect viax='' viay='' fill='none' width='0' height='0' id='connector%1pin' connectorName='%2' stroke-width='0' stroke='none'/>\n")
					.arg(contact.attribute("connectorIndex")).arg(signal);	
		svg += QString("<rect viax='' viay='' fill='none' width='0' height='0' id='connector%1terminal' stroke-width='0' stroke='none'/>\n")
					.arg(contact.attribute("connectorIndex"));
	}

	qreal ly = startTitle;
	foreach (QDomElement contact, lefts) {
		bool bus = contact.attribute("bus", 0).compare("1") == 0;
		if (!contact.isNull() && !bus) {
			QString signal = getConnectorName(contact);
			svg += QString("<line fill='none' stroke='%5' stroke-linejoin='round' stroke-linecap='round' stroke-width='%2' x1='%4' y1='%1' x2='%3' y2='%1' />\n")
						.arg(ly).arg(SW(pinThickness)).arg(pinLength).arg(halfPinThickness).arg(SchematicRectConstants::PinColor);
			viaY = ly;
			viaX = halfPinThickness;

			QString mid = QString("<rect x='0' y='%1' fill='none' width='%3' height='%4' id='connector%2pin' connectorName='%5' stroke-width='0' />\n")
						.arg(ly - halfPinThickness).arg(contact.attribute("connectorIndex")).arg(pinLength).arg(pinThickness).arg(signal);	
			mid += QString("<rect x='0' y='%1' fill='none' width='0' height='%3' id='connector%2terminal' stroke-width='0' />\n")
						.arg(ly - halfPinThickness).arg(contact.attribute("connectorIndex")).arg(pinThickness);
			svg += mid;
			if (busNames.contains(contact.attribute("signal"), Qt::CaseInsensitive)) {
				busMids.insert(contact.attribute("signal").toLower(), mid);
			}
			svg += schematicPinText(contact.attribute("connectorIndex"), signal, pinLength + pinTextIndent, ly + pinTextVert, bigPinFontSize, "start", false);
            svg += schematicPinNumber(pinLength / 2, ly, smallPinFontSize, contact.attribute("connectorIndex"), false, gotZero, gotOne);
		}

		if (!bus) ly += unitLength;
	}
	
	ly = startTitle;
	foreach (QDomElement contact, rights) {
		bool bus = contact.attribute("bus", 0).compare("1") == 0;
		if (!contact.isNull() && !bus) {
			QString signal = getConnectorName(contact);
			svg += QString("<line fill='none' stroke='%5' stroke-linejoin='round' stroke-linecap='round' stroke-width='%4' x1='%1' y1='%2' x2='%3' y2='%2' />\n")
						.arg(width - pinLength - halfPinThickness)
						.arg(ly)
						.arg(width - halfPinThickness)
						.arg(SW(pinThickness))
                        .arg(SchematicRectConstants::PinColor)
                        ;

			viaY = ly;
			viaX = width - pinLength - halfPinThickness;

			QString mid = QString("<rect x='%1' y='%2' fill='none' width='%4' height='%5' id='connector%3pin' connectorname='%6' stroke-width='0' />\n")
						.arg(width - pinLength)
						.arg(ly - halfPinThickness)
						.arg(contact.attribute("connectorIndex"))
						.arg(pinLength)
						.arg(pinThickness)
						.arg(signal);	
			mid += QString("<rect x='%1' y='%2' fill='none' width='0' height='%4' id='connector%3terminal' stroke-width='0' />\n")
						.arg(width)
						.arg(ly - halfPinThickness)
						.arg(contact.attribute("connectorIndex"))
						.arg(pinThickness);
			svg += mid;
			if (busNames.contains(contact.attribute("signal"), Qt::CaseInsensitive)) {
				busMids.insert(contact.attribute("signal").toLower(), mid);
			}
			svg += schematicPinText(contact.attribute("connectorIndex"), signal, width - pinLength - pinTextIndent, ly + pinTextVert, bigPinFontSize, "end", false);
            svg += schematicPinNumber(width - (pinLength / 2) , ly, smallPinFontSize, contact.attribute("connectorIndex"), false, gotZero, gotOne);
		}

		if (!bus) ly += unitLength;
	}

	qreal lx = width / 2;
	if (powers.count() - powersBuses > 2) {
		lx -= (unitLength * (powers.count() - powersBuses) / 2);
	}
    lx = qFloor(lx / unitLength) * unitLength;
	foreach (QDomElement contact, powers) {
		bool bus = contact.attribute("bus", 0).compare("1") == 0;
		if (!contact.isNull() && !bus) {
			QString signal = getConnectorName(contact);
			svg += QString("<line fill='none' stroke='%5' stroke-linejoin='round' stroke-linecap='round' stroke-width='%4' x1='%1' y1='%2' x2='%1' y2='%3' />\n")
						.arg(lx).arg(halfPinThickness).arg(pinLength).arg(SW(pinThickness)).arg(SchematicRectConstants::PinColor);
			viaY = halfPinThickness;
			viaX = lx;

			QString mid = QString("<rect x='%1' y='%2' fill='none' width='%4' height='%5' id='connector%3pin' connectorname='%6' stroke-width='0' />\n")
						.arg(lx - halfPinThickness).arg(0).arg(contact.attribute("connectorIndex")).arg(pinThickness).arg(pinLength).arg(signal);	
			mid += QString("<rect x='%1' y='%2' fill='none' width='%4' height='0' id='connector%3terminal' stroke-width='0' />\n")
						.arg(lx - halfPinThickness).arg(0).arg(contact.attribute("connectorIndex")).arg(pinThickness);
			svg += mid;
			if (busNames.contains(contact.attribute("signal"), Qt::CaseInsensitive)) {
				busMids.insert(contact.attribute("signal").toLower(), mid);
			}
            svg += schematicPinText(contact.attribute("connectorIndex"), signal, lx, pinLength - pinTextVert, bigPinFontSize, "end", true);
            svg += schematicPinNumber(lx, pinLength / 2, smallPinFontSize, contact.attribute("connectorIndex"), true, gotZero, gotOne);
		}

		if (!bus) lx += unitLength;
	}

	lx = width / 2;
	if (grounds.count() - groundsBuses > 2) {
		lx -= (unitLength * (grounds.count() - groundsBuses) / 2);
	}
    lx = qFloor(lx / unitLength) * unitLength;
	foreach (QDomElement contact, grounds) {
		bool bus = contact.attribute("bus", 0).compare("1") == 0;
		if (!contact.isNull() && !bus) {
			QString signal = getConnectorName(contact);
			svg += QString("<line fill='none' stroke='%5' stroke-linejoin='round' stroke-linecap='round' stroke-width='%4' x1='%1' y1='%2' x2='%1' y2='%3' />\n")
						.arg(lx).arg(height - pinLength).arg(height - halfPinThickness).arg(SW(pinThickness)).arg(SchematicRectConstants::PinColor);
			viaY = height - pinLength;
			viaX = lx;

			QString mid = QString("<rect x='%1' y='%2' fill='none' width='%4' height='%5' id='connector%3pin' connectorname='%6' stroke-width='0' />\n")
						.arg(lx - halfPinThickness).arg(height - pinLength).arg(contact.attribute("connectorIndex")).arg(pinThickness).arg(pinLength).arg(signal);	
			mid += QString("<rect x='%1' y='%2' fill='none' width='%4' height='0' id='connector%3terminal' stroke-width='0' />\n")
						.arg(lx - halfPinThickness).arg(height).arg(contact.attribute("connectorIndex")).arg(pinThickness);
			svg += mid;
			if (busNames.contains(contact.attribute("signal"), Qt::CaseInsensitive)) {
				busMids.insert(contact.attribute("signal").toLower(), mid);
			}

            svg += schematicPinText(contact.attribute("connectorIndex"), signal, lx, height - (pinLength - pinTextVert), bigPinFontSize, "start", true);
            svg += schematicPinNumber(lx, height - (pinLength / 2), smallPinFontSize, contact.attribute("connectorIndex"), true, gotZero, gotOne);
		}

		if (!bus) lx += unitLength;
	}

	svg.replace("viax=''", QString("x='%1'").arg(viaX));
	svg.replace("viay=''", QString("y='%1'").arg(viaY));

	QRegExp pin("connector[\\d]+pin");
	QRegExp terminal("connector[\\d]+terminal");
	foreach (QDomElement contact, busContacts) {
		QString mid = busMids.value(contact.attribute("signal").toLower(), "");
		if (mid.isEmpty()) continue;

		QString index = contact.attribute("connectorIndex");
		mid.replace(pin, QString("connector%1pin").arg(index));
		mid.replace(terminal, QString("connector%1terminal").arg(index));
		//qDebug() << "got bus connector" << index << contact.attribute("signal") << contact.attribute("connectorIndex");
		svg += mid;
	}

	svg += "</g>\n";
	svg += "</svg>\n";
	return svg;

}

