/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

********************************************************************/

#include <QSvgGenerator>
#include <QColor>
#include <QImageWriter>
#include <QInputDialog>
#include <QApplication>
#include <QMenuBar>
#include <QClipboard>
#include <QDebug>
#include <QSettings>
#include <QDesktopServices>
#include <QMimeData>

#include "mainwindow.h"
#include "../debugdialog.h"
#include "../waitpushundostack.h"
#include "../partseditor/pemainwindow.h"
#include "../help/aboutbox.h"
#include "../autoroute/mazerouter/mazerouter.h"
#include "../autoroute/autorouteprogressdialog.h"
#include "../autoroute/drc.h"
#include "../items/resizableboard.h"
#include "../items/jumperitem.h"
#include "../items/via.h"
#include "../items/note.h"
#include "../items/groundplane.h"
#include "../sketch/breadboardsketchwidget.h"
#include "../sketch/schematicsketchwidget.h"
#include "../sketch/pcbsketchwidget.h"
#include "../partsbinpalette/binmanager/binmanager.h"
#include "../utils/expandinglabel.h"
#include "../infoview/htmlinfoview.h"
#include "../utils/bendpointaction.h"
#include "../sketch/fgraphicsscene.h"
#include "../utils/fmessagebox.h"
#include "../utils/fileprogressdialog.h"
#include "../help/tipsandtricks.h"
#include "../dialogs/setcolordialog.h"
#include "../dialogs/fabuploaddialog.h"
#include "../utils/folderutils.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../items/moduleidnames.h"
#include "../utils/zoomslider.h"
#include "../dock/layerpalette.h"
#include "../program/programwindow.h"
#include "../utils/autoclosemessagebox.h"
#include "../processeventblocker.h"
#include "../sketchtoolbutton.h"
#include "../help/firsttimehelpdialog.h"
#include "../connectors/debugconnectors.h"
#include "mainwindow/fprobeactions.h"

////////////////////////////////////////////////////////

// help struct to create the example menu from a xml file
struct SketchDescriptor {
	SketchDescriptor(const QString &_id, const QString &_name, const QString &_src, QAction * _action) {
		id = _id;
		name = _name;
		src = _src;
		action = _action;
	}

	QString id;
	QString name;
	QString src;
	QAction * action;
};

bool sortSketchDescriptors(SketchDescriptor * s1, SketchDescriptor * s2) {
	return s1->name.toLower() < s2->name.toLower();
}

QDomElement getBestLanguageChild(const QString & localeName, const QDomElement & parent)
{
	QDomElement language = parent.firstChildElement("language");
	QDomElement backupLang;
	while (!language.isNull()) {
		if (language.attribute("country") == "en") backupLang = language;
		if (localeName.endsWith(language.attribute("country"))) {
			return language;
		}
		language = language.nextSiblingElement("language");
	}
	return backupLang;
}

////////////////////////////////////////////////////////

GridSizeThing::GridSizeThing(const QString & vName, const QString & sName, double defaultSize, const QString & gsText)
{
	defaultGridSize = defaultSize;
	viewName = vName;
	shortName = sName;
	gridSizeText = gsText;
}

GridSizeDialog::GridSizeDialog(GridSizeThing * gridSizeThing) : QDialog() {
	m_gridSizeThing = gridSizeThing;
}

GridSizeThing * GridSizeDialog::gridSizeThing() {
	return m_gridSizeThing;
}


/////////////////////////////////////////////////////////

void MainWindow::closeIfEmptySketch(MainWindow* mw) {
	int cascFactorX;
	int cascFactorY;
	// close empty sketch window if user opens from a file
	if (FolderUtils::isEmptyFileName(mw->m_fwFilename, untitledFileName()) && mw->undoStackIsEmpty()) {
		QTimer::singleShot(0, mw, SLOT(close()) );
		cascFactorX = 0;
		cascFactorY = 0;
	} else {
		cascFactorX = CascadeFactorX;
		cascFactorY = CascadeFactorY;
	}
	mw->move(x()+cascFactorX,y()+cascFactorY);
	mw->show();
	mw->activateWindow();
	mw->raise();
}

void MainWindow::mainLoad() {
	QString path;
	// if it's the first time load is called use Documents folder
	if(m_firstOpen) {
		path = defaultSaveFolder();
		m_firstOpen = false;
	}
	else {
		path = "";
	}

	QString fileName = FolderUtils::getOpenFileName(
	                       this,
						   tr("Select a Fritzing file to open"),
	                       path,
						   tr("Fritzing Files (*%1 *%2 *%3 *%4 *%5);;Fritzing (*%1);;Fritzing Shareable (*%2);;Fritzing Part (*%3);;Fritzing Bin (*%4);;Fritzing Shareable Bin (*%5)")
						   .arg(FritzingSketchExtension
						   , FritzingBundleExtension
						   , FritzingBundledPartExtension
						   , FritzingBinExtension
						   , FritzingBundledBinExtension)
#ifndef QT_NO_DEBUG
				// Loading an unbundled part is useful while creating a new part.
				// However, unbundled parts should not be distributed,
				// since this will lead to many errors (missing files, wrong files,
				// unreliable version numbers, different parts with using the same ID,
				// and many more. If you want to share a new part, you can export it
				// with Fritzing once it is finished, or manually bundle it as fzpz .
				// Note that loading unbundled parts is already possible in the release
				// version, too, but only via drag and drop.
							+
						   tr(";;Fritzing Unbundled Part (*%1)")
						   .arg(FritzingPartExtension)
#endif
					   );

	if (fileName.isEmpty()) return;

	if (fileName.endsWith(FritzingBundledPartExtension)) {
		m_binManager->importPartToMineBin(fileName);
		return;
	}

	if (fileName.endsWith(FritzingBinExtension) || fileName.endsWith(FritzingBundledBinExtension)) {
		m_binManager->openBinIn(fileName, false);
		return;
	}

	mainLoadAux(fileName);
}

void MainWindow::mainLoadAux(const QString & fileName)
{
	if (fileName.isNull()) return;

	if (fileName.endsWith(".txt")) {
		QFileInfo info(fileName);
		QFile file(fileName);
		if (file.open(QFile::ReadOnly)) {
			QTextStream stream(&file);
			while (!stream.atEnd()) {
				QString line = stream.readLine().trimmed();
				Q_FOREACH (QString ext, fritzingExtensions()) {
					if (line.endsWith(ext)) {
						QFileInfo lineInfo(line);
						if (lineInfo.exists()) {
							mainLoadAux(line);
						}
						else {
							QFileInfo lineInfo(info.absoluteDir().absoluteFilePath(line));
							if (lineInfo.exists()) {
								mainLoadAux(lineInfo.absoluteFilePath());
							}
						}
						break;
					}
				}
			}
		}
		return;
	}

	if (!fileName.endsWith(FritzingSketchExtension) && !fileName.endsWith(FritzingBundleExtension)) {
		loadWhich(fileName, false, false, true, "");
		return;
	}

	if (alreadyOpen(fileName)) return;

	QFile file(fileName);
	if (!file.exists()) {
		FMessageBox::warning(this, tr("Fritzing"),
		                     tr("Cannot find file %1.")
		                     .arg(fileName));


		return;
	}



	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		FMessageBox::warning(this, tr("Fritzing"),
		                     tr("Cannot read file  1 %1:\n%2.")
							 .arg(fileName, file.errorString()));
		return;
	}

	file.close();

	MainWindow* mw = newMainWindow(m_referenceModel, fileName, true, true, -1);
	mw->loadWhich(fileName, true, true, true, "");
	mw->clearFileProgressDialog();
	closeIfEmptySketch(mw);
}

void MainWindow::revert() {
	QMessageBox::StandardButton answer = QMessageBox::question(
	        this,
	        tr("Revert?"),
	        tr("This operation can not be undone--you will lose all of your changes."
	           "\n\nGo ahead and revert?"),
	        QMessageBox::Yes | QMessageBox::No,
	        QMessageBox::Yes
	                                     );
	// TODO: make button texts translatable
	if (answer != QMessageBox::Yes) {
		return;
	}

	revertAux();
}

MainWindow * MainWindow::revertAux()
{
	MainWindow* mw = newMainWindow( m_referenceModel, fileName(), true, true, this->currentTabIndex());
	mw->setGeometry(this->geometry());

	QFileInfo info(fileName());
	if (info.exists() || !FolderUtils::isEmptyFileName(this->m_fwFilename, untitledFileName())) {
		mw->loadWhich(fileName(), true, true, true, "");
	}
	else {
		mw->addDefaultParts();
		mw->show();
		mw->hideTempPartsBin();
	}

	mw->clearFileProgressDialog();

	// TODO: restore zoom, scroll, etc. for each view
	mw->setCurrentTabIndex(currentTabIndex());

	this->setCloseSilently(true);
	this->close();

	return mw;
}

bool MainWindow::loadWhich(const QString & fileName, bool setAsLastOpened, bool addToRecent, bool checkObsolete, const QString & displayName)
{
	if (!QFileInfo(fileName).exists()) {
		FMessageBox::warning(nullptr, tr("Fritzing"), tr("File '%1' not found").arg(fileName));
		return false;
	}

	bool result = false;
	if (fileName.endsWith(FritzingSketchExtension)) {
		QFileInfo info(fileName);

		QString bundledFileName;
		mainLoad(fileName, displayName, checkObsolete);
		result = true;

		QFile file(fileName);
		QDir dest(m_fzzFolder);
		FolderUtils::slamCopy(file, dest.absoluteFilePath(info.fileName()));			// copy the .fz file directly
		setCurrentFile(fileName, false, false);
	}
	else if(fileName.endsWith(FritzingBundleExtension)) {
		QString error = loadBundledSketch(fileName, addToRecent, setAsLastOpened, checkObsolete);
		result = error.isEmpty();
	}
	else if (
	    fileName.endsWith(FritzingBinExtension)
	    || fileName.endsWith(FritzingBundledBinExtension)
	) {
		m_binManager->load(fileName);
		result = true;
	}
	else if (fileName.endsWith(FritzingPartExtension)) {
		loadPart(fileName, true);
		result = true;
	}
	else if (fileName.endsWith(FritzingBundledPartExtension)) {
		loadBundledPart(fileName, true);
		result = true;
	}

	if (result) {
		this->show();
	}

	return result;
}

bool MainWindow::mainLoad(const QString & fileName, const QString & displayName, bool checkObsolete) {

	if (m_fileProgressDialog) {
		m_fileProgressDialog->setMaximum(200);
		m_fileProgressDialog->setValue(102);
	}
	this->show();
	ProcessEventBlocker::processEvents();

	QString displayName2 = displayName;
	if (displayName.isEmpty()) {
		QFileInfo fileInfo(fileName);
		displayName2 = fileInfo.fileName();
	}

	if (m_fileProgressDialog) {
		m_fileProgressDialog->setMessage(tr("loading %1 (model)").arg(displayName2));
		m_fileProgressDialog->setValue(110);
	}
	ProcessEventBlocker::processEvents();


	QList<ModelPart *> modelParts;

	bool doMigratePartLabelOffset = false;
	QMetaObject::Connection migratePartLabelOffsetConnection = connect(
		m_sketchModel, &ModelBase::migratePartLabelOffset, this, [&doMigratePartLabelOffset](const QString &fritzingVersion){
		DebugDialog::debug(QString("Migrate part labels for from %1 project to Fritzing 1.0.0").arg(fritzingVersion));
		doMigratePartLabelOffset = true;
	});

	connect(m_sketchModel, SIGNAL(loadedViews(ModelBase *, QDomElement &)),
	        this, SLOT(loadedViewsSlot(ModelBase *, QDomElement &)), Qt::DirectConnection);
	connect(m_sketchModel, SIGNAL(loadedRoot(const QString &, ModelBase *, QDomElement &)),
	        this, SLOT(loadedRootSlot(const QString &, ModelBase *, QDomElement &)), Qt::DirectConnection);
	connect(m_sketchModel, SIGNAL(obsoleteSMDOrientationSignal()),
	        this, SLOT(obsoleteSMDOrientationSlot()), Qt::DirectConnection);
	connect(m_sketchModel, SIGNAL(oldSchematicsSignal(const QString &, bool &)),
	        this, SLOT(oldSchematicsSlot(const QString &, bool &)), Qt::DirectConnection);	
	connect(m_sketchModel, &SketchModel::loadedProjectProperties,
			this, &MainWindow::loadedProjectPropertiesSlot, Qt::DirectConnection);

	m_obsoleteSMDOrientation = false;

	bool result = m_sketchModel->loadFromFile(fileName, m_referenceModel, modelParts, true);

	//DebugDialog::debug("core loaded");
	disconnect(m_sketchModel, &SketchModel::loadedProjectProperties,
			   this, &MainWindow::loadedProjectPropertiesSlot);
	disconnect(m_sketchModel, SIGNAL(loadedViews(ModelBase *, QDomElement &)),
	           this, SLOT(loadedViewsSlot(ModelBase *, QDomElement &)));
	disconnect(m_sketchModel, SIGNAL(loadedRoot(const QString &, ModelBase *, QDomElement &)),
	           this, SLOT(loadedRootSlot(const QString &, ModelBase *, QDomElement &)));
	disconnect(m_sketchModel, SIGNAL(obsoleteSMDOrientationSignal()),
	           this, SLOT(obsoleteSMDOrientationSlot()));

	ProcessEventBlocker::processEvents();
	if (m_fileProgressDialog) {
		m_fileProgressDialog->setValue(155);
		m_fileProgressDialog->setMessage(tr("loading %1 (breadboard)").arg(displayName2));
	}

	QList<long> newIDs;
	m_breadboardGraphicsView->loadFromModelParts(modelParts, BaseCommand::SingleView, nullptr, false, nullptr, false, newIDs);

	ProcessEventBlocker::processEvents();
	if (m_fileProgressDialog) {
		m_fileProgressDialog->setValue(170);
		m_fileProgressDialog->setMessage(tr("loading %1 (pcb)").arg(displayName2));
	}

	newIDs.clear();
	m_pcbGraphicsView->loadFromModelParts(modelParts, BaseCommand::SingleView, nullptr, false, nullptr, false, newIDs);


	ProcessEventBlocker::processEvents();
	if (m_fileProgressDialog) {
		m_fileProgressDialog->setValue(185);
		m_fileProgressDialog->setMessage(tr("loading %1 (schematic)").arg(displayName2));
	}

	newIDs.clear();
	m_schematicGraphicsView->setConvertSchematic(m_convertedSchematic);
	m_schematicGraphicsView->setOldSchematic(this->m_useOldSchematic);
	m_schematicGraphicsView->loadFromModelParts(modelParts, BaseCommand::SingleView, nullptr, false, nullptr, false, newIDs);
	m_schematicGraphicsView->setConvertSchematic(false);

	if (m_sketchModel->checkForReversedWires()) {
		m_pcbGraphicsView->checkForReversedWires();
		m_schematicGraphicsView->checkForReversedWires();
		m_breadboardGraphicsView->checkForReversedWires();
	}

	ProcessEventBlocker::processEvents();
	if (m_fileProgressDialog) {
		m_fileProgressDialog->setValue(198);
	}

	if (m_obsoleteSMDOrientation) {
		QSet<ItemBase *> toConvert;
		Q_FOREACH (QGraphicsItem * item, m_pcbGraphicsView->items()) {
			auto * itemBase = dynamic_cast<ItemBase *>(item);
			if (itemBase == nullptr) continue;

			itemBase = itemBase->layerKinChief();
			if (itemBase->modelPart()->flippedSMD() && itemBase->viewLayerPlacement() == ViewLayer::NewBottom) {
				toConvert.insert(itemBase);
			}
		}

		QList<ConnectorItem *> already;
		Q_FOREACH (ItemBase * itemBase, toConvert) {
			auto * paletteItem = qobject_cast<PaletteItem *>(itemBase);
			if (paletteItem == nullptr) continue;          // shouldn't happen

			paletteItem->rotateItem(180, true);
		}
	}

	if (m_programView) {
		QFileInfo fileInfo(m_fwFilename);
		m_programView->linkFiles(m_linkedProgramFiles, fileInfo.absoluteDir().absolutePath());
	}

	if (doMigratePartLabelOffset) {
		migratePartLabelOffset(modelParts);
	}
	disconnect(migratePartLabelOffsetConnection);

	if (!m_useOldSchematic && checkObsolete) {
		if (m_pcbGraphicsView) {
			QList<ItemBase *> items = m_pcbGraphicsView->selectAllObsolete();
			if (items.count() > 0) {
				checkSwapObsolete(items, true);
			}
		}
	}

	initZoom();
#ifndef QT_NO_DEBUG
	m_debugConnectors->onChangeConnection();
#endif
	return result;
}

void MainWindow::copy() {
	if (m_currentGraphicsView == nullptr) return;
	m_currentGraphicsView->copy();
}

void MainWindow::cut() {
	if (m_currentGraphicsView == nullptr) return;
	m_currentGraphicsView->cut();
}

void MainWindow::pasteInPlace() {
	pasteAux(true);
}

void MainWindow::paste() {
	pasteAux(false);
}

void MainWindow::pasteAux(bool pasteInPlace)
{
	if (m_currentGraphicsView == nullptr) return;

	QClipboard *clipboard = QApplication::clipboard();
	if (clipboard == nullptr) {
		// shouldn't happen
		return;
	}

	const QMimeData* mimeData = clipboard->mimeData(QClipboard::Clipboard);
	if (mimeData == nullptr) return;

	if (!mimeData->hasFormat("application/x-dnditemsdata")) return;

	QByteArray itemData = mimeData->data("application/x-dnditemsdata");
	QList<ModelPart *> modelParts;
	QHash<QString, QRectF> boundingRects;
	if (m_sketchModel->paste(m_referenceModel, itemData, modelParts, boundingRects, false)) {
		auto * parentCommand = new QUndoCommand("Paste"); // if you translate "Paste", you must also do so for the check in sketchwidget.cpp.

		QList<SketchWidget *> sketchWidgets;
		sketchWidgets << m_breadboardGraphicsView << m_schematicGraphicsView << m_pcbGraphicsView;
		sketchWidgets.removeOne(m_currentGraphicsView);
		sketchWidgets.prepend(m_currentGraphicsView);

		QList<long> newIDs;
		Q_FOREACH (SketchWidget * sketchWidget, sketchWidgets) {
			newIDs.clear();
			QRectF r;
			QRectF boundingRect = boundingRects.value(sketchWidget->viewName(), r);
			sketchWidget->loadFromModelParts(modelParts, BaseCommand::SingleView, parentCommand, true, pasteInPlace ? &r : &boundingRect, false, newIDs, pasteInPlace);
			Q_FOREACH (long id, newIDs) {
				auto * checkPartLabelLayerVisibilityCommand = new CheckPartLabelLayerVisibilityCommand(sketchWidget, id, parentCommand);
				checkPartLabelLayerVisibilityCommand->setRedoOnly();
			}
		}

		Q_FOREACH (long id, newIDs) {
			new IncLabelTextCommand(m_breadboardGraphicsView, id, parentCommand);
		}

		m_breadboardGraphicsView->setPasting(true);
		m_pcbGraphicsView->setPasting(true);
		m_schematicGraphicsView->setPasting(true);
		m_undoStack->push(parentCommand);
		m_breadboardGraphicsView->setPasting(false);
		m_pcbGraphicsView->setPasting(false);
		m_schematicGraphicsView->setPasting(false);
	}

	m_currentGraphicsView->updateInfoView();
}

void MainWindow::duplicate() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->copy();
	paste();

	//m_currentGraphicsView->duplicate();
}

void MainWindow::doDelete() {
	//DebugDialog::debug(QString("invoking do delete") );

	if (m_currentGraphicsView) {
		m_currentGraphicsView->deleteSelected(retrieveWire(), false);
	}
}

void MainWindow::doDeleteMinus() {
	if (m_currentGraphicsView) {
		m_currentGraphicsView->deleteSelected(retrieveWire(), true);
	}
}

void MainWindow::selectAll() {
	if (m_currentGraphicsView) {
		m_currentGraphicsView->selectDeselectAllCommand(true);
	}
}

void MainWindow::deselect() {
	if (m_currentGraphicsView) {
		m_currentGraphicsView->selectDeselectAllCommand(false);
	}
}

void MainWindow::about()
{
	AboutBox::showAbout();
}

void MainWindow::tipsAndTricks()
{
	TipsAndTricks::showTipsAndTricks();
}

void MainWindow::firstTimeHelp()
{
	if (m_currentGraphicsView == nullptr) return;

	FirstTimeHelpDialog::setViewID(m_currentGraphicsView->viewID());
	FirstTimeHelpDialog::showFirstTimeHelp();
}

void MainWindow::createActions()
{
	createRaiseWindowActions();

	createFileMenuActions();
	createEditMenuActions();
	createPartMenuActions();
	createViewMenuActions(true);
	createWindowMenuActions();
	createHelpMenuActions();
	createTraceMenuActions();
}

void MainWindow::createRaiseWindowActions() {
	m_raiseWindowAct = new QAction(m_fwFilename, this);
	m_raiseWindowAct->setCheckable(true);
	connect( m_raiseWindowAct, SIGNAL(triggered()), this, SLOT(raiseAndActivate()));
	updateRaiseWindowAction();
}

void MainWindow::createFileMenuActions() {
	m_newAct = new QAction(tr("New"), this);
	m_newAct->setShortcut(tr("Ctrl+N"));
	m_newAct->setStatusTip(tr("Create a new sketch"));
	connect(m_newAct, SIGNAL(triggered()), this, SLOT(createNewSketch()));

	m_openAct = new QAction(tr("&Open..."), this);
	m_openAct->setShortcut(tr("Ctrl+O"));
	m_openAct->setStatusTip(tr("Open a Fritzing sketch (.fzz, .fz), or load a Fritzing part (.fzpz), or a Fritzing parts bin (.fzb, .fzbz)"));
	connect(m_openAct, SIGNAL(triggered()), this, SLOT(mainLoad()));

	m_revertAct = new QAction(tr("Revert"), this);
	m_revertAct->setStatusTip(tr("Reload the sketch"));
	connect(m_revertAct, SIGNAL(triggered()), this, SLOT(revert()));

	createOpenRecentMenu();
	createOpenExampleMenu();
	createCloseAction();
	createExportActions();
	createOrderFabAct();

	QString name;
	QString path;
	QStringList args;
	if (externalProcess(name, path, args)) {
		m_launchExternalProcessAct = new QAction(name, this);
		m_launchExternalProcessAct->setStatusTip(tr("Shell launch %1").arg(path));
		connect(m_launchExternalProcessAct, SIGNAL(triggered()), this, SLOT(launchExternalProcess()));
	}

#ifndef QT_NO_DEBUG
	m_exceptionAct = new QAction(tr("throw test exception"), this);
	m_exceptionAct->setStatusTip(tr("throw a fake exception to see what happens"));
	connect(m_exceptionAct, SIGNAL(triggered()), this, SLOT(throwFakeException()));
#endif

	m_quitAct = new QAction(tr("&Quit"), this);
	m_quitAct->setShortcut(tr("Ctrl+Q"));
	m_quitAct->setStatusTip(tr("Quit the application"));
	connect(m_quitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows2()));
	m_quitAct->setMenuRole(QAction::QuitRole);

}

void MainWindow::createOpenExampleMenu() {
	m_openExampleMenu = new QMenu(tr("&Open Example"), this);
	QString folderPath = FolderUtils::getApplicationSubFolderPath("sketches")+"/";
	populateMenuFromXMLFile(m_openExampleMenu, m_openExampleActions, folderPath, "index.xml");
}

void MainWindow::populateMenuFromXMLFile(QMenu *parentMenu, QStringList &actionsTracker, const QString &folderPath, const QString &indexFileName)
{
	QDomDocument dom;
	QFile file(folderPath+indexFileName);
	if (!file.open(QIODevice::ReadOnly)) {
		DebugDialog::debug(QString("Unable to open :%1").arg(folderPath+indexFileName));
	}
	dom.setContent(&file);

	QDomElement domElem = dom.documentElement();
	QDomElement indexDomElem = domElem.firstChildElement("sketches");
	QDomElement taxonomyDomElem = domElem.firstChildElement("categories");

	QLocale locale;
	QString localeName = locale.name().toLower();     // get default translation, a string of the form "language_country" where country is a two-letter code.


	QHash<QString, struct SketchDescriptor *> index = indexAvailableElements(indexDomElem, folderPath, actionsTracker, localeName);
	QList<SketchDescriptor *> sketchDescriptors(index.values());
	std::sort(sketchDescriptors.begin(), sketchDescriptors.end(), sortSketchDescriptors);

	if (sketchDescriptors.size() > 0) {
		// set up the "all" category
		QDomElement all = dom.createElement("category");
		taxonomyDomElem.appendChild(all);
		QDomElement language = dom.createElement("language");
		language.setAttribute("name", tr("All"));
		all.appendChild(language);
		Q_FOREACH (SketchDescriptor * sketchDescriptor, sketchDescriptors) {
			QDomElement sketch = dom.createElement("sketch");
			sketch.setAttribute("id", sketchDescriptor->id);
			all.appendChild(sketch);
		}
	}
	populateMenuWithIndex(index, parentMenu, taxonomyDomElem, localeName);
	Q_FOREACH (SketchDescriptor * sketchDescriptor, index.values()) {
		delete sketchDescriptor;
	}
}

QHash<QString, struct SketchDescriptor *> MainWindow::indexAvailableElements(QDomElement &domElem, const QString &srcPrefix, QStringList & actionsTracker, const QString & localeName) {
	QHash<QString, struct SketchDescriptor *> retval;
	QDomElement sketch = domElem.firstChildElement("sketch");

	// TODO: eventually need to deal with language/country differences like pt_br vs. pt_pt

	while(!sketch.isNull()) {
		const QString id = sketch.attribute("id");
		QDomElement bestLang = getBestLanguageChild(localeName, sketch);
		QString name = bestLang.attribute("name");
		QString srcAux = bestLang.attribute("src");
		// if it's an absolute path, don't prefix it
		const QString src = QFileInfo(srcAux).exists()? srcAux: srcPrefix+srcAux;
		if(QFileInfo(src).exists()) {
			actionsTracker << name;
			auto * action = new QAction(name, this);
			action->setData(src);
			connect(action,SIGNAL(triggered()),this,SLOT(openRecentOrExampleFile()));
			retval[id] = new SketchDescriptor(id,name,src, action);
		}
		sketch = sketch.nextSiblingElement("sketch");
	}
	return retval;
}

void MainWindow::populateMenuWithIndex(const QHash<QString, struct SketchDescriptor *>  &index, QMenu * parentMenu, QDomElement &domElem, const QString & localeName) {
	// note: the <sketch> element here is not the same as the <sketch> element in indexAvailableElements()
	QDomElement e = domElem.firstChildElement();
	while(!e.isNull()) {
		if (e.nodeName() == "sketch") {
			QString id = e.attribute("id");
			if (!id.isEmpty()) {
				if(index[id]) {
					SketchDescriptor * sketchDescriptor = index[id];
					parentMenu->addAction(sketchDescriptor->action);
				}
				else
				{
					qWarning() << QString("MainWindow::populateMenuWithIndex: couldn't load example with id='%1'").arg(id);
				}
			}
		}
		else if (e.nodeName() == "category") {
			QDomElement bestLang = getBestLanguageChild(localeName, e);
			QString name = bestLang.attribute("name");
			auto * currMenu = new QMenu(name, parentMenu);
			parentMenu->addMenu(currMenu);
			populateMenuWithIndex(index, currMenu, e, localeName);
		}
		else if (e.nodeName() == "separator") {
			parentMenu->addSeparator();
		}
		else if (e.nodeName() == "url") {
			QDomElement bestLang = getBestLanguageChild(localeName, e);
			auto * action = new QAction(bestLang.attribute("name"), this);
			action->setData(bestLang.attribute("href"));
			connect(action, SIGNAL(triggered()), this, SLOT(openURL()));
			parentMenu->addAction(action);
		}
		e = e.nextSiblingElement();
	}
}

void MainWindow::populateMenuFromFolderContent(QMenu * parentMenu, const QString &path) {
	QDir *currDir = new QDir(path);
	QStringList content = currDir->entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
	if(content.size() > 0) {
		for(int i=0; i < content.size(); i++) {
			QString currFile = content.at(i);
			QString currFilePath = currDir->absoluteFilePath(currFile);
			if(QFileInfo(currFilePath).isDir()) {
				auto * currMenu = new QMenu(currFile, parentMenu);
				parentMenu->addMenu(currMenu);
				populateMenuFromFolderContent(currMenu, currFilePath);
			} else {
				QString actionText = QFileInfo(currFilePath).completeBaseName();
				m_openExampleActions << actionText;
				auto * currAction = new QAction(actionText, this);
				currAction->setData(currFilePath);
				connect(currAction,SIGNAL(triggered()),this,SLOT(openRecentOrExampleFile()));
				parentMenu->addAction(currAction);
			}
		}
	} else {
		parentMenu->setEnabled(false);
	}
	delete currDir;
}

void MainWindow::createOpenRecentMenu() {
	m_openRecentFileMenu = new QMenu(tr("&Open Recent Files"), this);

	for (auto & m_openRecentFileAct : m_openRecentFileActs) {
		m_openRecentFileAct = new QAction(this);
		m_openRecentFileAct->setVisible(false);
		connect(m_openRecentFileAct, SIGNAL(triggered()),this, SLOT(openRecentOrExampleFile()));
	}


	for (auto & m_openRecentFileAct : m_openRecentFileActs) {
		m_openRecentFileMenu->addAction(m_openRecentFileAct);
	}
	updateRecentFileActions();
}

void MainWindow::updateFileMenu() {
	m_printAct->setEnabled(m_currentGraphicsView || m_currentWidget->contentView() == m_programView);

	updateRecentFileActions();
	m_orderFabAct->setEnabled(true);

	m_revertAct->setEnabled(m_undoStack->canUndo());


}

void MainWindow::updateRecentFileActions() {
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	int ix = 0;
	for (int i = 0; i < files.size(); ++i) {
		QFileInfo finfo(files[i]);
		if (!finfo.exists()) continue;

		QString text = tr("&%1 %2").arg(ix + 1).arg(finfo.fileName());
		m_openRecentFileActs[ix]->setText(text);
		m_openRecentFileActs[ix]->setData(files[i]);
		m_openRecentFileActs[ix]->setVisible(true);
		m_openRecentFileActs[ix]->setStatusTip(files[i]);

		if (++ix >= (int) MaxRecentFiles) {
			break;
		}
	}

	for (int j = ix; j < MaxRecentFiles; ++j) {
		m_openRecentFileActs[j]->setVisible(false);
	}

	m_openRecentFileMenu->setEnabled(ix > 0);
}

void MainWindow::createEditMenuActions() {
	m_undoAct = m_undoGroup->createUndoAction(this, tr("Undo"));
	m_undoAct->setShortcuts(QKeySequence::Undo);
	m_undoAct->setText(tr("Undo"));

	m_redoAct = m_undoGroup->createRedoAction(this, tr("Redo"));
	m_redoAct->setShortcuts(QKeySequence::Redo);
	m_redoAct->setText(tr("Redo"));

	m_cutAct = new QAction(tr("&Cut"), this);
	m_cutAct->setShortcut(QKeySequence::Cut);
	m_cutAct->setStatusTip(tr("Cut selection"));
	connect(m_cutAct, SIGNAL(triggered()), this, SLOT(cut()));

	m_copyAct = new QAction(tr("&Copy"), this);
	m_copyAct->setShortcut(QKeySequence::Copy);
	m_copyAct->setStatusTip(tr("Copy selection"));
	connect(m_copyAct, SIGNAL(triggered()), this, SLOT(copy()));

	m_pasteAct = new QAction(tr("&Paste"), this);
	m_pasteAct->setShortcut(QKeySequence::Paste);
	m_pasteAct->setStatusTip(tr("Paste clipboard contents"));
	connect(m_pasteAct, SIGNAL(triggered()), this, SLOT(paste()));

	m_pasteInPlaceAct = new QAction(tr("Paste in Place"), this);
	m_pasteInPlaceAct->setShortcut(tr("Ctrl+Shift+V"));
	m_pasteInPlaceAct->setStatusTip(tr("Paste clipboard contents in place"));
	connect(m_pasteInPlaceAct, SIGNAL(triggered()), this, SLOT(pasteInPlace()));

	m_duplicateAct = new QAction(tr("&Duplicate"), this);
	m_duplicateAct->setShortcut(tr("Ctrl+D"));
	m_duplicateAct->setStatusTip(tr("Duplicate selection"));
	connect(m_duplicateAct, SIGNAL(triggered()), this, SLOT(duplicate()));

	m_deleteAct = new QAction(tr("&Delete"), this);
	m_deleteAct->setStatusTip(tr("Delete selection"));
	connect(m_deleteAct, SIGNAL(triggered()), this, SLOT(doDelete()));
#ifdef Q_OS_MACOS
	m_deleteAct->setShortcut(Qt::Key_Backspace);
#else
	m_deleteAct->setShortcut(QKeySequence::Delete);
#endif

	m_deleteMinusAct = new QAction(tr("Delete Minus"), this);
	m_deleteMinusAct->setStatusTip(tr("Delete selection without attached wires"));
	connect(m_deleteMinusAct, SIGNAL(triggered()), this, SLOT(doDeleteMinus()));
#ifdef Q_OS_MACOS
	m_deleteMinusAct->setShortcut(Qt::Key_Backspace | Qt::AltModifier);
#endif

	m_deleteWireAct = new WireAction(m_deleteAct);
	m_deleteWireAct->setText(tr("&Delete Wire"));
	connect(m_deleteWireAct, SIGNAL(triggered()), this, SLOT(doDelete()));

	m_deleteWireMinusAct = new WireAction(m_deleteMinusAct);
	m_deleteWireMinusAct->setText(tr("Delete Wire up to bendpoints"));
	connect(m_deleteWireMinusAct, SIGNAL(triggered()), this, SLOT(doDeleteMinus()));

	m_selectAllAct = new QAction(tr("&Select All"), this);
	m_selectAllAct->setShortcut(QKeySequence::SelectAll);
	m_selectAllAct->setStatusTip(tr("Select all elements"));
	connect(m_selectAllAct, SIGNAL(triggered()), this, SLOT(selectAll()));

	m_deselectAct = new QAction(tr("&Deselect"), this);
	m_deselectAct->setStatusTip(tr("Deselect"));
	connect(m_deselectAct, SIGNAL(triggered()), this, SLOT(deselect()));

	m_addNoteAct = new QAction(tr("Add Note"), this);
	m_addNoteAct->setStatusTip(tr("Add a note"));
	connect(m_addNoteAct, SIGNAL(triggered()), this, SLOT(addNote()));

	m_preferencesAct = new QAction(tr("&Preferences..."), this);
	m_preferencesAct->setStatusTip(tr("Edit the application's preferences"));
	m_preferencesAct->setMenuRole(QAction::PreferencesRole);						// make sure this is added to the correct menu on mac
	connect(m_preferencesAct, SIGNAL(triggered()), QApplication::instance(), SLOT(preferences()));
}

void MainWindow::createPartMenuActions() {
	m_openInPartsEditorNewAct = new QAction(tr("Edit (new parts editor)"), this);
	m_openInPartsEditorNewAct->setStatusTip(tr("Open the new parts editor on an existing part"));
	connect(m_openInPartsEditorNewAct, SIGNAL(triggered()), this, SLOT(openInPartsEditorNew()));

	m_disconnectAllAct = new QAction(tr("Disconnect All Wires"), this);
	m_disconnectAllAct->setStatusTip(tr("Disconnect all wires connected to this connector"));
	connect(m_disconnectAllAct, SIGNAL(triggered()), this, SLOT(disconnectAll()));

#ifndef QT_NO_DEBUG
	m_infoViewOnHoverAction = new QAction(tr("Update InfoView on hover"), this);
	m_infoViewOnHoverAction->setCheckable(true);
	bool infoViewOnHover = true;
	m_infoViewOnHoverAction->setChecked(infoViewOnHover);
	setInfoViewOnHover(infoViewOnHover);
	connect(m_infoViewOnHoverAction, SIGNAL(toggled(bool)), this, SLOT(setInfoViewOnHover(bool)));

	m_exportNormalizedSvgAction = new QAction(tr("Export Normalized SVG"), this);
	m_exportNormalizedSvgAction->setStatusTip(tr("Export 1000 dpi SVG of this part in this view"));
	connect(m_exportNormalizedSvgAction, SIGNAL(triggered()), this, SLOT(exportNormalizedSVG()));

	m_exportNormalizedFlattenedSvgAction = new QAction(tr("Export Normalized Flattened SVG"), this);
	m_exportNormalizedFlattenedSvgAction->setStatusTip(tr("Export 1000 dpi Flattened SVG of this part in this view"));
	connect(m_exportNormalizedFlattenedSvgAction, SIGNAL(triggered()), this, SLOT(exportNormalizedFlattenedSVG()));

	m_dumpAllPartsAction = new QAction(tr("Dump all parts"), this);
	m_dumpAllPartsAction->setStatusTip(tr("Debug dump all parts in this view"));
	connect(m_dumpAllPartsAction, SIGNAL(triggered()), this, SLOT(dumpAllParts()));
#endif
	m_testConnectorsAction = new QAction(tr("Test Connectors"), this);
	m_testConnectorsAction->setStatusTip(tr("Connect all connectors to a single test part"));
	connect(m_testConnectorsAction, &QAction::triggered, this, &MainWindow::testConnectors);

	m_rotate45cwAct = new QAction(tr("Rotate 45° Clockwise"), this);
	m_rotate45cwAct->setStatusTip(tr("Rotate current selection 45 degrees clockwise"));
	connect(m_rotate45cwAct, SIGNAL(triggered()), this, SLOT(rotate45cw()));

	m_rotate90cwAct = new QAction(tr("Rotate 90° Clockwise"), this);
	m_rotate90cwAct->setStatusTip(tr("Rotate the selected parts by 90 degrees clockwise"));
	m_rotate90cwAct->setShortcut(QKeySequence("]"));
	connect(m_rotate90cwAct, SIGNAL(triggered()), this, SLOT(rotate90cw()));

	m_rotate180Act = new QAction(tr("Rotate 180°"), this);
	m_rotate180Act->setStatusTip(tr("Rotate the selected parts by 180 degrees"));
	connect(m_rotate180Act, SIGNAL(triggered()), this, SLOT(rotate180()));

	m_rotate90ccwAct = new QAction(tr("Rotate 90° Counter Clockwise"), this);
	m_rotate90ccwAct->setStatusTip(tr("Rotate current selection 90 degrees counter clockwise"));
	m_rotate90ccwAct->setShortcut(QKeySequence("["));
	connect(m_rotate90ccwAct, SIGNAL(triggered()), this, SLOT(rotate90ccw()));

	m_rotate45ccwAct = new QAction(tr("Rotate 45° Counter Clockwise"), this);
	m_rotate45ccwAct->setStatusTip(tr("Rotate current selection 45 degrees counter clockwise"));
	connect(m_rotate45ccwAct, SIGNAL(triggered()), this, SLOT(rotate45ccw()));

	m_flipHorizontalAct = new QAction(tr("&Flip Horizontal"), this);
	m_flipHorizontalAct->setStatusTip(tr("Flip current selection horizontally"));
	m_flipHorizontalAct->setShortcut(QKeySequence("|"));
	connect(m_flipHorizontalAct, SIGNAL(triggered()), this, SLOT(flipHorizontal()));

	m_flipVerticalAct = new QAction(tr("&Flip Vertical"), this);
	m_flipVerticalAct->setStatusTip(tr("Flip current selection vertically"));
	connect(m_flipVerticalAct, SIGNAL(triggered()), this, SLOT(flipVertical()));

	m_bringToFrontAct = new QAction(tr("Bring to Front"), this);
	m_bringToFrontAct->setShortcut(tr("Shift+Ctrl+]"));
	m_bringToFrontAct->setStatusTip(tr("Bring selected object(s) to front of their layer"));
	connect(m_bringToFrontAct, SIGNAL(triggered()), this, SLOT(bringToFront()));
	m_bringToFrontWireAct = new WireAction(m_bringToFrontAct);
	connect(m_bringToFrontWireAct, SIGNAL(triggered()), this, SLOT(bringToFront()));

	m_bringForwardAct = new QAction(tr("Bring Forward"), this);
	m_bringForwardAct->setShortcut(tr("Ctrl+]"));
	m_bringForwardAct->setStatusTip(tr("Bring selected object(s) forward in their layer"));
	connect(m_bringForwardAct, SIGNAL(triggered()), this, SLOT(bringForward()));
	m_bringForwardWireAct = new WireAction(m_bringForwardAct);
	connect(m_bringForwardWireAct, SIGNAL(triggered()), this, SLOT(bringForward()));

	m_sendBackwardAct = new QAction(tr("Send Backward"), this);
	m_sendBackwardAct->setShortcut(tr("Ctrl+["));
	m_sendBackwardAct->setStatusTip(tr("Send selected object(s) back in their layer"));
	connect(m_sendBackwardAct, SIGNAL(triggered()), this, SLOT(sendBackward()));
	m_sendBackwardWireAct = new WireAction(m_sendBackwardAct);
	connect(m_sendBackwardWireAct, SIGNAL(triggered()), this, SLOT(sendBackward()));

	m_sendToBackAct = new QAction(tr("Send to Back"), this);
	m_sendToBackAct->setShortcut(tr("Shift+Ctrl+["));
	m_sendToBackAct->setStatusTip(tr("Send selected object(s) to the back of their layer"));
	connect(m_sendToBackAct, SIGNAL(triggered()), this, SLOT(sendToBack()));
	m_sendToBackWireAct = new WireAction(m_sendToBackAct);
	connect(m_sendToBackWireAct, SIGNAL(triggered()), this, SLOT(sendToBack()));

	m_alignLeftAct = new QAction(tr("Align Left"), this);
	m_alignLeftAct->setStatusTip(tr("Align selected items at the left"));
	connect(m_alignLeftAct, SIGNAL(triggered()), this, SLOT(alignLeft()));

	m_alignHorizontalCenterAct = new QAction(tr("Align Horizontal Center"), this);
	m_alignHorizontalCenterAct->setStatusTip(tr("Align selected items at the horizontal center"));
	connect(m_alignHorizontalCenterAct, SIGNAL(triggered()), this, SLOT(alignHorizontalCenter()));

	m_alignRightAct = new QAction(tr("Align Right"), this);
	m_alignRightAct->setStatusTip(tr("Align selected items at the right"));
	connect(m_alignRightAct, SIGNAL(triggered()), this, SLOT(alignRight()));

	m_alignTopAct = new QAction(tr("Align Top"), this);
	m_alignTopAct->setStatusTip(tr("Align selected items at the top"));
	connect(m_alignTopAct, SIGNAL(triggered()), this, SLOT(alignTop()));

	m_alignVerticalCenterAct = new QAction(tr("Align Vertical Center"), this);
	m_alignVerticalCenterAct->setStatusTip(tr("Align selected items at the vertical center"));
	connect(m_alignVerticalCenterAct, SIGNAL(triggered()), this, SLOT(alignVerticalCenter()));

	m_alignBottomAct = new QAction(tr("Align Bottom"), this);
	m_alignBottomAct->setStatusTip(tr("Align selected items at the bottom"));
	connect(m_alignBottomAct, SIGNAL(triggered()), this, SLOT(alignBottom()));

	m_moveLockAct = new QAction(tr("Lock Part"), this);
	m_moveLockAct->setStatusTip(tr("Prevent a part from being moved"));
	m_moveLockAct->setCheckable(true);
	connect(m_moveLockAct, SIGNAL(triggered()), this, SLOT(moveLock()));

	m_stickyAct = new QAction(tr("Sticky"), this);
	m_stickyAct->setStatusTip(tr("If a \"sticky\" part is moved, parts on top of it are also moved"));
	m_stickyAct->setCheckable(true);
	connect(m_stickyAct, SIGNAL(triggered()), this, SLOT(setSticky()));

	m_selectMoveLockAct = new QAction(tr("Select All Locked Parts"), this);
	m_selectMoveLockAct->setStatusTip(tr("Select all parts that can't be moved"));
	connect(m_selectMoveLockAct, SIGNAL(triggered()), this, SLOT(selectMoveLock()));

	m_showPartLabelAct = new QAction(tr("&Show part label"), this);
	m_showPartLabelAct->setStatusTip(tr("Show/hide the label for the selected parts"));
	connect(m_showPartLabelAct, SIGNAL(triggered()), this, SLOT(showPartLabels()));

	m_saveBundledPart = new QAction(tr("&Export..."), this);
	m_saveBundledPart->setStatusTip(tr("Export selected part"));
	connect(m_saveBundledPart, SIGNAL(triggered()), this, SLOT(saveBundledPart()));

	m_addBendpointAct = new BendpointAction(tr("Add Bendpoint"), this);
	m_addBendpointAct->setStatusTip(tr("Add a bendpoint to the selected wire"));
	connect(m_addBendpointAct, SIGNAL(triggered()), this, SLOT(addBendpoint()));

	m_convertToViaAct = new BendpointAction(tr("Convert Bendpoint to Via"), this);
	m_convertToViaAct->setStatusTip(tr("Convert the bendpoint to a via"));
	connect(m_convertToViaAct, SIGNAL(triggered()), this, SLOT(convertToVia()));

	m_convertToBendpointAct = new QAction(tr("Convert Via to Bendpoint"), this);
	m_convertToBendpointAct->setStatusTip(tr("Convert the via to a bendpoint"));
	connect(m_convertToBendpointAct, SIGNAL(triggered()), this, SLOT(convertToBendpoint()));

	m_flattenCurveAct = new BendpointAction(tr("Straighten Curve"), this);
	m_flattenCurveAct->setStatusTip(tr("Straighten the curve of the selected wire"));
	connect(m_flattenCurveAct, SIGNAL(triggered()), this, SLOT(flattenCurve()));

	m_selectAllObsoleteAct = new QAction(tr("Select outdated parts"), this);
	m_selectAllObsoleteAct->setStatusTip(tr("Select outdated parts"));
	connect(m_selectAllObsoleteAct, SIGNAL(triggered()), this, SLOT(selectAllObsolete()));

	m_swapObsoleteAct = new QAction(tr("Update selected parts"), this);
	m_swapObsoleteAct->setStatusTip(tr("Update selected parts"));
	connect(m_swapObsoleteAct, SIGNAL(triggered()), this, SLOT(swapObsolete()));

	m_findPartInSketchAct = new QAction(tr("Find part in sketch..."), this);
	m_findPartInSketchAct->setStatusTip(tr("Search for parts in a sketch by matching text"));
	m_findPartInSketchAct->setShortcut(QKeySequence::Find);
	connect(m_findPartInSketchAct, SIGNAL(triggered()), this, SLOT(findPartInSketch()));

	m_hidePartSilkscreenAct = new QAction(tr("Hide part silkscreen"), this);
	m_hidePartSilkscreenAct->setStatusTip(tr("Hide/show the silkscreen layer for only this part"));
	connect(m_hidePartSilkscreenAct, SIGNAL(triggered()), this, SLOT(hidePartSilkscreen()));

	m_regeneratePartsDatabaseAct = new QAction(tr("Regenerate parts database ..."), this);
	m_regeneratePartsDatabaseAct->setStatusTip(tr("Regenerate the parts database (should only be used if your parts database is broken)"));
	connect(m_regeneratePartsDatabaseAct, SIGNAL(triggered()), qApp, SLOT(regeneratePartsDatabase()));

}

void MainWindow::createViewMenuActions(bool showWelcome) {
	m_zoomInAct = new QAction(tr("&Zoom In"), this);
	m_zoomInAct->setShortcut(tr("Ctrl++"));
	m_zoomInAct->setStatusTip(tr("Zoom in"));
	connect(m_zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));

	// instead of creating a filter to grab the shortcut, let's create a new action
	// and append it to the window
	m_zoomInShortcut = new QAction(this);
	m_zoomInShortcut->setShortcut(tr("Ctrl+="));
	connect(m_zoomInShortcut, SIGNAL(triggered()), this, SLOT(zoomIn()));
	this->addAction(m_zoomInShortcut);

	m_zoomOutAct = new QAction(tr("&Zoom Out"), this);
	m_zoomOutAct->setShortcut(tr("Ctrl+-"));
	m_zoomOutAct->setStatusTip(tr("Zoom out"));
	connect(m_zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));

	m_fitInWindowAct = new QAction(tr("&Fit in Window"), this);
	m_fitInWindowAct->setShortcut(tr("Ctrl+0"));
	m_fitInWindowAct->setStatusTip(tr("Fit in window"));
	connect(m_fitInWindowAct, SIGNAL(triggered()), this, SLOT(fitInWindow()));

	m_actualSizeAct = new QAction(tr("&Actual Size"), this);
	m_actualSizeAct->setStatusTip(tr("Actual (real world physical) size"));
	connect(m_actualSizeAct, SIGNAL(triggered()), this, SLOT(actualSize()));

	m_100PercentSizeAct = new QAction(tr("100% Size"), this);
	m_100PercentSizeAct->setShortcut(tr("Shift+Ctrl+0"));
	m_100PercentSizeAct->setStatusTip(tr("100% (pixel) size"));
	connect(m_100PercentSizeAct, SIGNAL(triggered()), this, SLOT(hundredPercentSize()));

	m_alignToGridAct = new QAction(tr("Align to Grid"), this);
	m_alignToGridAct->setStatusTip(tr("Align items to grid when dragging"));
	m_alignToGridAct->setCheckable(true);
	connect(m_alignToGridAct, SIGNAL(triggered()), this, SLOT(alignToGrid()));

	m_colorWiresByLengthAct = new QAction(tr("Color Breadboard Wires By Length"), this);
	m_colorWiresByLengthAct->setStatusTip(tr("Display breadboard wires using standard color coding by length"));
	m_colorWiresByLengthAct->setCheckable(true);
	connect(m_colorWiresByLengthAct, SIGNAL(triggered()), this, SLOT(colorWiresByLength()));

	m_startSimulatorAct = new QAction(tr("Start Simulator"), this);
	m_startSimulatorAct->setStatusTip(tr("Starts the simulator (DC analysis)"));
	connect(m_startSimulatorAct, SIGNAL(triggered()), m_simulator, SLOT(startSimulation()));

	m_stopSimulatorAct = new QAction(tr("Stop Simulator"), this);
	m_stopSimulatorAct->setStatusTip(tr("Stops the simulator and removes simulator data"));
	connect(m_stopSimulatorAct, SIGNAL(triggered()), m_simulator, SLOT(stopSimulation()));

	m_showGridAct = new QAction(tr("Show Grid"), this);
	m_showGridAct->setStatusTip(tr("Show the grid"));
	m_showGridAct->setCheckable(true);
	connect(m_showGridAct, SIGNAL(triggered()), this, SLOT(showGrid()));

	m_setGridSizeAct = new QAction(tr("Set Grid Size..."), this);
	m_setGridSizeAct->setStatusTip(tr("Set the size of the grid in this view"));
	connect(m_setGridSizeAct, SIGNAL(triggered()), this, SLOT(setGridSize()));

	m_setBackgroundColorAct = new QAction(tr("Set Background Color..."), this);
	m_setBackgroundColorAct->setStatusTip(tr("Set the background color of this view"));
	connect(m_setBackgroundColorAct, SIGNAL(triggered()), this, SLOT(setBackgroundColor()));

	QStringList controls;
	controls << tr("Ctrl+1") << tr("Ctrl+2") << tr("Ctrl+3") << tr("Ctrl+4") << tr("Ctrl+5");
	int controlIndex = 0;
	if (showWelcome) {
		m_showWelcomeAct = new QAction(tr("&Show Welcome"), this);
		m_showWelcomeAct->setShortcut(controls.at(controlIndex++));
		m_showWelcomeAct->setStatusTip(tr("Show the welcome view"));
		m_showWelcomeAct->setCheckable(true);
		connect(m_showWelcomeAct, SIGNAL(triggered()), this, SLOT(showWelcomeView()));
	}

	m_showBreadboardAct = new QAction(tr("&Show Breadboard"), this);
	m_showBreadboardAct->setShortcut(controls.at(controlIndex++));
	m_showBreadboardAct->setStatusTip(tr("Show the breadboard view"));
	m_showBreadboardAct->setCheckable(true);
	connect(m_showBreadboardAct, SIGNAL(triggered()), this, SLOT(showBreadboardView()));

	m_showSchematicAct = new QAction(tr("&Show Schematic"), this);
	m_showSchematicAct->setShortcut(controls.at(controlIndex++));
	m_showSchematicAct->setStatusTip(tr("Show the schematic view"));
	m_showSchematicAct->setCheckable(true);
	connect(m_showSchematicAct, SIGNAL(triggered()), this, SLOT(showSchematicView()));

	m_showPCBAct = new QAction(tr("&Show PCB"), this);
	m_showPCBAct->setShortcut(controls.at(controlIndex++));
	m_showPCBAct->setStatusTip(tr("Show the PCB view"));
	m_showPCBAct->setCheckable(true);
	connect(m_showPCBAct, SIGNAL(triggered()), this, SLOT(showPCBView()));

	if (m_programView) {
		m_showProgramAct = new QAction(tr("Show Code"), this);
		m_showProgramAct->setShortcut(controls.at(controlIndex++));
		m_showProgramAct->setStatusTip(tr("Show the code (programming) view"));
		m_showProgramAct->setCheckable(true);
		connect(m_showProgramAct, SIGNAL(triggered()), this, SLOT(showProgramView()));
		QList<QAction *> viewMenuActions;
		if (m_welcomeView) viewMenuActions << m_showWelcomeAct;
		viewMenuActions << m_showBreadboardAct << m_showSchematicAct << m_showPCBAct << m_showProgramAct;
		m_programView->createViewMenuActions(viewMenuActions);
	}

	m_showPartsBinIconViewAct = new QAction(tr("Show Parts Bin Icon View"), this);
	m_showPartsBinIconViewAct->setStatusTip(tr("Display the parts bin in an icon view"));
	m_showPartsBinIconViewAct->setCheckable(true);
	connect(m_showPartsBinIconViewAct, SIGNAL(triggered()), this, SLOT(showPartsBinIconView()));

	m_showPartsBinListViewAct = new QAction(tr("Show Parts Bin List View"), this);
	m_showPartsBinListViewAct->setStatusTip(tr("Display the parts bin in a list view"));
	m_showPartsBinListViewAct->setCheckable(true);
	connect(m_showPartsBinListViewAct, SIGNAL(triggered()), this, SLOT(showPartsBinListView()));

	m_showAllLayersAct = new QAction(tr("&Show All Layers"), this);
	m_showAllLayersAct->setStatusTip(tr("Show all the available layers for the current view"));
	connect(m_showAllLayersAct, SIGNAL(triggered()), this, SLOT(showAllLayers()));

	m_hideAllLayersAct = new QAction(tr("&Hide All Layers"), this);
	m_hideAllLayersAct->setStatusTip(tr("Hide all the layers of the current view"));
	connect(m_hideAllLayersAct, SIGNAL(triggered()), this, SLOT(hideAllLayers()));

}

void MainWindow::createWindowMenuActions() {
	m_minimizeAct = new QAction(tr("&Minimize"), this);
	m_minimizeAct->setShortcut(tr("Ctrl+M"));
	m_minimizeAct->setStatusTip(tr("Minimize current window"));
	connect(m_minimizeAct, SIGNAL(triggered(bool)), this, SLOT(minimize()));

	/*
	m_toggleToolbarAct = new QAction(tr("&Toolbar"), this);
	m_toggleToolbarAct->setShortcut(tr("Shift+Ctrl+T"));
	m_toggleToolbarAct->setCheckable(true);
	m_toggleToolbarAct->setChecked(true);
	m_toggleToolbarAct->setStatusTip(tr("Toggle Toolbar visibility"));
	connect(m_toggleToolbarAct, SIGNAL(triggered(bool)), this, SLOT(toggleToolbar(bool)));
	*/

	m_toggleDebuggerOutputAct = new QAction(tr("Debugger Output"), this);
	m_toggleDebuggerOutputAct->setCheckable(true);
	connect(m_toggleDebuggerOutputAct, SIGNAL(triggered(bool)), this, SLOT(toggleDebuggerOutput(bool)));

	m_openProgramWindowAct = new QAction(tr("Open programming window"), this);
	m_openProgramWindowAct->setStatusTip(tr("Open microcontroller programming window"));
	connect(m_openProgramWindowAct, SIGNAL(triggered()), this, SLOT(openProgramWindow()));
}

void MainWindow::createHelpMenuActions() {
	m_openHelpAct = new QAction(tr("Online Tutorials"), this);
	m_openHelpAct->setShortcut(tr("Ctrl+?"));
	m_openHelpAct->setStatusTip(tr("Open Fritzing help"));
	connect(m_openHelpAct, SIGNAL(triggered(bool)), this, SLOT(openHelp()));

	m_examplesAct = new QAction(tr("Online Projects Gallery"), this);
	m_examplesAct->setStatusTip(tr("Open Fritzing examples"));
	connect(m_examplesAct, SIGNAL(triggered(bool)), this, SLOT(openExamples()));

	m_partsRefAct = new QAction(tr("Online Parts Reference"), this);
	m_partsRefAct->setStatusTip(tr("Open Parts Reference"));
	connect(m_partsRefAct, SIGNAL(triggered(bool)), this, SLOT(openPartsReference()));

	m_visitFritzingDotOrgAct = new QAction(tr("Visit fritzing.org"), this);
	m_visitFritzingDotOrgAct->setStatusTip(tr("fritzing.org"));
	connect(m_visitFritzingDotOrgAct, SIGNAL(triggered(bool)), this, SLOT(visitFritzingDotOrg()));

	m_checkForUpdatesAct = new QAction(tr("Check for updates..."), this);
	m_checkForUpdatesAct->setStatusTip(tr("Check whether a newer version of Fritzing is available for download"));
	connect(m_checkForUpdatesAct, SIGNAL(triggered()), QApplication::instance(), SLOT(checkForUpdates()));

	m_aboutAct = new QAction(tr("&About"), this);
	m_aboutAct->setStatusTip(tr("Show the application's about box"));
	connect(m_aboutAct, SIGNAL(triggered()), this, SLOT(about()));
	m_aboutAct->setMenuRole(QAction::AboutRole);

	m_tipsAndTricksAct = new QAction(tr("Tips, Tricks and Shortcuts"), this);
	m_tipsAndTricksAct->setStatusTip(tr("Display some handy Fritzing tips and tricks"));
	connect(m_tipsAndTricksAct, SIGNAL(triggered()), this, SLOT(tipsAndTricks()));

	m_firstTimeHelpAct = new QAction(tr("First Time Help"), this);
	m_firstTimeHelpAct->setStatusTip(tr("Display First Time Help"));
	connect(m_firstTimeHelpAct, SIGNAL(triggered()), this, SLOT(firstTimeHelp()));

	m_aboutQtAct = new QAction(tr("&About Qt"), this);
	m_aboutQtAct->setStatusTip(tr("Show Qt's about box"));
	connect(m_aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

	m_reportBugAct = new QAction(tr("Report a bug..."), this);
	m_reportBugAct->setStatusTip(tr("Report a but you've found in Fritzing"));
	connect(m_reportBugAct, SIGNAL(triggered()), this, SLOT(reportBug()));

	m_enableDebugAct = new QAction(tr("Enable debugging log"), this);
	m_enableDebugAct->setStatusTip(tr("Report a but you've found in Fritzing"));
	m_enableDebugAct->setCheckable(true);
	m_enableDebugAct->setChecked(DebugDialog::enabled());
	connect(m_enableDebugAct, SIGNAL(triggered()), this, SLOT(enableDebug()));

	m_partsEditorHelpAct = new QAction(tr("Parts Editor Help"), this);
	m_partsEditorHelpAct->setStatusTip(tr("Display Parts Editor help in a browser"));
	connect(m_partsEditorHelpAct, SIGNAL(triggered(bool)), this, SLOT(partsEditorHelp()));


}

void MainWindow::createMenus()
{
	createFileMenu();
	createEditMenu();
	createPartMenu();
	createViewMenu();
	m_programView->initMenus(menuBar());
	createWindowMenu();
	createTraceMenus();
	createHelpMenu();

	auto * actionProbe = new FProbeActions("MenuBar", menuBar());
}

QMenu * MainWindow::createRotateSubmenu(QMenu * parentMenu) {
	QMenu * rotateMenu = parentMenu->addMenu(tr("Rotate"));
	rotateMenu->addAction(m_rotate45cwAct);
	rotateMenu->addAction(m_rotate90cwAct);
	rotateMenu->addAction(m_rotate180Act);
	rotateMenu->addAction(m_rotate90ccwAct);
	rotateMenu->addAction(m_rotate45ccwAct);
	return rotateMenu;
}

QMenu * MainWindow::createZOrderSubmenu(QMenu * parentMenu) {
	QMenu *zOrderMenu = parentMenu->addMenu(tr("Raise and Lower"));
	zOrderMenu->addAction(m_bringToFrontAct);
	zOrderMenu->addAction(m_bringForwardAct);
	zOrderMenu->addAction(m_sendBackwardAct);
	zOrderMenu->addAction(m_sendToBackAct);

	return zOrderMenu;
}

/*
void MainWindow::createZOrderWireSubmenu(QMenu * parentMenu) {
	QMenu *zOrderWireMenu = parentMenu->addMenu(tr("Raise and Lower"));
	zOrderWireMenu->addAction(m_bringToFrontWireAct);
	zOrderWireMenu->addAction(m_bringForwardWireAct);
	zOrderWireMenu->addAction(m_sendBackwardWireAct);
	zOrderWireMenu->addAction(m_sendToBackWireAct);
}
*/

QMenu * MainWindow::createAlignSubmenu(QMenu * parentMenu) {
	QMenu *alignMenu = parentMenu->addMenu(tr("Align"));
	alignMenu->addAction(m_alignLeftAct);
	alignMenu->addAction(m_alignHorizontalCenterAct);
	alignMenu->addAction(m_alignRightAct);
	alignMenu->addAction(m_alignTopAct);
	alignMenu->addAction(m_alignVerticalCenterAct);
	alignMenu->addAction(m_alignBottomAct);

	return alignMenu;
}

QMenu * MainWindow::createAddToBinSubmenu(QMenu * parentMenu) {
	QMenu *addToBinMenu = parentMenu->addMenu(tr("&Add to bin..."));
	addToBinMenu->setStatusTip(tr("Add selected part to bin"));
	QList<QAction*> acts = m_binManager->openedBinsActions(selectedModuleID());
	addToBinMenu->addActions(acts);

	return addToBinMenu;
}

void MainWindow::createFileMenu() {
	m_fileMenu = menuBar()->addMenu(tr("&File"));
	m_fileMenu->addAction(m_newAct);
	m_fileMenu->addAction(m_openAct);
	m_fileMenu->addAction(m_revertAct);
	m_fileMenu->addMenu(m_openRecentFileMenu);
	m_fileMenu->addMenu(m_openExampleMenu);

	m_fileMenu->addSeparator();
	m_fileMenu->addAction(m_closeAct);
	m_fileMenu->addAction(m_saveAct);
	m_fileMenu->addAction(m_saveAsAct);
	m_fileMenu->addAction(m_shareOnlineAct);

	if (m_orderFabEnabled) {
		m_fileMenu->addAction(m_orderFabAct);
	}

	m_fileMenu->addSeparator();
	m_exportMenu = m_fileMenu->addMenu(tr("&Export"));
	connect(m_exportMenu, SIGNAL(aboutToShow()), this, SLOT(updateExportMenu()));
	//m_fileMenu->addAction(m_pageSetupAct);
	m_fileMenu->addAction(m_printAct);

	QString name;
	QString path;
	QStringList args;
	if (externalProcess(name, path, args)) {
		m_fileMenu->addSeparator();
		m_fileMenu->addAction(m_launchExternalProcessAct);
	}

#ifndef QT_NO_DEBUG
	m_fileMenu->addSeparator();
	m_fileMenu->addAction(m_exceptionAct);
#endif

	m_fileMenu->addSeparator();
	m_fileMenu->addAction(m_quitAct);
	connect(m_fileMenu, SIGNAL(aboutToShow()), this, SLOT(updateFileMenu()));

	populateExportMenu();

	m_exportMenu->addAction(m_exportBomAct);
	m_exportMenu->addAction(m_exportBomCsvAct);
	m_exportMenu->addAction(m_exportIpcAct);
	m_exportMenu->addAction(m_exportNetlistAct);
	m_exportMenu->addAction(m_exportSpiceNetlistAct);

	//m_exportMenu->addAction(m_exportEagleAct);
}

void MainWindow::populateExportMenu() {
	QMenu * imageMenu = m_exportMenu->addMenu(tr("as Image"));
	imageMenu->addAction(m_exportPngAct);
	imageMenu->addAction(m_exportJpgAct);
	imageMenu->addSeparator();
	imageMenu->addAction(m_exportSvgAct);
	imageMenu->addAction(m_exportPdfAct);

	QMenu * productionMenu = m_exportMenu->addMenu(tr("for Production"));
	productionMenu->addAction(m_exportEtchablePdfAct);
	productionMenu->addAction(m_exportEtchableSvgAct);
	productionMenu->addSeparator();
	productionMenu->addAction(m_exportGerberAct);
}


void MainWindow::createEditMenu()
{
	m_editMenu = menuBar()->addMenu(tr("&Edit"));
	m_editMenu->addAction(m_undoAct);
	m_editMenu->addAction(m_redoAct);
	m_editMenu->addSeparator();
	m_editMenu->addAction(m_cutAct);
	m_editMenu->addAction(m_copyAct);
	m_editMenu->addAction(m_pasteAct);
	m_editMenu->addAction(m_pasteInPlaceAct);
	m_editMenu->addAction(m_duplicateAct);
	m_editMenu->addAction(m_deleteAct);
	m_editMenu->addAction(m_deleteMinusAct);
	m_editMenu->addSeparator();
	m_editMenu->addAction(m_selectAllAct);
	m_editMenu->addAction(m_deselectAct);
	m_editMenu->addSeparator();
	m_editMenu->addAction(m_addNoteAct);
	m_editMenu->addSeparator();
#ifndef Q_OS_MACOS
	m_editMenu->addAction(m_preferencesAct);
#endif
	updateEditMenu();
	connect(m_editMenu, SIGNAL(aboutToShow()), this, SLOT(updateEditMenu()));
}

void MainWindow::createPartMenu() {
	m_partMenu = menuBar()->addMenu(tr("&Part"));
	connect(m_partMenu, SIGNAL(aboutToShow()), this, SLOT(updatePartMenu()));

	m_partMenu->addAction(m_openInPartsEditorNewAct);

	m_partMenu->addSeparator();
	m_partMenu->addAction(m_saveBundledPart);

	m_partMenu->addSeparator();
	m_partMenu->addAction(m_flipHorizontalAct);
	m_partMenu->addAction(m_flipVerticalAct);
	m_rotateMenu = createRotateSubmenu(m_partMenu);
	m_zOrderMenu = createZOrderSubmenu(m_partMenu);

	//createZOrderWireSubmenu(m_partMenu);
	m_alignMenu = createAlignSubmenu(m_partMenu);
	m_partMenu->addAction(m_moveLockAct);
	m_partMenu->addAction(m_stickyAct);
	m_partMenu->addAction(m_selectMoveLockAct);

	m_partMenu->addSeparator();
	m_addToBinMenu = createAddToBinSubmenu(m_partMenu);
	m_partMenu->addAction(m_showPartLabelAct);
	m_partMenu->addSeparator();
	m_partMenu->addAction(m_selectAllObsoleteAct);
	m_partMenu->addAction(m_swapObsoleteAct);

	m_partMenu->addSeparator();
	m_partMenu->addAction(m_findPartInSketchAct);


#ifndef QT_NO_DEBUG
	m_partMenu->addAction(m_dumpAllPartsAction);
#endif

	m_partMenu->addSeparator();
	m_partMenu->addAction(m_regeneratePartsDatabaseAct);
}

void MainWindow::createViewMenu()
{
	m_viewMenu = menuBar()->addMenu(tr("&View"));
	m_viewMenu->addAction(m_zoomInAct);
	m_viewMenu->addAction(m_zoomOutAct);
	m_viewMenu->addAction(m_fitInWindowAct);
	m_viewMenu->addAction(m_actualSizeAct);
	m_viewMenu->addAction(m_100PercentSizeAct);
	m_viewMenu->addSeparator();

	m_viewMenu->addAction(m_alignToGridAct);
	m_viewMenu->addAction(m_showGridAct);
	m_viewMenu->addAction(m_setGridSizeAct);
	m_viewMenu->addAction(m_setBackgroundColorAct);
	m_viewMenu->addAction(m_colorWiresByLengthAct);
	m_viewMenu->addSeparator();

	if (m_welcomeView) m_viewMenu->addAction(m_showWelcomeAct);
	m_viewMenu->addAction(m_showBreadboardAct);
	m_viewMenu->addAction(m_showSchematicAct);
	m_viewMenu->addAction(m_showPCBAct);
	if (m_programView) m_viewMenu->addAction(m_showProgramAct);
	m_viewMenu->addSeparator();
	if (m_binManager) {
		m_viewMenu->addAction(m_showPartsBinIconViewAct);
		m_viewMenu->addAction(m_showPartsBinListViewAct);
		m_viewMenu->addSeparator();
	}
	connect(m_viewMenu, SIGNAL(aboutToShow()), this, SLOT(updateLayerMenu()));
	m_numFixedActionsInViewMenu = m_viewMenu->actions().size();
}

void MainWindow::createWindowMenu() {
	m_windowMenu = menuBar()->addMenu(tr("&Window"));
	m_windowMenu->addAction(m_minimizeAct);
	m_windowMenu->addSeparator();
	//m_windowMenu->addAction(m_toggleToolbarAct);
	updateWindowMenu();
	connect(m_windowMenu, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));
}

void MainWindow::createTraceMenus()
{
	m_pcbTraceMenu = menuBar()->addMenu(tr("&Routing"));
	m_pcbTraceMenu->addAction(m_newAutorouteAct);
	m_pcbTraceMenu->addAction(m_newDesignRulesCheckAct);
	m_pcbTraceMenu->addAction(m_autorouterSettingsAct);
	m_pcbTraceMenu->addAction(m_fabQuoteAct);

	QMenu * groundFillMenu = m_pcbTraceMenu->addMenu(tr("Ground Fill"));

	groundFillMenu->addAction(m_copperFillAct);
	groundFillMenu->addAction(m_groundFillAct);
	groundFillMenu->addAction(m_copperFillOldAct);
	groundFillMenu->addAction(m_groundFillOldAct);
	groundFillMenu->addAction(m_removeGroundFillAct);
	groundFillMenu->addAction(m_setGroundFillSeedsAct);
	groundFillMenu->addAction(m_clearGroundFillSeedsAct);
	groundFillMenu->addAction(m_setGroundFillKeepoutAct);
	//m_pcbTraceMenu->addAction(m_updateRoutingStatusAct);
	m_pcbTraceMenu->addSeparator();

	m_pcbTraceMenu->addAction(m_viewFromBelowToggleAct);
	m_pcbTraceMenu->addAction(m_activeLayerBothAct);
	m_pcbTraceMenu->addAction(m_activeLayerBottomAct);
	m_pcbTraceMenu->addAction(m_activeLayerTopAct);
	m_pcbTraceMenu->addSeparator();

	m_pcbTraceMenu->addAction(m_changeTraceLayerAct);
	m_pcbTraceMenu->addAction(m_excludeFromAutorouteAct);
	m_pcbTraceMenu->addSeparator();

	m_pcbTraceMenu->addAction(m_showUnroutedAct);
	m_pcbTraceMenu->addAction(m_selectAllTracesAct);
	m_pcbTraceMenu->addAction(m_selectAllExcludedTracesAct);
	m_pcbTraceMenu->addAction(m_selectAllIncludedTracesAct);
	m_pcbTraceMenu->addAction(m_selectAllJumperItemsAct);
	m_pcbTraceMenu->addAction(m_selectAllViasAct);
	m_pcbTraceMenu->addAction(m_selectAllCopperFillAct);

	//m_pcbTraceMenu->addSeparator();
	//m_pcbTraceMenu->addAction(m_checkLoadedTracesAct);

	m_schematicTraceMenu = menuBar()->addMenu(tr("&Routing"));
	m_schematicTraceMenu->addAction(m_newAutorouteAct);
	m_schematicTraceMenu->addAction(m_excludeFromAutorouteAct);
	m_schematicTraceMenu->addAction(m_showUnroutedAct);
	m_schematicTraceMenu->addAction(m_selectAllTracesAct);
	m_schematicTraceMenu->addAction(m_selectAllExcludedTracesAct);
	m_schematicTraceMenu->addAction(m_selectAllIncludedTracesAct);
	//m_schematicTraceMenu->addAction(m_updateRoutingStatusAct);

#ifndef QT_NO_DEBUG
	m_schematicTraceMenu->addAction(m_tidyWiresAct);
#endif

	m_breadboardTraceMenu = menuBar()->addMenu(tr("&Routing"));
	m_breadboardTraceMenu->addAction(m_showUnroutedAct);
	m_breadboardTraceMenu->addAction(m_selectAllWiresAct);

	updateTraceMenu();
	connect(m_pcbTraceMenu, SIGNAL(aboutToShow()), this, SLOT(updateTraceMenu()));
	connect(m_schematicTraceMenu, SIGNAL(aboutToShow()), this, SLOT(updateTraceMenu()));
	connect(m_breadboardTraceMenu, SIGNAL(aboutToShow()), this, SLOT(updateTraceMenu()));

	menuBar()->addSeparator();
}

void MainWindow::createHelpMenu()
{
	m_helpMenu = menuBar()->addMenu(tr("&Help"));
	m_helpMenu->addAction(m_openHelpAct);
	m_helpMenu->addAction(m_examplesAct);
	m_helpMenu->addAction(m_partsRefAct);
	m_helpMenu->addSeparator();
	m_helpMenu->addAction(m_partsEditorHelpAct);
	m_helpMenu->addSeparator();
	m_helpMenu->addAction(m_checkForUpdatesAct);
	m_helpMenu->addSeparator();
	m_helpMenu->addAction(m_reportBugAct);
	m_helpMenu->addAction(m_enableDebugAct);
	m_helpMenu->addSeparator();
	m_helpMenu->addAction(m_aboutAct);
	m_helpMenu->addAction(m_tipsAndTricksAct);
	m_helpMenu->addAction(m_firstTimeHelpAct);
#ifndef QT_NO_DEBUG
	m_helpMenu->addAction(m_aboutQtAct);
#endif
#ifdef Q_OS_MACOS
	m_helpMenu->addAction(m_preferencesAct);
#endif
}


void MainWindow::updateLayerMenu(bool resetLayout) {
	if (m_viewMenu == nullptr) return;
	if (m_showAllLayersAct == nullptr) return;

	QList<QAction *> actions;
	actions << m_zoomInAct << m_zoomOutAct << m_zoomInShortcut << m_fitInWindowAct << m_actualSizeAct <<
			m_100PercentSizeAct << m_alignToGridAct << m_showGridAct << m_setGridSizeAct << m_setBackgroundColorAct <<
			m_colorWiresByLengthAct;

	bool enabled = (m_currentGraphicsView);
	Q_FOREACH (QAction * action, actions) action->setEnabled(enabled);

	actions.clear();

	if (m_showPartsBinIconViewAct) {
		if (m_binManager) {
			m_showPartsBinIconViewAct->setEnabled(true);
			m_showPartsBinListViewAct->setEnabled(true);
			actions << m_showPartsBinIconViewAct << m_showPartsBinListViewAct;
			setActionsIcons(m_binManager->currentViewIsIconView() ? 0 : 1, actions);
		}
		else {
			m_showPartsBinIconViewAct->setEnabled(false);
			m_showPartsBinListViewAct->setEnabled(false);
		}
	}

	removeActionsStartingAt(m_viewMenu, m_numFixedActionsInViewMenu);
	if (m_showAllLayersAct) m_viewMenu->addAction(m_showAllLayersAct);
	if (m_hideAllLayersAct) m_viewMenu->addAction(m_hideAllLayersAct);

	if (m_hideAllLayersAct) m_hideAllLayersAct->setEnabled(false);
	if (m_showAllLayersAct) m_showAllLayersAct->setEnabled(false);

	if (m_currentGraphicsView == nullptr) {
		return;
	}

	m_alignToGridAct->setChecked(m_currentGraphicsView->alignedToGrid());
	m_showGridAct->setChecked(m_currentGraphicsView->showingGrid());
	m_colorWiresByLengthAct->setChecked(m_breadboardGraphicsView->coloringWiresByLength());
	m_colorWiresByLengthAct->setEnabled(m_currentGraphicsView == m_breadboardGraphicsView);

	LayerHash viewLayers = m_currentGraphicsView->viewLayers();
	LayerList keys = viewLayers.keys();

	// make sure they're in ascending order when inserting into the menu
	std::sort(keys.begin(), keys.end());

	Q_FOREACH (ViewLayer::ViewLayerID key, keys) {
		ViewLayer * viewLayer = viewLayers.value(key);
		//DebugDialog::debug(QString("Layer: %1 is %2").arg(viewLayer->action()->text()).arg(viewLayer->action()->isEnabled()));
		if (viewLayer) {
			if (viewLayer->parentLayer()) continue;
			m_viewMenu->addAction(viewLayer->action());
			disconnect(viewLayer->action(), SIGNAL(triggered()), this, SLOT(updateLayerMenu()));
			connect(viewLayer->action(), SIGNAL(triggered()), this, SLOT(updateLayerMenu()));
		}
	}


	if (keys.count() <= 0) return;

	ViewLayer *prev = viewLayers.value(keys[0]);
	if (prev == nullptr) {
		// jrc: I think prev == NULL is actually a side effect from an earlier bug
		// but I haven't figured out the cause yet
		// at any rate, when this bug occurs, keys[0] is some big negative number that looks like an
		// uninitialized or scrambled layerID
		DebugDialog::debug(QString("updateAllLayersActions keys[0] failed %1").arg(keys[0]) );
		return;
	}

	bool sameState = prev->action()->isChecked();
	bool checked = prev->action()->isChecked();
	//DebugDialog::debug(QString("Layer: %1 is %2").arg(prev->action()->text()).arg(prev->action()->isChecked()));
	for (int i = 1; i < keys.count(); i++) {
		ViewLayer *viewLayer = viewLayers.value(keys[i]);
		//DebugDialog::debug(QString("Layer: %1 is %2").arg(viewLayer->action()->text()).arg(viewLayer->action()->isChecked()));
		if (viewLayer) {
			if (prev && prev->action()->isChecked() != viewLayer->action()->isChecked() ) {
				// if the actions aren't all checked or unchecked I don't bother about the "checked" variable
				sameState = false;
				break;
			}
			else {
				sameState = true;
				checked = viewLayer->action()->isChecked();
			}
			prev = viewLayer;
		}
	}

	//DebugDialog::debug(QString("sameState: %1").arg(sameState));
	//DebugDialog::debug(QString("checked: %1").arg(checked));
	if (sameState) {
		if(checked) {
			if (m_hideAllLayersAct) m_hideAllLayersAct->setEnabled(true);
		}
		else {
			if (m_showAllLayersAct) m_showAllLayersAct->setEnabled(true);
		}
	}
	else {
		if (m_showAllLayersAct) m_showAllLayersAct->setEnabled(true);
		if (m_hideAllLayersAct) m_hideAllLayersAct->setEnabled(true);
	}

	if (resetLayout) {
		m_layerPalette->resetLayout(viewLayers, keys);
	}
	m_layerPalette->updateLayerPalette(viewLayers, keys);
}

void MainWindow::updateWireMenu() {
	// assumes update wire menu is only called when right-clicking a wire
	// and that wire is cached by the menu in Wire::mousePressEvent

	Wire * wire = m_activeWire;
	m_activeWire = nullptr;

	if (wire) {
		enableAddBendpointAct(wire);
	}

	bool enableAll = true;
	bool deleteOK = false;
	bool createTraceOK = false;
	bool excludeOK = false;
	bool enableZOK = true;
	bool gotRat = false;
	bool ctlOK = false;

	if (wire) {

		if (wire->getRatsnest()) {
			QList<ConnectorItem *> ends;
			Wire * jt = wire->findTraced(m_currentGraphicsView->getTraceFlag(), ends);
			createTraceOK = (jt == nullptr) || (!jt->getTrace());
			deleteOK = true;
			gotRat = true;
			enableZOK = false;
		}
		else if (wire->getTrace()) {
			deleteOK = true;
			excludeOK = true;
			m_excludeFromAutorouteWireAct->setChecked(!wire->getAutoroutable());
			if (m_currentGraphicsView == m_pcbGraphicsView && m_currentGraphicsView->boardLayers() > 1) {
				if (wire->canSwitchLayers()) {
					if (ViewLayer::topLayers().contains(wire->viewLayerID())) {
						m_changeTraceLayerWireAct->setText(tr("Move to bottom layer"));
					}
					else {
						m_changeTraceLayerWireAct->setText(tr("Move to top layer"));
					}
					ctlOK = true;
				}
			}

		}
		else {
			deleteOK = true;
		}
	}


	QMenu* wireColorMenu = (m_currentGraphicsView == m_breadboardGraphicsView ? m_breadboardWireColorMenu : m_schematicWireColorMenu);
	if (wire) {
		wireColorMenu->setEnabled(true);
		QString colorString = wire->colorString();
		//DebugDialog::debug("wire colorstring " + colorString);
		Q_FOREACH (QAction * action, wireColorMenu->actions()) {
			QString colorName = action->data().toString();
			//DebugDialog::debug("colorname " + colorName);
			action->setChecked(colorName.compare(colorString) == 0);
		}
	}
	else {
		wireColorMenu->setEnabled(false);
	}

	m_bringToFrontWireAct->setWire(wire);
	m_bringForwardWireAct->setWire(wire);
	m_sendBackwardWireAct->setWire(wire);
	m_sendToBackWireAct->setWire(wire);
	m_createTraceWireAct->setWire(wire);
	m_createWireWireAct->setWire(wire);
	m_deleteWireAct->setWire(wire);
	m_deleteWireMinusAct->setWire(wire);
	m_excludeFromAutorouteWireAct->setWire(wire);
	m_changeTraceLayerWireAct->setWire(wire);

	m_bringToFrontWireAct->setEnabled(enableZOK);
	m_bringForwardWireAct->setEnabled(enableZOK);
	m_sendBackwardWireAct->setEnabled(enableZOK);
	m_sendToBackWireAct->setEnabled(enableZOK);
	m_createTraceWireAct->setEnabled(enableAll && createTraceOK);
	m_createWireWireAct->setEnabled(enableAll && createTraceOK);
	m_deleteWireAct->setEnabled(enableAll && deleteOK);
	m_deleteWireMinusAct->setEnabled(enableAll && deleteOK && !gotRat);
	m_excludeFromAutorouteWireAct->setEnabled(enableAll && excludeOK);
	m_changeTraceLayerAct->setEnabled(ctlOK);
	m_changeTraceLayerWireAct->setEnabled(ctlOK);

	if (gotRat) {
		m_deleteWireAct->setText(tr("Delete Ratsnest Line"));
	}
	else {
		m_deleteWireAct->setText(tr("Delete Wire"));
	}
}

void MainWindow::setEnableSubmenu( QMenu * menu, bool value ) {
	Q_FOREACH (QAction * action, menu->actions()) {
		action->setEnabled(value);
	}
	menu->setEnabled(value);
	menu->menuAction()->setEnabled(value);
}

void MainWindow::updatePartMenu() {
	if (m_partMenu == nullptr) return;

	if (m_currentGraphicsView == nullptr) {
		Q_FOREACH (QAction * action, m_partMenu->actions()) {
			action->setEnabled(false);
			if (action->menu()) {
				setEnableSubmenu(action->menu(), false);
			}
		}
		return;
	}

	ItemCount itemCount = m_currentGraphicsView->calcItemCount();

	bool enable = true;
	bool zenable = true;

	if (itemCount.selCount <= 0) {
		zenable = enable = false;
	}
	else {
		if (itemCount.itemsCount == itemCount.selCount) {
			// if all items are selected
			// z-reordering is a no-op
			zenable = false;
		}
	}

	setEnableSubmenu(m_alignMenu, itemCount.selCount - itemCount.wireCount > 1);
	setEnableSubmenu(m_zOrderMenu, zenable);

	m_moveLockAct->setEnabled(itemCount.selCount > 0 && itemCount.selCount > itemCount.wireCount);
	m_moveLockAct->setChecked(itemCount.moveLockCount > 0);
	m_selectMoveLockAct->setEnabled(true);

	m_showPartLabelAct->setEnabled((itemCount.hasLabelCount > 0) && enable);
	m_showPartLabelAct->setText(itemCount.visLabelCount == itemCount.hasLabelCount ? tr("Hide part label") : tr("Show part label"));
	m_showPartLabelAct->setData(itemCount.visLabelCount != itemCount.hasLabelCount);

	bool renable = (itemCount.selRotatable > 0);
	bool renable45 = (itemCount.sel45Rotatable > 0);

	setEnableSubmenu(m_rotateMenu, renable && enable);
	m_rotate45ccwAct->setEnabled(renable && renable45 && enable);
	m_rotate45cwAct->setEnabled(renable && renable45 && enable);


	m_flipHorizontalAct->setEnabled(enable && (itemCount.selHFlipable > 0) && (m_currentGraphicsView != m_pcbGraphicsView));
	m_flipVerticalAct->setEnabled(enable && (itemCount.selVFlipable > 0) && (m_currentGraphicsView != m_pcbGraphicsView));

	setEnableSubmenu(m_addToBinMenu, enable);

	updateItemMenu();
	updateEditMenu();


	bool ctbpVisible = false;
	bool ctbpEnabled = false;
	if (itemCount.selCount == 1) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(m_currentGraphicsView->scene()->selectedItems()[0]);
		enableAddBendpointAct(itemBase);

		Via * via = qobject_cast<Via *>(itemBase->layerKinChief());
		if (via) {
			ctbpVisible = true;
			int count = 0;
			QList<ConnectorItem *> viaConnectorItems;
			viaConnectorItems << via->connectorItem();
			if (via->connectorItem()->getCrossLayerConnectorItem()) {
				viaConnectorItems << via->connectorItem()->getCrossLayerConnectorItem();
			}

			Q_FOREACH (ConnectorItem * viaConnectorItem, viaConnectorItems) {
				Q_FOREACH (ConnectorItem * connectorItem, viaConnectorItem->connectedToItems()) {
					Wire * wire = qobject_cast<Wire *>(connectorItem->attachedTo());
					if (wire == nullptr) continue;
					if (wire->getRatsnest()) continue;

					if (wire->isTraceType(m_currentGraphicsView->getTraceFlag())) {
						count++;
						if (count > 1) {
							ctbpEnabled = true;
							break;
						}
					}
				}
				if (count > 1) break;
			}
		}


		m_openInPartsEditorNewAct->setEnabled(itemBase->canEditPart());
		m_stickyAct->setVisible(itemBase->isBaseSticky());
		m_stickyAct->setEnabled(true);
		m_stickyAct->setChecked(itemBase->isBaseSticky() && itemBase->isLocalSticky());

		QList<ItemBase *> itemBases;
		itemBases.append(itemBase);
		itemBases.append(itemBase->layerKinChief()->layerKin());
		bool hpsa = false;
		Q_FOREACH (ItemBase * lkpi, itemBases) {
			if (lkpi->viewLayerID() == ViewLayer::Silkscreen1 || lkpi->viewLayerID() == ViewLayer::Silkscreen0) {
				hpsa = true;
				m_hidePartSilkscreenAct->setText(lkpi->layerHidden() ? tr("Show part silkscreen") : tr("Hide part silkscreen"));
				break;
			}
		}

		m_hidePartSilkscreenAct->setEnabled(hpsa);
	}
	else {
		m_hidePartSilkscreenAct->setEnabled(false);
		m_stickyAct->setVisible(false);
		m_openInPartsEditorNewAct->setEnabled(false);
	}
	m_convertToBendpointAct->setEnabled(ctbpEnabled);
	m_convertToBendpointAct->setVisible(ctbpVisible);
	m_convertToBendpointSeparator->setVisible(ctbpVisible);

	// TODO: only enable if there is an obsolete part in the sketch
	m_selectAllObsoleteAct->setEnabled(true);
	m_swapObsoleteAct->setEnabled(itemCount.obsoleteCount > 0);

	m_findPartInSketchAct->setEnabled(m_currentGraphicsView);
	m_regeneratePartsDatabaseAct->setEnabled(true);
	m_openProgramWindowAct->setEnabled(true);
}

void MainWindow::updateTransformationActions() {
	// update buttons in sketch toolbar at bottom

	if (m_currentGraphicsView == nullptr) return;
	if (m_rotate90cwAct == nullptr) return;

	ItemCount itemCount = m_currentGraphicsView->calcItemCount();
	bool enable = (itemCount.selRotatable > 0);
	bool renable = (itemCount.sel45Rotatable > 0);

//	DebugDialog::debug(QString("enable rotate (1) %1 %2").arg(enable).arg(m_rotate90ccwAct->isEnabled()));

	m_rotate90cwAct->setEnabled(enable);
	m_rotate180Act->setEnabled(enable);
	m_rotate90ccwAct->setEnabled(enable);
	m_rotate45ccwAct->setEnabled(enable && renable);
	m_rotate45cwAct->setEnabled(enable && renable);
	Q_FOREACH(SketchToolButton* rotateButton, m_rotateButtons) {
		rotateButton->setEnabled(enable);
	}

	m_flipHorizontalAct->setEnabled((itemCount.selHFlipable > 0) && (m_currentGraphicsView != m_pcbGraphicsView));
	m_flipVerticalAct->setEnabled((itemCount.selVFlipable > 0) && (m_currentGraphicsView != m_pcbGraphicsView));

	enable = m_flipHorizontalAct->isEnabled() || m_flipVerticalAct->isEnabled();
	Q_FOREACH(SketchToolButton* flipButton, m_flipButtons) {
		flipButton->setEnabled(enable);
	}
}

void MainWindow::updateItemMenu() {
	if (m_currentGraphicsView == nullptr) return;

	ConnectorItem * activeConnectorItem = m_activeConnectorItem;
	m_activeConnectorItem = nullptr;

	QList<QGraphicsItem *> items = m_currentGraphicsView->scene()->selectedItems();

	int selCount = 0;
	ItemBase * itemBase = nullptr;
	Q_FOREACH(QGraphicsItem * item, items) {
		ItemBase * ib = ItemBase::extractTopLevelItemBase(item);
		if (ib == nullptr) continue;

		selCount++;
		if (selCount == 1) itemBase = ib;
		else if (selCount > 1) break;
	}

	auto *selected = qobject_cast<PaletteItem *>(itemBase);
	bool enabled = (selCount == 1) && (selected);

	m_saveBundledPart->setEnabled(enabled && !selected->modelPart()->isCore());


	// can't open wire in parts editor
	enabled &= selected && itemBase && itemBase->canEditPart();

	m_disconnectAllAct->setEnabled(enabled && m_currentGraphicsView->canDisconnectAll() && (itemBase->rightClickedConnector()));

	bool gfsEnabled = false;
	if (activeConnectorItem) {
		if (activeConnectorItem->attachedToItemType() != ModelPart::CopperFill) {
			gfsEnabled = true;
			m_setOneGroundFillSeedAct->setChecked(activeConnectorItem->isGroundFillSeed());
		}
	}
	m_setOneGroundFillSeedAct->setEnabled(gfsEnabled);
	m_setOneGroundFillSeedAct->setConnectorItem(activeConnectorItem);
}

void MainWindow::updateEditMenu() {
	if (m_currentGraphicsView == nullptr) {
		Q_FOREACH (QAction * action, m_editMenu->actions()) {
			action->setEnabled(action == m_preferencesAct);
		}
		return;
	}

	Q_FOREACH (QAction * action, m_editMenu->actions()) {
		action->setEnabled(true);
	}

	QClipboard *clipboard = QApplication::clipboard();
	m_pasteAct->setEnabled(false);
	m_pasteInPlaceAct->setEnabled(false);
	if (clipboard) {
		const QMimeData *mimeData = clipboard->mimeData(QClipboard::Clipboard);
		if (mimeData) {
			if (mimeData->hasFormat("application/x-dnditemsdata")) {
				m_pasteAct->setEnabled(true);
				m_pasteInPlaceAct->setEnabled(true);
				//DebugDialog::debug(QString("paste enabled: true"));
			}
		}
	}

	const QList<QGraphicsItem *> items =  m_currentGraphicsView->scene()->selectedItems();
	bool copyActsEnabled = false;
	bool deleteActsEnabled = false;
	Q_FOREACH (QGraphicsItem * item, items) {
		if (m_currentGraphicsView->canDeleteItem(item, items.count())) {
			deleteActsEnabled = true;
		}
		if (m_currentGraphicsView->canCopyItem(item, items.count())) {
			copyActsEnabled = true;
		}
	}

	//DebugDialog::debug(QString("enable cut/copy/duplicate/delete %1 %2 %3").arg(copyActsEnabled).arg(deleteActsEnabled).arg(m_currentWidget->viewID()) );
	m_deleteAct->setEnabled(deleteActsEnabled);
	m_deleteMinusAct->setEnabled(deleteActsEnabled);
	m_deleteAct->setText(tr("Delete"));
	m_cutAct->setEnabled(deleteActsEnabled && copyActsEnabled);
	m_copyAct->setEnabled(copyActsEnabled);
	m_duplicateAct->setEnabled(copyActsEnabled);
}

void MainWindow::updateTraceMenu() {
	if (m_pcbTraceMenu == nullptr) return;

	bool tEnabled = false;
	bool twEnabled = true;
	bool ctlEnabled = false;

	TraceMenuThing traceMenuThing;

	if (m_currentGraphicsView) {
		QList<QGraphicsItem *> items = m_currentGraphicsView->scene()->items();
		Q_FOREACH (QGraphicsItem * item, items) {
			Wire * wire = dynamic_cast<Wire *>(item);
			if (wire == nullptr) {
				if (m_currentGraphicsView == m_pcbGraphicsView) {
					updatePCBTraceMenu(item, traceMenuThing);
				}
				continue;
			}

			if (!wire->isEverVisible()) continue;
			if (wire->getRatsnest()) {
				//rEnabled = true;
				//if (wire->isSelected()) {
				//ctEnabled = true;
				//}
			}
			else if (wire->isTraceType(m_currentGraphicsView->getTraceFlag())) {
				tEnabled = true;
				twEnabled = true;
				if (wire->isSelected()) {
					traceMenuThing.exEnabled = true;
					if (wire->getAutoroutable()) {
						traceMenuThing.exChecked = false;
					}
				}
				if (m_currentGraphicsView == m_pcbGraphicsView && m_currentGraphicsView->boardLayers() > 1) {
					if (wire->canSwitchLayers()) {
						ctlEnabled = true;
					}
				}
			}
		}
	}

	bool anyOrNo = (traceMenuThing.boardCount >= 1 || m_currentGraphicsView != m_pcbGraphicsView);

	m_excludeFromAutorouteAct->setEnabled(traceMenuThing.exEnabled);
	m_excludeFromAutorouteAct->setChecked(traceMenuThing.exChecked);
	m_changeTraceLayerAct->setEnabled(ctlEnabled);
	m_orderFabAct->setEnabled(traceMenuThing.boardCount > 0);
	m_showUnroutedAct->setEnabled(true);
	m_selectAllTracesAct->setEnabled(tEnabled && anyOrNo);
	m_selectAllWiresAct->setEnabled(tEnabled && anyOrNo);
	m_selectAllCopperFillAct->setEnabled(traceMenuThing.gfrEnabled && traceMenuThing.boardCount >= 1);
	m_selectAllExcludedTracesAct->setEnabled(tEnabled && anyOrNo);
	m_selectAllIncludedTracesAct->setEnabled(tEnabled && anyOrNo);
	m_selectAllJumperItemsAct->setEnabled(traceMenuThing.jiEnabled && traceMenuThing.boardCount >= 1);
	m_selectAllViasAct->setEnabled(traceMenuThing.viaEnabled && traceMenuThing.boardCount >= 1);
	m_tidyWiresAct->setEnabled(twEnabled);

	QString sides;
	if (m_pcbGraphicsView->layerIsActive(ViewLayer::Copper0) && m_pcbGraphicsView->layerIsActive(ViewLayer::Copper1)) {
		sides = tr("top and bottom");
	}
	else if (m_pcbGraphicsView->layerIsActive(ViewLayer::Copper0)) {
		sides = tr("bottom");
	}
	else sides = tr("top");

	QString groundFillString = tr("Ground Fill (%1)").arg(sides);
	QString copperFillString = tr("Copper Fill (%1)").arg(sides);

	m_groundFillAct->setEnabled(traceMenuThing.boardCount >= 1);
	m_groundFillAct->setText(groundFillString);
	m_copperFillAct->setEnabled(traceMenuThing.boardCount >= 1);
	m_copperFillAct->setText(copperFillString);

	QString groundFillOldString = tr("Old Ground Fill (%1)").arg(sides);
	QString copperFillOldString = tr("Old Copper Fill (%1)").arg(sides);

	m_groundFillOldAct->setEnabled(traceMenuThing.boardCount >= 1);
	m_groundFillOldAct->setText(groundFillOldString);
	m_copperFillOldAct->setEnabled(traceMenuThing.boardCount >= 1);
	m_copperFillOldAct->setText(copperFillOldString);
	m_removeGroundFillAct->setEnabled(traceMenuThing.gfrEnabled && traceMenuThing.boardCount >= 1);

	// TODO: set and clear enabler logic
	m_setGroundFillSeedsAct->setEnabled(traceMenuThing.gfsEnabled && traceMenuThing.boardCount >= 1);
	m_clearGroundFillSeedsAct->setEnabled(traceMenuThing.gfsEnabled && traceMenuThing.boardCount >= 1);

	m_newDesignRulesCheckAct->setEnabled(traceMenuThing.boardCount >= 1);
	m_autorouterSettingsAct->setEnabled(m_currentGraphicsView == m_pcbGraphicsView);
	m_updateRoutingStatusAct->setEnabled(true);


	m_fabQuoteAct->setEnabled(m_currentGraphicsView == m_pcbGraphicsView);
}


void MainWindow::updatePCBTraceMenu(QGraphicsItem * item, TraceMenuThing & traceMenuThing)
{
	auto * itemBase = dynamic_cast<ItemBase *>(item);
	if (itemBase == nullptr) return;
	if (!itemBase->isEverVisible()) return;

	if (!traceMenuThing.gfsEnabled) {
		traceMenuThing.gfsEnabled = itemBase->itemType() != ModelPart::CopperFill && itemBase->hasConnectors();
	}

	if (Board::isBoard(itemBase)) {
		traceMenuThing.boardCount++;
		if (itemBase->isSelected()) traceMenuThing.boardSelectedCount++;
	}

	switch (itemBase->itemType()) {
	case ModelPart::Jumper:
		traceMenuThing.jiEnabled = true;
		if (itemBase->isSelected()) {
			traceMenuThing.exEnabled = true;
			if (qobject_cast<JumperItem *>(itemBase->layerKinChief())->getAutoroutable()) {
				traceMenuThing.exChecked = false;
			}
		}
		break;
	case ModelPart::Via:
		traceMenuThing.viaEnabled = true;
		if (itemBase->isSelected()) {
			traceMenuThing.exEnabled = true;
			if (qobject_cast<Via *>(itemBase->layerKinChief())->getAutoroutable()) {
				traceMenuThing.exChecked = false;
			}
		}
		break;
	case ModelPart::CopperFill:
		traceMenuThing.gfrEnabled = true;
	default:
		break;
	}
}

void MainWindow::zoomIn() {
	m_zoomSlider->zoomIn();
}

void MainWindow::zoomOut() {
	m_zoomSlider->zoomOut();
}

void MainWindow::fitInWindow() {
	if (m_currentGraphicsView == nullptr) return;

	double newZoom = m_currentGraphicsView->fitInWindow();
	m_zoomSlider->setValue(newZoom);
}

void MainWindow::hundredPercentSize() {
	m_currentGraphicsView->absoluteZoom(100);
	m_zoomSlider->setValue(100);
}

void MainWindow::actualSize() {
	QMessageBox::information(this, tr("Actual Size"),
	                         tr("It doesn't seem to be possible to automatically determine the actual physical size of the monitor, so "
	                            "'actual size' as currently implemented is only a guess. "
	                            "Your best bet would be to drag out a ruler part, then place a real (physical) ruler on top and zoom until they match up."
	                           ));


	int dpi = this->physicalDpiX();
	int l = this->logicalDpiX();

	DebugDialog::debug(QString("actual size %1 %2").arg(dpi).arg(l));

	// remember the parameter to the next two functions is a percent
	m_currentGraphicsView->absoluteZoom(dpi * 100.0 / GraphicsUtils::SVGDPI);
	m_zoomSlider->setValue(dpi * 100.0 / GraphicsUtils::SVGDPI);
}

void MainWindow::showWelcomeView() {
	setCurrentTabIndex(0);
}

void MainWindow::showBreadboardView() {
	int ix = (m_welcomeView == nullptr) ? 0 : 1;
	setCurrentTabIndex(ix);
}

void MainWindow::showSchematicView() {
	int ix = (m_welcomeView == nullptr) ? 1 : 2;
	setCurrentTabIndex(ix);
}

void MainWindow::showPCBView() {
	int ix = (m_welcomeView == nullptr) ? 2 : 3;
	setCurrentTabIndex(ix);
}

void MainWindow::showProgramView() {
	int ix = (m_welcomeView == nullptr) ? 3 : 4;
	setCurrentTabIndex(ix);
}

void MainWindow::setCurrentView(ViewLayer::ViewID viewID)
{
	if (viewID == ViewLayer::BreadboardView) showBreadboardView();
	else if (viewID == ViewLayer::SchematicView) showSchematicView();
	else if (viewID == ViewLayer::PCBView) showPCBView();
}

void MainWindow::showPartsBinIconView() {
	if (m_binManager) m_binManager->toIconView();
}

void MainWindow::showPartsBinListView() {
	if (m_binManager) m_binManager->toListView();
}

void MainWindow::openHelp() {
	QDesktopServices::openUrl(QString("https://fritzing.org/learning/"));
}

void MainWindow::openExamples() {
	QDesktopServices::openUrl(QString("https://fritzing.org/projects/"));
}

void MainWindow::openPartsReference() {
	QDesktopServices::openUrl(QString("https://fritzing.org/parts/"));
}

void MainWindow::visitFritzingDotOrg() {
	QDesktopServices::openUrl(QString("https://fritzing.org"));
}

void MainWindow::reportBug() {
	QDesktopServices::openUrl(QString("https://github.com/fritzing/fritzing-app/issues"));
}

void MainWindow::partsEditorHelp() {
	QDir dir = FolderUtils::getApplicationSubFolder("help");
	QString path = dir.absoluteFilePath("parts_editor_help.html");
	if (QFileInfo(path).exists()) {
		QDesktopServices::openUrl(QString("file:///%1").arg(path));
	}
}



void MainWindow::enableDebug() {
	bool enabled = m_enableDebugAct->isChecked();
	DebugDialog::setEnabled(enabled);
	if (!m_windowMenu->actions().contains(m_toggleDebuggerOutputAct)) {
		m_windowMenu->insertSeparator(m_windowMenuSeparator);
		m_windowMenu->insertAction(m_windowMenuSeparator, m_toggleDebuggerOutputAct);
	}
	toggleDebuggerOutput(enabled);
}


void MainWindow::openNewPartsEditor(PaletteItem * paletteItem)
{
	Q_FOREACH (QWidget *widget, QApplication::topLevelWidgets()) {
		auto * peMainWindow = qobject_cast<PEMainWindow *>(widget);
		if (peMainWindow == nullptr) continue;

		if (peMainWindow->editsModuleID(paletteItem->moduleID())) {
			if (peMainWindow->isMinimized()) peMainWindow->showNormal();
			else peMainWindow->show();
			peMainWindow->raise();
			return;
		}
	}

	auto * peMainWindow = new PEMainWindow(m_referenceModel, nullptr);
	peMainWindow->init(m_referenceModel, false);
	if (peMainWindow->setInitialItem(paletteItem)) {
		peMainWindow->show();
		peMainWindow->raise();
		connect(peMainWindow, SIGNAL(addToMyPartsSignal(ModelPart *, const QStringList &)), this, SLOT(addToMyParts(ModelPart *, const QStringList &)));
	}
	else {
		delete peMainWindow;
	}
}

void MainWindow::getPartsEditorNewAnd(ItemBase * fromItem)
{
	auto * paletteItem = qobject_cast<PaletteItem *>(fromItem);
	if (paletteItem == nullptr) return;

	openNewPartsEditor(paletteItem);
}

void MainWindow::openInPartsEditorNew() {
	if (m_currentGraphicsView == nullptr) return;

	PaletteItem *selectedPart = m_currentGraphicsView->getSelectedPart();
	openNewPartsEditor(selectedPart);
}

void MainWindow::createNewSketch() {
	MainWindow* mw = newMainWindow(m_referenceModel, "", true, true, -1);
	mw->move(x()+CascadeFactorX,y()+CascadeFactorY);
	ProcessEventBlocker::processEvents();

	mw->addDefaultParts();
	mw->show();
	mw->hideTempPartsBin();

	QSettings settings;
	settings.remove("lastOpenSketch");
	mw->clearFileProgressDialog();
}

void MainWindow::minimize() {
	this->showMinimized();
}

void MainWindow::toggleToolbar(bool toggle) {
	Q_UNUSED(toggle);
	/*if(toggle) {
		this->m_fileToolBar->show();
		this->m_editToolBar->show();
	} else {
		this->m_fileToolBar->hide();
		this->m_editToolBar->hide();
	}*/
}

void MainWindow::togglePartLibrary(bool toggle) {
	if(toggle) {
		m_binManager->show();
	} else {
		m_binManager->hide();
	}
}

void MainWindow::toggleInfo(bool toggle) {
	if(toggle) {
		((QDockWidget*)m_infoView->parent())->show();
	} else {
		((QDockWidget*)m_infoView->parent())->hide();
	}
}

void MainWindow::toggleUndoHistory(bool toggle) {
	if(toggle) {
		((QDockWidget*)m_undoView->parent())->show();
	} else {
		((QDockWidget*)m_undoView->parent())->hide();
	}
}

void MainWindow::toggleDebuggerOutput(bool toggle) {
	if (toggle) {
		DebugDialog::showDebug();
	}
	else
	{
		DebugDialog::hideDebug();
	}
}

void MainWindow::updateWindowMenu() {
	m_toggleDebuggerOutputAct->setChecked(DebugDialog::visible());
	Q_FOREACH (QWidget * widget, QApplication::topLevelWidgets()) {
		auto * mainWindow = qobject_cast<MainWindow *>(widget);
		if (mainWindow == nullptr) continue;

		QAction *action = mainWindow->raiseWindowAction();
		if (action) {
			action->setChecked(action == m_raiseWindowAct);
			m_windowMenu->addAction(action);
		}
	}
}

void MainWindow::pageSetup() {
	notYetImplemented(tr("Page Setup"));
}

void MainWindow::notYetImplemented(QString action) {
	QMessageBox::warning(this, tr("Fritzing"),
	                     tr("Sorry, \"%1\" has not been implemented yet").arg(action));
}

void MainWindow::rotateIncCW() {
	if (m_currentGraphicsView == nullptr) return;

	if (m_rotate45cwAct->isEnabled()) {
		rotate45cw();
	}
	else if (m_rotate90cwAct->isEnabled()) {
		rotate90cw();
	}
}

void MainWindow::rotateIncCWRubberBand() {
	if (m_currentGraphicsView == nullptr) return;

	if (m_rotate45cwAct->isEnabled()) {
		m_currentGraphicsView->rotateX(45, true, nullptr);
	}
	else if (m_rotate90cwAct->isEnabled()) {
		m_currentGraphicsView->rotateX(90, true, nullptr);
	}
}

void MainWindow::rotateIncCCW() {
	if (m_currentGraphicsView == nullptr) return;

	if (m_rotate45ccwAct->isEnabled()) {
		m_currentGraphicsView->rotateX(315, true, nullptr);
	}
	else if (m_rotate90ccwAct->isEnabled()) {
		m_currentGraphicsView->rotateX(270, true, nullptr);
	}
}

void MainWindow::rotateIncCCWRubberBand() {
	if (m_currentGraphicsView == nullptr) return;

	if (m_rotate45ccwAct->isEnabled()) {
		m_currentGraphicsView->rotateX(315, true, nullptr);
	}
	else if (m_rotate90ccwAct->isEnabled()) {
		m_currentGraphicsView->rotateX(270, true, nullptr);
	}
}

void MainWindow::rotate90cw() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->rotateX(90, false, nullptr);
}

void MainWindow::rotate90ccw() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->rotateX(270, false, nullptr);
}

void MainWindow::rotate45ccw() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->rotateX(315, false, nullptr);
}

void MainWindow::rotate45cw() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->rotateX(45, false, nullptr);
}

void MainWindow::rotate180() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->rotateX(180, false, nullptr);
}

void MainWindow::flipHorizontal() {
	m_currentGraphicsView->flipX(Qt::Horizontal, false);
}

void MainWindow::flipVertical() {
	m_currentGraphicsView->flipX(Qt::Vertical, false);
}

void MainWindow::sendToBack() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->sendToBack();
}

void MainWindow::sendBackward() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->sendBackward();
}

void MainWindow::bringForward() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->bringForward();
}

void MainWindow::bringToFront() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->bringToFront();
}

void MainWindow::alignLeft() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->alignItems(Qt::AlignLeft);
}

void MainWindow::alignVerticalCenter() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->alignItems(Qt::AlignVCenter);
}

void MainWindow::alignRight() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->alignItems(Qt::AlignRight);
}

void MainWindow::alignTop() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->alignItems(Qt::AlignTop);
}

void MainWindow::alignHorizontalCenter() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->alignItems(Qt::AlignHCenter);
}

void MainWindow::alignBottom() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->alignItems(Qt::AlignBottom);
}

void MainWindow::showAllLayers() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->setAllLayersVisible(true);
	updateLayerMenu();
}

void MainWindow::hideAllLayers() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->setAllLayersVisible(false);
	updateLayerMenu();
}

void MainWindow::openURL() {
	auto *action = qobject_cast<QAction *>(sender());
	if (action == nullptr) return;

	QString href = action->data().toString();
	if (href.isEmpty()) return;

	QDesktopServices::openUrl(href);
}

void MainWindow::openRecentOrExampleFile() {
	auto *action = qobject_cast<QAction *>(sender());
	if (action) {
		openRecentOrExampleFile(action->data().toString(), action->text());
	}
}

void MainWindow::openRecentOrExampleFile(const QString & filename, const QString & actionText) {
	if (alreadyOpen(filename)) {
		return;
	}

	if (!QFileInfo(filename).exists()) {
		QMessageBox::warning(nullptr, tr("Fritzing"), tr("File '%1' not found").arg(filename));
		return;
	}

	MainWindow* mw = newMainWindow(m_referenceModel, filename, true, true, -1);
	bool readOnly = m_openExampleActions.contains(actionText);
	mw->setReadOnly(readOnly);
	mw->loadWhich(filename, !readOnly,!readOnly,!readOnly, "");
	mw->clearFileProgressDialog();
	closeIfEmptySketch(mw);
}

void MainWindow::removeActionsStartingAt(QMenu * menu, int start) {
	QList<QAction*> actions = menu->actions();

	if(start == 0) {
		menu->clear();
	} else {
		for(int i=start; i < actions.size(); i++) {
			menu->removeAction(actions.at(i));
		}
	}
}

void MainWindow::hideShowProgramMenu() {
	if (m_currentWidget == nullptr) return;

	bool show = m_programView == nullptr || m_currentWidget->contentView() != m_programView;
	//if (m_fileMenu) m_fileMenu->menuAction()->setVisible(show);
	if (m_viewMenu) {
		m_viewMenu->menuAction()->setVisible(show);
		m_viewMenu->setEnabled(show);
	if (show) {
		m_viewMenu->setTitle(tr("&View"));
	} else {
		m_viewMenu->setTitle(tr("View"));
	}
	}
	if (m_partMenu) {
		m_partMenu->menuAction()->setVisible(show);
		m_partMenu->setEnabled(show);
	}
	if (m_editMenu) {
		m_editMenu->menuAction()->setVisible(show);
		m_editMenu->setEnabled(show);
		m_copyAct->setEnabled(show);
		m_cutAct->setEnabled(show);
		m_selectAllAct->setEnabled(show);
		m_undoAct->setEnabled(show);
		m_redoAct->setEnabled(show);
	if (show) {
		m_editMenu->setTitle(tr("&Edit"));
	} else {
		m_editMenu->setTitle(tr("Edit"));
	}
	}
	if (m_programView) m_programView->showMenus(!show);
}

void MainWindow::hideShowTraceMenu() {
	if (m_pcbTraceMenu) {
		m_pcbTraceMenu->menuAction()->setVisible(m_currentGraphicsView == m_pcbGraphicsView);
		if(m_currentGraphicsView == m_pcbGraphicsView) {
			m_pcbTraceMenu->setTitle(tr("&Routing"));
		} else {
			m_pcbTraceMenu->setTitle(tr("Routing"));
		}
	}
	if (m_schematicTraceMenu) {
		m_schematicTraceMenu->menuAction()->setVisible(m_currentGraphicsView == m_schematicGraphicsView);
		if(m_currentGraphicsView == m_schematicGraphicsView) {
			m_schematicTraceMenu->setTitle(tr("&Routing"));
		} else {
			m_schematicTraceMenu->setTitle(tr("Routing"));
		}
	}
	if (m_breadboardTraceMenu) {
		m_breadboardTraceMenu->menuAction()->setVisible(m_currentGraphicsView == m_breadboardGraphicsView);
		if(m_currentGraphicsView == m_breadboardGraphicsView) {
			m_breadboardTraceMenu->setTitle(tr("&Routing"));
		} else {
			m_breadboardTraceMenu->setTitle(tr("Routing"));
		}
	}
}

void MainWindow::createTraceMenuActions() {
	m_newAutorouteAct = new QAction(tr("Autoroute"), this);
	m_newAutorouteAct->setStatusTip(tr("Autoroute connections..."));
	m_newAutorouteAct->setShortcut(tr("Shift+Ctrl+A"));
	connect(m_newAutorouteAct, SIGNAL(triggered()), this, SLOT(newAutoroute()));

	createOrderFabAct();
	createActiveLayerActions();

	auto * traceAct = new QAction(tr("&Create trace from ratsnest"), this);
	traceAct->setStatusTip(tr("Create a trace from the ratsnest line"));
	m_createTraceWireAct = new WireAction(traceAct);
	connect(m_createTraceWireAct, SIGNAL(triggered()), this, SLOT(createTrace()));
	traceAct = new QAction(tr("&Create wire from ratsnest"), this);
	traceAct->setStatusTip(tr("Create a wire from the ratsnest line"));
	m_createWireWireAct = new WireAction(traceAct);
	connect(m_createWireWireAct, SIGNAL(triggered()), this, SLOT(createTrace()));

	m_excludeFromAutorouteAct = new QAction(tr("Do not autoroute"), this);
	m_excludeFromAutorouteAct->setStatusTip(tr("When autorouting, do not rip up this trace wire, via, or jumper item"));
	connect(m_excludeFromAutorouteAct, SIGNAL(triggered()), this, SLOT(excludeFromAutoroute()));
	m_excludeFromAutorouteAct->setCheckable(true);
	m_excludeFromAutorouteWireAct = new WireAction(m_excludeFromAutorouteAct);
	connect(m_excludeFromAutorouteWireAct, SIGNAL(triggered()), this, SLOT(excludeFromAutoroute()));

	m_changeTraceLayerAct = new QAction(tr("Move to other side of the board"), this);
	m_changeTraceLayerAct->setStatusTip(tr("Move selected traces to the other side of the board (note: the 'first' trace will be moved and the rest will follow to the same side)"));
	connect(m_changeTraceLayerAct, SIGNAL(triggered()), this, SLOT(changeTraceLayer()));
	m_changeTraceLayerWireAct = new WireAction(m_changeTraceLayerAct);
	connect(m_changeTraceLayerWireAct, SIGNAL(triggered()), this, SLOT(changeTraceLayer()));

	m_showUnroutedAct = new QAction(tr("Show unrouted"), this);
	m_showUnroutedAct->setStatusTip(tr("Highlight all unrouted connectors"));
	connect(m_showUnroutedAct, SIGNAL(triggered()), this, SLOT(showUnrouted()));

	m_selectAllTracesAct = new QAction(tr("Select All Traces"), this);
	m_selectAllTracesAct->setStatusTip(tr("Select all trace wires"));
	connect(m_selectAllTracesAct, SIGNAL(triggered()), this, SLOT(selectAllTraces()));

	m_selectAllWiresAct = new QAction(tr("Select All Wires"), this);
	m_selectAllWiresAct->setStatusTip(tr("Select all wires"));
	connect(m_selectAllWiresAct, SIGNAL(triggered()), this, SLOT(selectAllTraces()));

	m_selectAllCopperFillAct = new QAction(tr("Select All CopperFill"), this);
	m_selectAllCopperFillAct->setStatusTip(tr("Select all copper fill items"));
	connect(m_selectAllCopperFillAct, SIGNAL(triggered()), this, SLOT(selectAllCopperFill()));

	m_updateRoutingStatusAct = new QAction(tr("Force Update Routing Status and Ratsnests"), this);
	m_updateRoutingStatusAct->setStatusTip(tr("Recalculate routing status and ratsnest wires (in case the auto-update isn't working correctly)"));
	connect(m_updateRoutingStatusAct, SIGNAL(triggered()), this, SLOT(updateRoutingStatus()));

	m_selectAllExcludedTracesAct = new QAction(tr("Select All \"Don't Autoroute\" Traces"), this);
	m_selectAllExcludedTracesAct->setStatusTip(tr("Select all trace wires excluded from autorouting"));
	connect(m_selectAllExcludedTracesAct, SIGNAL(triggered()), this, SLOT(selectAllExcludedTraces()));

	m_selectAllIncludedTracesAct = new QAction(tr("Select All Autoroutable Traces"), this);
	m_selectAllIncludedTracesAct->setStatusTip(tr("Select all trace wires that can be changed during autorouting"));
	connect(m_selectAllIncludedTracesAct, SIGNAL(triggered()), this, SLOT(selectAllIncludedTraces()));

	m_selectAllJumperItemsAct = new QAction(tr("Select All Jumpers"), this);
	m_selectAllJumperItemsAct->setStatusTip(tr("Select all jumper item parts"));
	connect(m_selectAllJumperItemsAct, SIGNAL(triggered()), this, SLOT(selectAllJumperItems()));

	m_selectAllViasAct = new QAction(tr("Select All Vias"), this);
	m_selectAllViasAct->setStatusTip(tr("Select all via parts"));
	connect(m_selectAllViasAct, SIGNAL(triggered()), this, SLOT(selectAllVias()));

	m_tidyWiresAct = new QAction(tr("Tidy Wires"), this);
	m_tidyWiresAct->setStatusTip(tr("Tidy selected wires"));
	connect(m_tidyWiresAct, SIGNAL(triggered()), this, SLOT(tidyWires()));

	m_groundFillAct = new QAction(tr("Ground Fill"), this);
	m_groundFillAct->setStatusTip(tr("Fill empty regions of the copper layer--fill will include all traces connected to a GROUND"));
	connect(m_groundFillAct, SIGNAL(triggered()), this, SLOT(groundFill()));

	m_groundFillOldAct = new QAction(tr("Old Ground Fill"), this);
	m_groundFillOldAct->setStatusTip(tr("Fill empty regions of the copper layer--fill will include all traces connected to a GROUND"));
	connect(m_groundFillOldAct, SIGNAL(triggered()), this, SLOT(groundFillOld()));

	m_copperFillAct = new QAction(tr("Copper Fill"), this);
	m_copperFillAct->setStatusTip(tr("Fill empty regions of the copper layer--not including traces connected to a GROUND"));
	connect(m_copperFillAct, SIGNAL(triggered()), this, SLOT(copperFill()));

	m_copperFillOldAct = new QAction(tr("Old Copper Fill"), this);
	m_copperFillOldAct->setStatusTip(tr("Fill empty regions of the copper layer--not including traces connected to a GROUND"));
	connect(m_copperFillOldAct, SIGNAL(triggered()), this, SLOT(copperFillOld()));

	m_removeGroundFillAct = new QAction(tr("Remove Copper Fill"), this);
	m_removeGroundFillAct->setStatusTip(tr("Remove the copper fill"));
	connect(m_removeGroundFillAct, SIGNAL(triggered()), this, SLOT(removeGroundFill()));

	m_setGroundFillSeedsAct = new QAction(tr("Choose Ground Fill Seed(s)..."), this);
	m_setGroundFillSeedsAct->setStatusTip(tr("Fill empty regions of the copper layer--fill will include all traces connected to the seeds"));
	connect(m_setGroundFillSeedsAct, SIGNAL(triggered()), this, SLOT(setGroundFillSeeds()));

	m_setOneGroundFillSeedAct = new ConnectorItemAction(tr("Set Ground Fill Seed"), this);
	m_setOneGroundFillSeedAct->setStatusTip(tr("Treat this connector and its connections as a 'ground' during ground fill."));
	m_setOneGroundFillSeedAct->setCheckable(true);
	connect(m_setOneGroundFillSeedAct, SIGNAL(triggered()), this, SLOT(setOneGroundFillSeed()));

	m_clearGroundFillSeedsAct = new ConnectorItemAction(tr("Clear Ground Fill Seeds"), this);
	m_clearGroundFillSeedsAct->setStatusTip(tr("Clear ground fill seeds--enable copper fill only."));
	connect(m_clearGroundFillSeedsAct, SIGNAL(triggered()), this, SLOT(clearGroundFillSeeds()));

	m_setGroundFillKeepoutAct = new QAction(tr("Set Ground Fill Keepout..."), this);
	m_setGroundFillKeepoutAct->setStatusTip(tr("Set the minimum distance between ground fill and traces or connectors"));
	connect(m_setGroundFillKeepoutAct, SIGNAL(triggered()), this, SLOT(setGroundFillKeepout()));

	m_newDesignRulesCheckAct = new QAction(tr("Design Rules Check (DRC)"), this);
	m_newDesignRulesCheckAct->setStatusTip(tr("Highlights any parts that are too close together for safe board production"));
	m_newDesignRulesCheckAct->setShortcut(tr("Shift+Ctrl+D"));
	connect(m_newDesignRulesCheckAct, SIGNAL(triggered()), this, SLOT(newDesignRulesCheck()));

	m_autorouterSettingsAct = new QAction(tr("Autorouter/DRC settings..."), this);
	m_autorouterSettingsAct->setStatusTip(tr("Set autorouting parameters including keepout..."));
	connect(m_autorouterSettingsAct, SIGNAL(triggered()), this, SLOT(autorouterSettings()));

	m_fabQuoteAct = new QAction(tr("Fritzing Fab Quote..."), this);
	m_fabQuoteAct->setStatusTip(tr("How much would it cost to produce a PCB from this sketch with Fritzing Fab"));
	connect(m_fabQuoteAct, SIGNAL(triggered()), this, SLOT(fabQuote()));

}

void MainWindow::createActiveLayerActions() {

	m_viewFromBelowToggleAct = new QAction(tr("View from below"), this);
	m_viewFromBelowToggleAct->setStatusTip(tr("View the PCB from the bottom layers upwards"));
	m_viewFromBelowToggleAct->setCheckable(true);
	m_viewFromBelowToggleAct->setChecked(false);
	connect(m_viewFromBelowToggleAct, SIGNAL(triggered()), this, SLOT(setViewFromBelowToggle()));

	m_viewFromBelowAct = new QAction(tr("View from below"), this);
	m_viewFromBelowAct->setStatusTip(tr("View the PCB from the bottom layers upwards"));
	m_viewFromBelowAct->setCheckable(true);
	connect(m_viewFromBelowAct, SIGNAL(triggered()), this, SLOT(setViewFromBelow()));

	m_viewFromAboveAct = new QAction(tr("View from above"), this);
	m_viewFromAboveAct->setStatusTip(tr("View the PCB from the top layers downwards"));
	m_viewFromAboveAct->setCheckable(true);
	connect(m_viewFromAboveAct, SIGNAL(triggered()), this, SLOT(setViewFromAbove()));

	m_activeLayerBothAct = new QAction(tr("Set both copper layers clickable"), this);
	m_activeLayerBothAct->setStatusTip(tr("Set both copper layers clickable"));
	m_activeLayerBothAct->setShortcut(tr("Shift+Ctrl+3"));
	m_activeLayerBothAct->setCheckable(true);
	connect(m_activeLayerBothAct, SIGNAL(triggered()), this, SLOT(activeLayerBoth()));

	m_activeLayerTopAct = new QAction(tr("Set copper top layer clickable"), this);
	m_activeLayerTopAct->setStatusTip(tr("Set copper top layer clickable"));
	m_activeLayerTopAct->setShortcut(tr("Shift+Ctrl+2"));
	m_activeLayerTopAct->setCheckable(true);
	connect(m_activeLayerTopAct, SIGNAL(triggered()), this, SLOT(activeLayerTop()));

	m_activeLayerBottomAct = new QAction(tr("Set copper bottom layer clickable"), this);
	m_activeLayerBottomAct->setStatusTip(tr("Set copper bottom layer clickable"));
	m_activeLayerBottomAct->setShortcut(tr("Shift+Ctrl+1"));
	m_activeLayerBottomAct->setCheckable(true);
	connect(m_activeLayerBottomAct, SIGNAL(triggered()), this, SLOT(activeLayerBottom()));
}

void MainWindow::activeLayerBoth() {
	auto * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == nullptr) return;

	pcbSketchWidget->setLayerActive(ViewLayer::Copper1, true);
	pcbSketchWidget->setLayerActive(ViewLayer::Copper0, true);
	pcbSketchWidget->setLayerActive(ViewLayer::Silkscreen0, true);
	pcbSketchWidget->setLayerActive(ViewLayer::Silkscreen1, true);
	AutoCloseMessageBox::showMessage(this, tr("Copper Top and Copper Bottom layers are both active"));
	updateActiveLayerButtons();
}

void MainWindow::activeLayerTop() {
	auto * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == nullptr) return;

	pcbSketchWidget->setLayerActive(ViewLayer::Copper1, true);
	pcbSketchWidget->setLayerActive(ViewLayer::Silkscreen1, true);
	pcbSketchWidget->setLayerActive(ViewLayer::Copper0, false);
	pcbSketchWidget->setLayerActive(ViewLayer::Silkscreen0, false);
	AutoCloseMessageBox::showMessage(this, tr("Copper Top layer is active"));
	updateActiveLayerButtons();
}

void MainWindow::activeLayerBottom() {
	auto * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == nullptr) return;

	pcbSketchWidget->setLayerActive(ViewLayer::Copper1, false);
	pcbSketchWidget->setLayerActive(ViewLayer::Silkscreen1, false);
	pcbSketchWidget->setLayerActive(ViewLayer::Copper0, true);
	pcbSketchWidget->setLayerActive(ViewLayer::Silkscreen0, true);
	AutoCloseMessageBox::showMessage(this, tr("Copper Bottom layer is active"));
	updateActiveLayerButtons();
}

void MainWindow::toggleActiveLayer()
{
	auto * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == nullptr) return;

	int index = activeLayerIndex();
	switch (index) {
	case 0:
		activeLayerBottom();
		return;
	case 1:
		activeLayerTop();
		return;
	case 2:
		activeLayerBoth();
		return;
	default:
		return;
	}
}


void MainWindow::createOrderFabAct() {
	if (m_orderFabAct) return;

	m_orderFabAct = new QAction(tr("Order a PCB..."), this);
	m_orderFabAct->setStatusTip(tr("Order a PCB created from your sketch--from fabulous Fritzing Fab"));
	connect(m_orderFabAct, SIGNAL(triggered()), this, SLOT(orderFab()));
}


void MainWindow::newAutoroute() {
	auto * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == nullptr) return;

	ItemBase * board = nullptr;
	if (pcbSketchWidget->autorouteTypePCB()) {
		int boardCount;
		board = pcbSketchWidget->findSelectedBoard(boardCount);
		if (boardCount == 0) {
			QMessageBox::critical(this, tr("Fritzing"),
			                      tr("Your sketch does not have a board yet!  Please add a PCB in order to use the autorouter."));
			return;
		}
		if (board == nullptr) {
			QMessageBox::critical(this, tr("Fritzing"),
			                      tr("Please select the board you want to autoroute. The autorouter can only handle one board at a time."));
			return;
		}
	}

	dynamic_cast<SketchAreaWidget *>(pcbSketchWidget->parent())->routingStatusLabel()->setText(tr("Autorouting..."));

	bool copper0Active = pcbSketchWidget->layerIsActive(ViewLayer::Copper0);
	bool copper1Active = pcbSketchWidget->layerIsActive(ViewLayer::Copper1);

	AutorouteProgressDialog progress(tr("Autorouting Progress..."), true, true, true, true, pcbSketchWidget, this);
	progress.setModal(true);
	progress.show();
	QRect pr = progress.frameGeometry();
	QRect wr = this->frameGeometry();
	progress.move(wr.right() - pr.width(), pr.top());


	pcbSketchWidget->scene()->clearSelection();
	pcbSketchWidget->setIgnoreSelectionChangeEvents(true);
	Autorouter * autorouter = nullptr;
	autorouter = new MazeRouter(pcbSketchWidget, board, true);

	connect(autorouter, SIGNAL(wantTopVisible()), this, SLOT(activeLayerTop()), Qt::DirectConnection);
	connect(autorouter, SIGNAL(wantBottomVisible()), this, SLOT(activeLayerBottom()), Qt::DirectConnection);
	connect(autorouter, SIGNAL(wantBothVisible()), this, SLOT(activeLayerBoth()), Qt::DirectConnection);

	connect(&progress, SIGNAL(cancel()), autorouter, SLOT(cancel()), Qt::DirectConnection);
	connect(&progress, SIGNAL(skip()), autorouter, SLOT(cancelTrace()), Qt::DirectConnection);
	connect(&progress, SIGNAL(stop()), autorouter, SLOT(stopTracing()), Qt::DirectConnection);
	connect(&progress, SIGNAL(best()), autorouter, SLOT(useBest()), Qt::DirectConnection);
	connect(&progress, SIGNAL(spinChange(int)), autorouter, SLOT(setMaxCycles(int)), Qt::DirectConnection);

	connect(autorouter, SIGNAL(setMaximumProgress(int)), &progress, SLOT(setMaximum(int)), Qt::DirectConnection);
	connect(autorouter, SIGNAL(setProgressValue(int)), &progress, SLOT(setValue(int)), Qt::DirectConnection);
	connect(autorouter, SIGNAL(setProgressMessage(const QString &)), &progress, SLOT(setMessage(const QString &)));
	connect(autorouter, SIGNAL(setProgressMessage2(const QString &)), &progress, SLOT(setMessage2(const QString &)));
	connect(autorouter, SIGNAL(setCycleMessage(const QString &)), &progress, SLOT(setSpinLabel(const QString &)));
	connect(autorouter, SIGNAL(setCycleCount(int)), &progress, SLOT(setSpinValue(int)));
	connect(autorouter, SIGNAL(disableButtons()), &progress, SLOT(disableButtons()));

	ProcessEventBlocker::processEvents();
	ProcessEventBlocker::block();

	autorouter->start();
	pcbSketchWidget->setIgnoreSelectionChangeEvents(false);

	delete autorouter;

	pcbSketchWidget->setLayerActive(ViewLayer::Copper1, copper1Active);
	pcbSketchWidget->setLayerActive(ViewLayer::Silkscreen1, copper1Active);
	pcbSketchWidget->setLayerActive(ViewLayer::Copper0, copper0Active);
	pcbSketchWidget->setLayerActive(ViewLayer::Silkscreen0, copper0Active);
	updateActiveLayerButtons();
	ProcessEventBlocker::unblock();
	RoutingStatus routingStatus;
	routingStatus.zero();
	Q_EMIT pcbSketchWidget->routingStatusSignal(pcbSketchWidget, routingStatus);
}

void MainWindow::createTrace() {
	m_currentGraphicsView->createTrace(retrieveWire(), true);
}

void MainWindow::excludeFromAutoroute() {
	Wire * wire = retrieveWire();
	auto * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == nullptr) return;

	pcbSketchWidget->excludeFromAutoroute(wire == nullptr ? m_excludeFromAutorouteAct->isChecked() : m_excludeFromAutorouteWireAct->isChecked());
}

void MainWindow::selectAllTraces()
{
	m_currentGraphicsView->selectAllWires(m_currentGraphicsView->getTraceFlag());
}

void MainWindow::updateRoutingStatus() {
	RoutingStatus routingStatus;
	routingStatus.zero();
	m_currentGraphicsView->updateRoutingStatus(nullptr, routingStatus, true);
}

void MainWindow::selectAllExcludedTraces() {
	auto * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == nullptr) return;

	pcbSketchWidget->selectAllExcludedTraces();
}

void MainWindow::selectAllIncludedTraces() {
	auto * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == nullptr) return;

	pcbSketchWidget->selectAllIncludedTraces();
}

void MainWindow::selectAllJumperItems() {
	m_pcbGraphicsView->selectAllItemType(ModelPart::Jumper, tr("jumpers"));
}

void MainWindow::selectAllCopperFill() {
	m_pcbGraphicsView->selectAllItemType(ModelPart::CopperFill, tr("copperfill"));
}

void MainWindow::selectAllVias() {
	m_pcbGraphicsView->selectAllItemType(ModelPart::Via, tr("vias"));
}

void MainWindow::notClosableForAWhile() {
	m_dontClose = true;

	QTimer::singleShot(500, this, SLOT(ensureClosable()));
}

void MainWindow::ensureClosable() {
	m_dontClose = false;
}

void MainWindow::showPartLabels() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->showPartLabels(m_showPartLabelAct->data().toBool());
}

void MainWindow::addNote() {
	if (m_currentGraphicsView == nullptr) return;

	ViewGeometry vg;
	vg.setRect(0, 0, Note::initialMinWidth, Note::initialMinHeight);
	QPointF tl = m_currentGraphicsView->mapToScene(QPoint(0, 0));
	QSizeF vpSize = m_currentGraphicsView->viewport()->size();
	tl.setX(tl.x() + ((vpSize.width() - Note::initialMinWidth) / 2.0));
	tl.setY(tl.y() + ((vpSize.height() - Note::initialMinHeight) / 2.0));
	vg.setLoc(tl);

	auto * parentCommand = new QUndoCommand(tr("Add Note"));
	m_currentGraphicsView->stackSelectionState(false, parentCommand);
	m_currentGraphicsView->scene()->clearSelection();
	new AddItemCommand(m_currentGraphicsView, BaseCommand::SingleView, ModuleIDNames::NoteModuleIDName, m_currentGraphicsView->defaultViewLayerPlacement(nullptr), vg, ItemBase::getNextID(), false, -1, parentCommand);
	m_undoStack->push(parentCommand);
}

bool MainWindow::alreadyOpen(const QString & fileName) {
	Q_FOREACH (QWidget * widget, QApplication::topLevelWidgets()) {
		auto * mainWindow = qobject_cast<MainWindow *>(widget);
		if (mainWindow == nullptr) continue;

		// don't load two copies of the same file
		if (mainWindow->fileName().compare(fileName) == 0) {
			mainWindow->raise();
			return true;
		}
	}

	return false;
}

void MainWindow::enableAddBendpointAct(QGraphicsItem * graphicsItem) {
	m_addBendpointAct->setEnabled(false);
	m_convertToViaAct->setEnabled(false);
	m_flattenCurveAct->setEnabled(false);

	Wire * wire = dynamic_cast<Wire *>(graphicsItem);
	if (wire == nullptr) return;
	if (wire->getRatsnest()) return;

	m_flattenCurveAct->setEnabled(wire->isCurved());

	auto * bendpointAction = qobject_cast<BendpointAction *>(m_addBendpointAct);
	auto * convertToViaAction = qobject_cast<BendpointAction *>(m_convertToViaAct);
	auto * scene = qobject_cast<FGraphicsScene *>(graphicsItem->scene());
	if (scene) {
		bendpointAction->setLastLocation(scene->lastContextMenuPos());
		convertToViaAction->setLastLocation(scene->lastContextMenuPos());
	}

	bool enabled = false;
	bool ctvEnabled = false;
	if (m_currentGraphicsView->lastHoverEnterConnectorItem()) {
		bendpointAction->setText(tr("Remove Bendpoint"));
		bendpointAction->setLastHoverEnterConnectorItem(m_currentGraphicsView->lastHoverEnterConnectorItem());
		bendpointAction->setLastHoverEnterItem(nullptr);
		convertToViaAction->setLastHoverEnterConnectorItem(m_currentGraphicsView->lastHoverEnterConnectorItem());
		convertToViaAction->setLastHoverEnterItem(nullptr);
		ctvEnabled = enabled = true;
	}
	else if (m_currentGraphicsView->lastHoverEnterItem()) {
		bendpointAction->setText(tr("Add Bendpoint"));
		bendpointAction->setLastHoverEnterItem(m_currentGraphicsView->lastHoverEnterItem());
		bendpointAction->setLastHoverEnterConnectorItem(nullptr);
		convertToViaAction->setLastHoverEnterItem(nullptr);
		convertToViaAction->setLastHoverEnterConnectorItem(nullptr);
		enabled = true;
	}
	else {
		bendpointAction->setLastHoverEnterItem(nullptr);
		bendpointAction->setLastHoverEnterConnectorItem(nullptr);
		convertToViaAction->setLastHoverEnterItem(nullptr);
		convertToViaAction->setLastHoverEnterConnectorItem(nullptr);
	}

	m_addBendpointAct->setEnabled(enabled);
	m_convertToViaAct->setEnabled(ctvEnabled && (m_currentGraphicsView == m_pcbGraphicsView));
}

void MainWindow::addBendpoint()
{
	auto * bendpointAction = qobject_cast<BendpointAction *>(m_addBendpointAct);

	m_currentGraphicsView->addBendpoint(bendpointAction->lastHoverEnterItem(),
	                                    bendpointAction->lastHoverEnterConnectorItem(),
	                                    bendpointAction->lastLocation());
}

void MainWindow::convertToVia()
{
	auto * bendpointAction = qobject_cast<BendpointAction *>(m_convertToViaAct);

	m_pcbGraphicsView->convertToVia(bendpointAction->lastHoverEnterConnectorItem());
}

void MainWindow::convertToBendpoint()
{
	m_pcbGraphicsView->convertToBendpoint();
}

void MainWindow::flattenCurve()
{
	auto * bendpointAction = qobject_cast<BendpointAction *>(m_addBendpointAct);

	m_currentGraphicsView->flattenCurve(bendpointAction->lastHoverEnterItem(),
	                                    bendpointAction->lastHoverEnterConnectorItem(),
	                                    bendpointAction->lastLocation());
}

void MainWindow::tidyWires()
{
#ifndef QT_NO_DEBUG
	m_debugConnectors->onRepairErrors();
#endif
}

void MainWindow::copperFill() {
	groundFillAux2(false, false);
}

void MainWindow::groundFill()
{
	groundFillAux2(true, false);
}

void MainWindow::copperFillOld() {
	groundFillAux2(false, true);
}

void MainWindow::groundFillOld()
{
	groundFillAux2(true, true);
}

void MainWindow::groundFillAux2(bool fillGroundTraces, bool useOldVersion) {

	if (m_pcbGraphicsView->layerIsActive(ViewLayer::Copper0) && m_pcbGraphicsView->layerIsActive(ViewLayer::Copper1)) {
		groundFillAux(fillGroundTraces, ViewLayer::UnknownLayer, useOldVersion);
	}
	else if (m_pcbGraphicsView->layerIsActive(ViewLayer::Copper0)) {
		groundFillAux(fillGroundTraces, ViewLayer::GroundPlane0, useOldVersion);
	}
	else {
		groundFillAux(fillGroundTraces, ViewLayer::GroundPlane1, useOldVersion);
	}
}

void MainWindow::groundFillAux(bool fillGroundTraces, ViewLayer::ViewLayerID viewLayerID, bool useOldVersion)
{
	// TODO:
	//		what about leftover temp files from crashes?
	//		clear ground plane when anything changes
	//		some polygons can be combined
	//		remove old ground plane modules from paletteModel and database

	if (m_pcbGraphicsView == nullptr) return;

	int boardCount;
	ItemBase * board = m_pcbGraphicsView->findSelectedBoard(boardCount);
	if (boardCount == 0) {
		QMessageBox::critical(this, tr("Fritzing"),
		                      tr("Your sketch does not have a board yet!  Please add a PCB in order to use ground or copper fill."));
		return;
	}
	if (board == nullptr) {
		QMessageBox::critical(this, tr("Fritzing"),
		                      tr("Please select a PCB--copper fill only works for one board at a time."));
		return;
	}


	FileProgressDialog fileProgress(tr("Generating %1 fill...").arg(fillGroundTraces ? tr("ground") : tr("copper")), 0, this);
	fileProgress.setIndeterminate();
	auto * parentCommand = new QUndoCommand(fillGroundTraces ? tr("Ground Fill") : tr("Copper Fill"));
	m_pcbGraphicsView->blockUI(true);
	removeGroundFill(viewLayerID, parentCommand);
	bool success = false;
	if (useOldVersion) {
		success = m_pcbGraphicsView->groundFillOld(fillGroundTraces, viewLayerID, parentCommand);
	} else {
		success = m_pcbGraphicsView->groundFill(fillGroundTraces, viewLayerID, parentCommand);
	}
	if (success) {
		m_undoStack->push(parentCommand);
	}
	else {
		delete parentCommand;
	}
	m_pcbGraphicsView->blockUI(false);
}

void MainWindow::removeGroundFill() {
	removeGroundFill(ViewLayer::UnknownLayer, nullptr);
}

void MainWindow::removeGroundFill(ViewLayer::ViewLayerID viewLayerID, QUndoCommand * parentCommand) {
	QSet<ItemBase *> toDelete;
	int boardCount;
	ItemBase * board = m_pcbGraphicsView->findSelectedBoard(boardCount);
	if (boardCount == 0) {
		QMessageBox::critical(this, tr("Fritzing"),
		                      tr("Your sketch does not have a board yet!  Please add a PCB in order to remove copper fill."));
		return;
	}
	if (board == nullptr) {
		QMessageBox::critical(this, tr("Fritzing"),
		                      tr("Please select a PCB--ground fill operations only work on a one board at a time."));
		return;
	}

	Q_FOREACH (QGraphicsItem * item, m_pcbGraphicsView->scene()->collidingItems(board)) {
		auto * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == nullptr) continue;
		if (itemBase->moveLock()) continue;
		if (!isGroundFill(itemBase)) continue;

		if (viewLayerID != ViewLayer::UnknownLayer) {
			if (itemBase->viewLayerID() != viewLayerID) continue;
		}

		toDelete.insert(itemBase->layerKinChief());
	}

	if (toDelete.count() == 0) return;


	bool push = (parentCommand == nullptr);

	if (push) {
		parentCommand = new QUndoCommand(tr("Remove copper fill"));
	}

	new CleanUpWiresCommand(m_pcbGraphicsView, CleanUpWiresCommand::UndoOnly, parentCommand);
	new CleanUpRatsnestsCommand(m_pcbGraphicsView, CleanUpWiresCommand::UndoOnly, parentCommand);

	m_pcbGraphicsView->deleteMiddle(toDelete, parentCommand);
	Q_FOREACH (ItemBase * itemBase, toDelete) {
		itemBase->saveGeometry();
		m_pcbGraphicsView->makeDeleteItemCommand(itemBase, BaseCommand::CrossView, parentCommand);
	}

	new CleanUpRatsnestsCommand(m_pcbGraphicsView, CleanUpWiresCommand::RedoOnly, parentCommand);
	new CleanUpWiresCommand(m_pcbGraphicsView, CleanUpWiresCommand::RedoOnly, parentCommand);

	if (push) {
		m_undoStack->push(parentCommand);
	}
	else {
		Q_FOREACH (ItemBase * itemBase, toDelete) {
			// move them out of the way because they are about to be deleted anyhow
			itemBase->setPos(itemBase->pos() + board->sceneBoundingRect().bottomRight() + QPointF(10000, 10000));
		}
	}
}

bool MainWindow::isGroundFill(ItemBase * itemBase) {
	return (itemBase->itemType() == ModelPart::CopperFill);
}


QMenu *MainWindow::breadboardItemMenu() {
	auto *menu = new QMenu(QObject::tr("Part"), this);
	createRotateSubmenu(menu);
	viewItemMenuAux(menu);
	auto * probe = new FProbeActions("BreadboardItem", menu);
	return menu;
}

QMenu *MainWindow::schematicItemMenu() {
	auto *menu = new QMenu(QObject::tr("Part"), this);
	createRotateSubmenu(menu);
	menu->addAction(m_flipHorizontalAct);
	menu->addAction(m_flipVerticalAct);
	viewItemMenuAux(menu);
	auto * probe = new FProbeActions("SchematicItem", menu);
	return menu;

}

QMenu *MainWindow::pcbItemMenu() {
	auto *menu = new QMenu(QObject::tr("Part"), this);
	createRotateSubmenu(menu);
	menu = viewItemMenuAux(menu);
	menu->addAction(m_hidePartSilkscreenAct);
	menu->addSeparator();
	menu->addAction(m_convertToBendpointAct);
	m_convertToBendpointSeparator = menu->addSeparator();
	menu->addAction(m_setOneGroundFillSeedAct);
	menu->addAction(m_clearGroundFillSeedsAct);
	auto * probe = new FProbeActions("PCBItem", menu);
	return menu;
}

QMenu *MainWindow::breadboardWireMenu() {
	auto *menu = new QMenu(QObject::tr("Wire"), this);
//   createZOrderWireSubmenu(menu);
	createZOrderSubmenu(menu);
	menu->addSeparator();
	m_breadboardWireColorMenu = menu->addMenu(tr("&Wire Color"));
	Q_FOREACH(QString colorName, Wire::colorNames) {
		QString colorValue = Wire::colorTrans.value(colorName);
		auto * action = new QAction(colorName, this);
		m_breadboardWireColorMenu->addAction(action);
		action->setData(colorValue);
		action->setCheckable(true);
		action->setChecked(false);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(changeWireColor(bool)));
	}
	menu->addAction(m_createWireWireAct);
	menu->addSeparator();
	menu->addAction(m_deleteWireAct);
	menu->addAction(m_deleteWireMinusAct);
	menu->addSeparator();
	menu->addAction(m_addBendpointAct);
	menu->addAction(m_flattenCurveAct);

#ifndef QT_NO_DEBUG
	menu->addSeparator();
	menu->addAction(m_infoViewOnHoverAction);
#endif

	connect( menu, SIGNAL(aboutToShow()), this, SLOT(updateWireMenu()));
	auto * probe = new FProbeActions("BreadboardWire", menu);
	return menu;
}

QMenu *MainWindow::pcbWireMenu() {
	auto *menu = new QMenu(QObject::tr("Wire"), this);
	// createZOrderWireSubmenu(menu);
	createZOrderSubmenu(menu);
	menu->addSeparator();
	menu->addAction(m_changeTraceLayerWireAct);
	menu->addAction(m_createTraceWireAct);
	menu->addAction(m_excludeFromAutorouteWireAct);
	menu->addSeparator();
	menu->addAction(m_deleteWireAct);
	menu->addAction(m_deleteWireMinusAct);
	menu->addSeparator();
	menu->addAction(m_addBendpointAct);
	menu->addAction(m_convertToViaAct);
	menu->addAction(m_flattenCurveAct);

#ifndef QT_NO_DEBUG
	menu->addSeparator();
	menu->addAction(m_infoViewOnHoverAction);
#endif

	connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateWireMenu()));
	auto * probe = new FProbeActions("PCBWire", menu);
	return menu;
}

QMenu *MainWindow::schematicWireMenu() {
	auto *menu = new QMenu(QObject::tr("Wire"), this);
	// createZOrderWireSubmenu(menu);
	createZOrderSubmenu(menu);
	menu->addSeparator();
	m_schematicWireColorMenu = menu->addMenu(tr("&Wire Color"));
	Q_FOREACH(QString colorName, Wire::colorNames) {
		QString colorValue = Wire::colorTrans.value(colorName);
		if (colorValue == "white") continue;
		auto * action = new QAction(colorName, this);
		m_schematicWireColorMenu->addAction(action);
		action->setData(colorValue);
		action->setCheckable(true);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(changeWireColor(bool)));
	}
	menu->addAction(m_createTraceWireAct);
	menu->addAction(m_excludeFromAutorouteWireAct);
	menu->addSeparator();
	menu->addAction(m_deleteWireAct);
	menu->addAction(m_deleteWireMinusAct);
	menu->addSeparator();
	menu->addAction(m_addBendpointAct);
#ifndef QT_NO_DEBUG
	menu->addSeparator();
	menu->addAction(m_infoViewOnHoverAction);
#endif

	connect( menu, SIGNAL(aboutToShow()), this, SLOT(updateWireMenu()));
	auto * probe = new FProbeActions("SchematicWire", menu);
	return menu;
}

QMenu *MainWindow::viewItemMenuAux(QMenu* menu) {
	//  createZOrderWireSubmenu(menu);
	createZOrderSubmenu(menu);
	menu->addAction(m_moveLockAct);
	menu->addAction(m_stickyAct);
	menu->addSeparator();
	menu->addAction(m_copyAct);
	menu->addAction(m_duplicateAct);
	menu->addAction(m_deleteAct);
	menu->addAction(m_deleteMinusAct);

	menu->addSeparator();
	menu->addAction(m_openInPartsEditorNewAct);
	createAddToBinSubmenu(menu);
	menu->addSeparator();
	menu->addAction(m_showPartLabelAct);
#ifndef QT_NO_DEBUG
	menu->addSeparator();
	menu->addAction(m_infoViewOnHoverAction);
	menu->addAction(m_exportNormalizedSvgAction);
	menu->addAction(m_exportNormalizedFlattenedSvgAction);
#endif
	if (DebugDialog::enabled()) {
		menu->addSeparator();
		menu->addAction(m_testConnectorsAction);
		menu->addAction(m_disconnectAllAct);
	}

	connect(
	    menu,
	    SIGNAL(aboutToShow()),
	    this,
	    SLOT(updatePartMenu())
	);

	return menu;
}

void MainWindow::changeWireColor(bool checked) {
	if (checked == false) {
		// choosing the same color again (assuming this action can only apply to a single wire at a time)
		return;
	}

	auto * action = qobject_cast<QAction *>(sender());
	if (action == nullptr) return;

	QString colorName = action->data().toString();
	if (colorName.isEmpty()) return;

	m_currentGraphicsView->changeWireColor(colorName);
}

void MainWindow::startSaveInstancesSlot(const QString & fileName, ModelPart *, QXmlStreamWriter & streamWriter) {
	Q_UNUSED(fileName);

	if (m_backingUp) {
		streamWriter.writeTextElement("originalFileName", m_fwFilename);
	}

	m_projectProperties->saveProperties(streamWriter);

	if (m_pcbGraphicsView) {
		QList<ItemBase *> boards = m_pcbGraphicsView->findBoard();
		if (boards.count()) {
			streamWriter.writeStartElement("boards");
			Q_FOREACH (ItemBase * board, boards) {
				QRectF r = board->sceneBoundingRect();
				double w = 2.54 * r.width() / GraphicsUtils::SVGDPI;
				double h = 2.54 * r.height() / GraphicsUtils::SVGDPI;
				streamWriter.writeStartElement("board");
				streamWriter.writeAttribute("moduleId", QString("%1").arg(board->moduleID()));
				streamWriter.writeAttribute("title", QString("%1").arg(board->title()));
				streamWriter.writeAttribute("instance", QString("%1").arg(board->instanceTitle()));
				streamWriter.writeAttribute("width", QString("%1cm").arg(w));
				streamWriter.writeAttribute("height", QString("%1cm").arg(h));
				streamWriter.writeEndElement();
			}
			streamWriter.writeEndElement();
		}
	}

	if (m_breadboardGraphicsView) {
	}

	if (m_linkedProgramFiles.count() > 0) {
		streamWriter.writeStartElement("programs");
		QSettings settings;
		streamWriter.writeAttribute("pid", settings.value("pid").toString());
		Q_FOREACH (LinkedFile * linkedFile, m_linkedProgramFiles) {
			streamWriter.writeStartElement("program");
			streamWriter.writeAttribute("language", linkedFile->platform);
			streamWriter.writeCharacters(linkedFile->linkedFilename);
			streamWriter.writeEndElement();
		}
		streamWriter.writeEndElement();
	}

	streamWriter.writeStartElement("views");
	QList<SketchWidget *> views;
	views << m_breadboardGraphicsView << m_schematicGraphicsView << m_pcbGraphicsView;
	Q_FOREACH  (SketchWidget * sketchWidget, views) {
		streamWriter.writeStartElement("view");
		streamWriter.writeAttribute("name", ViewLayer::viewIDXmlName(sketchWidget->viewID()));
		streamWriter.writeAttribute("backgroundColor", sketchWidget->background().name());
		streamWriter.writeAttribute("gridSize", sketchWidget->gridSizeText());
		streamWriter.writeAttribute("showGrid", sketchWidget->showingGrid() ? "1" : "0");
		streamWriter.writeAttribute("alignToGrid", sketchWidget->alignedToGrid() ? "1" : "0");
		streamWriter.writeAttribute("viewFromBelow", sketchWidget->viewFromBelow() ? "1" : "0");
		QHash<QString, QString> autorouterSettings = sketchWidget->getAutorouterSettings();
		Q_FOREACH (QString key, autorouterSettings.keys()) {
			streamWriter.writeAttribute(key, autorouterSettings.value(key));
		}
		if (sketchWidget == m_breadboardGraphicsView) {
			streamWriter.writeAttribute("colorWiresByLength", m_breadboardGraphicsView->coloringWiresByLength() ? "1" : "0");
		}
		streamWriter.writeEndElement();
	}
	streamWriter.writeEndElement();
}

void MainWindow::obsoleteSMDOrientationSlot() {
	m_obsoleteSMDOrientation = true;
}

void MainWindow::oldSchematicsSlot(const QString &filename, bool & useOldSchematics) {
	useOldSchematics = m_convertedSchematic = m_useOldSchematic = false;
	if (m_noSchematicConversion) return;

	if (m_readOnly) {
		useOldSchematics = m_useOldSchematic = true;
		return;
	}

	QMessageBox::StandardButton answer = oldSchematicMessage(filename);
	if (answer == QMessageBox::No) {
		useOldSchematics = m_useOldSchematic = true;
		this->setReadOnly(true);
	}
	else {
		m_convertedSchematic = true;
	}
}

QMessageBox::StandardButton MainWindow::oldSchematicMessage(const QString & filename)
{
	QFileInfo info(filename);
	FMessageBox messageBox;
	messageBox.setWindowTitle(tr("Schematic view update"));
	messageBox.setText(tr("There is a new graphics standard for schematic-view part images, beginning with version 0.8.6.\n\n") +
	                   tr("Would you like to convert '%1' to the new standard now or open the file read-only?\n").arg(info.fileName())
	                  );
	messageBox.setInformativeText("<ul><li>" +
	                              tr("The conversion process will not modify '%1', until you save the file. ").arg(info.fileName()) +
	                              + "</li><li>" +
	                              tr("You will have to rearrange parts and connections in schematic view, as the sizes of most part images will have changed. Consider using the Autorouter to clean up traces. ") +
	                              + "</li><li>" +
	                              tr("Note that any custom parts will not be converted. A tool for converting 'rectangular' schematic images is available in the Parts Editor.") +
	                              + "</li></ul>"
	                             );
	messageBox.setDefaultButton(messageBox.addButton(tr("Convert"), QMessageBox::YesRole));
	messageBox.addButton(tr("Read-only"), QMessageBox::NoRole);
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setWindowModality(Qt::WindowModal);
	return (QMessageBox::StandardButton) messageBox.exec();
}


void MainWindow::loadedRootSlot(const QString & fname, ModelBase *, QDomElement & root) {
	if (root.isNull()) return;

	QDomElement programs = root.firstChildElement("programs");
	if (programs.isNull()) return;

	QString thatPid = programs.attribute("pid");
	QSettings settings;
	QString thisPid = settings.value("pid").toString();
	bool sameMachine = thatPid.isEmpty() || (!thisPid.isEmpty() && (thatPid.compare(thisPid) == 0));
	QFileInfo fileInfo(fname);
	QDir dir = fileInfo.absoluteDir();

	QDomElement program = programs.firstChildElement("program");
	while (!program.isNull()) {
		bool obsolete = false;
		bool inBundle = false;
		QString text;
		TextUtils::findText(program, text);
		if (!text.isEmpty()) {
			QString platform = program.attribute("language");
			QString path;
			if (thatPid.isEmpty()) {
				// pre 0.7.0 relative path
				QFileInfo newFileInfo(text);
				dir.cd(newFileInfo.dir().path());
				path = dir.absoluteFilePath(newFileInfo.fileName());
				obsolete = true;
			}
			else {
				path = text;
			}

			auto * linkedFile = new LinkedFile;
			QFileInfo info(path);
			if (!(sameMachine && info.exists())) {
				inBundle = true;
				path = dir.absoluteFilePath(info.fileName());
			}
			linkedFile->linkedFilename = path;
			linkedFile->platform = platform;
			linkedFile->fileFlags = LinkedFile::NoFlag;
			if (sameMachine) linkedFile->fileFlags |= LinkedFile::SameMachineFlag;
			if (obsolete) linkedFile->fileFlags |= LinkedFile::ObsoleteFlag;
			if (inBundle) linkedFile->fileFlags |= LinkedFile::InBundleFlag;
			if (this->m_readOnly) linkedFile->fileFlags |= LinkedFile::ReadOnlyFlag;

			m_linkedProgramFiles.append(linkedFile);
		}
		program = program.nextSiblingElement("program");
	}

}

void MainWindow::loadedViewsSlot(ModelBase *, QDomElement & views) {
	if (views.isNull()) return;

	QDomElement view = views.firstChildElement("view");
	while (!view.isNull()) {
		QString name = view.attribute("name");
		ViewLayer::ViewID viewID = ViewLayer::idFromXmlName(name);
		SketchWidget * sketchWidget = nullptr;
		switch (viewID) {
		case ViewLayer::BreadboardView:
			sketchWidget = m_breadboardGraphicsView;
			break;
		case ViewLayer::SchematicView:
			sketchWidget = m_schematicGraphicsView;
			break;
		case ViewLayer::PCBView:
			sketchWidget = m_pcbGraphicsView;
			break;
		default:
			break;
		}

		if (sketchWidget) {
			QString colorName = view.attribute("backgroundColor", "");
			QString gridSizeText = view.attribute("gridSize", "");
			QString alignToGridText = view.attribute("alignToGrid", "");
			QString showGridText = view.attribute("showGrid", "");
			QString viewFromBelowText = view.attribute("viewFromBelow", "");

			QHash<QString, QString> autorouterSettings;
			QDomNamedNodeMap map = view.attributes();
			for (int m = 0; m < map.count(); m++) {
				QDomNode node = map.item(m);
				autorouterSettings.insert(node.nodeName(), node.nodeValue());
			}
			sketchWidget->setAutorouterSettings(autorouterSettings);

			QColor color;
			color.setNamedColor(colorName);

			bool redraw = false;
			if (color.isValid()) {
				sketchWidget->setBackground(color);
				redraw = true;
			}
			if (!alignToGridText.isEmpty()) {
				sketchWidget->alignToGrid(alignToGridText.compare("1") == 0);
			}
			if (!showGridText.isEmpty()) {
				sketchWidget->showGrid(showGridText.compare("1") == 0);
				redraw = 1;
			}
			if (!gridSizeText.isEmpty()) {
				sketchWidget->setGridSize(gridSizeText);
				redraw = true;
			}

			if (sketchWidget == m_breadboardGraphicsView) {
				QString colorWiresByLengthText = view.attribute("colorWiresByLength", "");
				if (!colorWiresByLengthText.isEmpty()) {
					m_breadboardGraphicsView->colorWiresByLength(colorWiresByLengthText.compare("1") == 0);
				}
			}

			if (!viewFromBelowText.isEmpty()) {
				sketchWidget->setViewFromBelow(viewFromBelowText.compare("1") == 0);
				redraw = 1;
			}

			if (redraw) sketchWidget->invalidateScene();
		}

		view = view.nextSiblingElement("view");
	}
}

void MainWindow::loadedProjectPropertiesSlot(const QDomElement & projectProperties) {
	   m_projectProperties->load(projectProperties);
}

void MainWindow::disconnectAll() {
	m_currentGraphicsView->disconnectAll();
}

bool MainWindow::externalProcess(QString & name, QString & path, QStringList & args) {
	Q_EMIT externalProcessSignal(name, path, args);

	if (path.isEmpty()) return false;

	if (name.isEmpty()) {
		name = tr("Launch %1...").arg(path);
	}

	return true;
}

void MainWindow::launchExternalProcess() {
	QString name;
	QString path;
	QStringList args;
	if (!externalProcess(name, path, args)) return;

	args.append("-sketch");
	args.append(fileName());
	m_externalProcessOutput.clear();

	QFileInfo f = QFileInfo(path);
	auto * process = new QProcess(this);
	process->setWorkingDirectory(f.dir().absolutePath());
	process->setProcessChannelMode(QProcess::MergedChannels);
	process->setReadChannel(QProcess::StandardOutput);

	connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
	connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
	connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(processReadyRead()));
	connect(process, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(processStateChanged(QProcess::ProcessState)));

	process->start(path, args);
}


void MainWindow::processError(QProcess::ProcessError processError) {
	DebugDialog::debug(QString("process error %1").arg(processError));
}

void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
	DebugDialog::debug(QString("process finished %1 %2").arg(exitCode).arg(exitStatus));

	QString name, path;
	QStringList args;
	externalProcess(name, path, args);
	QMessageBox::information(this, name, QString(m_externalProcessOutput));

	sender()->deleteLater();
}

void MainWindow::processReadyRead() {
	QByteArray byteArray = qobject_cast<QProcess *>(sender())->readAllStandardOutput();
	m_externalProcessOutput.append(byteArray);

	DebugDialog::debug(byteArray.data());
}

void MainWindow::processStateChanged(QProcess::ProcessState newState) {
	switch(newState) {
	case QProcess::Running:
		DebugDialog::debug(QString("process running"));
		break;
	case QProcess::Starting:
		DebugDialog::debug(QString("process starting"));
		break;
	case QProcess::NotRunning:
		DebugDialog::debug(QString("process not running"));
		break;
	}
}

void MainWindow::shareOnline() {
	QUrl new_url(QString("https://fritzing.org/projects/create/"));
	QNetworkRequest request(new_url);

	QNetworkReply *reply = m_manager.get(request);
	connect(reply, SIGNAL(finished()), this, SLOT(onShareOnlineFinished()));
}

void MainWindow::onShareOnlineFinished() {
	auto *reply = qobject_cast<QNetworkReply*>(sender());

	if (reply->error() == QNetworkReply::NoError) {
		QDesktopServices::openUrl(QString("https://fritzing.org/projects/create/"));
	} else {
		FMessageBox::critical(this, tr("Fritzing"), QString("Online sharing is currently not available."));
	}
	reply->deleteLater();
}

void MainWindow::selectAllObsolete() {
	selectAllObsolete(true);
}

QList<ItemBase *> MainWindow::selectAllObsolete(bool displayFeedback) {
	QList<ItemBase *> items = m_pcbGraphicsView->selectAllObsolete();
	if (!displayFeedback) return items;

	if (items.count() <= 0) {
		QMessageBox::information(this, tr("Fritzing"), tr("No outdated parts found.\nAll your parts are up-to-date.") );
	}
	else {
		checkSwapObsolete(items, false);
	}

	return items;
}

void MainWindow::checkSwapObsolete(QList<ItemBase *> & items, bool includeUpdateLaterMessage) {
	QString msg = includeUpdateLaterMessage ? tr("\n\nNote: if you want to update later, there are options under the 'Part' menu for dealing with outdated parts individually. ") : "";

	QMessageBox::StandardButton answer = FMessageBox::question(
	        this,
	        tr("Outdated parts"),
	        tr("There are %n outdated part(s) in this sketch. ", "", items.count()) +
	        tr("We strongly recommend that you update these %n parts  to the latest version. ", "", items.count()) +
	        tr("This may result in changes to your sketch, as parts or connectors may be shifted. ") +
	        msg +
	        tr("\n\nDo you want to update now?"),
	        QMessageBox::Yes | QMessageBox::No,
	        QMessageBox::Yes
	                                     );
	// TODO: make button texts translatable
	if (answer == QMessageBox::Yes) {
		swapObsolete(true, items);
	}
}



ModelPart * MainWindow::findReplacedby(ModelPart * originalModelPart) {
	ModelPart * newModelPart = originalModelPart;
	while (true) {
		QString newModuleID = newModelPart->replacedby();
		if (newModuleID.isEmpty()) {
			return ((newModelPart == originalModelPart) ? nullptr : newModelPart);
		}

		ModelPart * tempModelPart = this->m_referenceModel->retrieveModelPart(newModuleID);
		if (tempModelPart == nullptr) {
			// something's screwy
			return nullptr;
		}

		newModelPart = tempModelPart;
	}
}

void MainWindow::swapObsolete() {
	QList<ItemBase *> items;
	swapObsolete(true, items);
}

void MainWindow::swapObsolete(bool displayFeedback, QList<ItemBase *> & items) {
	QSet<ItemBase *> itemBases;

	if (items.count() == 0) {
		Q_FOREACH (QGraphicsItem * item, m_pcbGraphicsView->scene()->selectedItems()) {
			auto * itemBase = dynamic_cast<ItemBase *>(item);
			if (itemBase == nullptr) continue;
			if (!itemBase->isObsolete()) continue;

			itemBase = itemBase->layerKinChief();
			itemBases.insert(itemBase);
		}

		if (itemBases.count() <= 0) return;
	}
	else {
		Q_FOREACH (ItemBase * itemBase, items) itemBases.insert(itemBase);
	}

	auto* parentCommand = new QUndoCommand();
	int count = 0;
	QMap<QString, QString> propsMap;
	Q_FOREACH (ItemBase * itemBase, itemBases) {
		ModelPart * newModelPart = findReplacedby(itemBase->modelPart());
		if (newModelPart == nullptr) {
			FMessageBox::information(
			    this,
			    tr("Sorry!"),
			    tr( "unable to find replacement for %1.\n").arg(itemBase->title())
			);
			continue;
		}

		count++;
		long newID = swapSelectedAuxAux(itemBase, newModelPart->moduleID(), itemBase->viewLayerPlacement(), propsMap, parentCommand);
		if (itemBase->modelPart()) {
			// special case for swapping old resistors.
			QString resistance = itemBase->modelPart()->properties().value("resistance", "");
			if (!resistance.isEmpty()) {
				QChar r = resistance.at(resistance.length() - 1);
				ushort ohm = r.unicode();
				if (ohm == 8486) {
					// ends with the ohm symbol
					resistance.chop(1);
				}
			}
			QString footprint = itemBase->modelPart()->properties().value("footprint", "");
			if (!resistance.isEmpty() && !footprint.isEmpty()) {
				new SetResistanceCommand(m_currentGraphicsView, newID, resistance, resistance, footprint, footprint, parentCommand);
			}

			// special case for swapping LEDs
			if (newModelPart->moduleID().contains(ModuleIDNames::ColorLEDModuleIDName)) {
				QString oldColor = itemBase->modelPart()->properties().value("color");
				QString newColor;
				if (oldColor.contains("red", Qt::CaseInsensitive)) {
					newColor = "Red (633nm)";
				}
				else if (oldColor.contains("blue", Qt::CaseInsensitive)) {
					newColor = "Blue (430nm)";
				}
				else if (oldColor.contains("yellow", Qt::CaseInsensitive)) {
					newColor = "Yellow (585nm)";
				}
				else if (oldColor.contains("green", Qt::CaseInsensitive)) {
					newColor = "Green (555nm)";
				}
				else if (oldColor.contains("white", Qt::CaseInsensitive)) {
					newColor = "White (4500K)";
				}

				if (newColor.length() > 0) {
					new SetPropCommand(m_currentGraphicsView, newID, "color", newColor, newColor, true, parentCommand);
				}
			}

		}
	}


	if (count == 0) {
		delete parentCommand;
	}
	else {
		parentCommand->setText(tr("Update %1 part(s)", "").arg(count));
		m_undoStack->push(parentCommand);
	}

	if (displayFeedback) {
		QMessageBox::information(this, tr("Fritzing"), tr("Successfully updated %1 part(s).\n"
		                         "Please check all views for potential side-effects.").arg(count) );
	}
	DebugDialog::debug(QString("updated %1 obsolete in %2").arg(count).arg(m_fwFilename));
}

void MainWindow::throwFakeException() {
	throw "fake exception";
}

void MainWindow::alignToGrid() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->alignToGrid(m_alignToGridAct->isChecked());
	setWindowModified(true);
}

void MainWindow::showGrid() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->showGrid(m_showGridAct->isChecked());
	setWindowModified(true);
}

void MainWindow::setGridSize()
{
	GridSizeThing gridSizeThing(m_currentGraphicsView->viewName(),
	                            m_currentGraphicsView->getShortName(),
	                            m_currentGraphicsView->defaultGridSizeInches(),
	                            m_currentGraphicsView->gridSizeText());

	GridSizeDialog dialog(&gridSizeThing);
	dialog.setWindowTitle(QObject::tr("Set Grid Size"));

	auto * vLayout = new QVBoxLayout(&dialog);

	vLayout->addWidget(createGridSizeForm(&gridSizeThing));

	auto * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
	buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

	connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

	vLayout->addWidget(buttonBox);

	int result = dialog.exec();

	if (result == QDialog::Accepted) {
		QString units = (gridSizeThing.inButton->isChecked() ? "in" : "mm");
		m_currentGraphicsView->setGridSize(gridSizeThing.lineEdit->text() + units);
		setWindowModified(true);
	}
}

QWidget * MainWindow::createGridSizeForm(GridSizeThing * gridSizeThing)
{
	this->setObjectName("gridSizeDia");
	auto * over = new QGroupBox("", this);

	auto * vLayout = new QVBoxLayout();

	auto * explain = new QLabel(tr("Set the grid size for %1.").arg(gridSizeThing->viewName));
	vLayout->addWidget(explain);

	auto * groupBox = new QGroupBox(this);

	auto * hLayout = new QHBoxLayout();

	auto * label = new QLabel(tr("Grid Size:"));
	hLayout->addWidget(label);

	gridSizeThing->lineEdit = new QLineEdit();

	gridSizeThing->lineEdit->setFixedWidth(55);

	gridSizeThing->validator = new QDoubleValidator(gridSizeThing->lineEdit);
	gridSizeThing->validator->setRange(0.001, 1.0, 4);
	gridSizeThing->validator->setNotation(QDoubleValidator::StandardNotation);
	gridSizeThing->validator->setLocale(QLocale::C);
	gridSizeThing->lineEdit->setValidator(gridSizeThing->validator);

	hLayout->addWidget(gridSizeThing->lineEdit);

	gridSizeThing->inButton = new QRadioButton(tr("in"), this);
	hLayout->addWidget(gridSizeThing->inButton);

	gridSizeThing->mmButton = new QRadioButton(tr("mm"), this);
	hLayout->addWidget(gridSizeThing->mmButton);

	groupBox->setLayout(hLayout);

	vLayout->addWidget(groupBox);
	vLayout->addSpacing(5);

	auto * pushButton = new QPushButton(this);
	pushButton->setText(tr("Restore Default"));
	pushButton->setMaximumWidth(150);
	vLayout->addWidget(pushButton);
	vLayout->addSpacing(10);

	over->setLayout(vLayout);

	if (gridSizeThing->gridSizeText.length() <= 2) {
		gridSizeThing->inButton->setChecked(true);
		gridSizeThing->lineEdit->setText(QString::number(gridSizeThing->defaultGridSize));
	}
	else {
		if (gridSizeThing->gridSizeText.endsWith("mm")) {
			gridSizeThing->mmButton->setChecked(true);
			gridSizeThing->validator->setTop(25.4);
		}
		else {
			gridSizeThing->inButton->setChecked(true);
		}
		QString szString = gridSizeThing->gridSizeText;
		szString.chop(2);
		gridSizeThing->lineEdit->setText(szString);
	}

	connect(gridSizeThing->inButton, SIGNAL(clicked(bool)), this, SLOT(gridUnits(bool)));
	connect(pushButton, SIGNAL(clicked()), this, SLOT(restoreDefaultGrid()));
	connect(gridSizeThing->mmButton, SIGNAL(clicked(bool)), this, SLOT(gridUnits(bool)));

	return over;
}

void MainWindow::colorWiresByLength() {
	if (m_breadboardGraphicsView == nullptr) return;

	m_breadboardGraphicsView->colorWiresByLength(m_colorWiresByLengthAct->isChecked());
	setWindowModified(true);
}


void MainWindow::openProgramWindow() {
	if (m_programWindow) {
		m_programWindow->setVisible(true);
		m_programWindow->raise();
		return;
	}

	m_programWindow = new ProgramWindow();
	connect(m_programWindow, SIGNAL(linkToProgramFile(const QString &, Platform *, bool, bool)),
	        this, SLOT(linkToProgramFile(const QString &, Platform *, bool, bool)));
	connect(m_programWindow, SIGNAL(changeActivationSignal(bool, QWidget *)), qApp, SLOT(changeActivation(bool, QWidget *)), Qt::DirectConnection);
	connect(m_programWindow, SIGNAL(destroyed(QObject *)), qApp, SLOT(topLevelWidgetDestroyed(QObject *)));

	QFileInfo fileInfo(m_fwFilename);
	m_programWindow->setup();
	m_programWindow->linkFiles(m_linkedProgramFiles, fileInfo.absoluteDir().absolutePath());
	m_programWindow->setVisible(true);
}

void MainWindow::linkToProgramFile(const QString & filename, Platform * platform, bool addLink, bool strong) {
#ifdef Q_OS_WIN
	Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
#else
	Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
#endif

	if (addLink && strong) {
		bool gotOne = false;
		Q_FOREACH (LinkedFile * linkedFile, m_linkedProgramFiles) {
			if (linkedFile->linkedFilename.compare(filename, sensitivity) == 0) {
				if (linkedFile->platform != platform->getName()) {
					linkedFile->platform = platform->getName();
					this->setWindowModified(true);
				}
				gotOne = true;
				break;
			}
		}
		if (!gotOne) {
			auto * linkedFile = new LinkedFile;
			linkedFile->linkedFilename = filename;
			linkedFile->platform = platform->getName();
			m_linkedProgramFiles.append(linkedFile);
			this->setWindowModified(true);
		}
		return;
	}
	else {
		for (int i = 0; i < m_linkedProgramFiles.count(); i++) {
			LinkedFile * linkedFile = m_linkedProgramFiles.at(i);
			if (linkedFile->linkedFilename.compare(filename, sensitivity) == 0) {
				if (strong) {
					m_linkedProgramFiles.removeAt(i);
					this->setWindowModified(true);
				}
				else {
					if (linkedFile->platform != platform->getName()) {
						linkedFile->platform = platform->getName();
						this->setWindowModified(true);
					}
				}
				return;
			}
		}
	}
}

QStringList MainWindow::newDesignRulesCheck()
{
	return newDesignRulesCheck(true);
}

QStringList MainWindow::newDesignRulesCheck(bool showOkMessage)
{
	QStringList results;

	if (m_currentGraphicsView == nullptr) return results;

	auto * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == nullptr) return results;

	ItemBase * board = nullptr;
	if (pcbSketchWidget->autorouteTypePCB()) {
		int boardCount;
		board = pcbSketchWidget->findSelectedBoard(boardCount);
		if (boardCount == 0) {
			QString message = tr("Your sketch does not have a board yet! DRC only works with a PCB.");
			results << message;
			FMessageBox::critical(this, tr("Fritzing"), message);
			return results;
		}
		if (board == nullptr) {
			QString message = tr("Please select a PCB. DRC only works on one board at a time.");
			results << message;
			FMessageBox::critical(this, tr("Fritzing"), message);
			return results;
		}
	}

	bool copper0Active = pcbSketchWidget->layerIsActive(ViewLayer::Copper0);
	bool copper1Active = pcbSketchWidget->layerIsActive(ViewLayer::Copper1);

	AutorouteProgressDialog progress(tr("DRC Progress..."), true, false, false, false, pcbSketchWidget, this);
	progress.setModal(true);
	progress.show();
	QRect pr = progress.frameGeometry();
	QRect wr = pcbSketchWidget->frameGeometry();
	QPoint p = pcbSketchWidget->mapTo(this, wr.topRight());
	progress.move(p.x() - pr.width(), pr.top());

	DRC drc(pcbSketchWidget, board);

	connect(&drc, SIGNAL(wantTopVisible()), this, SLOT(activeLayerTop()), Qt::DirectConnection);
	connect(&drc, SIGNAL(wantBottomVisible()), this, SLOT(activeLayerBottom()), Qt::DirectConnection);
	connect(&drc, SIGNAL(wantBothVisible()), this, SLOT(activeLayerBoth()), Qt::DirectConnection);

	connect(&progress, SIGNAL(cancel()), &drc, SLOT(cancel()), Qt::DirectConnection);

	connect(&drc, SIGNAL(setMaximumProgress(int)), &progress, SLOT(setMaximum(int)), Qt::DirectConnection);
	connect(&drc, SIGNAL(setProgressValue(int)), &progress, SLOT(setValue(int)), Qt::DirectConnection);
	connect(&drc, SIGNAL(setProgressMessage(const QString &)), &progress, SLOT(setMessage(const QString &)));
	connect(&drc, SIGNAL(hideProgress()), &progress, SLOT(close()));

	ProcessEventBlocker::processEvents();
	ProcessEventBlocker::block();
	results = drc.start(showOkMessage, pcbSketchWidget->getKeepout() * 1000 / GraphicsUtils::SVGDPI);     // pixels to mils
	ProcessEventBlocker::unblock();

	pcbSketchWidget->setLayerActive(ViewLayer::Copper1, copper1Active);
	pcbSketchWidget->setLayerActive(ViewLayer::Silkscreen1, copper1Active);
	pcbSketchWidget->setLayerActive(ViewLayer::Copper0, copper0Active);
	pcbSketchWidget->setLayerActive(ViewLayer::Silkscreen0, copper0Active);
	updateActiveLayerButtons();
	return results;
}

void MainWindow::changeTraceLayer() {
	if (m_currentGraphicsView == nullptr) return;
	if (m_currentGraphicsView != m_pcbGraphicsView) return;

	Wire * wire = retrieveWire();
	m_pcbGraphicsView->changeTraceLayer(wire, false, nullptr);
}

Wire * MainWindow::retrieveWire() {
	auto * wireAction = qobject_cast<WireAction *>(sender());
	if (wireAction == nullptr) return nullptr;

	return wireAction->wire();
}

ConnectorItem * MainWindow::retrieveConnectorItem() {
	auto * connectorItemAction = qobject_cast<ConnectorItemAction *>(sender());
	if (connectorItemAction == nullptr) return nullptr;

	return connectorItemAction->connectorItem();
}

void MainWindow::setSticky()
{
	QList<QGraphicsItem *> items = m_currentGraphicsView->scene()->selectedItems();
	if (items.count() < 1) return;

	auto * itemBase = dynamic_cast<ItemBase *>(items.at(0));
	if (itemBase == nullptr) return;

	if (!itemBase->isBaseSticky()) return;

	itemBase->setLocalSticky(!itemBase->isLocalSticky());
}

void MainWindow::moveLock()
{
	bool moveLock = true;

	Q_FOREACH (QGraphicsItem  * item, m_currentGraphicsView->scene()->selectedItems()) {
		ItemBase * itemBase = ItemBase::extractTopLevelItemBase(item);
		if (itemBase == nullptr) continue;
		if (itemBase->itemType() == ModelPart::Wire) continue;

		if (itemBase->moveLock()) {
			moveLock = false;
			break;
		}
	}

	ItemBase * viewedItem = m_infoView->currentItem();
	Q_FOREACH (QGraphicsItem  * item, m_currentGraphicsView->scene()->selectedItems()) {
		ItemBase * itemBase = ItemBase::extractTopLevelItemBase(item);
		if (itemBase == nullptr) continue;
		if (itemBase->itemType() == ModelPart::Wire) continue;

		itemBase->setMoveLock(moveLock);
		if (viewedItem && viewedItem->layerKinChief() == itemBase->layerKinChief()) {
			m_currentGraphicsView->viewItemInfo(itemBase);
		}
	}
}

void MainWindow::selectMoveLock()
{
	m_currentGraphicsView->selectAllMoveLock();
}

void MainWindow::autorouterSettings() {
	if (m_currentGraphicsView != m_pcbGraphicsView) return;

	m_pcbGraphicsView->autorouterSettings();
}

bool MainWindow::hasCopperFill() {
	Q_FOREACH(QGraphicsItem* item, m_pcbGraphicsView->scene()->items()) {
	    auto * base = dynamic_cast<GroundPlane *>(item);
	    if (base) {
		return true;
	    }
	}
	return false;
}

void MainWindow::orderFab()
{
	// save project if not cleanMainWindow::
	if (MainWindow::save()) {
		if (!hasCopperFill()) {
			const QString notShowAgain = QString("UploadCopperFillnotShowAgain");
			QSettings settings;

			QCheckBox *notAgain = new QCheckBox(tr("Don't show this again."));

			QMessageBox box(this);
			box.setWindowTitle(tr("Missing copper fill"));
			box.setText(tr("It is recommended to add copper/ground fill to your circuit to reduce acid usage during production.\n\nContinue upload?"));
			box.setIcon(QMessageBox::Icon::Question);
			QPushButton* cancelButton = box.addButton(QMessageBox::Cancel);
			box.addButton(QMessageBox::Ok);
			box.setDefaultButton(QMessageBox::Cancel);
			box.setCheckBox(notAgain);

			// Load the setting
			notAgain->setChecked(settings.value(notShowAgain, false).toBool());

			// If the checkbox is checked, don't show the message box
			if (!notAgain->isChecked()) {
				QObject::connect(notAgain, &QCheckBox::stateChanged, [&cancelButton](int state){
				    cancelButton->setEnabled(state == Qt::Unchecked);
				});

				int ret = box.exec();

				// Save the setting
				settings.setValue(notShowAgain, notAgain->isChecked());

				if (ret != QMessageBox::Ok) {
				    return;
				}
			}
		}
		// upload
		auto* manager = new QNetworkAccessManager();
		int boardCount;
		double width, height;
		QString boardTitle;
		m_pcbGraphicsView->calcBoardDimensions(boardCount, width, height, boardTitle);
		FabUploadDialog upload(manager, m_fwFilename, width, height, boardCount, boardTitle, this);
		upload.exec();
		delete manager;
	} else {
		FMessageBox::information(this, tr("Fritzing Fab Upload"), tr("Please first save your project in order to upload it."));
	}
}

void MainWindow::setGroundFillSeeds() {
	int boardCount;
	ItemBase * board = m_pcbGraphicsView->findSelectedBoard(boardCount);
	if (boardCount == 0) {
		QMessageBox::critical(this, tr("Fritzing"),
		                      tr("Your sketch does not have a board yet! Please add a PCB in order to use copper fill operations."));
		return;
	}
	if (board == nullptr) {
		QMessageBox::critical(this, tr("Fritzing"),
		                      tr("Please select a PCB. Copper fill operations only work on one board at a time."));
		return;
	}

	m_pcbGraphicsView->setGroundFillSeeds();
}

void MainWindow::clearGroundFillSeeds() {
	int boardCount;
	ItemBase * board = m_pcbGraphicsView->findSelectedBoard(boardCount);
	if (boardCount == 0) {
		QMessageBox::critical(this, tr("Fritzing"),
		                      tr("Your sketch does not have a board yet! Please add a PCB in order to use copper fill operations."));
		return;
	}
	if (board == nullptr) {
		QMessageBox::critical(this, tr("Fritzing"),
		                      tr("Please select a PCB. Copper fill operations only work on one board at a time."));
		return;
	}

	m_pcbGraphicsView->clearGroundFillSeeds();
}

void MainWindow::setOneGroundFillSeed() {
	auto * action = qobject_cast<ConnectorItemAction *>(sender());
	if (action == nullptr) return;

	ConnectorItem * connectorItem = action->connectorItem();
	if (connectorItem == nullptr) return;

	auto * command = new GroundFillSeedCommand(m_pcbGraphicsView, nullptr);
	command->addItem(connectorItem->attachedToID(), connectorItem->connectorSharedID(), action->isChecked());

	m_undoStack->push(command);
}

void MainWindow::gridUnits(bool checked) {
	QWidget * widget = qobject_cast<QWidget *>(sender());
	if (widget == nullptr) return;

	auto * dialog = qobject_cast<GridSizeDialog *>(widget->window());
	if (dialog == nullptr) return;

	GridSizeThing * gridSizeThing = dialog->gridSizeThing();

	QString units;
	if (sender() == gridSizeThing->inButton) {
		units = (checked) ? "in" : "mm";
	}
	else {
		units = (checked) ? "mm" : "in";
	}

	if (units.startsWith("mm")) {
		gridSizeThing->validator->setTop(25.4);
		gridSizeThing->lineEdit->setText(QString::number(gridSizeThing->lineEdit->text().toDouble() * 25.4));
	}
	else {
		gridSizeThing->validator->setTop(1.0);
		gridSizeThing->lineEdit->setText(QString::number(gridSizeThing->lineEdit->text().toDouble() / 25.4));
	}

}

void MainWindow::restoreDefaultGrid() {
	QWidget * widget = qobject_cast<QWidget *>(sender());
	if (widget == nullptr) return;

	auto * dialog = qobject_cast<GridSizeDialog *>(widget->window());
	if (dialog == nullptr) return;

	GridSizeThing * gridSizeThing = dialog->gridSizeThing();

	gridSizeThing->inButton->setChecked(true);
	gridSizeThing->mmButton->setChecked(false);
	gridSizeThing->lineEdit->setText(QString::number(gridSizeThing->defaultGridSize));
}

void MainWindow::setBackgroundColor()
{
	QColor cc = m_currentGraphicsView->background();
	QColor scc = m_currentGraphicsView->standardBackground();

	SetColorDialog setColorDialog(tr("%1 background").arg(m_currentGraphicsView->viewName()), cc, scc, true, this);
	int result = setColorDialog.exec();
	if (result == QDialog::Rejected) return;

	QColor newColor = setColorDialog.selectedColor();
	m_currentGraphicsView->setBackgroundColor(newColor, setColorDialog.isPrefsColor());
	setWindowModified(true);
}

void MainWindow::checkLoadedTraces() {
	if (m_pcbGraphicsView) m_pcbGraphicsView->checkLoadedTraces();
}

void MainWindow::showUnrouted()
{
	m_currentGraphicsView->showUnrouted();
}

void MainWindow::hidePartSilkscreen()
{
	m_pcbGraphicsView->hidePartSilkscreen();
}

void MainWindow::fabQuote() {
	if (m_pcbGraphicsView) m_pcbGraphicsView->fabQuote();
}

void MainWindow::findPartInSketch() {
	static QString lastSearchText;

	if (m_currentGraphicsView == nullptr) return;

	bool ok;
	QString text = QInputDialog::getText(this, tr("Enter Text"),
	                                     tr("Text will match part label, description, title, etc. Enter text to search for:"),
	                                     QLineEdit::Normal, lastSearchText, &ok);
	if (!ok || text.isEmpty()) return;

	lastSearchText = text;
	QSet<ItemBase *> itemBases;
	Q_FOREACH (QGraphicsItem * item, m_currentGraphicsView->scene()->items()) {
		auto * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == nullptr) continue;

		itemBases.insert(itemBase->layerKinChief());
	}

	QStringList strings;
	strings << text;
	QList<ItemBase *> exactMatched;
	QList<ItemBase *> partialMatched;

	// Iterate through all item bases to find matches.
	Q_FOREACH (ItemBase *itemBase, itemBases) {
		if (DebugDialog::enabled()) {
			if (QString::number(itemBase->id()).contains(text)) {
				partialMatched << itemBase;
				continue;
			}
		}

		// Prioritize exact matches on title
		if (itemBase->instanceTitle().compare(text, Qt::CaseInsensitive) == 0) {
			exactMatched << itemBase;
			continue;
		} else if (itemBase->instanceTitle().contains(text, Qt::CaseInsensitive)) {
			partialMatched << itemBase;
			continue;
		}

		QList<ModelPart *> modelParts;
		m_referenceModel->search(itemBase->modelPart(), strings, modelParts, true);
		if (!modelParts.isEmpty()) {
			partialMatched << itemBase;
		}
	}

	QList<ItemBase *> matched = partialMatched + exactMatched;

	if (matched.isEmpty()) {
		QMessageBox::information(this, tr("Search"), tr("No parts matched search term '%1'.").arg(text));
		return;
	}

	m_currentGraphicsView->selectItems(matched);
	m_currentGraphicsView->setFocus();
}

void MainWindow::setGroundFillKeepout() {
	if (m_pcbGraphicsView) m_pcbGraphicsView->setGroundFillKeepout();
}

void MainWindow::setViewFromBelowToggle() {
	if (m_pcbGraphicsView) {
		m_pcbGraphicsView->setViewFromBelow(m_viewFromBelowToggleAct->isChecked());
		updateActiveLayerButtons();
	}
}

void MainWindow::setViewFromBelow() {
	if (m_pcbGraphicsView) {
		m_pcbGraphicsView->setViewFromBelow(true);
		updateActiveLayerButtons();
	}
}

void MainWindow::setViewFromAbove() {
	if (m_pcbGraphicsView) {
		m_pcbGraphicsView->setViewFromBelow(false);
		updateActiveLayerButtons();
	}
}

void MainWindow::updateExportMenu() {
	bool enabled = m_currentGraphicsView;
	Q_FOREACH (QAction * action, m_exportMenu->actions()) {
		action->setEnabled(enabled);
	}
}

void MainWindow::testConnectors() {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->testConnectors();
}
