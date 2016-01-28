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

You should have received a copy of the GNU General Public Licensetriple
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 6998 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-28 13:51:10 +0200 (So, 28. Apr 2013) $

********************************************************************/

#ifndef ITEMBASE_H
#define ITEMBASE_H

#include <QXmlStreamWriter>
#include <QPointF>
#include <QSize>
#include <QHash>
#include <QList>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSvgItem>
#include <QPointer>
#include <QUrl>
#include <QMap>
#include <QTimer>
#include <QCursor>

#include "../viewgeometry.h"
#include "../viewlayer.h"
#include "../utils/misc.h"

class ConnectorItem;

typedef QMultiHash<ConnectorItem *, ConnectorItem *> ConnectorPairHash;

typedef bool (*SkipCheckFunction)(ConnectorItem *);

class ItemBase : public QGraphicsSvgItem
{
Q_OBJECT

public:
	enum PluralType {
		Singular,
		Plural,
		NotSure
	};

public:
	ItemBase(class ModelPart*, ViewLayer::ViewID, const ViewGeometry &, long id, QMenu * itemMenu);
	virtual ~ItemBase();

	qint64 id() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	double z();
	virtual void saveGeometry() = 0;
	ViewGeometry & getViewGeometry();
	ViewGeometry::WireFlags wireFlags() const;
	virtual bool itemMoved() = 0;
	QSizeF size();
	class ModelPart * modelPart();
	void setModelPart(class ModelPart *);
	class ModelPartShared * modelPartShared();
	virtual void writeXml(QXmlStreamWriter &) {}
	virtual void saveInstance(QXmlStreamWriter &);
	virtual void saveInstanceLocation(QXmlStreamWriter &) = 0;
	virtual void writeGeometry(QXmlStreamWriter &);
	virtual void moveItem(ViewGeometry &) = 0;
	virtual void setItemPos(QPointF & pos);
	virtual void rotateItem(double degrees, bool includeRatsnest);
	virtual void flipItem(Qt::Orientations orientation);
	virtual void transformItem(const QTransform &, bool includeRatsnest);
	virtual void transformItem2(const QMatrix &);
	virtual void removeLayerKin();
	ViewLayer::ViewID viewID();
	QString & viewIDName();
	ViewLayer::ViewLayerID viewLayerID() const;
	void setViewLayerID(ViewLayer::ViewLayerID, const LayerHash & viewLayers);
	void setViewLayerID(const QString & layerName, const LayerHash & viewLayers);
	bool topLevel();

	void collectConnectors(ConnectorPairHash & connectorHash, SkipCheckFunction);

	virtual void busConnectorItems(class Bus * bus, ConnectorItem *, QList<ConnectorItem *> & items);
	virtual void setHidden(bool hidden);
	virtual void setLayerHidden(bool hidden);
	bool hidden();
	bool layerHidden();
	virtual void setInactive(bool inactivate);
	bool inactive();
	ConnectorItem * findConnectorItemWithSharedID(const QString & connectorID, ViewLayer::ViewLayerPlacement);
	ConnectorItem * findConnectorItemWithSharedID(const QString & connectorID);
	void updateConnections(ConnectorItem *, bool includeRatsnest, QList<ConnectorItem *> & already);
	virtual void updateConnections(bool includeRatsnest, QList<ConnectorItem *> & already);
    virtual const QString & title();
    const QString & constTitle() const;
    bool getRatsnest();
	QList<class Bus *> buses();
	int itemType() const;					// wanted this to return ModelPart::ItemType but couldn't figure out how to get it to compile
	virtual bool isSticky();
	virtual bool isBaseSticky();
	virtual void setSticky(bool);
	void setLocalSticky(bool);
    bool isLocalSticky();
	virtual void addSticky(ItemBase *, bool stickem);
	virtual ItemBase * stickingTo();
	virtual QList< QPointer<ItemBase> > stickyList();
	virtual bool alreadySticking(ItemBase * itemBase);
	virtual bool stickyEnabled();
	ConnectorItem * anyConnectorItem();
	const QString & instanceTitle() const;
	QString label();
	virtual void updateTooltip();
	void setTooltip();
	void removeTooltip();
	bool hasConnectors();
	bool hasNonConnectors();
	bool hasConnections();
	bool canFlip(Qt::Orientations);
	bool canFlipHorizontal();
	void setCanFlipHorizontal(bool);
	bool canFlipVertical();
	void setCanFlipVertical(bool);
	virtual void clearModelPart();
	virtual bool hasPartLabel();
	ViewLayer::ViewLayerID partLabelViewLayerID();
	void clearPartLabel();
	bool isPartLabelVisible();
	void restorePartLabel(QDomElement & labelGeometry, ViewLayer::ViewLayerID);				// on loading from a file
	void movePartLabel(QPointF newPos, QPointF newOffset);												// coming down from the command object
	void partLabelMoved(QPointF oldPos, QPointF oldOffset, QPointF newPos, QPointF newOffset);			// coming up from the label
	void partLabelSetHidden(bool hide);
	void rotateFlipPartLabel(double degrees, Qt::Orientations);				// coming up from the label
	void doRotateFlipPartLabel(double degrees, Qt::Orientations);			// coming down from the command object
	QString makePartLabelSvg(bool blackOnly, double dpi, double printerScale);
	QPointF partLabelScenePos();
	QRectF partLabelSceneBoundingRect();
	virtual bool isSwappable();
    virtual void setSwappable(bool);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void collectWireConnectees(QSet<class Wire *> & wires);
	virtual bool collectFemaleConnectees(QSet<ItemBase *> & items);
	void prepareGeometryChange();
	virtual void resetID();
	void updateConnectionsAux(bool includeRatsnest, QList<ConnectorItem *> & already);
	void hoverEnterEvent( QGraphicsSceneHoverEvent * event );
	void hoverLeaveEvent( QGraphicsSceneHoverEvent * event );
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
	virtual void figureHover();
	virtual QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	virtual void slamZ(double newZ);
	bool isEverVisible();
	void setEverVisible(bool);
	virtual bool connectionIsAllowed(ConnectorItem *);
	virtual bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);
	virtual QString getProperty(const QString & key);
	ConnectorItem * rightClickedConnector();
	virtual bool canEditPart();
	virtual bool hasCustomSVG();
	virtual void setProp(const QString & prop, const QString & value);
	QString prop(const QString & p);
	bool isObsolete();
    virtual QHash<QString, QString> prepareProps(ModelPart *, bool wantDebug, QStringList & keys);
	void resetValues(const QString & family, const QString & prop);
	const QString & filename();
	void setFilename(const QString &);
	virtual PluralType isPlural();
	ViewLayer::ViewLayerPlacement viewLayerPlacement() const;
	void setViewLayerPlacement(ViewLayer::ViewLayerPlacement);
	virtual void calcRotation(QTransform & rotation, QPointF center, ViewGeometry &);
    void updateConnectors();
	const QString & moduleID();
	bool moveLock();
	virtual void setMoveLock(bool);
	void debugInfo(const QString & msg) const;
	void debugInfo2(const QString & msg) const;
	virtual void addedToScene(bool temporary);
	virtual bool hasPartNumberProperty();
	void collectPropsMap(QString & family, QMap<QString, QString> &);
	virtual bool rotationAllowed();
	virtual bool rotation45Allowed();
	virtual bool freeRotationAllowed();
	void ensureUniqueTitle(const QString &title, bool force);
	virtual void setDropOffset(QPointF offset);
	bool hasRubberBandLeg() const;
	void killRubberBandLeg();
	bool sceneEvent(QEvent *event);
	void clearConnectorItemCache();
	const QList<ConnectorItem *> & cachedConnectorItems();
	const QList<ConnectorItem *> & cachedConnectorItemsConst() const;
	bool inHover();
	virtual QRectF boundingRectWithoutLegs() const;
    QRectF boundingRect() const;
    virtual QPainterPath hoverShape() const;
	virtual const QCursor * getCursor(Qt::KeyboardModifiers);
	class PartLabel * partLabel();
	virtual void doneLoading();
	QString family();
	QPixmap * getPixmap(QSize size);
    class FSvgRenderer * fsvgRenderer() const;
	void setSharedRendererEx(class FSvgRenderer *);
	bool reloadRenderer(const QString & svg, bool fastload);
	bool resetRenderer(const QString & svg);
	bool resetRenderer(const QString & svg, QString & newSvg);
    void getPixmaps(QPixmap * &, QPixmap * &, QPixmap * &, bool swappingEnabled, QSize);
    class FSvgRenderer * setUpImage(class ModelPart * modelPart, class LayerAttributes &);
	void showConnectors(const QStringList &);
	void setItemIsSelectable(bool selectable);
	virtual bool inRotation();
	virtual void setInRotation(bool);
	const QString & spice() const;
	const QString & spiceModel() const;
    void addSubpart(ItemBase *);
    void setSuperpart(ItemBase *);
    ItemBase * superpart();
    ItemBase * findSubpart(const QString & connectorID, ViewLayer::ViewLayerPlacement);
    const QList< QPointer<ItemBase> > & subparts();
    void setSquashShape(bool);
    const QPainterPath & selectionShape();
    virtual void setTransform2(const QTransform &);
    void initLayerAttributes(LayerAttributes & layerAttributes, ViewLayer::ViewID, ViewLayer::ViewLayerID, ViewLayer::ViewLayerPlacement, bool doConnectors, bool doCreateShape);
    virtual QString getInspectorTitle();
    virtual void setInspectorTitle(const QString & oldText, const QString & newText);

public:
	virtual void getConnectedColor(ConnectorItem *, QBrush &, QPen &, double & opacity, double & negativePenWidth, bool & negativeOffsetRect);
	virtual void getUnconnectedColor(ConnectorItem *, QBrush &, QPen &, double & opacity, double & negativePenWidth, bool & negativeOffsetRect);
	virtual void getNormalColor(ConnectorItem *, QBrush &, QPen &, double & opacity, double & negativePenWidth, bool & negativeOffsetRect);
	virtual void getHoverColor(ConnectorItem *, QBrush &, QPen &, double & opacity, double & negativePenWidth, bool & negativeOffsetRect);
	virtual void getEqualPotentialColor(ConnectorItem *, QBrush &, QPen &, double & opacity, double & negativePenWidth, bool & negativeOffsetRect);

protected:
	static QPen NormalPen;
	static QPen HoverPen;
	static QPen ConnectedPen;
	static QPen UnconnectedPen;
	static QPen ChosenPen;
	static QPen EqualPotentialPen;
	static QBrush HoverBrush;
	static QBrush NormalBrush;
	static QBrush ConnectedBrush;
	static QBrush UnconnectedBrush;
	static QBrush ChosenBrush;
	static QBrush EqualPotentialBrush;
	static const double NormalConnectorOpacity;

public:
	static QColor connectedColor();
	static QColor unconnectedColor();
	static QColor standardConnectedColor();
	static QColor standardUnconnectedColor();
	static void setConnectedColor(QColor &);
	static void setUnconnectedColor(QColor &);

public:
	virtual void hoverEnterConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem * item);
	virtual void hoverLeaveConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem * item);
	virtual void hoverMoveConnectorItem(QGraphicsSceneHoverEvent * event, class ConnectorItem * item);
	void hoverEnterConnectorItem();
	void hoverLeaveConnectorItem();
	virtual void connectorHover(ConnectorItem *, ItemBase *, bool hovering);
	virtual void clearConnectorHover();
	virtual void hoverUpdate();
	virtual bool filterMousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual void mousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual void mouseDoubleClickConnectorEvent(ConnectorItem *);
	virtual void mouseMoveConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual void mouseReleaseConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual bool acceptsMousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual bool acceptsMousePressLegEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual void setAcceptsMousePressLegEvent(bool);
	virtual bool acceptsMouseDoubleClickConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual bool acceptsMouseMoveConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual bool acceptsMouseReleaseConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual void connectionChange(ConnectorItem * onMe, ConnectorItem * onIt, bool connect);
	virtual void connectedMoved(ConnectorItem * from, ConnectorItem * to, QList<ConnectorItem *> & already);
	virtual ItemBase * layerKinChief();
	virtual const QList<ItemBase *> & layerKin();
	virtual void findConnectorsUnder() = 0;
	virtual ConnectorItem* newConnectorItem(class Connector *connector);
	virtual ConnectorItem* newConnectorItem(ItemBase * layerkin, Connector *connector);

	virtual void setInstanceTitle(const QString &title, bool initial);
	void updatePartLabelInstanceTitle();

public slots:
	void showPartLabel(bool show, ViewLayer *);
	void hidePartLabel();
	void partLabelChanged(const QString &newText);
	virtual void swapEntry(const QString & text);
    void showInFolder();

public:
	static bool zLessThan(ItemBase * & p1, ItemBase * & p2);
	static qint64 getNextID();
	static qint64 getNextID(qint64 fromIndex);

protected:
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event );
	virtual void paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget); 
	virtual void paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget, const QPainterPath & shape); 
    virtual void paintSelected(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	virtual void paintBody(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value);

	virtual QStringList collectValues(const QString & family, const QString & prop, QString & value);

	void setInstanceTitleTooltip(const QString& text);
	virtual void setDefaultTooltip();
	void setInstanceTitleAux(const QString & title, bool initial);
	void saveLocAndTransform(QXmlStreamWriter & streamWriter);
    QPixmap * getPixmap(ViewLayer::ViewID, bool swappingEnabled, QSize size);
    virtual ViewLayer::ViewID useViewIDForPixmap(ViewLayer::ViewID, bool swappingEnabled);
    virtual bool makeLocalModifications(QByteArray & svg, const QString & filename);
    void updateHidden();
    void createShape(LayerAttributes & layerAttributes);

protected:
	static bool getFlipDoc(ModelPart * modelPart, const QString & filename, ViewLayer::ViewLayerID viewLayerID, ViewLayer::ViewLayerPlacement, QDomDocument &, Qt::Orientations);
	static bool fixCopper1(ModelPart * modelPart, const QString & filename, ViewLayer::ViewLayerID viewLayerID, ViewLayer::ViewLayerPlacement, QDomDocument &);

protected:
 	QSizeF m_size;
	qint64 m_id;
	ViewGeometry m_viewGeometry;
	QPointer<ModelPart> m_modelPart;
	ViewLayer::ViewID m_viewID;
	ViewLayer::ViewLayerID m_viewLayerID;
	int m_connectorHoverCount;
	int m_connectorHoverCount2;
	int m_hoverCount;
	bool m_hidden;
	bool m_layerHidden;
	bool m_inactive;
	bool m_sticky;
	QHash< long, QPointer<ItemBase> > m_stickyList;
	QMenu *m_itemMenu;
	bool m_canFlipHorizontal;
	bool m_canFlipVertical;
	bool m_zUninitialized;
	QPointer<class PartLabel> m_partLabel;
	bool m_spaceBarWasPressed;
	bool m_hoverEnterSpaceBarWasPressed;
	bool m_everVisible;
	ConnectorItem * m_rightClickedConnector;
	QMap<QString, QString> m_propsMap;
	QString m_filename;
	ViewLayer::ViewLayerPlacement m_viewLayerPlacement;
	bool m_moveLock;
	bool m_hasRubberBandLeg;
	QList<ConnectorItem *> m_cachedConnectorItems;
	QGraphicsSvgItem * m_moveLockItem;
	QGraphicsSvgItem * m_stickyItem;
    FSvgRenderer * m_fsvgRenderer;
	bool m_acceptsMousePressLegEvent;
    bool m_swappable;
 	bool m_inRotation;
    QPointer<ItemBase> m_superpart;
    QList< QPointer<ItemBase> > m_subparts;
    bool m_squashShape;
    QPainterPath m_selectionShape;
      
 protected:
	static long nextID;
	static QPointer<class ReferenceModel> TheReferenceModel;

public:
	static const QString ITEMBASE_FONT_PREFIX;
	static const QString ITEMBASE_FONT_SUFFIX;
	static QHash<QString, QString> TranslatedPropertyNames;
    static QString PartInstanceDefaultTitle;
	static const QList<ItemBase *> EmptyList;
	const static QColor HoverColor;
	const static double HoverOpacity;
	const static QColor ConnectorHoverColor;
	const static double ConnectorHoverOpacity;

public:
	static void initNames();
	static void cleanup();
	static ItemBase * extractTopLevelItemBase(QGraphicsItem * thing);
	static QString translatePropertyName(const QString & key);
	static void setReferenceModel(class ReferenceModel *);
    static void renderOne(QDomDocument *, QImage *, const QRectF & renderRect);



};


Q_DECLARE_METATYPE( ItemBase* );			// so we can stash them in a QVariant


#endif
