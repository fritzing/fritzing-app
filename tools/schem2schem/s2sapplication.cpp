#include "s2sapplication.h"

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

#include "../../src/utils/textutils.h"

#include <limits>


/////////////////////////////////
//
//	TODO:  
//
//		put measurements into a file that is part of resources
//      no pin numbers for hidden pins
//      no pin text for hidden pin texts
//
//      deal with parts that don't have terminal points (modify original fzp and svg)
//	
//      copy old schematics to obsolete (with prefix)
//      readonly old style vs. convert vs. new style
//      fix internal schematics generators
//

/////////////////////////////////
 
bool xLessThan(ConnectorLocation * cl1, ConnectorLocation * cl2)
{
    return cl1->terminalPoint.x() < cl2->terminalPoint.x();
}

bool yLessThan(ConnectorLocation * cl1, ConnectorLocation * cl2)
{
    return cl1->terminalPoint.y() < cl2->terminalPoint.y();
}

/////////////////////////////////

static const qreal PinWidth = 0.246944;  // millimeters
static const qreal PinSmallTextHeight = 0.881944444;
static const qreal PinBigTextHeight = 1.23472222;
static const qreal PinTextIndent = PinWidth * 2;   //  PinWidth * 3;
static const qreal PinTextVert = PinWidth * 1;
static const qreal PinSmallTextVert = -PinWidth / 2;
static const qreal LabelTextHeight = 1.49930556;
static const qreal LabelTextSpace = 0.1;
static const qreal RectWidth = 0.3175;

static const QString PinColor("#787878");
static const QString PinTextColor("#8c8c8c");
static const QString TextColor("#505050");
static const QString TitleColor("#000000");
static const QString RectColor("#000000");
static const QString RectFillColor("#FFFFFF");

static const qreal NewUnit = 0.1 * 25.4;      // .1in in mm

static QRegExp IntegerFinder("\\d+");
static QString FontFamily("'Droid Sans'");

static const double ImageFactor = 5;

///////////////////////////////////////////////////////

QString makePinNumber(const ConnectorLocation * connectorLocation, double x1, double y1, double x2, double y2) {

    if (connectorLocation->id < 0) return "";

    QString text;
    double tx = 0;
    double ty = 0;
    if (x1 == x2) {
        text += QString("<g transform='translate(%1,%2)'><g transform='rotate(%3)'>\n")
			        .arg(x2 - PinWidth + PinSmallTextVert)  
			        .arg((y2 + y1) / 2)      
			        .arg(270)
                    ;
    }
    else {
        tx = (x2 + x1) / 2;       
        ty =  y2 - PinWidth + PinSmallTextVert;      
    }

    text += QString("<text class='text' font-family=\"%8\" stroke='none' stroke-width='%6' fill='%7' font-size='%1' x='%2' y='%3' text-anchor='%4'>%5</text>\n")
					.arg(PinSmallTextHeight)
					.arg(tx)
					.arg(ty)
					.arg("middle")
					.arg(connectorLocation->id)
					.arg(0)  // SW(width)
					.arg(PinTextColor) 
                    .arg(FontFamily)
                    ; 

    if (x1 == x2) {
		text += "</g></g>\n";
    }


    return text;
}

QString makePinText(const ConnectorLocation * connectorLocation, double x1, double y1, double x2, double y2, bool anchorAtStart) 
{
    QString text;

    bool rotate = false;
    double xOffset = 0, yOffset = 0;
    if (x1 == x2) {
        rotate = true;
        yOffset = (anchorAtStart ? -PinTextIndent : PinTextIndent);
        xOffset = PinTextVert;
    }
    else if (y1 == y2) {
        // horizontal pin
        xOffset = (anchorAtStart ? PinTextIndent : -PinTextIndent);
        yOffset = PinTextVert;
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
						.arg(PinBigTextHeight)
						.arg(x2 + xOffset)
						.arg(y2 + yOffset)
						.arg(anchorAtStart ? "start" : "end")
						.arg(TextUtils::escapeAnd(connectorLocation->name))
						.arg(0)  // SW(width)
						.arg(PinTextColor) 
                        .arg(FontFamily)
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
            .arg(PinColor)
            .arg(PinWidth)
            .arg(connectorLocation->svgID)
            ;
}

QString makeTerminal(const ConnectorLocation * connectorLocation, double x, double y) {
    return QString("<rect class='terminal' x='%1' y='%2' width='%3' height='%4' fill='none' stroke='none' stroke-width='0' id='%5' />\n")
            .arg(x - (PinWidth / 2))
            .arg(y - (PinWidth / 2))
            .arg(PinWidth)
            .arg(PinWidth)
            .arg(connectorLocation->terminalID)
            ;
}

///////////////////////////////////////////////////////

S2SApplication::S2SApplication(int argc, char *argv[]) : QCoreApplication(argc, argv)
{
    m_image = new QImage(50 * ImageFactor, 5 * ImageFactor, QImage::Format_Mono);
}

void S2SApplication::start() {
    if (!initArguments()) {
        usage();
        return;
    }

	int ix = QFontDatabase::addApplicationFont(":/resources/fonts/DroidSans.ttf");
    if (ix < 0) return;

	ix = QFontDatabase::addApplicationFont(":/resources/fonts/DroidSans-Bold.ttf");
    if (ix < 0) return;

	ix = QFontDatabase::addApplicationFont(":/resources/fonts/DroidSansMono.ttf");
    if (ix < 0) return;

	ix = QFontDatabase::addApplicationFont(":/resources/fonts/OCRA.ttf");
    if (ix < 0) return;

    m_fzpDir.setPath(m_fzpPath);
    m_oldSvgDir.setPath(m_oldSvgPath);
    m_newSvgDir.setPath(m_newSvgPath);

    QFile file(m_filePath);
    if (!file.open(QFile::ReadOnly)) {
        message(QString("unable to open %1").arg(m_filePath));
        return;
    }

    QTextStream in(&file);
    while ( !in.atEnd() )
    {
        QString line = in.readLine();
        onefzp(line.trimmed());
    }
}

void S2SApplication::usage() {
    message("\nusage: s2s "
                "-ff <path to fzps> "
                "-f <path to list of fzps (each item in list must relative path)> "
                "-os <path to old schematic svgs> "
                "-ns <path to new schematic svgs> "
                "\n"
    );
}

bool S2SApplication::initArguments() {
    QStringList args = QCoreApplication::arguments();
    for (int i = 0; i < args.length(); i++) {
        if ((args[i].compare("-h", Qt::CaseInsensitive) == 0) ||
            (args[i].compare("-help", Qt::CaseInsensitive) == 0) ||
            (args[i].compare("--help", Qt::CaseInsensitive) == 0))
        {
            return false;
        }

		if (i + 1 < args.length()) {
			if ((args[i].compare("-ff", Qt::CaseInsensitive) == 0) ||
				(args[i].compare("--ff", Qt::CaseInsensitive) == 0))
			{
				m_fzpPath = args[++i];
			}
			else if ((args[i].compare("-os", Qt::CaseInsensitive) == 0) ||
				(args[i].compare("--os", Qt::CaseInsensitive) == 0))
			{
				 m_oldSvgPath = args[++i];
			}
			else if ((args[i].compare("-ns", Qt::CaseInsensitive) == 0) ||
				(args[i].compare("--ns", Qt::CaseInsensitive) == 0))
			{
				 m_newSvgPath = args[++i];
			}
			else if ((args[i].compare("-f", Qt::CaseInsensitive) == 0) ||
				(args[i].compare("--f", Qt::CaseInsensitive) == 0))
			{
				m_filePath = args[++i];
			}
		}
    }

    if (m_fzpPath.isEmpty()) {
        message("-ff fzp path parameter missing");
        usage();
        return false;
    }

    if (m_oldSvgPath.isEmpty()) {
        message("-os old svg path parameter missing");
        usage();
        return false;
    }

    if (m_newSvgPath.isEmpty()) {
        message("-ns new svg path parameter missing");
        usage();
        return false;
    }

    if (m_filePath.isEmpty()) {
        message("-f fzp list file parameter missing");
        usage();
        return false;
    }

    QStringList paths;
    paths << m_fzpPath << m_oldSvgPath << m_newSvgPath;
    foreach (QString path, paths) {
        QDir directory(path);
        if (!directory.exists()) {
            message(QString("path '%1' not found").arg(path));
            return false;
        }
    }

    QFile file(m_filePath);
    if (!file.exists()) {
        message(QString("file path '%1' not found").arg(m_filePath));
        return false;
    }

    return true;
}

void S2SApplication::message(const QString & msg) {
   // QTextStream cout(stdout);
  //  cout << msg;
   // cout.flush();

	qDebug() << msg;
}

void S2SApplication::saveFile(const QString & content, const QString & path) 
{
	QFile file(path);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream out(&file);
		out.setCodec("UTF-8");
		out << content;
		file.close();
	}
}

void S2SApplication::onefzp(QString & fzpFilename) {
    if (!fzpFilename.endsWith(".fzp")) return;

    m_lefts.clear();
    m_rights.clear();
    m_tops.clear();
    m_bottoms.clear();

    QFile file(m_fzpDir.absoluteFilePath(fzpFilename));

	QString errorStr;
	int errorLine;
	int errorColumn;

	QDomDocument dom;
	if (!dom.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		message(QString("failed loading fzp %1, %2 line:%3 col:%4").arg(fzpFilename).arg(errorStr).arg(errorLine).arg(errorColumn));
		return;
	}

    QDomElement root = dom.documentElement();
    QDomElement titleElement = root.firstChildElement("title");
    QString title;
    TextUtils::findText(titleElement, title);
    QStringList titles;
    titles << title;
    TextUtils::resplit(titles, " ");
    TextUtils::resplit(titles, "_");
    TextUtils::resplit(titles, "-");
    TextUtils::resplit(titles, ".");

    QString schematicFileName;
    QDomNodeList nodeList = root.elementsByTagName("schematicView");
    for (int i = 0; i < nodeList.count(); i++) {
        QDomElement schematicView = nodeList.at(i).toElement();
        QDomElement layers = schematicView.firstChildElement("layers");
        schematicFileName = layers.attribute("image");
        if (!schematicFileName.isEmpty()) break;
    }

    if (schematicFileName.isEmpty()) {
		message(QString("schematic not found for fzp %1").arg(fzpFilename));
        return;
    }

    message(schematicFileName);

    QSvgRenderer renderer;
    bool loaded = renderer.load(m_oldSvgDir.absoluteFilePath(schematicFileName));
    if (!loaded) {
		message(QString("unabled to load schematic %1 for fzp %2").arg(schematicFileName).arg(fzpFilename));
        return;
    }

    QList<ConnectorLocation *> connectorLocations = initConnectors(root, renderer, fzpFilename, schematicFileName);

    QRectF viewBox = renderer.viewBoxF();
    if (viewBox.isEmpty()) {
        qDebug() << "\tempty viewbox";
    }
    double oldUnit = lrtb(connectorLocations, viewBox);

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
		double w = stringWidthMM(PinBigTextHeight, m_lefts.at(i)->name);
		if (w > hPinTextMax) hPinTextMax = w;
	}
	for (int i = 0; i < m_rights.count(); i++) {
		double w = stringWidthMM(PinBigTextHeight, m_rights.at(i)->name);
		if (w > hPinTextMax) hPinTextMax = w;
	}
    int leftTextUnits = qCeil(hPinTextMax / NewUnit);
    int rightTextUnits = leftTextUnits;

	double vTopPinTextMax = 0;
	for (int i = 0; i < m_tops.count(); i++) {
		double w = stringWidthMM(PinBigTextHeight, m_tops.at(i)->name);
		if (w > vTopPinTextMax) vTopPinTextMax = w;
	}
    int topTextUnits = qCeil(vTopPinTextMax / NewUnit);
	double vBottomPinTextMax = 0;
	for (int i = 0; i < m_bottoms.count(); i++) {
		double w = stringWidthMM(PinBigTextHeight, m_bottoms.at(i)->name);
		if (w > vBottomPinTextMax) vBottomPinTextMax = w;
	}
    int bottomTextUnits = qCeil(vBottomPinTextMax / NewUnit);

    double labelTextMax = 0;
    foreach (QString title, titles) {
		double w = stringWidthMM(LabelTextHeight, title);
		if (w > labelTextMax) labelTextMax = w;
    }

    double textWidth = hPinTextMax + hPinTextMax + labelTextMax + (6 * PinTextIndent);   // PTI text PTI PTI title PTI PTI text PTI           
    double hTextUnits = qCeil(textWidth / NewUnit);
    if (hTextUnits > hUnits) hUnits = hTextUnits;

    textWidth = titles.count() * (LabelTextHeight + LabelTextSpace);
    double vTextUnits = qCeil(textWidth / NewUnit);
    if (vTextUnits > vUnits) vUnits = vTextUnits;

    vUnits += topTextUnits;
    vUnits += bottomTextUnits;

    double fullWidth = hUnits;
    if (m_lefts.count()) fullWidth += 2;
    if (m_rights.count()) fullWidth += 2;
    double fullHeight = vUnits;
    if (m_tops.count()) fullHeight += 2;
    if (m_bottoms.count()) fullHeight += 2;

    QString svg = TextUtils::makeSVGHeader(25.4, 25.4, fullWidth * NewUnit, fullHeight * NewUnit);
    double rectL = RectWidth / 2;
    double rectT = rectL;
    if (m_lefts.count()) rectL += 2 * NewUnit;
    if (m_tops.count()) rectT += 2 * NewUnit;
    svg += "<g id='schematic'>\n";
    svg += QString("<rect class='interior rect' x='%1' y='%2' width='%3' height='%4' fill='%5' stroke='%6' stroke-width='%7' stroke-linecap='round' />\n")
                .arg(rectL)
                .arg(rectT)
                .arg((hUnits * NewUnit) - RectWidth)
                .arg((vUnits * NewUnit) - RectWidth)
                .arg(RectFillColor)
                .arg(RectColor)
                .arg(RectWidth)
                ;

    double vPinOffset = 0;
    if (m_tops.count()) vPinOffset = 2 * NewUnit;
    vPinOffset += topTextUnits * NewUnit;
    int space = vUnits - vPinUnits - topTextUnits - bottomTextUnits;
    if (space > 1) {
        vPinOffset += (space / 2) * NewUnit;
    }

    foreach (ConnectorLocation * connectorLocation, m_rights) {
        int units = qRound((connectorLocation->terminalPoint.y() - minPinV) / oldUnit) + 1;
        double y = units * NewUnit + vPinOffset;
        double x1 = ((fullWidth - 2) * NewUnit) - (PinWidth / 2);
        double x2 = (fullWidth * NewUnit) - (PinWidth / 2);
        svg += makePin(connectorLocation, x1, y, x2, y);
        svg += makePinNumber(connectorLocation, x1, y, x2, y);
        svg += makePinText(connectorLocation, x2, y, x1, y, false);
        svg += makeTerminal(connectorLocation, fullWidth * NewUnit, y);
    }

    foreach (ConnectorLocation * connectorLocation, m_lefts) {
        int units = qRound((connectorLocation->terminalPoint.y() - minPinV) / oldUnit) + 1;
        double y = units * NewUnit + vPinOffset;
        double x1 = PinWidth / 2;
        double x2 = (2 * NewUnit) + (PinWidth / 2);
        svg += makePin(connectorLocation, x1, y, x2, y);
        svg += makePinNumber(connectorLocation, x1, y, x2, y);
        svg += makePinText(connectorLocation, x1, y, x2, y, true);
        svg += makeTerminal(connectorLocation, 0, y);
    }

    double hPinOffset = 0;
    if (m_lefts.count()) hPinOffset = 2 * NewUnit;
    space = hUnits - hPinUnits;
    if (space > 1) {
        hPinOffset += (space / 2) * NewUnit;
    }

    foreach (ConnectorLocation * connectorLocation, m_bottoms) {
        int units = qRound((connectorLocation->terminalPoint.x() - minPinH) / oldUnit) + 1;
        double x = units * NewUnit + hPinOffset;
        double y1 = ((fullHeight - 2) * NewUnit) - (PinWidth / 2);
        double y2 = (fullHeight * NewUnit) - (PinWidth / 2);
        svg += makePin(connectorLocation, x, y1, x, y2);
        svg += makePinNumber(connectorLocation, x, y1, x, y2);
        svg += makePinText(connectorLocation, x, y2, x, y1, true);
        svg += makeTerminal(connectorLocation, x, fullHeight * NewUnit);
    }

    foreach (ConnectorLocation * connectorLocation, m_tops) {
        int units = qRound((connectorLocation->terminalPoint.x() - minPinH) / oldUnit) + 1;
        double x = units * NewUnit + hPinOffset;
        double y1 =  PinWidth / 2;
        double y2 = (2 * NewUnit) + (PinWidth / 2);
        svg += makePin(connectorLocation, x, y1, x, y2);
        svg += makePinNumber(connectorLocation, x, y1, x, y2);
        svg += makePinText(connectorLocation, x, y1, x, y2, false);
        svg += makeTerminal(connectorLocation, x, 0);
    }

    if (title.contains("accelerometer", Qt::CaseInsensitive)) {
        qDebug() << title;
    }


    double y = vUnits * NewUnit / 2;
    y -= titles.count() * (LabelTextHeight + LabelTextSpace) / 2;
    y += rectT;
    y = qMax(y, rectT + (topTextUnits * NewUnit) + LabelTextHeight + LabelTextSpace + PinTextIndent);  // y seems to be the location of the baseline so add a line
    foreach (QString subTitle, titles) {
        svg += QString("<text class='text' id='label' font-family=\"%7\" stroke='none' stroke-width='%4' fill='%5' font-size='%1' x='%2' y='%3' text-anchor='middle'>%6</text>\n")
				    .arg(LabelTextHeight)
				    .arg(((hUnits * NewUnit) / 2) + rectL)
				    .arg(y)
				    .arg(0)  // SW(width)
				    .arg(TitleColor) 
                    .arg(subTitle)
                    .arg(FontFamily)
                    ; 
            y += (LabelTextHeight + LabelTextSpace);
        }


    svg +="</g>\n";
    svg +="</svg>\n";

    QString newName = m_newSvgDir.absoluteFilePath(schematicFileName);
    TextUtils::writeUtf8(newName, svg);
    qDebug() << "";

}


double S2SApplication::stringWidthMM(double fontSize, const QString & string) {
    // tried using FontMetrics but the result sucked

    QString svg = TextUtils::makeSVGHeader(25.4, 25.4, 50, 5);
    svg += QString("<text font=\"%1\" font-size='%2' stroke='none' stroke-width='0' fill='black' x='0' y='%4' text-anchor='start' >%3</text>")
        .arg(FontFamily).arg(fontSize).arg(string).arg(2.5);
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

    return bestX / ImageFactor;
}

QList<ConnectorLocation *> S2SApplication::initConnectors(const QDomElement & root, const QSvgRenderer & renderer, const QString & fzpFilename, const QString & svgFilename) 
{
    QList<ConnectorLocation *> connectorLocations;
    m_minLeft = std::numeric_limits<int>::max();
    m_minTop = std::numeric_limits<int>::max();
    m_maxRight = std::numeric_limits<int>::min();
    m_maxBottom = std::numeric_limits<int>::min();

    QDomElement connectors = root.firstChildElement("connectors");
    QDomElement connector = connectors.firstChildElement("connector");
    bool zeroBased = false;
    while (!connector.isNull()) {
        QDomElement schematicView = connector.firstChildElement("views").firstChildElement("schematicView");
        QString svgID = schematicView.firstChildElement("p").attribute("svgId");
        if (svgID.isEmpty()) {
            message(QString("missing connector %1 in %2 schematic of %3").arg(connector.attribute("id")).arg(svgFilename).arg(fzpFilename));
        }
        else {
            ConnectorLocation * connectorLocation = new ConnectorLocation;
            connectorLocation->id = -1;
            QString id = connector.attribute("id");
            int ix = IntegerFinder.indexIn(id);
            if (ix > 0) {
                connectorLocation->id = IntegerFinder.cap(0).toInt();
                if (connectorLocation->id == 0) zeroBased = true;
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

    if (zeroBased) {
        foreach (ConnectorLocation * connectorLocation, connectorLocations) {
            connectorLocation->id++;
        }
    }

    return connectorLocations;
}

double S2SApplication::lrtb(QList<ConnectorLocation *> & connectorLocations, const QRectF & viewBox) 
{
    double fudge = qMax(m_maxRight - m_minLeft, m_maxBottom - m_minTop) / 200;

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
        int d = qRound(distance / fudge);
        if (d > most) most = d;
    }

    QVector<int> dbins(most + 2, 0);
    foreach (double distance, distances) {
        int d = qRound(distance / fudge);
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

    double oldUnit = biggestIndex * fudge;

    message(QString("\tunit is roughly %1, bin:%2 pins:%3 fudge:%4").arg(oldUnit).arg(biggest).arg(totalPins).arg(fudge));
    message(QString("\tl:%1 t:%2 r:%3 b:%4").arg(m_lefts.count()).arg(m_tops.count()).arg(m_rights.count()).arg(m_bottoms.count()));

    return oldUnit;
}