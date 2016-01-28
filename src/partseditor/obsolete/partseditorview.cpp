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

$Revision: 6417 $:
$Author: cohen@irascible.com $:
$Date: 2012-09-14 23:34:09 +0200 (Fr, 14. Sep 2012) $

********************************************************************/

#include <QMessageBox>
#include <QFileDialog>
#include <QtDebug>
#include <QSvgGenerator>
#include <QGraphicsProxyWidget>
#include <QVarLengthArray>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>

#include "partseditorview.h"
#include "partseditorconnectoritem.h"
#include "fixfontsdialog.h"
#include "zoomcontrols.h"
#include "kicadmoduledialog.h"
#include "../items/layerkinpaletteitem.h"
#include "../items/partfactory.h"
#include "../layerattributes.h"
#include "../mainwindow/fritzingwindow.h"
#include "../fsvgrenderer.h"
#include "../debugdialog.h"
#include "../utils/folderutils.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"
#include "../utils/ratsnestcolors.h"
#include "../svg/gedaelement2svg.h"
#include "../svg/kicadmodule2svg.h"
#include "../svg/kicadschematic2svg.h"
#include "../connectors/connectorshared.h"


int PartsEditorView::ConnDefaultWidth = 5;
int PartsEditorView::ConnDefaultHeight = ConnDefaultWidth;

PartsEditorView::PartsEditorView(
		ViewLayer::ViewIdentifier viewId, QDir tempDir,
		bool showingTerminalPoints, QGraphicsProxyWidget *startItem,
		QWidget *parent, int size, bool deleteModelPartOnClearScene,
		ItemBase * fromItem)
	: SketchWidget(viewId, parent, size, size)
{
    m_alignToGrid = m_showGrid = false;
	m_viewItem = NULL;
	m_item = NULL;
	m_connsLayerID = ViewLayer::UnknownLayer;
	m_svgLoaded = false;
	m_deleteModelPartOnSceneClear = deleteModelPartOnClearScene;
	m_tempFolder = tempDir;
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setDefaultBackground();

	

	//spec
	m_svgFilePath = new SvgAndPartFilePath;
	m_startItem = startItem;
	if(m_startItem) {
		addFixedToCenterItem(startItem);
		ensureFixedToCenterItems();

		connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(ensureFixedItemsPositions()));
		connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(ensureFixedItemsPositions()));

		connect(this, SIGNAL(resizeSignal()), this, SLOT(ensureFixedItemsPositions()));
		connect(this, SIGNAL(wheelSignal()),  this, SLOT(ensureFixedItemsPositions()));


		// TODO: do we still need this?
		QTimer::singleShot(400, this, SLOT(ensureFixedItemsPositions()));
	}
	addDefaultLayers(fromItem);


	// conns
	m_showingTerminalPoints = showingTerminalPoints;
	m_lastSelectedConnId = "";

	setDragMode(QGraphicsView::ScrollHandDrag);

	setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

	m_terminalPointsTimer = new QTimer(this);
	connect(
		m_terminalPointsTimer,SIGNAL(timeout()),
		this,SLOT(recoverTerminalPointsState())
	);
	m_showingTerminalPointsBackup = m_showingTerminalPoints;

	m_fitItemInViewTimer = new QTimer(this);
	m_fitItemInViewTimer->setSingleShot(true);
	m_fitItemInViewTimer->setInterval(200);
	connect(m_fitItemInViewTimer,SIGNAL(timeout()),this,SLOT(fitCenterAndDeselect()));
}

PartsEditorView::~PartsEditorView() {
	if (m_startItem) delete m_startItem;
	delete m_svgFilePath;
	clearScene();
}

void PartsEditorView::addDefaultLayers(ItemBase * fromItem) {
	switch( m_viewIdentifier ) {
		case ViewLayer::BreadboardView: 
			addBreadboardViewLayers(); 
			break;
		case ViewLayer::SchematicView: 
			addSchematicViewLayers(); 
			break;
		case ViewLayer::PCBView: 
			addPcbViewLayers(); 
			if (fromItem && fromItem->modelPart()->flippedSMD()) {
				DebugDialog::debug("editing an SMD part");
				setViewLayerIDs(ViewLayer::Silkscreen1, ViewLayer::Copper1Trace, ViewLayer::Copper1, ViewLayer::PcbRuler, ViewLayer::PcbNote);
				this->m_viewLayers.remove(ViewLayer::Copper0);
				this->m_viewLayers.remove(ViewLayer::Silkscreen0);
				this->m_viewLayers.remove(ViewLayer::Copper0Trace);
			}
			break;
		default: 
			break;
	}
}

void PartsEditorView::addItemInPartsEditor(ModelPart * modelPart, SvgAndPartFilePath * svgFilePath) {
	if (modelPart == NULL) {
		throw "PartsEditorView::addItemInPartsEditor no model part";
	}
	clearScene();

	m_item = newPartsEditorPaletteItem(modelPart, svgFilePath);
	this->addItem(modelPart, defaultViewLayerSpec(), BaseCommand::CrossView, m_item->getViewGeometry(), m_item->id(), -1, NULL, m_item);

	fitCenterAndDeselect();

	setItemProperties();

	/*foreach(QWidget* w, m_fixedWidgets) {
		QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget();
		proxy->setWidget(w);

		addFixedToBottomRightItem(proxy);
	}*/

	emit connectorsFoundSignal(this->m_viewIdentifier,m_item->connectors());
}

ItemBase * PartsEditorView::addItemAux(ModelPart * modelPart, ViewLayer::ViewLayerSpec viewLayerSpec, const ViewGeometry &, long id, PaletteItem * paletteItemAux, bool doConnectors, ViewLayer::ViewIdentifier, bool temporary) {
	Q_UNUSED(id);
	Q_UNUSED(temporary);
	
	if(paletteItemAux == NULL) {
		paletteItemAux = newPartsEditorPaletteItem(modelPart);
	}
	PartsEditorPaletteItem *paletteItem = dynamic_cast<PartsEditorPaletteItem*>(paletteItemAux);
	if (paletteItem == NULL) {
		throw "PartsEditorView::addItemAux paletteItem not found";
	}

	if(paletteItem) {
		modelPart->initConnectors();    // is a no-op if connectors already in place
		QString layerFileName = getLayerFileName(modelPart);
		if(!layerFileName.isEmpty()) {
			if(paletteItem->createSvgPath(modelPart->path(), layerFileName)) {
				paletteItem->createSvgFile(paletteItem->svgFilePath()->absolutePath());
				ViewLayer::ViewLayerID viewLayerID =
					ViewLayer::viewLayerIDFromXmlString(
						findConnectorsLayerId(paletteItem->svgDom())
					);
				if(viewLayerID == ViewLayer::UnknownLayer) {
					viewLayerID = getViewLayerID(modelPart, m_viewIdentifier, viewLayerSpec);
				}
				addDefaultLayers(NULL);
				if (m_viewItem != NULL) {
					QHash<QString, QString> svgHash;
					QString svg = "";
					foreach (ViewLayer * vl, m_viewLayers.values()) {
						svg += m_viewItem->retrieveSvg(vl->viewLayerID(), svgHash, false, GraphicsUtils::StandardFritzingDPI);
					}
					if (!svg.isEmpty()) {
						QSizeF size = m_viewItem->size();
						svg = TextUtils::makeSVGHeader(GraphicsUtils::SVGDPI, GraphicsUtils::StandardFritzingDPI, size.width(), size.height()) + svg + "</svg>";
						paletteItem->setItemSVG(svg);
					}
				}

				QString error;
				if (paletteItem->renderImage(modelPart, m_viewIdentifier, m_viewLayers, viewLayerID, doConnectors, error)) {
					addToScene(paletteItemAux, paletteItemAux->viewLayerID());
					// layers are not needed on the parts editor (so far)
					return paletteItemAux;
				}
			}
		}
	}
	return NULL;
}

void PartsEditorView::fitCenterAndDeselect() {
	if(m_item) {
		m_item->setSelected(false);
		m_item->setHidden(false);
		m_item->setInactive(false);

		fitInView(m_item, Qt::KeepAspectRatio);

		QRectF viewRect = rect();
		QRectF itemsRect = scene()->itemsBoundingRect();

		double wRelation = viewRect.width()  / itemsRect.width();
		double hRelation = viewRect.height() / itemsRect.height();

		if(wRelation < hRelation) {
			m_scaleValue = (wRelation * 100);
		} else {
			m_scaleValue = (hRelation * 100);
		}

		emit zoomChanged(m_scaleValue);
	}
}

void PartsEditorView::setDefaultBackground() {
	QString bgColor = " PartsEditorView {background-color: rgb(%1,%2,%3);} ";
	QColor c = standardBackground();
	if (c.isValid()) {
		setStyleSheet(styleSheet()+bgColor.arg(c.red()).arg(c.green()).arg(c.blue()));
	}
}

void PartsEditorView::clearScene() {
	if(m_item) {
		deleteItem(m_item, m_deleteModelPartOnSceneClear, true, false);

		scene()->clear();
		m_item = NULL;
	}
}

void PartsEditorView::removeConnectors() {
	QList<PartsEditorConnectorItem*> list;
	for (int i = m_item->childItems().count()-1; i >= 0; i--) {
		PartsEditorConnectorItem * connectorItem = dynamic_cast<PartsEditorConnectorItem *>(m_item->childItems()[i]);
		if (connectorItem == NULL) continue;

		list << connectorItem;
	}

	for(int i=0; i < list.size(); i++) {
		list[i]->removeFromModel();
		delete list[i];
	}
}

ModelPart *PartsEditorView::createFakeModelPart(SvgAndPartFilePath *svgpath) {
	const QHash<QString,ConnectorTerminalSvgIdPair> connIds = getConnectorsSvgIds(svgpath->absolutePath());
	const QStringList layers = getLayers(svgpath->absolutePath());

	QString path = svgpath->relativePath().isEmpty() ? svgpath->absolutePath() : svgpath->relativePath();
	ModelPart * mp = createFakeModelPart(connIds, layers, path);

	return mp;
}

ModelPart *PartsEditorView::createFakeModelPart(const QHash<QString,ConnectorTerminalSvgIdPair> &conns, const QStringList &layers, const QString &svgFilePath) {
	QDomDocument *domDoc = new QDomDocument();
	QString errorStr;
	int errorLine;
	int errorColumn;
	QString fakeFzFile =
		QString("<module><views>\n")+
			QString("<%1><layers image='%2' >\n").arg(ViewLayer::viewIdentifierXmlName(m_viewIdentifier)).arg(svgFilePath);
		foreach(QString layer, layers) { fakeFzFile +=
			QString("    <layer layerId='%1' />\n").arg(layer);
		}
	fakeFzFile +=
			QString("</layers></%1>\n").arg(ViewLayer::viewIdentifierXmlName(m_viewIdentifier))+
			QString("</views><connectors>\n");

	QStringList defaultLayers = defaultLayerAsStringlist();

	foreach(QString id, conns.keys()) {
		QString terminalAttr = conns[id].terminalId.isEmpty() ? "" : QString("terminalId='%1'").arg(conns[id].terminalId);
		QString name = conns[id].connectorName.isEmpty() ? "" : QString("name='%1'").arg(conns[id].connectorName);		
		fakeFzFile += QString("<connector id='%1' %2><views>\n").arg(id).arg(name) +
							QString("<%1>\n").arg(ViewLayer::viewIdentifierXmlName(m_viewIdentifier));
		foreach (QString layer, defaultLayers) {
			if (layers.contains(layer)) {
				fakeFzFile += QString("<p layer='%1' svgId='%2' %3/>\n")
									.arg(layer)
									.arg(conns[id].connectorId)
									.arg(terminalAttr);
			}
		}
		fakeFzFile += QString("</%1>\n").arg(ViewLayer::viewIdentifierXmlName(m_viewIdentifier))+
						QString("</views></connector>\n");
	}
	fakeFzFile += QString("</connectors></module>\n");

    QString path = m_tempFolder.absolutePath()+"/"+FolderUtils::getRandText()+".fz";
    TextUtils::writeUtf8(path, fakeFzFile);

  	domDoc->setContent(fakeFzFile, &errorStr, &errorLine, &errorColumn);

  	ModelPart *retval = m_sketchModel->root();
  	retval->modelPartShared()->setDomDocument(domDoc);
  	retval->modelPartShared()->resetConnectorsInitialization();
	retval->modelPartShared()->setPath(path);
  	retval->initConnectors(true /*redo connectors*/);
	
	return retval;
}

const QHash<QString,ConnectorTerminalSvgIdPair> PartsEditorView::getConnectorsSvgIds(const QString &path) {
	QDomDocument dom ;
	QFile file(path);
	dom.setContent(&file);

	QDomElement docElem = dom.documentElement();
	getConnectorsSvgIdsAux(docElem);

	return m_svgIds;
}

void PartsEditorView::getConnectorsSvgIdsAux(QDomElement &docElem) {
	QDomElement e = docElem.firstChildElement();
	while(!e.isNull()) {
		QString id = e.attribute("id");
		if(id.startsWith("connector") && id.endsWith("terminal")) {
			QString conn = id.left(id.lastIndexOf(QRegExp("\\d"))+1);
			ConnectorTerminalSvgIdPair pair = m_svgIds.contains(conn) ? m_svgIds[conn] : ConnectorTerminalSvgIdPair();
			pair.terminalId = id;
			m_svgIds[conn] = pair;
		}
		else if(id.startsWith("connector") /*&& id.endsWith("pin") */ ) {
			QString conn = id.left(id.lastIndexOf(QRegExp("\\d"))+1);
			ConnectorTerminalSvgIdPair pair = m_svgIds.contains(conn) ? m_svgIds[conn] : ConnectorTerminalSvgIdPair();
			pair.connectorId = id;
			pair.connectorName = e.attribute("connectorname");
			m_svgIds[conn] = pair;
		}
		if(e.hasChildNodes()) {
			getConnectorsSvgIdsAux(e);
		}
		e = e.nextSiblingElement();
	}
}

const QStringList PartsEditorView::getLayers(const QString &path) {
	if(m_viewIdentifier == ViewLayer::IconView) { // defaulting layer to icon for iconview
		return defaultLayerAsStringlist();
	} else {
		QDomDocument dom;
		QFile file(path);
		dom.setContent(&file);
		return getLayers(&dom);
	}
}

const QStringList PartsEditorView::getLayers(const QDomDocument *dom, bool fakeDefaultIfNone) {
	QStringList retval;
	
	QDomNodeList nodeList = dom->elementsByTagName("g");
	for (uint i = 0; i < nodeList.length(); i++) {
		QDomElement e = nodeList.item(i).toElement();
		QString id = e.attribute("id");
		if (id.isEmpty()) continue;
		if (ViewLayer::viewLayerIDFromXmlString(id) == ViewLayer::UnknownLayer) continue;

		retval << id;
	}

	if(fakeDefaultIfNone && retval.isEmpty()) {
		retval << ViewLayer::viewIdentifierNaturalName(m_viewIdentifier);
	}

	return retval;
}

PartsEditorPaletteItem *PartsEditorView::newPartsEditorPaletteItem(ModelPart *modelPart) {
	return new PartsEditorConnectorsPaletteItem(this, modelPart, m_viewIdentifier);
}

PartsEditorPaletteItem *PartsEditorView::newPartsEditorPaletteItem(ModelPart * modelPart, SvgAndPartFilePath *path) {
	return new PartsEditorConnectorsPaletteItem(this, modelPart, m_viewIdentifier, path);
}

QDir PartsEditorView::tempFolder() {
	return m_tempFolder;
}

QString PartsEditorView::getOrCreateViewFolderInTemp() {
	QString viewFolder = ViewLayer::viewIdentifierNaturalName(m_viewIdentifier);

	if(!QFileInfo(m_tempFolder.absolutePath()+"/"+viewFolder).exists()) {
		bool mkResult = m_tempFolder.mkpath(m_tempFolder.absolutePath()+"/"+viewFolder);
		if (!mkResult) {
			throw "PartsEditorView::getOrCreateViewFolderInTemp failed";
		}
	}

	return viewFolder;
}

bool PartsEditorView::isEmpty() {
	return m_item == NULL;
}

bool PartsEditorView::ensureFilePath(const QString &filePath) {
	QString svgFolder = FolderUtils::getUserDataStorePath("parts")+"/svg";

	Qt::CaseSensitivity cs = Qt::CaseSensitive;
#ifdef Q_WS_WIN
	cs = Qt::CaseInsensitive;
#endif
	if(!filePath.contains(svgFolder, cs)) {
		// This has to be here in order of all this, to work in release mode
		m_tempFolder.mkpath(QFileInfo(filePath).absoluteDir().path());
	}
	return true;
}

ViewLayer::ViewLayerID PartsEditorView::connectorsLayerId() {
	//Q_ASSERT(m_item);
	findConnectorsLayerId();
	return m_connsLayerID;
}

QString PartsEditorView::terminalIdForConnector(const QString &connId) {
	//Q_ASSERT(m_item)

	if (m_item == NULL) return "";

	QString result = "";
	QDomElement elem = m_item->svgDom()->documentElement();
	if(terminalIdForConnectorIdAux(result, connId, elem, true)) {
		return result;
	} else {
		return "";
	}
}

bool PartsEditorView::terminalIdForConnectorIdAux(QString &result, const QString &connId, QDomElement &docElem, bool wantTerminal) {
	QDomElement e = docElem.firstChildElement();
	while(!e.isNull()) {
		QString id = e.attribute("id");
		if(id.startsWith(connId) && ((wantTerminal && id.endsWith("terminal")) || (!wantTerminal && !id.endsWith("terminal")))) {
			// the id is the one from the previous iteration
			result = id;
			return true;
		} else if(e.hasChildNodes()) {
			// potencial solution, if the next iteration returns true
			if(terminalIdForConnectorIdAux(result, connId, e, wantTerminal)) {
				return true;
			}
		}
		e = e.nextSiblingElement();
	}
	return false;
}

void PartsEditorView::findConnectorsLayerId() {
	if(m_connsLayerID == ViewLayer::UnknownLayer) {
		if (m_item != NULL) {
			m_connsLayerID = ViewLayer::viewLayerIDFromXmlString(
				findConnectorsLayerId(m_item->svgDom())
			);
		}
		if(m_connsLayerID == ViewLayer::UnknownLayer) {
			m_connsLayerID = SketchWidget::defaultConnectorLayer(m_viewIdentifier);
		}
	}
}

QStringList PartsEditorView::findConnectorsLayerIds(QDomDocument *svgDom) {
	QStringList result;
	QDomElement docElem = svgDom->documentElement();
	findConnectorsLayerIdsAux(result, docElem);
	if (result.count() > 0) return result;

	return defaultLayerAsStringlist();
}

void PartsEditorView::findConnectorsLayerIdsAux(QStringList &result, QDomElement &docElem) {
	QDomElement e = docElem.firstChildElement();
	while(!e.isNull()) {
		QString id = e.attribute("id");
		if (id.startsWith("connector")) {
			QDomElement parent = e.parentNode().toElement();
			QString id = parent.attribute("id");
			if (!id.isEmpty() && (ViewLayer::viewLayerIDFromXmlString(id) != ViewLayer::UnknownLayer)) {
				result << id;
			}
		} 
		else if(e.hasChildNodes()) {
			findConnectorsLayerIdsAux(result, e);
		}
		e = e.nextSiblingElement();
	}
}

QString PartsEditorView::findConnectorsLayerId(QDomDocument *svgDom) {
	QString result;
	QStringList layers;
	QDomElement docElem = svgDom->documentElement();
	if(findConnectorsLayerIdAux(result, docElem, layers)) {
		if(ViewLayer::viewLayerIDFromXmlString(result) == ViewLayer::UnknownLayer) {
			foreach(QString layer, layers) {
				ViewLayer::ViewLayerID vlid = ViewLayer::viewLayerIDFromXmlString(layer);
				if(m_viewLayers.keys().contains(vlid)) {
					result = layer;
				}
			}
		}
		return result;
	} else {
		return defaultLayerAsStringlist().at(0);
	}
}

bool PartsEditorView::findConnectorsLayerIdAux(QString &result, QDomElement &docElem, QStringList &prevLayers) {
	QDomElement e = docElem.firstChildElement();
	while(!e.isNull()) {
		QString id = e.attribute("id");
		if(id.startsWith("connector")) {
			// the id is the one from the previous iteration
			return true;
		} else if(e.hasChildNodes()) {
			// potencial solution, if the next iteration returns true
			result = id;
			prevLayers << id;
			if(findConnectorsLayerIdAux(result, e, prevLayers)) {
				return true;
			}
		}
		e = e.nextSiblingElement();
	}
	return false;
}

QString PartsEditorView::getLayerFileName(ModelPart * modelPart) {
    return modelPart->imageFileName(m_viewIdentifier);
}


// specs
void PartsEditorView::copySvgFileToDestiny(const QString &partFileName) {
	Qt::CaseSensitivity cs = Qt::CaseSensitive;
#ifdef Q_WS_WIN
	cs = Qt::CaseInsensitive;
#endif

	// if the svg file is in the temp folder, then copy it to destiny
	if(m_svgFilePath->absolutePath().startsWith(m_tempFolder.absolutePath(),cs)) {
		QString origFile = svgFilePath();
		setFriendlierSvgFileName(partFileName);
		QString destFile = FolderUtils::getUserDataStorePath("parts")+"/svg/user/"+m_svgFilePath->relativePath();

		ensureFilePath(origFile);
		QFile tempFile(origFile);
		DebugDialog::debug(QString("copying from %1 to %2")
				.arg(origFile)
				.arg(destFile));
		tempFile.copy(destFile);
		tempFile.close();

		// update the item info, to point to this file
		m_svgFilePath->setAbsolutePath(destFile);
	}
}

void PartsEditorView::loadFile() {
	QStringList extras;
	extras.append("");
	extras.append("");
	QString imageFiles;
	if (m_viewIdentifier == ViewLayer::PCBView) {
		imageFiles = tr("Image & Footprint Files (%1 %2 %3 %4 %5);;SVG Files (%1);;JPEG Files (%2);;PNG Files (%3);;gEDA Footprint Files (%4);;Kicad Module Files (%5)");   // 
		extras[0] = "*.fp";
		extras[1] = "*.mod";
	}
	else {
		imageFiles = tr("Image Files (%1 %2 %3);;SVG Files (%1);;JPEG Files (%2);;PNG Files (%3)%4%5");
	}

	if (m_viewIdentifier == ViewLayer::SchematicView) {
		extras[0] = "*.lib";
		imageFiles = tr("Image & Footprint Files (%1 %2 %3 %4);;SVG Files (%1);;JPEG Files (%2);;PNG Files (%3);;Kicad Schematic Files (%4)%5");   // 
	}

	QString origPath = FolderUtils::getOpenFileName(this,
		tr("Open Image"),
		m_originalSvgFilePath.isEmpty() ? FolderUtils::openSaveFolder() /* FolderUtils::getUserDataStorePath("parts")+"/parts/svg/" */ : m_originalSvgFilePath,
		imageFiles.arg("*.svg").arg("*.jpg *.jpeg").arg("*.png").arg(extras[0]).arg(extras[1])
	);

	if(origPath.isEmpty()) {
		return; // Cancel pressed
	} 

	if(!origPath.endsWith(".svg")) {
		try {
			origPath = createSvgFromImage(origPath);
		}
		catch (const QString & msg) {
    		QMessageBox::warning(
    			NULL,
    			tr("Conversion problem"),
    			tr("Unable to load image file: \n%1").arg(msg)
    		);
			return;
		}
	}
	if(!origPath.isEmpty()) {
		if(m_startItem) {
			m_fixedToCenterItems.removeAll(m_startItem);
			delete m_startItem;
			m_startItem = NULL;
		}
		m_viewItem = NULL;				// loading a new file, so m_viewItem is obsolete
		loadSvgFile(origPath);
	}
}

void PartsEditorView::updateModelPart(const QString& origPath) {
	m_undoStack->push(new QUndoCommand("Dummy parts editor command"));

	setSvgFilePath(origPath);
	copyToTempAndRenameIfNecessary(m_svgFilePath);
	m_item->setSvgFilePath(m_svgFilePath);

	ModelPart *mp = createFakeModelPart(m_svgFilePath);
	m_item->setModelPart(mp);
}

void PartsEditorView::loadSvgFile(const QString& origPath) {
	// back to an empty state
	m_drawnConns.clear();
	m_removedConnIds.clear();
	m_connsLayerID = ViewLayer::UnknownLayer;
	m_svgIds.clear();

	m_svgLoaded = true;

	bool canceled = false;
	beforeSVGLoading(origPath, canceled);

	if(!canceled) {
		m_undoStack->push(new QUndoCommand("Dummy parts editor command"));
		setSvgFilePath(origPath);

		// TODO: this code reuses the current modelpart and replaces its connectors,
		// it would be better to delete the modelpart and create a new one
		// however, one would have to tidy up the various objects that rely on pointers 
		// to the original modelpart and its connectors
		ModelPart * mp = createFakeModelPart(m_svgFilePath);
		loadSvgFile(mp);
	}
}

void PartsEditorView::beforeSVGLoading(const QString &filename, bool &canceled) {
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly )) {
    	QMessageBox::warning(
    		NULL,
    		tr("Couldn't open svg file"),
    		tr(
    		"The file couldn't be opened. If this file defines its dimensions \n"
    		"in non-real-world units (e.g. pixels), then they won't be translated \n"
    		"into real life ones.\n"
    		"Malformed font-family definitions won't be fixed either.")
    	);
        return;
    }

    QString fileContent(file.readAll());
	bool fileHasChanged = (m_viewIdentifier == ViewLayer::IconView) ? false : TextUtils::fixPixelDimensionsIn(fileContent);
	fileHasChanged |= TextUtils::cleanSodipodi(fileContent);
	fileHasChanged |= TextUtils::fixViewboxOrigin(fileContent);
	fileHasChanged |= TextUtils::tspanRemove(fileContent);
	fileHasChanged |= fixFonts(fileContent,filename,canceled);

	if(fileHasChanged) {
		file.close();
		if(!TextUtils::writeUtf8(filename, fileContent)) {
			QMessageBox::warning(
				NULL,
				tr("Couldn't write into file"),
				tr(
				"This file needs to be fixed to fit fritzing needs, but it couldn't\n"
				"be written.\n"
				"Fritzing is not compatible with this kind of svg files. Please \n"
				"check your permissions, and try again.\n\n"

				"More information at http://fritzing.org/using-svg-images-new-parts/"
				)
			);
		} 
	}

}

bool PartsEditorView::fixFonts(QString &fileContent, const QString &filename, bool &canceled) {
	bool changed = removeFontFamilySingleQuotes(fileContent, filename);
	changed |= fixUnavailableFontFamilies(fileContent, filename, canceled);

	return changed;
}

bool PartsEditorView::removeFontFamilySingleQuotes(QString &fileContent, const QString &filename) {
	QString pattern = "font-family=\"('.*')\"";
	QSet<QString> wrongFontFamilies = TextUtils::getRegexpCaptures(pattern,fileContent);

	foreach(QString ff, wrongFontFamilies) {
		QString wrongFF = ff;
		QString fixedFF = ff.remove('\'');
		fileContent.replace(wrongFF,fixedFF);
		DebugDialog::debug(
			QString("removing font-family single quotes: \"%1\" to \"%2\" in file '%3'")
				.arg(wrongFF).arg(fixedFF).arg(filename)
		);
	}

	return wrongFontFamilies.size() > 0;
}

bool PartsEditorView::fixUnavailableFontFamilies(QString &fileContent, const QString &filename, bool &canceled) {
	QSet<QString> definedFFs;
	definedFFs.unite(getAttrFontFamilies(fileContent));
	definedFFs.unite(getFontFamiliesInsideStyleTag(fileContent));

	FixedFontsHash fixedFonts = FixFontsDialog::fixFonts(this,definedFFs,canceled);

	if(!canceled) {
		foreach(QString oldF, fixedFonts.keys()) {
			QString newF = fixedFonts[oldF];
			fileContent.replace(oldF,newF);
			DebugDialog::debug(
				QString("replacing font-family: \"%1\" to \"%2\" in file '%3'")
					.arg(oldF).arg(newF).arg(filename)
			);
		}
	}

	return !canceled && fixedFonts.size() > 0;
}

QSet<QString> PartsEditorView::getAttrFontFamilies(const QString &fileContent) {
	/*
	 * font-family defined as attr example:

<text xmlns="http://www.w3.org/2000/svg" font-family="DroidSans"
id="text2732" transform="matrix(1 0 0 1 32.2012 236.969)"
font-size="9.9771" >A0</text>

	 */

	QString pattern = "font-family\\s*=\\s*\"(.|[^\"]*)\\s*\"";
	return TextUtils::getRegexpCaptures(pattern,fileContent);
}

QSet<QString> PartsEditorView::getFontFamiliesInsideStyleTag(const QString &fileContent) {
	/*
	 * regexp: font-family\s*:\s*(.|[^;"]*).*"
	 * font-family defined in a style attr example:

style="font-size:9;-inkscape-font-specification:Droid Sans;font-family:Droid Sans;font-weight:normal;font-style:normal;font-stretch:normal;font-variant:normal"

style="font-size:144px;font-style:normal;font-weight:normal;line-height:100%;fill:#ffffff;fill-opacity:1;stroke:none;stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1;font-family:Bitstream Vera Sans" x="18.000002"

	 */

	QString pattern = "font-family\\s*:\\s*(.|[^;\"]*).*\"";
	return TextUtils::getRegexpCaptures(pattern,fileContent);
}

void PartsEditorView::loadSvgFile(ModelPart * modelPart) {
	addItemInPartsEditor(modelPart, m_svgFilePath);
	copyToTempAndRenameIfNecessary(m_svgFilePath);
	m_item->setSvgFilePath(m_svgFilePath);
}

void PartsEditorView::loadFromModel(PaletteModel *paletteModel, ModelPart * modelPart) {
	clearScene();

	ViewGeometry viewGeometry;
	this->setPaletteModel(paletteModel);
	m_item = (PartsEditorPaletteItem*) addItemAux(modelPart, defaultViewLayerSpec(), viewGeometry, ItemBase::getNextID(), NULL, true, m_viewIdentifier, true);

	fitCenterAndDeselect();

	setItemProperties();

	if(m_item) {
		if(m_startItem) {
			m_fixedToCenterItems.removeAll(m_startItem);
			delete m_startItem;
			m_startItem = NULL;
		}


		SvgAndPartFilePath *sp = m_item->svgFilePath();

		copyToTempAndRenameIfNecessary(sp);
		m_item->setSvgFilePath(m_svgFilePath);
	}
}

void PartsEditorView::copyToTempAndRenameIfNecessary(SvgAndPartFilePath *filePathOrig) {
	m_originalSvgFilePath = filePathOrig->absolutePath();
	QString userSvgFolderPath = FolderUtils::getUserDataStorePath("parts")+"/svg";
	QString coreSvgFolderPath = FolderUtils::getApplicationSubFolderPath("parts")+"/svg";
	QString pfSvgFolderPath = PartFactory::folderPath()+"/svg"; 

	if(!(filePathOrig->absolutePath().startsWith(userSvgFolderPath)
		|| filePathOrig->absolutePath().startsWith(coreSvgFolderPath)
		|| filePathOrig->absolutePath().startsWith(pfSvgFolderPath))
		) 
	{ // it's outside the parts folder
		DebugDialog::debug(QString("copying from %1").arg(m_originalSvgFilePath));
		QString viewFolder = ViewLayer::viewIdentifierNaturalName(m_viewIdentifier);

		if(!QFileInfo(m_tempFolder.path()+"/"+viewFolder).exists()
		   && !m_tempFolder.mkdir(viewFolder)) return;
		if(!m_tempFolder.cd(viewFolder)) return;

		QString destFilePath = FolderUtils::getRandText()+".svg";
		DebugDialog::debug(QString("dest file: %1").arg(m_tempFolder.absolutePath()+"/"+destFilePath));

		ensureFilePath(m_tempFolder.absolutePath()+"/"+destFilePath);

		QFile tempFile(m_originalSvgFilePath);
		tempFile.copy(m_tempFolder.absolutePath()+"/"+destFilePath);
		tempFile.close();

		if(!m_tempFolder.cd("..")) return; // out of view folder

		m_svgFilePath->setRelativePath(viewFolder+"/"+destFilePath);
		m_svgFilePath->setAbsolutePath(m_tempFolder.absolutePath()+"/"+m_svgFilePath->relativePath());

	} else {
		QString relPathAux = filePathOrig->relativePath();
		m_svgFilePath->setAbsolutePath(m_originalSvgFilePath);
		if (relPathAux.count("/") > 2) {
			throw "PartsEditorView::copyToTempAndRenameIfNecessary bad path";
		}

		if(relPathAux.count("/") == 2) { // this means that core/user/contrib is still in the file name
			m_svgFilePath->setRelativePath(
				relPathAux.right(// remove user/core/contrib
					relPathAux.size() -
					relPathAux.indexOf("/") - 1
				)
			);
		} else { //otherwise, just leave it as it is
			m_svgFilePath->setRelativePath(relPathAux);
		}
	}
}

void PartsEditorView::setSvgFilePath(const QString &filePath) {
	ensureFilePath(filePath);
	m_originalSvgFilePath = filePath;

	QString userSvgFolder = FolderUtils::getUserDataStorePath("parts")+"/svg";
	QString coreSvgFolder = FolderUtils::getApplicationSubFolderPath("parts")+"/svg";
	QString pfSvgFolder = PartFactory::folderPath()+"/svg";

	QString tempFolder = m_tempFolder.absolutePath();

	QString relative;
	Qt::CaseSensitivity cs = Qt::CaseSensitive;
	QString filePathAux = filePath;

#ifdef Q_WS_WIN
	// seems to be necessary for Windows: getUserDataStorePath() returns a string starting with "c:"
	// but the file dialog returns a string beginning with "C:"
	cs = Qt::CaseInsensitive;
#endif
	if(filePath.contains(userSvgFolder, cs) || filePath.contains(coreSvgFolder, cs) || filePath.contains(pfSvgFolder, cs)) {
		int ix = filePath.indexOf("svg");
		// is core/user file
		relative = filePathAux.remove(0, ix + 4);
		//Mariano: I don't like this folder thing anymore
		relative = relative.mid(filePathAux.indexOf("/")+1); // remove core/user/contrib
	} else {
		// generated jpeg/png or file outside fritzing folder
		relative = "";
	}

	if (m_svgFilePath) delete m_svgFilePath;
	m_svgFilePath = new SvgAndPartFilePath(filePath,relative);
}


const QString PartsEditorView::svgFilePath() {
	return m_svgFilePath->absolutePath();
}

const SvgAndPartFilePath& PartsEditorView::svgFileSplit() {
	return *m_svgFilePath;
}

QString PartsEditorView::createSvgFromImage(const QString &origFilePath) {
	QString viewFolder = getOrCreateViewFolderInTemp();

	QString newFilePath = m_tempFolder.absolutePath()+"/"+viewFolder+"/"+FolderUtils::getRandText()+".svg";
	ensureFilePath(newFilePath);

	if (origFilePath.endsWith(".fp")) {
		// this is a geda footprint file
		GedaElement2Svg geda;
		QString svg = geda.convert(origFilePath, false);
		return saveSvg(svg, newFilePath);
	}

	if (origFilePath.endsWith(".lib")) {
		// Kicad schematic library file
		QStringList defs = KicadSchematic2Svg::listDefs(origFilePath);
		if (defs.count() == 0) {
			throw tr("no schematics found in %1").arg(origFilePath);
		}

		QString def;
		if (defs.count() > 1) {
			KicadModuleDialog kmd(tr("schematic part"), origFilePath, defs, this);
			int result = kmd.exec();
			if (result != QDialog::Accepted) {
				return "";
			}

			def = kmd.selectedModule();
		}
		else {
			def = defs.at(0);
		}

		KicadSchematic2Svg kicad;
		QString svg = kicad.convert(origFilePath, def);
		return saveSvg(svg, newFilePath);
	}

	if (origFilePath.endsWith(".mod")) {
		// Kicad footprint (Module) library file
		QStringList modules = KicadModule2Svg::listModules(origFilePath);
		if (modules.count() == 0) {
			throw tr("no footprints found in %1").arg(origFilePath);
		}

		QString module;
		if (modules.count() > 1) {
			KicadModuleDialog kmd("footprint", origFilePath, modules, this);
			int result = kmd.exec();
			if (result != QDialog::Accepted) {
				return "";
			}

			module = kmd.selectedModule();
		}
		else {
			module = modules.at(0);
		}

		KicadModule2Svg kicad;
		QString svg = kicad.convert(origFilePath, module, false);
		return saveSvg(svg, newFilePath);
	}

	// deal with png, jpg, etc.:


/* %1=witdh in mm
 * %2=height in mm
 * %3=width in local coords
 * %4=height in local coords
 * %5=binary data
 */
/*	QString svgTemplate =
"<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
"	<svg width='%1mm' height='%2mm' viewBox='0 0 %3 %4' xmlns='http://www.w3.org/2000/svg'\n"
"		xmlns:xlink='http://www.w3.org/1999/xlink' version='1.2' baseProfile='tiny'>\n"
"		<g fill='none' stroke='black' vector-effect='non-scaling-stroke' stroke-width='1'\n"
"			fill-rule='evenodd' stroke-linecap='square' stroke-linejoin='bevel' >\n"
"			<image x='0' y='0' width='%3' height='%4'\n"
"				xlink:href='data:image/png;base64,%5' />\n"
"		</g>\n"
"	</svg>";

	QPixmap pixmap(origFilePath);
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	pixmap.save(&buffer,"png"); // writes pixmap into bytes in PNG format

	QString svgDom = svgTemplate
		.arg(pixmap.widthMM()).arg(pixmap.heightMM())
		.arg(pixmap.width()).arg(pixmap.height())
		.arg(QString("data:image/png;base64,%2").arg(QString(bytes.toBase64())));

	QFile destFile(newFilePath);
	if(!destFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::information(NULL, "", "file not created");
		if(!destFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
				QMessageBox::information(NULL, "", "file not created 2");
			}
	}
	QTextStream out(&destFile);
	out << svgDom;
	destFile.close();
	qDebug() << newFilePath;
	bool existsResult = QFileInfo(newFilePath).exists();
	Q_ASSERT(existsResult);
*/

	QImage img(origFilePath);
	QSvgGenerator svgGenerator;
	svgGenerator.setResolution(90);
	svgGenerator.setFileName(newFilePath);
	QSize sz = img.size();
    svgGenerator.setSize(sz);
	svgGenerator.setViewBox(QRect(0, 0, sz.width(), sz.height()));
	QPainter svgPainter(&svgGenerator);
	svgPainter.drawImage(QPoint(0,0), img);
	svgPainter.end();

	return newFilePath;
}

QString PartsEditorView::setFriendlierSvgFileName(const QString &partFileName) {
	QString aux = partFileName;
	aux = aux
		.remove(FritzingPartExtension)
		.replace(" ","_");
	if(aux.length()>40) aux.truncate(40);
	aux+=QString("__%1__%2.svg")
			.arg(ViewLayer::viewIdentifierNaturalName(m_viewIdentifier))
			.arg(FolderUtils::getRandText());
	int slashIdx = m_svgFilePath->relativePath().indexOf("/");
	QString relpath = m_svgFilePath->relativePath();
	QString relpath2 = relpath;
	QString abspath = m_svgFilePath->absolutePath();
	QString viewFolder = relpath.remove(slashIdx,relpath.size()-slashIdx+1);
	m_svgFilePath->setAbsolutePath(abspath.remove(relpath2)+viewFolder+"/"+aux);
	m_svgFilePath->setRelativePath(viewFolder+"/"+aux);
	return aux;
}


// conns
void PartsEditorView::wheelEvent(QWheelEvent* event) {
	if(m_showingTerminalPoints) {
		if(!m_terminalPointsTimer->isActive()) {
			m_showingTerminalPointsBackup = m_showingTerminalPoints;
			showTerminalPoints(false);
			m_terminalPointsTimer->start(50);
		}
	} else if(m_terminalPointsTimer->isActive()) {
		m_terminalPointsTimer->stop();
		m_terminalPointsTimer->start(50);
	}
	SketchWidget::wheelEvent(event);
}

void PartsEditorView::mousePressEvent(QMouseEvent *event) {
	SketchWidget::mousePressEvent(event);
}

void PartsEditorView::mouseMoveEvent(QMouseEvent *event) {
	QGraphicsView::mouseMoveEvent(event);
}

void PartsEditorView::mouseReleaseEvent(QMouseEvent *event) {
	SketchWidget::mouseReleaseEvent(event);
}

void PartsEditorView::resizeEvent(QResizeEvent * event) {
	SketchWidget::resizeEvent(event);
	if(m_fitItemInViewTimer->isActive()) {
		m_fitItemInViewTimer->stop();
	}
	m_fitItemInViewTimer->start();

}

void PartsEditorView::drawConector(Connector *conn, bool showTerminalPoint) {
	QSize size(ConnDefaultWidth,ConnDefaultHeight);
	createConnector(conn,size,showTerminalPoint);
}

void PartsEditorView::createConnector(Connector *conn, const QSize &connSize, bool showTerminalPoint) {
	QString connId = conn->connectorSharedID();

	QRectF bounds = m_item
			? QRectF(m_item->boundingRect().center(),connSize)
			: QRectF(scene()->itemsBoundingRect().center(),connSize);
	PartsEditorConnectorsConnectorItem *connItem = new PartsEditorConnectorsConnectorItem(conn, m_item, m_showingTerminalPoints, bounds);
	m_drawnConns[connId] = connItem;
	connItem->setShowTerminalPoint(showTerminalPoint);

	m_undoStack->push(new QUndoCommand(
		QString("connector '%1' added to %2 view")
		.arg(connId).arg(ViewLayer::viewIdentifierName(m_viewIdentifier))
	));
}

void PartsEditorView::removeConnector(const QString &connId) {
	ConnectorItem *connToRemove = NULL;
	foreach(QGraphicsItem *item, items()) {
		ConnectorItem *connItem = dynamic_cast<ConnectorItem*>(item);
		if(connItem && connItem->connector()->connectorSharedID() == connId) {
			connToRemove = connItem;
			break;
		}
	}

	if(connToRemove) {
		scene()->removeItem(connToRemove);
		scene()->update();
		m_undoStack->push(new QUndoCommand(
			QString("connector '%1' removed from %2 view")
			.arg(connId).arg(ViewLayer::viewIdentifierName(m_viewIdentifier))
		));

		PartsEditorConnectorsConnectorItem *connToRemoveAux = dynamic_cast<PartsEditorConnectorsConnectorItem*>(connToRemove);
		m_drawnConns.remove(connToRemoveAux->connectorSharedID());
		m_removedConnIds << connId;
	}
}

void PartsEditorView::setItemProperties() {
	if(m_item) {
		m_item->setFlag(QGraphicsItem::ItemIsSelectable, false);
		m_item->setFlag(QGraphicsItem::ItemIsMovable, false);
		m_item->setFlag(QGraphicsItem::ItemClipsToShape, true);
		//m_item->setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
		myItem()->highlightConnectors(m_lastSelectedConnId);

		double size = 500; // just make sure the user get enough space to play
		setSceneRect(0,0,size,size);


		m_item->setPos((size-m_item->size().width())/2,(size-m_item->size().height())/2);
		centerOn(m_item);

	}
	//ensureFixedToBottomRight(m_zoomControls);
}

void PartsEditorView::informConnectorSelection(const QString &connId) {
	if(m_item) {
		m_lastSelectedConnId = connId;
		myItem()->highlightConnectors(connId);
	}
}

void PartsEditorView::informConnectorSelectionFromView(const QString &connId) {
	informConnectorSelection(connId);
	emit connectorSelected(connId);
}

void PartsEditorView::setMismatching(ViewLayer::ViewIdentifier viewId, const QString &id, bool mismatching) {
	if(m_item && viewId == m_viewIdentifier) {
		for (int i = 0; i < m_item->childItems().count(); i++) {
			PartsEditorConnectorsConnectorItem * connectorItem
				= dynamic_cast<PartsEditorConnectorsConnectorItem *>(m_item->childItems()[i]);
			if(connectorItem == NULL) continue;

			if(connectorItem->connector()->connectorSharedID() == id) {
				connectorItem->setMismatching(mismatching);
			}
		}
	}
}

void PartsEditorView::aboutToSave(bool fakeDefaultIfNone) {
	if(m_item) {
		FSvgRenderer renderer;
		QByteArray bytes = renderer.loadSvg(m_item->flatSvgFilePath());
		if (!bytes.isEmpty()) {
			QRectF svgViewBox = renderer.viewBoxF();
			QSizeF sceneViewBox = renderer.defaultSizeF();
			QDomDocument *svgDom = m_item->svgDom();

			// this may change the layers defined in the file, so
			// let's get the connectorsLayer after it
			bool somethingChanged = addDefaultLayerIfNotInSvg(svgDom, fakeDefaultIfNone);

			QDomElement elem = svgDom->documentElement();

			somethingChanged |= removeConnectorsIfNeeded(elem);
			QStringList connectorsLayerIds = findConnectorsLayerIds(svgDom);
			foreach (QString connectorsLayerId, connectorsLayerIds) {
				somethingChanged |= updateTerminalPoints(svgDom, sceneViewBox, svgViewBox, connectorsLayerId);
				somethingChanged |= addConnectorsIfNeeded(svgDom, sceneViewBox, svgViewBox, connectorsLayerId);
			}
			somethingChanged |= (m_viewItem != NULL);

			if(somethingChanged) {
				QString viewFolder = getOrCreateViewFolderInTemp();

				QString tempFile = m_tempFolder.absolutePath()+"/"+viewFolder+"/"+FolderUtils::getRandText()+".svg";

				ensureFilePath(tempFile);

				if(!TextUtils::writeUtf8(tempFile, TextUtils::removeXMLEntities(svgDom->toString()))) {
					/*QMessageBox::information(NULL,"",
						QString("Couldn't open file for update, after drawing connectors: '%1'")
							.arg(tempFile)
					);*/
                    DebugDialog::debug(QString("Couldn't open file for update, after drawing connectors: '%1'").arg(tempFile));
				}
			}
		} else {
			DebugDialog::debug("updating part view svg file: could not load file "+m_item->flatSvgFilePath());
		}
	}
}

bool PartsEditorView::addConnectorsIfNeeded(QDomDocument *svgDom, const QSizeF &sceneViewBox, const QRectF &svgViewBox, const QString &connectorsLayerId) {
	bool changed = false;
	if(!m_drawnConns.isEmpty()) {
		QRectF bounds;
		QString connId;

		foreach(PartsEditorConnectorsConnectorItem* drawnConn, m_drawnConns.values()) {
			bounds = drawnConn->mappedRect();
			connId = drawnConn->connector()->connectorSharedID();

			QRectF svgRect = mapFromSceneToSvg(bounds,sceneViewBox,svgViewBox);
			QString svgId = svgIdForConnector(drawnConn->connector(), connId);
			addRectToSvg(svgDom,svgId,svgRect, connectorsLayerId);
		}
		changed = true;
	}

	return changed;
}


bool PartsEditorView::addDefaultLayerIfNotInSvg(QDomDocument *svgDom, bool fakeDefaultIfNone) 
{
	QStringList defaultLayers = defaultLayerAsStringlist();
	QStringList layers = getLayers(svgDom, fakeDefaultIfNone);
	foreach (QString defaultLayer, defaultLayers) {
		if (layers.contains(defaultLayer)) return false;
	}

	// jrc 4/7/2010: not sure if making a new top level layer is correct
	// since it could swallow other layers

	QDomElement docElem = svgDom->documentElement();

	QDomElement newTopLevel = svgDom->createElement("g");
	newTopLevel.setAttribute("id", defaultLayers.at(0));

	// place the child in a aux list, cause the
	// qdomnodelist takes care of the references
	QList<QDomNode> children;
	for(QDomNode child=docElem.firstChild(); !child.isNull(); child=child.nextSibling()) {
		children << child;
	}

	foreach(QDomNode child, children) {
		newTopLevel.appendChild(child);
	}

	docElem.appendChild(newTopLevel);

	return true;
}

LayerList PartsEditorView::defaultLayers() {
	LayerList layers;
	switch( m_viewIdentifier ) {
		case ViewLayer::IconView: 
			layers << ViewLayer::Icon; 
			break;
		case ViewLayer::BreadboardView: 
			layers << ViewLayer::Breadboard; 
			break;
		case ViewLayer::SchematicView: 
			layers << ViewLayer::Schematic; 
			break;
		case ViewLayer::PCBView: 
			layers << ViewLayer::Copper0 << ViewLayer::Copper1;
			break;
		default: 
			layers << ViewLayer::UnknownLayer;
			break;
	}
	return layers;
}

QStringList PartsEditorView::defaultLayerAsStringlist() {
	QStringList layers;
	foreach (ViewLayer::ViewLayerID viewLayerID, defaultLayers()) {
		layers << ViewLayer::viewLayerXmlNameFromID(viewLayerID);
	}

	return layers;
}

QString PartsEditorView::svgIdForConnector(const QString &connId) {
	//Q_ASSERT(m_item)

	if (m_item == NULL) return connId;


	QString result = "";
	QDomElement elem = m_item->svgDom()->documentElement();
	if(terminalIdForConnectorIdAux(result, connId, elem, false)) {
		return result;
	} 

/*
	foreach(Connector* conn, m_item->connectors()) {
		QString svgId = svgIdForConnector(conn, connId);
		if(connId != svgId) {
			return svgId;
		}
	}

*/

	return connId;
}

QString PartsEditorView::svgIdForConnector(Connector* conn, const QString &connId) {
	if (conn->connectorShared() && conn->connectorSharedID() == connId) {
		foreach(SvgIdLayer *sil, conn->connectorShared()->pins().values(m_viewIdentifier)) {
			return sil->m_svgId;
		}
	}
	return connId;
}

bool PartsEditorView::updateTerminalPoints(QDomDocument *svgDom, const QSizeF &sceneViewBox, const QRectF &svgViewBox, const QString &connectorsLayerId) {
	QList<PartsEditorConnectorsConnectorItem*> connsWithNewTPs;
	QStringList tpIdsToRemove;
	foreach(QGraphicsItem *item, items()) { 
		PartsEditorConnectorsConnectorItem *citem = dynamic_cast<PartsEditorConnectorsConnectorItem*>(item);
		if(citem) {
			TerminalPointItem *tp = citem->terminalPointItem();
			if (!tp) {
				citem->setShowTerminalPoint(citem->isShowingTerminalPoint());
				tp = citem->terminalPointItem();
			}

			QString connId = citem->connector()->connectorSharedID();
			QString terminalId = connId+"terminal";

			if(tp && !tp->isInTheCenter()) {
				if(tp->hasBeenMoved() || citem->hasBeenMoved()) {
					connsWithNewTPs << citem;
					tpIdsToRemove << terminalId;
					//DebugDialog::debug("<<<< MOVED! removing terminal "+terminalId+" in view: "+ViewLayer::viewIdentifierName(m_viewIdentifier));
					updateSvgIdLayer(connId, terminalId, connectorsLayerId);
				}
			} else {
				//DebugDialog::debug("<<<< removing terminal "+terminalId+" in view: "+ViewLayer::viewIdentifierName(m_viewIdentifier));
				tpIdsToRemove << terminalId;
				emit removeTerminalPoint(connId, m_viewIdentifier);
			}
		}
	}
	QDomElement elem = svgDom->documentElement();
	removeTerminalPoints(tpIdsToRemove,elem);
	addNewTerminalPoints(connsWithNewTPs, svgDom, sceneViewBox, svgViewBox, connectorsLayerId);
	return !tpIdsToRemove.isEmpty();
}

void PartsEditorView::updateSvgIdLayer(const QString &connId, const QString &terminalId, const QString &connectorsLayerId) {
	ViewLayer::ViewLayerID viewLayerID = ViewLayer::viewLayerIDFromXmlString(connectorsLayerId);
	foreach(Connector *conn, m_item->connectors()) {
		foreach(SvgIdLayer *sil, conn->connectorShared()->pins().values(m_viewIdentifier)) {
			if(conn->connectorSharedID() == connId) {
				if (sil->m_svgViewLayerID == viewLayerID) {
					sil->m_terminalId = terminalId;
					return;
				}
			}
		}

		foreach(SvgIdLayer *sil, conn->connectorShared()->pins().values(m_viewIdentifier)) {
			if(conn->connectorSharedID() == connId) {
				sil->m_terminalId = terminalId;
				
				if(viewLayerID != ViewLayer::UnknownLayer) {
					sil->m_svgViewLayerID = viewLayerID;
				}
			}
		}
	}
}

void PartsEditorView::removeTerminalPoints(const QStringList &tpIdsToRemove, QDomElement &docElem) {
	QDomElement e = docElem.firstChildElement();
	while(!e.isNull()) {
		bool doRemove = false;
		QString id = e.attribute("id");
		if(tpIdsToRemove.contains(id)) {
			doRemove = true;
		} else if(e.hasChildNodes()) {
			removeTerminalPoints(tpIdsToRemove,e);
		}
		QDomElement e2;
		if(doRemove) {
			e2 = e;
		}
		e = e.nextSiblingElement();
		if(doRemove) {
			e2.removeAttribute("id");
		}
	}
}

void PartsEditorView::addNewTerminalPoints(
			const QList<PartsEditorConnectorsConnectorItem*> &connsWithNewTPs, QDomDocument *svgDom,
			const QSizeF &sceneViewBox, const QRectF &svgViewBox, const QString &connectorsLayerId
) {
	foreach(PartsEditorConnectorsConnectorItem* citem, connsWithNewTPs) {
		QString connId = citem->connector()->connectorSharedID();
		TerminalPointItem *tp = citem->terminalPointItem();
		if (tp == NULL) {
			throw "PartsEditorView::addNewTerminalPoints tp missing";
		}

		if(tp) {
			QRectF tpointRect(tp->mappedPoint(), QPointF(0,0));
			QRectF svgTpRect = mapFromSceneToSvg(tpointRect,sceneViewBox,svgViewBox);

			double halfTPSize = 0.001; // a tiny rectangle
			svgTpRect.setSize(QSizeF(halfTPSize*2,halfTPSize*2));

			addRectToSvg(svgDom,connId+"terminal",svgTpRect, connectorsLayerId);
		} else {
			qWarning() << tr(
				"Parts Editor: couldn't save terminal "
				"point for connector %1 in %2 view")
				.arg(citem->connector()->connectorSharedID())
				.arg(ViewLayer::viewIdentifierNaturalName(m_viewIdentifier));
		}
	}
}

bool PartsEditorView::removeConnectorsIfNeeded(QDomElement &docElem) {
	if(!m_removedConnIds.isEmpty()) {
		//Q_ASSERT(docElem.tagName() == "svg");

		QDomElement e = docElem.firstChildElement();
		while(!e.isNull()) {
			QString id = e.attribute("id");
			if(isSupposedToBeRemoved(id)) {
				e.removeAttribute("id");
			} else if(e.hasChildNodes()) {
				removeConnectorsIfNeeded(e);
			}
			e = e.nextSiblingElement();
		}
		return true;
	}
	return false;
}

QRectF PartsEditorView::mapFromSceneToSvg(const QRectF &itemRect, const QSizeF &sceneViewBox, const QRectF &svgViewBox) {
	double relationW = svgViewBox.width() / sceneViewBox.width();
	double relationH = svgViewBox.height() / sceneViewBox.height();

	double x = itemRect.x() * relationW;
	double y = itemRect.y() * relationH;
	double width = itemRect.width() * relationW;
	double height = itemRect.height() * relationH;

	return QRectF(x,y,width,height);
}

bool PartsEditorView::addRectToSvg(QDomDocument* svgDom, const QString &id, const QRectF &rect, const QString &connectorsLayerId) {
	QDomElement connElem = svgDom->createElement("rect");
	connElem.setAttribute("id",id);
	connElem.setAttribute("x",rect.x());
	connElem.setAttribute("y",rect.y());
	connElem.setAttribute("width",rect.width());
	connElem.setAttribute("height",rect.height());
	connElem.setAttribute("fill","none");

	if(connectorsLayerId.isEmpty()) {
		svgDom->firstChildElement("svg").appendChild(connElem);
		return true;
	} else {
		QDomElement docElem = svgDom->documentElement();
		return addRectToSvgAux(docElem, connectorsLayerId, connElem);
	}
}

bool PartsEditorView::addRectToSvgAux(QDomElement &docElem, const QString &connectorsLayerId, QDomElement &rectElem) {
	QDomElement e = docElem.firstChildElement();
	while(!e.isNull()) {
		QString id = e.attribute("id");
		if(id == connectorsLayerId) {
			e.appendChild(rectElem);
			return true;
		} else if(e.hasChildNodes()) {
			if(addRectToSvgAux(e, connectorsLayerId, rectElem)) {
				return true;
			}
		}
		e = e.nextSiblingElement();
	}
	return false;
}


bool PartsEditorView::isSupposedToBeRemoved(const QString& id) {
	if (id.isEmpty()) return false;

	// TODO: m_removedConnIds should be svg ids and not connectorSharedIDs (from fzp)

	foreach(QString toBeRemoved, m_removedConnIds) {
		if(id.startsWith(toBeRemoved)) {
			QString temp = id;
			temp = temp.remove(0, toBeRemoved.length());
			if (temp.length() == 0) return true;

			// assumes svg id is always prefixDsuffix where D is some string of decimal digits 
			// and prefixD matches toBeRemoved
			if (!temp.at(0).isDigit()) {
				return true;
			}
		}
	}
	return false;
}

PartsEditorConnectorsPaletteItem *PartsEditorView::myItem() {
	return dynamic_cast<PartsEditorConnectorsPaletteItem*>(m_item.data());
}

void PartsEditorView::showTerminalPoints(bool show) {
	m_showingTerminalPoints = show;
	foreach(QGraphicsItem *item, items()) {
		PartsEditorConnectorsConnectorItem *connItem
			= dynamic_cast<PartsEditorConnectorsConnectorItem*>(item);
		if(connItem) {
			connItem->setShowTerminalPoint(show);
		}
	}
	scene()->update();

	/*if(!m_showingTerminalPoints) {
		m_terminalPointsTimer->stop();
	}*/
}

bool PartsEditorView::showingTerminalPoints() {
	return m_showingTerminalPoints;
}

void PartsEditorView::inFileDefinedConnectorChanged(PartsEditorConnectorsConnectorItem *connItem) {
	QString connId = connItem->connectorSharedID();
	m_drawnConns[connId] = connItem;
	if(!m_removedConnIds.contains(connId)) {
		m_removedConnIds << connId;
	}
}


void PartsEditorView::addFixedToBottomRight(QWidget *widget) {
	m_fixedWidgets << widget;
	QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget();
	proxy->setWidget(widget);

	addFixedToBottomRightItem(proxy);
}

bool PartsEditorView::imageLoaded() {
	return m_item != NULL;
}

void PartsEditorView::drawBackground(QPainter *painter, const QRectF &rect) {
	SketchWidget::drawBackground(painter,rect);

	// 10mm spacing grid
	/*const int gridSize = 10*width()/widthMM();

	QRectF itemRect = m_item->mapToScene(m_item->boundingRect()).boundingRect();
	painter->drawRect(itemRect);
	double itemTop = itemRect.top();
	double itemLeft = itemRect.left();

	QVarLengthArray<QLineF, 100> lines;

	for (double x = itemLeft; x < rect.right(); x += gridSize) {
		lines.append(QLineF(x, rect.top(), x, rect.bottom()));
	}
	for (double x = itemLeft-gridSize; x > rect.left(); x -= gridSize) {
		lines.append(QLineF(x, rect.top(), x, rect.bottom()));
	}

	for (double y = itemTop; y < rect.bottom(); y += gridSize) {
		lines.append(QLineF(rect.left(), y, rect.right(), y));
	}
	for (double y = itemTop-gridSize; y > rect.top(); y -= gridSize) {
		lines.append(QLineF(rect.left(), y, rect.right(), y));
	}

	painter->drawLines(lines.data(), lines.size());*/
}

void PartsEditorView::recoverTerminalPointsState() {
	showTerminalPoints(m_showingTerminalPointsBackup);
	m_terminalPointsTimer->stop();
}

bool PartsEditorView::connsPosOrSizeChanged() {
	foreach(QGraphicsItem *item, items()) {
		PartsEditorConnectorsConnectorItem *citem =
			dynamic_cast<PartsEditorConnectorsConnectorItem*>(item);
		if(citem) {
			TerminalPointItem *tp = citem->terminalPointItem();
			if((tp && tp->hasBeenMoved()) || citem->hasBeenMoved() || citem->hasBeenResized()) {
				return true;
			}
		}
	}
	return false;
}

void PartsEditorView::setViewItem(ItemBase * item) {
	m_viewItem = item;
}


void PartsEditorView::checkConnectorLayers(ViewLayer::ViewIdentifier viewIdentifier, const QString & connId, Connector* existingConnector, Connector * newConnector)
{
	if (m_viewIdentifier != viewIdentifier) return;

	Q_UNUSED(connId);
	QList<SvgIdLayer *> newpins = newConnector->connectorShared()->pins().values(viewIdentifier);
	QList<SvgIdLayer *> oldpins = existingConnector->connectorShared()->pins().values(viewIdentifier);

	LayerList layerList;

	QList<SvgIdLayer *> changes;
	foreach (SvgIdLayer * newSvgIdLayer, newpins) {
		bool gotOne = false;
		layerList << newSvgIdLayer->m_svgViewLayerID;

		foreach (SvgIdLayer * oldSvgIdLayer, oldpins) {
			if (newSvgIdLayer->m_svgViewLayerID == oldSvgIdLayer->m_svgViewLayerID) {
				gotOne = true;
				break;
			}
		}
		if (!gotOne) {
			changes << newSvgIdLayer;
		}
	}

	
	foreach (SvgIdLayer * svgIdLayer, changes) {
		SvgIdLayer * cpy = svgIdLayer->copyLayer();
		existingConnector->connectorShared()->insertPin(viewIdentifier, cpy);
	}

	changes.clear();
	foreach (SvgIdLayer * oldSvgIdLayer, oldpins) {
		bool gotOne = false;
		foreach (SvgIdLayer * newSvgIdLayer, newpins) {
			if (newSvgIdLayer->m_svgViewLayerID == oldSvgIdLayer->m_svgViewLayerID) {
				gotOne = true;
				break;
			}
		}
		if (!gotOne) {
			changes << oldSvgIdLayer;
		}
	}

	foreach (SvgIdLayer * svgIdLayer, changes) {
		existingConnector->connectorShared()->removePin(viewIdentifier, svgIdLayer);
	}
}

void PartsEditorView::updatePinsInfo(QList< QPointer<ConnectorShared> > connsShared) {
	if(!m_svgLoaded) return;  // if the user has not changed the svg file, there's nothing to update

	ViewLayer::ViewLayerID layerID = connectorsLayerId();
	QList<ConnectorShared *> notFound;

	foreach(ConnectorShared* cs, connsShared) {
		QString connId = cs->id();
		SvgIdLayer* pinInfo = cs->fullPinInfo(m_viewIdentifier, layerID);
		if (pinInfo == NULL) {
			notFound << cs;
		}
		else if(!m_svgIds[connId].connectorId.isEmpty()) {
			pinInfo->m_svgId = m_svgIds[connId].connectorId;
			// terminal points are already updated (by the function updateTerminalPoints)
				// pinInfo->m_terminalId = m_svgIds[connId].terminalId;
		}
	}

	if (notFound.length() == 0) return;

	// not sure this is the right place to handle the change of connector layers...

	LayerList alts = ViewLayer::findAlternativeLayers(layerID);
	if (alts.length() == 0) return;

	foreach (ViewLayer::ViewLayerID vlid, alts) {
		QList<ConnectorShared*> found;
		foreach(ConnectorShared* cs, notFound) {
			QString connId = cs->id();
			SvgIdLayer* pinInfo = cs->fullPinInfo(m_viewIdentifier, vlid);
			if (pinInfo != NULL && !m_svgIds[connId].connectorId.isEmpty()) {
				pinInfo->m_svgId = m_svgIds[connId].connectorId;
				pinInfo->m_svgViewLayerID = layerID;
				found << cs;
			}
		}

		foreach (ConnectorShared * cs, found) {
			notFound.removeOne(cs);
		}
	}
}

QString PartsEditorView::saveSvg(const QString & svg, const QString & newFilePath) {
    if (!TextUtils::writeUtf8(newFilePath, svg)) {
        throw tr("unable to open temp file %1").arg(newFilePath);
    }
	return newFilePath;
}

void PartsEditorView::clearFixedItems() {
	m_fixedToBottomLeftItems.clear();
	m_fixedToBottomRightItems.clear();
	m_fixedToCenterItems.clear();
	m_fixedToTopLeftItems.clear();
	m_fixedToTopRightItems.clear();
}


void PartsEditorView::ensureFixedItemsPositions() {

	//DebugDialog::debug("ensure fixed items positions");

	ensureFixedToBottomLeftItems();
	ensureFixedToCenterItems();
	ensureFixedToTopLeftItems();
	ensureFixedToTopRightItems();
	ensureFixedToBottomRightItems();

	scene()->update(sceneRect());
}

void PartsEditorView::addFixedToTopLeftItem(QGraphicsItem *item) {
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	if(!scene()->items().contains(item)) {
		scene()->addItem(item);
	}
	m_fixedToTopLeftItems << item;
	ensureFixedToTopLeft(item);
}

void PartsEditorView::addFixedToTopRightItem(QGraphicsItem *item) {
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	if(!scene()->items().contains(item)) {
		scene()->addItem(item);
	}
	m_fixedToTopRightItems << item;
	ensureFixedToTopRight(item);
}

void PartsEditorView::addFixedToBottomLeftItem(QGraphicsItem *item) {
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	if(!scene()->items().contains(item)) {
		scene()->addItem(item);
	}
	m_fixedToBottomLeftItems << item;
	ensureFixedToBottomLeft(item);
}

void PartsEditorView::addFixedToBottomRightItem(QGraphicsItem *item) {
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	if(!scene()->items().contains(item)) {
		scene()->addItem(item);
	}
	m_fixedToBottomRightItems << item;
	ensureFixedToBottomRight(item);
}

void PartsEditorView::addFixedToCenterItem(QGraphicsItem *item) {
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	if(!scene()->items().contains(item)) {
		scene()->addItem(item);
	}
	m_fixedToCenterItems << item;
	ensureFixedToCenter(item);
}

void PartsEditorView::ensureFixedToTopLeftItems() {
	if(isVisible()) {
		QList<QGraphicsItem*> toRemove;

		foreach(QGraphicsItem* item, m_fixedToTopLeftItems) {
			if(scene()->items().contains(item)) {
				ensureFixedToTopLeft(item);
			} else {
				toRemove << item;
			}
		}

		foreach(QGraphicsItem* item, toRemove) {
			m_fixedToTopLeftItems.removeAll(item);
		}
	}
}

void PartsEditorView::ensureFixedToTopLeft(QGraphicsItem* item) {
	item->setPos(mapToScene(0,0));
}

void PartsEditorView::ensureFixedToTopRightItems() {
	if(isVisible()) {
		QList<QGraphicsItem*> toRemove;

		foreach(QGraphicsItem* item, m_fixedToTopRightItems) {
			if(scene()->items().contains(item)) {
				ensureFixedToTopRight(item);
			} else {
				toRemove << item;
			}
		}

		foreach(QGraphicsItem* item, toRemove) {
			m_fixedToTopRightItems.removeAll(item);
		}
	}
}

void PartsEditorView::ensureFixedToTopRight(QGraphicsItem* item) {
	int x = (int) (width()-fixedItemWidth(item));
	int y = 0;

	item->setPos(mapToScene(x,y));
}

void PartsEditorView::ensureFixedToBottomLeftItems() {
	if(isVisible()) {
		QList<QGraphicsItem*> toRemove;

		foreach(QGraphicsItem* item, m_fixedToBottomLeftItems) {
			if(scene()->items().contains(item)) {
				ensureFixedToBottomLeft(item);
			} else {
				toRemove << item;
			}
		}

		foreach(QGraphicsItem* item, toRemove) {
			m_fixedToBottomLeftItems.removeAll(item);
		}
	}
}

void PartsEditorView::ensureFixedToBottomLeft(QGraphicsItem* item) {
	int x = 0;
	int y = (int) (height()-fixedItemHeight(item));

	item->setPos(mapToScene(x,y));
}

void PartsEditorView::ensureFixedToBottomRightItems() {
	if(isVisible()) {
		QList<QGraphicsItem*> toRemove;

		foreach(QGraphicsItem* item, m_fixedToBottomRightItems) {
			if(scene()->items().contains(item)) {
				ensureFixedToBottomRight(item);
			} else {
				toRemove << item;
			}
		}

		foreach(QGraphicsItem* item, toRemove) {
			m_fixedToBottomRightItems.removeAll(item);
		}
	}
}

void PartsEditorView::ensureFixedToBottomRight(QGraphicsItem* item) {
	int x = (int) (width()-fixedItemWidth(item));
	int y = (int) (height()-fixedItemHeight(item));

	item->setPos(mapToScene(x,y));
}

void PartsEditorView::ensureFixedToCenterItems() {
	if(isVisible()) {
		QList<QGraphicsItem*> toRemove;

		foreach(QGraphicsItem* item, m_fixedToCenterItems) {
			if(scene()->items().contains(item)) {
				ensureFixedToCenter(item);
			} else {
				toRemove << item;
			}
		}

		foreach(QGraphicsItem* item, toRemove) {
			m_fixedToCenterItems.removeAll(item);
		}
	}
}

void PartsEditorView::ensureFixedToCenter(QGraphicsItem* item) {
	double x = (width()-fixedItemWidth(item))/2;
	double y = (height()-fixedItemHeight(item))/2;

	QPointF pos = mapToScene(x,y);

	if(pos.x() < scene()->width() && pos.y() < scene()->height()) {
		item->setPos(pos);
	}
}

void PartsEditorView::removeIfFixedPos(QGraphicsItem *item) {
	m_fixedToBottomLeftItems.removeAll(item);
	m_fixedToBottomRightItems.removeAll(item);
	m_fixedToCenterItems.removeAll(item);
	m_fixedToTopLeftItems.removeAll(item);
	m_fixedToTopRightItems.removeAll(item);
}


double PartsEditorView::fixedItemWidth(QGraphicsItem* item) {
	QGraphicsProxyWidget* gWidget = dynamic_cast<QGraphicsProxyWidget*>(item);
	if(gWidget) {
		return gWidget->widget()->width();
	} else {
		return item->boundingRect().width();
	}
}

double PartsEditorView::fixedItemHeight(QGraphicsItem* item) {
	QGraphicsProxyWidget* gWidget = dynamic_cast<QGraphicsProxyWidget*>(item);
	if(gWidget) {
		return gWidget->widget()->height();
	} else {
		return item->boundingRect().height();
	}
}

void PartsEditorView::deleteItem(ItemBase * itemBase, bool deleteModelPart, bool doEmit, bool later)
{
	removeIfFixedPos(itemBase);
	SketchWidget::deleteItem(itemBase, deleteModelPart, doEmit, later);
}

void PartsEditorView::setPaletteModel(PaletteModel * paletteModel)
{
    m_paletteModel = paletteModel;
}
