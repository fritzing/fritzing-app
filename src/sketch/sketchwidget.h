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


#ifndef SKETCHWIDGET_H
#define SKETCHWIDGET_H

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QUndoStack>
#include <QRubberBand>
#include <QGraphicsEllipseItem>
#include <QSet>
#include <QHash>
#include <QMap>
#include <QTimer>

#include "../items/paletteitem.h"
#include "../referencemodel/referencemodel.h"
#include "../model/sketchmodel.h"
#include "../viewgeometry.h"
#include "infographicsview.h"
#include "../viewlayer.h"
#include "../utils/misc.h"
#include "../commands.h"

struct ItemCount {
	int selCount;
	int hasLabelCount;
	int visLabelCount;
	int itemsCount;
	int selRotatable;
	int sel45Rotatable;
	int selHFlipable;
	int selVFlipable;
	int obsoleteCount;
	int moveLockCount;
	int wireCount;
};

struct SwapThing {
	bool firstTime;
	long newID;
	ItemBase * itemBase;
	long newModelIndex;
	QString newModuleID;
	ViewLayer::ViewLayerPlacement viewLayerPlacement;
	QList<Wire *> wiresToDelete;
	QUndoCommand * parentCommand;
	QHash<ConnectorItem *, ChangeConnectionCommand *> reconnections;
	QHash<ConnectorItem *, Connector *> byWire;
	QHash<ConnectorItem *, ConnectorItem *> toConnectorItems;
	QHash<ConnectorItem *, Connector *> swappedGender;
	SketchWidget * bbView;
	QMap<QString, QString> propsMap;
};

struct RenderThing {
	bool selectedItems;
	double printerScale;
	bool blackOnly;
	QRectF imageRect;
	QRectF offsetRect;
	double dpi;
	bool renderBlocker;
	QRectF itemsBoundingRect;
	QGraphicsItem * board;
	bool empty;
	bool hideTerminalPoints;
};

class SizeItem : public QObject, public QGraphicsLineItem
{
	Q_OBJECT

public:
	SizeItem() = default;
	~SizeItem() = default;
};

class SketchWidget : public InfoGraphicsView
{
	Q_OBJECT
	Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor DESIGNABLE true)
	Q_PROPERTY(double ratsnestWidth READ ratsnestWidth WRITE setRatsnestWidth DESIGNABLE true)
	Q_PROPERTY(double ratsnestOpacity READ ratsnestOpacity WRITE setRatsnestOpacity DESIGNABLE true)

public:
	SketchWidget(ViewLayer::ViewID, QWidget *parent=0, int size=400, int minSize=300);
	~SketchWidget();

	void pushCommand(QUndoCommand *, QObject * signalTarget);
	class WaitPushUndoStack * undoStack();
	ItemBase * addItem(ModelPart *, ViewLayer::ViewLayerPlacement, BaseCommand::CrossViewType, const ViewGeometry &, long id, long modelIndex, AddDeleteItemCommand * originatingCommand);
	ItemBase * addItem(const QString & moduleID, ViewLayer::ViewLayerPlacement, BaseCommand::CrossViewType, const ViewGeometry &, long id, long modelIndex, AddDeleteItemCommand * originatingCommand);
	void deleteItem(long id, bool deleteModelPart, bool doEmit, bool later);
	virtual void deleteItem(ItemBase *, bool deleteModelPart, bool doEmit, bool later);
	void simpleMoveItem(long id, QPointF);
	void moveItem(long id, ViewGeometry &, bool updateRatsnest);
	void moveItem(long id, const QPointF & p, bool updateRatsnest);
	void updateWire(long id, const QString & connectorID, bool updateRatsnest);

	void rotateItem(long id, double degrees);
	void transformItem(long id, const QMatrix &);
	void flipItem(long id, Qt::Orientations orientation);
	void selectItem(long id, bool state, bool updateInfoView, bool doEmit);
	void selectItem(ItemBase * itemBase);
	void selectItems(QList<ItemBase *>);
	void selectItemsWithModuleID(ModelPart *);
	void addToSketch(QList<ModelPart *> &);
	void selectDeselectAllCommand(bool state);
	void changeWire(long fromID, QLineF line, QPointF pos, bool updateConnections, bool updateRatsnest);
	void changeLeg(long fromID, const QString & connectorID, const QPolygonF &, bool relative, const QString & why);
	void recalcLeg(long fromID, const QString & connectorID, const QPolygonF &, bool relative, bool active, const QString & why);
	void rotateLeg(long fromID, const QString & connectorID, const QPolygonF &, bool active);
	void cut();
	void copy();
	void setReferenceModel(class ReferenceModel *referenceModel);
	class ReferenceModel * referenceModel();
	void setSketchModel(SketchModel *);
	void setUndoStack(class WaitPushUndoStack *);
	void clearSelection();
	virtual void loadFromModelParts(QList<ModelPart *> & modelParts, BaseCommand::CrossViewType, QUndoCommand * parentCommand,
	                                bool offsetPaste, const QRectF * boundingRect, bool seekOutsideConnections, QList<long> & newIDs);
	void changeZ(QHash<long, RealPair * >, double (*pairAccessor)(RealPair *) );
	void sendToBack();
	void sendBackward();
	void bringForward();
	void bringToFront();
	void alignItems(Qt::Alignment);
	double fitInWindow();
	void rotateX(double degrees, bool rubberBandLegEnabled, ItemBase * originatingItem);
	void flipX(Qt::Orientations orientation, bool rubberBandLegEnabled);
	void addBendpoint(ItemBase * lastHoverEnterItem, ConnectorItem * lastHoverEnterConnectorItem, QPointF lastLocation);
	void flattenCurve(ItemBase * lastHoverEnterItem, ConnectorItem * lastHoverEnterConnectorItem, QPointF lastLocation);

	void deleteSelected(Wire *, bool minus);
	PaletteItem *getSelectedPart();

	void addViewLayer(ViewLayer *);
	void setAllLayersVisible(bool visible);
	void setLayerVisible(ViewLayer * viewLayer, bool visible, bool doChildLayers);
	void setLayerVisible(ViewLayer::ViewLayerID viewLayerID, bool visible, bool doChildLayers);
	void setLayerActive(ViewLayer * viewLayer, bool active);
	void setLayerActive(ViewLayer::ViewLayerID viewLayerID, bool active);
	bool layerIsVisible(ViewLayer::ViewLayerID);
	bool layerIsActive(ViewLayer::ViewLayerID);
	void sortAnyByZ(const QList<QGraphicsItem *> & items, QList<ItemBase *> & bases);
	void mousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	void setBackground(QColor);
	void setBackgroundColor(QColor, bool setPref);
	const QColor& background();
	QColor standardBackground();
	void setItemMenu(QMenu*);
	void setWireMenu(QMenu*);
	virtual void changeConnection(long fromID,
	                              const QString & fromConnectorID,
	                              long toID, const QString & toConnectorID,
	                              ViewLayer::ViewLayerPlacement,
	                              bool connect, bool doEmit,
	                              bool updateConnections);

	ItemCount calcItemCount();

	constexpr ViewLayer::ViewID viewID() const noexcept { return m_viewID; }
	void setViewLayerIDs(ViewLayer::ViewLayerID part, ViewLayer::ViewLayerID wire, ViewLayer::ViewLayerID connector, ViewLayer::ViewLayerID ruler, ViewLayer::ViewLayerID note);
	void stickem(long stickTargetID, long stickSourceID, bool stick);
	void stickyScoop(ItemBase * stickyOne, bool checkCurrent, CheckStickyCommand *);
	void setChainDrag(bool);
	void hoverEnterItem(QGraphicsSceneHoverEvent * event, ItemBase * item);
	void hoverLeaveItem(QGraphicsSceneHoverEvent * event, ItemBase * item);
	void hoverEnterConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem * item);
	void hoverLeaveConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem * item);
	void cleanUpWires(bool doEmit, class CleanUpWiresCommand *);

	void partLabelChanged(ItemBase *, const QString & oldText, const QString &newtext);
	void noteChanged(ItemBase *, const QString & oldText, const QString &newtext, QSizeF oldSize, QSizeF newSize);

	void setInfoViewOnHover(bool infoViewOnHover);
	virtual ItemBase * addItemAux(ModelPart *, ViewLayer::ViewLayerPlacement, const ViewGeometry &, long id, bool doConnectors, ViewLayer::ViewID, bool temporary);
	ItemBase * addItemAuxTemp(ModelPart *, ViewLayer::ViewLayerPlacement, const ViewGeometry &, long id, bool doConnectors, ViewLayer::ViewID, bool temporary);

	bool swappingEnabled(ItemBase *);

	virtual void addViewLayers();

	void changeWireColor(long wireId, const QString& color, double opacity);
	void changeWireWidth(long wireId, double width);
	void changeWireFlags(long wireId, ViewGeometry::WireFlags wireFlags);
	void setIgnoreSelectionChangeEvents(bool);
	void hideConnectors(bool hide);
	void saveLayerVisibility();
	void restoreLayerVisibility();
	void updateRoutingStatus(CleanUpWiresCommand*, RoutingStatus &, bool manual);
	void updateRoutingStatus(RoutingStatus &, bool manual);
	virtual bool hasAnyNets();
	void ensureLayerVisible(ViewLayer::ViewLayerID);

	const QString &selectedModuleID();
	virtual bool canDeleteItem(QGraphicsItem * item, int count);
	virtual bool canCopyItem(QGraphicsItem * item, int count);
	constexpr const QString & viewName() const noexcept { return m_viewName; }
	void makeDeleteItemCommand(ItemBase * itemBase, BaseCommand::CrossViewType, QUndoCommand * parentCommand);
	virtual void forwardRoutingStatus(const RoutingStatus &);

	void collectParts(QList<ItemBase *> & partList);

	void movePartLabel(long itemID, QPointF newPos, QPointF newOffset);

	void updateInfoView();
	virtual void setCurrent(bool current);
	void partLabelMoved(ItemBase *, QPointF oldPos, QPointF oldOffset, QPointF newPos, QPointF newOffset);
	void rotateFlipPartLabel(ItemBase *, double degrees, Qt::Orientations flipDirection);
	void rotateFlipPartLabel(long itemID, double degrees, Qt::Orientations flipDirection);
	void showPartLabels(bool show);
	void hidePartLabel(ItemBase * item);
	void noteSizeChanged(ItemBase * itemBase, const QSizeF & oldSize, const QSizeF & newSize);
	void resizeNote(long itemID, const QSizeF & );
	class SelectItemCommand* stackSelectionState(bool pushIt, QUndoCommand * parentCommand);
	QString renderToSVG(RenderThing &, QGraphicsItem * board, const LayerList &);
	bool spaceBarIsPressed() noexcept;
	virtual long setUpSwap(SwapThing &, bool master);
	ConnectorItem * lastHoverEnterConnectorItem();
	ItemBase * lastHoverEnterItem();
	LayerHash & viewLayers();
	virtual void createTrace(Wire*, bool useLastWireColor);
	virtual void selectAllWires(ViewGeometry::WireFlag);
	virtual void tidyWires();
	const QString & getShortName();
	virtual void setClipEnds(class ClipableWire *, bool);
	void getBendpointWidths(class Wire *, double w, double & w1, double & w2, bool & negativeOffsetRect);
	virtual bool includeSymbols();
	void disconnectAll();
	virtual bool canDisconnectAll();
	virtual bool ignoreFemale();
	virtual ViewLayer::ViewLayerID getWireViewLayerID(const ViewGeometry & viewGeometry, ViewLayer::ViewLayerPlacement);
	ItemBase * findItem(long id);
	long createWire(ConnectorItem * from, ConnectorItem * to, ViewGeometry::WireFlags, bool dontUpdate, BaseCommand::CrossViewType, QUndoCommand * parentCommand);
	QList<ItemBase *> selectAllObsolete();
	int selectAllMoveLock();
	void setMoveLock(long id, bool lock);
	bool partLabelsVisible();
	void restorePartLabel(long itemID, QDomElement & element);
	void loadLogoImage(ItemBase *, const QString & oldSvg, const QSizeF oldAspectRatio, const QString & oldFilename, const QString & newFilename, bool addName);
	void loadLogoImage(long itemID, const QString & oldSvg, const QSizeF oldAspectRatio, const QString & oldFilename);
	void loadLogoImage(long itemID, const QString & newFilename, bool addName);
	void setNoteFocus(QGraphicsItem *, bool inFocus);

	void alignToGrid(bool);
	bool alignedToGrid();
	void showGrid(bool);
	bool showingGrid();
	void setGridSize(const QString &);
	QString gridSizeText();
	void saveZoom(double);
	double retrieveZoom();
	void initGrid();
	virtual double defaultGridSizeInches();
	void clearPasteOffset();
	virtual ViewLayer::ViewLayerPlacement defaultViewLayerPlacement(ModelPart *);
	void collectAllNets(QHash<class ConnectorItem *, int> & indexer, QList< QList<class ConnectorItem *>* > & allPartConnectorItems, bool includeSingletons, bool bothSides);
	virtual bool routeBothSides();
	virtual void changeLayer(long id, double z, ViewLayer::ViewLayerID viewLayerID);
	void ratsnestConnect(ConnectorItem * connectorItem, bool connect);
	void ratsnestConnect(ItemBase *, bool connect);
	void ratsnestConnect(ConnectorItem * c1, ConnectorItem * c2, bool connect, bool wait);
	virtual void addDefaultParts();
	float getTopZ();
	QGraphicsItem * addWatermark(const QString & filename);
	void copyHeart(QList<ItemBase *> & bases, bool saveBoundingRects, QByteArray & itemData, QList<long> & modelIndexes);
	void pasteHeart(QByteArray & itemData, bool seekOutsideConnections);
	ViewGeometry::WireFlag getTraceFlag();
	void changeBus(ItemBase *, bool connec, const QString & oldBus, const QString & newBus, QList<ConnectorItem *> &, const QString & message, const QString & oldLayout, const QString & newLayout);
	const QString & filenameIf();
	void setItemDropOffset(long id, QPointF offset);
	void prepLegBendpointMove(ConnectorItem * from, int index, QPointF oldPos, QPointF newPos, ConnectorItem * to, bool changeConnections);
	void prepLegCurveChange(ConnectorItem * from, int index, const class Bezier * oldB, const class Bezier * newB, bool triggerFirstTime);
	void prepLegBendpointChange(ConnectorItem * from, int oldCount, int newCount, int index, QPointF pos, const class Bezier *, const class Bezier *, const class Bezier *, bool triggerFirstTime);
	void prepLegSelection(ItemBase *);
	void changeWireCurve(long id, const Bezier *, bool autoroutable);
	void changeLegCurve(long id, const QString & connectorID, int index, const Bezier *);
	void addLegBendpoint(long id, const QString & connectorID, int index, QPointF, const class Bezier *, const class Bezier *);
	void removeLegBendpoint(long id, const QString & connectorID, int index, const class Bezier *);
	void moveLegBendpoint(long id, const QString & connectorID, int index, QPointF);
	bool curvyWires();
	void setCurvyWires(bool);
	bool curvyWiresIndicated(Qt::KeyboardModifiers);
	void triggerRotate(ItemBase *, double degrees);
	void makeWiresChangeConnectionCommands(const QList<Wire *> & wires, QUndoCommand * parentCommand);
	void renamePins(ItemBase *, const QStringList & oldLabels, const QStringList & newLabels, bool singleRow);
	void renamePins(long itemID, const QStringList & labels, bool singleRow);
	void getRatsnestColor(QColor &);
	VirtualWire * makeOneRatsnestWire(ConnectorItem * source, ConnectorItem * dest, bool routed, QColor color, bool force);
	double ratsnestOpacity();
	void setRatsnestOpacity(double);
	double ratsnestWidth();
	void setRatsnestWidth(double);
	void setAnyInRotation();
	ConnectorItem * findConnectorItem(ConnectorItem * foreignConnectorItem);
	void setGroundFillSeed(long id, const QString & connectorID, bool seed);
	void setWireExtras(long id, QDomElement &);
	void resolveTemporary(bool, ItemBase *);
	virtual bool sameElectricalLayer2(ViewLayer::ViewLayerID, ViewLayer::ViewLayerID);
	void deleteMiddle(QSet<ItemBase *> & deletedItems, QUndoCommand * parentCommand);
	void setPasting(bool);
	void showUnrouted();
	QPointF alignOneToGrid(ItemBase * itemBase);
	void showEvent(QShowEvent * event);
	void blockUI(bool);
	void viewItemInfo(ItemBase * item);
	virtual QHash<QString, QString> getAutorouterSettings();
	virtual void setAutorouterSettings(QHash<QString, QString> &);
	void hidePartLayer(long id, ViewLayer::ViewLayerID, bool hide);
	void hidePartLayer(ItemBase *, ViewLayer::ViewLayerID, bool hide);
	void moveItem(ItemBase *, double x, double y);
	constexpr const QColor& gridColor() const noexcept { return m_gridColor; }
	void setGridColor(QColor);
	constexpr bool everZoomed() const noexcept { return m_everZoomed; }
	void setEverZoomed(bool);
	void testConnectors();
	void updateWires();
	void checkForReversedWires();

protected:
	void dragEnterEvent(QDragEnterEvent *);
	bool dragEnterEventAux(QDragEnterEvent *);
	virtual bool canDropModelPart(ModelPart *);

	void dragLeaveEvent(QDragLeaveEvent *);
	void dragMoveEvent(QDragMoveEvent *);
	void dropEvent(QDropEvent *);
	virtual void mousePressEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
	void mouseDoubleClickEvent (QMouseEvent *);
	void contextMenuEvent(QContextMenuEvent *);
	bool viewportEvent(QEvent *);
	void paintEvent(QPaintEvent *);
	virtual PaletteItem* addPartItem(ModelPart *, ViewLayer::ViewLayerPlacement, PaletteItem *, bool doConnectors, bool & ok, ViewLayer::ViewID, bool temporary);
	void clearHoldingSelectItem();
	bool startZChange(QList<ItemBase *> & bases);
	void continueZChange(QList<ItemBase *> & bases, int start, int end, bool (*test)(int current, int start), int inc, const QString & text);
	void continueZChangeMax(QList<ItemBase *> & bases, int start, int end, bool (*test)(int current, int start), int inc, const QString & text);
	void continueZChangeAux(QList<ItemBase *> & bases, const QString & text);
	virtual ViewLayer::ViewLayerID getDragWireViewLayerID(ConnectorItem *);
	ViewLayer::ViewLayerID getPartViewLayerID();
	ViewLayer::ViewLayerID getRulerViewLayerID();
	ViewLayer::ViewLayerID getConnectorViewLayerID();
	virtual ViewLayer::ViewLayerID getLabelViewLayerID(ItemBase *);
	ViewLayer::ViewLayerID getNoteViewLayerID();
	void dragMoveHighlightConnector(QPoint eventPos);

	void addToScene(ItemBase * item, ViewLayer::ViewLayerID viewLayerID);
	ConnectorItem * findConnectorItem(ItemBase * item, const QString & connectorID, ViewLayer::ViewLayerPlacement);
	bool checkMoved(bool wait);

	void changeConnectionAux(long fromID, const QString & fromConnectorID,
	                         long toID, const QString & toConnectorID,
	                         ViewLayer::ViewLayerPlacement,
	                         bool connect, bool updateConnections);

	void cutDeleteAux(QString undoStackMessage, bool minus, Wire * wire);
	void deleteAux(QSet<ItemBase *> & deletedItems, QUndoCommand * parentCommand, bool doPush);

	ChangeConnectionCommand * extendChangeConnectionCommand(BaseCommand::CrossViewType, long fromID, const QString & fromConnectorID,
	        long toID, const QString & toConnectorID,
	        ViewLayer::ViewLayerPlacement,
	        bool connect, QUndoCommand * parent);
	ChangeConnectionCommand * extendChangeConnectionCommand(BaseCommand::CrossViewType, ConnectorItem * fromConnectorItem, ConnectorItem * toConnectorItem,
	        ViewLayer::ViewLayerPlacement,
	        bool connect, QUndoCommand * parentCommand);


	void keyPressEvent(QKeyEvent *);
	void keyReleaseEvent(QKeyEvent *);
	void clearTemporaries();
	void dragWireChanged(class Wire* wire, ConnectorItem * from, ConnectorItem * to);
	void dragRatsnestChanged();
	void killDroppingItem();
	ViewLayer::ViewLayerID getViewLayerID(ModelPart *, ViewLayer::ViewID, ViewLayer::ViewLayerPlacement);
	ItemBase * overSticky(ItemBase *);
	virtual void setNewPartVisible(ItemBase *);
	virtual bool collectFemaleConnectees(ItemBase *, QSet<ItemBase *> &);
	virtual bool checkUnder();
	virtual void findConnectorsUnder(ItemBase * item);

	bool currentlyInfoviewed(ItemBase *item);
	void resizeEvent(QResizeEvent *);

	void addViewLayersAux(const LayerList &, const LayerList & layersFromBelow, float startZ = 1.5);
	virtual bool disconnectFromFemale(ItemBase * item, QHash<long, ItemBase *> & savedItems, ConnectorPairHash &, bool doCommand, bool rubberBandLegEnabled, QUndoCommand * parentCommand);
	void clearDragWireTempCommand();
	bool draggingWireEnd();
	void moveItems(QPoint globalPos, bool checkAutoScroll, bool rubberBandLegEnabled);
	virtual ViewLayer::ViewLayerID multiLayerGetViewLayerID(ModelPart * modelPart, ViewLayer::ViewID, ViewLayer::ViewLayerPlacement, LayerList &);
	virtual BaseCommand::CrossViewType wireSplitCrossView();
	virtual bool canChainMultiple();
	virtual bool canChainWire(Wire *);
	virtual bool canDragWire(Wire *);
	virtual bool canCreateWire(Wire * dragWire, ConnectorItem * from, ConnectorItem * to);
	//virtual bool modifyNewWireConnections(Wire * dragWire, ConnectorItem * fromOnWire, ConnectorItem * from, ConnectorItem * to, QUndoCommand * parentCommand);
	virtual void setUpColor(ConnectorItem * fromConnectorItem, ConnectorItem * toConnectorItem, Wire * wire, QUndoCommand * parentCommand);
	void setupAutoscroll(bool moving);
	void turnOffAutoscroll();
	bool checkAutoscroll(QPoint globalPos);
	virtual void setWireVisible(Wire *);
	bool matchesLayer(ModelPart * modelPart);

	QByteArray removeOutsideConnections(const QByteArray & itemData, QList<long> & modelIndexes);
	void addWireExtras(long newID, QDomElement & view, QUndoCommand * parentCommand);
	virtual const QString & hoverEnterWireConnectorMessage(QGraphicsSceneHoverEvent * event, ConnectorItem * item);
	virtual const QString & hoverEnterPartConnectorMessage(QGraphicsSceneHoverEvent * event, ConnectorItem * item);
	void partLabelChangedAux(ItemBase * pitem,const QString & oldText, const QString &newText);
	void drawBackground( QPainter * painter, const QRectF & rect );
	void handleConnect(QDomElement & connect, ModelPart *, const QString & fromConnectorID, ViewLayer::ViewLayerID, QStringList & alreadyConnected,
	                   QHash<long, ItemBase *> & newItems, QUndoCommand * parentCommand, bool seekOutsideConnections);
	void setUpSwapReconnect(SwapThing &, ItemBase * itemBase, long newID, bool master);
	void makeSwapWire(SketchWidget *, ItemBase *, long newID, ConnectorItem * fromConnectorItem, ConnectorItem * toConnectorItem, Connector * newConnector, QUndoCommand * parentCommand);
	bool swappedGender(ConnectorItem * originalConnectorItem, Connector * newConnector);
	void setLastPaletteItemSelected(PaletteItem * paletteItem);
	void setLastPaletteItemSelectedIf(ItemBase * itemBase);
	void prepDragBendpoint(Wire *, QPoint eventPos, bool dragCurve);
	void prepDragWire(Wire *);
	void clickBackground(QMouseEvent *);
	void categorizeDragWires(QSet<Wire *> & wires, QList<ItemBase *> & freeWires);
	void categorizeDragLegs(bool rubberBandLegEnabled);
	void prepMove(ItemBase * originatingItem, bool rubberBandLegEnabled, bool includeRatsnest);
	void initBackgroundColor();
	QPointF calcNewLoc(ItemBase * moveBase, ItemBase * detachFrom);
	long findPartOrWire(long itemID);
	AddItemCommand * newAddItemCommand(BaseCommand::CrossViewType crossViewType, ModelPart *,
	                                   QString moduleID, ViewLayer::ViewLayerPlacement, ViewGeometry & viewGeometry, qint64 id,
	                                   bool updateInfoView, long modelIndex, bool addSubparts, QUndoCommand *parent);
	int selectAllItems(QSet<ItemBase *> & itemBases, const QString & msg);
	bool moveByArrow(double dx, double dy, QKeyEvent * );
	double gridSizeInches();
	virtual bool canAlignToTopLeft(ItemBase *);
	virtual bool canAlignToCenter(ItemBase *);
	virtual void findAlignmentAnchor(ItemBase * originatingItem, QHash<long, ItemBase *> & savedItems, QHash<Wire *, ConnectorItem *> & savedWires);
	void alignLoc(QPointF & loc, const QPointF startPoint, const QPointF newLoc, const QPointF originalLoc);
	void copyAux(QList<ItemBase *> & bases, bool saveBoundingRects);
	void copyDrop();
	void dropItemEvent(QDropEvent *event);
	virtual QString checkDroppedModuleID(const QString & moduleID);
	QString makeWireSVG(Wire * wire, QPointF offset, double dpi, double printerscale, bool blackOnly);
	QString makeWireSVGAux(Wire * wire, double width, const QString & color, QPointF offset, double dpi, double printerScale, bool blackOnly, bool dashed);

	QString makeMoveSVG(double printerScale, double dpi, QPointF & offset);
	void prepDeleteProps(ItemBase * itemBase, long id, const QString & newModuleID, QMap<QString, QString> & propsMap, QUndoCommand * parentCommand);
	void prepDeleteOtherProps(ItemBase * itemBase, long id, const QString & newModuleID, QMap<QString, QString> & propsMap, QUndoCommand * parentCommand);
	virtual ViewLayer::ViewLayerPlacement getViewLayerPlacement(ModelPart *, QDomElement & instance, QDomElement & view, ViewGeometry &);
	virtual ViewLayer::ViewLayerPlacement wireViewLayerPlacement(ConnectorItem *);

	virtual bool resizingJumperItemPress(ItemBase *);
	virtual bool resizingJumperItemRelease();
	bool resizingBoardPress(ItemBase *);
	bool resizingBoardRelease();
	void resizeBoard();
	void resizeWithHandle(ItemBase * itemBase, double mmW, double mmH);
	virtual bool acceptsTrace(const ViewGeometry &);
	virtual ItemBase * placePartDroppedInOtherView(ModelPart *, ViewLayer::ViewLayerPlacement, const ViewGeometry & viewGeometry, long id, SketchWidget * dropOrigin);
	void showPartLabelsAux(bool show, QList<ItemBase *> & itemBases);
	virtual void extraRenderSvgStep(ItemBase *, QPointF offset, double dpi, double printerScale, QString & outputSvg);
	virtual ViewLayer::ViewLayerPlacement createWireViewLayerPlacement(ConnectorItem * from, ConnectorItem * to);
	virtual Wire * createTempWireForDragging(Wire * fromWire, ModelPart * wireModel, ConnectorItem * connectorItem, ViewGeometry & viewGeometry, ViewLayer::ViewLayerPlacement);
	virtual void prereleaseTempWireForDragging(Wire*);
	void checkFit(ModelPart * newModelPart, ItemBase * itemBase, long newID,
	              QHash<ConnectorItem *, Connector *> & found, QList<ConnectorItem *> & notFound,
	              QHash<ConnectorItem *, ConnectorItem *> & m2f, QHash<ConnectorItem *, Connector *> & byWire,
	              QHash<QString, QPolygonF> & legs, 	QHash<QString, ConnectorItem *> & formerLegs, QUndoCommand * parentCommand);
	void checkFitAux(ItemBase * tempItemBase, ItemBase * itemBase, long newID,
	                 QHash<ConnectorItem *, Connector *> & found, QList<ConnectorItem *> & notFound,
	                 QHash<ConnectorItem *, ConnectorItem *> & m2f, QHash<ConnectorItem *, Connector *> & byWire,
	                 QHash<QString, QPolygonF> & legs, QHash<QString, ConnectorItem *> & formerLegs, QUndoCommand * parentCommand);
	void changeLegAux(long fromID, const QString & fromConnectorID, const QPolygonF &, bool reset, bool relative, bool active, const QString & why);
	void moveLegBendpoints(bool undoOnly, QUndoCommand * parentCommand);
	void moveLegBendpointsAux(ConnectorItem * connectorItem, bool undoOnly, QUndoCommand * parentCommand);
	virtual void rotatePartLabels(double degrees, QTransform &, QPointF center, QUndoCommand * parentCommand);
	bool checkUpdateRatsnest(QList<ConnectorItem *> & connectorItems);
	void makeRatsnestViewGeometry(ViewGeometry & viewGeometry, ConnectorItem * source, ConnectorItem * dest);
	virtual double getTraceWidth();
	virtual const QString & traceColor(ViewLayer::ViewLayerPlacement);
	void createTrace(Wire * fromWire, const QString & commandString, ViewGeometry::WireFlag, bool useLastWireColor);
	bool createOneTrace(Wire * wire, ViewGeometry::WireFlag flag, bool allowAny, QList<Wire *> & done, bool useLastWireColor, QUndoCommand * parentCommand);
	void removeWire(Wire * w, QList<ConnectorItem *> & ends, QList<Wire *> & done, QUndoCommand * parentCommand);
	void selectAllWiresFrom(ViewGeometry::WireFlag flag, QList<QGraphicsItem *> & items);
	bool canConnect(ItemBase * from, ItemBase * to);
	virtual bool canConnect(Wire * from, ItemBase * to);
	void removeDragWire();
	QGraphicsItem * getClickedItem(QList<QGraphicsItem *> & items);
	void cleanupRatsnests(QList< QPointer<ConnectorItem> > & connectorItems, bool connect);
	void rotateWire(Wire *, QTransform & rotation, QPointF center, bool undoOnly, QUndoCommand * parentCommand);
	QString renderToSVG(RenderThing &, const LayerList &);
	QString renderToSVG(RenderThing &, QList<QGraphicsItem *> & itemsAndLabels);
	QList<ItemBase *> collectSuperSubs(ItemBase *);
	void squashShapes(QPointF scenePos);
	void unsquashShapes();
	virtual bool updateOK(ConnectorItem *, ConnectorItem *);
	virtual void viewGeometryConversionHack(ViewGeometry &, ModelPart *);

protected:
	static bool lessThan(int a, int b);
	static bool greaterThan(int a, int b);

signals:
	void itemAddedSignal(ModelPart *, ItemBase *, ViewLayer::ViewLayerPlacement, const ViewGeometry &, long id, SketchWidget * dropOrigin);
	void itemDeletedSignal(long id);
	void clearSelectionSignal();
	void itemSelectedSignal(long id, bool state);
	void itemMovedSignal(ItemBase *);
	void wireDisconnectedSignal(long fromID, QString fromConnectorID);
	void wireConnectedSignal(long fromID,  QString fromConnectorID, long toID, QString toConnectorID);
	void changeConnectionSignal(long fromID, QString fromConnectorID,
	                            long toID, QString toConnectorID,
	                            ViewLayer::ViewLayerPlacement,
	                            bool connect, bool updateConnections);
	void copyBoundingRectsSignal(QHash<QString, QRectF> &);
	void cleanUpWiresSignal(CleanUpWiresCommand *);
	void selectionChangedSignal();

	void resizeSignal();
	void dropSignal(const QPoint &pos);
	void routingStatusSignal(SketchWidget *, const RoutingStatus &);
	void movingSignal(SketchWidget *, QUndoCommand * parentCommand);
	void selectAllItemsSignal(bool state, bool doEmit);
	void checkStickySignal(long id, bool doEmit, bool checkCurrent, CheckStickyCommand *);
	void disconnectAllSignal(QList<ConnectorItem *>, QHash<ItemBase *, SketchWidget *> & itemsToDelete, QUndoCommand * parentCommand);
	void setResistanceSignal(long itemID, QString resistance, QString pinSpacing, bool doEmit);
	void setPropSignal(long itemID, const QString & prop, const QString & value, bool doRedraw, bool doEmit);
	void setInstanceTitleSignal(long id, const QString & oldTitle, const QString & newTitle, bool isUndoable, bool doEmit);
	void statusMessageSignal(QString, int timeout);
	void showLabelFirstTimeSignal(long itemID, bool show, bool doEmit);
	void dropPasteSignal(SketchWidget *);
	void changeBoardLayersSignal(int, bool doEmit);
	void deleteTracesSignal(QSet<ItemBase *> & deletedItems, QHash<ItemBase *, SketchWidget *> & otherDeletedItems, QList<long> & deletedIDs, bool isForeign, QUndoCommand * parentCommand);
	void makeDeleteItemCommandPrepSignal(ItemBase * itemBase, bool foreign, QUndoCommand * parentCommand);
	void makeDeleteItemCommandFinalSignal(ItemBase * itemBase, bool foreign, QUndoCommand * parentCommand);
	void cursorLocationSignal(double xinches, double yinches, double width=0.0, double height=0.0);
	void ratsnestConnectSignal(long id, const QString & connectorID, bool connect, bool doEmit);
	void updatePartLabelInstanceTitleSignal(long itemID);
	void filenameIfSignal(QString & filename);
	void collectRatsnestSignal(QList<SketchWidget *> & foreignSketchWidgets);
	void removeRatsnestSignal(QList<struct ConnectorEdge *> & cutSet, QUndoCommand * parentCommand);
	void updateLayerMenuSignal();
	void swapBoardImageSignal(SketchWidget * sketchWidget, ItemBase * itemBase, const QString & filename, const QString & moduleID, bool addName);
	void canConnectSignal(Wire * from, ItemBase * to, bool & connect);
	void swapStartSignal(SwapThing & swapThing, bool master);
	void showing(SketchWidget *);
	void clickedItemCandidateSignal(QGraphicsItem *, bool & ok);
	void resizedSignal(ItemBase *);
	void cleanupRatsnestsSignal(bool doEmit);
	void addSubpartSignal(long id, long subpartID, bool doEmit);
	void getDroppedItemViewLayerPlacementSignal(ModelPart * modelPart, ViewLayer::ViewLayerPlacement &);
	void packItemsSignal(int columns, const QList<long> & ids, QUndoCommand *parent, bool doEmit);

protected slots:
	void itemAddedSlot(ModelPart *, ItemBase *, ViewLayer::ViewLayerPlacement, const ViewGeometry &, long id, SketchWidget * dropOrigin);
	void itemDeletedSlot(long id);
	void clearSelectionSlot();
	void itemSelectedSlot(long id, bool state);
	void selectionChangedSlot();
	void wireChangedSlot(class Wire*, const QLineF & oldLine, const QLineF & newLine, QPointF oldPos, QPointF newPos, ConnectorItem * from, ConnectorItem * to);
	void wireChangedCurveSlot(class Wire*, const Bezier * oldB, const Bezier * newB, bool triggerFirstTime);
	virtual void wireSplitSlot(class Wire*, QPointF newPos, QPointF oldPos, const QLineF & oldLine);
	void wireJoinSlot(class Wire*, ConnectorItem * clickedConnectorItem);
	void toggleLayerVisibility();
	void wireConnectedSlot(long fromID, QString fromConnectorID, long toID, QString toConnectorID);
	void wireDisconnectedSlot(long fromID, QString fromConnectorID);
	void changeConnectionSlot(long fromID, QString fromConnectorID, long toID, QString toConnectorID, ViewLayer::ViewLayerPlacement, bool connect, bool updateConnections);
	void restartPasteCount();
	void dragIsDoneSlot(class ItemDrag *);
	void statusMessage(QString message, int timeout = 0);
	void cleanUpWiresSlot(CleanUpWiresCommand *);
	void updateInfoViewSlot();
	void spaceBarIsPressedSlot(bool);
	void autoScrollTimeout();
	void dragAutoScrollTimeout();
	void moveAutoScrollTimeout();
	void rememberSticky(long id, QUndoCommand * parentCommand);
	void rememberSticky(ItemBase *, QUndoCommand * parentCommand);
	void copyBoundingRectsSlot(QHash<QString, QRectF> &);
	void deleteRatsnest(Wire *, QUndoCommand * parentCommand);
	void deleteTracesSlot(QSet<ItemBase *> & deletedItems, QHash<ItemBase *, SketchWidget *> & otherDeletedItems, QList<long> & deletedIDs, bool isForeign, QUndoCommand * parentCommand);
	void vScrollToZero();
	void arrowTimerTimeout();
	void makeDeleteItemCommandPrepSlot(ItemBase * itemBase, bool foreign, QUndoCommand * parentCommand);
	void makeDeleteItemCommandFinalSlot(ItemBase * itemBase, bool foreign, QUndoCommand * parentCommand);
	void updatePartLabelInstanceTitleSlot(long itemID);
	void changePinLabelsSlot(ItemBase * itemBase, bool singleRow);
	void changePinLabels(ItemBase *, bool singleRow);
	void collectRatsnestSlot(QList<SketchWidget *> & foreignSketchWidgets);
	void removeRatsnestSlot(QList<struct ConnectorEdge *> & cutSet, QUndoCommand * parentCommand);
	void deleteTemporary();
	void canConnect(Wire * from, ItemBase * to, bool & connect);
	long swapStart(SwapThing & swapThing, bool master);
	virtual void getDroppedItemViewLayerPlacement(ModelPart * modelPart, ViewLayer::ViewLayerPlacement &);

public slots:
	void changeWireColor(const QString newColor);
	void changeWireWidthMils(const QString newWidth);
	void selectAllItems(bool state, bool doEmit);
	void setNoteText(long itemID, const QString & newText);
	void setInstanceTitle(long id, const QString & oldTitle, const QString & newTitle, bool isUndoable, bool doEmit);
	void incInstanceTitle(long id);
	void showPartLabel(long id, bool showIt);
	void checkSticky(long id, bool doEmit, bool checkCurrent, CheckStickyCommand *);
	virtual ItemBase * resizeBoard(long id, double w, double h);
	void resizeJumperItem(long id, QPointF pos, QPointF c0, QPointF c1);
	void disconnectAllSlot(QList<ConnectorItem *>, QHash<ItemBase *, SketchWidget *> & itemsToDelete, QUndoCommand * parentCommand);
	void setResistance(long itemID, QString resistance, QString pinSpacing, bool doEmit);
	void setResistance(QString resistance, QString pinSpacing);
	void setProp(long itemID, const QString & prop, const QString & value, bool redraw, bool doEmit);
	virtual void setProp(ItemBase *, const QString & propName, const QString & translatedPropName, const QString & oldValue, const QString & newValue, bool redraw);
	void setHoleSize(ItemBase *, const QString & propName, const QString & translatedPropName, const QString & oldValue, const QString & newValue, QRectF & oldRect, QRectF & newRect, bool redraw);
	virtual void showLabelFirstTime(long itemID, bool show, bool doEmit);
	void resizeBoard(double w, double h, bool doEmit);
	virtual void changeBoardLayers(int layers, bool doEmit);
	void updateConnectors();
	void ratsnestConnect(long id, const QString & connectorID, bool connect, bool doEmit);
	void cleanupRatsnests(bool doEmit);
	void addSubpart(long id, long subpartid, bool doEmit);
	void packItems(int columns, const QList<long> & ids, QUndoCommand *parent, bool doEmit);

protected:
	enum StatusConnectStatus {
		StatusConnectNotTried,
		StatusConnectSucceeded,
		StatusConnectFailed
	};

protected:
	QPointer<class ReferenceModel> m_referenceModel;
	QPointer<SketchModel> m_sketchModel;
	ViewLayer::ViewID m_viewID;
	class WaitPushUndoStack * m_undoStack = nullptr;
	class SelectItemCommand * m_holdingSelectItemCommand = nullptr;
	class SelectItemCommand * m_tempDragWireCommand = nullptr;
	LayerHash m_viewLayers;
	QHash<ViewLayer::ViewLayerID, bool> m_viewLayerVisibility;
	QPointer<Wire> m_connectorDragWire;
	QPointer<Wire> m_bendpointWire;
	ViewGeometry m_bendpointVG;
	QPointer<ConnectorItem> m_connectorDragConnector;
	bool m_droppingWire = false;
	QPointF m_droppingOffset;
	QPointer<ItemBase> m_droppingItem;
	int m_moveEventCount = 0;
	//QList<QGraphicsItem *> m_lastSelected;  hack for 4.5.something
	ViewLayer::ViewLayerID m_wireViewLayerID;
	ViewLayer::ViewLayerID m_partViewLayerID;
	ViewLayer::ViewLayerID m_rulerViewLayerID;
	ViewLayer::ViewLayerID m_connectorViewLayerID;
	ViewLayer::ViewLayerID m_noteViewLayerID;
	QList<QGraphicsItem *> m_temporaries;
	bool m_chainDrag = false;
	QPointF m_mousePressScenePos;
	QPointF m_mousePressGlobalPos;
	QTimer m_autoScrollTimer;
	volatile int m_autoScrollX = 0;
	volatile int m_autoScrollY = 0;
	volatile int m_autoScrollCount = 0;
	QPoint m_globalPos;

	QPointer<PaletteItem> m_lastPaletteItemSelected;

	int m_pasteCount = 0;
	QPointF m_pasteOffset;

	// Part Menu
	QMenu *m_itemMenu = nullptr;
	QMenu *m_wireMenu = nullptr;

	bool m_infoViewOnHover;

	QHash<long, ItemBase *> m_savedItems;
	QHash<Wire *, ConnectorItem *> m_savedWires;
	QList<ItemBase *> m_additionalSavedItems;
	int m_ignoreSelectionChangeEvents = 0;
	bool m_current = false;

	QString m_lastColorSelected;

	ConnectorPairHash m_moveDisconnectedFromFemale;
	bool m_spaceBarIsPressed = false;
	bool m_spaceBarWasPressed = false;

	QPointer<ConnectorItem> m_lastHoverEnterConnectorItem;
	QPointer<ItemBase> m_lastHoverEnterItem;
	QString m_shortName;
	QPointer<Wire> m_dragBendpointWire;
	bool m_dragCurve = false;
	QPoint m_dragBendpointPos;
	StatusConnectStatus m_statusConnectState = StatusConnectNotTried;
	QList<QGraphicsItem *> m_inFocus;
	QString m_viewName;
	bool m_movingByArrow = false;
	double m_arrowTotalX = 0.0;
	double m_arrowTotalY = 0.0;
	bool m_movingByMouse = false;
	bool m_alignToGrid = true;
	bool m_showGrid = true;
	double m_gridSizeInches = 0.0;
	QString m_gridSizeText;
	QPointer<ItemBase> m_alignmentItem;
	QPointer<ItemBase> m_originatingItem;
	QPointF m_alignmentStartPoint;
	double m_zoom = 100;
	bool m_draggingBendpoint = false;
	QPointer<SizeItem> m_sizeItem;
	int m_autoScrollThreshold = 0;
	bool m_clearSceneRect = false;
	QPointer<ItemBase> m_moveReferenceItem;
	QPointer<QSvgRenderer> m_movingSVGRenderer;
	QPointF m_movingSVGOffset;
	QPointer<QGraphicsSvgItem> m_movingItem;
	QList< QPointer<ConnectorItem> > m_ratsnestUpdateDisconnect;
	QList< QPointer<ConnectorItem> > m_ratsnestUpdateConnect;
	QList< QPointer<ConnectorItem> > m_ratsnestCacheDisconnect;
	QList< QPointer<ConnectorItem> > m_ratsnestCacheConnect;
	QList <ItemBase *> m_checkUnder;
	bool m_addDefaultParts = false;
	QPointer<ItemBase> m_addedDefaultPart;
	float m_z;
	QTimer m_arrowTimer;
	bool m_middleMouseIsPressed = false;
	QMultiHash<ItemBase *, ConnectorItem *> m_stretchingLegs;
	bool m_curvyWires = false;
	bool m_rubberBandLegWasEnabled = false;
	RoutingStatus m_routingStatus;
	bool m_anyInRotation;
	bool m_pasting = false;
	QPointer<class ResizableBoard> m_resizingBoard;
	QList< QPointer<ItemBase> > m_squashShapes;
	QColor m_gridColor;
	bool m_everZoomed = false;
	double m_ratsnestOpacity = 0.0;
	double m_ratsnestWidth = 0.0;

public:
	static ViewLayer::ViewLayerID defaultConnectorLayer(ViewLayer::ViewID viewId);
	static constexpr int PropChangeDelay = 100;
	static bool m_blockUI;

protected:
	static constexpr int MoveAutoScrollThreshold = 5;
	static constexpr int DragAutoScrollThreshold = 10;
};

#endif
