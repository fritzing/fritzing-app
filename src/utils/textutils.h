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

#ifndef TEXTUTILS_H
#define TEXTUTILS_H

#include <QPointF>
#include <QDomElement>
#include <QSet>
#include <QMatrix>
#include <QTransform>
#include <QXmlStreamReader>

typedef QString (*CopyPinFunction)(int pin, const QString & argString, void * userData);
typedef QString (*MultiplyPinFunction)(int pin, double increment, double value);

class TextUtils
{

public:
	static QSet<QString> getRegexpCaptures(const QString &pattern, const QString &textToSearchIn);
	static QDomElement findElementWithAttribute(QDomElement element, const QString & attributeName, const QString & attributeValue);
	static void findElementsWithAttribute(QDomElement & element, const QString & att, QList<QDomElement> & elements);
	static double convertToInches(const QString & string, bool * ok, bool isIllustrator);
	static double convertToInches(const QString & string);
	static QString convertToPowerPrefix(double);
	static double convertFromPowerPrefix(const QString & val, const QString & symbol);
	static double convertFromPowerPrefixU(QString & val, const QString & symbol);

	static QString replaceTextElement(const QString & svg, const QString & id, const QString &  newValue);
	static QByteArray replaceTextElement(const QByteArray & svg, const QString & id, const QString &  newValue);
	static QString replaceTextElements(const QString & svg, const QHash<QString, QString> &);
    static bool squashElement(QDomDocument &, const QString & elementName, const QString & attName, const QRegExp & matchContent);
    static QString mergeSvg(const QString & svg1, const QString & svg2, const QString & id, bool flip);
	static QString mergeSvgFinish(QDomDocument & doc);
	static bool mergeSvg(QDomDocument & doc1, const QString & svg, const QString & id);
	static QString makeSVGHeader(double printerscale, double dpi, double width, double height);
	static bool isIllustratorFile(const QString &fileContent);
	static bool isIllustratorFile(const QByteArray &fileContent);
	static bool isIllustratorDoc(const QDomDocument & doc);
	static QString removeXMLEntities(QString svgContent);
	static bool cleanSodipodi(QString &bytes);
	static bool fixPixelDimensionsIn(QString &fileContent);
	static bool addCopper1(const QString & filename, QDomDocument & doc, const QString & srcAtt, const QString & destAtt);
	static void setSVGTransform(QDomElement &, QMatrix &);
	static QString svgMatrix(const QMatrix &);
	static QString svgMatrix(const QTransform &);
	static QString svgTransform(const QString & svg, QTransform & transform, bool translate, const QString extra);
	static bool getSvgSizes(QDomDocument & doc, double & sWidth, double & sHeight, double & vbWidth, double & vbHeight);
	static bool findText(const QDomNode & node, QString & text);
	static QString stripNonValidXMLCharacters(const QString & str); 
	static QString convertExtendedChars(const QString & str); 
	static QString escapeAnd(const QString &);
	static QMatrix elementToMatrix(QDomElement & element);
	static QMatrix transformStringToMatrix(const QString & transform);
    static QList<double> getTransformFloats(QDomElement & element);
	static QList<double> getTransformFloats(const QString & transform);
	static QString svgNSOnly(QString svgContent);
	static QString killXMLNS(QString svgContent);
	static void gWrap(QDomDocument & domDocument, const QHash<QString, QString> & attributes);
	static void slamStrokeAndFill(QDomElement &, const QString & stroke, const QString & strokeWidth, const QString & fill);
	static QString slamStrokeAndFill(const QString & svg, const QString & stroke, const QString & strokeWidth, const QString & fill);
	static QString incrementTemplate(const QString & filename, int pins, double unitIncrement, MultiplyPinFunction, CopyPinFunction, void * userData);
	static QString incrementTemplateString(const QString & templateString, int pins, double increment, MultiplyPinFunction, CopyPinFunction, void * userData);
	static QString standardCopyPinFunction(int pin, const QString & argString, void * userData);
	static QString standardMultiplyPinFunction(int pin, double increment, double value);
	static QString noCopyPinFunction(int pin, const QString & argString, void * userData);
	static QString incMultiplyPinFunction(int pin, double increment, double value);
	static QString incCopyPinFunction(int pin, const QString & argString, void * userData);
	static QString negIncCopyPinFunction(int pin, const QString & argString, void * userData);
	static double getViewBoxCoord(const QString & svg, int coord);
	static QString makeLineSVG(QPointF p1, QPointF p2, double width, QString colorString, double dpi, double printerScale, bool blackOnly, bool dashed, const QVector<qreal> &);
	static QString makeCubicBezierSVG(const QPolygonF & controlPoints, double width, QString colorString, double dpi, double printerScale, bool blackOnly, bool dashed, const QVector<qreal> &);
	static QString makeRectSVG(QRectF r, QPointF offset, double dpi, double printerscale);
	static QString makePolySVG(const QPolygonF & poly, QPointF offset, double width, QString colorString, double dpi, double printerScale, bool blackOnly);
	static QPolygonF polygonFromElement(QDomElement & element);
	static QString pointToSvgString(QPointF p, QPointF offset, double dpi, double printerScale);
	static void replaceChildText(QDomNode & node, const QString & text);
	static void replaceElementChildText(QDomElement & root, const QString & elementName, const QString & text);
	static QString removeSVGHeader(QString & string);
	//static QString getMacAddress();
	static QString expandAndFill(const QString & svg, const QString & color, double expandBy);
	static void expandAndFillAux(QDomElement &, const QString & color, double expandBy);
    static bool writeUtf8(const QString & fileName, const QString & text);
    static int getPinsAndSpacing(const QString & expectedFileName, QString & spacingString);
	static QSizeF parseForWidthAndHeight(QXmlStreamReader &, QRectF & viewBox, bool getViewBox);
	static QSizeF parseForWidthAndHeight(QXmlStreamReader &);
	static QSizeF parseForWidthAndHeight(const QString & svg, QRectF & viewBox, bool getViewBox);
	static QSizeF parseForWidthAndHeight(const QString & svg);
    static void gornTree(QDomDocument &);
    static bool elevateTransform(QDomElement &);
    static bool fixMuch(QString &svg, bool fixStrokeWidth);
	static bool fixInternalUnits(QString & svg);
	static bool fixFonts(QString & svg, const QString & destFont, bool & reallyFixed);
	static void fixStyleAttribute(QDomElement & element);
    static QString parseForModuleID(const QString & fzpXmlString);
    static QString parseFileForModuleID(const QString & fzpPath);
    static QString getRandText();
    static bool ensureViewBox(QDomDocument doc, double dpi, QRectF & rect, bool toInches, double & w, double & h, bool getwh);
    static QString findAnchor(const QDomElement & text);
    static double getStrokeWidth(QDomElement &, double defaultValue);
    static void resplit(QStringList & names, const QString & split);
    static QString elementToString(const QDomElement &);

public:
	static const QRegExp FindWhitespace;
	static const QString SMDFlipSuffix;
	static const QString MicroSymbol;
    static const ushort MicroSymbolCode;
	static const QString PowerPrefixesString;
	static const QString CreatedWithFritzingString;
	static const QString CreatedWithFritzingXmlComment;
	static void collectLeaves(QDomElement & element, int & index, QVector<QDomElement> & leaves);
	static void collectLeaves(QDomElement & element, QList<QDomElement> & leaves);
	static const QRegExp floatingPointMatcher;
	static const QString RegexFloatDetector;
	static const QString AdobeIllustratorIdentifier;

protected:
	static bool pxToInches(QDomElement &elem, const QString &attrName, bool isIllustrator);
    static void squashNotElement(QDomElement & element, const QString & elementName, const QString & attName, const QRegExp & matchContent, bool & result);
	static void initPowerPrefixes();
	static QDomElement copyText(QDomDocument & svgDom, QDomElement & parent, QDomElement & text, const QString & defaultX, const QString & defaultY, bool copyAttributes);
    static void gornTreeAux(QDomElement &);
    static bool noPatternAux(QDomDocument & svgDom, const QString & tag);
    static bool noUseAux(QDomDocument & svgDom);
    static bool tspanRemoveAux(QDomDocument & svgDom);
    static void fixStyleAttribute(QDomElement & element, QString & style, const QString & attributeName);
    static bool fixStrokeWidth(QDomDocument & svgDoc);
    static bool fixViewBox(QDomElement & root);
    static void chopNotDigits(QString &);
    static void collectTransforms(QDomElement & root, QList<QDomElement> & transforms);

};

#endif
