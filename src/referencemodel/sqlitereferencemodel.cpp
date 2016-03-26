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
GNU General Public License for more details.floadfrom

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 6956 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-07 12:14:50 +0200 (So, 07. Apr 2013) $

********************************************************************/

// copying a table from one SQWL database to another http://sqlite.phxsoftware.com/forums/t/285.aspx

#include <QSqlRecord>
#include <QSqlError>
#include <QMessageBox>
#include <QVector>
#include <QSqlResult>
#include <QSqlDriver>
#include <QDebug>
#include <limits>

#include "sqlitereferencemodel.h"
#include "../debugdialog.h"
#include "../connectors/svgidlayer.h"
#include "../connectors/connector.h"
#include "../connectors/connectorshared.h"
#include "../connectors/busshared.h"
#include "../utils/folderutils.h"
#include "../utils/fmessagebox.h"
#include "../utils/textutils.h"


#define MAX_CONN_TRIES 3

static const qulonglong NO_ID = std::numeric_limits<qulonglong>::max();

void debugError(bool result, QSqlQuery & query) {
    if (result) return;

    QSqlError error = query.lastError();
    DebugDialog::debug(QString("%1 %2 %3").arg(error.text()).arg(error.number()).arg(error.type()));
}

static ModelPart * DebugModelPart = NULL;

void debugExec(const QString & msg, QSqlQuery & query) {
    DebugDialog::debug(
			"SQLITE: " + msg + "\n"
			"\t "+ query.lastQuery() + "\n"
			"\t ERROR DRIVER: "+ query.lastError().driverText() + "\n"
			"\t ERROR DB: " + query.lastError().databaseText() + "\n"
            "\t moduleid:" + (DebugModelPart == NULL ? "" : DebugModelPart->moduleID()) + ""
		);
    QMap<QString, QVariant> map = query.boundValues();
    foreach (QString name, map.keys()) {
        DebugDialog::debug(QString("\t%1:%2").arg(name).arg(map.value(name).toString()));
    }
}

void killConnectors(QVector<Connector *> & connectors) {
    foreach (Connector * connector, connectors) {
        delete connector->connectorShared();
        delete connector;
    }
    connectors.clear();
}

void killBuses(QVector<BusShared *> & buses) {
    foreach (BusShared * bus, buses) {
        delete bus;
    }
    buses.clear();
}

QStringList FailurePartMessages;
QStringList FailurePropertyMessages;

void noSwappingMessage()
{
	FMessageBox::warning(NULL,
			QObject::tr("Oops!"),
			QObject::tr("Sorry, we have a problem with the swapping mechanism.\nFritzing still works, but you won't be able to change parts properties."),
			QMessageBox::Ok);
}

///////////////////////////////////////////////////

SqliteReferenceModel::SqliteReferenceModel() {
	m_swappingEnabled = false;
	m_lastWasExactMatch = true;
}

bool SqliteReferenceModel::loadAll(const QString & databaseName, bool fullLoad, bool dbExists)
{
    FailurePartMessages.clear();
    FailurePropertyMessages.clear();
    m_fullLoad = fullLoad;
	initParts(dbExists);

	int tries = 0;
    m_keepGoing = true;
	while(!m_swappingEnabled && tries < MAX_CONN_TRIES && m_keepGoing) {
        createDatabase(databaseName, fullLoad);
		if(!m_swappingEnabled) {
			deleteConnection();
		}
		tries++;
	}
	/* TODO Mariano: perhaps we should check that there are no parts with
	 * the same family and providing exactly the same properties set
	 */

    DebugDialog::debug("referenceModel::loadAll completed");
	if (!m_swappingEnabled) {
        noSwappingMessage();
	}
    else if (FailurePartMessages.count() > 0) {
        QString message = tr("The swapping mechanism is disabled for:\n\n");
        if (FailurePartMessages.count() == 1) {
            message += FailurePartMessages.at(0);
        }
        else if (FailurePartMessages.count() < 6) {
            message += FailurePartMessages.join("\n");
        }
        else {
            for (int i = 0; i < 4; i++) {
                message += FailurePartMessages.at(i);
                message += ("\n");
            }
            message += "\n" + tr("and %1 other parts").arg(FailurePartMessages.count() - 4);
        }
        FMessageBox::warning(QApplication::activeWindow(), QObject::tr("Oops!"), message, QMessageBox::Ok);
    }
    else if (FailurePropertyMessages.count() > 0) {
        QString message = tr("The swapping mechanism is disabled for:\n\n");
        if (FailurePropertyMessages.count() == 1) {
            message += FailurePropertyMessages.at(0);
        }
        else if (FailurePropertyMessages.count() < 6) {
            message += FailurePropertyMessages.join("\n");
        }
        else {
            for (int i = 0; i < 4; i++) {
                message += FailurePropertyMessages.at(i);
                message += ("\n");
            }
            message += "\n" + tr("and %1 other properties").arg(FailurePropertyMessages.count() - 4);
        }
        FMessageBox::warning(QApplication::activeWindow(), QObject::tr("Oops!"), message, QMessageBox::Ok);
    }
    return m_swappingEnabled;

}

bool SqliteReferenceModel::loadFromDB(const QString & databaseName) 
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "temporary");
	db.setDatabaseName(databaseName);

    /*
    QVariant v = m_database.driver()->handle();
    if (v.isValid() && qstrcmp(v.typeName(), "sqlite3*") == 0) {
        // v.data() returns a pointer to the handle
        sqlite3 *handle = *static_cast<sqlite3 **>(v.data());
        if (handle != 0) { // check that it is not NULL
            const char * message = sqlite3_errmsg(handle);
            qDebug() << "testing sqlite3" << message;
        }
    }
    */

    m_swappingEnabled = loadFromDB(m_database, db);
    if (db.isOpen()) db.close();
    if (!m_swappingEnabled) {
        killParts();
        noSwappingMessage();
    }

    return m_swappingEnabled;
}

bool SqliteReferenceModel::loadFromDB(QSqlDatabase & keep_db, QSqlDatabase & db) 
{
    bool opened = false;
    for (int i = 0; i < MAX_CONN_TRIES; i++) {
	    if (!db.open()) continue;

        opened = true;
        break;
    }

    if (!opened) {
        return false;
    }

    m_sha = "";
    QSqlQuery  query = db.exec("SELECT sha FROM lastcommit where id=0");
    debugError(query.isActive(), query);
    if (query.isActive()) {
        if (query.next()) {
            m_sha = query.value(0).toString();
        }
    }
 
    query = db.exec("SELECT COUNT(*) FROM parts");
    debugError(query.isActive(), query);
    if (!query.isActive() || !query.next()) return false;

    int count = query.value(0).toInt();
    if (count == 0) {
        return false;
    }

    DebugDialog::debug(QString("parts count %1").arg(count));


    QVector<ModelPart *> parts(count + 1, NULL);
    QVector<qulonglong > oldToNew(count + 1, 0);

    query = db.exec("SELECT path, moduleID, id, family, version, replacedby, fritzingversion, author, title, label, date, description, spice, spicemodel, taxonomy, itemtype FROM parts");
    debugError(query.isActive(), query);
    if (!query.isActive()) return false;

    QSqlQuery q2(keep_db);
    bool result = q2.prepare("INSERT INTO parts(moduleID, family, core) VALUES (:moduleID, :family, :core)");
    debugError(result, q2);

    QFileInfo info(db.databaseName());
    QDir partsDir = info.absoluteDir();  // parts folder

    while (query.next()) {
        int ix = 0;

        QString path = query.value(ix++).toString();
        QString moduleID = query.value(ix++).toString();
        qulonglong dbid = query.value(ix++).toULongLong();

        if (m_partHash.value(moduleID, NULL) != NULL) {
            // a part with this moduleID was already loaded--the file version overrides the db version
            continue;
        }

        if (!path.startsWith(ResourcePath)) {        // not the resources path
            path = partsDir.absoluteFilePath(path);
            if (QFileInfo(path).exists()) {
                CoreList << moduleID;
            }
        }

		ModelPart * modelPart = new ModelPart();
        ModelPartShared * modelPartShared = new ModelPartShared();
        modelPart->setModelPartShared(modelPartShared);
        
        modelPartShared->setModuleID(moduleID);
        modelPartShared->setDBID(dbid);
        QString family = query.value(ix++).toString();
        modelPartShared->setFamily(family);
        modelPartShared->setVersion(query.value(ix++).toString());
        modelPartShared->setReplacedby(query.value(ix++).toString());
        modelPartShared->setFritzingVersion(query.value(ix++).toString());
        modelPartShared->setAuthor(query.value(ix++).toString());
        modelPartShared->setTitle(query.value(ix++).toString());
        modelPartShared->setLabel(query.value(ix++).toString());
        modelPartShared->setDate(query.value(ix++).toString());
        modelPartShared->setDescription(query.value(ix++).toString());
        modelPartShared->setSpice(query.value(ix++).toString());
        modelPartShared->setSpiceModel(query.value(ix++).toString());
        modelPartShared->setTaxonomy(query.value(ix++).toString());
        modelPart->setItemType((ModelPart::ItemType) query.value(ix++).toInt());
        modelPartShared->setPath(path);
        modelPart->setCore(true);

        modelPartShared->setConnectorsInitialized(true);

        m_partHash.insert(modelPartShared->moduleID(), modelPart);
        parts[dbid] = modelPart;

	    q2.bindValue(":moduleID", modelPartShared->moduleID());
	    q2.bindValue(":family", family);
	    q2.bindValue(":core", "1");
        bool result = q2.exec();
        if (!result) debugExec("unable to add part to memory", q2);

        qulonglong newid = q2.lastInsertId().toULongLong();
        oldToNew[dbid] = newid;
	}

    query = db.exec("SELECT viewid, image, layers, sticky, flipvertical, fliphorizontal, part_id FROM viewimages");
    debugError(query.isActive(), query);
    if (!query.isActive()) return false;

    while (query.next()) {
        int ix = 0;
        ViewImage * viewImage = new ViewImage(ViewLayer::BreadboardView);
        viewImage->viewID = (ViewLayer::ViewID) query.value(ix++).toInt();
        viewImage->image = query.value(ix++).toString();
        viewImage->layers = query.value(ix++).toULongLong();
        viewImage->sticky = query.value(ix++).toULongLong();
        viewImage->canFlipVertical = query.value(ix++).toInt() == 0 ? false : true;
        viewImage->canFlipHorizontal = query.value(ix++).toInt() == 0 ? false : true;
        qulonglong dbid = query.value(ix++).toULongLong();

        ModelPart * modelPart = parts.at(dbid);
        if (modelPart) {
            parts.at(dbid)->setViewImage(viewImage);
        }
    }

    query = db.exec("SELECT tag, part_id FROM tags");
    debugError(query.isActive(), query);
    if (!query.isActive()) return false;

    while (query.next()) {
        int ix = 0;
        QString tag = query.value(ix++).toString();
        qulonglong dbid = query.value(ix++).toULongLong();
        ModelPart * modelPart = parts.at(dbid);
        if (modelPart) {
            parts.at(dbid)->setTag(tag);
        }
    }

    query = db.exec("SELECT name, value, part_id, show_in_label FROM properties");
    debugError(query.isActive(), query);
    if (!query.isActive()) return false;

    QSqlQuery q3(keep_db);
	result = q3.prepare("INSERT INTO properties(name, value, part_id, show_in_label) VALUES (:name, :value, :part_id, :show_in_label)");
    debugError(result, q3);

    while (query.next()) {
        int ix = 0;
        QString name = query.value(ix++).toString();
        QString value = query.value(ix++).toString();
        qulonglong dbid = query.value(ix++).toULongLong();
        int showInLabel = query.value(ix++).toInt();
        ModelPart * modelPart = parts.at(dbid);
        if (modelPart) {
            parts.at(dbid)->setProperty(name, value, showInLabel);
	        q3.bindValue(":name", name.toLower().trimmed());
	        q3.bindValue(":value", value);
	        q3.bindValue(":part_id", oldToNew[dbid]);
	        q3.bindValue(":show_in_label", showInLabel);
            bool result = q3.exec();
            if (!result) debugExec("unable to add property to memory", q3);
        }
    }

    query = db.exec("SELECT COUNT(*) FROM connectors");
    debugError(query.isActive(), query);
    if (!query.isActive() || !query.next()) return false;

    int connectorCount = query.value(0).toInt();
    if (connectorCount == 0) return false;

    QVector<Connector *> connectors(connectorCount + 1, NULL);

    query = db.exec("SELECT id, connectorid, type, name, description, replacedby, part_id FROM connectors");
    debugError(query.isActive(), query);
    if (!query.isActive()) {
        killConnectors(connectors);
        return false;
    }

    while (query.next()) {
        int ix = 0;
        qulonglong cid = query.value(ix++).toULongLong();
        QString connectorid = query.value(ix++).toString();
        Connector::ConnectorType type = (Connector::ConnectorType) query.value(ix++).toInt();
        QString name = query.value(ix++).toString();
        QString description = query.value(ix++).toString();
        QString replacedby = query.value(ix++).toString();
        qulonglong dbid = query.value(ix++).toULongLong();
        ModelPart * modelPart = parts.at(dbid);
        if (modelPart) {
            ConnectorShared * connectorShared = new ConnectorShared();
            connectorShared->setConnectorType(type);
            connectorShared->setDescription(description);
            connectorShared->setReplacedby(replacedby);
            connectorShared->setSharedName(name);
            connectorShared->setId(connectorid);

            Connector * connector = new Connector(connectorShared, modelPart);
            modelPart->addConnector(connector);

            connectors[cid] = connector;
        }
    }

    query = db.exec("SELECT view, layer, svgid, hybrid, terminalid, legid, connector_id FROM connectorlayers");
    debugError(query.isActive(), query);
    if (!query.isActive()) {
        killConnectors(connectors);
        return false;
    }

    while (query.next()) {
        int ix = 0;
        ViewLayer::ViewID viewID = (ViewLayer::ViewID) query.value(ix++).toInt();
        ViewLayer::ViewLayerID viewLayerID = (ViewLayer::ViewLayerID) query.value(ix++).toInt();
        QString svgID = query.value(ix++).toString();
        bool hybrid = query.value(ix++).toInt() == 0 ? false : true;
        QString terminalID = query.value(ix++).toString();
        QString legID = query.value(ix++).toString();
        qulonglong cid = query.value(ix++).toULongLong();
        Connector * connector = connectors.at(cid);
        if (connector) {
            connectors[cid]->addPin(viewID, svgID, viewLayerID, terminalID, legID, hybrid);
        }
    }

    query = db.exec("SELECT COUNT(*) FROM buses");
    debugError(query.isActive(), query);
    if (!query.isActive() || !query.next()) return false;

    int busCount = query.value(0).toInt();
    if (busCount == 0) return false;

    QVector<BusShared *> buses(busCount + 1, NULL);
    QHash<BusShared *, qulonglong> busids;

    query = db.exec("SELECT id, name, part_id FROM buses");
    debugError(query.isActive(), query);
    if (!query.isActive()) {
        killConnectors(connectors);
        killBuses(buses);
        return false;
    }

    while (query.next()) {
        int ix = 0;
        qulonglong bid = query.value(ix++).toULongLong();
        QString name = query.value(ix++).toString();
        qulonglong dbid = query.value(ix++).toULongLong();
        ModelPart * modelPart = parts.at(dbid);
        if (modelPart) {
            BusShared * busShared = new BusShared(name);
            ModelPart * modelPart = parts.at(dbid);
            modelPart->modelPartShared()->insertBus(busShared);

            buses[bid] = busShared;
            busids.insert(busShared, dbid);
        }
    }

    query = db.exec("SELECT connectorid, bus_id FROM busmembers");
    debugError(query.isActive(), query);
    if (!query.isActive()) {
        killConnectors(connectors);
        killBuses(buses);
        return false;
    }

    while (query.next()) {
        int ix = 0;
        
        QString connectorid = query.value(ix++).toString();
        qulonglong bid = query.value(ix++).toULongLong();
        BusShared * busShared = buses[bid];
        if (busShared) {
            qulonglong dbid = busids.value(busShared);
            ModelPart * modelPart = parts.at(dbid);
            if (modelPart) {
                ConnectorShared * connectorShared = modelPart->modelPartShared()->getConnectorShared(connectorid);
                busShared->addConnectorShared(connectorShared);
            }
        }
    }

    query = db.exec("SELECT subpart_id, part_id FROM schematic_subparts");
    debugError(query.isActive(), query);
    if (query.isActive()) {
        while (query.next()) {
            int ix = 0;
            QString subpartID = query.value(ix++).toString();
            qulonglong dbid = query.value(ix++).toULongLong();
            ModelPart * modelPart = parts.at(dbid);
            if (modelPart) {
                QString subModuleID = modelPart->moduleID() + "_" + subpartID;
                ModelPart * subModelPart = m_partHash.value(subModuleID);
                if (subModelPart) {
                    subModelPart->setSubpartID(subpartID);
                    modelPart->modelPartShared()->addSubpart(subModelPart->modelPartShared());
                }
            }
        }
    }

    if (m_root == NULL) {
        m_root = new ModelPart();
    }
    foreach (ModelPart * modelPart, m_partHash.values()) {
        if (modelPart->dbid() != 0) {
            // initConnectors is not redundant here
            // there may be parts in m_partHash loaded from a file rather from the database
            // 
            modelPart->initConnectors();    
            modelPart->flipSMDAnd();
            modelPart->initBuses();
            modelPart->setParent(m_root);
            modelPart->lookForZeroConnector();
        }
    }

    return true;
}


SqliteReferenceModel::~SqliteReferenceModel() {
	deleteConnection();
}

void SqliteReferenceModel::initParts(bool dbExists) {
	m_init = true;
	PaletteModel::initParts(dbExists);
	m_init = false;
}

bool SqliteReferenceModel::createDatabase(const QString & databaseName, bool fullLoad) {
	m_swappingEnabled = true;
	m_database = QSqlDatabase::addDatabase("QSQLITE");
	m_database.setDatabaseName(databaseName.isEmpty() ? ":memory:" : databaseName);
	if (!m_database.open()) {
		m_swappingEnabled = false;
	} 
    else {
        m_keepGoing = false;
        bool gotTransaction = m_database.transaction();
        DebugDialog::debug(gotTransaction ? "got transaction" : "no transaction");

		bool result = createParts(m_database, fullLoad);

        QSqlQuery query;
        result = query.exec("CREATE TABLE lastcommit (\n"
            "id INTEGER PRIMARY KEY NOT NULL,\n"
            "sha TEXT NOT NULL"
        ")");
        debugError(result, query);

        result = query.exec("CREATE TABLE viewimages (\n"
			"id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,\n"
			"viewid INTEGER NOT NULL,\n"                     // ViewLayer::ViewID
			"image TEXT NOT NULL,\n"
			"layers INTEGER NOT NULL,\n"                     // bit flag (max 8 bytes = 64 layers)
			"sticky INTEGER NOT NULL,\n"                     // bit flag (max 8 bytes = 64 layers)
			"flipvertical INTEGER NOT NULL,\n"               // boolean
			"fliphorizontal INTEGER NOT NULL,\n"             // boolean
            "part_id INTEGER NOT NULL"
		")");
        debugError(result, query);

        result = query.exec("CREATE TABLE connectors (\n"
			"id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,\n"
			"connectorid TEXT NOT NULL,\n"
			"type INTEGER NOT NULL,\n"
			"name TEXT NOT NULL,\n"
			"description TEXT,\n"
			"replacedby TEXT,\n"
            "part_id INTEGER NOT NULL"
		")");
        debugError(result, query);

        result = query.exec("CREATE TABLE connectorlayers (\n"
			"id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,\n"
			"view INTEGER NOT NULL,\n"                      // ViewLayer::ViewID
			"layer INTEGER NOT NULL,\n"                     // ViewLayer::ViewLayerID
			"svgid TEXT NOT NULL,\n"
			"hybrid INTEGER NOT NULL,\n"
			"terminalid TEXT,\n"
			"legid TEXT,\n"
            "connector_id INTEGER NOT NULL"
		")");
        debugError(result, query);

        result = query.exec("CREATE TABLE buses (\n"
			"id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,\n"
			"name TEXT NOT NULL,\n"
            "part_id INTEGER NOT NULL"
		")");
        debugError(result, query);

        result = query.exec("CREATE TABLE busmembers (\n"
			"id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,\n"
			"connectorid TEXT NOT NULL,\n"
            "bus_id INTEGER NOT NULL"
		")");
        debugError(result, query);

        result = query.exec("CREATE TABLE schematic_subparts (\n"
			"id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,\n"
			"label TEXT NOT NULL,\n"
			"subpart_id TEXT NOT NULL,\n"
            "part_id INTEGER NOT NULL"
		")");
        debugError(result, query);

        // TODO: create a dictionary table so redundant tags, property names, and property values aren't stored multiple times

		result = query.exec("CREATE TABLE tags (\n"
			"id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL ,\n"
			"tag TEXT NOT NULL,\n"
            "part_id INTEGER NOT NULL"
		")");
        debugError(result, query);

        result = createProperties(m_database);

		result = query.exec("CREATE TRIGGER unique_part__moduleID \n"
			"BEFORE INSERT ON parts \n"
			"FOR EACH ROW BEGIN \n"
				"SELECT RAISE(ROLLBACK, 'insert on table \"parts\" violates unique constraint \"unique_part__moduleID\"') \n"
					"WHERE (SELECT count(*) FROM parts WHERE moduleID = NEW.moduleID) > 0; \n"
			"END; "
		);
        debugError(result, query);

        if (fullLoad) {
            QSqlQuery query;
            query.prepare("INSERT INTO lastcommit(id, sha) VALUES (:id, :sha)");
            query.bindValue(":id", 0);
            query.bindValue(":sha", m_sha);
            result = query.exec();
            debugError(result, query);

		    foreach(ModelPart* mp, m_partHash.values()) {
			    mp->initConnectors(false);
            }                      
		}

		foreach(ModelPart* mp, m_partHash.values()) {
			addPartAux(mp, fullLoad);
		}

        createIndexes();
        createMoreIndexes(m_database);

		m_database.commit();

	}
	return m_swappingEnabled;
}

void SqliteReferenceModel::deleteConnection() {
	QSqlDatabase::removeDatabase("SQLITE");
}

ModelPart *SqliteReferenceModel::loadPart(const QString & path, bool update) {
	ModelPart *modelPart = PaletteModel::loadPart(path, update);
	if (modelPart == NULL) return modelPart;

	if (!m_init) addPart(modelPart, update);
	return modelPart;
}

ModelPart *SqliteReferenceModel::retrieveModelPart(const QString &moduleID) {
	if (moduleID.isEmpty()) {
		return NULL;
	}
	return m_partHash.value(moduleID, NULL);
}

QString SqliteReferenceModel::retrieveModuleIdWith(const QString &family, const QString &propertyName, bool closestMatch) {
	QString moduleID = retrieveModuleId(family,m_recordedProperties,propertyName, closestMatch);
	m_recordedProperties.clear();
	return moduleID;
}

QString SqliteReferenceModel::retrieveModuleId(const QString &family, const QMultiHash<QString, QString> &properties, const QString &propertyName, bool closestMatch) 
{
	QString propertyValue;

	if(properties.size() > 0) {
		QString queryStr =
			"SELECT moduleID FROM parts part \n"
			"WHERE part.family = :family AND (1=1 ";
		QHash<QString, QString> params;
        int ix = 0;
		foreach (QString name, properties.uniqueKeys()) {
            foreach (QString value, properties.values(name)) {
			    QString propParam = QString(":prop%1").arg(ix++);
			    queryStr += QString(" AND EXISTS (SELECT * FROM properties prop WHERE prop.part_id = part.id AND prop.name = %1_name AND prop.value = %1_value)").arg(propParam);
			    params[propParam+"_name"] = name;
			    params[propParam+"_value"] = value;
			    if(name == propertyName) {
				    propertyValue = value;
			    }
            }
		}
		queryStr += ") order by part.core desc";

		QSqlQuery query;
		query.prepare(queryStr);

		query.bindValue(":family",family.toLower().trimmed());
		foreach(QString name, params.uniqueKeys()) {
			query.bindValue(name,params[name].toLower().trimmed());
		}

		QString moduleId;
		if(query.exec()) {
			if(query.next()) {
				moduleId =  query.value(0).toString(); //grab the first
			}
			//DebugDialog::debug("SQLITE: found: "+moduleId);
		} else {
            debugExec("couldn't retrieve part", query);
		}

		if(!moduleId.isEmpty()) {
			m_lastWasExactMatch = true;
			return moduleId;
		} else if(closestMatch || !propertyName.isEmpty()) {
			m_lastWasExactMatch = false;
			return closestMatchId(family, properties, propertyName, propertyValue);
		} else {
			return ___emptyString___;
		}
	} else {
		return ___emptyString___;
	}
}

QString SqliteReferenceModel::closestMatchId(const QString &family, const QMultiHash<QString, QString> &properties, const QString &propertyName, const QString &propertyValue) {
	QStringList possibleMatches = getPossibleMatches(family, properties, propertyName, propertyValue);
	return getClosestMatch(family, properties, possibleMatches);
}

QStringList SqliteReferenceModel::getPossibleMatches(const QString &family, const QMultiHash<QString, QString> &properties, const QString &propertyName, const QString &propertyValue) {
	Q_UNUSED(properties);

    QStringList result;
	QString queryStr =
		"SELECT moduleID FROM parts part \n"
		"WHERE part.family = :family AND EXISTS ( \n"
			"SELECT * FROM properties prop \n"
			"WHERE prop.part_id = part.id %1 \n"
		") ";
	queryStr = queryStr.arg((propertyName.isEmpty()) ? "" : "AND prop.name = :prop_name  AND prop.value = :prop_value ");
	QSqlQuery query;
	query.prepare(queryStr);

	query.bindValue(":family", family.toLower().trimmed());
	if (!propertyName.isEmpty()) {
		query.bindValue(":prop_name", propertyName.toLower().trimmed());
		query.bindValue(":prop_value", propertyValue);
	}


	if(query.exec()) {
		while(query.next()) {
			result << query.value(0).toString();
		}
		//DebugDialog::debug(QString("SQLITE: %1 possible matches found").arg(result.size()));
	} else {
        debugExec("couldn't get possible match", query);
	}

	return result;
}

QString SqliteReferenceModel::getClosestMatch(const QString &family, const QMultiHash<QString, QString> &properties, QStringList possibleMatches) {
	int propsInCommonCount = 0;
	int propsInCommonCountAux = 0;
	QString result;
	foreach(QString modId, possibleMatches) {
		propsInCommonCountAux = countPropsInCommon(family, properties, retrieveModelPart(modId));
		if(propsInCommonCountAux > propsInCommonCount) {
			result = modId;
			propsInCommonCount = propsInCommonCountAux;
		}
	}
	return result;
}

int SqliteReferenceModel::countPropsInCommon(const QString &family, const QMultiHash<QString, QString> &properties, const ModelPart *part2) {
	Q_UNUSED(family)
        
    if (part2 == NULL) {
		DebugDialog::debug("countPropsInCommon failure");
		return 0;
	}

	int result = 0;
	QMultiHash<QString,QString> props2 = part2->properties();
	foreach(QString prop, properties.uniqueKeys()) {
        QStringList values1 = properties.values(prop);
		QStringList values2 = props2.values(prop);
        foreach (QString value1, values1) {
		    if (values2.contains(value1)) {
			 result++;
            }
		}
	}
	return result;
}

bool SqliteReferenceModel::lastWasExactMatch() {
	return m_lastWasExactMatch;
}

bool SqliteReferenceModel::addPartAux(ModelPart * newModel, bool fullLoad) {
	try {
		bool result = insertPart(newModel, fullLoad);
		return result;
	}
	catch (const char * msg) {
		DebugDialog::debug(msg);
	}
	catch (const QString & msg) {
		DebugDialog::debug(msg);
	}
	catch (...) {
		DebugDialog::debug("SqliteReferenceModel::addPartAux failure");
	}

    return false;
}

bool SqliteReferenceModel::addPart(ModelPart * newModel, bool update) {
	bool result;
	if(update && containsModelPart(newModel->moduleID())) {
		result = updatePart(newModel);
	} else {
		result = addPartAux(newModel, false);
	}
	return result;
}

ModelPart * SqliteReferenceModel::addPart(QString newPartPath, bool addToReference, bool updateIdAlreadyExists)
{
	return PaletteModel::addPart(newPartPath, addToReference, updateIdAlreadyExists);
}

bool SqliteReferenceModel::removePart(const QString &moduleId) {
    m_partHash.remove(moduleId);
    return removePartFromDataBase(moduleId);
}

bool SqliteReferenceModel::removePartFromDataBase(const QString & moduleId) {
    qulonglong partId = this->partId(moduleId);
    if(partId == NO_ID) return false;

    removePart(partId);
    removeProperties(partId);
    removeViewImages(partId);
    removeConnectors(partId);
    removeBuses(partId);
    removeSchematicSubparts(partId);
    removeTags(partId);

    return true;
}

ModelPart * SqliteReferenceModel::reloadPart(const QString & path, const QString & moduleID) {
    m_partHash.remove(moduleID);
    ModelPart *modelPart = PaletteModel::loadPart(path, false);
	if (modelPart == NULL) return modelPart;

	updatePart(modelPart);
	return modelPart;
}

bool SqliteReferenceModel::updatePart(ModelPart * newModel) {
	if(m_swappingEnabled) {
        if (removePartFromDataBase(newModel->moduleID())) {
			return addPartAux(newModel, false);
		} else {
			return false;
		}
	} else {
		return false;
	}
}

bool SqliteReferenceModel::insertPart(ModelPart * modelPart, bool fullLoad) {
    DebugModelPart = modelPart;

    QHash<QString, QString> properties = modelPart->properties();
	QSqlQuery query;
    QString fields;
    QString values;
    if (fullLoad) {
        fields =  " version,  replacedby,  fritzingversion,  author,  title,  label,  date,  description,  spice,  spicemodel,  taxonomy,  itemtype,  path";
        values = " :version, :replacedby, :fritzingversion, :author, :title, :label, :date, :description, :spice, :spicemodel, :taxonomy, :itemtype, :path";
    }
    else {
        fields =  " core";
        values = " :core";
    }
	query.prepare(QString("INSERT INTO parts(moduleID, family, %1) VALUES (:moduleID, :family, %2)").arg(fields).arg(values));
	query.bindValue(":moduleID", modelPart->moduleID());
	query.bindValue(":family", properties.value("family").toLower().trimmed());
    if (fullLoad) {
        QString path = modelPart->path();
        QString prefix = FolderUtils::getAppPartsSubFolderPath("");

        if (path.startsWith(ResourcePath)) {
        }
        else if (path.startsWith(prefix)) {
            path = path.mid(prefix.count() + 1);  // + 1 to remove the beginning "/"
        }
        else {
            bool bail = true;
            if (modelPart->itemType() == ModelPart::SchematicSubpart) {
                ModelPartShared * mps = modelPart->modelPartShared();
                if (mps && mps->superpart() && mps->superpart()->path().startsWith(prefix)) {
                    bail = false;
                }
            }

            if (bail) {
                DebugDialog::debug(QString("part path not in parts:%1 %2 %3").arg(path).arg(prefix).arg(fullLoad));
                return true;
            }
        }


        query.bindValue(":version", modelPart->version());
        query.bindValue(":replacedby", modelPart->replacedby());
	    query.bindValue(":fritzingversion", modelPart->fritzingVersion());
	    query.bindValue(":author", modelPart->author());
	    query.bindValue(":title", modelPart->title());
	    query.bindValue(":label", modelPart->label());
	    query.bindValue(":date", modelPart->date());
	    query.bindValue(":description", modelPart->description());
	    query.bindValue(":spice", modelPart->spice());
	    query.bindValue(":spicemodel", modelPart->spiceModel());
	    query.bindValue(":taxonomy", modelPart->taxonomy());
	    query.bindValue(":itemtype", (int) modelPart->itemType());
        
	    query.bindValue(":path", path);
    }
    else {
	    query.bindValue(":core", modelPart->isCore() ? "1" : "0");
    }


	if (query.exec()) {
        qulonglong id = query.lastInsertId().toULongLong();
        modelPart->setDBID(id);
        foreach (QString prop, properties.keys()) {
            if (prop == "family") continue;

			bool result = insertProperty(prop, properties.value(prop), id, modelPart->showInLabel(prop));
            if (fullLoad && !result) {
                m_swappingEnabled = false;
            }
            if (!result) {
                FailurePropertyMessages << tr("property '%1' in part '%2' with id '%3'.")
                    .arg(prop).arg(modelPart->path()).arg(modelPart->moduleID());
            }
		}

        if (fullLoad) {
            foreach (QString tag, modelPart->tags()) {
                insertTag(tag, id);
            }

            foreach (ViewImage * viewImage, modelPart->viewImages()) {
                insertViewImage(viewImage, id);
            }

            foreach (Connector * connector, modelPart->connectors().values()) {
                insertConnector(connector, id);
            }

            foreach (Bus * bus, modelPart->buses().values()) {
                insertBus(bus, id);
            }

            ModelPartShared * mps = modelPart->modelPartShared();
            if (mps) {
                foreach (ModelPartShared * sub, mps->subparts()) {
                    insertSubpart(sub, id);
                }
            }
        }
	} else {
        debugExec("couldn't insert part", query);
        FailurePartMessages << tr("part '%1' with id '%2'; possibly because it has no 'family' property.")
                                .arg(modelPart->path()).arg(modelPart->moduleID());
	}

    DebugModelPart = NULL;
	return true;
}

bool SqliteReferenceModel::insertProperty(const QString & name, const QString & value, qulonglong id, bool showInLabel) {
	QSqlQuery query;
	query.prepare("INSERT INTO properties(name, value, part_id, show_in_label) VALUES (:name, :value, :part_id, :show_in_label)");
	query.bindValue(":name", name.toLower().trimmed());
	query.bindValue(":value", value);
	query.bindValue(":part_id", id);
	query.bindValue(":show_in_label", showInLabel ? 1 : 0);
	if(!query.exec()) {
        debugExec("couldn't insert property", query);
		return false;
	} else {
		// qulonglong id = query.lastInsertId().toULongLong();
	}

	return true;
}

bool SqliteReferenceModel::insertTag(const QString & tag, qulonglong id) 
{
	QSqlQuery query;
	query.prepare("INSERT INTO tags(tag, part_id) VALUES (:tag, :part_id)");
	query.bindValue(":tag", tag.toLower().trimmed());
	query.bindValue(":part_id", id);
	if(!query.exec()) {
        debugExec("couldn't insert tag", query);
		m_swappingEnabled = false;
	} else {
		// qulonglong id = query.lastInsertId().toULongLong();
	}

	return true;
}

bool SqliteReferenceModel::insertViewImage(const ViewImage * viewImage, qulonglong id) 
{
    if (viewImage->image.isEmpty() && viewImage->layers == 0) return true;

	QSqlQuery query;
	query.prepare("INSERT INTO viewimages(viewid, image, layers, sticky, flipvertical, fliphorizontal, part_id) "
                    "VALUES (:viewid, :image, :layers, :sticky, :flipvertical, :fliphorizontal, :part_id)");
	query.bindValue(":viewid", viewImage->viewID);
	query.bindValue(":image", viewImage->image);
	query.bindValue(":layers", viewImage->layers);
	query.bindValue(":sticky", viewImage->sticky);
	query.bindValue(":flipvertical", viewImage->canFlipVertical ? 1 : 0);
	query.bindValue(":fliphorizontal", viewImage->canFlipHorizontal ? 1 : 0);
	query.bindValue(":part_id", id);
	if(!query.exec()) {
        debugExec("couldn't insert viewimage", query);
		m_swappingEnabled = false;
	} else {
		// qulonglong id = query.lastInsertId().toULongLong();
	}

	return true;
}

bool SqliteReferenceModel::insertBus(const Bus * bus, qulonglong id) 
{
	QSqlQuery query;
	query.prepare("INSERT INTO buses(name, part_id) VALUES (:name, :part_id)");
	query.bindValue(":name", bus->id());
	query.bindValue(":part_id", id);
	if(!query.exec()) {
        debugExec("couldn't insert bus", query);
		m_swappingEnabled = false;
	} else {
		qulonglong bid = query.lastInsertId().toULongLong();
        foreach (Connector * connector, bus->connectors()) {
            insertBusMember(connector, bid);
        }
	}

	return true;
}

bool SqliteReferenceModel::insertBusMember(const Connector * connector, qulonglong id) 
{
	QSqlQuery query;
	query.prepare("INSERT INTO busmembers(connectorid, bus_id) VALUES (:connectorid, :bus_id)");
	query.bindValue(":connectorid", connector->connectorSharedID());
	query.bindValue(":bus_id", id);
	if(!query.exec()) {
        debugExec("couldn't insert busmember", query);
		m_swappingEnabled = false;
	} else {
		//qulonglong id = query.lastInsertId().toULongLong();
	}

	return true;
}

bool SqliteReferenceModel::insertConnector(const Connector * connector, qulonglong id) 
{
	QSqlQuery query;
	query.prepare("INSERT INTO connectors(connectorid, type, name, description, replacedby, part_id) VALUES (:connectorid, :type, :name, :description, :replacedby, :part_id)");
	query.bindValue(":connectorid", connector->connectorSharedID());
	query.bindValue(":type", (int) connector->connectorType());
	query.bindValue(":name", connector->connectorSharedName());
	query.bindValue(":description", connector->connectorSharedDescription());
	query.bindValue(":replacedby", connector->connectorSharedReplacedby());
	query.bindValue(":part_id", id);
	if(!query.exec()) {
        debugExec("couldn't insert connector", query);
		m_swappingEnabled = false;
	} else {
		qulonglong id = query.lastInsertId().toULongLong();
        foreach (SvgIdLayer * layer, connector->svgIdLayers()) {
            insertConnectorLayer(layer, id);
        }
	}

	return true;
}


bool SqliteReferenceModel::insertConnectorLayer(const SvgIdLayer * svgIdLayer, qulonglong id) 
{

	QSqlQuery query;
	query.prepare("INSERT INTO connectorLayers(view, layer, svgid, hybrid, terminalid, legid, connector_id) VALUES "
                                            "(:view, :layer, :svgid, :hybrid, :terminalid, :legid, :connector_id)");
	query.bindValue(":view", svgIdLayer->m_viewID);
	query.bindValue(":layer", svgIdLayer->m_svgViewLayerID);
	query.bindValue(":svgid", svgIdLayer->m_svgId);
	query.bindValue(":hybrid", svgIdLayer->m_hybrid ? 1 : 0);
	query.bindValue(":terminalid", svgIdLayer->m_terminalId);
	query.bindValue(":legid", svgIdLayer->m_legId);
	query.bindValue(":connector_id", id);
	if(!query.exec()) {
        debugExec("couldn't insert viewimage", query);
		m_swappingEnabled = false;
	} else {
		// qulonglong id = query.lastInsertId().toULongLong();
	}

	return true;
}

QStringList SqliteReferenceModel::propValues(const QString &family, const QString &propName, bool distinct) {
	QStringList retval;

	QSqlQuery query;
	query.prepare(QString(
		"SELECT %1 prop.value FROM properties prop JOIN parts PART ON part.id = prop.part_id \n"
		"WHERE part.family = :family AND prop.name = :propName ORDER BY prop.value \n"
		).arg(distinct ? " DISTINCT ":"")
	);
	query.bindValue(":family",family.toLower().trimmed());
	query.bindValue(":propName",propName.toLower().trimmed());

	if(query.exec()) {
		while(query.next()) {
            QString value = query.value(0).toString();
            if (!value.isEmpty()) retval << value;
		}
	} else {
        debugExec("couldn't retrieve values", query);
		m_swappingEnabled = false;
	}

	return retval;
}

QMultiHash<QString, QString> SqliteReferenceModel::allPropValues(const QString &family, const QString &propName) {
	QMultiHash<QString, QString> retval;

	QSqlQuery query;
	query.prepare(QString(
		"SELECT value, moduleID FROM properties prop JOIN parts part ON part.id = prop.part_id \n"
		"WHERE part.family = :family AND prop.name = :propName\n"
		)
	);
	query.bindValue(":family",family.toLower().trimmed());
	query.bindValue(":propName",propName.toLower().trimmed());

	if(query.exec()) {
		while(query.next()) {
			//QSqlRecord record = query.record();
			//for (int i = 0; i < record.count(); i++) {
			//	DebugDialog::debug("result " + record.fieldName(i) + " " + record.value(i).toString());
			//}
            QString prop = query.value(0).toString();
            if (!prop.isEmpty()) retval.insert(prop, query.value(1).toString());
		}
	} else {
        debugExec("couldn't retrieve values", query);
		m_swappingEnabled = false;
	}

	return retval;
}

void SqliteReferenceModel::recordProperty(const QString &name, const QString &value) {
	DebugDialog::debug(QString("RECORDING PROPERTY %1:%2").arg(name).arg(value));
	m_recordedProperties.insert(name,value);
}

bool SqliteReferenceModel::swapEnabled() const {
	return m_swappingEnabled;
}

bool SqliteReferenceModel::containsModelPart(const QString & moduleID) {
	return partId(moduleID) != NO_ID;
}

qulonglong SqliteReferenceModel::partId(QString moduleID) {
	qulonglong partId = NO_ID;

	QSqlQuery query;
	query.prepare(
		"SELECT id FROM parts \n"
		"WHERE moduleID = :moduleID "
	);
	query.bindValue(":moduleID",moduleID);

	if(query.exec()) {
		//DebugDialog::debug("SQLITE: retrieving id: "+moduleID);
		if(query.next()) {
			partId =  query.value(0).toULongLong(); //grab the first
		}
		//DebugDialog::debug(QString("SQLITE: found: %1").arg(partId));
	} else {
        debugExec("couldn't retrieve part", query);
	}

	return partId;
}

bool SqliteReferenceModel::removePart(qulonglong partId) {
    return removex(partId, "parts", "id");
}

bool SqliteReferenceModel::removeProperties(qulonglong partId) {
    return removex(partId, "properties", "part_id");
}

QString SqliteReferenceModel::partTitle(const QString & moduleID) {
	ModelPart *mp = retrieveModelPart(moduleID);
	if(mp) {
		return mp->modelPartShared()->title();
	} else {
		return ___emptyString___;
	}
}

void SqliteReferenceModel::killParts()
{
    foreach (ModelPart * modelPart, m_partHash.values()) {
        delete modelPart;
    }
    m_partHash.clear();
}

bool SqliteReferenceModel::createProperties(QSqlDatabase & db) {
	QSqlQuery query = db.exec("CREATE TABLE properties (\n"
			"id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL ,\n"
			"name TEXT NOT NULL,\n"
			"value TEXT NOT NULL,\n"
			"show_in_label INTEGER NOT NULL,\n"
			"part_id INTEGER NOT NULL"
		")");
    debugError(query.isActive(), query);
    return query.isActive();
}

bool SqliteReferenceModel::createParts(QSqlDatabase & db, bool fullLoad) 
{  
    QString extra;
    if (fullLoad) {
        extra = "version TEXT,\n"
            "replacedby TEXT,\n"
	        "fritzingversion TEXT,\n"
            "author TEXT,\n"
			"title TEXT,\n"
			"label TEXT,\n"
			"date TEXT,\n"
			"description TEXT,\n"
			"spice TEXT,\n"
			"spicemodel TEXT,\n"
			"taxonomy TEXT,\n"
			"itemtype INTEGER NOT NULL,\n"
			"path TEXT\n"
          ;
    }
    else {
        extra = "core TEXT NOT NULL\n"
        ;
    }


    QString string = QString("CREATE TABLE parts (\n"
			"id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,\n"
			"moduleID TEXT NOT NULL,\n"
			"family TEXT NOT NULL,\n"
            "%1"
		    ")")
          .arg(extra);

    QSqlQuery query = db.exec(string);
    debugError(query.isActive(), query);
    return query.isActive();
}

bool SqliteReferenceModel::insertSubpart(ModelPartShared * mps, qulonglong id) 
{
	QSqlQuery query;
	query.prepare("INSERT INTO schematic_subparts(label, subpart_id, part_id) VALUES (:label, :subpart_id, :part_id)");
	query.bindValue(":label", mps->label());
	query.bindValue(":subpart_id", mps->subpartID());
	query.bindValue(":part_id", id);
	if(!query.exec()) {
        debugExec("couldn't insert bus", query);
		m_swappingEnabled = false;
	} 
    //else {
	//	qulonglong sid = query.lastInsertId().toULongLong();
	//}

	return true;
}

void SqliteReferenceModel::createIndexes() {
    // supposedly faster to add these after the data is inserted

    QSqlQuery query;
	bool result = query.exec("CREATE INDEX idx_viewimage_part_id ON viewimages (part_id ASC)");
    debugError(result, query);
	result = query.exec("CREATE INDEX idx_connector_part_id ON connectors (part_id ASC)");
    debugError(result, query);
	result = query.exec("CREATE INDEX idx_connectorlayer_connector_id ON connectorlayers (connector_id ASC)");
    debugError(result, query);
	result = query.exec("CREATE INDEX idx_bus_part_id ON buses (part_id ASC)");
    debugError(result, query);
	result = query.exec("CREATE INDEX idx_busmember_bus_id ON busmembers (bus_id ASC)");
    debugError(result, query);
	result = query.exec("CREATE INDEX idx_schematic_subpart_part_id ON schematic_subparts (part_id ASC)");
    debugError(result, query);
	result = query.exec("CREATE INDEX idx_tag_part_id ON tags (part_id ASC)");
    debugError(result, query);
}

void SqliteReferenceModel::createMoreIndexes(QSqlDatabase & db)
{
    // supposedly faster to add these after the data is inserted

    QSqlQuery query = db.exec("CREATE INDEX idx_property_name ON properties (name ASC)");
    debugError(query.isActive(), query);

	query = db.exec("CREATE INDEX idx_part_id ON parts (id ASC)");
    debugError(query.isActive(), query);

    query = db.exec("CREATE INDEX idx_part_moduleID ON parts (moduleID ASC)");
    debugError(query.isActive(), query);

    query = db.exec("CREATE INDEX idx_part_family ON parts (family ASC)");
    debugError(query.isActive(), query);
}

void SqliteReferenceModel::setSha(const QString & sha) {
    m_sha = sha;
}

const QString & SqliteReferenceModel::sha() const {
    return m_sha;
}

bool SqliteReferenceModel::removeViewImages(qulonglong partId) {
    return removex(partId, "viewimages", "part_id");
}

bool SqliteReferenceModel::removeConnectors(qulonglong partId) {
    QSqlQuery query;
    query.prepare("SELECT id from connectors WHERE part_id = :id");
    query.bindValue(":id",partId);
    if (query.exec()) {
        while (query.next()) {
            qulonglong cid = query.value(0).toULongLong();
            removex(cid, "connectorlayers", "connector_id");
        }
    }


    return removex(partId, "connectors", "part_id");
}

bool SqliteReferenceModel::removeBuses(qulonglong partId) {
    QSqlQuery query;
    query.prepare("SELECT id from buses WHERE part_id = :id");
    query.bindValue(":id",partId);
    if (query.exec()) {
        while (query.next()) {
            qulonglong bid = query.value(0).toULongLong();
            removex(bid, "busmembers", "bus_id");
        }
    }

    return removex(partId, "buses", "part_id");
}

bool SqliteReferenceModel::removeSchematicSubparts(qulonglong partId) {
    return removex(partId, "schematic_subparts", "part_id");
}


bool SqliteReferenceModel::removeTags(qulonglong partId) {
    return removex(partId, "tags", "part_id");
}

bool SqliteReferenceModel::removex(qulonglong id, const QString & tableName, const QString & idName) {
    bool result = true;

    QSqlQuery query;
    query.prepare(QString("DELETE FROM %1 WHERE %2 = :id").arg(tableName).arg(idName));
    query.bindValue(":id", id);

    if(query.exec()) {
        result = true;
    } else {
        DebugDialog::debug(
            "SQLITE: couldn't delete from " + tableName + "\n"
            "\t "+query.lastQuery()+"\n"
            "\t ERROR DRIVER: "+query.lastError().driverText()+"\n"
            "\t ERROR DB: "+query.lastError().databaseText()+"\n"
        );
        result = false;
    }

    return result;
}

