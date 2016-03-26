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

$Revision: 7004 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-29 13:10:59 +0200 (Mo, 29. Apr 2013) $

********************************************************************/

#include "palettemodel.h"
#include <QFile>
#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QDomElement>

#include "../debugdialog.h"
#include "modelpart.h"
#include "../version/version.h"
#include "../layerattributes.h"
#include "../utils/folderutils.h"
#include "../utils/fmessagebox.h"
#include "../utils/textutils.h"
#include "../items/moduleidnames.h"
#include "../items/partfactory.h"

static bool JustAppendAllPartsInstances = false;
static bool FirstTime = true;

QString PaletteModel::s_fzpOverrideFolder;

const static QString InstanceTemplate(
        		"\t\t<instance moduleIdRef=\"%1\" path=\"%2\">\n"
				"\t\t\t<views>\n"
        		"\t\t\t\t<iconView layer=\"icon\">\n"
        		"\t\t\t\t\t<geometry z=\"-1\" x=\"-1\" y=\"-1\"></geometry>\n"
        		"\t\t\t\t</iconView>\n"
        		"\t\t\t</views>\n"
        		"\t\t</instance>\n");


void setFlip(ViewLayer::ViewID viewID, QXmlStreamReader & xml, QHash<ViewLayer::ViewID, ViewImage *> & viewImages)
{
    bool fv = xml.attributes().value("flipvertical", "").toString().compare("true") == 0;
    bool fh = xml.attributes().value("fliphorizontal", "").toString().compare("true") == 0;
    viewImages.value(viewID)->canFlipHorizontal = fh;
    viewImages.value(viewID)->canFlipVertical = fv;
}

/////////////////////////////////////

PaletteModel::PaletteModel() : ModelBase(true) {
	m_loadedFromFile = false;
	m_loadingContrib = false;
    m_fullLoad = false;
}

PaletteModel::PaletteModel(bool makeRoot, bool doInit) : ModelBase( makeRoot ) {
	m_loadedFromFile = false;
	m_loadingContrib = false;
    m_fullLoad = false;

	if (doInit) {
		initParts(false);
	}
}

PaletteModel::~PaletteModel()
{
    foreach (ModelPart * modelPart, m_partHash.values()) {
        delete modelPart;
    }
}

void PaletteModel::initParts(bool dbExists) {
	loadParts(dbExists);
	if (m_root == NULL) {
	    FMessageBox::information(NULL, QObject::tr("Fritzing"),
	                             QObject::tr("No parts found.") );
	}
}

void PaletteModel::initNames() {
}

ModelPart * PaletteModel::retrieveModelPart(const QString & moduleID) {
	ModelPart * modelPart = m_partHash.value(moduleID, NULL);
	if (modelPart != NULL) return modelPart;

	if (m_referenceModel != NULL) {
		return m_referenceModel->retrieveModelPart(moduleID);
	}

	return NULL;
}

bool PaletteModel::containsModelPart(const QString & moduleID) {
	return m_partHash.contains(moduleID);
}

void PaletteModel::loadParts(bool dbExists) {
	QStringList nameFilters;
	nameFilters << "*" + FritzingPartExtension;

	JustAppendAllPartsInstances = true;   
	/// !!!!!!!!!!!!!!!!  "JustAppendAllPartsInstances = CreateAllPartsBinFile"
	/// !!!!!!!!!!!!!!!!  is incorrect
	/// !!!!!!!!!!!!!!!!  this flag was originally set up because sometimes we were appending a
	/// !!!!!!!!!!!!!!!!  single instance into an already existing file,
	/// !!!!!!!!!!!!!!!!  so simply appending new items as text gave us xml errors.
	/// !!!!!!!!!!!!!!!!  The problem was that there was no easy way to set the flag directly on the actual
	/// !!!!!!!!!!!!!!!!  function being used:  PaletteModel::LoadPart(), though maybe this deserves another look.
	/// !!!!!!!!!!!!!!!!  However, since we're starting from scratch in LoadParts, we can use the much faster 
	/// !!!!!!!!!!!!!!!!  file append method.  Since CreateAllPartsBinFile is false in release mode,
	/// !!!!!!!!!!!!!!!!  Fritzing was taking forever to start up.


	if (FirstTime) {
	}

	int totalPartCount = 0;
	emit loadedPart(0, totalPartCount);

    QDir dir1 = FolderUtils::getAppPartsSubFolder("");
    QDir dir2(FolderUtils::getUserPartsPath());
    QDir dir3(":/resources/parts");
    QDir dir4(s_fzpOverrideFolder);

    if (m_fullLoad || !dbExists) {
        // otherwise these will already be in the database
        countParts(dir1, nameFilters, totalPartCount);
        countParts(dir3, nameFilters, totalPartCount);
    }

    if (!m_fullLoad) {
        // don't include local parts when doing full load
        countParts(dir2, nameFilters, totalPartCount);
        if (!s_fzpOverrideFolder.isEmpty()) {
            countParts(dir4, nameFilters, totalPartCount);
        }
    }

    emit partsToLoad(totalPartCount);

	int loadingPart = 0;
    if (m_fullLoad || !dbExists) {
        // otherwise these will already be in the database
        loadPartsAux(dir1, nameFilters, loadingPart, totalPartCount);
        loadPartsAux(dir3, nameFilters, loadingPart, totalPartCount);
    }

    if (!m_fullLoad) {
        loadPartsAux(dir2, nameFilters, loadingPart, totalPartCount);
        if (!s_fzpOverrideFolder.isEmpty()) {
            loadPartsAux(dir4, nameFilters, loadingPart, totalPartCount);
        }
    }

	if (FirstTime) {
	}
	
	JustAppendAllPartsInstances = false;   
	/// !!!!!!!!!!!!!!!!  "JustAppendAllPartsInstances = !CreateAllPartsBinFile"
	/// !!!!!!!!!!!!!!!!  is incorrect
	/// !!!!!!!!!!!!!!!!  See above.  We simply want to restore the default, so that other functions calling
	/// !!!!!!!!!!!!!!!!  writeInstanceInCommonBin via LoadPart() will use the slower DomDocument methods,
	/// !!!!!!!!!!!!!!!!  since in that case we are appending to an already existing file.

	FirstTime = false;

}

void PaletteModel::countParts(QDir & dir, QStringList & nameFilters, int & partCount) {
    QStringList list = dir.entryList(nameFilters, QDir::Files | QDir::NoSymLinks);
	partCount += list.size();

    QStringList dirs = dir.entryList(QDir::AllDirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for (int i = 0; i < dirs.size(); ++i) {
    	QString temp2 = dirs[i];
       	dir.cd(temp2);

    	countParts(dir, nameFilters, partCount);
    	dir.cdUp();
    }
}

void PaletteModel::loadPartsAux(QDir & dir, QStringList & nameFilters, int & loadingPart, int totalPartCount) {
    //QString temp = dir.absolutePath();
    QFileInfoList list = dir.entryInfoList(nameFilters, QDir::Files | QDir::NoSymLinks);
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        QString path = fileInfo.absoluteFilePath ();
        //DebugDialog::debug(QString("part path:%1 core? %2").arg(path).arg(m_loadingCore? "true" : "false"));
        loadPart(path, false);
		emit loadedPart(++loadingPart, totalPartCount);
        //DebugDialog::debug("loadedok");
    }

    QStringList dirs = dir.entryList(QDir::AllDirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for (int i = 0; i < dirs.size(); ++i) {
    	QString temp2 = dirs[i];
       	dir.cd(temp2);

		m_loadingContrib = (temp2 == "contrib");

    	loadPartsAux(dir, nameFilters, loadingPart, totalPartCount);
    	dir.cdUp();
    }
}

ModelPart * PaletteModel::loadPart(const QString & path, bool update) {
  
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        FMessageBox::warning(NULL, QObject::tr("Fritzing"),
                             QObject::tr("Cannot read file %1:\n%2.")
                             .arg(path)
                             .arg(file.errorString()));
        return NULL;
    }

	//DebugDialog::debug(QString("loading %2 %1").arg(path).arg(QTime::currentTime().toString("HH:mm:ss.zzz")));

	ModelPart::ItemType type = ModelPart::Part;
	QString moduleID;
    QString title;
	QString propertiesText;

	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;
	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		FMessageBox::information(NULL, QObject::tr("Fritzing"),
								QObject::tr("Parse error (2) at line %1, column %2:\n%3\n%4")
								.arg(errorLine)
								.arg(errorColumn)
								.arg(errorStr)
								.arg(path));
		return NULL;
	}

	QDomElement root = domDocument.documentElement();
   	if (root.isNull()) {
		//QMessageBox::information(NULL, QObject::tr("Fritzing"), QObject::tr("The file is not a Fritzing file (8)."));
   		return NULL;
	}

	if (root.tagName() != "module") {
		//QMessageBox::information(NULL, QObject::tr("Fritzing"), QObject::tr("The file is not a Fritzing file (9)."));
		return NULL;
	}

	moduleID = root.attribute("moduleId");
	if (moduleID.isNull() || moduleID.isEmpty()) {
		//QMessageBox::information(NULL, QObject::tr("Fritzing"), QObject::tr("The file is not a Fritzing file (10)."));
		return NULL;
	}

	// check if it's a wire
	QDomElement propertiesElement = root.firstChildElement("properties");
	propertiesText = propertiesElement.text();

	QDomElement t = root.firstChildElement("title");
	TextUtils::findText(t, title);

    //DebugDialog::debug("module ID " + moduleID);

	// FIXME: properties is nested right now
	if (moduleID.compare(ModuleIDNames::WireModuleIDName) == 0) {
		type = ModelPart::Wire;
	}
	else if (moduleID.compare(ModuleIDNames::JumperModuleIDName) == 0) {
		type = ModelPart::Jumper;
	}
	else if (moduleID.endsWith(ModuleIDNames::LogoImageModuleIDName)) {
		type = ModelPart::Logo;
	}
	else if (moduleID.endsWith(ModuleIDNames::LogoTextModuleIDName)) {
		type = ModelPart::Logo;
	}
	else if (moduleID.compare(ModuleIDNames::GroundPlaneModuleIDName) == 0) {
		type = ModelPart::CopperFill;
	}
	else if (moduleID.compare(ModuleIDNames::NoteModuleIDName) == 0) {
		type = ModelPart::Note;
	}
	else if (moduleID.endsWith(ModuleIDNames::TwoPowerModuleIDName)) {
		type = ModelPart::Part;
	}
	else if (moduleID.endsWith(ModuleIDNames::PowerModuleIDName)) {
		type = ModelPart::Symbol;
	}
	else if (moduleID.compare(ModuleIDNames::GroundModuleIDName) == 0) {
		type = ModelPart::Symbol;
	}
	else if (moduleID.endsWith(ModuleIDNames::NetLabelModuleIDName)) {
		type = ModelPart::Symbol;
	}
	else if (moduleID.compare(ModuleIDNames::PowerLabelModuleIDName) == 0) {
		type = ModelPart::Symbol;
	}
	else if (moduleID.compare(ModuleIDNames::RulerModuleIDName) == 0) {
		type = ModelPart::Ruler;
	}
	else if (moduleID.compare(ModuleIDNames::ViaModuleIDName) == 0) {
		type = ModelPart::Via;
	}
	else if (moduleID.compare(ModuleIDNames::HoleModuleIDName) == 0) {
		type = ModelPart::Hole;
	}
	else if (moduleID.endsWith(ModuleIDNames::PerfboardModuleIDName)) {
		type = ModelPart::Breadboard;
	}
	else if (moduleID.endsWith(ModuleIDNames::StripboardModuleIDName)) {
		type = ModelPart::Breadboard;
	}
	else if (moduleID.endsWith(ModuleIDNames::Stripboard2ModuleIDName)) {
		type = ModelPart::Breadboard;
	}
	else if (propertiesText.contains("breadboard", Qt::CaseInsensitive)) {
		type = ModelPart::Breadboard;
	}
	else if (propertiesText.contains("plain vanilla pcb", Qt::CaseInsensitive)) {
		if (propertiesText.contains("shield", Qt::CaseInsensitive) || title.contains("custom", Qt::CaseInsensitive)) {
			type = ModelPart::Board;
		}
		else {
			type = ModelPart::ResizableBoard;
		}
	}

	ModelPart * modelPart = new ModelPart(domDocument, path, type);
	if (modelPart == NULL) return NULL;

    if (path.startsWith(ResourcePath)) {
	    modelPart->setCore(true);
    }
    else if (onCoreList(moduleID)) {
        // for database entries which have existing fzp files.
	    modelPart->setCore(true);
    }

	modelPart->setContrib(m_loadingContrib);

    QDomElement subparts = root.firstChildElement("schematic-subparts");
    QDomElement subpart = subparts.firstChildElement("subpart");
    while (!subpart.isNull()) {
        ModelPart * subModelPart = makeSubpart(modelPart, subpart);
        m_partHash.insert(subModelPart->moduleID(), subModelPart);
        subpart = subpart.nextSiblingElement("subpart");
    }

	if (m_partHash.value(moduleID, NULL)) {
		if(!update) {
			FMessageBox::warning(NULL, QObject::tr("Fritzing"),
							 QObject::tr("The part '%1' at '%2' does not have a unique module id '%3'.")
							 .arg(modelPart->title())
							 .arg(path)
							 .arg(moduleID));
			return NULL;
		} else {
			m_partHash[moduleID]->copyStuff(modelPart);
		}
	} else {
		m_partHash.insert(moduleID, modelPart);
	}

    if (m_root == NULL) {
		 m_root = modelPart;
	}
	else {
    	modelPart->setParent(m_root);
   	}

    return modelPart;
}

bool PaletteModel::loadFromFile(const QString & fileName, ModelBase * referenceModel, bool checkViews) {
	QList<ModelPart *> modelParts;
	bool result = ModelBase::loadFromFile(fileName, referenceModel, modelParts, checkViews);
	if (result) {
		m_loadedFromFile = true;
		m_loadedFrom = fileName;
	}
	return result;
}

bool PaletteModel::loadedFromFile() {
	return m_loadedFromFile;
}

QString PaletteModel::loadedFrom() {
	if(m_loadedFromFile) {
		return m_loadedFrom;
	} else {
		return ___emptyString___;
	}
}

ModelPart * PaletteModel::addPart(QString newPartPath, bool addToReference, bool updateIdAlreadyExists) {
	/*ModelPart * modelPart = loadPart(newPartPath, updateIdAlreadyExists);;
	if (m_referenceModel != NULL && addToReference) {
		modelPart = m_referenceModel->addPart(newPartPath, addToReference);
		if (modelPart != NULL) {
			return addModelPart( m_root, modelPart);
		}
	}*/

	ModelPart * modelPart = loadPart(newPartPath, updateIdAlreadyExists);
	if (m_referenceModel && addToReference) {
		m_referenceModel->addPart(modelPart,updateIdAlreadyExists);
	}

	return modelPart;
}

void PaletteModel::removePart(const QString &moduleID) {
	ModelPart *mpToRemove = NULL;
	QList<QObject *>::const_iterator i;
    for (i = m_root->children().constBegin(); i != m_root->children().constEnd(); ++i) {
		ModelPart* mp = qobject_cast<ModelPart *>(*i);
		if (mp == NULL) continue;

        //DebugDialog::debug(QString("remove part %1").arg(mp->moduleID()));
		if(mp->moduleID() == moduleID) {
			mpToRemove = mp;
			break;
		}
	}
	if(mpToRemove) {
		mpToRemove->setParent(NULL);

		delete mpToRemove;
	}
    //DebugDialog::debug(QString("part hash count %1").arg(m_partHash.count()));
    m_partHash.remove(moduleID);
    //DebugDialog::debug(QString("part hash count %1").arg(m_partHash.count()));
}

void PaletteModel::removeParts() {
    QList<ModelPart *> modelParts;
    foreach (QObject * child, m_root->children()) {
        ModelPart * modelPart = qobject_cast<ModelPart *>(child);
        if (modelPart == NULL) continue;

        modelParts.append(modelPart);
    }

    foreach(ModelPart * modelPart, modelParts) {
        modelPart->setParent(NULL);
        m_partHash.remove(modelPart->moduleID());
        delete modelPart;
    }
}

void PaletteModel::clearPartHash() {
	foreach (ModelPart * modelPart, m_partHash.values()) {
        ModelPartShared * modelPartShared = modelPart->modelPartShared();
        if (modelPartShared) {
            modelPart->setModelPartShared(NULL);
            delete modelPartShared;
        }

		delete modelPart;
	}
	m_partHash.clear();
}

void PaletteModel::setOrdererChildren(QList<QObject*> children) {
	m_root->setOrderedChildren(children);
}

QList<ModelPart *> PaletteModel::search(const QString & searchText, bool allowObsolete) {
	QList<ModelPart *> modelParts;

	QStringList strings = searchText.split(" ");
    search(m_root, strings, modelParts, allowObsolete);
    return modelParts;
}

void PaletteModel::search(ModelPart * modelPart, const QStringList & searchStrings, QList<ModelPart *> & modelParts, bool allowObsolete) {
	// TODO: eventually move all this into the database?
	// or use lucene
	// or google search api

    int count = 0;
    foreach (QString searchString, searchStrings) {
        bool gotOne = false;
        if (modelPart->title().contains(searchString, Qt::CaseInsensitive)) {
            gotOne = true;
	    }
        else if (modelPart->description().contains(searchString, Qt::CaseInsensitive)) {
            gotOne = true;
	    }
        else if (modelPart->url().contains(searchString, Qt::CaseInsensitive)) {
            gotOne = true;
	    }
        else if (modelPart->author().contains(searchString, Qt::CaseInsensitive)) {
            gotOne = true;
	    }
        else if (modelPart->moduleID().contains(searchString, Qt::CaseInsensitive)) {
            gotOne = true;
	    }
        else {
		    foreach (QString string, modelPart->tags()) {
			    if (string.contains(searchString, Qt::CaseInsensitive)) {
                    gotOne = true;
				    break;
			    }
		    }
	    }
        if (!gotOne) {
		    foreach (QString string, modelPart->properties().values()) {
			    if (string.contains(searchString, Qt::CaseInsensitive)) {
                    gotOne = true;
				    break;
			    }
		    }
	    }
        if (!gotOne) {
		    foreach (QString string, modelPart->properties().keys()) {
			    if (string.contains(searchString, Qt::CaseInsensitive)) {
                    gotOne = true;
				    break;
			    }
		    }
	    }
        if (!gotOne) break;

        count++;
    }

    if ((count == searchStrings.count()) && !modelParts.contains(modelPart)) {
        if (!allowObsolete && modelPart->isObsolete()) {
        }
        else
        {
            modelParts.append(modelPart);
            emit addSearchMaximum(1);
        }
    }

    emit addSearchMaximum(modelPart->children().count());

	foreach(QObject * child, modelPart->children()) {
		ModelPart * mp = qobject_cast<ModelPart *>(child);
		if (mp == NULL) continue;

        search(mp, searchStrings, modelParts, allowObsolete);
        emit incSearch();
	}
}

QList<ModelPart *> PaletteModel::findContribNoBin() {
    QList<ModelPart *> modelParts;
    foreach (ModelPart * modelPart, m_partHash.values()) {
        if (modelPart->isContrib()) {
            if (!modelPart->isInBin()) {
                modelParts << modelPart;
            }
        }
    }
    return modelParts;
}

ModelPart * PaletteModel::makeSubpart(ModelPart * originalModelPart, const QDomElement & originalSubpart) {
    QString newLabel = originalSubpart.attribute("label");
    QString newID = originalSubpart.attribute("id");
    QDomElement originalRoot = originalSubpart.ownerDocument().documentElement();
    QString moduleID = originalRoot.attribute("moduleId") + "_" + newID;
    ModelPart * modelPart = retrieveModelPart(moduleID);
    if (modelPart) {
        return modelPart;
    }

    QDomDocument subdoc = originalSubpart.ownerDocument().cloneNode(true).toDocument();
    QDomElement root = subdoc.documentElement();
    QDomElement subparts = root.firstChildElement("schematic-subparts");
    root.removeChild(subparts);
    root.setAttribute("moduleId", moduleID);
    QDomElement label = root.firstChildElement("label");
    if (!label.isNull()) {
        TextUtils::replaceChildText(label, newLabel);
    }
    QDomElement views = root.firstChildElement("views");
    QDomElement schematicView = views.firstChildElement(ViewLayer::viewIDXmlName(ViewLayer::SchematicView));
    QDomElement schematicLayers = schematicView.firstChildElement("layers");
    QString image = schematicLayers.attribute("image");
    image.replace(".svg", "_" + newID + ".svg");
    schematicLayers.setAttribute("image", image);
    views.removeChild(schematicView);
    QDomElement view = views.firstChildElement();
    while (!view.isNull()) {
        QDomElement layers = view.firstChildElement("layers");
        view.removeChild(layers);
        layers = schematicLayers.cloneNode(true).toElement();
        view.appendChild(layers);
        view = view.nextSiblingElement();
    }
    views.appendChild(schematicView);

    QDomElement connectors = root.firstChildElement("connectors");
    QDomElement connector = connectors.firstChildElement("connector");
    QHash<QString, QDomElement> connectorHash;
    while (!connector.isNull()) {
        QDomElement next = connector.nextSiblingElement("connector");
        connectors.removeChild(connector);
        connectorHash.insert(connector.attribute("id"), connector);
        connector = next;
    }
    QDomElement originalConnectors = originalSubpart.firstChildElement("connectors");
    QDomElement originalConnector = originalConnectors.firstChildElement("connector");
    QString schematicLayerName = ViewLayer::viewLayerXmlNameFromID(ViewLayer::Schematic);
    while (!originalConnector.isNull()) {
        QString id = originalConnector.attribute("id");
        QDomElement connector = connectorHash.value(id);
        connectors.appendChild(connector);
        QDomElement cviews = connector.firstChildElement("views");
        QDomElement view = cviews.firstChildElement();
        while (!view.isNull()) {
            QDomElement p = view.firstChildElement("p");
            bool firstTime = true;
            while (!p.isNull()) {
                QDomElement next = p.nextSiblingElement("p");
                if (firstTime) {
                    p.setAttribute("layer", schematicLayerName);
                    firstTime = false;
                }
                else {
                    view.removeChild(p);
                }
                p = next;
            }
            view = view.nextSiblingElement();
        }

        originalConnector = originalConnector.nextSiblingElement("connector");
    }

    QString path = PartFactory::fzpPath() + moduleID + ".fzp";
    QString fzp = subdoc.toString(4);
    TextUtils::writeUtf8(path, fzp);

    modelPart = new ModelPart(subdoc, path, ModelPart::SchematicSubpart);
    modelPart->setSubpartID(newID);
    originalModelPart->modelPartShared()->addSubpart(modelPart->modelPartShared());
    m_partHash.insert(moduleID, modelPart);
    return modelPart;
}

QList<ModelPart *> PaletteModel::allParts() {
    QList<ModelPart *> modelParts;
    foreach (ModelPart * modelPart, m_partHash.values()) {
        if (!modelPart->isObsolete()) modelParts.append(modelPart);
    }
    return modelParts;
}

void PaletteModel::setFzpOverrideFolder(const QString & fzpOverrideFolder) {
    s_fzpOverrideFolder = fzpOverrideFolder;
}
