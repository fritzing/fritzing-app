#ifndef APPLICATION_H
#define APPLICATION_H

#include <QCoreApplication>
#include <QDomDocument>
#include <QRectF>
#include <QDir>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>

struct FillStroke {
	QString fill;
	qreal fillOpacity;
	QString stroke;
	qreal strokeWidth;
};

class BrdApplication : public QCoreApplication
{
public:
    BrdApplication(int argc, char *argv[]);
    void start();

protected:
    void usage();
    void message(const QString &);
    bool initArguments();
	QRectF getDimensions(QDomElement & root, QDomElement & maxElement, const QString & layer, bool deep);
	QRectF getPlainBounds(QDomElement & root, const QString & layer);
	QString genPCB(QDomElement & root, QDomElement & paramsRoot);
	QString genSchematic(QDomElement & root, QDomElement & paramsRoot, class DifParam *);
	QString genBreadboard(QDomElement & root, QDomElement & paramsRoot, class DifParam *, const QStringList & ICs, QHash<QString, QString> & subpartAliases);
	QString genGenericBreadboard(QDomElement & root, QDomElement & paramsRoot, class DifParam *, QDir & brdFolder);
	QString genFZP(QDomElement & root, QDomElement & paramsRoot, class DifParam *, const QString & prefix, const QString & connectorType, const QDir & descrsFolder);
	QString genParams(QDomElement & root, const QString & prefix); 
	void genBin(QStringList & fileList, const QString & title, const QString & binPath);
	void genXml(QDir & brdFolder, QDir & ulpFolder, const QString & brdname, QDir & xmlFolder);
	void collectLayerElements(QList<QDomElement> & from, QList<QDomElement> & to, const QString & layerID);
	void genLayerElements(QDomElement &root, QDomElement & paramsRoot, QString & svg, const QString & layerID, bool skipText, qreal minArea, bool doFillings, const QString & textColor);
	void genLayerElement(QDomElement & paramsRoot, QDomElement &, QString & svg, const QString & layerID, bool skipText, qreal minArea, bool doFillings, const QString & textColor);
	void genCircle(QDomElement & element, QString & svg, bool forDimension, const QString & fill, const QString & stroke, qreal strokeWidth);
	void genPath(QDomElement & element, QString & svg, const QString & fill, const QString & stroke, bool doFillings);
	void genRect(QDomElement & element, QString & svg, bool forDimension);
	void genLine(QDomElement & element, QString & svg);
	void genArc(QDomElement & element, QString & svg);
	void genPad(QDomElement & contact, QString & svg, const QString & layerID, const QString & copperColor, const QString & padString, bool integrateVias);
	void genPadAux(QDomElement & contact, QDomElement & pad, QString & svg, const QString & layerID, const QString & copperColor, const QString & padString, bool integrateVias);
	void genSmd(QDomElement & contact, QString & svg, const QString & layerID, const QString & copperColor, const QString & padString);
	void genCopperElements(QDomElement &root, QDomElement & paramsRoot, QString & svg, const QString & layerID, const QString & copperColor, const QString & padString, bool integrateVias);
	void genText(QDomElement & element, const QString & text, QString & svg, QDomElement & paramsRoot, const QString & textColor);
	qreal flipy(qreal y);
	bool inBounds(QDomElement & package);
	bool inBounds(qreal x1, qreal y1, qreal x2, qreal y2);
	void saveFile(const QString & content, const QString & suffix);
	void collectContacts(QDomElement &root, QDomElement & paramsRoot, QList<QDomElement> & contacts, QStringList & busNames);
	void collectPackages(QDomElement &root, QList<QDomElement> & packages);
	void getSides(QDomElement & root, QDomElement & paramsRoot, 
				QList<QDomElement> & powers, QList<QDomElement> & grounds, QList<QDomElement> & lefts, QList<QDomElement> & rights, QList<QDomElement> & unused, QList<QDomElement> & vias,
				QStringList & busNames, bool collectSpaces, bool integrateVias); 
	void collectWires(QDomElement & element, QList<QDomElement> & wires, bool useFillings);
	QString genContact(QDomElement & contact);
	void collectConnectors(QDomElement &paramsRoot, QList<QDomElement> & connectorList, bool collectSpaces);
	void collectFakeVias(QDomElement &paramsRoot, QList<QDomElement> & connectorList);
	QDomDocument loadParams(QFile & paramsFile, const QString & basename);
	void collectPadSmdPackages(QDomElement & root, QList<QDomElement> & padSmdPackages);
	bool polyFromWires(QDomElement & root, const QString & boardColor, const QString & stroke, qreal strokeWidth, QString & svg, bool & clockwise);
	QString genMaxShape(QDomElement & root, QDomElement & paramsRoot, const QString & boardColor, const QString & stroke, qreal strokeWidth); 
	void genOverlaps(QDomElement & root, const FillStroke & normal, const FillStroke & IC, 
						QString & svg, bool offBoardOnly, const QStringList & ICs, QHash<QString, QString> & subpartAliases, bool includeSubparts); 
	bool isUsed(QDomElement & contact);
	bool isBus(QDomElement & contact);
	QString genHole(QDomElement hole, qreal inset, bool clockwise);
	QString genHole2(qreal cx, qreal cy, qreal r, int sweepFlag);
	void addSubparts(QDomElement & root, QDomElement & paramsRoot, QString & svg, QHash<QString, QString> & subpartAliases);
	bool bigEnough(QDomElement & package, qreal minArea);
	void loadDifParams(QDir & workingFolder, QHash<QString, class DifParam *> & csvParams);
	bool getArcBounds(QDomElement wire, QDomElement arc, qreal & x1, qreal & y1, qreal & x2, qreal & y2);
	void includeSvg(QDomDocument & doc, const QString & path, const QString & name, qreal x, qreal y);
	void getPackagesBounds(QDomElement & root, QRectF & bounds, const QString & layer, bool reset, bool deep);
	void addElements(QDomElement & root, QList<QDomElement> & to, qreal minArea);
	QString getBoardName(QDomElement & root);
	QString loadDescription(const QString & prefix, const QString & url, const QDir & descrsFolder);
	QString genPolyString(QList<QDomElement> &, QDomElement & element, qreal & width);
	QString genPolyString(QList<struct WireTree *> &, QDomElement & element, qreal & width);
	QString addPathUnit(WireTree * wireTree, QPointF p, qreal rDelta);
	void replaceXY(QString & string);
	QString translateBoardColor(const QString & color);
	bool match(QDomElement & contact, QDomElement & connector, bool doDebug);
	bool matchAnd(QDomElement & contact, QDomElement & connector);
    QString findSubpart(const QString & name, QHash<QString, QString> & subpartAliases, QDir & subpartsFolder);
    bool registerFonts();
    QString schematicPinNumber(qreal x, qreal y, qreal pinSmallTextHeight, const QString & id, bool rotate, bool gotZero, bool gotOne);
    QString schematicPinText(const QString & id, const QString & signal, qreal x, qreal y, qreal bigPinFontSize, const QString & anchor, bool rotate);

protected:
    QString m_workingPath;
    QString m_andPath;
    QString m_eaglePath;
    QString m_fritzingSubpartsPath;
	QDomDocument m_boardDoc;
	QRectF m_trueBounds;
	QRectF m_boardBounds;
	QDomElement m_maxElement;
	bool m_genericSMD;
	QString m_core;
	qreal m_cxLast;
	qreal m_cyLast;
	qreal m_shrinkHolesFactor;
	QNetworkAccessManager * m_networkAccessManager;
};

class DifParam {

public:
	DifParam() {}

public:
	QString filename;
	QString title;
	QString description;
	QString author;
	QHash<QString, QString> properties;
	QStringList tags;
	QString boardColor;
	QString url;
};

class Renamer
{
public:
	Renamer(const QDomElement &);

public:
	QString element;
	QString package;
    QString signal;
    QString name;
    QString to;
};


#endif // APPLICATION_H
