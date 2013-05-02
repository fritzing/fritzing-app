/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2012 Fachhochschule Potsdam - http://fh-potsdam.de

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

#ifndef STRIPBOARD_H
#define STRIPBOARD_H

#include <QRectF>
#include <QPainterPath>
#include <QGraphicsPathItem>

#include "perfboard.h"

class ConnectorItem;

class Stripbit : public QGraphicsPathItem
{
public:
	Stripbit(const QPainterPath & path, ConnectorItem *, int x, int y, QGraphicsItem * parent);
	~Stripbit();

	void setRight(Stripbit *);
	Stripbit * right();
	void setRemoved(bool);
	bool removed();
	void setChanged(bool);
	bool changed();
	ConnectorItem * connectorItem();
	int y();
	int x();

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void hoverEnterEvent( QGraphicsSceneHoverEvent * event );
	void hoverLeaveEvent( QGraphicsSceneHoverEvent * event );

protected:
	ConnectorItem * m_connectorItem;
	bool m_removed;
	bool m_inHover;
	int m_x;
	int m_y;
	bool m_changed;
	Stripbit * m_right;
};

class Stripboard : public Perfboard 
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	Stripboard(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~Stripboard();

	QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);
	void addedToScene(bool temporary);
	void setProp(const QString & prop, const QString & value);
	void reinitBuses(bool triggerUndo);
	void initCutting(Stripbit *);
	void getConnectedColor(ConnectorItem *, QBrush * &, QPen * &, double & opacity, double & negativePenWidth, bool & negativeOffsetRect);
	void restoreRowColors(Stripbit * stripbit);

protected:
	void nextBus(QList<ConnectorItem *> & soFar);
	QString getRowLabel();
	QString getColumnLabel();

public:
	static QString genFZP(const QString & moduleID);
	static QString genModuleID(QMap<QString, QString> & currPropsMap);

protected:
	QVector<ConnectorItem *> m_lastColumn;
	QVector<Stripbit *> m_firstColumn;
	QList<class BusShared *> m_buses;
	QString m_beforeCut;

};

#endif
