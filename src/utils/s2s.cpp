#include "s2s.h"

#include "stdio.h"

#include <QStringList>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QtDebug>
#include <QRegExp>
#include <qmath.h>
#include <QDomNodeList>
#include <QDomDocument>
#include <QDomElement>
#include <QSvgRenderer>
#include <QFontDatabase>
#include <QFont>
#include <QFontInfo>
#include <QResource>
#include <QImage>
#include <QPainter>
#include <QBitArray>

#include "../../src/utils/textutils.h"
#include "../../src/utils/schematicrectconstants.h"

#include <limits>

/////////////////////////////////
 
bool xLessThan(ConnectorLocation * cl1, ConnectorLocation * cl2)
{
    return cl1->terminalPoint.x() < cl2->terminalPoint.x();
}

bool yLessThan(ConnectorLocation * cl1, ConnectorLocation * cl2)
{
    return cl1->terminalPoint.y() < cl2->terminalPoint.y();
}

double getY(const QPointF & p) {
    return p.y();
}

double getX(const QPointF & p) {
    return p.x();
}

void setHiddenAux(QList<ConnectorLocation *> & allConnectorLocations, QList<ConnectorLocation *> & sideConnectorLocations, double (*get)(const QPointF &), double fudge)
{
    int i = 0;
    while (i < sideConnectorLocations.count() - 1) {
        QList<ConnectorLocation *> same;
        ConnectorLocation * basis = sideConnectorLocations.at(i);
        same << basis;
        for (int j = i + 1; j < sideConnectorLocations.count(); j++) {
            i = j;
            ConnectorLocation * next = sideConnectorLocations.at(j);
            if (qAbs(get(basis->terminalPoint )- get(next->terminalPoint)) < fudge) {
                same << next;
            }
            else {
                break;
            }
        }
        if (same.count() > 1) {
            foreach (ConnectorLocation * connectorLocation, same) {
                connectorLocation->hidden = true;
            }
            for (int ix = allConnectorLocations.count() - 1; ix >= 0; ix--) {
                ConnectorLocation * that = allConnectorLocations.at(ix);
                if (same.contains(that)) {
                    // assume the topmost is the one at the end of allConnectorLocations
                    // the rest are hidden
                    that->hidden = false;
                    sideConnectorLocations.removeOne(that);
                    int maxIndex = -1;
                    foreach (ConnectorLocation * s, same) {
                        int ix = sideConnectorLocations.indexOf(s);
                        if (ix > maxIndex) maxIndex = ix;
                    }
                    // visible one should be the topmost of the set
                    sideConnectorLocations.insert(maxIndex + 1, that);
                    break;
                }
            }
        }
    }

}

/////////////////////////////////

static QRegExp IntegerFinder("\\d+");
static const double ImageFactor = 5;
static const double FudgeDivisor = 300;
static const QRegExp VersionRegexp("[ -_][vV][\\d]+");

///////////////////////////////////////////////////////

QString makePinNumber(const ConnectorLocation * connectorLocation, double x1, double y1, double x2, double y2) {

    if (connectorLocation->id < 0) return "";
    if (connectorLocation->hidden) return "";
    if (!connectorLocation->displayPinNumber) return "";

    QString text;
    double tx = 0;
    double ty = 0;
    if (x1 == x2) {
        text += QString("<g transform='translate(%1,%2)'><g transform='rotate(%3)'>\n")
			        .arg(x2 - SchematicRectConstants::PinWidth + SchematicRectConstants::PinSmallTextVert)  
			        .arg((y2 + y1) / 2)      
			        .arg(270)
                    ;
    }
    else {
        tx = (x2 + x1) / 2;       
        ty =  y2 - SchematicRectConstants::PinWidth + SchematicRectConstants::PinSmallTextVert;      
    }

    text += QString("<text class='text' font-family=\"%8\" stroke='none' stroke-width='%6' fill='%7' font-size='%1' x='%2' y='%3' text-anchor='%4'>%5</text>\n")
					.arg(SchematicRectConstants::PinSmallTextHeight)
					.arg(tx)
					.arg(ty)
					.arg("middle")
					.arg(connectorLocation->id)
					.arg(0)  // SW(width)
					.arg(SchematicRectConstants::PinTextColor) 
                    .arg(SchematicRectConstants::FontFamily)
                    ; 

    if (x1 == x2) {
		text += "</g></g>\n";
    }


    return text;
}

QString makePinText(const ConnectorLocation * connectorLocation, double x1, double y1, double x2, double y2, bool anchorAtStart) 
{
    if (connectorLocation->hidden) return "";

    QString text;

    bool rotate = false;
    double xOffset = 0, yOffset = 0;
    if (x1 == x2) {
        rotate = true;
        yOffset = (anchorAtStart ? -SchematicRectConstants::PinTextIndent : SchematicRectConstants::PinTextIndent);
        xOffset = SchematicRectConstants::PinTextVert;
    }
    else if (y1 == y2) {
        // horizontal pin
        xOffset = (anchorAtStart ? SchematicRectConstants::PinTextIndent : -SchematicRectConstants::PinTextIndent);
        yOffset = SchematicRectConstants::PinTextVert;
    }
    else {
        return "";
    }

    if (rotate) {
		text += QString("<g transform='translate(%1,%2)'><g transform='rotate(%3)'>\n")
			.arg(x2 + xOffset)
			.arg(y2 + yOffset)
			.arg(270);
		x2 = 0;
		y2 = 0;
        xOffset = yOffset = 0;
	}

	text += QString("<text class='text' font-family=\"%8\" stroke='none' stroke-width='%6' fill='%7' font-size='%1' x='%2' y='%3' text-anchor='%4'>%5</text>\n")
						.arg(SchematicRectConstants::PinBigTextHeight)
						.arg(x2 + xOffset)
						.arg(y2 + yOffset)
						.arg(anchorAtStart ? "start" : "end")
						.arg(TextUtils::escapeAnd(connectorLocation->name))
						.arg(0)  // SW(width)
						.arg(SchematicRectConstants::PinTextColor) 
                        .arg(SchematicRectConstants::FontFamily)
                        ;  

    if (rotate) {
		text += "</g></g>\n";
	}

    return text;
}

QString makePin(const ConnectorLocation * connectorLocation, double x1, double y1, double x2, double y2) {
    return QString("<line class='pin' x1='%1' y1='%2' x2='%3' y2='%4' fill='none' stroke='%5' stroke-width='%6' stroke-linecap='round' id='%7' />\n")
            .arg(x1)
            .arg(y1)
            .arg(x2)
            .arg(y2)
            .arg(connectorLocation->hidden ? "none" : SchematicRectConstants::PinColor)
            .arg(connectorLocation->hidden ? 0 : SchematicRectConstants::PinWidth)
            .arg(connectorLocation->svgID)
            ;
}

QString makeTerminal(const ConnectorLocation * connectorLocation, double x, double y) {
    return QString("<rect class='terminal' x='%1' y='%2' width='%3' height='%4' fill='none' stroke='none' stroke-width='0' id='%5' />\n")
            .arg(x - (SchematicRectConstants::PinWidth / 2))
            .arg(y - (SchematicRectConstants::PinWidth / 2))
            .arg(SchematicRectConstants::PinWidth)
            .arg(SchematicRectConstants::PinWidth)
            .arg(connectorLocation->terminalID)
            ;
}

///////////////////////////////////////////////////////


S2S::S2S(bool fzpzStyle) : QObject()
{
    m_image = new QImage(50 * ImageFactor, 5 * ImageFactor, QImage::Format_Mono);
    m_fzpzStyle = fzpzStyle;
}

void S2S::message(const QString & msg) {
   // QTextStream cout(stdout);
  //  cout << msg;
   // cout.flush();

	qDebug() << msg;
    emit messageSignal(msg);
}

void S2S::saveFile(const QString & content, const QString & path) 
{
	QFile file(path);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream out(&file);
		out.setCodec("UTF-8");
		out << content;
		file.close();
	}
}

bool S2S::onefzp(QString & fzpFilePath, QString & schematicFilePath) {
    if (!fzpFilePath.endsWith(".fzp")) {
        return false;
    }

    bool createSchematicFile = schematicFilePath.isEmpty();
    QString newSchematicFilePath = schematicFilePath;

    m_lefts.clear();
    m_rights.clear();
    m_tops.clear();
    m_bottoms.clear();

    QFile file(fzpFilePath);

	QString errorStr;
	int errorLine;
	int errorColumn;

	QDomDocument dom;
	if (!dom.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		message(tr("Failed loading '%1', %2 line:%3 col:%4").arg(fzpFilePath).arg(errorStr).arg(errorLine).arg(errorColumn));
		return false;
	}

    QDomElement root = dom.documentElement();
    QDomElement titleElement = root.firstChildElement("title");
    QString title;
    TextUtils::findText(titleElement, title);
    //int ix = VersionRegexp.lastIndexIn(title);
    //if (ix > 0 && ix + VersionRegexp.cap(0).count() == title.count()) {
    //    title.chop(VersionRegexp.cap(0).count());
    //}
    QStringList titles;
    titles << title;
    TextUtils::resplit(titles, " ");
    TextUtils::resplit(titles, "_");
    TextUtils::resplit(titles, "-");


    if (createSchematicFile) {
        QString schematicFileName;
        QDomNodeList nodeList = root.elementsByTagName("schematicView");
        for (int i = 0; i < nodeList.count(); i++) {
            QDomElement schematicView = nodeList.at(i).toElement();
            QDomElement layers = schematicView.firstChildElement("layers");
            schematicFileName = layers.attribute("image");
            if (!schematicFileName.isEmpty()) break;
        }

        if (schematicFileName.isEmpty()) {
		    message(tr("Schematic not found for '%1'").arg(fzpFilePath));
            return false;
        }

        if (m_fzpzStyle) {
             schematicFileName.replace("/", ".");
             schematicFileName = "svg." + schematicFileName;
        }

        schematicFilePath = m_oldSvgDir.absoluteFilePath(schematicFileName);
        newSchematicFilePath = m_newSvgDir.absoluteFilePath(schematicFileName);
    }

    if (!ensureTerminalPoints(fzpFilePath, schematicFilePath, root)) {
        return false;
    }

    qDebug() << schematicFilePath;

    QSvgRenderer renderer;
    bool loaded = renderer.load(schematicFilePath);
    if (!loaded) {
		message(tr("Unable to load schematic '%1' for '%2'").arg(schematicFilePath).arg(fzpFilePath));
        return false;
    }

    QList<ConnectorLocation *> connectorLocations = initConnectors(root, renderer, fzpFilePath, schematicFilePath);

    QRectF viewBox = renderer.viewBoxF();
    if (viewBox.isEmpty()) {
        qDebug() << "\tempty viewbox";
    }
    double oldUnit = lrtb(connectorLocations, viewBox);

    if (qAbs(oldUnit - SchematicRectConstants::NewUnit) < (SchematicRectConstants::NewUnit / 25)) {
		message(tr("Schematic '%1' is already using the 0.1inch standard.").arg(schematicFilePath));
        return false;
    }

    setHidden(connectorLocations);

    double minPinV = 0;
    double maxPinV = 0;
    if (m_lefts.count() && m_rights.count()) {
        minPinV = qMin(m_lefts.first()->terminalPoint.y(), m_rights.first()->terminalPoint.y());
        maxPinV = qMax(m_lefts.last()->terminalPoint.y(), m_rights.last()->terminalPoint.y());
    }
    else if (m_rights.count()) {
        minPinV = m_rights.first()->terminalPoint.y();
        maxPinV = m_rights.last()->terminalPoint.y();
    }
    else if (m_lefts.count()) {
        minPinV = m_lefts.first()->terminalPoint.y();
        maxPinV = m_lefts.last()->terminalPoint.y();
    }
    int vPinUnits = qRound((maxPinV - minPinV) / oldUnit) + 2;
    int vUnits = vPinUnits;

    double minPinH = 0;
    double maxPinH = 0;
    if (m_bottoms.count() && m_tops.count()) {
        minPinH = qMin(m_bottoms.first()->terminalPoint.x(), m_tops.first()->terminalPoint.x());
        maxPinH = qMax(m_bottoms.last()->terminalPoint.x(), m_tops.last()->terminalPoint.x());
    }
    else if (m_bottoms.count()) {
        minPinH = m_bottoms.first()->terminalPoint.x();
        maxPinH = m_bottoms.last()->terminalPoint.x();
    }
    else if (m_tops.count()) {
        minPinH = m_tops.first()->terminalPoint.x();
        maxPinH = m_tops.last()->terminalPoint.x();
    }
    int hPinUnits = qRound((maxPinH - minPinH) / oldUnit) + 2;
    int hUnits = hPinUnits;

	double hPinTextMax = 0;
	for (int i = 0; i < m_lefts.count(); i++) {
		double w = stringWidthMM(SchematicRectConstants::PinBigTextHeight, m_lefts.at(i)->name);
		if (w > hPinTextMax) hPinTextMax = w;
	}
	for (int i = 0; i < m_rights.count(); i++) {
		double w = stringWidthMM(SchematicRectConstants::PinBigTextHeight, m_rights.at(i)->name);
		if (w > hPinTextMax) hPinTextMax = w;
	}
    int leftTextUnits = qCeil(hPinTextMax / SchematicRectConstants::NewUnit);
    int rightTextUnits = leftTextUnits;


	double vTopPinTextMax = 0;
	for (int i = 0; i < m_tops.count(); i++) {
		double w = stringWidthMM(SchematicRectConstants::PinBigTextHeight, m_tops.at(i)->name);
		if (w > vTopPinTextMax) vTopPinTextMax = w;
	}
    int topTextUnits = qCeil(vTopPinTextMax / SchematicRectConstants::NewUnit);
	double vBottomPinTextMax = 0;
	for (int i = 0; i < m_bottoms.count(); i++) {
		double w = stringWidthMM(SchematicRectConstants::PinBigTextHeight, m_bottoms.at(i)->name);
		if (w > vBottomPinTextMax) vBottomPinTextMax = w;
	}
    int bottomTextUnits = qCeil(vBottomPinTextMax / SchematicRectConstants::NewUnit);

    double labelTextMax = spaceTitle(titles, hUnits - leftTextUnits - rightTextUnits - 2);

    double textWidth = hPinTextMax + hPinTextMax + labelTextMax + (6 * SchematicRectConstants::PinTextIndent);   // PTI text PTI PTI title PTI PTI text PTI           
    double hTextUnits = qCeil(textWidth / SchematicRectConstants::NewUnit);
    if (hTextUnits > hUnits) hUnits = hTextUnits;

    textWidth = titles.count() * (SchematicRectConstants::LabelTextHeight + SchematicRectConstants::LabelTextSpace);
    double vTextUnits = qCeil((textWidth / SchematicRectConstants::NewUnit) + (2 * SchematicRectConstants::PinTextIndent));
    if (vTextUnits > vUnits) vUnits = vTextUnits;

    vUnits += topTextUnits;
    vUnits += bottomTextUnits;

    double fullWidth = hUnits;
    if (m_lefts.count()) fullWidth += 2;
    if (m_rights.count()) fullWidth += 2;
    double fullHeight = vUnits;
    if (m_tops.count()) fullHeight += 2;
    if (m_bottoms.count()) fullHeight += 2;



    // construct svg

    QString svg = TextUtils::makeSVGHeader(25.4, 25.4, fullWidth * SchematicRectConstants::NewUnit, fullHeight * SchematicRectConstants::NewUnit);
    double rectL = SchematicRectConstants::RectStrokeWidth / 2;
    double rectT = rectL;
    if (m_lefts.count()) rectL += 2 * SchematicRectConstants::NewUnit;
    if (m_tops.count()) rectT += 2 * SchematicRectConstants::NewUnit;
    svg += "<g id='schematic'>\n";
    svg += QString("<rect class='interior rect' x='%1' y='%2' width='%3' height='%4' fill='%5' stroke='%6' stroke-width='%7' stroke-linecap='round' />\n")
                .arg(rectL)
                .arg(rectT)
                .arg((hUnits * SchematicRectConstants::NewUnit) - SchematicRectConstants::RectStrokeWidth)
                .arg((vUnits * SchematicRectConstants::NewUnit) - SchematicRectConstants::RectStrokeWidth)
                .arg(SchematicRectConstants::RectFillColor)
                .arg(SchematicRectConstants::RectStrokeColor)
                .arg(SchematicRectConstants::RectStrokeWidth)
                ;

    double vPinOffset = 0;
    if (m_tops.count()) vPinOffset = 2 * SchematicRectConstants::NewUnit;
    vPinOffset += topTextUnits * SchematicRectConstants::NewUnit;
    int space = vUnits - vPinUnits - topTextUnits - bottomTextUnits;
    if (space > 1) {
        vPinOffset += (space / 2) * SchematicRectConstants::NewUnit;
    }

    foreach (ConnectorLocation * connectorLocation, m_rights) {
        int units = qRound((connectorLocation->terminalPoint.y() - minPinV) / oldUnit) + 1;
        double y = units * SchematicRectConstants::NewUnit + vPinOffset;
        double x1 = ((fullWidth - 2) * SchematicRectConstants::NewUnit) - (SchematicRectConstants::PinWidth / 2);
        double x2 = (fullWidth * SchematicRectConstants::NewUnit) - (SchematicRectConstants::PinWidth / 2);
        svg += makePin(connectorLocation, x1, y, x2, y);
        svg += makePinNumber(connectorLocation, x1, y, x2, y);
        svg += makePinText(connectorLocation, x2, y, x1, y, false);
        svg += makeTerminal(connectorLocation, fullWidth * SchematicRectConstants::NewUnit, y);
    }

    foreach (ConnectorLocation * connectorLocation, m_lefts) {
        int units = qRound((connectorLocation->terminalPoint.y() - minPinV) / oldUnit) + 1;
        double y = units * SchematicRectConstants::NewUnit + vPinOffset;
        double x1 = SchematicRectConstants::PinWidth / 2;
        double x2 = (2 * SchematicRectConstants::NewUnit) + (SchematicRectConstants::PinWidth / 2);
        svg += makePin(connectorLocation, x1, y, x2, y);
        svg += makePinNumber(connectorLocation, x1, y, x2, y);
        svg += makePinText(connectorLocation, x1, y, x2, y, true);
        svg += makeTerminal(connectorLocation, 0, y);
    }

    double hPinOffset = 0;
    if (m_lefts.count()) hPinOffset = 2 * SchematicRectConstants::NewUnit;
    space = hUnits - hPinUnits;
    if (space > 1) {
        hPinOffset += (space / 2) * SchematicRectConstants::NewUnit;
    }

    foreach (ConnectorLocation * connectorLocation, m_bottoms) {
        int units = qRound((connectorLocation->terminalPoint.x() - minPinH) / oldUnit) + 1;
        double x = units * SchematicRectConstants::NewUnit + hPinOffset;
        double y1 = ((fullHeight - 2) * SchematicRectConstants::NewUnit) - (SchematicRectConstants::PinWidth / 2);
        double y2 = (fullHeight * SchematicRectConstants::NewUnit) - (SchematicRectConstants::PinWidth / 2);
        svg += makePin(connectorLocation, x, y1, x, y2);
        svg += makePinNumber(connectorLocation, x, y1, x, y2);
        svg += makePinText(connectorLocation, x, y2, x, y1, true);
        svg += makeTerminal(connectorLocation, x, fullHeight * SchematicRectConstants::NewUnit);
    }

    foreach (ConnectorLocation * connectorLocation, m_tops) {
        int units = qRound((connectorLocation->terminalPoint.x() - minPinH) / oldUnit) + 1;
        double x = units * SchematicRectConstants::NewUnit + hPinOffset;
        double y1 =  SchematicRectConstants::PinWidth / 2;
        double y2 = (2 * SchematicRectConstants::NewUnit) + (SchematicRectConstants::PinWidth / 2);
        svg += makePin(connectorLocation, x, y1, x, y2);
        svg += makePinNumber(connectorLocation, x, y1, x, y2);
        svg += makePinText(connectorLocation, x, y1, x, y2, false);
        svg += makeTerminal(connectorLocation, x, 0);
    }

    double y = vUnits * SchematicRectConstants::NewUnit / 2;
    y -= titles.count() * (SchematicRectConstants::LabelTextHeight + SchematicRectConstants::LabelTextSpace) / 2;
    y += rectT;
    y = qMax(y, rectT + (topTextUnits * SchematicRectConstants::NewUnit) + SchematicRectConstants::LabelTextHeight + SchematicRectConstants::LabelTextSpace + SchematicRectConstants::PinTextIndent);  // y seems to be the location of the baseline so add a line
    foreach (QString subTitle, titles) {
        svg += QString("<text class='text' id='label' font-family=\"%7\" stroke='none' stroke-width='%4' fill='%5' font-size='%1' x='%2' y='%3' text-anchor='middle'>%6</text>\n")
				    .arg(SchematicRectConstants::LabelTextHeight)
				    .arg(((hUnits * SchematicRectConstants::NewUnit) / 2) + rectL)
				    .arg(y)
				    .arg(0)  // SW(width)
				    .arg(SchematicRectConstants::TitleColor) 
                    .arg(TextUtils::escapeAnd(subTitle))
                    .arg(SchematicRectConstants::FontFamily)
                    ; 
            y += (SchematicRectConstants::LabelTextHeight + SchematicRectConstants::LabelTextSpace);
        }


    svg +="</g>\n";
    svg +="</svg>\n";

    TextUtils::writeUtf8(newSchematicFilePath, svg);
    qDebug() << newSchematicFilePath;
    qDebug() << "";
    return true;
}


double S2S::stringWidthMM(double fontSize, const QString & string) {

    /*
    // tried using FontMetrics but the result is always too short
    QFont font("Droid Sans");
	font.setPointSizeF(fontSize * 72 / 25.4);
	QFontMetricsF fontMetrics(font);
    double pixels = fontMetrics.width(string);
    double mm = 25.4 * pixels / 90;
    */

    QString svg = TextUtils::makeSVGHeader(25.4, 25.4, 50, 5);
    svg += QString("<text font=\"%1\" font-size='%2' stroke='none' stroke-width='0' fill='black' x='0' y='%4' text-anchor='start' >%3</text>")
        .arg(SchematicRectConstants::FontFamily).arg(fontSize).arg(TextUtils::escapeAnd(string)).arg(2.5);
    svg += "</svg>";

    QSvgRenderer renderer(svg.toUtf8());
    m_image->fill(0xffffffff);
	QPainter painter;
	painter.begin(m_image);
	painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
	renderer.render(&painter);
	painter.end();
    //QDir dir(QCoreApplication::applicationDirPath());
    //dir.cdUp();
    //image.save(dir.absoluteFilePath("bloody_font.png"));
    int bestX = 0;
    for (int y = 0; y < m_image->height(); y++) {
        for (int x = bestX; x < m_image->width(); x++) {
            if (m_image->pixel(x, y) == 0xff000000) {
                bestX = x;
            }
        }
    }

    //if (mm < bestX / ImageFactor) {
    //    qDebug() << "string width" << mm << (bestX / ImageFactor) << string;
    //}

    return bestX / ImageFactor;
}

QList<ConnectorLocation *> S2S::initConnectors(const QDomElement & root, const QSvgRenderer & renderer, const QString & fzpFilename, const QString & svgFilename) 
{
    QList<ConnectorLocation *> connectorLocations;
    m_minLeft = std::numeric_limits<int>::max();
    m_minTop = std::numeric_limits<int>::max();
    m_maxRight = std::numeric_limits<int>::min();
    m_maxBottom = std::numeric_limits<int>::min();

    QDomElement connectors = root.firstChildElement("connectors");
    QDomElement connector = connectors.firstChildElement("connector");
    QBitArray idlist;
    int noIDCount = 0;
    while (!connector.isNull()) {
        QDomElement schematicView = connector.firstChildElement("views").firstChildElement("schematicView");
        QString svgID = schematicView.firstChildElement("p").attribute("svgId");
        if (svgID.isEmpty()) {
            message(tr("Missing connector %1 in '%2' schematic of '%3'").arg(connector.attribute("id")).arg(svgFilename).arg(fzpFilename));
        }
        else {
            ConnectorLocation * connectorLocation = new ConnectorLocation;
            connectorLocation->id = -1;
            connectorLocation->hidden = false;
            connectorLocation->displayPinNumber = false;
            QString id = connector.attribute("id");
            int ix = IntegerFinder.indexIn(id);
            if (ix > 0) {
                connectorLocation->id = IntegerFinder.cap(0).toInt();
                if (idlist.size() < connectorLocation->id + 1) {
                    int oldsize = idlist.size();
                    idlist.resize(connectorLocation->id + 1);
                    for (int i = oldsize; i < idlist.size(); i++) idlist.setBit(i, false);
                }
                idlist.setBit(connectorLocation->id, true);
            }
            else {
                noIDCount++;
            }
            connectorLocation->name = connector.attribute("name");
            connectorLocation->terminalID = schematicView.firstChildElement("p").attribute("terminalId");
            connectorLocation->svgID = svgID;
            QRectF bounds = renderer.boundsOnElement(svgID);
		    QMatrix m = renderer.matrixForElement(svgID);
		    connectorLocation->bounds = m.mapRect(bounds);
            connectorLocation->terminalPoint = connectorLocation->bounds.center();

            if (!connectorLocation->terminalID.isEmpty()) {
                bounds = renderer.boundsOnElement(connectorLocation->terminalID);
                m = renderer.matrixForElement(connectorLocation->terminalID);
                connectorLocation->terminalPoint = m.mapRect(bounds).center();
            }
            connectorLocations.append(connectorLocation);
            if (connectorLocation->terminalPoint.x() < m_minLeft) {
                m_minLeft = connectorLocation->terminalPoint.x();
            }
            if (connectorLocation->terminalPoint.x() > m_maxRight) {
                m_maxRight = connectorLocation->terminalPoint.x();
            }
            if (connectorLocation->terminalPoint.y() < m_minTop) {
                m_minTop = connectorLocation->terminalPoint.y();
            }
            if (connectorLocation->terminalPoint.y() > m_maxBottom) {
                m_maxBottom = connectorLocation->terminalPoint.y();
            }
        }

        connector = connector.nextSiblingElement("connector");
    }

    if (noIDCount > 0) {
        qDebug() << "no id count" << noIDCount;
    }

    bool display = true;
    if (!idlist.at(0) && !idlist.at(1)) {
        display = false;
    }
    else {
        int present = 0;
        for (int i = 0; i < idlist.size(); i++) {
            if (idlist.at(i)) present++;
        }
        display = (present >= .66 * idlist.size());
    }

    foreach (ConnectorLocation * connectorLocation, connectorLocations) {
        connectorLocation->displayPinNumber = display;
    }

    if (display && idlist.at(0)) {
        foreach (ConnectorLocation * connectorLocation, connectorLocations) {
            connectorLocation->id++;
        }
    }

    return connectorLocations;
}

double S2S::lrtb(QList<ConnectorLocation *> & connectorLocations, const QRectF & viewBox) 
{
    m_fudge = qMax(m_maxRight - m_minLeft, m_maxBottom - m_minTop) / FudgeDivisor;

    foreach (ConnectorLocation * connectorLocation, connectorLocations) {
        double d[4];
        d[0] = connectorLocation->terminalPoint.x() - viewBox.left();
        d[1] = connectorLocation->terminalPoint.y() - viewBox.top();
        d[2] = viewBox.right() - connectorLocation->terminalPoint.x();
        d[3] = viewBox.bottom() - connectorLocation->terminalPoint.y();
        int ix = 0;
        int dx = d[0];
        for (int i = 1; i < 4; i++) {
            if (d[i] < dx) {
                dx = d[i];
                ix = i;
            }
        }
        switch (ix) {
            case 0:
                m_lefts << connectorLocation;
                break;
            case 1:
                m_tops << connectorLocation;
                break;
            case 2:
                m_rights << connectorLocation;
                break;
            case 3:
                m_bottoms << connectorLocation;
                break;
            default:
                qDebug() << "shouldn't happen" << ix;
        }

    }

    qSort(m_lefts.begin(), m_lefts.end(), yLessThan);
    qSort(m_rights.begin(), m_rights.end(), yLessThan);
    qSort(m_tops.begin(), m_tops.end(), xLessThan);
    qSort(m_bottoms.begin(), m_bottoms.end(), xLessThan);

    QList<double> distances;
    for (int i = 1; i < m_lefts.count(); i++) {
        ConnectorLocation * l1 = m_lefts.at(i - 1);
        ConnectorLocation * l2 = m_lefts.at(i);
        distances << l2->terminalPoint.y() - l1->terminalPoint.y();
    }
    for (int i = 1; i < m_rights.count(); i++) {
        ConnectorLocation * l1 = m_rights.at(i - 1);
        ConnectorLocation * l2 = m_rights.at(i);
        distances << l2->terminalPoint.y() - l1->terminalPoint.y();
    }
    for (int i = 1; i < m_tops.count(); i++) {
        ConnectorLocation * l1 = m_tops.at(i - 1);
        ConnectorLocation * l2 = m_tops.at(i);
        distances << l2->terminalPoint.x() - l1->terminalPoint.x();
    }
    for (int i = 1; i < m_bottoms.count(); i++) {
        ConnectorLocation * l1 = m_bottoms.at(i - 1);
        ConnectorLocation * l2 = m_bottoms.at(i);
        distances << l2->terminalPoint.x() - l1->terminalPoint.x();
    }

    qSort(distances.begin(), distances.end());

    int totalPins = m_lefts.count() + m_rights.count() + m_bottoms.count() + m_tops.count();

    int most = 0;
    foreach (double distance, distances) {
        int d = qRound(distance / m_fudge);
        if (d > most) most = d;
    }

    QVector<int> dbins(most + 2, 0);
    foreach (double distance, distances) {
        int d = qRound(distance / m_fudge);
        dbins[d] += 1;
    }

    int biggest = dbins[1];
    int biggestIndex = 1;
    for (int i = 2; i < dbins.count(); i++) {
        if (dbins[i] > biggest) {
            biggest = dbins[i];
            biggestIndex = i;
        }
    }

    if (biggest == 0) {
        qDebug() << "\tbiggest 0" << totalPins;
    }

    double oldUnit = biggestIndex * m_fudge;

    qDebug() << QString("\tunit is roughly %1mm, bin:%2 pins:%3 fudge:%4").arg(oldUnit).arg(biggest).arg(totalPins).arg(m_fudge);
    qDebug() << QString("\tl:%1 t:%2 r:%3 b:%4").arg(m_lefts.count()).arg(m_tops.count()).arg(m_rights.count()).arg(m_bottoms.count());

    return oldUnit;
}


void S2S::setHidden(QList<ConnectorLocation *> & connectorLocations) 
{
    setHiddenAux(connectorLocations, m_lefts, getY, m_fudge);
    setHiddenAux(connectorLocations, m_rights, getY, m_fudge);
    setHiddenAux(connectorLocations, m_tops, getX, m_fudge);
    setHiddenAux(connectorLocations, m_bottoms, getX, m_fudge);
}

bool S2S::ensureTerminalPoints(const QString & fzpFilePath, const QString & svgFilePath, QDomElement & fzpRoot) {
    QList<QDomElement> missing;
    QDomElement connectors = fzpRoot.firstChildElement("connectors");
    QDomElement connector = connectors.firstChildElement("connector");
    while (!connector.isNull()) {
        QDomElement views = connector.firstChildElement("views");
        QDomElement schematicView = views.firstChildElement("schematicView");
        QDomElement p = schematicView.firstChildElement("p");
        QString terminalID = p.attribute("terminalId");
        if (terminalID.isEmpty()) {
            missing << p;
        }

        connector = connector.nextSiblingElement("connector");
    }

    if (missing.count() == 0) return true;

    QSvgRenderer renderer;
    bool loaded = renderer.load(svgFilePath);
    if (!loaded) {
		message(tr("Uunable to load schematic '%1' for '%2'").arg(svgFilePath).arg(fzpFilePath));
        return false;
    }

	QString errorStr;
	int errorLine;
	int errorColumn;

    QFile file(svgFilePath);
	QDomDocument dom;
	if (!dom.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		message(tr("Failed loading schematic '%1', %2 line:%3 col:%4").arg(svgFilePath).arg(errorStr).arg(errorLine).arg(errorColumn));
		return false;
	}

    QDomElement svgRoot = dom.documentElement();

    bool fzpChanged = false;
    bool svgChanged = false;

    foreach (QDomElement p, missing) {
        QDomElement connector = p.parentNode().parentNode().parentNode().toElement();
        QString name = connector.attribute("id");
        if (name.isEmpty()) {
            qDebug() << "empty name in connector";
        }
        else {
            // assumes the file is well behaved, and the terminalID isn't already in use
            QString terminalName = name + "terminal";
            p.setAttribute("terminalId", terminalName);
            fzpChanged = true;

            QDomElement terminalElement = TextUtils::findElementWithAttribute(svgRoot, "id", terminalName);
            if (terminalElement.isNull()) {
                QString svgID = p.attribute("svgId");
                QDomElement connectorElement = TextUtils::findElementWithAttribute(svgRoot, "id", svgID);
                QRectF bounds = renderer.boundsOnElement(svgID);
                QDomElement rect = svgRoot.ownerDocument().createElement("rect");
                connectorElement.parentNode().insertAfter(rect, connectorElement);
                rect.setAttribute("x", QString::number(bounds.left()));
                rect.setAttribute("y", QString::number(bounds.top()));
                rect.setAttribute("width", QString::number(bounds.width()));
                rect.setAttribute("height", QString::number(bounds.height()));
                rect.setAttribute("fill", "none");
                rect.setAttribute("stroke", "none");
                rect.setAttribute("stroke-width", 0);
                rect.setAttribute("id", terminalName);
                svgChanged = true;
            }
        }
    }

    if (svgChanged) {
        TextUtils::writeUtf8(svgFilePath, TextUtils::removeXMLEntities(dom.toString(4)));
    }
    if (fzpChanged) {
        TextUtils::writeUtf8(fzpFilePath, TextUtils::removeXMLEntities(fzpRoot.ownerDocument().toString(4)));
    }

    return true;
}

double S2S::spaceTitle(QStringList & titles, int openUnits)
{
    double labelTextMax = 0;
    foreach (QString title, titles) {
		double w = stringWidthMM(SchematicRectConstants::LabelTextHeight, title);
		if (w > labelTextMax) labelTextMax = w;
    }

    double useMax = qMax(openUnits * SchematicRectConstants::NewUnit, labelTextMax);

    bool changed = false;
    int ix = 0;
    while (ix < titles.count() - 1) {
        QString combined = titles.at(ix);
        if (!(combined.endsWith("_") || combined.endsWith("-"))) {
            combined.append(" ");
        }
        combined.append(titles.at(ix + 1));
        double w = stringWidthMM(SchematicRectConstants::LabelTextHeight, combined);
        if (w <= useMax) {
            titles[ix] = combined;
            titles.removeAt(ix + 1);
            changed = true;
            continue;
        }

        ix++;
    }

    if (!changed) return labelTextMax;
     
    labelTextMax = 0;
    foreach (QString title, titles) {
		double w = stringWidthMM(SchematicRectConstants::LabelTextHeight, title);
		if (w > labelTextMax) labelTextMax = w;
    }
    return labelTextMax;
}

void S2S::setSvgDirs(QDir & oldDir, QDir & newDir) {
    m_oldSvgDir = oldDir;
    m_newSvgDir = newDir;
}

