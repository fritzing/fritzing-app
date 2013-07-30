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
#include <QFontMetrics>
#include <QFontMetrics>
#include <QFont>
#include <QResource>

#include "../../src/utils/textutils.h"

#include <limits>


/////////////////////////////////
//
//	TODO:  
//
//		put measurements into a file
//      pin numbers
//      pin labels
//      title
//      centering/first pin offset
//
//				
//
/////////////////////////////////
 
bool xLessThan(ConnectorLocation & cl1, ConnectorLocation & cl2)
{
    return cl1.terminalPoint.x() < cl2.terminalPoint.x();
}

bool yLessThan(ConnectorLocation & cl1, ConnectorLocation & cl2)
{
    return cl1.terminalPoint.y() < cl2.terminalPoint.y();
}

/////////////////////////////////

static const qreal PinWidth = 0.246944;  // millimeters
static const qreal PinSmallTextHeight = 0.881944444;
static const qreal PinBigTextHeight = 1.23472222;
static const qreal PinTextIndent = PinWidth * 3;
static const qreal PinTextVert = PinWidth * 1;
static const qreal PinSmallTextVert = -PinWidth / 2;
static const qreal LabelTextHeight = 1.49930556;
static const qreal LabelTextSpace = 0.1;
static const qreal RectWidth = 0.3175;

static const QString PinColor("#787878");
static const QString PintextColor("#8c8c8c");
static const QString TextColor("#505050");
static const QString TitleColor("#000000");
static const QString RectColor("#000000");
static const QString RectFillColor("#FFFFFF");


static const qreal NewUnit = 0.1 * 25.4;      // .1in in mm

///////////////////////////////////////////////////////

S2SApplication::S2SApplication(int argc, char *argv[]) : QCoreApplication(argc, argv)
{
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

void S2SApplication::onefzp(QString & filename) {
    if (!filename.endsWith(".fzp")) return;

    QFile file(m_fzpDir.absoluteFilePath(filename));

	QString errorStr;
	int errorLine;
	int errorColumn;

	QDomDocument dom;
	if (!dom.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		message(QString("failed loading fzp %1, %2 line:%3 col:%4").arg(filename).arg(errorStr).arg(errorLine).arg(errorColumn));
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
		message(QString("schematic not found for fzp %1").arg(filename));
        return;
    }

    QSvgRenderer renderer;
    bool loaded = renderer.load(m_oldSvgDir.absoluteFilePath(schematicFileName));
    if (!loaded) {
		message(QString("unabled to load schematic %1 for fzp %2").arg(schematicFileName).arg(filename));
        return;
    }

    QList<ConnectorLocation> connectorLocations;
    double minLeft = std::numeric_limits<int>::max();
    double minTop = std::numeric_limits<int>::max();
    double maxRight = std::numeric_limits<int>::min();
    double maxBottom = std::numeric_limits<int>::min();

    QDomElement connectors = root.firstChildElement("connectors");
    QDomElement connector = connectors.firstChildElement("connector");
    while (!connector.isNull()) {
        QDomElement schematicView = connector.firstChildElement("views").firstChildElement("schematicView");
        QString svgID = schematicView.firstChildElement("p").attribute("svgId");
        if (svgID.isEmpty()) {
            message(QString("missing connector %1 in %2 schematic of %3").arg(connector.attribute("id")).arg(schematicFileName).arg(filename));
        }
        else {
            ConnectorLocation connectorLocation;
            connectorLocation.name = connector.attribute("name");
            connectorLocation.terminalID = schematicView.firstChildElement("p").attribute("terminalId");
            connectorLocation.svgID = svgID;
            QRectF bounds = renderer.boundsOnElement(svgID);
		    QMatrix m = renderer.matrixForElement(svgID);
		    connectorLocation.bounds = m.mapRect(bounds);
            connectorLocation.terminalPoint = connectorLocation.bounds.center();

            if (!connectorLocation.terminalID.isEmpty()) {
                bounds = renderer.boundsOnElement(connectorLocation.terminalID);
                m = renderer.matrixForElement(connectorLocation.terminalID);
                connectorLocation.terminalPoint = m.mapRect(bounds).center();
            }
            connectorLocations.append(connectorLocation);
            if (connectorLocation.terminalPoint.x() < minLeft) {
                minLeft = connectorLocation.terminalPoint.x();
            }
            if (connectorLocation.terminalPoint.x() > maxRight) {
                maxRight = connectorLocation.terminalPoint.x();
            }
            if (connectorLocation.terminalPoint.y() < minTop) {
                minTop = connectorLocation.terminalPoint.y();
            }
            if (connectorLocation.terminalPoint.y() > maxBottom) {
                maxBottom = connectorLocation.terminalPoint.y();
            }
        }

        connector = connector.nextSiblingElement("connector");
    }


    QRectF viewBox = renderer.viewBoxF();
    if (viewBox.isEmpty()) {
        qDebug() << "\tempty viewbox";
    }
    double fudge = qMax(maxRight - minLeft, maxBottom - minTop) / 200;


    message(schematicFileName);

    QList<ConnectorLocation> lefts;
    QList<ConnectorLocation> tops;
    QList<ConnectorLocation> rights;
    QList<ConnectorLocation> bottoms;

    foreach (ConnectorLocation connectorLocation, connectorLocations) {
        double d[4];
        d[0] = connectorLocation.terminalPoint.x() - viewBox.left();
        d[1] = connectorLocation.terminalPoint.y() - viewBox.top();
        d[2] = viewBox.right() - connectorLocation.terminalPoint.x();
        d[3] = viewBox.bottom() - connectorLocation.terminalPoint.y();
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
                lefts << connectorLocation;
                break;
            case 1:
                tops << connectorLocation;
                break;
            case 2:
                rights << connectorLocation;
                break;
            case 3:
                bottoms << connectorLocation;
                break;
            default:
                qDebug() << "shouldn't happen" << ix;
        }

    }

    qSort(lefts.begin(), lefts.end(), yLessThan);
    qSort(rights.begin(), rights.end(), yLessThan);
    qSort(tops.begin(), tops.end(), xLessThan);
    qSort(bottoms.begin(), bottoms.end(), xLessThan);

    QList<double> distances;
    for (int i = 1; i < lefts.count(); i++) {
        ConnectorLocation l1 = lefts.at(i - 1);
        ConnectorLocation l2 = lefts.at(i);
        distances << l2.terminalPoint.y() - l1.terminalPoint.y();
    }
    for (int i = 1; i < rights.count(); i++) {
        ConnectorLocation l1 = rights.at(i - 1);
        ConnectorLocation l2 = rights.at(i);
        distances << l2.terminalPoint.y() - l1.terminalPoint.y();
    }
    for (int i = 1; i < tops.count(); i++) {
        ConnectorLocation l1 = tops.at(i - 1);
        ConnectorLocation l2 = tops.at(i);
        distances << l2.terminalPoint.x() - l1.terminalPoint.x();
    }
    for (int i = 1; i < bottoms.count(); i++) {
        ConnectorLocation l1 = bottoms.at(i - 1);
        ConnectorLocation l2 = bottoms.at(i);
        distances << l2.terminalPoint.x() - l1.terminalPoint.x();
    }

    qSort(distances.begin(), distances.end());

    int totalPins = lefts.count() + rights.count() + bottoms.count() + tops.count();

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

    int biggest = dbins[0];
    int biggestIndex = 0;
    for (int i = 1; i < dbins.count(); i++) {
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
    message(QString("\tl:%1 t:%2 r:%3 b:%4").arg(lefts.count()).arg(tops.count()).arg(rights.count()).arg(bottoms.count()));


    double minPinV = 0;
    double maxPinV = 0;
    if (lefts.count() && rights.count()) {
        minPinV = qMin(lefts.first().terminalPoint.y(), rights.first().terminalPoint.y());
        maxPinV = qMax(lefts.last().terminalPoint.y(), rights.last().terminalPoint.y());
    }
    else if (rights.count()) {
        minPinV = rights.first().terminalPoint.y();
        maxPinV = rights.last().terminalPoint.y();
    }
    else if (lefts.count()) {
        minPinV = lefts.first().terminalPoint.y();
        maxPinV = lefts.last().terminalPoint.y();
    }
    int vUnits = qRound((maxPinV - minPinV) / oldUnit) + 2;

    double minPinH = 0;
    double maxPinH = 0;
    if (bottoms.count() && tops.count()) {
        minPinH = qMin(bottoms.first().terminalPoint.x(), tops.first().terminalPoint.x());
        maxPinH = qMax(bottoms.last().terminalPoint.x(), tops.last().terminalPoint.x());
    }
    else if (bottoms.count()) {
        minPinH = bottoms.first().terminalPoint.x();
        maxPinH = bottoms.last().terminalPoint.x();
    }
    else if (tops.count()) {
        minPinH = tops.first().terminalPoint.x();
        maxPinH = tops.last().terminalPoint.x();
    }
    int hUnits = qRound((maxPinH - minPinH) / oldUnit) + 2;

	double hPinTextMax = 0;
	QFont pinFont("Droid Sans", PinBigTextHeight * 72 / 25.4, QFont::Normal);
	QFontMetricsF pinfm(pinFont);
	for (int i = 0; i < lefts.count(); i++) {
		double w = pinfm.width(lefts.at(i).name);
		if (w > hPinTextMax) hPinTextMax = w;
	}
	for (int i = 0; i < rights.count(); i++) {
		double w = pinfm.width(rights.at(i).name);
		if (w > hPinTextMax) hPinTextMax = w;
	}
	double vPinTextMax = 0;
	for (int i = 0; i < tops.count(); i++) {
		double w = pinfm.width(tops.at(i).name);
		if (w > vPinTextMax) vPinTextMax = w;
	}
	for (int i = 0; i < bottoms.count(); i++) {
		double w = pinfm.width(bottoms.at(i).name);
		if (w > vPinTextMax) vPinTextMax = w;
	}

	QFont labelFont("Droid Sans", LabelTextHeight * 72 / 25.4, QFont::Normal);
	QFontMetricsF labelfm(labelFont);
    double labelTextMax = 0;
    foreach (QString title, titles) {
		double w = labelfm.width(title);
		if (w > labelTextMax) labelTextMax = w;
    }

    double tw = hPinTextMax + hPinTextMax + labelTextMax;
    double textWidth = 25.4 * tw / 90;              // millimeters
    double hTextUnits = qRound(textWidth / NewUnit);
    if (hTextUnits > hUnits) hUnits = hTextUnits;

    tw = vPinTextMax * 2;
    textWidth = 25.4 * tw / 90;
    textWidth += titles.count() * LabelTextHeight;
    double vTextUnits = qRound(textWidth / NewUnit);
    if (vTextUnits > vUnits) vUnits = vTextUnits;

    double fullWidth = hUnits;
    if (lefts.count()) fullWidth += 2;
    if (rights.count()) fullWidth += 2;
    double fullHeight = vUnits;
    if (tops.count()) fullHeight += 2;
    if (bottoms.count()) fullHeight += 2;

    QString svg = TextUtils::makeSVGHeader(25.4, 25.4, fullWidth * NewUnit, fullHeight * NewUnit);
    double rl = RectWidth / 2;
    double rt = rl;
    if (lefts.count()) rl += 2 * NewUnit;
    if (tops.count()) rt += 2 * NewUnit;
    svg += "<g id='schematic'>\n";
    svg += QString("<rect x='%1' y='%2' width='%3' height='%4' fill='%5' stroke='%6' stroke-width='%7' stroke-linecap='round' />\n")
                .arg(rl)
                .arg(rt)
                .arg((hUnits * NewUnit) - RectWidth)
                .arg((vUnits * NewUnit) - RectWidth)
                .arg(RectFillColor)
                .arg(RectColor)
                .arg(RectWidth)
                ;

    double vPinOffset = 0;
    if (tops.count()) vPinOffset = 2 * NewUnit;
    foreach (ConnectorLocation connectorLocation, rights) {
        int units = qRound((connectorLocation.terminalPoint.y() - minPinV) / oldUnit) + 1;
        svg += QString("<line x1='%1' y1='%2' x2='%3' y2='%2' fill='none' stroke='%4' stroke-width='%5' stroke-linecap='round' />\n")
            .arg(((fullWidth - 2) * NewUnit) - (PinWidth / 2))
            .arg(units * NewUnit + vPinOffset)
            .arg((fullWidth * NewUnit) - (PinWidth / 2))
            .arg(PinColor)
            .arg(PinWidth)
            ;
    }

    foreach (ConnectorLocation connectorLocation, lefts) {
        int units = qRound((connectorLocation.terminalPoint.y() - minPinV) / oldUnit) + 1;
        svg += QString("<line x1='%1' y1='%2' x2='%3' y2='%2' fill='none' stroke='%4' stroke-width='%5' stroke-linecap='round' />\n")
            .arg(PinWidth / 2)
            .arg(units * NewUnit + vPinOffset)
            .arg((2 * NewUnit) + (PinWidth / 2))
            .arg(PinColor)
            .arg(PinWidth)
            ;
    }

    double hPinOffset = 0;
    if (lefts.count()) hPinOffset = 2 * NewUnit;
    foreach (ConnectorLocation connectorLocation, bottoms) {
        int units = qRound((connectorLocation.terminalPoint.x() - minPinH) / oldUnit) + 1;
        svg += QString("<line x1='%1' y1='%2' x2='%1' y2='%3' fill='none' stroke='%4' stroke-width='%5' stroke-linecap='round' />\n")
            .arg(units * NewUnit + hPinOffset)
            .arg(((fullHeight - 2) * NewUnit) - (PinWidth / 2))
            .arg((fullHeight * NewUnit) - (PinWidth / 2))
            .arg(PinColor)
            .arg(PinWidth)
            ;
    }

    foreach (ConnectorLocation connectorLocation, tops) {
        int units = qRound((connectorLocation.terminalPoint.x() - minPinH) / oldUnit) + 1;
        svg += QString("<line x1='%1' y1='%2' x2='%1' y2='%3' fill='none' stroke='%4' stroke-width='%5' stroke-linecap='round' />\n")
            .arg(units * NewUnit + hPinOffset)
            .arg(PinWidth / 2)
            .arg((2 * NewUnit) + (PinWidth / 2))
            .arg(PinColor)
            .arg(PinWidth)
            ;
    }

    svg +="</g>\n";
    svg +="</svg>\n";

    QString newName = m_newSvgDir.absoluteFilePath(schematicFileName);
    bool result = TextUtils::writeUtf8(newName, svg);
    qDebug() << "";

}




