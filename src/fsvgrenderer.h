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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#ifndef FSVGRENDERER_H
#define FSVGRENDERER_H

#include <QHash>
#include <QSvgRenderer>
#include <QXmlStreamReader>
#include <QDomDocument>
#include <QMatrix>
#include <QStringList>

#include "viewlayer.h"

struct ConnectorInfo {
	bool gotCircle;
	double radius;
	double strokeWidth;
	QMatrix matrix;
	//QRectF cbounds;
	QMatrix terminalMatrix;
	QMatrix legMatrix;
	QString legColor;
	QLineF legLine;
	double legStrokeWidth;
    bool gotPath;
};

typedef QHash<ViewLayer::ViewLayerID, class FSvgRenderer *> RendererHash;

struct LoadInfo {
     QString filename;
     QStringList connectorIDs;
     QStringList terminalIDs;
     QStringList legIDs;
     QString setColor;
     QString colorElementID;
     bool findNonConnectors;
     bool parsePaths;

     LoadInfo() {
         findNonConnectors = parsePaths = false;
     }
};

class FSvgRenderer : public QSvgRenderer
{
Q_OBJECT
public:
	FSvgRenderer(QObject * parent = 0);
	~FSvgRenderer();

	QByteArray loadSvg(const LoadInfo &);
	QByteArray loadSvg(const QString & filename);
	QByteArray loadSvg( const QByteArray & contents, const LoadInfo &);     // for SvgSplitter loads
	QByteArray loadSvg( const QByteArray & contents, const QString & filename, bool findNonConnectors);						// for SvgSplitter loads
	bool loadSvgString(const QString & svg);
	bool loadSvgString(const QString & svg, QString & newSvg);
	bool fastLoad(const QByteArray & contents);	
    QByteArray finalLoad(QByteArray & cleanContents, const QString & filename);
	const QString & filename();
	QSizeF defaultSizeF();
	bool setUpConnector(class SvgIdLayer * svgIdLayer, bool ignoreTerminalPoint, ViewLayer::ViewLayerPlacement);
	QList<SvgIdLayer *> setUpNonConnectors(ViewLayer::ViewLayerPlacement);

public:
	static void cleanup();
	static QSizeF parseForWidthAndHeight(QXmlStreamReader &);
	static QPixmap * getPixmap(QSvgRenderer * renderer, QSize size);
    static void initNames();

protected:
	bool determineDefaultSize(QXmlStreamReader &);
	QByteArray loadAux (const QByteArray & contents, const LoadInfo &);
	bool initConnectorInfo(QDomDocument &, const LoadInfo &);
	ConnectorInfo * initConnectorInfoStruct(QDomElement & connectorElement, const QString & filename, bool parsePaths);
	bool initConnectorInfoStructAux(QDomElement &, ConnectorInfo * connectorInfo, const QString & filename, bool parsePaths);
    bool initConnectorInfoCircle(QDomElement & element, ConnectorInfo * connectorInfo, const QString & filename);
    bool initConnectorInfoPath(QDomElement & element, ConnectorInfo * connectorInfo, const QString & filename);
	void initNonConnectorInfo(QDomDocument & domDocument, const QString & filename);
	void initNonConnectorInfoAux(QDomElement & element, const QString & filename);
	void initTerminalInfoAux(QDomElement & element, const LoadInfo &);
	void initLegInfoAux(QDomElement & element, const LoadInfo &, bool & gotOne);
	void initConnectorInfoAux(QDomElement & element, const LoadInfo &);
	QPointF calcTerminalPoint(const QString & terminalId, const QRectF & connectorRect, bool ignoreTerminalPoint, const QRectF & viewBox, QMatrix & terminalMatrix);
	bool initLegInfoAux(QDomElement & element, ConnectorInfo * connectorInfo);
	void calcLeg(SvgIdLayer *, const QRectF & viewBox, ConnectorInfo * connectorInfo);
	ConnectorInfo * getConnectorInfo(const QString & connectorID);
	void clearConnectorInfoHash(QHash<QString, ConnectorInfo *> & hash);

protected:
	QString m_filename;
	QSizeF m_defaultSizeF;
	QHash<QString, ConnectorInfo *> m_connectorInfoHash;
	QHash<QString, ConnectorInfo *> m_nonConnectorInfoHash;

public:
	static QString NonConnectorName;

};


#endif
