#include "miscutils.h"
#include <QtDebug>
#include <qmath.h>
#include "utils/textutils.h"
#include "svg/svgfilesplitter.h"

///////////////////////////////////////////////////////



WireTree::WireTree(QDomElement & w) 
{
    element = w;
	sweep = 0;
    failed = false;
	MiscUtils::x1y1x2y2(w, x1, y1, x2, y2);
	curve = w.attribute("curve", "0").toDouble();
	//qDebug() << "wiretree" << this << x1 << y1 << x2 << y2 << curve;

	width = w.attribute("width", "");

	if (curve != 0) {
		QDomElement piece = w.firstChildElement("piece");
		if (!piece.isNull()) {
			QDomElement arc = piece.firstChildElement("arc");

			if (!arc.isNull()) {
				MiscUtils::x1y1x2y2(arc, ax1, ay1, ax2, ay2);
				qreal width;
				MiscUtils::rwaa(arc, radius, width, angle1, angle2);
				oa1 = angle1;
				oa2 = angle2;
				if (ax1 != x1 || ay1 != y1) {
					// qDebug() << "eagle exchanged the arc";
					sweep = 1;
					angle2 = oa1;
					angle1 = oa2;
				}
			}
		}
	}
	left = right = NULL;
}

void WireTree::turn() {
	qreal temp = x1;
	x1 = x2;
	x2 = temp;
	temp = y1;
	y1 = y2;
	y2 = temp;
	if (curve) {
		resetArc();
		// qDebug() << "flipping arc";
	}
}

void WireTree::resetArc() {
	angle1 = oa1;
	angle2 = oa2;
	sweep = 0;
	if (ax1 != x1 || ay1 != y1) {
		// qDebug() << "eagle exchanged the arc";
		sweep = 1;
		angle2 = oa1;
		angle1 = oa2;
	}
}

////////////////////////////////////////////

bool MiscUtils::makePartsDirectories(const QDir & workingFolder, const QString & core, QDir & fzpFolder, QDir & breadboardFolder, QDir & schematicFolder, QDir & pcbFolder, QDir & iconFolder) {
	workingFolder.mkdir("parts");

	QDir partsFolder(workingFolder.absolutePath());
	partsFolder.cd("parts");
	if (!partsFolder.exists()) {
		qDebug() << QString("unable to create parts folder:%1").arg(partsFolder.absolutePath());
		return false;
	}

	partsFolder.mkdir("svg");
	partsFolder.mkdir(core);

	fzpFolder = partsFolder;
	fzpFolder.cd(core);
	if (!fzpFolder.exists()) {
		qDebug() << QString("unable to create fzp folder:%1").arg(fzpFolder.absolutePath());
		return false;
	}

	if (!partsFolder.cd("svg")) {
		qDebug() << QString("unable to create svg folder:%1").arg(partsFolder.absolutePath());
		return false;
	}

	partsFolder.mkdir(core);
	if (!partsFolder.cd(core)) {
		qDebug() << QString("unable to create svg %1 folder:%2").arg(core).arg(partsFolder.absolutePath());
		return false;
	}
	
	partsFolder.mkdir("breadboard");
	partsFolder.mkdir("schematic");
	partsFolder.mkdir("pcb");
	partsFolder.mkdir("icon");

	breadboardFolder = partsFolder;
	breadboardFolder.cd("breadboard");
	if (!breadboardFolder.exists()) {
		qDebug() << QString("unable to create breadboard folder:%1").arg(breadboardFolder.absolutePath());
		return false;
	}

	schematicFolder = partsFolder;
	schematicFolder.cd("schematic");
	if (!schematicFolder.exists()) {
		qDebug() << QString("unable to create schematic folder:%1").arg(schematicFolder.absolutePath());
		return false;
	}

	pcbFolder = partsFolder;
	pcbFolder.cd("pcb");
	if (!pcbFolder.exists()) {
		qDebug() << QString("unable to create pcb folder:%1").arg(pcbFolder.absolutePath());
		return false;
	}

	iconFolder = partsFolder;
	iconFolder.cd("icon");
	if (!iconFolder.exists()) {
		qDebug() << QString("unable to create icon folder:%1").arg(iconFolder.absolutePath());
		return false;
	}

	return true;
}

void MiscUtils::calcTextAngle(qreal & angle, int mirror, int spin, qreal size, qreal & x, qreal & y, bool & anchorAtStart)
{
	bool anchorAtTop;

	if ( mirror > 0 ) {
		if (angle >= 0 && angle < 90) {
			anchorAtStart=false; anchorAtTop=false; 
		}
		else if (angle >= 90 && angle < 180) {
			anchorAtStart=true; anchorAtTop=true;
		}
		else if (angle >= 180 && angle < 270) {
			anchorAtStart=true; anchorAtTop=true;
		}
		else if (angle >= 270 && angle < 360) {
			anchorAtStart=false; anchorAtTop=false; 
		}
		else {
			qDebug() << "bad angle in gen text" << angle;
			return;
		} 
	} 
	else 
	{
		if (angle >= 0 && angle <= 90) {
			anchorAtStart=true; anchorAtTop=false;
		}
		else if (angle > 90 && angle < 360) {
			anchorAtStart=false; anchorAtTop=true;
		}
		else {
			qDebug() << "bad angle in gen text" << angle;
			return;
		} 
	}

	if (angle == 0) {
	}
	else if (angle > 0 && angle <= 90) {   // angle >= 0 && angle < 90
		angle = 360 - angle;
	}
	else if (angle > 90 && angle <= 180) {
		if (anchorAtTop)  {
			y -= size;
		}
		angle = 180 - angle;
	}
	else if (angle > 180 && angle < 270) {
		if (anchorAtTop)  {
			y -= size;
		}
		angle = 180 - angle;
	} 
	else 
	{
		if (anchorAtTop) {
			x += size;
		}

		angle = angle - 360;
	}

	if (spin == 1) {
		if (angle == 0) {
		}
		else if (angle > 0 && angle < 90) {
			anchorAtStart = !anchorAtStart;
			angle += 180;
			if (anchorAtTop)  {
				y += size;
			}		
		}
		else {
			if (angle >= 180 && angle < 270) {
				if (anchorAtTop)  {
					y += size;
				}
			}
			else {
				if (anchorAtTop) {
					x -= size;
				}
			}

			anchorAtStart = !anchorAtStart;
			angle += 180;
		}
	}
}


QString MiscUtils::makeGeneric(const QDir & workingFolder, const QString & boardColor, QList<QDomElement> & powers, 
								QString & copper, const QString & boardName, QSizeF outerChipSize, QSizeF innerChipSize,
								GetConnectorNameFn getConnectorName, GetConnectorNameFn getConnectorIndex, bool noText)
{
    // assumes 1000 dpi

    bool includeChip = innerChipSize.width() > 0 && innerChipSize.height() > 0;

	copper.remove(QRegExp("id=.connector[\\d]*p..."));

	int halfPowers = powers.count() / 2;
	qreal width = halfPowers * 100;		// width in mils; assume we always have an even number

	qreal chipDivisor = includeChip ? 2 : 1;
    if (outerChipSize.width() / chipDivisor > width - 200) {
        width = (outerChipSize.width() / chipDivisor) + 200;
    }

	//qreal chipHeight = qSqrt((bounds.width() * bounds.width()) + (bounds.height() * bounds.height()));
	qreal chipHeight = outerChipSize.height() / chipDivisor;
	chipHeight += noText ? 200 : 300;			// add room for pins and pin labels
	chipHeight = 200 * qCeil(chipHeight / 200);		// ensure an even number of rows

	// add textHeight

	// TODO: if the chip width is too wide find another factor to make it fit


	qreal height = qMax(400.0, chipHeight);				

	QString svg = TextUtils::makeSVGHeader(1000, 1000, width, height);
	svg += "<desc>Fritzing breadboard generated by brd2svg</desc>\n";
	svg += "<g id='breadboard'>\n";
	svg += "<g id='icon'>\n";						// make sure we can use this image for icon view


	svg += QString("<path fill='%1' stroke='none' d='M0,0L0,%2A30,30 0 0 1 0,%3L0,%4L%5,%4L%5,0L0,0z' stroke-width='0'/>\n")
				.arg(boardColor)
				.arg((height / 2.0) - 30)
				.arg((height / 2.0) + 30)
				.arg(height)
				.arg(width);



	qreal subx = outerChipSize.width() / 2;
	qreal suby = outerChipSize.height() / 2;
    if (!includeChip) {
	    svg += QString("<g transform='translate(%1,%2)'>\n")
		    .arg((width / 2) - subx)
		    .arg((height / 2) - suby);
        svg += TextUtils::removeSVGHeader(copper);
        svg += "</g>\n";
    }
    else {
	    QMatrix matrix;
	    matrix.translate(subx, suby);
	    //matrix.rotate(45);
	    matrix.scale(1 / chipDivisor, 1 / chipDivisor);
	    matrix.translate(-subx, -suby);  
	    svg += QString("<g transform='translate(%1,%2)'>\n")
		    .arg((width / 2) - subx)
		    .arg((height / 2) - suby);
	    svg += QString("<g transform='%1'>\n").arg(TextUtils::svgMatrix(matrix));

	    svg += TextUtils::removeSVGHeader(copper);

	    qreal icLeft = (outerChipSize.width() - innerChipSize.width()) / 2;
	    qreal icTop = (outerChipSize.height() - innerChipSize.height()) / 2;
	    svg += QString("<rect x='%1' y='%2' fill='#303030' width='%3' height='%4' stroke='none' stroke-width='0' />\n")
						    .arg(icLeft)
						    .arg(icTop)
						    .arg(innerChipSize.width())
						    .arg(innerChipSize.height());
	    svg += QString("<polygon fill='#1f1f1f' points='%1,%2 %3,%2 %4,%5 %6,%5' />\n")
						    .arg(icLeft)
						    .arg(icTop)
						    .arg(icLeft + innerChipSize.width())
						    .arg(icLeft + innerChipSize.width() - 10)
						    .arg(icTop + 10)
						    .arg(icLeft + 10);
	    svg += QString("<polygon fill='#1f1f1f' points='%1,%2 %3,%2 %4,%5 %6,%5' />\n")
						    .arg(icLeft)
						    .arg(icTop + innerChipSize.height())
						    .arg(icLeft + innerChipSize.width())
						    .arg(icLeft + innerChipSize.width() - 10)
						    .arg(icTop + innerChipSize.height() - 10)
						    .arg(icLeft + 10);
	    svg += QString("<polygon fill='#000000' points='%1,%2 %1,%3 %4,%5 %4,%6' />\n")
						    .arg(icLeft)
						    .arg(icTop)
						    .arg(icTop + innerChipSize.height())
						    .arg(icLeft + 10)
						    .arg(icTop + innerChipSize.height() - 10)
						    .arg(icTop +  10);
	    svg += QString("<polygon fill='#3d3d3d' points='%1,%2 %1,%3 %4,%5 %4,%6' />\n")
						    .arg(icLeft + innerChipSize.width())
						    .arg(icTop)
						    .arg(icTop + innerChipSize.height())
						    .arg(icLeft + innerChipSize.width() - 10)
						    .arg(icTop + innerChipSize.height() - 10)
						    .arg(icTop +  10);
	    svg += QString("<circle fill='#1f1f1f' cx='%1' cy='%2' r='10' stroke='none' stroke-width='0' />\n")
				    .arg(icLeft + 10 + 10 + 10)
				    .arg(icTop + innerChipSize.height() - 10 - 10 - 10);

    }

	svg += "</g>\n";		// chip xform
	svg += "</g>\n";		// chip xform

    if (!noText) {
	    qreal fontSize = 65;
	    svg += QString("<text id='label' font-family='OCRA' stroke='none' stroke-width='0' fill='white' font-size='%1' x='%2' y='%3' text-anchor='%4'>%5</text>\n")
							    .arg(fontSize)
							    .arg(50)
							    .arg((height / 2.0) + (fontSize / 2) - (fontSize / 8))
							    .arg("start")
							    .arg(boardName);
    }

	bool gotIncludes = false;
	qreal sWidth, sHeight;
	QDir includesFolder(workingFolder);
	includesFolder.cdUp();
	includesFolder.cd("includes");
	if (includesFolder.exists()) {
		QString errorStr;
		int errorLine;
		int errorColumn;
		QDomDocument pinDoc;
		QFile file(includesFolder.absoluteFilePath("bb_pin.svg"));
		if (pinDoc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
			qreal vbWidth, vbHeight;
			TextUtils::getSvgSizes(pinDoc, sWidth, sHeight, vbWidth, vbHeight);
			gotIncludes = true;
		}
	}

    double xOffset = (width - (halfPowers * 100)) / 2;
	if (gotIncludes) {
		qreal fontSize = 45;
		for (int i = 0; i < halfPowers; i++) {
			qreal x = (i * 100) + 50 + (fontSize / 2) - (fontSize / 5);
			qreal y = height - 50 - 8 - (sHeight * 1000 / 2); 
	
			QDomElement power = powers.at(i);
            if (power.attribute("empty", "").isEmpty() && !noText) {
			    svg += QString("<g transform='translate(%1,%2)'><g transform='rotate(%3)'>\n")
				    .arg(x + xOffset)
				    .arg(y)
				    .arg(-90);
			    svg += QString("<text font-family='OCRA' stroke='none' stroke-width='0' fill='white' font-size='%1' x='%2' y='%3' text-anchor='%4'>%5</text>\n")
							    .arg(fontSize)
							    .arg(0)
							    .arg(0)
							    .arg("start")
							    .arg(TextUtils::escapeAnd(getConnectorName(power)));
			    svg += "</g></g>\n";
            }

			y = 50 + 8 + (sHeight * 1000 / 2);
			power = powers.at(powers.count() - 1 - i);
            if (power.attribute("empty", "").isEmpty() && !noText) {
			    svg += QString("<g transform='translate(%1,%2)'><g transform='rotate(%3)'>\n")
				    .arg(x + xOffset)
				    .arg(y)
				    .arg(-90);
			    svg += QString("<text font-family='OCRA' stroke='none' stroke-width='0' fill='white' font-size='%1' x='%2' y='%3' text-anchor='%4'>%5</text>\n")
							    .arg(fontSize)
							    .arg(0)
							    .arg(0)
							    .arg("end")
							    .arg(TextUtils::escapeAnd(getConnectorName(power)));
			    svg += "</g></g>\n";
            }

		}
	}

	svg += "</g>\n";		// icon
	svg += "</g>\n";		// breadboard
	svg += "</svg>\n";

	if (!gotIncludes) return svg;

	QDomDocument doc;
	TextUtils::mergeSvg(doc, svg, "breadboard");

	for (int i = 0; i < halfPowers; i++) {
		qreal x = (i * 100) + 50 - (sWidth * 1000 / 2);
		QDomElement power = powers.at(i);
		qreal y = height - 50 - (sHeight * 1000 / 2);
        if (power.attribute("empty", "").isEmpty()) {
		    includeSvg2(doc, includesFolder.absoluteFilePath("bb_pin.svg"), getConnectorIndex(power), x + xOffset, y);
        }
	
		power = powers.at(powers.count() - 1 - i);
        if (power.attribute("empty", "").isEmpty()) {
		    includeSvg2(doc, includesFolder.absoluteFilePath("bb_pin.svg"), getConnectorIndex(power), x + xOffset, 50 - (sHeight * 1000 / 2));
        }
	}

	svg = TextUtils::mergeSvgFinish(doc);

	return svg;
}

void MiscUtils::includeSvg2(QDomDocument & doc, const QString & path, const QString & name, qreal x, qreal y) {
	QFile file(path);
	if (!file.exists()) {
		qDebug() << "file '" << path << "' not found.";
		return;
	}

	SvgFileSplitter splitter;
	if (!splitter.load(&file)) return;

	QDomDocument subDoc = splitter.domDocument();
	QDomElement root = subDoc.documentElement();
	QDomElement child = TextUtils::findElementWithAttribute(root, "id", "connectorXpin");
	if (!child.isNull()) {
		child.setAttribute("id", name);
	}

    double factor;
	splitter.normalize(1000, "", false, factor);
	QHash<QString, QString> attributes;
	attributes.insert("transform", QString("translate(%1,%2)").arg(x).arg(y));
	splitter.gWrap(attributes);
	TextUtils::mergeSvg(doc, splitter.toString(), "breadboard");
}

bool MiscUtils::makeWireTrees(QList<QDomElement> & wireList, QList<WireTree *> & wireTrees) 
{
	foreach (QDomElement wire, wireList) {
		WireTree * wireTree = new WireTree(wire);
		wireTrees.append(wireTree);
	}

	QList<WireTree *> failed;

	foreach (WireTree * wireTree, wireTrees) {
		if (!wireTree->left) {
			foreach (WireTree * wt, wireTrees) {
				if (wt == wireTree) continue;

				if (wireTree->x1 == wt->x1 && wireTree->y1 == wt->y1) {
					if (wt->left == NULL) {
						wireTree->left = wt;
						wt->left = wireTree;
						break;
					}
				}
				if (wireTree->x1 == wt->x2 && wireTree->y1 == wt->y2) {
					if (wt->right == NULL) {
						wireTree->left = wt;
						wt->right = wireTree;
						break;
					}
				}
			}
		}
		if (!wireTree->right) {
			foreach (WireTree * wt, wireTrees) {
				if (wt == wireTree) continue;

				if (wireTree->x2 == wt->x1 && wireTree->y2 == wt->y1) {
					if (wt->left == NULL) {
						wireTree->right = wt;
						wt->left = wireTree;
						break;
					}
				}
				if (wireTree->x2 == wt->x2 && wireTree->y2 == wt->y2) {
					if (wt->right == NULL) {
						wireTree->right = wt;
						wt->right = wireTree;
						break;
					}
				}
			}
		}

		if (wireTree->left == NULL || wireTree->right == NULL)  {
			failed << wireTree;
		}
	}

	if (failed.count() > 0) {
		foreach (WireTree * wt, failed) {

			QString info = QString("x1:%1 y1:%2 x2:%3 y2:%4 c:%5").arg(wt->x1).arg(wt->y1).arg(wt->x2).arg(wt->y2).arg(wt->curve);
			//qreal radius, angle1, angle2;
			//qDebug() << "wiretree failure" << wt << wt->left << wt->right << info;
            wt->failed = true;
		}
		return false;
	}

	return true;
}


bool MiscUtils::rwaa(QDomElement & element, qreal & radius, qreal & width, qreal & angle1, qreal & angle2)
{
	bool ok = true;
	radius = strToMil(element.attribute("r", ""), ok);
	if (!ok) return false;

	width = strToMil(element.attribute("width", ""), ok);
	if (!ok) return false;

	angle1 = element.attribute("angle1", "").toDouble(&ok);
	if (!ok) return false;

	angle2 = element.attribute("angle2", "").toDouble(&ok);
	return ok;
}

bool MiscUtils::x1y1x2y2(const QDomElement & element, qreal & x1, qreal & y1, qreal & x2, qreal & y2)
{
	bool ok = true;
	x1 = element.attribute("x1", "").toDouble(&ok);
	if (!ok) {
        x1 = strToMil(element.attribute("x1", ""), ok);
        if (!ok) return false;
    }

	y1 = element.attribute("y1", "").toDouble(&ok);
	if (!ok) {
        y1 = strToMil(element.attribute("y1", ""), ok);
        if (!ok) return false;
    }

	x2 = element.attribute("x2", "").toDouble(&ok);
	if (!ok) {
        x2 = strToMil(element.attribute("x2", ""), ok);
        if (!ok) return false;
    }

	y2 = element.attribute("y2", "").toDouble(&ok);
	if (!ok) {
        y2 = strToMil(element.attribute("y2", ""), ok);
    }
    return ok;
}


qreal MiscUtils::strToMil(const QString & str, bool & ok) {
	ok = false;
	if (!str.endsWith("mil")) return 0;

	QString sub = str.left(str.length() - 3);
	return sub.toDouble(&ok);
}