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

$Revision: 6565 $:
$Author: irascibl@gmail.com $:
$Date: 2012-10-15 12:10:48 +0200 (Mo, 15. Okt 2012) $

********************************************************************/

#ifndef MODELBASE_H
#define MODELBASE_H

#include <QObject>
#include "modelpart.h"

class ModelBase : public QObject
{
Q_OBJECT

public:
	ModelBase(bool makeRoot);
	virtual ~ModelBase();

	ModelPart * root();
	ModelPartSharedRoot * rootModelPartShared();
	virtual ModelPart* retrieveModelPart(const QString & moduleID);
	virtual ModelPart * addModelPart(ModelPart * parent, ModelPart * copyChild);
    bool loadFromFile(const QString & fileName, ModelBase* referenceModel, QList<ModelPart *> & modelParts, bool checkInstances);
	void save(const QString & fileName, bool asPart);
	void save(const QString & fileName, class QXmlStreamWriter &, bool asPart);
	virtual ModelPart * addPart(QString newPartPath, bool addToReference);
	virtual bool addPart(ModelPart * modelPart, bool update);
	virtual ModelPart * addPart(QString newPartPath, bool addToReference, bool updateIdAlreadyExists);
	bool paste(ModelBase * referenceModel, QByteArray & data, QList<ModelPart *> & modelParts, QHash<QString, QRectF> & boundingRects, bool preserveIndex);
	void setReportMissingModules(bool);
	ModelPart * genFZP(const QString & moduleID, ModelBase * referenceModel);
	const QString & fritzingVersion();
    void setReferenceModel(ModelBase *);
    bool checkForReversedWires();

public:
    static bool onCoreList(const QString & moduleID);

signals:
	void loadedViews(ModelBase *, QDomElement & views);
	void loadedRoot(const QString & fileName, ModelBase *, QDomElement & root);
	void loadingInstances(ModelBase *, QDomElement & instances);
	void loadingInstance(ModelBase *, QDomElement & instance);
	void loadedInstances(ModelBase *, QDomElement & instances);
    void obsoleteSMDOrientationSignal();
    void oldSchematicsSignal(const QString & filename, bool & useOldSchematics);

protected:
	void renewModelIndexes(QDomElement & root, const QString & childName, QHash<long, long> & oldToNew);
	bool loadInstances(QDomDocument &, QDomElement & root, QList<ModelPart *> & modelParts, bool checkViews);
	ModelPart * fixObsoleteModuleID(QDomDocument & domDocument, QDomElement & instance, QString & moduleIDRef);
	static bool isRatsnest(QDomElement & instance);
	static void checkTraces(QDomElement & instance);
	static void checkMystery(QDomElement & instance);
	static bool checkObsoleteOrientation(QDomElement & instance);
	static bool checkOldSchematics(QDomElement & instance);
    ModelPart * createOldSchematicPart(ModelPart *, QString & moduleIDRef);
    ModelPart * createOldSchematicPartAux(ModelPart *, const QString & oldModuleIDRef, const QString & oldSchematicFileName, const QString & oldSvgPath);

protected:
	QPointer<ModelPart> m_root;
	QPointer<ModelBase> m_referenceModel;
	bool m_reportMissingModules;
	QString m_fritzingVersion;
    bool m_useOldSchematics;
    bool m_checkForReversedWires;

protected:
    static QList<QString> CoreList;

};

#endif
