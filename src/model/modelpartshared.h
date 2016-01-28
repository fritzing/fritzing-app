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

$Revision: 6955 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-06 23:14:37 +0200 (Sa, 06. Apr 2013) $

********************************************************************/

#ifndef MODELPARTSHARED_H
#define MODELPARTSHARED_H

#include <QDomDocument>
#include <QDomElement>
#include <QList>
#include <QStringList>
#include <QHash>
#include <QDate>
#include <QPointer>

#include "../viewlayer.h"

struct ViewImage {
    ViewLayer::ViewID viewID;
    qulonglong layers;
    qulonglong sticky;
    qulonglong flipped;
    QString image;
    bool canFlipHorizontal;
    bool canFlipVertical;

    ViewImage(ViewLayer::ViewID);
};

class ModelPartShared : public QObject
{
Q_OBJECT
public:
	ModelPartShared();
	ModelPartShared(QDomDocument &, const QString & path);
	~ModelPartShared();

	//QDomDocument * domDocument();

	void copy(ModelPartShared* other);

	const QString & uri();
	void setUri(QString);
	const QString & moduleID();
	void setModuleID(QString);
	const QString & version();
	void setVersion(QString);
	const QString & author();
	void setAuthor(QString);
	const QString & description();
	void setDescription(QString);
	const QString & spice();
	void setSpice(QString);
	const QString & spiceModel();
	void setSpiceModel(QString);
	const QString & url();
	void setUrl(QString);
	const QString & title();
	void setTitle(QString);
	const QString & label() const;
	void setLabel(QString);
	const QDate & date();
	void setDate(QDate);
	const QString & dateAsStr();
	void setDate(QString);
    void setDBID(qulonglong);
    qulonglong dbid();
    const QString & fritzingVersion();
    void setFritzingVersion(const QString &);

    const QList<ViewImage *> viewImages();
    QString imageFileName(ViewLayer::ViewID, ViewLayer::ViewLayerID);
    void setImageFileName(ViewLayer::ViewID, const QString & filename);
    QString imageFileName(ViewLayer::ViewID);
    const QList< QPointer<ModelPartShared> > & subparts();
    void addSubpart(ModelPartShared * subpart);
    bool hasSubparts();
    void setSubpartID(const QString &);
    const QString & subpartID() const;
    ModelPartShared * superpart();
    void setSuperpart(ModelPartShared *);
    bool anySticky(ViewLayer::ViewID);
    bool hasMultipleLayers(ViewLayer::ViewID);
    bool canFlipHorizontal(ViewLayer::ViewID);
    bool canFlipVertical(ViewLayer::ViewID);
    bool hasViewID(ViewLayer::ViewID viewID);
    LayerList viewLayers(ViewLayer::ViewID viewID);
    LayerList viewLayersFlipped(ViewLayer::ViewID viewID);

	const QString & path();
	void setPath(QString path);
	const QString & taxonomy();
	void setTaxonomy(QString taxonomy);

	const QList< QPointer<class ConnectorShared> > connectorsShared();
	void setConnectorsShared(QList< QPointer<class ConnectorShared> > connectors);
	void connectorIDs(ViewLayer::ViewID viewId, ViewLayer::ViewLayerID viewLayerID, QStringList & connectorIDs, QStringList & terminalIDs, QStringList & legIDs);

	const QStringList &tags();
	void setTags(const QStringList &tags);
	void setTag(const QString &tag);

	QString family();
	void setFamily(const QString &family);

	QHash<QString,QString> & properties();
	void setProperties(const QHash<QString,QString> &properties);
	const QStringList & displayKeys();


	void initConnectors();
    void setConnectorsInitialized(bool); 
	ConnectorShared * getConnectorShared(const QString & id);
	bool ignoreTerminalPoints();

	void setProperty(const QString & key, const QString & value, bool showInLabel);
    bool showInLabel(const QString & key);
	const QString & replacedby();
	void setReplacedby(const QString & replacedby);

	void flipSMDAnd();
	void setFlippedSMD(bool);
	bool flippedSMD();	
	bool needsCopper1();
	bool hasViewFor(ViewLayer::ViewID);
	bool hasViewFor(ViewLayer::ViewID, ViewLayer::ViewLayerID);
	QString hasBaseNameFor(ViewLayer::ViewID);
    void setViewImage(ViewImage *);
    void addConnector(ConnectorShared *);
    void insertBus(class BusShared *);
    void lookForZeroConnector();
    bool hasZeroConnector();
    void addOwner(QObject *);
    void setSubpartOffset(QPointF);
    QPointF subpartOffset() const;

protected:
	void loadTagText(QDomElement parent, QString tagName, QString &field);
	// used to populate de StringList that contains both the <tags> and the <properties> values
	void populateTags(QDomElement parent, QStringList &list);
	void populateProperties(QDomElement parent, QHash<QString,QString> &hash, QStringList & displayKeys);
	void commonInit();
	void ensurePartNumberProperty();
    void copyPins(ViewLayer::ViewLayerID from, ViewLayer::ViewLayerID to);
    LayerList viewLayersAux(ViewLayer::ViewID viewID, qulonglong (*accessor)(ViewImage *));
    void addSchematicText(ViewImage *);
	bool setDomDocument(QDomDocument &);

protected slots:
    void removeOwner();

public:
	static const QString PartNumberPropertyName;

protected:

	//QDomDocument* m_domDocument;

	QString m_uri;
	QString m_moduleID;
    QString m_fritzingVersion;
	QString m_version;
	QString m_author;
	QString m_title;
	QString m_label;
	QString m_description;
    QString m_spice;
    QString m_spiceModel;
	QString m_url;
	QString m_date;
	QString m_replacedby;

	QString m_path;
	QString m_taxonomy;

	QStringList m_tags;
	QStringList m_displayKeys;
	QHash<QString,QString> m_properties;

	QHash<QString, QPointer<class ConnectorShared> > m_connectorSharedHash;
	QHash<QString, class BusShared *> m_buses;
    QHash<ViewLayer::ViewID, ViewImage *> m_viewImages;

	bool m_connectorsInitialized;
	bool m_ignoreTerminalPoints;

	bool m_flippedSMD;
	bool m_needsCopper1;				// for converting pre-two-layer parts
    qulonglong m_dbid;
    bool m_hasZeroConnector;
    int m_ownerCount;
    QList< QPointer<ModelPartShared> > m_subparts;
    QPointer<ModelPartShared> m_superpart;
    QString m_subpartID;
    QPointF m_subpartOffset;
};

class ModelPartSharedRoot : public ModelPartShared
{
	Q_OBJECT
public:
	const QString & icon();
	void setIcon(const QString & filename);
	const QString & searchTerm();
	void setSearchTerm(const QString & searchTerm);

protected:
	QString m_icon;
	QString m_searchTerm;

};


#endif
