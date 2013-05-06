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

$Revision: 6955 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-06 23:14:37 +0200 (Sa, 06. Apr 2013) $

********************************************************************/



#ifndef SQLITEREFERENCEMODEL_H_
#define SQLITEREFERENCEMODEL_H_

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QApplication>

#include "referencemodel.h"

class SqliteReferenceModel : public ReferenceModel {
	Q_OBJECT
	public:
		SqliteReferenceModel();
		~SqliteReferenceModel();

		bool loadAll(const QString & databaseName, bool fullLoad, bool dbExists);
		bool loadFromDB(const QString & databaseName);
		ModelPart *loadPart(const QString & path, bool update);
		ModelPart *reloadPart(const QString & path, const QString & moduleID);

		ModelPart *retrieveModelPart(const QString &moduleID);

		bool addPart(ModelPart * newModel, bool update);
		bool updatePart(ModelPart * newModel);
		ModelPart * addPart(QString newPartPath, bool addToReference, bool updateIdAlreadyExists);

		bool swapEnabled();
		bool containsModelPart(const QString & moduleID);

		QString partTitle(const QString & moduleID);
		QStringList propValues(const QString &family, const QString &propName, bool distinct);
		QMultiHash<QString, QString> allPropValues(const QString &family, const QString &propName);
		void recordProperty(const QString &name, const QString &value);
		QString retrieveModuleIdWith(const QString &family, const QString &propertyName, bool closestMatch);
		QString retrieveModuleId(const QString &family, const QMultiHash<QString /*name*/, QString /*value*/> &properties, const QString &propertyName, bool closestMatch);
		bool lastWasExactMatch();

	protected:
		void initParts(bool dbExists);
        void killParts();

	protected:
		bool addPartAux(ModelPart * newModel, bool fullLoad);

		QString closestMatchId(const QString &family, const QMultiHash<QString, QString> &properties, const QString &propertyName, const QString &propertyValue);
		QStringList getPossibleMatches(const QString &family, const QMultiHash<QString, QString> &properties, const QString &propertyName, const QString &propertyValue);
		QString getClosestMatch(const QString &family, const QMultiHash<QString, QString> &properties, QStringList possibleMatches);
		int countPropsInCommon(const QString &family, const QMultiHash<QString, QString> &properties, const ModelPart *part2);

		bool createConnection(const QString & databaseName, bool fullLoad);
		void deleteConnection();
		bool insertPart(ModelPart *, bool fullLoad);
		bool insertProperty(const QString & name, const QString & value, qulonglong id, bool showInLabel);
		bool insertTag(const QString & tag, qulonglong id);
		bool insertViewImage(const struct ViewImage *, qulonglong id);
        bool insertConnector(const class Connector *, qulonglong id);
        bool insertConnectorLayer(const class SvgIdLayer *, qulonglong id);  // connector db id
        bool insertBus(const Bus *, qulonglong id);
        bool insertBusMember(const Connector *, qulonglong id);
		qulonglong partId(QString moduleID);
		bool removePart(qulonglong partId);
		bool removeProperties(qulonglong partId);
        bool loadFromDB(QSqlDatabase & keep_db, QSqlDatabase & db);
        bool createProperties(QSqlDatabase & db);
        bool createParts(QSqlDatabase & db, bool fullLoad);
        bool insertSubpart(ModelPartShared *, qulonglong id);
        bool insertSubpartConnector(const ConnectorShared * cs, qulonglong id);

protected:
		volatile bool m_swappingEnabled;
		volatile bool m_lastWasExactMatch;
        volatile bool m_keepGoing;
		bool m_init;
        QSqlDatabase m_database;
		QMultiHash<QString /*name*/, QString /*value*/> m_recordedProperties;
};

#endif /* SQLITEREFERENCEMODEL_H_ */
