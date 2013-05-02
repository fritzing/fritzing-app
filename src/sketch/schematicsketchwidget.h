/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2013 Fachhochschule Potsdam - http://fh-potsdam.de

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

$Revision: 6979 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-21 23:20:35 +0200 (So, 21. Apr 2013) $

********************************************************************/


#ifndef SCHEMATICSKETCHWIDGET_H
#define SCHEMATICSKETCHWIDGET_H

#include "pcbsketchwidget.h"

class SchematicSketchWidget : public PCBSketchWidget
{
	Q_OBJECT

public:
    SchematicSketchWidget(ViewLayer::ViewID, QWidget *parent=0);

	void addViewLayers();
	ViewLayer::ViewLayerID getWireViewLayerID(const ViewGeometry & viewGeometry, ViewLayer::ViewLayerSpec);
	ViewLayer::ViewLayerID getDragWireViewLayerID(ConnectorItem *);
	void initWire(Wire *, int penWidth);
	bool autorouteTypePCB();
	double getKeepout();
	void tidyWires();
	void ensureTraceLayersVisible();
	void ensureTraceLayerVisible();
	void setClipEnds(ClipableWire * vw, bool);
	void getBendpointWidths(class Wire *, double w, double & w1, double & w2, bool & negativeOffsetRect);
	void getLabelFont(QFont &, QColor &, ViewLayer::ViewLayerSpec);
	void setNewPartVisible(ItemBase *);
	bool canDropModelPart(ModelPart * modelPart); 
	bool includeSymbols();
	bool hasBigDots();
	void changeConnection(long fromID,
						  const QString & fromConnectorID,
						  long toID, const QString & toConnectorID,
						  ViewLayer::ViewLayerSpec,
						  bool connect, bool doEmit, 
						  bool updateConnections);
	double defaultGridSizeInches();
	const QString & traceColor(ConnectorItem * forColor);
	const QString & traceColor(ViewLayer::ViewLayerSpec);
	bool isInLayers(ConnectorItem *, ViewLayer::ViewLayerSpec);
	bool routeBothSides();
	void addDefaultParts();
	bool sameElectricalLayer2(ViewLayer::ViewLayerID, ViewLayer::ViewLayerID);
	bool acceptsTrace(const ViewGeometry &);
	ViewGeometry::WireFlag getTraceFlag();
	double getTraceWidth();
	double getAutorouterTraceWidth();
	QString generateCopperFillUnit(ItemBase * itemBase, QPointF whereToStart);
	double getWireStrokeWidth(Wire *, double wireWidth);
	Wire * createTempWireForDragging(Wire * fromWire, ModelPart * wireModel, ConnectorItem * connectorItem, ViewGeometry & viewGeometry, ViewLayer::ViewLayerSpec);
	void rotatePartLabels(double degrees, QTransform &, QPointF center, QUndoCommand * parentCommand);
	void loadFromModelParts(QList<ModelPart *> & modelParts, BaseCommand::CrossViewType, QUndoCommand * parentCommand, 
							bool offsetPaste, const QRectF * boundingRect, bool seekOutsideConnections, QList<long> & newIDs);
    LayerList routingLayers(ViewLayer::ViewLayerSpec);
    bool attachedToTopLayer(ConnectorItem *);
    bool attachedToBottomLayer(ConnectorItem *);
    QSizeF jumperItemSize();
    QHash<QString, QString> getAutorouterSettings();
    void setAutorouterSettings(QHash<QString, QString> &);

public slots:
	void setVoltage(double voltage, bool doEmit);
	void setProp(ItemBase *, const QString & propName, const QString & translatedPropName, const QString & oldValue, const QString & newValue, bool redraw);
	void setInstanceTitle(long id, const QString & oldTitle, const QString & newTitle, bool isUndoable, bool doEmit);

protected slots:
	void updateBigDots();
    void getDroppedItemViewLayerSpec(ModelPart * modelPart, ViewLayer::ViewLayerSpec &);

protected:
	double getRatsnestOpacity();
	double getRatsnestWidth();
	ViewLayer::ViewLayerID getLabelViewLayerID(ViewLayer::ViewLayerSpec);
	QPoint calcFixedToCenterItemOffset(const QRect & viewPortRect, const QSizeF & helpSize);
	void extraRenderSvgStep(ItemBase *, QPointF offset, double dpi, double printerScale, QString & outputSvg);
	QString makeCircleSVG(QPointF p, double r, QPointF offset, double dpi, double printerScale);
	ViewLayer::ViewLayerSpec createWireViewLayerSpec(ConnectorItem * from, ConnectorItem * to);
    void selectAllWires(ViewGeometry::WireFlag flag);
    bool canConnect(Wire * from, ItemBase * to);
    QString checkDroppedModuleID(const QString & moduleID);

protected:
	QTimer m_updateDotsTimer;

    static QSizeF m_jumperItemSize;

};

#endif
