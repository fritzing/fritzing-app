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
	Stripbit(const QPainterPath & path, int x, int y, bool horizontal, QGraphicsItem * parent);
	~Stripbit();

    bool horizontal();
	void setRemoved(bool);
	bool removed();
	void setChanged(bool);
	bool changed();
	int y();
	int x();
    QString makeRemovedString();

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void hoverEnterEvent( QGraphicsSceneHoverEvent * event );
	void hoverLeaveEvent( QGraphicsSceneHoverEvent * event );

protected:
	bool m_removed;
	bool m_inHover;
	int m_x;
	int m_y;
	bool m_changed;
    bool m_horizontal;
};

struct StripConnector {
    ConnectorItem * connectorItem;
    Stripbit * down;
    Stripbit * right;

    StripConnector();
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
	void getConnectedColor(ConnectorItem *, QBrush &, QPen &, double & opacity, double & negativePenWidth, bool & negativeOffsetRect);
	void restoreRowColors(Stripbit * stripbit);
    void swapEntry(const QString & text);
    QStringList collectValues(const QString & family, const QString & prop, QString & value);

protected:
	void nextBus(QList<ConnectorItem *> & soFar);
	QString getRowLabel();
	QString getColumnLabel();
    void makeInitialPath();
    void collectConnected(int x, int y, QList<ConnectorItem *> & connected);
    StripConnector * getStripConnector(int x, int y);
    void collectTo(QSet<ConnectorItem *> &);
    void initStripLayouts();

public:
	static QString genFZP(const QString & moduleID);
	static QString genModuleID(QMap<QString, QString> & currPropsMap);

protected:
    QList<StripConnector *> m_strips;
	QList<class BusShared *> m_buses;
	QString m_beforeCut;
    int m_x;
    int m_y;
    QString m_layout;
};

#endif
