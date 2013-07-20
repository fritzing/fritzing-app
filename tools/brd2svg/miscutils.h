

#ifndef MISCUTILS_H
#define MISCUTILS_H

#include <QDir>
#include <QString>
#include <QDomElement>
#include <QList>
#include <QRectF>

typedef QString (*GetConnectorNameFn)(const QDomElement &);

class MiscUtils {

public:
	static bool makePartsDirectories(const QDir & workingFolder, const QString & core, QDir & fzpFolder, QDir & breadboardFolder, QDir & schematicFolder, QDir & pcbFolder, QDir & iconFolder);
	static void calcTextAngle(qreal & angle, int mirror, int spin, qreal size, qreal & x, qreal & y, bool & anchorAtStart);
	static QString makeGeneric(const QDir & workingFolder, const QString & boardColor, QList<QDomElement> & powers, 
			QString & copper, const QString & boardName, QSizeF outerChipSize, QSizeF innerChipSize,
            GetConnectorNameFn getConnectorName, GetConnectorNameFn getConnectorIndex, bool noText);
    static bool makeWireTrees(QList<QDomElement> & wireList, QList<struct WireTree *> & wireTrees);
    static bool rwaa(QDomElement & element, qreal & radius, qreal & width, qreal & angle1, qreal & angle2);
    static bool x1y1x2y2(const QDomElement & element, qreal & x1, qreal & y1, qreal & x2, qreal & y2);
    static qreal strToMil(const QString & str, bool & ok);

protected:
	static void includeSvg2(QDomDocument & doc, const QString & path, const QString & name, qreal x, qreal y);

};

struct WireTree {
	qreal x1, x2, y1, y2, curve;
	qreal radius, angle1, angle2;
	WireTree * left;
	WireTree * right;
	QString width;
	int sweep;
	qreal ax1, ax2, ay1, ay2, oa1, oa2;
    bool failed;
    QDomElement element;

    WireTree(QDomElement & w);
	void turn();
    void resetArc();
};

#endif
