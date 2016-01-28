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

$Revision: 6956 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-07 12:14:50 +0200 (So, 07. Apr 2013) $

********************************************************************/


#ifndef MODELPART_H
#define MODELPART_H

#include <QString>
#include <QPointF>
#include <QSize>
#include <QGraphicsItem>
#include <QDomDocument>
#include <QTextStream>
#include <QXmlStreamWriter>
#include <QHash>
#include <QList>
#include <QPointer>
#include <QSharedPointer>

#include "modelpartshared.h"
#include "../connectors/connector.h"
#include "../connectors/bus.h"

class ModelPart : public QObject
{
	Q_OBJECT

public:
	enum ItemType {
		 Part,
		 Wire,
		 Breadboard,
		 Board,
		 ResizableBoard,
		 Note,
		 Symbol,
		 Jumper,
		 CopperFill,
		 Logo,
		 Hole,
		 Via,
		 Ruler,
         SchematicSubpart,
		 Space,
		 Unknown
	};

	enum LocationFlag {
		NoFlag = 0,
		CoreFlag = 1,
		ContribFlag = 2,
		AlienFlag = 4,
		FzzFlag = 8,
        InBinFlag = 16
	};
	Q_DECLARE_FLAGS(LocationFlags, LocationFlag)

public:
	ModelPart(QDomDocument &, const QString& path, ItemType type);
	ModelPart(ItemType type = ModelPart::Unknown);
	~ModelPart();

	ModelPart::ItemType itemType() const;
	void setItemType(ItemType);
	const QString & moduleID() const;
	void copy(ModelPart *);
	void copyNew(ModelPart *);
	void copyStuff(ModelPart * modelPart);
	ModelPartShared * modelPartShared();
    ModelPartSharedRoot * modelPartSharedRoot();
	void setModelPartShared(ModelPartShared *modelPartShared);
	void saveInstances(const QString & fileName, QXmlStreamWriter & streamWriter, bool startDocument);
	void saveAsPart(QXmlStreamWriter & streamWriter, bool startDocument);
	void addViewItem(class ItemBase *);
	void removeViewItem(class ItemBase *);
    void killViewItems();
	class ItemBase * viewItem(QGraphicsScene * scene);
	class ItemBase * viewItem(ViewLayer::ViewID);
	bool hasViewItems();
	void initConnectors(bool force=false);
	const QHash<QString, QPointer<Connector> > & connectors();
	long modelIndex();
	void setModelIndex(long index);
	void setModelIndexFromMultiplied(long multipliedIndex);
	void setInstanceDomElement(const QDomElement &);
	const QDomElement & instanceDomElement();
	Connector * getConnector(const QString & id);

    const QString & fritzingVersion();

	const QString & title();
	const QString & description();
	const QString & spice();
	const QString & spiceModel();
	const QString & url();
	const QStringList & tags();
	const QStringList & displayKeys();
	const QHash<QString,QString> & properties() const;
	const QHash<QString, QPointer<Bus> > & buses();
	const QString & taxonomy();
	const QString & version();
	const QString & path();
	const QString & label();
	const QString & author();
	const QString & language();
	const QString & uri();
	const QDate & date();
	QString family();
    void setDBID(qulonglong);
    qulonglong dbid();

	Bus * bus(const QString & busID);
	bool ignoreTerminalPoints();

	bool isCore();
	void setCore(bool core);
	bool isContrib();
	void setContrib(bool contrib);
	bool isAlien(); // from "outside" 
	void setAlien(bool alien);
	bool isFzz(); // from "outside" 
	void setFzz(bool alien);
	void setLocationFlag(bool setting, LocationFlag flag);
    bool isInBin();
    void setInBin(bool);

	bool hasViewID(ViewLayer::ViewID);
	bool canFlipVertical(ViewLayer::ViewID);
	bool canFlipHorizontal(ViewLayer::ViewID);
	bool anySticky(ViewLayer::ViewID);
    QString imageFileName(ViewLayer::ViewID);
    void setImageFileName(ViewLayer::ViewID, const QString & filename);
    QString imageFileName(ViewLayer::ViewID, ViewLayer::ViewLayerID);
    LayerList viewLayers(ViewLayer::ViewID);
    void setViewImage(struct ViewImage *);

	QList<ModelPart*> getAllParts();
	QList<ModelPart*> getAllNonCoreParts();
	bool hasViewID(long id);

	const QString & instanceTitle() const;
	const QString & instanceText();
	void setInstanceTitle(QString, bool initial);
	void setInstanceText(QString);
	QString getNextTitle(const QString & candidate);

	void setOrderedChildren(QList<QObject*> children);
	void setLocalProp(const QString & name, const QVariant & value);
	QVariant localProp(const QString & name) const;
	void setLocalProp(const char * name, const QVariant & value);
	QVariant localProp(const char * name) const;
    void setTag(const QString &tag);
	void setProperty(const QString & key, const QString & value, bool showInLabel);
    bool showInLabel(const QString & key);
    
	const QString & replacedby();
	bool isObsolete();

	bool flippedSMD();
	bool needsCopper1();
	bool hasViewFor(ViewLayer::ViewID);
	bool hasViewFor(ViewLayer::ViewID, ViewLayer::ViewLayerID);
	QString hasBaseNameFor(ViewLayer::ViewID);
	void initBuses();
	void clearBuses();
	void setConnectorLocalName(const QString & id, const QString & name);
	QString connectorLocalName(const QString & id);
	void setLocalTitle(const QString &);
    const QList<ViewImage *> viewImages();
    void addConnector(Connector *);
    void flipSMDAnd();
    void lookForZeroConnector();
    bool hasZeroConnector();
    bool hasSubparts();
    void setSubpartID(const QString &);

public:
	static long nextIndex();
	static void updateIndex(long index);
	static const int indexMultiplier;
	static const QStringList & possibleFolders();

signals:
	void startSaveInstances(const QString & fileName, ModelPart *, QXmlStreamWriter &);

protected:
	void writeTag(QXmlStreamWriter & streamWriter, QString tagName, QString tagValue);
	void writeNestedTag(QXmlStreamWriter & streamWriter, QString tagName, const QStringList &values, QString childTag);
	void writeNestedTag(QXmlStreamWriter & streamWriter, QString tagName, const QHash<QString,QString> &values, QString childTag, QString attrName);

	void commonInit(ItemType type);
	void saveInstance(QXmlStreamWriter & streamWriter);
	QList< QPointer<ModelPart> > * ensureInstanceTitleIncrements(const QString & prefix);
	void clearOldInstanceTitle(const QString & title);
    bool setSubpartInstanceTitle();

protected:
	QList< QPointer<class ItemBase> > m_viewItems;

	ItemType m_type;
	QPointer<ModelPartShared> m_modelPartShared;
	QHash<QString, QPointer<Connector> > m_connectorHash;
	QHash<QString, QPointer<Bus> > m_busHash;
	long m_index;						// only used at save time to identify model parts in the xml
	QDomElement m_instanceDomElement;	// only used at load time (so far)

	LocationFlags m_locationFlags;
	bool m_indexSynched;

	QString m_instanceTitle;
	QString m_instanceText;
	QString m_localTitle;

	QList<QObject*> m_orderedChildren;

protected:
	static QHash<ItemType, QString> itemTypeNames;
	static long m_nextIndex;
	static QStringList m_possibleFolders;
};

Q_DECLARE_METATYPE( ModelPart* );			// so we can stash them in a QVariant
typedef QList< QPointer<ModelPart> > ModelPartList;
Q_DECLARE_OPERATORS_FOR_FLAGS(ModelPart::LocationFlags)

#endif
