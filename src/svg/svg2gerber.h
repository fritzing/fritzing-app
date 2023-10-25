/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

********************************************************************/

#ifndef SVG2GERBER_H
#define SVG2GERBER_H

#include <QString>
#include <QDomElement>
#include <QObject>
#include <QTransform>
#include <QMultiHash>

class SVG2gerber : public QObject
{
	Q_OBJECT

public:
	SVG2gerber() = default;

public:
	enum ForWhy {
		ForCopper,
		ForSilk,
		ForOutline,
		ForMask,
		ForDrill,
		ForPasteMask
	};

	int convert(const QString & svgStr, bool doubleSided, const QString & mainLayerName, ForWhy, QSizeF boardSize);
	QString getGerber();

protected:
	QDomDocument m_SVGDom;
	QString m_gerber_header;
	QString m_gerber_paths;
	QString m_drill_slots;
	QSizeF m_boardSize;
	QMultiHash<QString, QString> m_platedApertures;
	QMultiHash<QString, QString> m_holeApertures;

	double m_pathstart_x = 0.0;
	double m_pathstart_y = 0.0;

	// Fritzing internal scale (1000mil) to gerber scale factor
	// legacy gerber export is 1000mil (3 decimals). For 6 decimal
	// gerber export, this will be set to 1000.0
	double m_f2g = 1.0;
	QString m_G54 = "G54";

protected:

	void normalizeSVG();
	void convertShapes2paths(QDomNode);
	void flattenSVG(QDomNode);
	QTransform parseTransform(QDomElement);

	QDomElement ellipse2path(QDomElement);

	void copyStyles(QDomElement, QDomElement);

	int renderGerber(bool doubleSided, const QString & mainLayerName, ForWhy);
	int allPaths2gerber(ForWhy);
	QString path2gerber(QDomElement);
	void handleOblongPath(QDomElement & path, int & dcode_index);
	QString standardAperture(QDomElement & element, QHash<QString, QString> & apertureMap, QString & current_dcode, int & dcode_index, double stroke_width);
	double flipy(double y);

	// Transform from Fritzing scale to Gerber scale
	QString f2gerber(double value);

	void doPoly(QDomElement & polygon, ForWhy forWhy, bool closedCurve,
	            QHash<QString, QString> & apertureMap, QString & current_dcode, int & dcode_index);



protected Q_SLOTS:
	void path2gerbCommandSlot(QChar command, bool relative, QList<double> & args, void * userData);


};

#endif // SVG2GERBER_H
