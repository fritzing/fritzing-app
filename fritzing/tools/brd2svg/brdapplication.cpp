#include "brdapplication.h"
#include "miscutils.h"

#include "stdio.h"

#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QtDebug>
#include <QFont>
#include <QDate>
#include <QRegExp>
#include <QFontMetricsF>
#include <QFontDatabase>
#include <QFontInfo>
#include <QResource>
#include <QProcess>
#include <qmath.h>
#include <QDomNodeList>
#include <QUrl>
#include <QNetworkRequest>
//#include <QScriptEngine>
//#include <QScriptValue>
#include <QTextDocument> 

#include <limits>

#include "utils/textutils.h"
#include "utils/graphicsutils.h"
#include "utils/schematicrectconstants.h"
#include "svg/svgfilesplitter.h"

#ifdef Q_WS_WIN
#define getenvUser() getenv("USERNAME")
#else
#define getenvUser() getenv("USER")
#endif

/////////////////////////////////
//
//	TODO:  
//
//		elliptical pads
//		draw holes with metal, but not treated as connector
//
//      pcb fails with no connectors
//
/////////////////////////////////
 
const QString DimensionsLayer("20");
const QString TopLayer("1");
const QString BottomLayer("16");
const QString PadsLayer("17");
const QString TopPlaceLayer("21");
const QString BottomPlaceLayer("22");

const qreal MinPCBPackageArea = 250 * 250;

const QString FritzingVersion = "0.5.2b.02.18.4756";

QList<QString> GroundNames;
QList<QString> PowerNames;
QList<QString> ExternalElementNames;

QString HybridString(" hybrid='yes' ");

QDomElement Spacer;

bool cxcyrw(QDomElement & element, qreal & cx, qreal & cy, qreal & radius, qreal & width);

inline QString SW(qreal strokeWidth)
{
	return QString("%1").arg(strokeWidth);
}

static QMultiHash<QString, class Renamer *> Renamers;

bool numericByIndex(QDomElement & e1, QDomElement & e2)
{
	return e1.attribute("connectorIndex").toInt() < e2.attribute("connectorIndex").toInt();
}

QString getConnectorIndex(const QDomElement & element) {
    return "connector" + element.attribute("connectorIndex") + "pin";
}

QString getConnectorName(const QDomElement & element)
{
	// note:  assumes GetSides has already been called
	QStringList packages;
	packages << "DUEMILANOVE_SHIELD_NOHOLES" << "DUEMILANOVE_SHIELD_NOLABELS" << "DUEMILANOVE_SHIELD";

	QString signal = element.attribute("signal");
	QList<Renamer *> renamers = Renamers.values(signal);
	if (renamers.count() > 0) {
        QDomElement package = element.parentNode().parentNode().toElement();
		QString packageName = package.attribute("name");
		QString elementName = package.parentNode().toElement().attribute("name");
        QString signalName = element.attribute("name");

        qDebug() << packageName << elementName << signalName;

        foreach (Renamer * renamer, renamers) {
            qDebug() << "\t" << renamer->package << renamer->element << renamer->name;

            if (renamer->package.compare(packageName) == 0
                   && renamer->name.compare(signalName) == 0)
            {
                return renamer->to;
            }
        }
        foreach (Renamer * renamer, renamers) {
            if (renamer->package.compare(packageName) == 0 && renamer->element.compare(elementName) == 0) {
                return renamer->to;
            }
        }
    }

	QString name = element.attribute("name");
	if (signal.contains(name, Qt::CaseInsensitive)) return signal;		// superset
	if (signal.isEmpty() || signal.startsWith("N$", Qt::CaseInsensitive)) return name;	// signal is empty
	if (name.isEmpty()) return signal;										// name is empty

	QDomElement package = element.parentNode().parentNode().toElement();
	QString tagName = package.tagName();
	if (tagName.compare("package") != 0) return signal;

	QString packageName = package.attribute("name");
	if (!packages.contains(packageName, Qt::CaseInsensitive)) return signal;

	QString ltrb = element.parentNode().toElement().attribute("ltrb");
	if (ltrb.compare("power") == 0) return signal;
	if (ltrb.compare("ground") == 0) return signal;

	//qDebug() << "using" << signal << "+" << name;
	return signal + " - " + name;
}

bool alphaBySignal(QDomElement & e1,QDomElement & e2)
{
	QString s1 = getConnectorName(e1).toLower();
	QString s2 = getConnectorName(e2).toLower();
	if (s1.isEmpty() || s2.isEmpty()) return s1 < s2;

	// if same prefix, try to sort by number

	int len = qMin(s1.length(), s2.length());
	int sameUpTo = -1;
	for (int ix = 0; ix < len; ix++) {
		if (s1.at(0) != s2.at(0)) break;

		sameUpTo = ix;
	}

	if (sameUpTo == len - 1) return s1 < s2;

	for (int ix = sameUpTo + 1; ix < s1.length(); ix++) {
		if (!s1.at(ix).isDigit()) return s1 < s2;
	}
	for (int ix = sameUpTo + 1; ix < s2.length(); ix++) {
		if (!s2.at(ix).isDigit()) return s1 < s2;
	}

	return s1.remove(0, sameUpTo + 1).toInt() < s2.remove(0, sameUpTo + 1).toInt();
}

Renamer::Renamer(const QDomElement & element)
{
	this->element = element.attribute("element");
	this->package = element.attribute("package");
	this->signal = element.attribute("signal");
    this->name = element.attribute("name");
	this->to = element.attribute("to");
}

///////////////////////////////////////////////////////

BrdApplication::BrdApplication(int argc, char *argv[]) : QCoreApplication(argc, argv)
{
	m_networkAccessManager = NULL;

	GroundNames << "gnd"<<  "vss" << "vee" << "com"; 
	PowerNames << "vcc" << "+5v" << "+3v" << "+3.3v" << "5v" << "3v" << "3.3v" << "-" << "+" << "vdd" << "v+" << "v-" << "vin" << "vout" << "+v";
	ExternalElementNames << "sv" << "j";

	m_core = "core";
}

void BrdApplication::start() {
    if (!initArguments()) {
        usage();
        return;
    }

	QDir workingFolder(m_workingPath);
	QDir fzpFolder, breadboardFolder, schematicFolder, pcbFolder, iconFolder;
	if (!MiscUtils::makePartsDirectories(workingFolder, m_core, fzpFolder, breadboardFolder, schematicFolder, pcbFolder, iconFolder)) return;

	workingFolder.mkdir("xml");
	workingFolder.mkdir("bins");
	workingFolder.mkdir("params");
	workingFolder.mkdir("descriptions");

	QDir xmlFolder(m_workingPath);
	xmlFolder.cd("xml");
	if (!xmlFolder.exists()) {
		qDebug() << QString("unable to create xml folder:%1").arg(xmlFolder.absolutePath());
		return;
	}

	QDir allPackagesFolder(m_andPath);
	QString AllPackagesPath = allPackagesFolder.absoluteFilePath("all.packages.txt");
    qDebug();
    qDebug() << "looking for all.packages.txt in" << AllPackagesPath;
    qDebug();

	QDir paramsFolder(workingFolder);
	paramsFolder.cd("params");
	if (!paramsFolder.exists()) {
		qDebug() << QString("unable to find params folder:%1").arg(paramsFolder.absolutePath());
		return;	
	}

	QDir binsFolder(m_workingPath);
	binsFolder.cd("bins");
	if (!binsFolder.exists()) {
		qDebug() << QString("unable to create bins folder:%1").arg(binsFolder.absolutePath());
		return;
	}

	QDir brdFolder(workingFolder);
	brdFolder.cd("brds");
	if (!brdFolder.exists()) {
		qDebug() << QString("unable to find brds folder:%1").arg(brdFolder.absolutePath());
		return;	
	}

	QDir descriptionsFolder(workingFolder);
	descriptionsFolder.cd("descriptions");
	if (!descriptionsFolder.exists()) {
		qDebug() << QString("unable to find descriptions folder:%1").arg(brdFolder.absolutePath());
		return;	
	}

    QDir andFolder(m_andPath);
    bool fontsOK = registerFonts();
    if (!fontsOK) {
		qDebug() << "unable to register fonts";
		return;	
    }

	QStringList nameFilters;
	nameFilters << "*.brd";
	QStringList fileList = brdFolder.entryList(nameFilters, QDir::Files | QDir::NoDotAndDotDot);
	foreach (QString filename, fileList) {
		genXml(workingFolder, andFolder, filename, xmlFolder);
	}

	QHash<QString, DifParam *> difParams;
	loadDifParams(workingFolder, difParams);

	QStringList ICs;
    QHash<QString, QString> SubpartAliases;
	QFile file(AllPackagesPath);
	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument doc;
	if (doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {	
		QDomElement root = doc.documentElement();
		QDomElement package = root.firstChildElement("package");
		while (!package.isNull()) {
			if (package.attribute("ic").compare("yes", Qt::CaseInsensitive) == 0) {
				ICs.append(package.attribute("name"));
			}
			package = package.nextSiblingElement("package");
		}
		QDomElement map = root.firstChildElement("map");
		while (!map.isNull()) {
			QString from = map.attribute("package");
            QString to = map.attribute("to");
            if (!from.isEmpty() && !to.isEmpty()) {
                SubpartAliases.insert(to.toLower(), from);
            }
			map = map.nextSiblingElement("map");
		}
	}

	//QString txt = TextUtils::escapeAnd(this->loadDescription("ThermalPrinter", "http://www.sparkfun.com/products/10438", descriptionsFolder));


	QSet<QString> packageNames;
	foreach (QString filename, fileList) {
		QString errorStr;
		int errorLine;
		int errorColumn;

		QFileInfo fileInfo(filename);
		QString basename = fileInfo.completeBaseName();
		QString xmlname = basename + ".xml";

		DifParam * difParam = difParams.value(basename.toLower(), NULL);
		QFile file(xmlFolder.absoluteFilePath(xmlname));
		if (!m_boardDoc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
			message(QString("unable to parse board xml '%1': %2 line:%3 col:%4").arg(xmlname).arg(errorStr).arg(errorLine).arg(errorColumn));
			qDebug() << "";
			continue;
		}

		QDomElement root = m_boardDoc.documentElement();
		if (root.isNull()) {
			message(QString("file '%1' has no root").arg(xmlname));
			qDebug() << "";
			continue;
		}

		if (root.tagName() != "board") {
			message(QString("file '%1' was not generated by brd2xml.ulp").arg(xmlname));
			qDebug() << "";
			continue;
		}

		qDebug() << "parsing" << xmlname;

		m_boardBounds = m_trueBounds = getDimensions(root, m_maxElement, DimensionsLayer, false);
		if (m_maxElement.isNull()) {
			qDebug() << "No board bounds found!";
		}

		//qDebug() << "true bounds:" << m_trueBounds;

		if (!m_genericSMD) {
			getPackagesBounds(root, m_trueBounds, "", false, false);
			//qDebug() << "adjusted true bounds:" << m_trueBounds;
		}

		QString paramsPath = paramsFolder.absoluteFilePath(basename + ".params");
		QFile paramsFile(paramsPath);

		QDomDocument paramsDoc;
		if (paramsFile.exists()) {
			paramsDoc = loadParams(paramsFile, basename);
		}
        QDomElement paramsRoot = paramsDoc.documentElement();
		QDomElement connectors = paramsRoot.firstChildElement("connectors");
		QDomElement renames = connectors.firstChildElement("renames");
		QDomElement rename = renames.firstChildElement("rename");
		while (!rename.isNull()) {
			Renamer * renamer =  new Renamer(rename);
			Renamers.insert(renamer->signal, renamer);

			rename = rename.nextSiblingElement("rename");
		}

		QList<QDomElement> packages;
		collectPackages(root, packages);
		foreach (QDomElement package, packages) {
			QString name = package.attribute("name", "").toLower();
			packageNames.insert(name);
		}

		m_shrinkHolesFactor = 1.0;
		QString shf = paramsRoot.attribute("shrink-holes-factor", "");
		if (!shf.isEmpty()) {
			bool ok;
			qreal shfn = shf.toDouble(&ok);
			if (ok) {
				m_shrinkHolesFactor = shfn;
			}
		}


		//qDebug() << "generating schematic";
        QString schematicsvg = genSchematic(root, paramsRoot, difParam);
		SvgFileSplitter splitter;
		splitter.load(schematicsvg);
        double factor;
		splitter.normalize(72, "", false, factor);
		saveFile(splitter.toString(), schematicFolder.absoluteFilePath(basename + "_schematic.svg"));

		//qDebug() << "generating pcb";
        QString pcbsvg = genPCB(root, paramsRoot);
		splitter.load(pcbsvg);
		splitter.normalize(72, "", false, factor);
		saveFile(splitter.toString(), pcbFolder.absoluteFilePath(basename + "_pcb.svg"));

		//qDebug() << "generating breadboard";

		QString breadboardsvg;
		if (m_genericSMD) {
			breadboardsvg = genGenericBreadboard(root, paramsRoot, difParam, workingFolder);
		}
		else {
			breadboardsvg = genBreadboard(root, paramsRoot, difParam, ICs, SubpartAliases);
		}
		splitter.load(breadboardsvg);
		splitter.normalize(72, "", false, factor);
		saveFile(splitter.toString(), breadboardFolder.absoluteFilePath(basename + "_breadboard.svg"));

		//qDebug() << "generating fzp";
		QString gender = paramsRoot.attribute("gender", "female");
        QString fzp = genFZP(root, paramsRoot, difParam, basename, gender, descriptionsFolder);
		QString fzpName = basename + ".fzp";
		if (m_genericSMD) fzpName = "SMD_" + fzpName;
		saveFile(fzp, fzpFolder.absoluteFilePath(fzpName));

		if (!paramsFile.exists()) {
			qDebug() << "generating params";
			QString params = genParams(root, basename);
			saveFile(params, paramsPath);
		}

		qDebug() << "";
	}

		
	if (!QFileInfo(AllPackagesPath).exists()) {
		QStringList names;
		foreach (QString name, packageNames) {
			names << name;
		}
		qSort(names);
		QString packageString;
		packageString += "<packages>\n";
		foreach (QString name, names) {
			packageString += QString("<package name='%1' ic='no' />\n").arg(Qt::escape(name));
		}
		packageString += "</packages>\n";
		saveFile(packageString, AllPackagesPath);
	}

	qDebug() << "generating bin";
	qDebug() << "";
    QString binName = workingFolder.dirName();
	genBin(fileList, binName, binsFolder.absoluteFilePath(binName + ".fzb"));

	qDebug() << "done";
	qDebug() << "";
}

void BrdApplication::getPackagesBounds(QDomElement & root, QRectF & bounds, const QString & layer, bool reset, bool deep) {
	if (reset) {
		bounds.setCoords(std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), std::numeric_limits<int>::min(), std::numeric_limits<int>::min());
	}
	QList<QDomElement> padSmdPackages;
	collectPadSmdPackages(root, padSmdPackages);
	foreach (QDomElement package, padSmdPackages) {
		qreal x1, y1, x2, y2;
		if (!MiscUtils::x1y1x2y2(package, x1, y1, x2, y2)) continue;

		QDomElement maxElement;
		QRectF r = getDimensions(package, maxElement, layer, deep);
		
		if (r.left() < bounds.left()) {
			bounds.setLeft(r.left());
		}
		if (r.right() > bounds.right()) {
			bounds.setRight(r.right());
		}
		if (r.top() < bounds.top()) {
			bounds.setTop(r.top());
		}
		if (r.bottom() > bounds.bottom()) {
			bounds.setBottom(r.bottom());
		}
	}
}

void BrdApplication::genBin(QStringList & fileList, const QString & titleString, const QString & binPath) {

	QFile file(binPath);
	QString errorStr;
	int errorLine;
	int errorColumn;

	QDomDocument binDoc;
	if (!binDoc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		QDomElement module = binDoc.createElement("module");
		binDoc.appendChild(module);
		module.setAttribute("fritzingVersion", FritzingVersion); 
		QDomElement title = binDoc.createElement("title");
		module.appendChild(title);
		QDomNode text = binDoc.createTextNode(titleString);
		title.appendChild(text);
		QDomElement instances = binDoc.createElement("instances");
		module.appendChild(instances);
	}
	file.close();

	QDomElement root = binDoc.documentElement();
	QDomElement instances = root.firstChildElement("instances");
	if (instances.isNull()) {
		qDebug() << "bin file has no 'instances' element";
		return;
	}

	foreach (QString filename, fileList) {
		QFileInfo fileInfo(filename);
		QString basename = fileInfo.completeBaseName();
		QDomElement instance = instances.firstChildElement("instance");
		bool gotOne = false;
		while (!instance.isNull()) {
			if (instance.attribute("moduleIdRef").compare(basename) == 0) {
				gotOne = true;
				break;
			}

			instance = instance.nextSiblingElement("instance");
		}
		if (!gotOne) {
			QDomElement instance = binDoc.createElement("instance");
			instances.appendChild(instance);
			instance.setAttribute("moduleIdRef", basename);
			QDomElement views = binDoc.createElement("views");
			instance.appendChild(views);
			QDomElement iconView = binDoc.createElement("iconView");
			views.appendChild(iconView);
			iconView.setAttribute("layer", "icon");
			QDomElement geometry = binDoc.createElement("geometry");
			iconView.appendChild(geometry);
			geometry.setAttribute("x", "-1");
			geometry.setAttribute("y", "-1");
			geometry.setAttribute("z", "-1");
		}
	}

	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream out(&file);
		out.setCodec("UTF-8");
		binDoc.save(out,1,QDomNode::EncodingFromTextStream);
		file.close();
	}
}

QDomDocument BrdApplication::loadParams(QFile & paramsFile, const QString & basename) {
	QDomDocument paramsDoc;
	QString errorStr;
	int errorLine;
	int errorColumn;

	if (!paramsDoc.setContent(&paramsFile, true, &errorStr, &errorLine, &errorColumn)) {
		message(QString("unable to parse params file '%1': %2 line:%3 col:%4").arg(basename).arg(errorStr).arg(errorLine).arg(errorColumn));
	}

	return paramsDoc;
}

void BrdApplication::saveFile(const QString & content, const QString & path) 
{
	QFile file(path);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream out(&file);
		out.setCodec("UTF-8");
		out << content;
		file.close();
	}
}

bool BrdApplication::initArguments() {
	m_workingPath = m_eaglePath = "";
	m_genericSMD = false;
    QStringList args = QCoreApplication::arguments();
    for (int i = 0; i < args.length(); i++) {
        if ((args[i].compare("-h", Qt::CaseInsensitive) == 0) ||
            (args[i].compare("-help", Qt::CaseInsensitive) == 0) ||
            (args[i].compare("--help", Qt::CaseInsensitive) == 0))
        {
            return false;
        }

		if ((args[i].compare("-g", Qt::CaseInsensitive) == 0) ||
            (args[i].compare("-generic", Qt::CaseInsensitive) == 0)||
            (args[i].compare("--generic", Qt::CaseInsensitive) == 0))
        {
             m_genericSMD = true;
			 continue;
        }

		if (i + 1 < args.length()) {
			if ((args[i].compare("-w", Qt::CaseInsensitive) == 0) ||
				(args[i].compare("-working", Qt::CaseInsensitive) == 0)||
				(args[i].compare("--working", Qt::CaseInsensitive) == 0))
			{
				m_workingPath = args[++i];
			}
			else if ((args[i].compare("-e", Qt::CaseInsensitive) == 0) ||
				(args[i].compare("-eagle", Qt::CaseInsensitive) == 0)||
				(args[i].compare("--eagle", Qt::CaseInsensitive) == 0))
			{
				 m_eaglePath = args[++i];
			}
			else if ((args[i].compare("-a", Qt::CaseInsensitive) == 0) ||
				(args[i].compare("-and", Qt::CaseInsensitive) == 0)||
				(args[i].compare("--and", Qt::CaseInsensitive) == 0))
			{
				 m_andPath = args[++i];
			}
			else if ((args[i].compare("-c", Qt::CaseInsensitive) == 0) ||
				(args[i].compare("-core", Qt::CaseInsensitive) == 0)||
				(args[i].compare("--core", Qt::CaseInsensitive) == 0))
			{
				m_core = args[++i];
			}
			else if ((args[i].compare("-s", Qt::CaseInsensitive) == 0) ||
				(args[i].compare("-subparts", Qt::CaseInsensitive) == 0)||
				(args[i].compare("--subparts", Qt::CaseInsensitive) == 0))
			{
				m_fritzingSubpartsPath = args[++i];
			}

		}
    }

    if (m_eaglePath.isEmpty()) {
        message("-e <path to eagle executable> parameter missing");
        return false;
    }

    if (m_workingPath.isEmpty()) {
        message("-b <path to brd folder> parameter missing");
        return false;
    }

    QDir directory(m_workingPath);
    if (!directory.exists()) {
        message(QString("working folder '%1' not found").arg(m_workingPath));
        return false;
    }

    QFile file(m_eaglePath);
    if (!file.exists()) {
        message(QString("eagle executable '%1' not found").arg(m_eaglePath));
        return false;
    }

    return true;
}

void BrdApplication::usage() {
    message("\nusage: brd2svg -w <path to working folder (containing brds folder)> "
                "-e <path to eagle executable> "
                "-c <core or contrib> "
                "-g (set only for generic smd ic generation) "
                "-s <path to subparts folder> "
                "-p <path to second subparts folder> "
                "-a <path to 'and' folder> "
                "\n"
    );
}

void BrdApplication::message(const QString & msg) {
   // QTextStream cout(stdout);
  //  cout << msg;
   // cout.flush();

	qDebug() << msg;
 }

void BrdApplication::collectLayerElements(QList<QDomElement> & from, QList<QDomElement> & to, const QString & layerID) {
	foreach (QDomElement element, from) {
		if (element.isNull()) continue;

		QDomElement child = element.firstChildElement();
		while (!child.isNull()) {
			QString layer = child.attribute("layer", "");
			
			if (layer.compare(layerID) == 0 || layerID.isEmpty()) {
				to.append(child);
			}
			child = child.nextSiblingElement();
		}
	}
}

QRectF BrdApplication::getDimensions(QDomElement & root, QDomElement & maxElement, const QString & layer, bool deep) 
{
	qreal left = std::numeric_limits<int>::max();
	qreal right = std::numeric_limits<int>::min();
	qreal top = std::numeric_limits<int>::max();
	qreal bottom = std::numeric_limits<int>::min();

	QList<QDomElement> from;
	from.append(root.firstChildElement("wires"));
	from.append(root.firstChildElement("circles"));
	from.append(root.firstChildElement("polygons"));
	from.append(root.firstChildElement("rects"));
	QList<QDomElement> to;
	collectLayerElements(from, to, layer);

	if (deep) {
		addElements(root, to, 0);
		QDomElement contacts = root.firstChildElement("contacts");
		QDomElement contact = contacts.firstChildElement("contact");
		while (!contact.isNull()) {
			QDomElement pad = contact.firstChildElement("pad");
			if (!pad.isNull()) {
				to.append(pad);
			}
			else {
				QDomElement smd = contact.firstChildElement("smd");
				if (!smd.isNull()) to.append(smd);
			}

			contact = contact.nextSiblingElement("contact");
		}
	}

	foreach (QDomElement element, to) {
		if (element.isNull()) continue;

        QString string;
        QTextStream stream(&string);
        element.save(stream, 0);

		QRectF r = getPlainBounds(element, layer);
		if (!r.isNull()) {
			if (r.left() < left) {
				left = r.left();
				maxElement = element;
			}
			if (r.right() > right) {
				right = r.right();
				maxElement = element;
			}
			if (r.top() < top) {
				top = r.top();
				maxElement = element;
			}
			if (r.bottom() > bottom) {
				bottom = r.bottom();
				maxElement = element;
			}
		}
	}

	return QRectF(left, top, right - left, bottom - top);
}

QRectF BrdApplication::getPlainBounds(QDomElement & element, const QString & layer) {
	QString tagName = element.tagName();

	QRectF bounds;
	if (tagName.compare("circle") == 0) {
		qreal cx, cy, radius, width;
		if (!cxcyrw(element, cx, cy, radius, width)) return bounds;

		bounds.setCoords(cx - radius, cy - radius, cx + radius, cy + radius);
		return bounds;
	}

	if (tagName.compare("wire") == 0) {
		qreal left = std::numeric_limits<int>::max();
		qreal right = std::numeric_limits<int>::min();
		qreal top = std::numeric_limits<int>::max();
		qreal bottom = std::numeric_limits<int>::min();

		QDomElement piece = element.firstChildElement("piece");
		while (!piece.isNull()) {
			QDomElement line = piece.firstChildElement("line");
			qreal x1, y1, x2, y2;
			bool ok = false;
			if (!line.isNull()) {
				ok = MiscUtils::x1y1x2y2(line, x1, y1, x2, y2);
			}
			else {
				QDomElement arc = piece.firstChildElement("arc");
				if (!arc.isNull()) {
					ok = getArcBounds(element, arc, x1, y1, x2, y2);
				}
			}

			if (ok) {
				left = qMin(left, qMin(x1, x2));
				right = qMax(right, qMax(x1, x2));
				top = qMin(top, qMin(y1, y2));
				bottom = qMax(bottom, qMax(y1, y2));
			}

			piece = piece.nextSiblingElement("piece");
		}

		bounds.setCoords(left, top, right, bottom);
		return bounds;
	}

	if (tagName.compare("rect") == 0) {
		qreal x1, y1, x2, y2;
		if (!MiscUtils::x1y1x2y2(element, x1, y1, x2, y2)) return bounds;

		if (x2 < x1) {
			qreal temp = x1;
			x1 = x2;
			x2 = temp;
		}

		if (y2 < y1) {
			qreal temp = y1;
			y1 = y2;
			y2 = temp;
		}

		bounds.setCoords(x1, y1, x2, y2);
		return bounds;
	}

	if (tagName.compare("polygon") == 0 ) {
		qreal left = std::numeric_limits<int>::max();
		qreal right = std::numeric_limits<int>::min();
		qreal top = std::numeric_limits<int>::max();
		qreal bottom = std::numeric_limits<int>::min();

		QList<QDomElement> wires;
		collectWires(element, wires, false);
		bool gotOne = false;
		foreach (QDomElement wire, wires) {
			QRectF r = getPlainBounds(wire, layer);
			if (!r.isNull()) {
				left = qMin(left, r.left());
				right = qMax(right, r.right());
				top = qMin(top, r.top());
				bottom = qMax(bottom, r.bottom());
				gotOne = true;
			}
		}
		if (gotOne) {
			bounds.setCoords(left, top, right, bottom);
		}
		return bounds;
	}

	if (tagName.compare("pad") == 0) {
		bool ok = true;
		qreal cx = MiscUtils::strToMil(element.attribute("x", ""), ok);
		if (!ok) return bounds;

		qreal cy =  MiscUtils::strToMil(element.attribute("y", ""), ok);
		if (!ok) return bounds;

		qreal diameter = 0;
		QDomElement layer = element.firstChildElement("layer");
		while (!layer.isNull()) {
			qreal diameter = MiscUtils::strToMil(element.attribute("diameter", "0"), ok);
			if (diameter != 0) break;
		}

		return QRectF(cx - (diameter / 2), cy - (diameter / 2), diameter, diameter);
	}

	if (tagName.compare("smd") == 0) {
		bool ok = true;
		qreal cx = MiscUtils::strToMil(element.attribute("x", ""), ok);
		if (!ok) return bounds;

		qreal cy =  MiscUtils::strToMil(element.attribute("y", ""), ok);
		if (!ok) return bounds;

		qreal w = MiscUtils::strToMil(element.attribute("dx", ""), ok);
		if (!ok) return bounds;

		qreal h = MiscUtils::strToMil(element.attribute("dy", ""), ok);
		if (!ok) return bounds;

		return QRectF(cx - (w / 2), cy - (h / 2), w, h);
	}

	if (tagName.compare("element") == 0) {
		QDomElement maxElement;
		getDimensions(element, maxElement, layer, false);
		QDomElement package = element.firstChildElement("package");
		if (!package.isNull()) {
			getDimensions(package, maxElement, layer, false);
		}
		return bounds;
	}

	qDebug() << "getPlainBounds missed tag" << tagName;
	return bounds;
}

bool BrdApplication::getArcBounds(QDomElement wire, QDomElement arc, qreal & x1, qreal & y1, qreal & x2, qreal & y2)
{
	qreal ax1, ay1, ax2, ay2;
	if (!MiscUtils::x1y1x2y2(arc, ax1, ay1, ax2, ay2)) return false;

	qreal radius, width, angle1, angle2;
	if (!MiscUtils::rwaa(arc, radius, width, angle1, angle2)) return false;

	bool ok;
	qreal xc = MiscUtils::strToMil(arc.attribute("xc", ""), ok);
	if (!ok) return false;

	qreal yc = MiscUtils::strToMil(arc.attribute("yc", ""), ok);
	if (!ok) return false;

	// from http://groups.google.com/group/comp.graphics.algorithms/browse_thread/thread/1adbcc734e44d024/79201c57a09149fe?lnk=gst&q=arc+bounding+box#79201c57a09149fe
	// Assuming that the arc is the counterclockwise arc from s to e. 
	// Precondition: 0 <= s < 360  and 0 <= e < 360 
	if (angle2 < angle1) angle2 = angle2 + 360; 
	x1 = qMin(ax1, ax2);
	y1 = qMin(ay1, ay2);
	x2 = qMax(ax1, ax2);
	y2 = qMax(ay1, ay2);

	if (angle2 > 90) { 
		if (angle1 < 90) y2 = yc + radius; 
		if (angle2 > 180) { 
			if (angle1 < 180) x1 = xc - radius; 
			if (angle2 > 270) { 
				if (angle1 < 270) y1 = yc - radius; 
				if (angle2 > 360) { 
					x2 = xc + radius; 
					if (angle2 > 450) { 
						y2 = yc + radius; 
						if (angle2 > 540) { 
							x1 = xc - radius; 
							if (angle2 > 630) y1 = yc - radius; 
						} 
					} 
				} 
			} 
		} 
	} 

	return true;
}

void BrdApplication::genXml(QDir & workingFolder, QDir & ulpDir, const QString & brdname, QDir & xmlFolder) {
	QFileInfo fileInfo(brdname);
	QString targetname = xmlFolder.absoluteFilePath(fileInfo.completeBaseName() + ".xml");
	QFile target(targetname);
	if (target.exists()) return;

	QDir brdFolder(workingFolder);
	brdFolder.cd("brds");

	QFile file(workingFolder.absoluteFilePath("brd2xml.scr"));
	file.remove();
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream out(&file);
		// note the single quotes around filenames--this is eagle's non-standard way of dealing with spaces
        out << QString("EDIT '%1';\nRUN '%3' '%2';\nQUIT;\n")
                        .arg(brdFolder.absoluteFilePath(brdname))
                        .arg(targetname)
                        .arg(ulpDir.absoluteFilePath("brd2xml.ulp"));
		file.close();
		
		QProcess process;
        QString thing = brdFolder.absolutePath();
        process.setWorkingDirectory(thing);

        process.start(m_eaglePath, QStringList() << "-C" << QString("SCRIPT %1").arg(workingFolder.absoluteFilePath("brd2xml.scr")) << "doesntexist.brd");
		if (!process.waitForStarted()) {
			qDebug() << QString("unable to start %1").arg(brdname);
			file.remove();
			return;
		}

		if (!process.waitForFinished()) {
			qDebug() << QString("unable to finish %1").arg(brdname);
			file.remove();
			return;
		}

		qDebug() << QString("wrote %1 xml").arg(brdname);
		file.remove();
	}
	else {
		qDebug() << "unable to open" << "brd2xml.scr";
	}
}

QString BrdApplication::genParams(QDomElement & root, const QString & prefix) 
{
	QString params = "<?xml version='1.0' encoding='UTF-8'?>\n";
	params += QString("<board-params board='%1' include-vias='false' shrink-holes-factor='1.0' >\n").arg(prefix);

	params += ("<!-- please fill in the author, title, description, tags, and properties elements-->\n");
	params += QString("<author>%1</author>\n").arg(getenvUser());
	params += QString("<title></title>\n");
	params += QString("<url></url>\n");
	params += QString("<description></description>\n");
    params += QString("<tags>\n");    
	params += QString("<!-- <tag>sample tag</tag> -->\n");   
	params += QString("</tags>\n");
    params += QString("<properties>\n");
	params += QString("<!-- <property name='family'>sample family</property> -->\n");
    params += QString("</properties>\n");

	params += QString("<breadboard breadboard-color='%1'>\n").arg("#1F7A34");
	params += QString("<extra-layers>\n");
	params += ("<!-- to add extra layers to the breadboard,\n"
					"uncomment the following <layer> elements,\n"
					"and change the 'number' attribute to the appropriate eagle layer number.\n"
					"You can add as many <layer> elements as you like -->\n");
	
	params += QString("<layer number='21' />\n");
	params += QString("<!-- <layer number='1' /> -->\n");
	params += QString("<!-- <layer number='29' /> -->\n");
	params += QString("</extra-layers>\n");

	params += QString("<includes>\n");
	params += ("<!-- to include other svgs in the breadboard,\n"
					"uncomment the following <include> elements,\n"
					"and change the src and coordinates (coordinates without units are treated as 90 dpi px).\n"
					"You can add as many <include> elements as you like -->\n");
	
	params += QString("<!-- <include src='full path name to something.svg' x='0in' y='0in'/> -->\n");
	params += QString("</includes>\n");
	params += QString("<!-- Add 'nudges' to modify how packages and texts are displayed  -->\n");
	params += QString("<nudges>\n"
					  "<!-- <nudge element='JP13' package='SFE-NEW-WEBLOGO' lines='no'/> -->\n"
					  "<!-- <nudge element='JP20' package='SFE-NEW-WEBLOGO' show='no'/> -->\n"
					  "<!-- <nudge element='JP17' package='1X08' x='-1.1mm' y='0.2mm' angle='90' gender='female' />  -->\n"
					  "<!-- <nudge element='R1' text='330' x='2mm'><match y='2299' /></nudge>  -->\n"
					  "</nudges>\n");

	params += QString("</breadboard>\n");

	QList<QDomElement> powers;
	QList<QDomElement> grounds;
	QList<QDomElement> lefts;
	QList<QDomElement> rights;
	QList<QDomElement> unused;
	QList<QDomElement> vias;
	QDomElement paramsRoot;

	QStringList busNames;
	getSides(root, paramsRoot, powers, grounds, lefts, rights, unused, vias, busNames, false, true);
	params += QString("<connectors>\n");

	params += QString("<!-- Add 'rename' elements to modify how signals are named in fritzing  -->\n");
	params += QString("<renames> -->\n");
	params += QString("<!-- <rename signal='' to='lionel' package='MA06-1' element='FTDI' /> -->\n");
	params += QString("<!-- <rename signal='N$12' to='i dunno' package='MA06-1' element='FTDI' /> -->\n");
	params += QString("</renames>\n");


	params += QString("<power>\n");
	params += QString("<!-- left to right -->\n");
	foreach(QDomElement contact, powers) {
		params += genContact(contact);
	}
	params += QString("</power>\n");
	params += QString("<ground>\n");
	params += QString("<!-- left to right -->\n");
	foreach(QDomElement contact, grounds) {
		params += genContact(contact);
	}
	params += QString("</ground>\n");
	params += QString("<left>\n");
	params += QString("<!-- top to bottom -->\n");
	foreach(QDomElement contact, lefts) {
		params += genContact(contact);
	}
	params += QString("</left>\n");
	params += QString("<right>\n");
	params += QString("<!-- top to bottom -->\n");
	foreach(QDomElement contact, rights) {
		params += genContact(contact);
	}
	params += QString("</right>\n");
	params += QString("<unused>\n");
	foreach(QDomElement contact, unused) {
		params += genContact(contact);
	}
	params += QString("</unused>\n");

	params += QString("</connectors>\n");

	params += QString("<!-- Add fake-via connectors to change pads to vias for hybrid parts  -->\n");
	params += QString("<!-- <fake-vias>  -->\n");
	params += QString("<!--		<connector signal='' id='173' package='1X3_STRIP' element='U$10' name='2' type='pad'/> -->\n");
	params += QString("<!-- </fake-vias> -->\n");

	params += QString("<!-- Add buses to connect connectors that don't have matching signal names  -->\n");

	params += QString("<!-- <buses>  -->\n");
	params += QString("<!--		<bus>  -->\n");
	params += QString("<!--			<connector signal='' id='173' package='1X3_STRIP' element='U$10' name='2' type='pad'/>  -->\n");
	params += QString("<!--			<connector signal='' id='174' package='1X3_STRIP' element='U$10' name='3' type='pad'/>  -->\n");
	params += QString("<!--			<via signal='N$84' />  -->\n");
	params += QString("<!--		</bus>  -->\n");
	params += QString("<!-- </buses>  -->\n");

	params += QString("</board-params>\n");
	return params;
}

QString BrdApplication::genContact(QDomElement & contact) {
	QString type;
	QString signal = contact.attribute("signal");
	if (!contact.firstChildElement("pad").isNull()) type = "pad";
	else if (!contact.firstChildElement("smd").isNull()) type = "smd";

	return QString("<connector signal='%1' id='%2' package='%3' element='%4' name='%5' type='%6'/>\n")
			.arg(TextUtils::escapeAnd(signal))
			.arg(contact.attribute("connectorIndex"))
			.arg(TextUtils::escapeAnd(contact.parentNode().parentNode().toElement().attribute("name")))
			.arg(TextUtils::escapeAnd(contact.parentNode().parentNode().parentNode().toElement().attribute("name")))
			.arg(TextUtils::escapeAnd(contact.attribute("name")))
			.arg(type)
			;
}

QString BrdApplication::genFZP(QDomElement & root, QDomElement & paramsRoot, DifParam* difParam, const QString & prefix, const QString & connectorType, const QDir & descriptionsFolder) 
{
	QString fzp = "<?xml version='1.0' encoding='UTF-8'?>\n";
	fzp += QString("<module fritzingVersion='%2' moduleId='%1'>\n").arg(prefix).arg(FritzingVersion);
	fzp += QString("<version>4</version>\n");
    fzp += QString("<date>%1</date>\n").arg(QDate::currentDate().toString(Qt::ISODate));

	QString author = getenvUser();
	QStringList tags;
	QHash<QString, QString> properties;
	QString description;
	QString title = prefix;
	QString url;

	QList< QStringList > genders;

	if (!paramsRoot.isNull()) {

		QDomElement bb = paramsRoot.firstChildElement("breadboard");
		QDomElement nudges = bb.firstChildElement("nudges");
		QDomElement nudge = nudges.firstChildElement("nudge");
		while (!nudge.isNull()) {
			QString gender = nudge.attribute("gender");
			if (!gender.isEmpty()) {
				QString element = nudge.attribute("element");
				QString package = nudge.attribute("package");
				QStringList strings;
				strings << element << package << gender;
				genders.append(strings);
			}
			nudge = nudge.nextSiblingElement("nudge");
		}

		QDomElement authorElement = paramsRoot.firstChildElement("author");
		QString a;
		TextUtils::findText(authorElement, a);
		if (!a.isEmpty()) author = a;

		QDomElement titleElement = paramsRoot.firstChildElement("title");
		QString t;
		TextUtils::findText(titleElement, t);
		if (!t.isEmpty()) title = t;

		QDomElement descElement = paramsRoot.firstChildElement("description");
		QString d;
		TextUtils::findText(descElement, d);
		if (!d.isEmpty()) description = d;

		QDomElement urlElement = paramsRoot.firstChildElement("url");
		QString u;
		TextUtils::findText(urlElement, u);
		if (!u.isEmpty()) url = u;

		QDomElement tagsElement = paramsRoot.firstChildElement("tags");
		QDomElement tagElement = tagsElement.firstChildElement("tag");
		while (!tagElement.isNull()) {
			QString t;
			TextUtils::findText(tagElement, t);
			if (!t.isEmpty()) tags.append(t);
			tagElement = tagElement.nextSiblingElement("tag");
		}
		
		QDomElement propsElement = paramsRoot.firstChildElement("properties");
		QDomElement propElement = propsElement.firstChildElement("property");
		while (!propElement.isNull()) {
			QString n = propElement.attribute("name");
			QString v;
			TextUtils::findText(propElement, v);
			if (!n.isEmpty() && !v.isEmpty()) properties.insert(n, v);
			propElement = propElement.nextSiblingElement("property");
		}

		if (difParam != NULL) {
			if (!difParam->url.isEmpty()) url = difParam->url;
			if (!difParam->author.isEmpty()) author = difParam->author;
			if (!difParam->description.isEmpty()) description = difParam->description;
			if (!difParam->title.isEmpty()) {
				//difParam->title.replace("-", " ");
				//difParam->title.replace("_", " ");
				title = difParam->title;
			}
			if (difParam->tags.count() > 0) {
				tags.clear();
				tags.append(difParam->tags);
			}
			if (difParam->properties.count() > 0) {
				properties.clear();
				foreach (QString prop, difParam->properties.keys()) {
					properties.insert(prop, difParam->properties.value(prop));
				}
			}
							
		}

		if (description.isEmpty()) {
			description = loadDescription(prefix, url, descriptionsFolder);
			if (description.isEmpty()) {
				//qDebug() << QString("description not found:%1 %2").arg(prefix).arg(url);
			}
		}
		
		fzp += QString("<author>%1</author>\n").arg(TextUtils::escapeAnd(author));
		fzp += QString("<description>%1</description>\n").arg(TextUtils::escapeAnd(description));
		fzp += QString("<title>%1</title>\n").arg(TextUtils::escapeAnd(title));
		if (!url.isEmpty()) {
			fzp += QString("<url>%1</url>\n").arg(TextUtils::escapeAnd(url));
		}
		fzp += QString("<tags>\n");    
		foreach (QString tag, tags) {
			fzp += QString("<tag>%1</tag>\n").arg(TextUtils::escapeAnd(tag));
		}
		fzp += QString("</tags>\n");
		fzp += QString("<properties>\n");
		foreach (QString prop, properties.keys()) {
			fzp += QString("<property name='%1'>%2</property>\n")
				.arg(TextUtils::escapeAnd(prop))
				.arg(TextUtils::escapeAnd(properties.value(prop)));
		}
		fzp += QString("</properties>\n");
	}

	fzp += QString("<views>\n");

    fzp += QString("<breadboardView>\n");
    fzp += QString("<layers image='breadboard/%1_breadboard.svg'>\n").arg(prefix);
    fzp += QString("<layer layerId='breadboard'/>\n");
    fzp += QString("</layers>\n");
    fzp += QString("</breadboardView>\n");

    fzp += QString("<schematicView>\n");
    fzp += QString("<layers image='schematic/%1_schematic.svg'>\n").arg(prefix);
    fzp += QString("<layer layerId='schematic'/>\n");
    fzp += QString("</layers>\n");
    fzp += QString("</schematicView>\n");

    fzp += QString("<pcbView>\n");
    fzp += QString("<layers image='pcb/%1_pcb.svg'>\n").arg(prefix);
    fzp += QString("<layer layerId='copper1'/>\n");
    fzp += QString("<layer layerId='silkscreen'/>\n");
	if (!m_genericSMD) {
		fzp += QString("<layer layerId='copper0'/>\n");
	}
    fzp += QString("</layers>\n");
    fzp += QString("</pcbView>\n");

    fzp += QString("<iconView>\n");
    fzp += QString("<layers image='breadboard/%1_breadboard.svg'>\n").arg(prefix);
    fzp += QString("<layer layerId='icon'/>\n");
    fzp += QString("</layers>\n");
    fzp += QString("</iconView>\n");

    fzp += QString("</views>\n");
    fzp += QString("<connectors>\n");

	QList<QDomElement> contacts;
	QStringList busNames;
	collectContacts(root, paramsRoot, contacts, busNames);
	foreach (QDomElement contact, contacts) {
		if (!isUsed(contact)) continue;

		bool hybrid = (contact.tagName().compare("via") == 0);

		QString index = contact.attribute("connectorIndex");
		QString connectorName = getConnectorName(contact);

		QString ct = connectorType;
		if (contact.tagName().compare("via") == 0) {
			ct = "female";
		}
		if (genders.count() > 0) {
			QString packageName = contact.parentNode().parentNode().toElement().attribute("name");
			QString elementName = contact.parentNode().parentNode().parentNode().toElement().attribute("name");
			foreach (QStringList strings, genders) {
				if (strings.at(0).compare(elementName) == 0 && strings.at(1).compare(packageName) == 0) {
					ct = strings.at(2);
					break;
				}
			}
		}

		fzp += QString("<connector id='connector%1' type='%3' name='%2'>\n").arg(index).arg(connectorName).arg(ct);
        fzp += QString("<description>%1</description>").arg(connectorName);
        fzp += QString("<views>\n");
        fzp += QString("<breadboardView>\n");
        fzp += QString("<p layer='breadboard' svgId='connector%1pin'/>").arg(index);
        fzp += QString("</breadboardView>\n");
        fzp += QString("<schematicView>\n");
        fzp += QString("<p layer='schematic' svgId='connector%1pin' terminalId='connector%1terminal'%2/>").arg(index).arg(hybrid ? HybridString : "");
        fzp += QString("</schematicView>\n");
        fzp += QString("<pcbView>\n");
		if (!m_genericSMD) {
			fzp += QString("<p layer='copper0' svgId='connector%1pad'%2/>").arg(index).arg(hybrid ? HybridString : "");
		}
        fzp += QString("<p layer='copper1' svgId='connector%1pad'%2/>").arg(index).arg(hybrid ? HybridString : "");
        fzp += QString("</pcbView>\n");
        fzp += QString("</views>\n");
        fzp += QString("</connector>\n");
	}

    fzp += QString("</connectors>\n");

    fzp += QString("<buses>\n");

	QList<int> gotBuses;
	QDomElement buses = paramsRoot.firstChildElement("buses");
	foreach (QString busName, busNames) {
		fzp += QString("<bus id='%1'>\n").arg(busName);
		QStringList members;
		foreach (QDomElement contact, contacts) {
			if (!isUsed(contact)) continue;

			if (busName.compare(contact.attribute("signal"), Qt::CaseInsensitive) == 0) {
				QString connectorIndex = contact.attribute("connectorIndex");
				if (!members.contains(connectorIndex)) {
					fzp += QString("<nodeMember connectorId='connector%1'/>\n").arg(connectorIndex);
					members.append(connectorIndex);
					//qDebug() << "signal match" << busName << connectorIndex;
				}
			}
		}

		// merge any buses in params.xml
		QDomElement bus = buses.firstChildElement("bus");
		int busIndex = 0;
		while (!bus.isNull()) {
			QDomElement element = bus.firstChildElement();
			bool addThese = false;
			while (!element.isNull()) {
				if (element.attribute("signal").compare(busName, Qt::CaseInsensitive) == 0) {
					addThese = true;
					break;
				}
				element = element.nextSiblingElement();
			}
			if (addThese) {
				gotBuses.append(busIndex);
				element = bus.firstChildElement();
				while (!element.isNull()) {
					QString connectorIndex;
					foreach (QDomElement contact, contacts) {
						if (matchAnd(contact, element)) {
							QString connectorIndex = contact.attribute("connectorIndex");
							if (!members.contains(connectorIndex)) {
								fzp += QString("<nodeMember connectorId='connector%1'/>\n").arg(connectorIndex);
								members.append(connectorIndex);
								//qDebug() << "bus match and" << busName << element.attribute("element") << element.attribute("package") << contact.attribute("name") << contact.attribute("signal") << connectorIndex;
							}
						}
					}

					element = element.nextSiblingElement();
				}
			}

			bus = bus.nextSiblingElement("bus");
			busIndex++;
		}


		fzp += QString("</bus>\n");
	}


	QDomElement bus = buses.firstChildElement("bus");
	int busIndex = 0;
	while (!bus.isNull()) {
		if (!gotBuses.contains(busIndex)) {
			fzp += QString("<bus id='bus_%1'>\n").arg(busIndex);
			QStringList members;
			QDomElement element = bus.firstChildElement();
			while (!element.isNull()) {
				foreach (QDomElement contact, contacts) {
					if (matchAnd(contact, element)) {
						QString connectorIndex = contact.attribute("connectorIndex");
						if (!members.contains(connectorIndex)) {
							fzp += QString("<nodeMember connectorId='connector%1'/>\n").arg(connectorIndex);
							members.append(connectorIndex);
							//qDebug() << "match and" << element.attribute("element") << element.attribute("package") << contact.attribute("name") << contact.attribute("signal") << connectorIndex;
						}
					}					
				}

				element = element.nextSiblingElement();
			}
			fzp += QString("</bus>\n");
		}
		bus = bus.nextSiblingElement("bus");
		busIndex++;
	}

    fzp += QString("</buses>\n");

	fzp += QString("</module>\n");

	return fzp;
}

void BrdApplication::replaceXY(QString & string) {
	string.replace("cx=''", QString("cx='%1'").arg(m_cxLast - m_trueBounds.left()));
	string.replace("cy=''", QString("cy='%1'").arg(flipy(m_cyLast)));
}

QString BrdApplication::genPCB(QDomElement & root, QDomElement & paramsRoot) {

	QString svg = TextUtils::makeSVGHeader(1000, 1000, m_trueBounds.width(), m_trueBounds.height());
	svg += "<desc>Fritzing footprint generated by brd2svg</desc>\n";
	svg += "<g id='silkscreen'>\n";
	svg += genMaxShape(root, paramsRoot, "none", "white", 8);
	QDomElement skipParams;
	qreal minArea = qMin(m_trueBounds.width() * m_trueBounds.height() / 16, MinPCBPackageArea);
	genLayerElements(root, skipParams, svg, TopPlaceLayer, true, minArea, true, "#ffffff");   
	QStringList noICs;
    QHash<QString, QString> noSubparts;
	FillStroke fs;
	fs.fill = "none";
	fs.fillOpacity = 1.0;
	fs.stroke = "white";
	fs.strokeWidth = 8;
    genOverlaps(root, fs, fs, svg, true, noICs, noSubparts, false);
	svg += "</g>\n";
	QString c1, c0;
	genCopperElements(root, paramsRoot, c1, TopLayer, "#F7BD13", "pad", false);   
	if (!m_genericSMD) {
		genCopperElements(root, paramsRoot, c0, BottomLayer, "#F7BD13", "pad", false);
	}
	if (!c1.isEmpty() && !c0.isEmpty()) {
		svg += "<g id='copper1'>\n";
		// TODO: this could be a much more sophisticated comparison
		if (c1.compare(c0) == 0) {
			svg += "<g id='copper0'>\n";
			replaceXY(c1);
			svg += c1;
			svg += "</g>\n";
		}
		else {
			replaceXY(c1);
			replaceXY(c0);
			svg += c1;
			svg += "</g>\n";
			svg += "<g id='copper0'>\n";
			svg += c0;
		}
	}
	else if (!c1.isEmpty()) {
		svg += "<g id='copper1'>\n";
		replaceXY(c1);
		svg += c1;
	}
	else if (!c0.isEmpty()) {
		svg += "<g id='copper0'>\n";
		replaceXY(c0);
		svg += c0;
	}

	svg += "</g>\n";
	svg += "</svg>\n";
	return svg;
}

bool viasFirst(QDomElement & contact1, QDomElement & contact2)
{
	if (contact1.tagName().compare("via") == 0) return true;
	if (contact2.tagName().compare("via") == 0) return false;
	return true;
}


QString BrdApplication::genGenericBreadboard(QDomElement & root, QDomElement & paramsRoot, DifParam * difParam, QDir & workingFolder) 
{
	QString boardColor = "#1F7A34";

	if (!paramsRoot.isNull()) {
		QDomElement bb = paramsRoot.firstChildElement("breadboard");
		QString bc = bb.attribute("breadboard-color");
		if (!bc.isEmpty()) boardColor = translateBoardColor(bc);
	}

	if (difParam && !difParam->boardColor.isEmpty()) {
		boardColor = translateBoardColor(difParam->boardColor);
	}

	QList<QDomElement> powers;
	QList<QDomElement> grounds;
	QList<QDomElement> lefts;
	QList<QDomElement> rights;
	QList<QDomElement> unused;	
	QList<QDomElement> vias;	
	QStringList busNames;
	getSides(root, paramsRoot, powers, grounds, lefts, rights, unused, vias, busNames, false, true);
	powers.append(lefts);
	powers.append(rights);
	powers.append(grounds);

	QString copper;
	genCopperElements(root, paramsRoot, copper, TopLayer, "#8c8c8c", "pad", true);   
    QRectF outerChipRect;
    getPackagesBounds(root, outerChipRect, TopLayer, true, false);  

	QRectF innerChipRect;
	getPackagesBounds(root, innerChipRect, TopPlaceLayer, true, false);  

    qSort(powers.begin(), powers.end(), numericByIndex);
	return MiscUtils::makeGeneric(workingFolder, boardColor, powers, copper, getBoardName(root), innerChipRect.size(), innerChipRect.size(), getConnectorName, getConnectorIndex, false);
}

QString BrdApplication::genBreadboard(QDomElement & root, QDomElement & paramsRoot, DifParam * difParam, const QStringList & ICs, QHash<QString, QString> & subpartAliases) 
{
	QString svg = TextUtils::makeSVGHeader(1000, 1000, m_trueBounds.width(), m_trueBounds.height());
	svg += "<desc>Fritzing breadboard generated by brd2svg</desc>\n";
	svg += "<g id='breadboard'>\n";
	svg += "<g id='icon'>\n";						// make sure we can use this image for icon view

	QString boardColor = "#1F7A34";

	if (!paramsRoot.isNull()) {
		QDomElement bb = paramsRoot.firstChildElement("breadboard");
		QString bc = bb.attribute("breadboard-color");
		if (!bc.isEmpty()) boardColor = translateBoardColor(bc);
	}

	QString textColor = "#ffffff";

	if (difParam && !difParam->boardColor.isEmpty()) {
		if (difParam->boardColor.compare("white", Qt::CaseInsensitive) == 0) {
			textColor = "#000000";
		}
		boardColor = translateBoardColor(difParam->boardColor);
	}

	svg += genMaxShape(root, paramsRoot, boardColor, "none", 0);

	if (!paramsRoot.isNull()) {
		QDomElement bb = paramsRoot.firstChildElement("breadboard");
		QDomElement extraLayers = bb.firstChildElement("extra-layers");
		QDomElement layer = extraLayers.firstChildElement("layer");
		while (!layer.isNull()) {
			svg += QString("<g><title>layer %1</title>\n").arg(layer.attribute("number"));
			QString layerID = layer.attribute("number");
			bool doFillings = (layerID.compare(TopPlaceLayer) == 0);
			genLayerElements(root, paramsRoot, svg, layerID, false, 0, doFillings, textColor);
			layer = layer.nextSiblingElement("layer");
			svg += QString("</g>\n");
		}
	}

	FillStroke fs1;
	fs1.fill = "#ffffff";
	fs1.fillOpacity = 0.15;
	fs1.stroke = "none";
	fs1.strokeWidth = 0;
	FillStroke fs2;
	fs2.fill = "#333333";
	fs2.fillOpacity = 1.0;
	fs2.stroke = "none";
	fs2.strokeWidth = 0;

	genOverlaps(root, fs1, fs2, svg, false, ICs, subpartAliases, true);
	genCopperElements(root, paramsRoot, svg, TopLayer, "#9A916C", "pin", true);

	svg += "</g>\n";
	svg += "</g>\n";
	svg += "</svg>\n";

	addSubparts(root, paramsRoot, svg, subpartAliases);
	return svg;
}


void BrdApplication::addSubparts(QDomElement & root, QDomElement & paramsRoot, QString & svg, QHash<QString, QString> & subpartAliases)
{
	QDir subpartsFolder(m_fritzingSubpartsPath);
	subpartsFolder.cd("breadboard");
	if (!subpartsFolder.exists()) {
		qDebug() << "subparts folder" << subpartsFolder.absolutePath() << "not found";
		return;
	}

	bool gotPackage = false;
	QList<QDomElement> packages;
	collectPackages(root, packages);
	foreach (QDomElement package, packages) {
        QString name = package.attribute("name", "");
        QString sname = findSubpart(name, subpartAliases, subpartsFolder);
		if (!sname.isEmpty()) {
            qDebug() << "\tfound subpart (1)" << name << sname;
			gotPackage = true;
			break;
		}
	}

	QDomElement bb = paramsRoot.firstChildElement("breadboard");
	QDomElement nudges = bb.firstChildElement("nudges");
	QDomElement includes = bb.firstChildElement("includes");
	QDomElement include = includes.firstChildElement("include");
	bool gotIncludes = !include.isNull();

	if (!gotPackage && !gotIncludes) return;

	QDomDocument doc;
	TextUtils::mergeSvg(doc, svg, "breadboard");

	if (gotPackage) {
		foreach (QDomElement package, packages) {
			QString name = package.attribute("name", "").toLower();

			qreal offsetX = 0;
			qreal offsetY = 0;
			qreal nudgeAngle = 999999;
			bool show = true;
			QDomElement nudge = nudges.firstChildElement("nudge");
			while (!nudge.isNull()) {
				if (nudge.attribute("package").compare(name, Qt::CaseInsensitive) == 0) {
					QDomElement parent = package.parentNode().toElement();
					if (parent.attribute("name").compare(nudge.attribute("element"), Qt::CaseInsensitive) == 0) {
						offsetX = TextUtils::convertToInches(nudge.attribute("x", "0")) * 1000;
						offsetY = TextUtils::convertToInches(nudge.attribute("y", "0")) * 1000;
						if (!nudge.attribute("angle").isEmpty()) {
							nudgeAngle = nudge.attribute("angle").toDouble();
						}
						if (nudge.attribute("show").compare("no") == 0) show = false;
						break;
					}
				}

				nudge = nudge.nextSiblingElement("nudge");
			}

			if (!show) continue;

			//qDebug() << subpartsFolder.absoluteFilePath(name + ".svg");
			QString sname = findSubpart(name, subpartAliases, subpartsFolder);
			if (sname.isEmpty()) continue;

            qDebug() << "\tfound subpart (2)" << name << sname;

			qreal x1,y1,x2,y2;
			if (!MiscUtils::x1y1x2y2(package, x1, y1, x2, y2)) continue;

			SvgFileSplitter splitter;
            QFile file(subpartsFolder.absoluteFilePath(sname + ".svg"));
			if (!splitter.load(&file)) continue;

            double factor;
			splitter.normalize(1000, "", false, factor);

			qreal sWidth, sHeight, vbWidth, vbHeight;
			QDomDocument dd = splitter.domDocument();
			TextUtils::getSvgSizes(dd, sWidth, sHeight, vbWidth, vbHeight);

			bool ok = true;
			qreal angle = package.parentNode().toElement().attribute("angle").toDouble(&ok);
			if (nudgeAngle != 999999) {
				angle = nudgeAngle;
				ok = true;
			}

			qreal subx = (sWidth * 1000 / 2);
			qreal suby =  (sHeight * 1000 / 2);

			if (angle != 0 && ok) {
				QMatrix matrix;
				matrix.translate(subx, suby);
				matrix.rotate(angle);
				matrix.translate(-subx, -suby);  
				QHash<QString, QString> attributes;
				attributes.insert("transform", TextUtils::svgMatrix(matrix));
				splitter.gWrap(attributes);
			}

			QHash<QString, QString> attributes;
			attributes.insert("id", TextUtils::escapeAnd(name));
			attributes.insert("transform", QString("translate(%1,%2)")
				.arg(((x2 + x1) / 2) - m_trueBounds.left() - subx + offsetX)  
				.arg(flipy((y2 + y1) / 2)  - suby + offsetY) 
				);

			splitter.gWrap(attributes);
			TextUtils::mergeSvg(doc, splitter.toString(), "breadboard");
		}
	}

	while (gotIncludes && !include.isNull()) {
		QString name = include.attribute("src");
        QFileInfo info(name);
		qreal x = TextUtils::convertToInches(include.attribute("x"));
		qreal y = TextUtils::convertToInches(include.attribute("y"));
		includeSvg(doc, name, info.fileName(), 
			(x * 1000) - m_trueBounds.left() + m_boardBounds.left(), 
			(y * 1000) - m_trueBounds.top() + m_boardBounds.top());
		include = include.nextSiblingElement("include");
	}

	svg = TextUtils::mergeSvgFinish(doc);
}

void BrdApplication::includeSvg(QDomDocument & doc, const QString & path, const QString & name, qreal x, qreal y) {
	QFile file(path);
	if (!file.exists()) {
		qDebug() << "file '" << path << "' not found.";
		return;
	}

	SvgFileSplitter splitter;
	if (!splitter.load(&file)) return;

    double factor;
	splitter.normalize(1000, "", false, factor);
	QHash<QString, QString> attributes;
	attributes.insert("id", TextUtils::escapeAnd(name));
	attributes.insert("transform", QString("translate(%1,%2)").arg(x).arg(y));
	splitter.gWrap(attributes);
	TextUtils::mergeSvg(doc, splitter.toString(), "breadboard");
}

QString BrdApplication::genSchematic(QDomElement & root, QDomElement & paramsRoot, DifParam * difParam) 
{
	QList<QDomElement> powers;
	QList<QDomElement> grounds;
	QList<QDomElement> lefts;
	QList<QDomElement> rights;
	QList<QDomElement> unused;
	QList<QDomElement> vias;

	// TODO: always leaves room on the left and right even if there are no connectors

	QStringList busNames;
	getSides(root, paramsRoot, powers, grounds, lefts, rights, unused, vias, busNames, true, false);

	QString boardName = getBoardName(root);
	bool usingParam = false;
	if (difParam && !difParam->title.isEmpty()) {
		boardName = difParam->title;
		usingParam = true;
	}
    else {
        QDomElement titleElement = paramsRoot.firstChildElement("title");
	    QString name;
	    TextUtils::findText(titleElement, name);
        if (!name.isEmpty()) boardName = name;
    }

    return SchematicRectConstants::genSchematicDIP(powers, grounds, lefts, rights, vias, busNames, boardName, usingParam, m_genericSMD, getConnectorName);
}

QString BrdApplication::getBoardName(QDomElement & root) 
{
	QDomElement title = root.firstChildElement("title");
	QString boardName;
	TextUtils::findText(title, boardName);
	QFileInfo fileInfo(boardName);
	return fileInfo.completeBaseName();
}

void BrdApplication::getSides(QDomElement & root, QDomElement & paramsRoot, 
							QList<QDomElement> & powers, QList<QDomElement> & grounds, 
							QList<QDomElement> & lefts, QList<QDomElement> & rights, 
							QList<QDomElement> & unused, QList<QDomElement> & vias, 
							QStringList & busNames, bool collectSpaces, bool integrateVias) 
{
	if (!paramsRoot.isNull()) {
		QList<QDomElement> connectors;
		collectConnectors(paramsRoot, connectors, collectSpaces);
		QList<QDomElement> contacts;
		collectContacts(root, paramsRoot, contacts, busNames);

		for (int ix = 0; ix < contacts.count(); ix++) {
			QDomElement contact = contacts.at(ix);
			if (contact.tagName().compare("via") == 0) {
				vias.append(contact);
				contacts.removeAt(ix--);
			}
		}
		
		foreach (QDomElement connector, connectors) {
			if (connector.attribute("space", "0").compare("1") == 0) {
				QString parentName = connector.parentNode().toElement().tagName();
				if (parentName.compare("power") == 0) {
					powers.append(Spacer);
				}
				else if (parentName.compare("ground") == 0) {
					grounds.append(Spacer);
				}
				else if (parentName.compare("left") == 0) {
					lefts.append(Spacer);
				}
				else if (parentName.compare("right") == 0) {
					rights.append(Spacer);
				}
				continue;
			}

			for (int ix = 0; ix < contacts.count(); ix++) {
				QDomElement contact = contacts.at(ix);


				if (match(contact, connector, false)) {
					contacts.removeAt(ix--);
					QString parentName = connector.parentNode().toElement().tagName();
					if (parentName.compare("power") == 0) {
						powers.append(contact);
					}
					else if (parentName.compare("ground") == 0) {
						grounds.append(contact);
					}
					else if (parentName.compare("left") == 0) {
						lefts.append(contact);
					}
					else if (parentName.compare("right") == 0) {
						rights.append(contact);
					}
					else if (parentName.compare("unused") == 0) {
						unused.append(contact);
					}
					else {
						unused.append(contact);
					}
					break;
				}
			}
		}

		unused.append(contacts);

		if (integrateVias) {
			int viaCount = vias.count();
			for (int v = 0; v < viaCount; v++) {
				QDomElement contact = vias.takeFirst();
				//qDebug() << "via?" << contact.tagName();
				if (v < viaCount / 2) lefts.append(contact);
				else rights.append(contact);
			}
		}

		return;
	}

	QList<QDomElement> contacts;
	collectContacts(root, paramsRoot, contacts, busNames);

	if (m_genericSMD) {
		foreach (QDomElement contact, contacts) {
			if (contact.attribute("signal", "").isEmpty()) {
				contact.setAttribute("signal", contact.attribute("connectorIndex").toInt() + 1);
			}
		}
	}

	foreach (QDomElement contact, contacts) {
		if (!isUsed(contact)) {
			unused.append(contact);
			continue;
		}

		QDomElement pad = contact.firstChildElement("pad");
		if (pad.isNull()) {
			if (contact.tagName().compare("via") == 0) {
			}
			else {
				QDomElement smd = contact.firstChildElement("smd");
				if (smd.isNull()) continue;

				// TODO: check layer?
			}

		}
		else {
			QDomElement layer = pad.firstChildElement("layer");
			while (!layer.isNull()) {
				QString lid = layer.attribute("layer", "");
				if (lid.compare(PadsLayer) == 0) break;

				layer = layer.nextSiblingElement("layer");
			}
			if (layer.isNull()) continue;
		}

		QString signal = contact.attribute("signal", "");

		bool gotOne = false;
		foreach (QString groundName, GroundNames) {
			if (signal.compare(groundName, Qt::CaseInsensitive) == 0) {
				grounds.append(contact);
				gotOne = true;
				break;
			}
		}

		if (gotOne) continue;

		foreach (QString powerName, PowerNames) {
			if (signal.compare(powerName, Qt::CaseInsensitive) == 0) {
				powers.append(contact);
				gotOne = true;
				break;
			}
		}

		if (gotOne) continue;

		foreach (QString powerName, PowerNames) {
			if (signal.startsWith(powerName, Qt::CaseInsensitive)) {
				powers.append(contact);
				gotOne = true;
				break;
			}
		}

		if (gotOne) continue;

		if (signal.startsWith("v", Qt::CaseInsensitive)) {
			powers.append(contact);
			continue;
		}

		if (signal.startsWith("+", Qt::CaseInsensitive)) {
			powers.append(contact);
			continue;
		}

		if (signal.startsWith("-", Qt::CaseInsensitive)) {
			powers.append(contact);
			continue;
		}

		lefts.append(contact);
	}

	qSort(lefts.begin(), lefts.end(), alphaBySignal);

	int mid = lefts.count() / 2;
	for (int i = lefts.count() - 1; i >= mid; i--) {
		rights.append(lefts.takeLast());
	} 
}

void BrdApplication::genCopperElements(QDomElement &root, QDomElement & paramsRoot, QString & svg, const QString & layerID, const QString & copperColor, const QString & padString, bool integrateVias) {
	QList<QDomElement> contacts;
	QStringList busNames;
	collectContacts(root, paramsRoot, contacts, busNames);
	if (!integrateVias) {
		// do vias first, so they are hidden beneath other connectors
		// size must be non-zero or the connector is ignored rather than just treated as a hybrid
		qSort(contacts.begin(), contacts.end(), viasFirst);
	}

	foreach (QDomElement contact, contacts) {
		if (!isUsed(contact)) continue;

		genPad(contact, svg, layerID, copperColor, padString, integrateVias);
	}
}

void BrdApplication::collectContacts(QDomElement & root, QDomElement & paramsRoot, QList<QDomElement> & contactsList, QStringList & busNames) {
	QList<QDomElement> packages;
	collectPackages(root, packages);
	foreach(QDomElement package, packages) {
		QDomElement contacts = package.firstChildElement("contacts");
		QDomElement contact = contacts.firstChildElement("contact");
		while (!contact.isNull()) {
			//QString temp;
			//QTextStream textStream(&temp);
			//contact.save(textStream, 1);
			//qDebug() << package.parentNode().toElement().attribute("name") << package.attribute("name") << temp;

			int used = 0;
			bool append = false;
			QDomElement pad = contact.firstChildElement("pad");
			if (pad.isNull()) {
				QDomElement smd = contact.firstChildElement("smd");
				if (!smd.isNull()) {
					append = true;
					if (m_genericSMD) {
						if (smd.attribute("flags").toInt() == 7) {
							used = 1;
						}
						else {
							used = 0;
							append = false;
						}
					}
				}
			}
			else {
				append = true;
				QString elementName = package.parentNode().toElement().attribute("name");
				foreach (QString externalElementName, ExternalElementNames) {
					if (elementName.startsWith(externalElementName, Qt::CaseInsensitive)) {
						used = 1;
						break;
					}
				}
			}
			if (append) {
				if (contact.attribute("connectorIndex", "").isEmpty()) {
					contact.setAttribute("connectorIndex", contactsList.length());
				}
				contact.setAttribute("used", used);
				contactsList.append(contact);
				if (m_genericSMD && used != 0 && contact.attribute("signal").isEmpty()) {
					contact.setAttribute("signal", contactsList.length());  // just got incremented
				}
			}

			contact = contact.nextSiblingElement("contact");
		}
	}

	if (!paramsRoot.isNull()) {
		foreach (QDomElement contact, contactsList) {
			contact.setAttribute("used", 0);
		}

		QList<QDomElement> connectors;
		collectConnectors(paramsRoot, connectors, false);	

		foreach (QDomElement connector, connectors) {
			QString connectorSignal = connector.attribute("signal");
			if (connectorSignal.isEmpty() && m_genericSMD) {
				QString connectorName = connector.attribute("name");
				connectorSignal = connectorName;
				connector.setAttribute("signal", connectorName);
			}

			QString connectorElement = connector.attribute("element");
			foreach (QDomElement contact, contactsList) {
				if (match(contact, connector, false)) {
					//QString id = connector.attribute("id");
					//contact.setAttribute("connectorIndex", id);
					int used = connector.attribute("used").toInt();
					contact.setAttribute("used", used); 
					break;
				}
			}
		}

		if (paramsRoot.attribute("include-vias").compare("true") == 0) {
			QDomElement _signals = root.firstChildElement("signals");
			QDomElement signal = _signals.firstChildElement("signal");
			while (!signal.isNull()) {
				QDomElement via = signal.firstChildElement("via");
				while (!via.isNull()) {
					bool ok;
					qreal drill = MiscUtils::strToMil(via.attribute("drill", ""), ok);
					if (ok && drill > 30) {
						via.setAttribute("used", 1);
						via.setAttribute("signal", signal.attribute("name"));
											
						if (via.attribute("connectorIndex", "").isEmpty()) {
							via.setAttribute("connectorIndex", contactsList.length());
						}
						//qDebug() << "via signal" << via.attribute("signal");
						contactsList.append(via);
					}
					else {
						// ignore tiny vias
						via.setAttribute("used", 0);
					}
					via = via.nextSiblingElement("via");
				}

				signal = signal.nextSiblingElement("signal");
			}


			QList<QDomElement> fakeVias;
			collectFakeVias(paramsRoot, fakeVias);
			if (fakeVias.count()) {
				// assumes items on the <fake-vias> list are not also found in the <connectors> list in params.xml
				foreach(QDomElement package, packages) {
					QDomElement contacts = package.firstChildElement("contacts");
					QDomElement contact = contacts.firstChildElement("contact");
					while (!contact.isNull()) {
						foreach (QDomElement fakeVia, fakeVias) {
							if (match(contact, fakeVia, false)) {
								QDomElement via = contact.firstChildElement();
								via.setTagName("via");
								QString signalName = contact.attribute("signal");
								via.setAttribute("signal", signalName.isEmpty() ? contact.attribute("name") : signalName);
								if (via.attribute("connectorIndex", "").isEmpty()) {
									via.setAttribute("connectorIndex", contactsList.length());
								}
								via.setAttribute("used", 1);
								contactsList.append(via);
							}
						}

						contact = contact.nextSiblingElement("contact");
					}
				}
			}
		}
	}

	QHash<QString, int> nameCounts;
	foreach (QDomElement contact, contactsList) {
		if (!isUsed(contact)) continue;
		if (contact.tagName().compare("via") == 0) continue;

		QString busName = contact.attribute("signal").toLower();
		if (busName.isEmpty()) continue;

		int count = nameCounts.value(busName, 0);
		nameCounts.insert(busName, count + 1);
		if (count > 0) {
			contact.setAttribute("bus", 1);
			if (count == 1) busNames.append(busName);
		}
	}
}

void BrdApplication::collectPackages(QDomElement &root, QList<QDomElement> & packages)
{
	QDomElement elements = root.firstChildElement("elements");
	if (!elements.isNull()) {
		QDomElement element = elements.firstChildElement("element");
		while (!element.isNull()) {
			QDomElement package = element.firstChildElement("package");
			if (!package.isNull()) {
				//QString name = package.attribute("name");
				//qDebug() << name;
				if (inBounds(package)) {
					packages.append(package);
				}
			}
			element = element.nextSiblingElement("element");
		}
	}
}

void BrdApplication::collectConnectors(QDomElement &paramsRoot, QList<QDomElement> & connectorList, bool collectSpaces)
{
	QDomElement connectors = paramsRoot.firstChildElement("connectors");
	QDomElement lrtbu = connectors.firstChildElement();
	while (!lrtbu.isNull()) {
		QDomElement connector = lrtbu.firstChildElement();
		while (!connector.isNull()) {
			QString tagName = connector.tagName();
			if (tagName.compare("space") == 0) {
				if (collectSpaces) {
					connector.setAttribute("space", 1);
					connectorList.append(connector);
				}
			}
			else if (tagName.compare("connector") == 0) {
				connector.setAttribute("used", (lrtbu.tagName().compare("unused") == 0) ? 0 : 1);
				connectorList.append(connector);
			}
			connector = connector.nextSiblingElement();
		}
		lrtbu = lrtbu.nextSiblingElement();
	}
}

void BrdApplication::collectFakeVias(QDomElement &paramsRoot, QList<QDomElement> & connectorList)
{
	QDomElement fakeVias = paramsRoot.firstChildElement("fake-vias");
	QDomElement connector = fakeVias.firstChildElement();
	while (!connector.isNull()) {
		connectorList.append(connector);
		connector = connector.nextSiblingElement();
	}
}

void BrdApplication::genSmd(QDomElement & contact, QString & svg, const QString & layerID, const QString & copperColor, const QString & padString) 
{
	QDomElement smd = contact.firstChildElement("smd");
	if (smd.isNull()) {
		return; 
	}

	bool ok = true;
	qreal cx = MiscUtils::strToMil(smd.attribute("x", ""), ok);
	if (!ok) return;

	qreal cy =  MiscUtils::strToMil(smd.attribute("y", ""), ok);
	if (!ok) return;

	qreal w = MiscUtils::strToMil(smd.attribute("dx", ""), ok);
	if (!ok) return;

	qreal h = MiscUtils::strToMil(smd.attribute("dy", ""), ok);
	if (!ok) return;

	qreal angle = smd.attribute("angle", "").toDouble(&ok);
	if (!ok) return;

	qreal roundness = smd.attribute("roundness", "").toDouble(&ok);
	if (!ok) return;

	qreal subx = cx - m_trueBounds.left();
	qreal suby = flipy(cy);
	if (angle != 0) {
		QMatrix matrix;
		matrix.translate(subx, suby);
		matrix.rotate(angle);
		matrix.translate(-subx, -suby); 
		svg += QString("<g transform='%1' >\n").arg(TextUtils::svgMatrix(matrix));
	}

	qreal rx = qMin(w, h) * roundness / 200;			// the 200 is a magic figure that comes from eagle2svg.ulp
	qreal ry = rx;										// want rounded rect, not full ellipse
	QString rstring = (rx == 0 && ry == 0) ? "" : QString(" rx='%1' ry='%2' ").arg(rx).arg(ry);

	svg += QString("<rect fill='%5' stroke='none' x='%1' y='%2' width='%3' height='%4' stroke-width='0' id='connector%6%7' %8/>\n")
							.arg(cx - (w / 2) - m_trueBounds.left())
							.arg(flipy(cy + (h / 2)))			
							.arg(w)
							.arg(h)
							.arg(copperColor)
							.arg(contact.attribute("connectorIndex"))
							.arg(padString)
							.arg(rstring);

	if (angle != 0) {
		svg += "</g>\n";
	}
}

void BrdApplication::genPad(QDomElement & contact, QString & svg, const QString & layerID, const QString & copperColor, const QString & padString, bool integrateVias) 
{

	QDomElement pad = contact.firstChildElement("pad");
	if (!pad.isNull()) {
		genPadAux(contact, pad, svg, layerID, copperColor, padString, true);
		return;
	}

	if (contact.tagName().compare("via") == 0) {
		genPadAux(contact, contact, svg, TopLayer, copperColor, padString, integrateVias);
		return;
	}

	genSmd(contact, svg, layerID, copperColor, padString);
}

void BrdApplication::genPadAux(QDomElement & contact, QDomElement & pad, QString & svg, const QString & layerID, const QString & copperColor, const QString & padString, bool integrateVias) 
{
	QDomElement layer = pad.firstChildElement("layer");
	while (!layer.isNull()) {
		QString lid = layer.attribute("layer", "");
		if (lid.compare(layerID) == 0) {
			bool ok = true;
			qreal cx = MiscUtils::strToMil(pad.attribute("x", ""), ok);
			if (!ok) return;

			qreal cy = MiscUtils::strToMil(pad.attribute("y", ""), ok);
			if (!ok) return;

			qreal drill = MiscUtils::strToMil(pad.attribute("drill", ""), ok);
			if (!ok) return;

			qreal diameter = MiscUtils::strToMil(layer.attribute("diameter", ""), ok);
			if (!ok) return;

			int elongation = layer.attribute("elongation", "0").toInt();

			QString shape = layer.attribute("shape", "");
			if (shape.isEmpty()) return;

			if (m_shrinkHolesFactor != 1) {
				qreal w = diameter - drill;
				drill *= m_shrinkHolesFactor;
				diameter = drill + w;
			}

			QString cxString, cyString;
			if (!integrateVias) {
				drill = 0;
				diameter = 0;
			}
			else {
				m_cxLast = cx;
				m_cyLast = cy;
				cxString = QString::number(cx - m_trueBounds.left());
				cyString = QString::number(flipy(cy));
			}

			qreal width = (diameter - drill) / 2;				// division by 2 doesn't make sense, but it's the closest I've gotten to the right size
			qreal dr = (diameter / 2) - (width / 2);
			// for the moment only draw circular pads

			svg += QString("<circle fill='none' cx='%1' cy='%2' r='%3' stroke='%8' stroke-width='%4' id='connector%5%6' connectorname='%7'/>\n")
				.arg(cxString)
				.arg(cyString)
				.arg(dr)
				.arg(SW(width))
				.arg(contact.attribute("connectorIndex"))
				.arg(padString)
				.arg(getConnectorName(pad))
				.arg(copperColor);

			if (shape == "square") {				
				svg += QString("<rect fill='none' stroke='%6' x='%1' y='%2' width='%3' height='%4' stroke-width='%5' />\n")
							.arg(cx - dr - m_trueBounds.left())
							.arg(flipy(cy + dr))			// note "+ dr" rather than "- dr" because we flip all the y values
							.arg(dr * 2)
							.arg(dr * 2)
							.arg(SW(width))
							.arg(copperColor);
			}
			else if (shape == "long") {
				qreal angle = pad.attribute("angle", "0").toDouble();
				qreal rx = dr * 2.3;
				qreal ry = dr;
				if (angle == 90 || angle == 270) {
					rx = dr;
					ry = dr * 2.3;
				}
				QString hole = genHole2(cx, cy, dr, 0);

				svg += QString("<path stroke='none' stroke-width='0' d='m%1,%2 %3,0 0,%4 -%3,0 0,-%4%6' fill='%5' />\n")
							.arg(cx - rx - m_trueBounds.left())
							.arg(flipy(cy + ry))			// note "+ dr" rather than "- dr" because we flip all the y values
							.arg(rx * 2)
							.arg(ry * 2)
							.arg(copperColor)
							.arg(hole)
							;
			}
			else if (shape == "offset") {
				QString color = copperColor;
				QRectF rect(cx - dr, cy + dr, dr * 2, dr * 2);  // note "+ dr" rather than "- dr" because we flip all the y values
				int angle = pad.attribute("angle", "0").toDouble();
				if (angle == 0) {
					rect.setWidth(rect.width() + dr);
				}
				else if (angle == 270) {
					rect.setHeight(rect.height() + dr);
				}
				else if (angle == 180) {
					rect.setLeft(rect.left() - dr);
				}
				else if (angle == 90) {
					// weirdness because y gets flipped
					rect.setTop(rect.top() + dr);
					rect.setHeight(dr * 3);
				}
				else {
					qDebug() << "bad angle in genpadaux" << angle;
					return;
				}
				
				QString hole = genHole2(cx, cy, dr, 0);
				svg += QString("<path stroke='none' stroke-width='0' d='m%1,%2 %3,0 0,%4 -%3,0 0,-%4%6z' fill='%5' />\n")
							.arg(rect.left() - m_trueBounds.left())
							.arg(flipy(rect.top()))			// note "+ dr" rather than "- dr" because we flip all the y values
							.arg(rect.width())
							.arg(rect.height())
							.arg(color)
							.arg(hole)
							;
			}

			return;
		}
		layer = layer.nextSiblingElement("layer");
	}
}

void BrdApplication::genLayerElements(QDomElement &root, QDomElement &paramsRoot, QString & svg, const QString & layerID, bool skipText, qreal minArea, bool doFillings, const QString & textColor) {
	QList<QDomElement> from;
	from.append(root.firstChildElement("wires"));
	from.append(root.firstChildElement("circles"));
	from.append(root.firstChildElement("polygons"));
	from.append(root.firstChildElement("rects"));
	from.append(root.firstChildElement("texts"));

	QList<QDomElement> to;
	collectLayerElements(from, to, layerID);

	addElements(root, to, minArea);

	foreach (QDomElement element,  to) {
		genLayerElement(paramsRoot, element, svg, layerID, skipText, minArea, doFillings, textColor);
	}
}

void BrdApplication::addElements(QDomElement & root, QList<QDomElement> & to, qreal minArea) {
	QDomElement elements = root.firstChildElement("elements");
	if (!elements.isNull()) {
		QDomElement element = elements.firstChildElement("element");
		while (!element.isNull()) {
			QDomElement package = element.firstChildElement("package");
			if (!package.isNull()) {
				if (inBounds(package) && bigEnough(package, minArea)) {
					to.append(element);
				}
			}
			else {
				qDebug() << "element without package" << element.attribute("name", "");
			}
			element = element.nextSiblingElement("element");
		}
	}
}


void BrdApplication::genText(QDomElement & element, const QString & text, QString & svg, QDomElement & paramsRoot, const QString & textColor) 
{
	bool checkedWires = false;
	QDomElement wires = element.firstChildElement("wires");
	QDomElement wire = wires.firstChildElement("wire");
	while (!wire.isNull()) {
		// ensure text is on board
		qreal x1, y1, x2, y2;
		MiscUtils::x1y1x2y2(wire, x1, y1, x2, y2);
		if (x1 < m_boardBounds.left() || x1 > m_boardBounds.right()) return;
		if (y1 < m_boardBounds.top() || y1 > m_boardBounds.bottom()) return;
		if (x2 < m_boardBounds.left() || x2 > m_boardBounds.right()) return;
		if (y2 < m_boardBounds.top() || y2 > m_boardBounds.bottom()) return;

		checkedWires = true;
		wire = wire.nextSiblingElement("wire");
	}

	bool ok = true;
	qreal x = MiscUtils::strToMil(element.attribute("x", ""), ok);
	if (!ok) return;

	qreal y = MiscUtils::strToMil(element.attribute("y", ""), ok);
	if (!ok) return;

	qreal width = MiscUtils::strToMil(element.attribute("width", ""), ok);
	if (!ok) {
		qDebug() << "text width not provided for" << text;
		return;
	}

	qreal angle = element.attribute("angle", "").toDouble(&ok);
	if (!ok) return;

	int mirror = element.attribute("mirror", "").toInt(&ok);
	if (!ok) return;

	int spin = element.attribute("spin", "").toInt(&ok);
	if (!ok) return;

	qreal size = MiscUtils::strToMil(element.attribute("size", ""), ok);
	if (!ok) return;

	QString elementName;
	QDomElement bb = paramsRoot.firstChildElement("breadboard");
	QDomElement nudges = bb.firstChildElement("nudges");
	QDomElement nudge = nudges.firstChildElement("nudge");
	while (!nudge.isNull()) {
		if (nudge.attribute("text").compare(text) == 0) {
			if (elementName.isEmpty()) {
				QDomElement parent = element.parentNode().toElement();
				while (!parent.isNull()) {
					if (parent.tagName().compare("element") == 0) {
						elementName = parent.attribute("name");
						break;
					}
					parent = parent.parentNode().toElement();
				}
			}
			if (nudge.attribute("element").compare(elementName) == 0) { 
				bool doNudge = true;
				QDomElement match = nudge.firstChildElement("match");
				if (!match.isNull()) {
					// look for another way to match in case there are duplicates
					QDomNamedNodeMap matchAttributes = match.attributes();
					QDomNamedNodeMap elementAttributes = element.attributes();
					for (int i = 0; i < matchAttributes.count(); i++) {
						QDomNode matchNode = matchAttributes.item(i);
						bool gotOne = false;
						for (int j = 0; j < elementAttributes.count(); j++) {
							QDomNode elementNode = elementAttributes.item(j);
							if (elementNode.nodeName() == matchNode.nodeName() && elementNode.nodeValue() == matchNode.nodeValue()) {
								gotOne = true;
								break;
							}
						}
						if (!gotOne) {
							doNudge = false;
							break;
						}
					}
				}

				if (doNudge) {
					qreal temp = nudge.attribute("angle", "").toDouble(&ok);
					if (ok) angle = temp;
					double offsetX = TextUtils::convertToInches(nudge.attribute("x", "0")) * 1000;
					double offsetY = TextUtils::convertToInches(nudge.attribute("y", "0")) * 1000;
					x += offsetX;
					y += offsetY;
					break;
				}
			}
		}
		nudge = nudge.nextSiblingElement("nudge");
	}

	size *= 1.16;			// this is a hack, but it seems to help
	width *= ((width <= 7) ? 0.75 : 0.60);

	bool anchorAtStart;
	MiscUtils::calcTextAngle(angle, mirror, spin, size, x, y, anchorAtStart);
   
	if (!checkedWires) {
		QRectF r(x, y, width, size);
		if (!m_boardBounds.contains(r)) {
			//qDebug() << "clip text:" << text;
			return;
		}
	}

	if (angle != 0) {
		svg += QString("<g transform='translate(%1,%2)'><g transform='rotate(%3)'>\n")
			.arg(x - m_trueBounds.left())
			.arg(flipy(y))
			.arg(angle);
		x = m_trueBounds.left();
		y = m_trueBounds.bottom();
	}
	svg += QString("<text font-family='OCRA' stroke='none' stroke-width='%6' fill='%7' font-size='%1' x='%2' y='%3' text-anchor='%4'>%5</text>\n")
						.arg(size)
						.arg(x - m_trueBounds.left())
						.arg(flipy(y))
						.arg(anchorAtStart ? "start" : "end")
						.arg(TextUtils::escapeAnd(text))
						.arg(0)  // SW(width)
						.arg(textColor)
					;
	if (angle != 0) {
		svg += "</g></g>\n";
	}
}


void BrdApplication::genLayerElement(QDomElement & paramsRoot, QDomElement & element, QString & svg, const QString & layerID, bool skipText, qreal minArea, bool doFillings, const QString & textColor) 
{
	QString tagName = element.tagName();

	if (tagName.compare("circle") == 0) {
		genCircle(element, svg, false, "none", "white", 0);
		return;
	}

	if (tagName.compare("rect") == 0) {
		genRect(element, svg, false);
		return;
	}

	if (tagName.compare("wire") == 0) {
		QDomElement piece = element.firstChildElement("piece");
		while (!piece.isNull()) {
			QDomElement line = piece.firstChildElement("line");
			if (!line.isNull()) {
				genLine(line, svg);
			}
			else {
				QDomElement arc = piece.firstChildElement("arc");
				if (!arc.isNull()) {
					genArc(arc, svg);
				}
			}

			piece = piece.nextSiblingElement("piece");
		}
		return;
	}

	if (tagName.compare("polygon") == 0 ) {
		bool hackfill = (layerID.compare(TopLayer) == 0) || (layerID.compare(BottomPlaceLayer) == 0);
		genPath(element, svg, hackfill ? "white" : "none", hackfill ? "none" : "white", doFillings);
		return;
	}

	if (tagName.compare("text") == 0) {
		if (skipText) return;

		QString t;
		TextUtils::findText(element, t);
		svg += QString("<g><title>text:%1</title>\n").arg(TextUtils::escapeAnd(t));
		QDomElement child = element.firstChildElement("wires");
		if (!child.isNull() && false) {
			genPath(element, svg, "none", "white", doFillings);
		}
		else {
			genText(element, t, svg, paramsRoot, textColor);
		}
		svg += QString("</g>\n");
		
		return;
	}

	if (tagName.compare("element") == 0) {
		QString elementName = element.attribute("name", "");
		QDomElement package = element.firstChildElement("package");
		QString packageName = package.attribute("name", "");
		if (!package.isNull()) {		
			QDomElement bb = paramsRoot.firstChildElement("breadboard");
			QDomElement nudges = bb.firstChildElement("nudges");
			QDomElement nudge = nudges.firstChildElement("nudge");
			while (!nudge.isNull()) {
				if (nudge.attribute("package").compare(packageName, Qt::CaseInsensitive) == 0 && 
					nudge.attribute("element").compare(elementName, Qt::CaseInsensitive) == 0) 
				{
					if (nudge.attribute("lines").compare("no") == 0) {
						// don't draw the layer element
						return;
					}
					break;
				}
					
				nudge = nudge.nextSiblingElement("nudge");
			}
		}

		svg += QString("<g><title>element:%1</title>\n").arg(TextUtils::escapeAnd(elementName));
		QDomElement skipParams;
		genLayerElements(element, paramsRoot, svg, layerID, skipText, minArea, doFillings, textColor);
		if (!package.isNull()) {
			svg += QString("<g><title>package:%1</title>\n").arg(TextUtils::escapeAnd(packageName));
			genLayerElements(package, paramsRoot, svg, layerID, skipText, minArea, doFillings, textColor);
			svg += QString("</g>\n");
		}
		svg += QString("</g>\n");
		return;
	}

	qDebug() << "unknown layer element" << tagName;
}

void BrdApplication::genCircle(QDomElement & element, QString & svg, bool forDimension, const QString & fill, const QString & stroke, qreal strokeWidth) 
{
	qreal cx, cy, radius, width;
	if (!cxcyrw(element, cx, cy, radius, width)) return;

	if (strokeWidth <= 0) {
		strokeWidth = width;
	}

	qreal dr = (forDimension) ? strokeWidth / 2 : 0;

	svg += QString("<circle fill='%5' cx='%1' cy='%2' r='%3' stroke='%6' stroke-width='%4' />\n")
				.arg(cx - m_trueBounds.left())
				.arg(flipy(cy))
				.arg(radius - dr)
				.arg(SW(strokeWidth))
				.arg(fill)
				.arg(stroke);
}

void BrdApplication::genRect(QDomElement & element, QString & svg, bool forDimension) 
{
	// TODO: handle rotation
	qreal x1, y1, x2, y2;
	if (!MiscUtils::x1y1x2y2(element, x1, y1, x2, y2)) return;

	if (!forDimension) {
		if (!inBounds(x1, y1, x2, y2)) return;
	}

	bool ok = true;
	qreal width = 0;
	qreal temp = MiscUtils::strToMil(element.attribute("width", ""), ok);
	if (ok) width = temp;

	qreal dr = (forDimension) ? width / 2 : 0;
	QString stroke = "white";
	QString fill = "none";
	if (width == 0) {
		stroke = "none";
		fill = "white";
	}

	svg += QString("<rect x='%1' y='%2' width='%3' height='%4' stroke-width='%5' fill='%6' stroke='%7' />\n")
				.arg(x1 + dr - m_trueBounds.left())
				.arg(flipy(y1 + dr))
				.arg(x2 - x1 - dr - dr)
				.arg(y2 - y1 - dr - dr)
				.arg(SW(width))
				.arg(fill)
				.arg(stroke);
}

void BrdApplication::genLine(QDomElement & element, QString & svg) 
{
	qreal x1, y1, x2, y2;
	if (!MiscUtils::x1y1x2y2(element, x1, y1, x2, y2)) return;

	if (!inBounds(x1, y1, x2, y2)) return;

	bool ok = true;
	qreal width = MiscUtils::strToMil(element.attribute("width", ""), ok);
	if (!ok) return;

	int cap = element.attribute("cap", "1").toInt();

	svg += QString("<line stroke='white' x1='%1' y1='%2' x2='%3' y2='%4' stroke-width='%5' stroke-linecap='%6' />\n")
				.arg(x1 - m_trueBounds.left())
				.arg(flipy(y1))
				.arg(x2 - m_trueBounds.left())
				.arg(flipy(y2))
				.arg(SW(width))
				.arg((cap == 0) ? "butt" : "round");  
}


void BrdApplication::genArc(QDomElement & element, QString & svg) 
{
	qreal x1, y1, x2, y2;
	if (!MiscUtils::x1y1x2y2(element, x1, y1, x2, y2)) return;
	if (!inBounds(x1, y1, x2, y2)) return;

	qreal radius, width, angle1, angle2;
	if (!MiscUtils::rwaa(element, radius, width, angle1, angle2)) return;

	svg += QString("<path fill='none' stroke='white' d='M%1,%2 A%3,%3 0 %4 0 %5,%6' stroke-width='%7'/>\n")
				.arg(x1 - m_trueBounds.left())
				.arg(flipy(y1))
				.arg(radius)
				.arg((qAbs(angle2 - angle1) < 180.0) ? 0 : 1)
				.arg(x2 - m_trueBounds.left())
				.arg(flipy(y2))
				.arg(SW(width));
}

qreal BrdApplication::flipy(qreal y) {
	return m_trueBounds.height() - (y - m_trueBounds.top());
}

bool BrdApplication::inBounds(QDomElement & package) {
	qreal x1, y1, x2, y2;
	if (!MiscUtils::x1y1x2y2(package, x1, y1, x2, y2)) return false;

	return inBounds(x1, y1, x2, y2);
}

bool BrdApplication::bigEnough(QDomElement & package, qreal minArea) {
	if (minArea <= 0) return true;

	qreal x1, y1, x2, y2;
	if (!MiscUtils::x1y1x2y2(package, x1, y1, x2, y2)) return false;

	return qAbs(x2 - x1) * qAbs(y2 - y1) >= minArea;
}

bool BrdApplication::inBounds(qreal x1, qreal y1, qreal x2, qreal y2) {

	if (m_trueBounds.contains(QPointF(x1, y1))) {
		return true;
	}

	if (m_trueBounds.contains(QPointF(x2, y2))) {
		return true;
	}

	if (m_trueBounds.contains(QPointF(x1, y2))) {
		return true;
	}

	if (m_trueBounds.contains(QPointF(x2, y1))) {
		return true;
	}

	if (m_trueBounds.contains(QPointF((x2 + x1) / 2, (y2 + y1) / 2))) {
		return true;
	}

	return false;
}

void BrdApplication::genPath(QDomElement & element, QString & svg, const QString & fillArg, const QString & stroke, bool doFillings) {

	svg += QString("<g><title>polygon</title>\n");

	QList<QDomElement> wires;
	collectWires(element, wires, doFillings);

	QList<WireTree *> wireTrees;
	bool allConnected = MiscUtils::makeWireTrees(wires, wireTrees);
	QString path;
	qreal width;
	QString fill = fillArg;
	if (allConnected) {
		path = genPolyString(wireTrees, element, width);
	}
	else {
		path = genPolyString(wires, element, width);
	}
	foreach (WireTree * wireTree, wireTrees) delete wireTree;

	if (stroke.compare("none") == 0) {
		width = 0;
	}

	svg += QString("<path stroke-linecap='round' stroke-width='%1' fill='%2' stroke='%3' d='")
		.arg(SW(width))
		.arg(fill)
		.arg(stroke);
	svg += path;
	svg += "'/>\n";
	svg += QString("</g>\n");
}

QString BrdApplication::genPolyString(QList<WireTree *> & wireTrees, QDomElement & element, qreal & width) 
{
	bool ok;
	width = MiscUtils::strToMil(element.attribute("width", ""), ok);
	bool needsWidth = !ok;
	QString path;

	WireTree * current = NULL;
	WireTree * first = NULL;
	while (wireTrees.count() > 0) {
		if (current == NULL) {
			first = current = wireTrees.first();
			QPointF p(current->x1, current->y1);
			path += QString("M%1,%2").arg(p.x() - m_trueBounds.left()).arg(flipy(p.y()));
		}

		if (needsWidth) {
			width = MiscUtils::strToMil(current->width, ok);
			needsWidth = !ok;
		}

		qreal dr = width / 2;

		QPointF p(current->x2, current->y2);
		path += addPathUnit(current, p, dr);


		WireTree * next = current->right;
		if (next == first) break;

		if (next->right == current) {
			// TODO: not sure you can flip an arc this way
			next->turn();
			WireTree * t = next->right;
			next->right = next->left;
			next->left = t;
		}

		current = next;
	}

	return path;
}

QString BrdApplication::addPathUnit(WireTree * wireTree, QPointF p, qreal rDelta) 
{
	if (wireTree->curve == 0) {
		return QString("L%1,%2\n").arg(p.x() - m_trueBounds.left()).arg(flipy(p.y()));
	}

	return QString("A%1,%1 0 %2 %3 %4,%5\n")
						.arg(wireTree->radius - rDelta)
						.arg((qAbs(wireTree->angle2 - wireTree->angle1) < 180.0) ? 0 : 1)
						.arg(wireTree->sweep)
						.arg(p.x() - m_trueBounds.left())
						.arg(flipy(p.y())
					);
}

QString BrdApplication::genPolyString(QList<QDomElement> & wires, QDomElement & element, qreal & width) 
{
	bool ok;
	width = MiscUtils::strToMil(element.attribute("width", ""), ok);
	bool needsWidth = !ok;

	QString path;

	foreach (QDomElement wire, wires) {
		if (needsWidth) {
			width = MiscUtils::strToMil(wire.attribute("width", ""), ok);
			needsWidth = !ok;
		}
		QDomElement piece = wire.firstChildElement("piece");
		while (!piece.isNull()) {
			qreal x1, y1, x2, y2;
			QDomElement line = piece.firstChildElement("line");
			if (!line.isNull()) {
				if (MiscUtils::x1y1x2y2(line, x1, y1, x2, y2)) {
					path += QString("M%1,%2L%3,%4\n").arg(x1 - m_trueBounds.left()).arg(flipy(y1)).arg(x2 - m_trueBounds.left()).arg(flipy(y2));
				}
			}
			else {
				QDomElement arc = piece.firstChildElement("arc");
				if (!arc.isNull()) {
					qreal radius, width, angle1, angle2;
					if (MiscUtils::x1y1x2y2(line, x1, y1, x2, y2) && MiscUtils::rwaa(element, radius, width, angle1, angle2)) {
						path += QString("M%1,%2 A%3,%3 0 %4 0 %5,%6\n")
										.arg(x1 - m_trueBounds.left())
										.arg(flipy(y1))
										.arg(radius)
										.arg((qAbs(angle2 - angle1) < 180.0) ? 0 : 1)
										.arg(x2 - m_trueBounds.left())
										.arg(flipy(y2));
					}
				}				
			}

			piece = piece.nextSiblingElement("piece");
		}
	}

	return path;
}

void BrdApplication::collectWires(QDomElement & element, QList<QDomElement> & wiresList, bool useFillings) {
	QDomElement contours = element.firstChildElement("contours");
	if (contours.isNull()) {
		// wires from text
		QDomElement wires = element.firstChildElement("wires");
		QDomElement wire = wires.firstChildElement("wire");
		while (!wire.isNull()) {
			wiresList.append(wire);
			wire = wire.nextSiblingElement("wire");
		}
		return;
	}
		
	QDomElement wire = contours.firstChildElement("wire");
	while (!wire.isNull()) {
		wiresList.append(wire);
		wire = wire.nextSiblingElement("wire");
	}
	if (useFillings) {
		QDomElement fillings = element.firstChildElement("fillings");
		wire = fillings.firstChildElement("wire");
		while (!wire.isNull()) {
			wiresList.append(wire);
			wire = wire.nextSiblingElement("wire");
		}
	}
}

bool cxcyrw(QDomElement & element, qreal & cx, qreal & cy, qreal & radius, qreal & width)
{
	bool ok = true;
	radius = MiscUtils::strToMil(element.attribute("r", ""), ok);
	if (!ok) return false;

	width = MiscUtils::strToMil(element.attribute("width", ""), ok);
	if (!ok) return false;

	cx = MiscUtils::strToMil(element.attribute("cx", ""), ok);
	if (!ok) return false;

	cy = MiscUtils::strToMil(element.attribute("cy", ""), ok);
	return ok;
}

void BrdApplication::collectPadSmdPackages(QDomElement & root, QList<QDomElement> & padSmdPackages) {
	QList<QDomElement> packages;
	collectPackages(root, packages);
	foreach(QDomElement package, packages) {
		QString name = package.attribute("name");
		if (name.contains("DUEMILANOVE", Qt::CaseSensitive)) {
			//qDebug() << "skipping" << name;
			continue;
		}

		QDomElement contacts = package.firstChildElement("contacts");
		QDomElement contact = contacts.firstChildElement("contact");
		bool hasContact = false;
		while (!contact.isNull()) {
			if (!contact.firstChildElement("pad").isNull()) {
				hasContact = true;
				break;
			}
			else if (!contact.firstChildElement("smd").isNull()) {
				hasContact = true;
				break;
			}
			contact = contact.nextSiblingElement("contact");
		}
		if (hasContact) {
			padSmdPackages.append(package);
		}
	}
}


bool BrdApplication::polyFromWires(QDomElement & root, const QString & boardColor, const QString & stroke, qreal strokeWidth, QString & svg, bool & clockwise) {
	// note path is not closed here
	
	bool noStroke = stroke.compare("none") == 0;
	QList<QDomElement> wireList;
	QDomElement wires = root.firstChildElement("wires");
	QDomElement wire = wires.firstChildElement("wire");
	while (!wire.isNull()) {
		if (wire.attribute("layer").compare(DimensionsLayer) == 0) {
			wireList.append(wire);
		}
		wire = wire.nextSiblingElement("wire");
	}
	if (wireList.count() < 2) return false;

	QList<WireTree *> wireTrees;
	bool allConnected = MiscUtils::makeWireTrees(wireList, wireTrees);
	if (!allConnected) {
		foreach (WireTree * wireTree, wireTrees) delete wireTree;
		return false;
	}

	QString path = QString("<path fill='%1' stroke='%2' stroke-width='%3' d='").arg(boardColor).arg(stroke).arg(SW(strokeWidth));
	WireTree * first = wireTrees.first();
	WireTree * current = first;
	QMatrix matrix;
	qreal dr = 0;
	if (!noStroke) {
		matrix.translate(m_trueBounds.center().x(), m_trueBounds.center().y());
		matrix.scale((m_trueBounds.width() - strokeWidth) / m_trueBounds.width(), (m_trueBounds.height() - strokeWidth) / m_trueBounds.height());
		matrix.translate(-m_trueBounds.center().x(), -m_trueBounds.center().y());
		dr = strokeWidth / 2;
	}
	QPointF p(current->x1, current->y1);
	QPointF q = matrix.map(p);
	path += QString("M%1,%2").arg(q.x() - m_trueBounds.left()).arg(flipy(q.y()));
	bool firstTime = true;
	while (true) {
		QPointF p(current->x2, current->y2);
		QPointF q = matrix.map(p);
		path += addPathUnit(current, q, dr);

		WireTree * next = current->right;
		if (next == first) break;

		if (next->right == current) {
			next->turn();
			WireTree * t = next->right;
			next->right = next->left;
			next->left = t;
		}

		if (firstTime) {
			firstTime = false;
			// http://stackoverflow.com/questions/4437986/how-to-find-direction-of-a-vector-path
			// (b1-a1)(c2-b2) - (b2-a2)(c1-b1)
			// If this coordinate is positive then your path is counterclockwise, if it is negative then it is clockwise.
			qreal calc = ((current->x2 - current->x1) * (next->y2 - current->y2)) - ((current->y2 - current->y1) * (next->x2 - current->x2));
			clockwise = (calc < 0);
		}

		current = next;
	}
	path += "\n";    // do not close the path with a single-quote here, there is potentially more to be added
	svg += path;

	foreach (WireTree * wireTree, wireTrees) delete wireTree;
	return true;
}

QString BrdApplication::genMaxShape(QDomElement & root, QDomElement & paramsRoot, const QString & boardColor, const QString & stroke, qreal strokeWidth) 
{
	QString path;
	bool noStroke = stroke.compare("none") == 0;
	QString tagName = m_maxElement.tagName();
	bool gotMaxShape = false;
	bool clockwise = true;
	// eventually could do this with a vector-to-raster flood fill trick
	if (tagName.compare("circle") == 0) {
		clockwise = false;
		qreal cx, cy, radius, width;
		if (cxcyrw(m_maxElement, cx, cy, radius, width)) {
			if (strokeWidth <= 0) strokeWidth = width;

			if (!noStroke) {
				// center line would extend beyond border
				radius -= (strokeWidth / 2);
			}

			// counterclockwise
			path = QString("<path fill='%1' stroke='%2' stroke-width='%3' d='M%4,%5a%6,%6 0 1 0 %7,0 %6,%6 0 1 0 -%7,0z\n")
				.arg(boardColor)
				.arg(stroke)
				.arg(SW(strokeWidth))
				.arg(cx - m_trueBounds.left() - radius)
				.arg(flipy(cy))
				.arg(radius)
				.arg(2 * radius);

			//qDebug() << "max shape is circle";
			gotMaxShape = true;

		}
	}
	else if (tagName.compare("polygon") == 0) {
		//qDebug() << "max shape is polygon";
	}
	else if (tagName.compare("wire") == 0) {
		//qDebug() << "max shape is wires";
		gotMaxShape = polyFromWires(root, boardColor, stroke, strokeWidth, path, clockwise);
	}
	if (!gotMaxShape && !m_genericSMD) {
		clockwise = true;
		//qDebug() << "max shape is rect";
		qreal dr = noStroke ? 0 : strokeWidth / 2;
		path = QString("<path fill='%1' stroke='%2' stroke-width='%3' d='M%4,%5l%6,0 0,%7 -%6,0 0,-%7z\n")
				.arg(boardColor)
				.arg(stroke)
				.arg(SW(strokeWidth))
				.arg(m_boardBounds.left() - m_trueBounds.left() + dr)
				.arg(m_boardBounds.top() - m_trueBounds.top() + dr)
				.arg(m_boardBounds.width() - dr - dr)
				.arg(m_boardBounds.height() -dr - dr);
	}

	QDomNodeList nodeList = root.elementsByTagName("hole");
	for (int i = 0; i < nodeList.count(); i++) {
		QDomElement hole = nodeList.item(i).toElement();
		if (hole.isNull()) continue;

		path += genHole(hole, noStroke ? 0 : strokeWidth / 2, !clockwise);
	}

	if (noStroke) {
		QList<QDomElement> contacts;
		QStringList busNames;
		collectContacts(root, paramsRoot, contacts, busNames);
		foreach (QDomElement contact, contacts) {
			if (!isUsed(contact)) continue;

			QDomElement pad = contact.firstChildElement("pad");
			if (!pad.isNull()) {
				path += genHole(pad, 0, !clockwise);
				continue;
			}

			if (contact.tagName().compare("via") == 0) {
				path += genHole(contact, 0, !clockwise);
				continue;
			}
		}
	}

	// close the path
	if (path.length() > 0) {
		path += "'/>\n";
	}
	return path;
}

void BrdApplication::genOverlaps(QDomElement & root, const FillStroke & fsNormal, const FillStroke & fsIC, 
									QString & svg, bool offBoardOnly, const QStringList & ICs, QHash<QString, QString> & subpartAliases, bool includeSubparts) 
{
	QDir subpartsFolder(m_fritzingSubpartsPath);

	QList<QDomElement> padSmdPackages;
	collectPadSmdPackages(root, padSmdPackages);
	foreach (QDomElement package, padSmdPackages) {
		QString packageName = package.attribute("name", "").toLower();
		if (!offBoardOnly && includeSubparts) {
            QString sname = findSubpart(packageName, subpartAliases, subpartsFolder);
			if (!sname.isEmpty()) {
                qDebug() << "\tfound subpart (3)" << packageName << sname;
                continue;									// package will be drawn as a subpart
            }
		}

		qreal x1, y1, x2, y2;
		if (!MiscUtils::x1y1x2y2(package, x1, y1, x2, y2)) continue;
		
		QRectF overlap(x1, y1, x2 - x1, y2 - y1);
		bool offBoard = !m_boardBounds.contains(overlap);
		if (offBoardOnly && !offBoard) continue;

		QDomElement maxElement;
		QRectF r = getDimensions(package, maxElement, "", false);
		if (r.isEmpty()) continue;
		if (!overlap.contains(r)) continue;

		offBoard = !m_boardBounds.contains(r);
		if (offBoardOnly && !offBoard) continue;

		if (offBoard) {
			//qDebug() << "offboard" << package.attribute("name");
		}
		else {
			// only draw on-board package if TopPlaceLayer marks it
			r = getDimensions(package, maxElement, TopPlaceLayer, false);
			if (r.isEmpty()) {
				//qDebug() << "package" << packageName << "not on layer 21";
				continue;
			}
			if (!overlap.contains(r)) continue;

			// only draw on-board package if it doesn't overlap used contacts
			bool used = false;
			QDomElement contacts = package.firstChildElement("contacts");
			QDomElement contact = contacts.firstChildElement("contact");
			while (!contact.isNull()) {
				if (isUsed(contact)) {
					used = true;
					break;
				}
				contact = contact.nextSiblingElement("contact");
			}
			if (used) continue;


			QDomNodeList nodeList = package.elementsByTagName("hole");
			if (nodeList.count() > 0) continue;
		}

		if (offBoardOnly) {
			// only draw the part extending past the edge
			// assumes overlap on one side
			qreal dr = fsNormal.strokeWidth / 2;
			bool drawLeft = false;
			bool drawTop = false;
			bool drawRight = false;
			bool drawBottom = false;
			if (r.left() < m_boardBounds.left()) {
				drawLeft = drawTop = drawBottom = true;
				r.setRight(m_boardBounds.left());
			}
			else if (r.right() > m_boardBounds.right()) {
				r.setLeft(m_boardBounds.right());
				drawRight = drawTop = drawBottom = true;
			}
			else if (r.top() < m_boardBounds.top()) {
				drawRight = drawTop = drawLeft = true;
				r.setBottom(m_boardBounds.top());
			}
			else if (r.bottom() > m_boardBounds.bottom()) {
				r.setTop(m_boardBounds.bottom());
				drawRight = drawBottom = drawLeft = true;
			}
			r.setLeft(qMax(m_trueBounds.left() + dr, r.left()));
			r.setRight(qMin(m_trueBounds.right() - dr, r.right()));
			r.setTop(qMax(m_trueBounds.top() + dr, r.top()));
			r.setBottom(qMin(m_trueBounds.bottom() - dr, r.bottom()));

			QString lineString = QString("<line x1='%1' y1='%2' x2='%3' y2='%4' fill='%5' fill-opacity='%6' stroke='%7' stroke-width='%8' />\n");

			if (drawTop) {
				svg += lineString
					.arg(r.left() - m_trueBounds.left()).arg(flipy(r.top())).arg(r.right() - m_trueBounds.left()).arg(flipy(r.top()))
					.arg(fsNormal.fill).arg(fsNormal.fillOpacity) .arg(fsNormal.stroke) .arg(SW(fsNormal.strokeWidth));
			}
			if (drawBottom) {
				svg += lineString
					.arg(r.left() - m_trueBounds.left()).arg(flipy(r.bottom())).arg(r.right() - m_trueBounds.left()).arg(flipy(r.bottom()))
					.arg(fsNormal.fill).arg(fsNormal.fillOpacity) .arg(fsNormal.stroke) .arg(SW(fsNormal.strokeWidth));
			}
			if (drawLeft) {
				svg += lineString
					.arg(r.left() - m_trueBounds.left()).arg(flipy(r.top())).arg(r.left() - m_trueBounds.left()).arg(flipy(r.bottom()))
					.arg(fsNormal.fill).arg(fsNormal.fillOpacity) .arg(fsNormal.stroke) .arg(SW(fsNormal.strokeWidth));
			}
			if (drawRight) {
				svg += lineString
					.arg(r.right() - m_trueBounds.left()).arg(flipy(r.top())).arg(r.right() - m_trueBounds.left()).arg(flipy(r.bottom()))
					.arg(fsNormal.fill).arg(fsNormal.fillOpacity) .arg(fsNormal.stroke) .arg(SW(fsNormal.strokeWidth));
			}

			continue;
		}

		//qDebug() << "offboard package" << package.attribute("name");

		qreal y = flipy( r.center().y());
		y -= (r.height() / 2);

		const FillStroke * fs = &fsNormal;
		if (ICs.contains(packageName, Qt::CaseInsensitive)) {
			fs = &fsIC;
			//qDebug() << "using IC" << packageName << r.width() << r.height();
		}

		double angle = package.parentNode().toElement().attribute("angle", "0").toDouble();
		if ((qRound(angle) / 45) % 2 == 1) {
			QString points = QString("%1,%2, %3,%4 %5,%2 %3,%6")
					.arg(r.left() - m_trueBounds.left())
					.arg(flipy(r.center().y()))
					.arg(r.center().x() - m_trueBounds.left())
					.arg(flipy(r.top()))
					.arg(r.right() - m_trueBounds.left())
					.arg(flipy(r.bottom()))
					;
			svg += QString("<polygon fill='%1' fill-opacity='%2' stroke='%3' stroke-width='%4' points='%5' />\n")
					.arg(fs->fill)
					.arg(fs->fillOpacity)
					.arg(fs->stroke)
					.arg(SW(fs->strokeWidth))
					.arg(points)
					;	
		}
		else {
			svg += QString("<rect x='%1' y='%2' width='%3' height='%4' fill='%5' fill-opacity='%6' stroke='%7' stroke-width='%8' />\n")
					.arg(r.left() - m_trueBounds.left())
					.arg(y)
					.arg(r.width())
					.arg(r.height())
					.arg(fs->fill)
					.arg(fs->fillOpacity)
					.arg(fs->stroke)
					.arg(SW(fs->strokeWidth))
					;	
		}
	}
}

bool BrdApplication::isUsed(QDomElement & contact) {
	return (contact.attribute("used", "0").compare("1") == 0);
}

bool BrdApplication::isBus(QDomElement & contact) {
	return (contact.attribute("bus", "0").compare("1") == 0);
}

QString BrdApplication::genHole2(qreal cx, qreal cy, qreal r, int sweepFlag)
{
	return QString("M%1,%2a%3,%3 0 1 %5 %4,0 %3,%3 0 1 %5 -%4,0z\n")
				.arg(cx - m_trueBounds.left() - r)
				.arg(flipy(cy))
				.arg(r)
				.arg(2 * r)
				.arg(sweepFlag);
}

QString BrdApplication::genHole(QDomElement hole, qreal inset, bool clockwise) {
	bool ok;
	qreal x = MiscUtils::strToMil(hole.attribute("x", ""), ok);
	if (!ok) return "";

	qreal y = MiscUtils::strToMil(hole.attribute("y", ""), ok);
	if (!ok) return "";

	qreal drill = MiscUtils::strToMil(hole.attribute("drill", ""), ok);
	if (!ok) return "";

	drill *= m_shrinkHolesFactor;

	qreal radius = (drill / 2.0) - inset;

	int sweepflag = clockwise ? 1 : 0;

	return genHole2(x, y, radius, sweepflag);
}

void BrdApplication::loadDifParams(QDir & workingFolder, QHash<QString, DifParam *> & difParams)
{
	QFile difFile(workingFolder.absoluteFilePath("metadata.dif"));
	if (!difFile.exists()) {
		//qDebug() << "no 'metadata.dif' params file found";
		return;
	}

	if (!difFile.open(QIODevice::ReadOnly)) {
		qDebug() << "unable to open 'metadata.dif'";
		return;
	}

	QTextStream in(&difFile);
	while (true) {
		QString line = in.readLine();
		if (line.isEmpty()) {
			qDebug() << "unexpected format (1) in 'metadata.dif'";
			return;
		}

		if (line.startsWith("DATA", Qt::CaseInsensitive)) break;
	}
	in.readLine();
	in.readLine();
	in.readLine();
	QString line = in.readLine();
	if (!line.startsWith("BOT", Qt::CaseInsensitive)) {
		qDebug() << "unexpected format (2) in 'metadata.dif'";
		return;
	}

	QStringList fields;
	while (true) {
		QString pair = in.readLine();
		QString value = in.readLine();
		if (pair.startsWith("1")) {
			value.replace("\"", "");
			fields.append(value.toLower());
		}
		else if (pair.startsWith("0")) {
			QStringList s = pair.split(",");
			if (s.count() != 2) {
				qDebug() << "unexpected format (3) in 'metadata.dif'";
				return;
			}
			fields.append(s.at(1));
		}
		else {
			if (value.startsWith("BOT", Qt::CaseInsensitive)) {
				break;
			}

			qDebug() << "unexpected format (4) in 'metadata.dif'";
			return;
		}
	}

	int filenameIndex = fields.indexOf("filename");
	if (filenameIndex < 0) {
		qDebug() << "'filename' column not found in 'metadata.dif'";
		return;
	}

	//int titleIndex = fields.indexOf("title");
	//if (titleIndex < 0) {
		//qDebug() << "'title' column not found in 'metadata.dif'";
		//return;
	//}

	int nameIndex = fields.indexOf("name");
	if (nameIndex < 0) {
		qDebug() << "'name' column not found in 'metadata.dif'";
		return;
	}

	int urlIndex = fields.indexOf("url");
	if (urlIndex < 0) {
		qDebug() << "'url' column not found in 'metadata.dif'";
		return;
	}

	//int descriptionIndex = fields.indexOf("description");
	//if (descriptionIndex < 0) {
		//qDebug() << "'description' column not found in 'metadata.dif'";
		//return;
	//}

	int authorIndex = fields.indexOf("author");
	if (authorIndex < 0) {
		qDebug() << "'author' column not found in 'metadata.dif'";
		return;
	}

	int familyIndex = fields.indexOf("family");
	if (familyIndex < 0) {
		qDebug() << "'family' column not found in 'metadata.dif'";
		return;
	}

	int colorIndex = fields.indexOf("color");
	if (colorIndex < 0) {
		qDebug() << "'color' column not found in 'metadata.dif'";
		return;
	}

	int propertiesIndex = fields.indexOf("properties");
	if (propertiesIndex < 0) {
		qDebug() << "'properties' column not found in 'metadata.dif'";
		return;
	}

	int tagsIndex = fields.indexOf("tags");
	if (tagsIndex < 0) {
		qDebug() << "'tags' column not found in 'metadata.dif'";
		return;
	}

	bool done = false;
	while (!done) {
		QStringList values;
		while (true) {
			QString pair = in.readLine();
			QString value = in.readLine();

			if (pair.startsWith("1")) {
				value.replace("\"", "");
				values.append(value);
			}
			else if (pair.startsWith("0")) {
				QStringList s = pair.split(",");
				if (s.count() != 2) {
					qDebug() << "unexpected format (5) in 'metadata.dif'";
					return;
				}
				values.append(s.at(1));
			}
			else {
				if (value.startsWith("BOT", Qt::CaseInsensitive)) {
					break;
				}
				if (value.startsWith("EOD", Qt::CaseInsensitive)) {
					done = true;
					break;
				}

				qDebug() << "unexpected format (6) in 'metadata.dif'";
				return;
			}
		}
		
		QString filename = values.at(filenameIndex).trimmed();
		if (filename.isEmpty()) continue;

		QFileInfo fileInfo(filename);
		filename = fileInfo.completeBaseName().toLower();

		DifParam * difParam = new DifParam;
		difParams.insert(filename, difParam);

		difParam->author = values.at(authorIndex).trimmed();
		difParam->url = values.at(urlIndex).trimmed();
		//difParam->title = values.at(titleIndex).trimmed();
		if (difParam->title.isEmpty()) {
			difParam->title = values.at(nameIndex).trimmed();
		}
		difParam->boardColor = values.at(colorIndex).trimmed().toLower();

		//difParam->description = values.at(descriptionIndex).trimmed();
		QString family =  values.at(familyIndex).trimmed();
		if (!family.isEmpty()) {
			difParam->properties.insert("family", family);
		}
		QString allTags = values.at(tagsIndex).trimmed();
		QStringList tags = allTags.split(";");
		foreach (QString tag, tags) {
			if (tag.isEmpty()) continue;

			tag = tag.trimmed();
			if (tag.isEmpty()) continue;

			difParam->tags.append(tag);
		}
		QString allProps = values.at(propertiesIndex).trimmed();
		QStringList props = allProps.split(";");
		foreach (QString prop, props) {
			if (prop.isEmpty()) continue;

			QStringList ps = prop.split(":");
			if (ps.count() != 2) continue;

			QString p = ps.at(0).trimmed();
			if (p.isEmpty()) continue;

			QString v = ps.at(1).trimmed();
			if (v.isEmpty()) continue;

			difParam->properties.insert(p, v);
		}
	}
}


QString BrdApplication::loadDescription(const QString & prefix, const QString & url, const QDir & descriptionsFolder) 
{
	QFile file(descriptionsFolder.absoluteFilePath(prefix + ".txt"));
	if (file.open(QIODevice::ReadOnly)) {
		QByteArray byteArray = file.readAll();
		return QString(byteArray);
	}

	if (url.isEmpty()) return "";

	if (m_networkAccessManager == NULL) {
		m_networkAccessManager = new QNetworkAccessManager(this);
	}

	// execute an event loop to process the request (nearly-synchronous)
	QEventLoop eventLoop;
	connect(m_networkAccessManager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));

	QNetworkReply * reply = m_networkAccessManager->get(QNetworkRequest(QUrl(url + ".json")));

	eventLoop.exec();
	QByteArray data = reply->readAll();
	reply->deleteLater();

	if (data.isEmpty()) return "";

    /*
    QScriptEngine scriptEngine;
    QScriptValue scriptValue = scriptEngine.evaluate("(" + QString(data) + ")");   // In new versions it may need to look like engine.evaluate("(" + QString(result) + ")");
	QString descr = scriptValue.property("description").toString();
    */
    QString descr;
	if (descr.isEmpty()) return "";

	QString textString;

	/*
	QXmlStreamReader xml(descr);
	while (!xml.atEnd()) {
		if (xml.readNext() == QXmlStreamReader::Characters) {
			textString+= xml.text();
		}
	}
	*/

	textString = descr;

	saveFile(textString, descriptionsFolder.absoluteFilePath(prefix + ".txt"));
	return textString;
}

QString BrdApplication::translateBoardColor(const QString & color)
{
	QHash<QString,QString> colors;
	colors.insert("blue", "#147390");
	colors.insert("red", "#C62717");
	colors.insert("green", "#1F7A34");
	colors.insert("purple", "#672E58");
	colors.insert("black", "#1C1A1D");
	colors.insert("white", "#fbfbfb");
	return colors.value(color, color);
}

bool BrdApplication::match(QDomElement & contact, QDomElement & connector, bool doDebug)
{
	QDomElement parent = contact.parentNode().toElement();
	while (!parent.isNull()) {
		if (parent.tagName().compare("package") == 0) break;

		parent = parent.parentNode().toElement();
	}

	if (parent.isNull()) return false;

	QString packageName = parent.attribute("name");
	QString elementName = parent.parentNode().toElement().attribute("name");

	if (doDebug) {
		QString debug;
		debug += QString("contact %1 %2 %3 %4\n").arg(elementName).arg(packageName).arg(contact.attribute("name")).arg(contact.attribute("signal"));
		debug += QString("\tconnector %1 %2 %3 %4\n").arg(connector.attribute("element")).arg(connector.attribute("package")).arg(connector.attribute("name")).arg(connector.attribute("signal"));

		QFile file("debug.txt");
		if (file.open(QIODevice::Append | QIODevice::Text)) {
			QTextStream out(&file);
			out.setCodec("UTF-8");
			out << debug;
			file.close();
		}
	}

	QString connectorSignal = connector.attribute("signal");
	QString contactSignal = contact.attribute("signal");
	if (connectorSignal.isEmpty() && !contactSignal.isEmpty()) {
		connectorSignal = connector.attribute("name");
	}

	return (contactSignal.compare(connectorSignal) == 0 &&
			contact.attribute("name").compare(connector.attribute("name")) == 0 &&
			packageName.compare(connector.attribute("package")) == 0 &&
			elementName.compare(connector.attribute("element")) == 0);
}


bool BrdApplication::matchAnd(QDomElement & contact, QDomElement & connector) {
	if (match(contact, connector, false)) return true;

	if (connector.tagName().compare("via") != 0) return false;
	if (contact.tagName().compare("via") != 0) return false;
	
	return (connector.attribute("signal").compare(contact.attribute("signal")) == 0);
}

QString BrdApplication::findSubpart(const QString & name, QHash<QString, QString> & subpartAliases, QDir & subpartsFolder) {
	QFile file(subpartsFolder.absoluteFilePath(name + ".svg"));
	if (file.exists()) {
        return name;
	}

    QString aname = subpartAliases.value(name.toLower(), "");
    if (aname.isEmpty()) return "";

    QFile file2(subpartsFolder.absoluteFilePath(aname + ".svg"));
	if (file2.exists()) {
        return aname;
	}

    return "";
}

bool BrdApplication::registerFonts() {

	int ix = QFontDatabase::addApplicationFont(":/resources/fonts/DroidSans.ttf");
    if (ix < 0) return false;

	ix = QFontDatabase::addApplicationFont(":/resources/fonts/DroidSans-Bold.ttf");
    if (ix < 0) return false;

	ix = QFontDatabase::addApplicationFont(":/resources/fonts/DroidSansMono.ttf");
    if (ix < 0) return false;

	ix = QFontDatabase::addApplicationFont(":/resources/fonts/OCRA.ttf");
    return ix >= 0;
}
