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

$Revision: 6715 $:
$Author: irascibl@gmail.com $:
$Date: 2012-12-17 11:27:32 +0100 (Mo, 17. Dez 2012) $

********************************************************************/

#ifndef INFOGRAPHICSVIEW_H
#define INFOGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QMenu>
#include <QHash>
#include <QList>

#include "../items/itembase.h"
#include "zoomablegraphicsview.h"

class InfoGraphicsView : public ZoomableGraphicsView
{
	Q_OBJECT

public:
	InfoGraphicsView(QWidget* parent = 0);

	virtual void viewItemInfo(ItemBase *);
	virtual void hoverEnterItem(QGraphicsSceneHoverEvent * event, ItemBase *);
	virtual void hoverLeaveItem(QGraphicsSceneHoverEvent * event, ItemBase *);
    void updateRotation(ItemBase *);

	virtual bool swappingEnabled(ItemBase *) = 0;

	virtual void hoverEnterConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem *);
	virtual void hoverLeaveConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem *);

	void setInfoView(class HtmlInfoView *);
	class HtmlInfoView * infoView();

	virtual void mousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);

	
	virtual void hidePartLabel(ItemBase * item);
	virtual void partLabelMoved(ItemBase *, QPointF oldPos, QPointF oldOffset, QPointF newPos, QPointF newOffset);
	virtual void rotateFlipPartLabel(ItemBase *, double degrees, Qt::Orientations flipDirection);
    virtual void noteSizeChanged(ItemBase * itemBase, const QSizeF & oldSize, const QSizeF & newSize);

	virtual bool spaceBarIsPressed(); 
	virtual void initWire(class Wire *, int penWidth);

	virtual void setIgnoreSelectionChangeEvents(bool) {}
	virtual void getBendpointWidths(class Wire *, double w, double & w1, double & w2, bool & negativeOffsetRect);
	virtual void getLabelFont(QFont &, QColor &, ItemBase *);
    virtual double getLabelFontSizeTiny();
	virtual double getLabelFontSizeSmall();
	virtual double getLabelFontSizeMedium();
	virtual double getLabelFontSizeLarge();
	virtual bool hasBigDots();

	virtual LayerHash & viewLayers();
	virtual void loadLogoImage(ItemBase *, const QString & oldSvg, const QSizeF oldAspectRatio, const QString & oldFilename, const QString & newFilename, bool addName);

	virtual void setNoteFocus(QGraphicsItem *, bool inFocus);

	int boardLayers();
	virtual void setBoardLayers(int, bool redraw);
	virtual class VirtualWire * makeOneRatsnestWire(ConnectorItem * source, ConnectorItem * dest, bool routed, QColor, bool force);
	virtual void getRatsnestColor(QColor &);

	virtual void changeBus(ItemBase *, bool connect, const QString & oldBus, const QString & newBus, QList<ConnectorItem *> &, const QString & message, const QString & oldLayout, const QString & newLayout);
	virtual const QString & filenameIf();
	virtual QString generateCopperFillUnit(ItemBase * itemBase, QPointF whereToStart);
	virtual void prepLegBendpointMove(ConnectorItem * from, int index, QPointF oldPos, QPointF newPos, ConnectorItem * to, bool changeConnections);
	virtual void prepLegCurveChange(ConnectorItem * from, int index, const class Bezier * oldB, const class Bezier * newB, bool triggerFirstTime);
	virtual void prepLegBendpointChange(ConnectorItem * from, int oldCount, int newCount, int index, QPointF pos, const class Bezier *, const class Bezier *, const class Bezier *, bool triggerFirstTime);
	virtual void prepLegSelection(ItemBase *);
	virtual double getWireStrokeWidth(Wire *, double wireWidth);
	virtual bool curvyWiresIndicated(Qt::KeyboardModifiers);
	virtual void triggerRotate(ItemBase *, double degrees);
	virtual void changePinLabels(ItemBase *, bool singleRow);
	virtual void renamePins(ItemBase *, const QStringList & oldLabels, const QStringList & newLabels, bool singleRow);
	virtual ViewGeometry::WireFlag getTraceFlag();
	virtual void setAnyInRotation();

	virtual void partLabelChanged(ItemBase *, const QString &oldText, const QString & newText);
	virtual void noteChanged(ItemBase *, const QString &oldText, const QString & newText, QSizeF oldSize, QSizeF newSize);
	virtual void setResistance(QString resistance, QString pinSpacing);
	virtual void setProp(ItemBase *, const QString & propName, const QString & translatedPropName, const QString & oldValue, const QString & newValue, bool redraw);
	virtual void setHoleSize(ItemBase *, const QString & propName, const QString & translatedPropName, const QString & oldValue, const QString & newValue, QRectF & oldRect, QRectF & newRect, bool redraw);
	virtual void changeWireWidthMils(const QString newWidth);
	virtual void changeWireColor(const QString newColor);
	virtual void swap(const QString & family, const QString & prop, QMap<QString, QString> & propsMap, ItemBase *);
    virtual void resolveTemporary(bool, ItemBase *);
	virtual void newWire(Wire *);

	void setActiveWire(Wire *);
	void setActiveConnectorItem(ConnectorItem *);
    void setSMDOrientation(Qt::Orientations);
    Qt::Orientations smdOrientation();
    virtual void moveItem(ItemBase *, double x, double y);
	virtual void rotateX(double degrees, bool rubberBandLegEnabled, ItemBase * originatingItem);

public slots:
	virtual void setVoltage(double, bool doEmit);
	virtual void resizeBoard(double w, double h, bool doEmit);
	virtual void setInstanceTitle(long id, const QString & oldTitle, const QString & newTitle, bool isUndoable, bool doEmit);

signals:
	void setVoltageSignal(double, bool doEmit);
	void swapSignal(const QString & family, const QString & prop, QMap<QString, QString> & propsMap, ItemBase *);
	void changePinLabelsSignal(ItemBase *, bool singleRow);
	void setActiveWireSignal(Wire *);
	void setActiveConnectorItemSignal(ConnectorItem *);
	void newWireSignal(Wire *);

public:
	static InfoGraphicsView * getInfoGraphicsView(QGraphicsItem *);

protected:
	QGraphicsItem *selectedAux();
	class HtmlInfoView *m_infoView;
	int m_boardLayers;
	bool m_hoverEnterMode;
	bool m_hoverEnterConnectorMode;
    Qt::Orientations m_smdOrientation;
};

#endif
