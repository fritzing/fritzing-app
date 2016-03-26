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
GNU General Public License for more details.old

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 6963 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-14 10:13:52 +0200 (So, 14. Apr 2013) $

********************************************************************/

#include "modelbase.h"
#include "../debugdialog.h"
#include "../items/partfactory.h"
#include "../items/moduleidnames.h"
#include "../utils/textutils.h"
#include "../utils/folderutils.h"
#include "../utils/fmessagebox.h"
#include "../version/version.h"
#include "../viewgeometry.h"

#include <QMessageBox>

QList<QString> ModelBase::CoreList;

/////////////////////////////////////////////////

ModelBase::ModelBase( bool makeRoot )
{
    m_checkForReversedWires = m_useOldSchematics = false;
	m_reportMissingModules = true;
	m_referenceModel = NULL;
	m_root = NULL;
	if (makeRoot) {
		m_root = new ModelPart();
		m_root->setModelPartShared(new ModelPartSharedRoot());
	}
}

ModelBase::~ModelBase() {
	if (m_root) {
        ModelPartShared * modelPartShared = m_root->modelPartShared();
        if (modelPartShared) {
            m_root->setModelPartShared(NULL);
            delete modelPartShared;
        }
		delete m_root;
	}
}

ModelPart * ModelBase::root() {
	return m_root;
}

ModelPart * ModelBase::retrieveModelPart(const QString & /* moduleID */)  {
	return NULL;
}

// loads a model from an fz file--assumes a reference model exists with all parts
bool ModelBase::loadFromFile(const QString & fileName, ModelBase * referenceModel, QList<ModelPart *> & modelParts, bool checkViews) {
	m_referenceModel = referenceModel;

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        FMessageBox::warning(NULL, QObject::tr("Fritzing"),
                             QObject::tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QString errorStr;
    int errorLine;
    int errorColumn;
    QDomDocument domDocument;

    if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
        FMessageBox::information(NULL, QObject::tr("Fritzing"),
                                 QObject::tr("Parse error (1) at line %1, column %2:\n%3\n%4")
                                 .arg(errorLine)
                                 .arg(errorColumn)
                                 .arg(errorStr)
								 .arg(fileName));
        return false;
    }

    QDomElement root = domDocument.documentElement();
   	if (root.isNull()) {
        FMessageBox::information(NULL, QObject::tr("Fritzing"), QObject::tr("The file %1 is not a Fritzing file (2).").arg(fileName));
   		return false;
	}

	emit loadedRoot(fileName, this, root);

    if (root.tagName() != "module") {
        FMessageBox::information(NULL, QObject::tr("Fritzing"), QObject::tr("The file %1 is not a Fritzing file (4).").arg(fileName));
        return false;
    }

    bool checkForOldSchematics = true;
	bool checkForRats = true;
	bool checkForTraces = true;
    bool checkForMysteryParts = true;
    bool checkForObsoleteSMDOrientation = true;
	m_fritzingVersion = root.attribute("fritzingVersion");
	if (m_fritzingVersion.length() > 0) {
		// with version 0.4.3 ratsnests in fz files are obsolete
		VersionThing versionThingRats;
		versionThingRats.majorVersion = 0;
		versionThingRats.minorVersion = 4;
		versionThingRats.minorSubVersion = 2;
		versionThingRats.releaseModifier = "";
		VersionThing versionThingFz;
		Version::toVersionThing(m_fritzingVersion,versionThingFz);
		checkForRats = !Version::greaterThan(versionThingRats, versionThingFz);
		// with version 0.6.5 traces are copied to all views
		versionThingRats.minorVersion = 6;
		versionThingRats.minorSubVersion = 4;
		checkForTraces = !Version::greaterThan(versionThingRats, versionThingFz);
        // with version 0.7.6 mystery part spacing implementation changes
		versionThingRats.minorVersion = 7;
		versionThingRats.minorSubVersion = 5;
		checkForMysteryParts = !Version::greaterThan(versionThingRats, versionThingFz);
        // with version 0.8.0 flipSMD is horizontal
	    versionThingRats.minorVersion = 7;
	    versionThingRats.minorSubVersion = 13;
	    checkForObsoleteSMDOrientation = !Version::greaterThan(versionThingRats, versionThingFz);
        // with version 0.8.6 we get a new schematic template
        versionThingRats.minorVersion = 8;
        versionThingRats.minorSubVersion = 5;
        checkForOldSchematics = !Version::greaterThan(versionThingRats, versionThingFz);
        // with version 0.9.3 we don't have to worry about reversed wires
        versionThingRats.minorVersion = 9;
        versionThingRats.minorSubVersion = 2;
        m_checkForReversedWires = !Version::greaterThan(versionThingRats, versionThingFz);

	}
    
	ModelPartSharedRoot * modelPartSharedRoot = this->rootModelPartShared();

    QDomElement title = root.firstChildElement("title");
	if (!title.isNull()) {
		if (modelPartSharedRoot) {
			modelPartSharedRoot->setTitle(title.text());
		}
	}

    // ensures changeBinIcon() is not available
    // this may be a bug?
	QString iconFilename = root.attribute("icon");
	if (iconFilename.isEmpty()) {
		iconFilename = title.text() + ".png";
	}

	if (!iconFilename.isEmpty()) {
		if (modelPartSharedRoot) {
			modelPartSharedRoot->setIcon(iconFilename);
		}
	}

	QString searchTerm = root.attribute("search");
	if (!searchTerm.isEmpty() && modelPartSharedRoot != NULL) {
		modelPartSharedRoot->setSearchTerm(searchTerm);
	}

    QDomElement views = root.firstChildElement("views");
	emit loadedViews(this, views);

	QDomElement instances = root.firstChildElement("instances");
	if (instances.isNull()) {
        FMessageBox::information(NULL, QObject::tr("Fritzing"), QObject::tr("The file %1 is not a Fritzing file (3).").arg(fileName));
        return false;
	}

	// delete any aready-existing model parts
    for (int i = m_root->children().count() - 1; i >= 0; i--) {
    	QObject* child = m_root->children()[i];
    	child->setParent(NULL);
    	delete child;
   	}

	emit loadingInstances(this, instances);

	if (checkForRats) {
		QDomElement instance = instances.firstChildElement("instance");
   		while (!instance.isNull()) {
			QDomElement nextInstance = instance.nextSiblingElement("instance");
			if (isRatsnest(instance)) {
				instances.removeChild(instance);
			}
			
			instance = nextInstance;
		}
	}

	if (checkForTraces) {
		QDomElement instance = instances.firstChildElement("instance");
   		while (!instance.isNull()) {
			checkTraces(instance);
			instance = instance.nextSiblingElement("instance");
		}
	}

	if (checkForMysteryParts) {
		QDomElement instance = instances.firstChildElement("instance");
   		while (!instance.isNull()) {
			checkMystery(instance);
			instance = instance.nextSiblingElement("instance");
		}
	}

    if (checkForObsoleteSMDOrientation) {
		QDomElement instance = instances.firstChildElement("instance");
   		while (!instance.isNull()) {
			if (checkObsoleteOrientation(instance)) {
                emit obsoleteSMDOrientationSignal();
                break;
            }
			instance = instance.nextSiblingElement("instance");
		}
    }

    m_useOldSchematics = false;
    if (checkForOldSchematics) {
        QDomElement instance = instances.firstChildElement("instance");
   		while (!instance.isNull()) {
			if (checkOldSchematics(instance)) {
                emit oldSchematicsSignal(fileName, m_useOldSchematics);
                break;
            }
			instance = instance.nextSiblingElement("instance");
		}    
    }

	bool result = loadInstances(domDocument, instances, modelParts, checkViews);
	emit loadedInstances(this, instances);
	return result;
}

ModelPart * ModelBase::fixObsoleteModuleID(QDomDocument & domDocument, QDomElement & instance, QString & moduleIDRef) {
	return PartFactory::fixObsoleteModuleID(domDocument, instance, moduleIDRef, m_referenceModel);
}

bool ModelBase::loadInstances(QDomDocument & domDocument, QDomElement & instances, QList<ModelPart *> & modelParts, bool checkViews)
{
	QHash<QString, QString> missingModules;
   	QDomElement instance = instances.firstChildElement("instance");
   	ModelPart* modelPart = NULL;
   	while (!instance.isNull()) {
		emit loadingInstance(this, instance);

        if (checkViews) {
            QDomElement views = instance.firstChildElement("views");
            QDomElement view = views.firstChildElement();
            if (views.isNull() || view.isNull()) {
                // do not load a part with no views
                //QString text;
                //QTextStream stream(&text);
                //instance.save(stream, 0);
                //DebugDialog::debug(text);
                instance = instance.nextSiblingElement("instance");
                continue;
            }
        }

   		// for now assume all parts are in the palette
   		QString moduleIDRef = instance.attribute("moduleIdRef");

        //DebugDialog::debug("loading " + moduleIDRef);
		if (moduleIDRef.compare(ModuleIDNames::SpacerModuleIDName) == 0) {
			ModelPart * mp = new ModelPart(ModelPart::Space);
			mp->setInstanceText(instance.attribute("path"));
			mp->setParent(m_root);
            mp->modelPartShared()->setModuleID(ModuleIDNames::SpacerModuleIDName);
            mp->modelPartShared()->setPath(instance.attribute("path"));
			modelParts.append(mp);
			instance = instance.nextSiblingElement("instance");
			continue;
		}

        bool generated = false;
   		modelPart = m_referenceModel->retrieveModelPart(moduleIDRef);
        if (modelPart == NULL) {
			DebugDialog::debug(QString("module id %1 not found in database").arg(moduleIDRef));
			modelPart = fixObsoleteModuleID(domDocument, instance, moduleIDRef);
			if (modelPart == NULL) {
                modelPart = genFZP(moduleIDRef, m_referenceModel);
				if (modelPart != NULL) {
                    instance.setAttribute("moduleIdRef", modelPart->moduleID());
                    moduleIDRef = modelPart->moduleID();
                    generated = true;
				}
				if (modelPart == NULL) {
					missingModules.insert(moduleIDRef, instance.attribute("path"));
   					instance = instance.nextSiblingElement("instance");
   					continue;
				}
			}
   		}

        if (modelPart->isCore() && m_useOldSchematics) {
            modelPart = createOldSchematicPart(modelPart, moduleIDRef);
        }

        modelPart->setInBin(true);
   		modelPart = addModelPart(m_root, modelPart);
   		modelPart->setInstanceDomElement(instance);
		modelParts.append(modelPart);

   		// TODO Mariano: i think this is not the way
   		QString instanceTitle = instance.firstChildElement("title").text();
   		if(!instanceTitle.isNull() && !instanceTitle.isEmpty()) {
   			modelPart->setInstanceTitle(instanceTitle, false);
   		}

		QDomElement localConnectors = instance.firstChildElement("localConnectors");
		QDomElement localConnector = localConnectors.firstChildElement("localConnector");
		while (!localConnector.isNull()) {
			modelPart->setConnectorLocalName(localConnector.attribute("id"), localConnector.attribute("name"));
			localConnector = localConnector.nextSiblingElement("localConnector");
		}

   		QString instanceText = instance.firstChildElement("text").text();
   		if(!instanceText.isNull() && !instanceText.isEmpty()) {
   			modelPart->setInstanceText(instanceText);
   		}

   		bool ok;
   		long index = instance.attribute("modelIndex").toLong(&ok);
   		if (ok) {
			// set the index so we can find the same model part later, as we continue loading
			modelPart->setModelIndex(index);
  		}

		// note: this QDomNamedNodeMap loop is obsolete, but leaving it here so that old sketches don't get broken (jc, 22 Oct 2009)
		QDomNamedNodeMap map = instance.attributes();
		for (int m = 0; m < map.count(); m++) {
			QDomNode node = map.item(m);
			QString nodeName = node.nodeName();

			if (nodeName.isEmpty()) continue;
			if (nodeName.compare("moduleIdRef") == 0) continue;
			if (nodeName.compare("modelIndex") == 0) continue;
			if (nodeName.compare("originalModelIndex") == 0) continue;
			if (nodeName.compare("path") == 0) continue;

			modelPart->setLocalProp(nodeName, node.nodeValue());
		}

		// "property" loop replaces previous QDomNamedNodeMap loop (jc, 22 Oct 2009)
		QDomElement prop = instance.firstChildElement("property");
		while(!prop.isNull()) {
			QString name = prop.attribute("name");
			if (!name.isEmpty()) {
				QString value = prop.attribute("value");
				if (!value.isEmpty()) {
					modelPart->setLocalProp(name, value);
				}
			}

			prop = prop.nextSiblingElement("property");
		}

   		instance = instance.nextSiblingElement("instance");
  	}

	if (m_reportMissingModules && missingModules.count() > 0) {
		QString unableToFind = QString("<html><body><b>%1</b><br/><table style='border-spacing: 0px 12px;'>")
			.arg(tr("Unable to find the following %n part(s):", "", missingModules.count()));
		foreach (QString key, missingModules.keys()) {
			unableToFind += QString("<tr><td>'%1'</td><td><b>%2</b></td><td>'%3'</td></tr>")
				.arg(key).arg(tr("at")).arg(missingModules.value(key, ""));
		}
		unableToFind += "</table></body></html>";
		FMessageBox::warning(NULL, QObject::tr("Fritzing"), unableToFind);
	}


	return true;
}

ModelPart * ModelBase::addModelPart(ModelPart * parent, ModelPart * copyChild) {

    //if (copyChild->moduleID() == "df9d072afa2b594ac67b60b4153ff57b_29") {
    //    DebugDialog::debug("alive in here");
    //}

	ModelPart * modelPart = new ModelPart();
	modelPart->copyNew(copyChild);
	modelPart->setParent(parent);
	modelPart->initConnectors();
    modelPart->flipSMDAnd();
	return modelPart;
}

ModelPart * ModelBase::addPart(QString newPartPath, bool addToReference) {
	Q_UNUSED(newPartPath);
	Q_UNUSED(addToReference);
	throw "ModelBase::addPart should not be invoked";
	return NULL;
}

ModelPart * ModelBase::addPart(QString newPartPath, bool addToReference, bool updateIdAlreadyExists)
{
	Q_UNUSED(updateIdAlreadyExists);
	Q_UNUSED(newPartPath);
	Q_UNUSED(addToReference);
	throw "ModelBase::addPart should not be invoked";
	return NULL;
}

// TODO Mariano: this function should never get called. Make pure virtual
bool ModelBase::addPart(ModelPart * modelPart, bool update) {
	Q_UNUSED(modelPart);
	Q_UNUSED(update);
	throw "ModelBase::addPart should not be invoked";
	return false;
}


void ModelBase::save(const QString & fileName, bool asPart) {
	QFileInfo info(fileName);
	QDir dir = info.absoluteDir();

	QString temp = dir.absoluteFilePath("temp.xml");
    QFile file1(temp);
    if (!file1.open(QFile::WriteOnly | QFile::Text)) {
        FMessageBox::warning(NULL, QObject::tr("Fritzing"),
                             QObject::tr("Cannot write file temp:\n%1\n%2\n%3.")
							  .arg(temp)
							  .arg(fileName)
                              .arg(file1.errorString())
							  );
        return;
    }

    QXmlStreamWriter streamWriter(&file1);
	save(fileName, streamWriter, asPart);
	file1.close();
	QFile original(fileName);
	if(original.exists() && !original.remove()) {
		file1.remove();
		FMessageBox::warning(
			NULL,
			tr("File save failed!"),
			tr("Couldn't overwrite file '%1'.\nReason: %2 (errcode %3)")
				.arg(fileName)
				.arg(original.errorString())
				.arg(original.error())
			);
		return;
	}
	file1.rename(fileName);
}

void ModelBase::save(const QString & fileName, QXmlStreamWriter & streamWriter, bool asPart) {
    streamWriter.setAutoFormatting(true);
    if(asPart) {
    	m_root->saveAsPart(streamWriter, true);
    } else {
    	m_root->saveInstances(fileName, streamWriter, true);
    }
}

bool ModelBase::paste(ModelBase * referenceModel, QByteArray & data, QList<ModelPart *> & modelParts, QHash<QString, QRectF> & boundingRects, bool preserveIndex)
{
	m_referenceModel = referenceModel;

	QDomDocument domDocument;
	QString errorStr;
	int errorLine;
	int errorColumn;
	bool result = domDocument.setContent(data, &errorStr, &errorLine, &errorColumn);
	if (!result) return false;

	QDomElement module = domDocument.documentElement();
	if (module.isNull()) {
		return false;
	}

	QDomElement boundingRectsElement = module.firstChildElement("boundingRects");
	if (!boundingRectsElement.isNull()) {
		QDomElement boundingRect = boundingRectsElement.firstChildElement("boundingRect");
		while (!boundingRect.isNull()) {
			QString name = boundingRect.attribute("name");
			QString rect = boundingRect.attribute("rect");
			QRectF br;
			if (!rect.isEmpty()) {
				QStringList s = rect.split(" ");
				if (s.count() == 4) {
					QRectF r(s[0].toDouble(), s[1].toDouble(), s[2].toDouble(), s[3].toDouble());
					br = r;
				}
			}
			boundingRects.insert(name, br);
			boundingRect = boundingRect.nextSiblingElement("boundingRect");
		}
	}

	QDomElement instances = module.firstChildElement("instances");
   	if (instances.isNull()) {
   		return false;
	}

	if (!preserveIndex) {
		// need to map modelIndexes from copied parts to new modelIndexes
		QHash<long, long> oldToNew;
		QDomElement instance = instances.firstChildElement("instance");
		while (!instance.isNull()) {
			long oldModelIndex = instance.attribute("modelIndex").toLong();
			oldToNew.insert(oldModelIndex, ModelPart::nextIndex());
			instance = instance.nextSiblingElement("instance");
		}
		renewModelIndexes(instances, "instance", oldToNew);
	}

	//QFile file("test.xml");
	//file.open(QFile::WriteOnly);
	//file.write(domDocument.toByteArray());
	//file.close();

	return loadInstances(domDocument, instances, modelParts, true);
}

void ModelBase::renewModelIndexes(QDomElement & parentElement, const QString & childName, QHash<long, long> & oldToNew)
{
	QDomElement instance = parentElement.firstChildElement(childName);
	while (!instance.isNull()) {
		long oldModelIndex = instance.attribute("modelIndex").toLong();
		instance.setAttribute("modelIndex", QString::number(oldToNew.value(oldModelIndex)));
		QDomElement views = instance.firstChildElement("views");
		if (!views.isNull()) {
			QDomElement view = views.firstChildElement();
			while (!view.isNull()) {
                bool ok;
                int superpart = view.attribute("superpart").toLong(&ok);
                if (ok) {
                    view.setAttribute("superpart", QString::number(oldToNew.value(superpart)));
                }
				QDomElement connectors = view.firstChildElement("connectors");
				if (!connectors.isNull()) {
					QDomElement connector = connectors.firstChildElement("connector");
					while (!connector.isNull()) {
						QDomElement connects = connector.firstChildElement("connects");
						if (!connects.isNull()) {
							QDomElement connect = connects.firstChildElement("connect");
							while (!connect.isNull()) {
								bool ok;
								oldModelIndex = connect.attribute("modelIndex").toLong(&ok);
								if (ok) {
									long newModelIndex = oldToNew.value(oldModelIndex, -1);
									if (newModelIndex != -1) {
										connect.setAttribute("modelIndex", QString::number(newModelIndex));
									}
									else {
										//DebugDialog::debug(QString("keep old model index %1").arg(oldModelIndex));
									}
								}
								connect = connect.nextSiblingElement("connect");
							}
						}
						connector = connector.nextSiblingElement("connector");
					}
				}

				view = view.nextSiblingElement();
			}
		}

		instance = instance.nextSiblingElement(childName);
	}
}

void ModelBase::setReportMissingModules(bool b) {
	m_reportMissingModules = b;
}

ModelPart * ModelBase::genFZP(const QString & moduleID, ModelBase * referenceModel) {
	QString path = PartFactory::getFzpFilename(moduleID);
	if (path.isEmpty()) return NULL;

	ModelPart* mp = referenceModel->addPart(path, true, true);
	if (mp) mp->setCore(true);
	return mp;
}

ModelPartSharedRoot * ModelBase::rootModelPartShared() {
	if (m_root == NULL) return NULL;

	return m_root->modelPartSharedRoot();
}

bool ModelBase::isRatsnest(QDomElement & instance) {
	QString moduleIDRef = instance.attribute("moduleIdRef");
	if (moduleIDRef.compare(ModuleIDNames::WireModuleIDName) != 0) return false;

	QDomElement views = instance.firstChildElement("views");
	if (views.isNull()) return false;

	QDomElement view = views.firstChildElement();
	while (!view.isNull()) {
		QDomElement geometry = view.firstChildElement("geometry");
		if (!geometry.isNull()) {
			int flags = geometry.attribute("wireFlags").toInt();
			if (flags & ViewGeometry::RatsnestFlag) {
				return true;
			}
			if (flags & ViewGeometry::ObsoleteJumperFlag) {
				return true;
			}
		}

		view = view.nextSiblingElement();
	}

	return false;
}


bool ModelBase::checkOldSchematics(QDomElement & instance)
{
	if (instance.attribute("moduleIdRef").compare(ModuleIDNames::WireModuleIDName) != 0) {
        return false;
    }

    QDomElement views = instance.firstChildElement("views");
    QDomElement schematicView = views.firstChildElement("schematicView");
    QDomElement geometry = schematicView.firstChildElement("geometry");
    if (geometry.isNull()) return false;

    int flags = geometry.attribute("wireFlags", "0").toInt();
    return (flags & ViewGeometry::SchematicTraceFlag) != 0;
}


bool ModelBase::checkObsoleteOrientation(QDomElement & instance)
{
	QString flippedSMD = instance.attribute("flippedSMD", "");
	if (flippedSMD != "true") return false;

	QDomElement views = instance.firstChildElement("views");
	QDomElement pcbView = views.firstChildElement("pcbView");
    return (pcbView.attribute("layer", "") == "copper0");
}

void ModelBase::checkTraces(QDomElement & instance) {
	QString moduleIDRef = instance.attribute("moduleIdRef");
	if (moduleIDRef.compare(ModuleIDNames::WireModuleIDName) != 0) return;

	QDomElement views = instance.firstChildElement("views");
	if (views.isNull()) return;

	QDomElement bbView = views.firstChildElement("breadboardView");
	QDomElement schView = views.firstChildElement("schematicView");
	QDomElement pcbView = views.firstChildElement("pcbView");

	if (!bbView.isNull() && !schView.isNull() && !pcbView.isNull()) {
		// if it's a breadboard wire; just make sure flag is correct

		QList<QDomElement> elements;
		elements << bbView << schView << pcbView;
		foreach (QDomElement element, elements) {
			QDomElement geometry = element.firstChildElement("geometry");
			if (!geometry.isNull()) {
				int flags = geometry.attribute("wireFlags").toInt();
				if (flags & ViewGeometry::PCBTraceFlag) return;				// not a breadboard wire, bail out
				if (flags & ViewGeometry::SchematicTraceFlag) return;		// not a breadboard wire, bail out

				if ((flags & ViewGeometry::NormalFlag) == 0) {
					flags |= ViewGeometry::NormalFlag;
					geometry.setAttribute("wireFlags", QString::number(flags));
				}
			}
		}


		return;
	}

	if (!bbView.isNull()) {
		QDomElement geometry = bbView.firstChildElement("geometry");
		if (!geometry.isNull()) {
			int flags = geometry.attribute("wireFlags").toInt();
			if ((flags & ViewGeometry::NormalFlag) == 0) {
				flags |= ViewGeometry::NormalFlag;
				geometry.setAttribute("wireFlags", QString::number(flags));
			}
		}
		schView = bbView.cloneNode(true).toElement();
		pcbView = bbView.cloneNode(true).toElement();
		schView.setTagName("schematicView");
		pcbView.setTagName("pcbView");
		views.appendChild(pcbView);
		views.appendChild(schView);
		return;
	}

	if (!schView.isNull()) {
		QDomElement geometry = schView.firstChildElement("geometry");
		if (!geometry.isNull()) {
			int flags = geometry.attribute("wireFlags").toInt();
			if (flags & ViewGeometry::PCBTraceFlag) {
				flags ^= ViewGeometry::PCBTraceFlag;
				flags |= ViewGeometry::SchematicTraceFlag;
				geometry.setAttribute("wireFlags", QString::number(flags));
			}
		}
		pcbView = schView.cloneNode(true).toElement();
		bbView = schView.cloneNode(true).toElement();
		pcbView.setTagName("pcbView");
		bbView.setTagName("breadboardView");
		views.appendChild(bbView);
		views.appendChild(pcbView);
		return;
	}
	
	if (!pcbView.isNull()) {
		schView = pcbView.cloneNode(true).toElement();
		bbView = pcbView.cloneNode(true).toElement();
		schView.setTagName("schematicView");
		bbView.setTagName("breadboardView");
		views.appendChild(bbView);
		views.appendChild(schView);
		return;
	}

	QDomElement iconView = views.firstChildElement("iconView");
	if (!iconView.isNull()) return;

	QString string;
	QTextStream stream(&string);
	instance.save(stream, 0);
	stream.flush();
	DebugDialog::debug(QString("no wire view elements in fz file %1").arg(string));	
}

const QString & ModelBase::fritzingVersion() {
	return m_fritzingVersion;
}

void ModelBase::setReferenceModel(ModelBase * modelBase) {
    m_referenceModel = modelBase;
}

void ModelBase::checkMystery(QDomElement & instance) 
{
	QString moduleIDRef = instance.attribute("moduleIdRef");
    bool mystery = false;
    bool sip = false;
    bool dip = false;
	if (moduleIDRef.contains("mystery", Qt::CaseInsensitive)) mystery = true;	
    else if (moduleIDRef.contains("sip", Qt::CaseInsensitive)) sip = true;    
    else if (moduleIDRef.contains("dip", Qt::CaseInsensitive)) dip = true;
    else return;

    QString spacing;
    int pins = TextUtils::getPinsAndSpacing(moduleIDRef, spacing);

    QDomElement prop = instance.firstChildElement("property");
    while (!prop.isNull()) {
        if (prop.attribute("name", "").compare("spacing") == 0) {
            QString trueSpacing = prop.attribute("value", "");
            if (trueSpacing.isEmpty()) trueSpacing = "300mil";

            if (moduleIDRef.contains(spacing)) {
                moduleIDRef.replace(spacing, trueSpacing);
                instance.setAttribute("moduleIdRef", moduleIDRef);
                return;
            }

            // if we're here, it's a single sided mystery part.
            moduleIDRef = QString("mystery_part_sip_%1_100mil").arg(pins);
            instance.setAttribute("moduleIdRef", moduleIDRef);
            return;
        }
        prop = prop.nextSiblingElement("property");
    }
}

bool ModelBase::onCoreList(const QString & moduleID) {
    // CoreList contains db entries that are (presumably) overridden by an fzp in the parts folder
    return CoreList.contains(moduleID);

}

ModelPart * ModelBase::createOldSchematicPart(ModelPart * modelPart, QString & moduleIDRef) {
    QString schematicFilename = modelPart->imageFileName(ViewLayer::SchematicView, ViewLayer::Schematic);
    if (!schematicFilename.startsWith("schematic")) {
        schematicFilename = modelPart->imageFileName(ViewLayer::SchematicView, ViewLayer::SchematicFrame);
        if (!schematicFilename.startsWith("schematic")) {
            return modelPart;
        }
    }

    DebugDialog::debug("schematic " + schematicFilename);
    QString oldModuleIDRef = PartFactory::OldSchematicPrefix + moduleIDRef;
    ModelPart * oldModelPart = m_referenceModel->retrieveModelPart(oldModuleIDRef);         // cached after the first time it's created
    if (oldModelPart) {
        moduleIDRef = oldModuleIDRef;
        return oldModelPart;
    }

    int ix = schematicFilename.indexOf("/");
    schematicFilename.insert(ix + 1, PartFactory::OldSchematicPrefix);
    QString oldSvgPath = FolderUtils::getAppPartsSubFolderPath("") + "/svg/obsolete/"+ schematicFilename;
    oldModelPart = createOldSchematicPartAux(modelPart, oldModuleIDRef, schematicFilename, oldSvgPath);
    if (oldModelPart) {
        moduleIDRef = oldModuleIDRef;
        return oldModelPart;
    }

    oldSvgPath = ":resources/parts/svg/obsolete/"+ schematicFilename;
    oldModelPart = createOldSchematicPartAux(modelPart, oldModuleIDRef, schematicFilename, oldSvgPath);
    if (oldModelPart) {
        moduleIDRef = oldModuleIDRef;
        return oldModelPart;
    }


    // see whether it's a generated part
    oldSvgPath = PartFactory::getSvgFilename(schematicFilename);
    if (!oldSvgPath.isEmpty()) {
        oldModelPart = createOldSchematicPartAux(modelPart, oldModuleIDRef, schematicFilename, oldSvgPath);
        if (oldModelPart) {
            moduleIDRef = oldModuleIDRef;
            return oldModelPart;
        }    
    }
    
    return modelPart;
}

ModelPart * ModelBase::createOldSchematicPartAux(ModelPart * modelPart, const QString & oldModuleIDRef, const QString & oldSchematicFileName, const QString & oldSvgPath)
{
    if (!QFile::exists(oldSvgPath)) return NULL;

    // create oldModelPart, set up the new image file name, add it to refmodel
    QFile newFzp(modelPart->path());
    QDomDocument oldDoc;
    bool ok = oldDoc.setContent(&newFzp);
    if (!ok) {
        // this shouldn't happen
        return NULL;
    }

    QDomElement root = oldDoc.documentElement();
    root.setAttribute("moduleId", oldModuleIDRef);
    QDomElement views = root.firstChildElement("views");
    QDomElement schematicView = views.firstChildElement("schematicView");
    QDomElement layers = schematicView.firstChildElement("layers");
    if (layers.isNull()) {
        // this shouldn't happen
        return NULL;
    }

    layers.setAttribute("image", oldSchematicFileName);

    QString oldFzpPath = PartFactory::fzpPath() + oldModuleIDRef + ".fzp";
    if (!TextUtils::writeUtf8(oldFzpPath, oldDoc.toString())) {
        // this shouldn't happen
        return NULL;
    }

    ModelPart * oldModelPart = m_referenceModel->addPart(oldFzpPath, true, true);
    oldModelPart->setCore(modelPart->isCore());
    oldModelPart->setContrib(modelPart->isContrib());
    return oldModelPart;
}

bool ModelBase::checkForReversedWires() {
    return m_checkForReversedWires;
}
