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

$Revision: 6962 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-14 00:08:36 +0200 (So, 14. Apr 2013) $

********************************************************************/

#include "partfactory.h"
#include "../debugdialog.h"
#include "../viewgeometry.h"
#include "../model/modelpart.h"
#include "../model/modelbase.h"
#include "paletteitem.h"
#include "symbolpaletteitem.h"
#include "wire.h"
#include "virtualwire.h"
#include "tracewire.h"
#include "jumperitem.h"
#include "resizableboard.h"
#include "logoitem.h"
#include "schematicframe.h"
#include "resistor.h"
#include "moduleidnames.h"
#include "mysterypart.h"
#include "groundplane.h"
#include "note.h"
#include "ruler.h"
#include "dip.h"
#include "pinheader.h"
#include "screwterminal.h"
#include "hole.h"
#include "via.h"
#include "pad.h"
#include "capacitor.h"
#include "perfboard.h"
#include "breadboard.h"
#include "stripboard.h"
#include "led.h"
#include "schematicsubpart.h"
#include "layerkinpaletteitem.h"
#include "../utils/folderutils.h"
#include "../utils/lockmanager.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"

#include <qmath.h>

static QString PartFactoryFolderPath;
static QHash<QString, LockedFile *> LockedFiles;
static QString SvgFilesDir = "svg";
static QHash<QString, QPointF> SubpartOffsets;

const QString PartFactory::OldSchematicPrefix("0.3.schem.");

ItemBase * PartFactory::createPart( ModelPart * modelPart, ViewLayer::ViewLayerPlacement viewLayerPlacement, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, QMenu * wireMenu, bool doLabel)
{
	modelPart->setModelIndexFromMultiplied(id);			// make sure the model index is synched with the id; this is not always the case when parts are first created.
	ItemBase * itemBase = createPartAux(modelPart, viewID, viewGeometry, id, itemMenu, wireMenu, doLabel);
	if (itemBase) {
		itemBase->setViewLayerPlacement(viewLayerPlacement);
	}
	return itemBase;
}

ItemBase * PartFactory::createPartAux( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, QMenu * wireMenu, bool doLabel)
{
	switch (modelPart->itemType()) {
		case ModelPart::Wire:
		{
			bool ratsnest = viewGeometry.getRatsnest();
			if (ratsnest) {
				return new VirtualWire(modelPart, viewID, viewGeometry, id, wireMenu);		
			}
			if (viewGeometry.getAnyTrace()) {
				TraceWire * traceWire = new TraceWire(modelPart, viewID, viewGeometry, id, wireMenu);
				return traceWire;
			}
			return new Wire(modelPart, viewID, viewGeometry, id, wireMenu, false);

		}
		case ModelPart::Note:
			return new Note(modelPart, viewID, viewGeometry, id, NULL);
		case ModelPart::CopperFill:
			return new GroundPlane(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::Jumper:
			return new JumperItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::ResizableBoard:
			return new ResizableBoard(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::Board:
			return new Board(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::Logo:
			if (modelPart->moduleID().contains("copper", Qt::CaseInsensitive)) {
				return new CopperLogoItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
			}
            else if (modelPart->moduleID().contains("schematic", Qt::CaseInsensitive)) {
                return new SchematicLogoItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
            }
            else if (modelPart->moduleID().contains("breadboard", Qt::CaseInsensitive)) {
                return new BreadboardLogoItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
            }
            // make sure "board" match comes after "breadboard" match
            else if (modelPart->moduleID().contains("board", Qt::CaseInsensitive)) {
                return new BoardLogoItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
            }
			return new LogoItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::Ruler:
			return new Ruler(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::Symbol:
            if (modelPart->moduleID().contains(ModuleIDNames::NetLabelModuleIDName)) {
			    return new NetLabel(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
            }
			return new SymbolPaletteItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::Via:
			return new Via(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::Hole:
			return new Hole(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
        case ModelPart::SchematicSubpart:
            return new SchematicSubpart(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
		default:
			{
				QString moduleID = modelPart->moduleID();
				if (moduleID.endsWith(ModuleIDNames::ModuleIDNameSuffix)) {
					if (moduleID.endsWith(ModuleIDNames::ResistorModuleIDName)) {
						return new Resistor(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::CapacitorModuleIDName)) {
						return new Capacitor(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::CrystalModuleIDName)) {
						return new Capacitor(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::ThermistorModuleIDName)) {
						return new Capacitor(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::ZenerDiodeModuleIDName)) {
						return new Capacitor(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::PotentiometerModuleIDName)) {
						return new Capacitor(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::InductorModuleIDName)) {
						return new Capacitor(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::TwoPowerModuleIDName)) {
						return new Capacitor(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::ColorLEDModuleIDName)) {
						return new LED(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::ColorFluxLEDModuleIDName)) {
						return new LED(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::LEDModuleIDName)) {
						return new Capacitor(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::PerfboardModuleIDName)) {
						return new Perfboard(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::StripboardModuleIDName)) {
						return new Stripboard(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::Stripboard2ModuleIDName)) {
						return new Stripboard(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::SchematicFrameModuleIDName)) {
						return new SchematicFrame(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::PadModuleIDName)) {
						return new Pad(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::CopperBlockerModuleIDName)) {
						return new CopperBlocker(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}
					// must get the subclasses first
					if (modelPart->itemType() == ModelPart::Breadboard) {
						return new Breadboard(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
					}

				}
                // TODO: use the list in properties.xml
                if (moduleID == "alps-starter-pot9mm") {
				    return new Capacitor(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
                }
				QString family = modelPart->properties().value("family", "");
				if (family.compare("mystery part", Qt::CaseInsensitive) == 0) {
					return new MysteryPart(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
				}
				if (family.compare("screw terminal", Qt::CaseInsensitive) == 0) {
					return new ScrewTerminal(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
				}
				if (family.compare("pin header", Qt::CaseInsensitive) == 0) {
					return new PinHeader(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
				}
				if (family.compare("generic IC", Qt::CaseInsensitive) == 0) {
					return new Dip(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);
				}
				return new PaletteItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel);

			}
	}
}

QString PartFactory::getSvgFilename(ModelPart * modelPart, const QString & baseName, bool generate, bool handleSubparts) 
{
	QStringList tempPaths;
	QString postfix = "/"+ SvgFilesDir +"/%1/"+ baseName;
    QString userStore = FolderUtils::getUserPartsPath()+postfix;
    QString pfPath = PartFactory::folderPath() + postfix;
	if(!modelPart->path().isEmpty()) {
        QString path = modelPart->path();
		QDir dir(path);			// is a path to a filename
		dir.cdUp();									// lop off the filename
		dir.cdUp();									// parts root
		tempPaths << dir.absolutePath() + postfix;
        tempPaths << FolderUtils::getAppPartsSubFolderPath("")+postfix;    // some svgs may still be in the fritzing parts folder, though the other svgs are in the user folder
        if (tempPaths.at(0).compare(userStore) != 0) {
            tempPaths << userStore;
        }
        if (tempPaths.at(0).compare(pfPath) != 0) {
            tempPaths << pfPath;
        }
        //DebugDialog::debug("temp path");
        //foreach (QString tempPath, tempPaths) {
        //    DebugDialog::debug(tempPath);
        //}
    } 
	else {
        DebugDialog::debug("modelPart with no path--this shouldn't happen");
        tempPaths << FolderUtils::getAppPartsSubFolderPath("")+postfix;
		tempPaths << userStore;
	}
	tempPaths << ":resources/parts/svg/%1/" + baseName;

		//DebugDialog::debug(QString("got tempPath %1").arg(tempPath));

	QString filename;
    bool exists = false;
	foreach (QString tempPath, tempPaths) {
		foreach (QString possibleFolder, ModelPart::possibleFolders()) {
			filename = tempPath.arg(possibleFolder);
			if (QFileInfo(filename).exists()) {
                exists = true;
				if (possibleFolder == "obsolete") {
					DebugDialog::debug(QString("module %1:%2 obsolete svg %3").arg(modelPart->title()).arg(modelPart->moduleID()).arg(filename));
				}
				break;
			} 
		}
        if (exists) break;
	}

    if (!exists && generate) {
	    filename = PartFactory::getSvgFilename(baseName);
    }

    if (handleSubparts && modelPart->modelPartShared() && modelPart->modelPartShared()->hasSubparts())
    {
        ModelPartShared * superpart = modelPart->modelPartShared();
        QString schematicName = superpart->imageFileName(ViewLayer::SchematicView);
        QString originalPath = getSvgFilename(modelPart, schematicName, true, false);  
        foreach (ModelPartShared * mps, superpart->subparts()) {
            QString schematicFileName = mps->imageFileName(ViewLayer::SchematicView);
            if (schematicFileName.isEmpty()) continue;

            QString path = partPath() + schematicFileName;
            QFileInfo info(path);
            if (info.exists()) {
                mps->setSubpartOffset(SubpartOffsets.value(path, QPointF(0, 0)));
                continue;
            }

            QFile file(originalPath);
	        QString errorStr;
	        int errorLine;
	        int errorColumn;
	        QDomDocument doc;
	        if (!doc.setContent(&file, &errorStr, &errorLine, &errorColumn)) {
                DebugDialog::debug(QString("xml failure %1 %2 %3").arg(errorStr).arg(errorLine).arg(errorColumn));
                continue;
            }

            QDomElement root = doc.documentElement();
            QDomElement top = showSubpart(root, mps->subpartID());
            fixSubpartBounds(top, mps);
            SubpartOffsets.insert(path, mps->subpartOffset());
            TextUtils::writeUtf8(path, doc.toString(4));
        }
    }

    return filename;
}

QString PartFactory::getSvgFilename(const QString & fileName) 
{
    QString osFileName(fileName);
    osFileName.remove(OldSchematicPrefix);

	if (osFileName.startsWith("pcb/dip_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &MysteryPart::makePcbDipSvg);
	}

	if (osFileName.startsWith("pcb/mystery_part_", Qt::CaseInsensitive)) {
        if (osFileName.contains("dip")) {
		    return getSvgFilenameAux(fileName, &MysteryPart::makePcbDipSvg);
        }
        return getSvgFilenameAux(fileName, &PinHeader::makePcbSvg);
	}

	if (osFileName.startsWith("breadboard/mystery_part_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &MysteryPart::makeBreadboardSvg);
	}

	if (osFileName.startsWith("breadboard/screw_terminal_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &ScrewTerminal::makeBreadboardSvg);
	}

	if (osFileName.startsWith("schematic/screw_terminal_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &ScrewTerminal::makeSchematicSvg);
	}

	if (osFileName.startsWith("pcb/screw_terminal_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &ScrewTerminal::makePcbSvg);
	}

	if (osFileName.startsWith("breadboard/generic_sip_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &Dip::makeBreadboardSvg);
	}

	if (osFileName.startsWith("schematic/mystery_part_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &MysteryPart::makeSchematicSvg);
	}

	if (osFileName.startsWith("schematic/generic_sip_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &MysteryPart::makeSchematicSvg);
	}

	if (osFileName.startsWith("pcb/generic_sip_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &PinHeader::makePcbSvg);
	}

	if (osFileName.startsWith("schematic/generic_ic_dip_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &Dip::makeSchematicSvg);
	}

	if (osFileName.startsWith("breadboard/generic_ic_dip_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &Dip::makeBreadboardSvg);
	}

	if (osFileName.startsWith("pcb/generic_ic_dip_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &MysteryPart::makePcbDipSvg);
	}

	if (osFileName.startsWith("pcb/nsjumper_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &PinHeader::makePcbSvg);
	}

	if (osFileName.startsWith("pcb/jumper_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &PinHeader::makePcbSvg);
	}

	if (osFileName.startsWith("pcb/shrouded_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &PinHeader::makePcbSvg);
	}

	if (osFileName.startsWith("pcb/molex_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &PinHeader::makePcbSvg);
	}

	if (osFileName.startsWith("pcb/longpad_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &PinHeader::makePcbSvg);
	}

	if (osFileName.startsWith("bread/molex_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &PinHeader::makeBreadboardSvg);
	}

	if (osFileName.startsWith("bread/longpad_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(fileName, &PinHeader::makeBreadboardSvg);
	}

	if (osFileName.contains("pin_header", Qt::CaseInsensitive)) {
		if (osFileName.contains("schematic", Qt::CaseInsensitive)) {
			return getSvgFilenameAux(fileName, &PinHeader::makeSchematicSvg);
		}
		else if (osFileName.contains("bread", Qt::CaseInsensitive)) {
			return getSvgFilenameAux(fileName, &PinHeader::makeBreadboardSvg);		
		}
		else if (osFileName.contains("pcb", Qt::CaseInsensitive)) {
			return getSvgFilenameAux(fileName, &PinHeader::makePcbSvg);		
		}
	}

	if (osFileName.contains("perfboard", Qt::CaseInsensitive) || osFileName.contains("stripboard", Qt::CaseInsensitive)) {
		if (osFileName.contains("icon")) return fileName;

		return getSvgFilenameAux(fileName, &Perfboard::makeBreadboardSvg);		
	}

	return "";
}

bool PartFactory::svgFileExists(const QString & expectedFileName, QString & path) {
    QString p = FolderUtils::getAppPartsSubFolderPath("") + "/"+ SvgFilesDir + "/core/";
	if (QFileInfo(p + expectedFileName).exists()) {
        path = expectedFileName;
        return true;
    }

	path = partPath() + expectedFileName;
	QFileInfo info(path);
	return info.exists();
}

QString PartFactory::getSvgFilenameAux(const QString & expectedFileName, GenSvg genSvg)
{
    QString path;
    if (svgFileExists(expectedFileName, path)) return path;

	QString svg = (*genSvg)(expectedFileName);
	if (TextUtils::writeUtf8(path, svg)) {
		return path;
	}

	return "";
}


bool PartFactory::fzpFileExists(const QString & moduleID, QString & path) {
    QString expectedFileName = moduleID + FritzingPartExtension;
    path = FolderUtils::getAppPartsSubFolderPath("") + "/core/" + expectedFileName;
	if (QFileInfo(path).exists()) {
        path = expectedFileName;
        return true;
    }

	path = fzpPath() + expectedFileName;
	QFileInfo info(path);
	return info.exists();
}

QString PartFactory::getFzpFilenameAux(const QString & moduleID, QString (*getFzp)(const QString &))
{
	QString path;
    if (fzpFileExists(moduleID, path)) return path;

	QString fzp = (*getFzp)(moduleID);
	if (TextUtils::writeUtf8(path, fzp)) {
		return path;
	}

	return "";
}


QString PartFactory::getFzpFilename(const QString & moduleID) 
{
    QString filename = fzpPath() + moduleID + ".fzp";
    QFileInfo info(filename);
    if (info.exists()) return filename;

	if (moduleID.endsWith(ModuleIDNames::PerfboardModuleIDName)) {
		return getFzpFilenameAux(moduleID, &Perfboard::genFZP);
	}

	if (moduleID.endsWith(ModuleIDNames::StripboardModuleIDName)) {
		return getFzpFilenameAux(moduleID, &Stripboard::genFZP);
	}

	if (moduleID.endsWith(ModuleIDNames::Stripboard2ModuleIDName)) {
		return getFzpFilenameAux(moduleID, &Stripboard::genFZP);
	}

	if (moduleID.startsWith("generic_ic_dip")) {
		return getFzpFilenameAux(moduleID, &Dip::genDipFZP);
	}

	if (moduleID.startsWith("screw_terminal")) {
		return getFzpFilenameAux(moduleID, &ScrewTerminal::genFZP);
	}

	if (moduleID.startsWith("generic_sip")) {
		return getFzpFilenameAux(moduleID, &Dip::genSipFZP);
	}

	if (moduleID.contains("_pin_header_") && moduleID.startsWith("generic_")) {
		return getFzpFilenameAux(moduleID, &PinHeader::genFZP);
	}

	if (moduleID.startsWith("mystery_part")) {
		if (moduleID.contains("dip", Qt::CaseInsensitive)) {
			return getFzpFilenameAux(moduleID, &MysteryPart::genDipFZP);
		}
		else {
			return getFzpFilenameAux(moduleID, &MysteryPart::genSipFZP);
		}
	}

	return "";
}

void PartFactory::initFolder()
{
	LockManager::initLockedFiles("partfactory", PartFactoryFolderPath, LockedFiles, LockManager::SlowTime);
	QFileInfoList backupList;
	LockManager::checkLockedFiles("partfactory", backupList, LockedFiles, true, LockManager::SlowTime);
	FolderUtils::makePartFolderHierarchy(PartFactoryFolderPath, "core");
	FolderUtils::makePartFolderHierarchy(PartFactoryFolderPath, "contrib");
}

void PartFactory::cleanup()
{
	LockManager::releaseLockedFiles(PartFactoryFolderPath, LockedFiles);
}

ModelPart * PartFactory::fixObsoleteModuleID(QDomDocument & domDocument, QDomElement & instance, QString & moduleIDRef, ModelBase * referenceModel) {
	// TODO: less hard-coding
	if (moduleIDRef.startsWith("generic_male")) {
		ModelPart * modelPart = referenceModel->retrieveModelPart(moduleIDRef);
		if (modelPart != NULL) {
			instance.setAttribute("moduleIdRef", moduleIDRef);
			QDomElement prop = domDocument.createElement("property");
			instance.appendChild(prop);
			prop.setAttribute("name", "form");
			prop.setAttribute("value", PinHeader::MaleFormString);
			return modelPart;
		}
	}

	if (moduleIDRef.startsWith("generic_rounded_female")) {
		ModelPart * modelPart = referenceModel->retrieveModelPart(moduleIDRef);
		if (modelPart != NULL) {
			instance.setAttribute("moduleIdRef", moduleIDRef);
			QDomElement prop = domDocument.createElement("property");
			instance.appendChild(prop);
			prop.setAttribute("name", "form");
			prop.setAttribute("value", PinHeader::FemaleRoundedFormString);
			return modelPart;
		}
	}

	return NULL;
}

QString PartFactory::folderPath() {
	return PartFactoryFolderPath;
}

QString PartFactory::fzpPath() {
    return PartFactoryFolderPath + "/core/";
}

QString PartFactory::partPath() {
    return PartFactoryFolderPath + "/svg/core/";
}

QString PartFactory::makeSchematicSipOrDipOr(const QStringList & labels, bool hasLayout, bool sip) 
{		
	if (hasLayout) {
		return MysteryPart::makeSchematicSvg(labels, false);
	}
		
    if (sip) {
		return MysteryPart::makeSchematicSvg(labels, true);
	}

	return Dip::makeSchematicSvg(labels);
}

QDomElement PartFactory::showSubpart(QDomElement & root, const QString & subpart)
{    
    if (subpart.isEmpty()) return ___emptyElement___;

    QString id = root.attribute("id");
    if (id == subpart) {
        return root;
    }

    QDomElement top;
    QDomElement child = root.firstChildElement();
    while (!child.isNull()) {
        QDomElement candidate = showSubpart(child, subpart);
        if (!candidate.isNull()) {
            top = candidate;
        }
        child = child.nextSiblingElement();
    }

    if (root.tagName() != "g" && root.tagName() != "svg") {
        root.setTagName("g");
    }
    
    return top;
}

void PartFactory::fixSubpartBounds(QDomElement & top, ModelPartShared * mps)
{    
    if (top.isNull()) return;

    QString transform = top.attribute("transform", "");
    top.removeAttribute("transform");
    QDomElement g = top.ownerDocument().createElement("g");
    if (!transform.isEmpty()) g.setAttribute("transform", transform);
    QDomElement child = top.firstChildElement();
    while (!child.isNull()) {
        g.appendChild(child);
        child = top.firstChildElement();
    }
    top.appendChild(g);

    QSvgRenderer renderer;
    QByteArray byteArray = top.ownerDocument().toByteArray();
    bool loaded = renderer.load(byteArray);
    if (!loaded) return;

    QRectF viewBox;
    double wignore, hignore;
    TextUtils::ensureViewBox(top.ownerDocument(), 1, viewBox, false, wignore, hignore, false);
    double sWidth, sHeight, vbWidth, vbHeight;
    QDomDocument doc = top.ownerDocument();
    TextUtils::getSvgSizes(doc, sWidth, sHeight, vbWidth, vbHeight);

	QMatrix m = renderer.matrixForElement(mps->subpartID());
    QRectF elementBounds = renderer.boundsOnElement(mps->subpartID());
	QRectF bounds = m.mapRect(elementBounds);                                   // bounds is in terms of the whole svg

    // unfortunately, QSvgRenderer doesn't deal with text bounds

    int w = qCeil(sWidth * GraphicsUtils::SVGDPI);
    int h = qCeil(sWidth * GraphicsUtils::SVGDPI);
    QImage image(w,h, QImage::Format_Mono);

    QDomNodeList nodeList = top.elementsByTagName("text");
    QList<QDomElement> texts;
    for (int i = 0; i < nodeList.count(); i++) {
        texts.append(nodeList.at(i).toElement());
    }

    int ix = 0;
    foreach (QDomElement text, texts) {
        text.setTagName("g");
    }

    ix = 0;
    foreach (QDomElement text, texts) {
        int minX, minY, maxX, maxY;
        QMatrix matrix;
        QRectF viewBox2;
        SchematicTextLayerKinPaletteItem::renderText(image, text, minX, minY, maxX, maxY, matrix, viewBox2);  
        QRectF r(minX * viewBox.width() / image.width(), 
                minY * viewBox.height() / image.height(), 
                (maxX - minX) * viewBox.width() / image.width(),
                (maxY - minY) * viewBox.height() / image.height());
        bounds |= r;
    }

    foreach (QDomElement text, texts) {
        text.setTagName("text");
    }

    QDomElement root = top.ownerDocument().documentElement();
    double newW = sWidth * bounds.width() / vbWidth;
    double newH = sHeight * bounds.height() / vbHeight;
    root.setAttribute("width", QString("%1in").arg(newW));
    root.setAttribute("height", QString("%1in").arg(newH));
    root.setAttribute("viewBox", QString("0 0 %1 %2").arg(bounds.width()).arg(bounds.height()));

    if (bounds.left() != 0 || bounds.top() != 0) {
        QDomElement g2 = top.ownerDocument().createElement("g");
        g2.appendChild(g);
        top.appendChild(g2);
        g2.setAttribute("transform", QString("translate(%1,%2)").arg(-bounds.left()).arg(-bounds.top()));
    }

    mps->setSubpartOffset(QPointF(bounds.left() * sWidth / vbWidth, bounds.top() * sHeight / vbHeight));
}


