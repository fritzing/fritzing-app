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

$Revision: 6941 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-26 15:03:18 +0100 (Di, 26. Mrz 2013) $

********************************************************************/

#ifndef GROUNDPLANEGENERATOR_H
#define GROUNDPLANEGENERATOR_H

#include "../connectors/connectoritem.h"
#include "../lib/clipper/clipper.hpp"

#include <QImage>
#include <QList>
#include <QRect>
#include <QPolygon>
#include <QString>
#include <QStringList>
#include <QGraphicsItem>

struct GroundFillSeed {
    GroundFillSeed(QRectF relativeRect_):relativeRect(relativeRect_) {
    }
    // rect has to be scaled by board size to get real rect.
    QRectF relativeRect;
};

struct GPGParams {
    QString boardSvg;
    QSizeF boardImageSize;
    QString svg;
    QSizeF copperImageSize; 
	QStringList exceptions;
    QGraphicsItem * board;
    double res;
    QString color;
    double keepoutMils;
    QList<GroundFillSeed> seeds;
    QPointF *seedPoint;
};

class GroundPlaneGenerator : public QObject
{
	Q_OBJECT
public:
	GroundPlaneGenerator();
	~GroundPlaneGenerator();

    bool generateGroundPlane(const QString & boardSvg, QSizeF boardImageSize, const QString & svg, QSizeF copperImageSize, QStringList & exceptions,
                             QGraphicsItem * board, double res, const QString & color, double keepoutMils, QList<GroundFillSeed> seeds);
	bool generateGroundPlaneUnit(const QString & boardSvg, QSizeF boardImageSize, const QString & svg, QSizeF copperImageSize, QStringList & exceptions, 
							 QGraphicsItem * board, double res, const QString & color, QPointF whereToStart, double keepoutMils);
    const QStringList & newSVGs();
	const QList<QPointF> & newOffsets();
	void setStrokeWidthIncrement(double);
	void setLayerName(const QString &);
	const QString & layerName();
	void setMinRunSize(int minRunSize, int minRiseSize);
    QString mergeSVGs(const QString & initialSVG, const QString & layerName);

public:
    static QString ConnectorName;

protected:
    bool generateGroundPlaneFn(GPGParams &); 
    void makeCopperFillFromPolygons(QList<ClipperLib::Paths> &sortedPolygons, double res, const QString &colorString, bool makeConnectorFlag, QSizeF minAreaInches, double minDimensionInches);

protected:
    QStringList m_newSVGs;
	QList<QPointF> m_newOffsets;
	double m_blurBy;
	QString m_layerName;
	double m_strokeWidthIncrement;
	int m_minRunSize;
	int m_minRiseSize;

public:
    static const QString KeepoutSettingName;
    static const double KeepoutDefaultMils;

    void createGroundThermalPads(GPGParams &params, double clipperDPI, std::vector<ClipperLib::Path> &groundConnectorsZone, std::vector<ClipperLib::Path> &groundThermalConnectors);
};

#endif
