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

$Revision: 6980 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-22 01:45:43 +0200 (Mo, 22. Apr 2013) $

********************************************************************/


#ifndef SVGFILESPLITTER_H_
#define SVGFILESPLITTER_H_

#include <QString>
#include <QByteArray>
#include <QDomElement>
#include <QObject>
#include <QMatrix>
#include <QPainterPath>
#include <QRegExp>
#include <QFile>

struct PathUserData {
	QString string;
    QMatrix transform;
	double sNewWidth;
	double sNewHeight;
	double vbWidth; 
	double vbHeight;
	double x;
	double y;
	bool pathStarting;
	QPainterPath * painterPath;
};

class SvgFileSplitter : public QObject {
	Q_OBJECT

public:
	SvgFileSplitter();
	bool split(const QString & filename, const QString & elementID);
	bool splitString(QString & contents, const QString & elementID);
	const QByteArray & byteArray();
	const QDomDocument & domDocument();
	bool normalize(double dpi, const QString & elementID, bool blackOnly, double & factor);
	QString shift(double x, double y, const QString & elementID, bool shiftTransforms);
	QString elementString(const QString & elementID);
    virtual bool parsePath(const QString & data, const char * slot, PathUserData &, QObject * slotTarget, bool convertHV);
	QVector<QVariant> simpleParsePath(const QString & data);
	QPainterPath painterPath(double dpi, const QString & elementID);			// note: only partially implemented
	void shiftChild(QDomElement & element, double x, double y, bool shiftTransforms);
	bool load(const QString * filename);
	bool load(QFile *);
	bool load(const QString string);
	QString toString();
	void gWrap(const QHash<QString, QString> & attributes);
	void gReplace(const QString & id);

public:
	static bool getSvgSizeAttributes(const QString & svg, QString & width, QString & height, QString & viewBox);
	static bool changeStrokeWidth(const QString & svg, double delta, bool absolute, bool changeOpacity, QByteArray &);
	static void changeStrokeWidth(QDomElement & element, double delta, bool absolute, bool changeOpacity);
	static void forceStrokeWidth(QDomElement & element, double delta, const QString & stroke, bool recurse, bool fill);
	static bool changeColors(const QString & svg, QString & toColor, QStringList & exceptions, QByteArray &);
	static void changeColors(QDomElement & element, QString & toColor, QStringList & exceptions);
	static void fixStyleAttributeRecurse(QDomElement & element);
	static void fixColorRecurse(QDomElement & element, const QString & newColor, const QStringList & exceptions);
    static QByteArray hideText(const QString & filename);
    static QByteArray showText(const QString & filename, bool & hasText);
    static QByteArray hideText2(const QByteArray & svg);
    static QByteArray showText2(const QByteArray & svg, bool & hasText);
    static QString hideText3(const QString & svg);
    static QString showText3(const QString & svg, bool & hasText);

protected:
	void normalizeChild(QDomElement & childElement, 
						double sNewWidth, double sNewHeight,
						double vbWidth, double vbHeight, bool blackOnly);
	bool normalizeAttribute(QDomElement & element, const char * attributeName, double num, double denom);
	void painterPathChild(QDomElement & element, QPainterPath & ppath);			// note: only partially implemented
	void normalizeTranslation(QDomElement & element, 
							double sNewWidth, double sNewHeight,
							double vbWidth, double vbHeight);
	bool shiftTranslation(QDomElement & element, double x, double y);
	void standardArgs(bool relative, bool starting, QList<double> & args, PathUserData * pathUserData);

protected:
	static bool shiftAttribute(QDomElement & element, const char * attributeName, double d);
	static void setStrokeOrFill(QDomElement & element, bool doIt, const QString & color, bool force);
    static void hideTextAux(QDomElement & parent, bool hideChildren);
    static void showTextAux(QDomElement & parent, bool & hasText, bool root);

protected slots:
	void normalizeCommandSlot(QChar command, bool relative, QList<double> & args, void * userData);
	void shiftCommandSlot(QChar command, bool relative, QList<double> & args, void * userData);
    virtual void rotateCommandSlot(QChar, bool, QList<double> &, void *){}
	void painterPathCommandSlot(QChar command, bool relative, QList<double> & args, void * userData);
	void convertHVSlot(QChar command, bool relative, QList<double> & args, void * userData);

protected:
	QByteArray m_byteArray;
	QDomDocument m_domDocument;

};

#endif
