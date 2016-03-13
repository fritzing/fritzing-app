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


#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QtDebug>
#include <QInputDialog>
#include <QDropEvent>
#include <QMimeData>

#include "binmanager.h"
#include "stacktabwidget.h"
#include "stacktabbar.h"

#include "../../model/modelpart.h"
#include "../../mainwindow/mainwindow.h"
#include "../../model/palettemodel.h"
#include "../../waitpushundostack.h"
#include "../../debugdialog.h"
#include "../../utils/folderutils.h"
#include "../../utils/textutils.h"
#include "../../utils/fileprogressdialog.h"
#include "../../referencemodel/referencemodel.h"
#include "../../items/partfactory.h"
#include "../partsbinpalettewidget.h"
#include "../partsbinview.h"

///////////////////////////////////////////////////////////

QString BinLocation::toString(BinLocation::Location location) {
	switch (location) {
	case BinLocation::User:
			return "user";
	case BinLocation::More:
			return "more";
	case BinLocation::App:
			return "app";
	case BinLocation::Outside:
	default:
		return "outside";
	}
}

BinLocation::Location BinLocation::fromString(const QString & locationString) {
	if (locationString.compare("user", Qt::CaseInsensitive) == 0) return BinLocation::User;
	if (locationString.compare("more", Qt::CaseInsensitive) == 0) return BinLocation::More;
	if (locationString.compare("app", Qt::CaseInsensitive) == 0) return BinLocation::App;
	return BinLocation::Outside;
}

BinLocation::Location BinLocation::findLocation(const QString & filename) 
{

    if (filename.startsWith(FolderUtils::getUserBinsPath())) {
		return BinLocation::User;
	}
    else if (filename.startsWith(FolderUtils::getAppPartsSubFolderPath("bins") + "/more")) {
		return BinLocation::More;
	}
    else if (filename.startsWith(FolderUtils::getAppPartsSubFolderPath("bins"))) {
		return BinLocation::App;
	}

	return BinLocation::Outside;
}
///////////////////////////////////////////////////////////

QString BinManager::Title;
QString BinManager::MyPartsBinLocation;
QString BinManager::MyPartsBinTemplateLocation;
QString BinManager::SearchBinLocation;
QString BinManager::SearchBinTemplateLocation;
QString BinManager::ContribPartsBinLocation;
QString BinManager::TempPartsBinTemplateLocation;
QString BinManager::CorePartsBinLocation;

QString BinManager::StandardBinStyle = "background-color: gray;";
QString BinManager::CurrentBinStyle = "background-color: black;";

QHash<QString, QString> BinManager::StandardBinIcons;

BinManager::BinManager(class ReferenceModel *referenceModel, class HtmlInfoView *infoView, WaitPushUndoStack *undoStack, MainWindow* parent)
	: QFrame(parent)
{
	BinManager::Title = tr("Parts");

    m_combinedMenu = NULL;
    m_showListViewAction = m_showIconViewAction = NULL;

	m_referenceModel = referenceModel;
	m_infoView = infoView;
	m_undoStack = undoStack;
    m_defaultSaveFolder = FolderUtils::getUserBinsPath();
	m_mainWindow = parent;
	m_currentBin = NULL;

	connect(this, SIGNAL(savePartAsBundled(const QString &)), m_mainWindow, SLOT(saveBundledPart(const QString &)));

	m_unsavedBinsCount = 0;

	QVBoxLayout *lo = new QVBoxLayout(this);

	m_stackTabWidget = new StackTabWidget(this);   
	m_stackTabWidget->setTabPosition(QTabWidget::West);
	lo->addWidget(m_stackTabWidget);

	lo->setMargin(0);
	lo->setSpacing(0);
    setMaximumHeight(500);
}

BinManager::~BinManager() {
}

void BinManager::initStandardBins()
{
	createCombinedMenu();
	createContextMenus();

	//DebugDialog::debug("init bin manager");
	QList<BinLocation *> actualLocations;
	findAllBins(actualLocations);

    hackLocalContrib(actualLocations);

	restoreStateAndGeometry(actualLocations);
	foreach (BinLocation * location, actualLocations) {
		PartsBinPaletteWidget* bin = newBin();
        bin->load(location->path, m_mainWindow->fileProgressDialog(), true);
		m_stackTabWidget->addTab(bin, bin->icon(), bin->title());
		m_stackTabWidget->stackTabBar()->setTabToolTip(m_stackTabWidget->count() - 1, bin->title());
		registerBin(bin);
		delete location;
	}
    actualLocations.clear();

	//DebugDialog::debug("open core bin");
	openCoreBinIn();
		
	//DebugDialog::debug("after core bin");
    currentChanged(m_stackTabWidget->currentIndex());

	connectTabWidget();
}

void BinManager::addBin(PartsBinPaletteWidget* bin) {
	m_stackTabWidget->addTab(bin, bin->icon(), bin->title());
	registerBin(bin);
	setAsCurrentBin(bin);
}

void BinManager::registerBin(PartsBinPaletteWidget* bin) {

	if (!bin->fileName().isEmpty()) {
		m_openedBins[bin->fileName()] = bin;

	    if (bin->fileName().compare(CorePartsBinLocation) == 0) {
		    bin->setAllowsChanges(false);
	    }
	    else if (bin->fileName().compare(SearchBinLocation) == 0) {
		    bin->setAllowsChanges(false);
	    }
	    else if (bin->fileName().compare(ContribPartsBinLocation) == 0) {
		    bin->setAllowsChanges(false);
	    }
	    else if (bin->fileName().compare(m_tempPartsBinLocation) == 0) {
		    bin->setAllowsChanges(false);
	    }
        else if (bin->fileName().contains(FolderUtils::getAppPartsSubFolderPath("bins"))) {
		    bin->setAllowsChanges(false);
	    }
    }
}

void BinManager::connectTabWidget() {
	connect(
		m_stackTabWidget, SIGNAL(currentChanged(int)),
		this, SLOT(currentChanged(int))
	);
	connect(
		m_stackTabWidget, SIGNAL(tabCloseRequested(int)),
		this, SLOT(tabCloseRequested(int))
	);
}

void BinManager::insertBin(PartsBinPaletteWidget* bin, int index) {
	registerBin(bin);
	m_stackTabWidget->insertTab(index, bin, bin->icon(), bin->title());
	m_stackTabWidget->setCurrentIndex(index);
}

bool BinManager::beforeClosing() {
	bool retval = true;

	for(int j = 0; j < m_stackTabWidget->count(); j++) {
		PartsBinPaletteWidget *bin = qobject_cast<PartsBinPaletteWidget*>(m_stackTabWidget->widget(j));
		if (bin && !bin->fastLoaded()) {
			setAsCurrentTab(bin);
			retval = retval && bin->beforeClosing();
			if(!retval) break;
		}
	}


	if(retval) {
		saveStateAndGeometry();
	}

	return retval;
}

void BinManager::setAsCurrentTab(PartsBinPaletteWidget* bin) {
	m_stackTabWidget->setCurrentWidget(bin);
}


bool BinManager::hasAlienParts() {
	return false;

}

void BinManager::setInfoViewOnHover(bool infoViewOnHover) {
	Q_UNUSED(infoViewOnHover);
}

void BinManager::addNewPart(ModelPart *modelPart) {
	PartsBinPaletteWidget* myPartsBin = getOrOpenMyPartsBin();
	myPartsBin->addPart(modelPart);
	setDirtyTab(myPartsBin);
}

PartsBinPaletteWidget* BinManager::getOrOpenMyPartsBin() {
    return getOrOpenBin(MyPartsBinLocation, MyPartsBinTemplateLocation);
}

PartsBinPaletteWidget* BinManager::getOrOpenSearchBin() {
    PartsBinPaletteWidget * bin = getOrOpenBin(SearchBinLocation, SearchBinTemplateLocation);
	if (bin) {
		bin->setSaveQuietly(true);
	}
	return bin;
}

PartsBinPaletteWidget* BinManager::getOrOpenBin(const QString & binLocation, const QString & binTemplateLocation) {

    PartsBinPaletteWidget* partsBin = findBin(binLocation);

    if(!partsBin) {
        QString fileToOpen = QFileInfo(binLocation).exists() ? binLocation : createIfBinNotExists(binLocation, binTemplateLocation);
        partsBin = openBinIn(fileToOpen, false);
	}
	if (partsBin != NULL && partsBin->fastLoaded()) {
		partsBin->load(partsBin->fileName(), partsBin, false);
	}

    return partsBin;
}

PartsBinPaletteWidget* BinManager::findBin(const QString & binLocation) {
	for (int i = 0; i < m_stackTabWidget->count(); i++) {
		PartsBinPaletteWidget* bin = (PartsBinPaletteWidget *) m_stackTabWidget->widget(i);
        if(bin->fileName() == binLocation) {
            return bin;
		}
	}

	return NULL;
}

QString BinManager::createIfMyPartsNotExists() {
    return createIfBinNotExists(MyPartsBinLocation, MyPartsBinTemplateLocation);
}

QString BinManager::createIfSearchNotExists() {
    return createIfBinNotExists(SearchBinLocation, SearchBinTemplateLocation);
}

QString BinManager::createIfBinNotExists(const QString & dest, const QString & source)
{
    QString binPath = dest;
    QFile file(source);
	FolderUtils::slamCopy(file, binPath);
	return binPath;
}

void BinManager::addPartToBin(ModelPart *modelPart, int position) {
	PartsBinPaletteWidget *bin = m_currentBin? m_currentBin: getOrOpenMyPartsBin();
	addPartToBinAux(bin,modelPart,position);
}

void BinManager::addToMyParts(ModelPart *modelPart) {
	PartsBinPaletteWidget *bin = getOrOpenMyPartsBin();
	if (bin) {
		addPartToBinAux(bin,modelPart);
		setAsCurrentTab(bin);
	}
}

void BinManager::addToTempPartsBin(ModelPart *modelPart) {
	PartsBinPaletteWidget *bin = getOrOpenBin(m_tempPartsBinLocation, TempPartsBinTemplateLocation);
	if (bin) {
		addPartToBinAux(bin,modelPart);
		setAsCurrentTab(bin);
		bin->setDirty(false);
	}
}

void BinManager::hideTempPartsBin() {
	for (int i = 0; i < m_stackTabWidget->count(); i++) {
		PartsBinPaletteWidget* bin = (PartsBinPaletteWidget *) m_stackTabWidget->widget(i);
        if (bin->fileName().compare(m_tempPartsBinLocation) == 0) {
            m_stackTabWidget->removeTab(i);
			break;
		}
	}
}

void BinManager::addPartToBinAux(PartsBinPaletteWidget *bin, ModelPart *modelPart, int position) {
	if(bin) {
		if (bin->fastLoaded()) {
			bin->load(bin->fileName(), bin, false);
		}
		bin->addPart(modelPart, position);
		setDirtyTab(bin);
	}
}

void BinManager::load(const QString& filename) {
	openBin(filename);
}


void BinManager::setDirtyTab(PartsBinPaletteWidget* w, bool dirty) {
	/*
	if (!w->windowTitle().contains(FritzingWindow::QtFunkyPlaceholder)) {
		// trying to deal with the warning in QWidget::setWindowModified
		// but setting the title here doesn't work
		QString t = w->windowTitle();
		if (t.isEmpty()) t = " ";
		w->setWindowTitle(t);
	}
	*/
	w->setWindowModified(dirty);
	if(m_stackTabWidget != NULL) {
		int tabIdx = m_stackTabWidget->indexOf(w);
		m_stackTabWidget->setTabText(tabIdx, w->title()+(dirty? " *": ""));
	} else {
		qWarning() << tr("BinManager::setDirtyTab: Couldn't set the bin '%1' as dirty").arg(w->title());
	}
}

void BinManager::updateTitle(PartsBinPaletteWidget* w, const QString& newTitle) {
	if(m_stackTabWidget != NULL) {
		m_stackTabWidget->setTabText(m_stackTabWidget->indexOf(w), newTitle+" *");
		setDirtyTab(w);
	}
	else {
		qWarning() << tr("BinManager::updateTitle: Couldn't set the bin '%1' as dirty").arg(w->title());
	}
}

PartsBinPaletteWidget* BinManager::newBinIn() {
	PartsBinPaletteWidget* bin = newBin();
	bin->setPaletteModel(new PaletteModel(true, false), true);
	bin->setTitle(tr("New bin (%1)").arg(++m_unsavedBinsCount));
	insertBin(bin, m_stackTabWidget->count());
    bin->setReadOnly(false);
	renameBin();
	return bin;
}

PartsBinPaletteWidget* BinManager::openBinIn(QString fileName, bool fastLoad) {
	if(fileName.isNull() || fileName.isEmpty()) {
		fileName = QFileDialog::getOpenFileName(
				this,
				tr("Select a Fritzing Parts Bin file to open"),
				m_defaultSaveFolder,
				tr("Fritzing Bin Files (*%1 *%2);;Fritzing Bin (*%1);;Fritzing Shareable Bin (*%2)")
				.arg(FritzingBinExtension).arg(FritzingBundledBinExtension)
		);
        if (fileName.isNull()) return NULL;
	}
	PartsBinPaletteWidget* bin = NULL;
	bool createNewOne = false;
	if(m_openedBins.contains(fileName)) {
		bin = m_openedBins[fileName];
		if(m_stackTabWidget) {
			m_stackTabWidget->setCurrentWidget(bin);
		} else {
			m_openedBins.remove(fileName);
			createNewOne = true;
		}
	} else {
		createNewOne = true;
	}

	if(createNewOne) {
		bin = newBin();
		if(bin->open(fileName, bin, fastLoad)) {
			m_openedBins[fileName] = bin;
			insertBin(bin, m_stackTabWidget->count());

			// to force the user to take a decision of what to do with the imported parts
			if(fileName.endsWith(FritzingBundledBinExtension)) {
				setDirtyTab(bin);
			} 
		}
	}
	if (!fastLoad) {
		setAsCurrentBin(bin);
	}
	return bin;
}

PartsBinPaletteWidget* BinManager::openCoreBinIn() {
	PartsBinPaletteWidget* bin = findBin(CorePartsBinLocation);
	if (bin != NULL) {
		setAsCurrentTab(bin);
	}
	else {
		bin = newBin();
		bin->setAllowsChanges(false);
		bin->load(BinManager::CorePartsBinLocation, bin, false);
		insertBin(bin, 0);
	}
	setAsCurrentBin(bin);
	return bin;
}

PartsBinPaletteWidget* BinManager::newBin() {
	PartsBinPaletteWidget* bin = new PartsBinPaletteWidget(m_referenceModel, m_infoView, m_undoStack,this);
	connect(
		bin, SIGNAL(fileNameUpdated(PartsBinPaletteWidget*, const QString&, const QString&)),
		this, SLOT(updateFileName(PartsBinPaletteWidget*, const QString&, const QString&))
	);
	connect(
		bin, SIGNAL(focused(PartsBinPaletteWidget*)),
		this, SLOT(setAsCurrentBin(PartsBinPaletteWidget*))
	);
	connect(bin, SIGNAL(saved(bool)), m_mainWindow, SLOT(binSaved(bool)));
	connect(m_mainWindow, SIGNAL(alienPartsDismissed()), bin, SLOT(removeAlienParts()));

	return bin;
}

void BinManager::currentChanged(int index) {
	for (int i = 0; i < m_stackTabWidget->count(); i++) {
		PartsBinPaletteWidget* bin = (PartsBinPaletteWidget *) m_stackTabWidget->widget(i);
        if (bin == NULL) continue;
        if (!bin->hasMonoIcon()) continue;

        if (i == index) {
            m_stackTabWidget->setTabIcon(i, bin->icon());
        }
        else {
            m_stackTabWidget->setTabIcon(i, bin->monoIcon());
        }

    }

        

    

	PartsBinPaletteWidget *bin = getBin(index);
	if (bin) setAsCurrentBin(bin);
}

void BinManager::setAsCurrentBin(PartsBinPaletteWidget* bin) {
	if (bin == NULL) {
		qWarning() << tr("Cannot set a NULL bin as the current one");
		return;
	}

	if (bin->fastLoaded()) {
		bin->load(bin->fileName(), bin, false);
	}

	if (m_currentBin == bin) return;

	if (bin->fileName().compare(SearchBinLocation) == 0) {
		bin->focusSearch();
	}

    /*

    // jrc 3-july-2013 commented out this stylesheet change:
    //      it causes the tab bar to lose its scroll position
    //      the stylesheet change is commented out in the qss file, so visually it's a no-op

	QString style = m_mainWindow->styleSheet();
	if(m_currentBin && m_stackTabWidget) {
		StackTabBar *currTabBar = m_stackTabWidget->stackTabBar();
		currTabBar->setProperty("current","false");
		currTabBar->setStyleSheet("");
		currTabBar->setStyleSheet(style);
	}
	if(m_stackTabWidget) {
		m_currentBin = bin;
		StackTabBar *currTabBar = m_stackTabWidget->stackTabBar();
		currTabBar->setProperty("current","true");
		currTabBar->setStyleSheet("");
		currTabBar->setStyleSheet(style);
	}
    */
}

void BinManager::closeBinIn(int index) {
	if (m_stackTabWidget->count() == 1) return;

	int realIndex = index == -1? m_stackTabWidget->currentIndex(): index;
	PartsBinPaletteWidget *w = getBin(realIndex);
	if(w && w->beforeClosing()) {
		m_stackTabWidget->removeTab(realIndex);
		m_openedBins.remove(w->fileName());
	}
}

PartsBinPaletteWidget* BinManager::getBin(int index) {
	return qobject_cast<PartsBinPaletteWidget*>(m_stackTabWidget->widget(index));
}

PartsBinPaletteWidget* BinManager::currentBin() {
	return qobject_cast<PartsBinPaletteWidget*>(m_stackTabWidget->currentWidget());
}

void BinManager::updateFileName(PartsBinPaletteWidget* bin, const QString &newFileName, const QString &oldFilename) {
	m_openedBins.remove(oldFilename);
	m_openedBins[newFileName] = bin;
}

void BinManager::saveStateAndGeometry() {
	QSettings settings;
	settings.remove("bins2"); // clean up previous state
	settings.beginGroup("bins2");

	for(int j = m_stackTabWidget->count() - 1; j >= 0; j--) {
		PartsBinPaletteWidget *bin = qobject_cast<PartsBinPaletteWidget*>(m_stackTabWidget->widget(j));
		if (bin) {
			settings.beginGroup(QString::number(j));
			settings.setValue("location", BinLocation::toString(bin->location()));
			settings.setValue("title", bin->title());
			settings.setValue("path", bin->fileName());
			settings.endGroup();
		}
	}

	settings.endGroup();
}

void BinManager::restoreStateAndGeometry(QList<BinLocation *> & actualLocations) {
	QList<BinLocation *> theoreticalLocations;

	QSettings settings;
	settings.beginGroup("bins2");
	int size = settings.childGroups().size();
	if (size == 0) { 
		// first time
		readTheoreticalLocations(theoreticalLocations);
	} 
	else {
		for (int i = 0; i < size; ++i) {
			settings.beginGroup(QString::number(i));
			BinLocation  * location = new BinLocation;
			location->location = BinLocation::fromString(settings.value("location").toString());
			location->path = settings.value("path").toString();
			location->title = settings.value("title").toString();
			theoreticalLocations.append(location);
			settings.endGroup();
		}
	}

	foreach (BinLocation * location, actualLocations) {
		location->marked = false;
	}
	foreach (BinLocation * tLocation, theoreticalLocations) {
		foreach (BinLocation * aLocation, actualLocations) {
			if (aLocation->title.compare(tLocation->title) == 0 && aLocation->location == tLocation->location) {
				aLocation->marked = true;
				break;
			}
		}
	}

	QList<BinLocation *> tempLocations(actualLocations);
	actualLocations.clear();

	foreach (BinLocation * tLocation, theoreticalLocations) {
        tLocation->marked = false;
        bool gotOne = false;

		for (int ix = 0; ix < tempLocations.count(); ix++) {
			BinLocation  * aLocation = tempLocations[ix];
			if (aLocation->title.compare(tLocation->title) == 0 && aLocation->location == tLocation->location) {
				gotOne = true;
				actualLocations.append(aLocation);
				tempLocations.removeAt(ix);
                //DebugDialog::debug("adding loc 1 " + aLocation->path);
				break;
			}
		}
		if (gotOne) continue;

		if (tLocation->title == "___*___") {
			for (int ix = 0; ix < tempLocations.count(); ix++) {
				BinLocation * aLocation = tempLocations[ix];
				if (!aLocation->marked && aLocation->location == tLocation->location) {
					gotOne = true;
					actualLocations.append(aLocation);
					tempLocations.removeAt(ix);
                    //DebugDialog::debug("adding loc 2 " + aLocation->path);
                    break;
				}
			}
		}
		if (gotOne) continue;

		if (!tLocation->path.isEmpty()) {
			QFileInfo info(tLocation->path);
			if (info.exists()) {
                //DebugDialog::debug("adding loc 3 " + tLocation->path);
				actualLocations.append(tLocation);
                tLocation->marked = true;
			}		
		}
	}

        foreach (BinLocation * tLocation, theoreticalLocations) {
            if (!tLocation->marked) {
                delete tLocation;
            }
	}

	// catch the leftovers
	actualLocations.append(tempLocations);
    //foreach (BinLocation * aLocation, tempLocations) {
        //DebugDialog::debug("adding loc 4 " + aLocation->path);
    //}


}

void BinManager::readTheoreticalLocations(QList<BinLocation *> & theoreticalLocations) 
{
	QFile file(":/resources/bins/order.xml");
	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;

	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		DebugDialog::debug(QString("unable to parse order.xml: %1 %2 %3").arg(errorStr).arg(errorLine).arg(errorColumn));
		return;
	}

	QDomElement bin = domDocument.documentElement().firstChildElement("bin");
	while (!bin.isNull()) {
		BinLocation * location = new BinLocation;
		location->title = bin.attribute("title", "");
		location->location = BinLocation::fromString(bin.attribute("location", ""));
		theoreticalLocations.append(location);
		bin = bin.nextSiblingElement("bin");
	}
}

void BinManager::hackLocalContrib(QList<BinLocation *> & locations) 
{
    // with release 0.7.12, there is no more local contrib bin
    // so clear out existing local contrib bins by copying parts to mine bin

    BinLocation * localContrib = NULL;
    BinLocation * myParts = NULL;
    foreach (BinLocation * location, locations) {
        if (location->location == BinLocation::User) {
            if (location->title == "Contributed Parts") {
                localContrib = location;
            }
            else if (location->title == "My Parts") {
                myParts = location;
            }
        }
    }

    if (localContrib == NULL) return;

    if (myParts == NULL) {
        createIfBinNotExists(MyPartsBinLocation, MyPartsBinTemplateLocation);
	    myParts = new BinLocation;
	    myParts->location = BinLocation::User;
	    myParts->path = MyPartsBinLocation;
	    QString icon;
	    getBinTitle(myParts->path, myParts->title, icon);
	    locations.append(myParts);
    }

	QString errorStr;
	int errorLine;
	int errorColumn;

    QFile contribFile(localContrib->path);
    QDomDocument contribDoc;
    bool result = contribDoc.setContent(&contribFile, true, &errorStr, &errorLine, &errorColumn);
    locations.removeOne(localContrib);
    contribFile.close();
    bool removed = contribFile.remove();
    if (!removed) {
        DebugDialog::debug("failed to remove contrib bin");
    }

    if (!result) return;

    QFile myPartsFile(myParts->path);
    QDomDocument myPartsDoc;
    if (!myPartsDoc.setContent(&myPartsFile, true, &errorStr, &errorLine, &errorColumn)) {
        return;
    }

    QDomElement myPartsRoot = myPartsDoc.documentElement();
    QDomElement myPartsInstances = myPartsRoot.firstChildElement("instances");

    bool changed = false;

    QDomElement contribRoot = contribDoc.documentElement();
    QDomElement contribInstances = contribRoot.firstChildElement("instances");
    QDomElement contribInstance = contribInstances.firstChildElement("instance");
    while (!contribInstance.isNull()) {
        QString moduleIDRef = contribInstance.attribute("moduleIdRef");
        QDomElement myPartsInstance = myPartsInstances.firstChildElement("instance");
        bool already = false;
        while (!myPartsInstance.isNull()) {
            QString midr = myPartsInstance.attribute("moduleIdRef");
            if (midr == moduleIDRef) {
                already = true;
                break;
            }
            myPartsInstance = myPartsInstance.nextSiblingElement("instance");
        }

        if (!already) {
            QDomNode node = contribInstance.cloneNode(true);
            myPartsInstances.appendChild(node);
            changed = true;
        }

        contribInstance = contribInstance.nextSiblingElement("instance");
    }

    if (changed) {
        TextUtils::writeUtf8(myParts->path, myPartsDoc.toString(0));
    }
}

void BinManager::findAllBins(QList<BinLocation *> & locations) 
{
	BinLocation * location = new BinLocation;
	location->location = BinLocation::App;
	location->path = CorePartsBinLocation;
	QString icon;
	getBinTitle(location->path, location->title, icon);
	locations.append(location);

	location = new BinLocation;
	location->location = BinLocation::App;
	location->path = ContribPartsBinLocation;
	getBinTitle(location->path, location->title, icon);
	locations.append(location);

    QDir userBinsDir(FolderUtils::getUserBinsPath());
	findBins(userBinsDir, locations, BinLocation::User);

    QDir dir(FolderUtils::getAppPartsSubFolderPath("bins"));
	dir.cd("more");
	findBins(dir, locations, BinLocation::More);
}

void BinManager::findBins(QDir & dir, QList<BinLocation *> & locations, BinLocation::Location loc) {

	QStringList filters;
	filters << "*"+FritzingBinExtension;
	QFileInfoList files = dir.entryInfoList(filters);
	foreach(QFileInfo info, files) {
		BinLocation * location = new BinLocation;
		location->path = info.absoluteFilePath();
		location->location = loc;
		QString icon;
		getBinTitle(location->path, location->title, icon);
		locations.append(location);
	}
}

bool BinManager::getBinTitle(const QString & filename, QString & binName, QString & iconName) {
	QFile file(filename);
	file.open(QFile::ReadOnly);
	QXmlStreamReader xml(&file);
	xml.setNamespaceProcessing(false);

	while (!xml.atEnd()) {
        switch (xml.readNext()) {
        case QXmlStreamReader::StartElement:
			if (xml.name().toString().compare("module") == 0) {
				iconName = xml.attributes().value("icon").toString();
			}
			else if (xml.name().toString().compare("title") == 0) {
				binName = xml.readElementText();
				return true;
			}
			break;
			
		default:
			break;
		}
	}

	return false;
}

void BinManager::tabCloseRequested(int index) {
	closeBinIn(index);
}

void BinManager::addPartTo(PartsBinPaletteWidget* bin, ModelPart* mp, bool setDirty) {
	if(mp) {
		bool alreadyIn = bin->contains(mp->moduleID());
		bin->addPart(mp);
		if(!alreadyIn && setDirty) {
			setDirtyTab(bin,true);
		}
	}
}

void BinManager::editSelectedPartNewFrom(PartsBinPaletteWidget* bin) {
	ItemBase * itemBase = bin->selectedItemBase();
	m_mainWindow->getPartsEditorNewAnd(itemBase);
}

bool BinManager::isTabReorderingEvent(QDropEvent* event) {
	const QMimeData *m = event->mimeData();
	QStringList formats = m->formats();
	return formats.contains("action") && (m->data("action") == "tab-reordering");
}

const QString &BinManager::getSelectedModuleIDFromSketch() {
	return m_mainWindow->selectedModuleID();
}

QList<QAction*> BinManager::openedBinsActions(const QString &moduleId) {
	QMap<QString,QAction*> titlesAndActions; // QMap sorts values by key

	for (int i = 0; i < m_stackTabWidget->count(); i++) {
		PartsBinPaletteWidget* pppw = (PartsBinPaletteWidget *) m_stackTabWidget->widget(i);
        if (pppw->readOnly()) continue;

		QAction *act = pppw->addPartToMeAction();
		act->setEnabled(!pppw->contains(moduleId));
		titlesAndActions[pppw->title()] = act;
	}

	return titlesAndActions.values();
}

void BinManager::openBin(const QString &filename) {
	openBinIn(filename, false);
}

MainWindow* BinManager::mainWindow() {
	return m_mainWindow;
}

void BinManager::initNames() {
    BinManager::MyPartsBinLocation = FolderUtils::getUserBinsPath()+"/my_parts.fzb";
    BinManager::MyPartsBinTemplateLocation =":/resources/bins/my_parts.fzb";
    BinManager::SearchBinLocation = FolderUtils::getUserBinsPath()+"/search.fzb";
    BinManager::SearchBinTemplateLocation =":/resources/bins/search.fzb";
    BinManager::ContribPartsBinLocation = FolderUtils::getAppPartsSubFolderPath("bins")+"/contribParts.fzb";
    BinManager::CorePartsBinLocation = FolderUtils::getAppPartsSubFolderPath("bins")+"/core.fzb";
    BinManager::TempPartsBinTemplateLocation =":/resources/bins/temp.fzb";

	StandardBinIcons.insert(BinManager::MyPartsBinLocation, "Mine.png");
	StandardBinIcons.insert(BinManager::SearchBinLocation, "Search.png");
	StandardBinIcons.insert(BinManager::ContribPartsBinLocation, "Contrib.png");
	StandardBinIcons.insert(BinManager::CorePartsBinLocation, "Core.png");
}

void BinManager::search(const QString & searchText) {
    PartsBinPaletteWidget * searchBin = getOrOpenSearchBin();
    if (searchBin == NULL) return;

    FileProgressDialog progress(tr("Searching..."), 0, this);
    progress.setIncValueMod(10);
    connect(m_referenceModel, SIGNAL(addSearchMaximum(int)), &progress, SLOT(addMaximum(int)));
    connect(m_referenceModel, SIGNAL(incSearch()), &progress, SLOT(incValue()));

    QList<ModelPart *> modelParts = m_referenceModel->search(searchText, false);

    progress.setIncValueMod(1);
    searchBin->removeParts();
    foreach (ModelPart * modelPart, modelParts) {
        //DebugDialog::debug(modelPart->title());
        if (modelPart->itemType() == ModelPart::SchematicSubpart) {
        }
        else if (modelPart->moduleID().contains(PartFactory::OldSchematicPrefix)) {
        }
        else {
            this->addPartTo(searchBin, modelPart, false);
        }
        progress.incValue();
    }
 
    setDirtyTab(searchBin);
}

bool BinManager::currentViewIsIconView() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return true;
	
	return bin->currentViewIsIconView();
}

void BinManager::toIconView() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	bin->toIconView();
}

void BinManager::toListView() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	bin->toListView();
}

void BinManager::updateBinCombinedMenuCurrent() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	updateBinCombinedMenu(bin);
}

void BinManager::updateBinCombinedMenu(PartsBinPaletteWidget * bin) {
    if (m_combinedMenu == NULL) return;

	m_saveBinAction->setEnabled(bin->allowsChanges());
	m_renameBinAction->setEnabled(bin->canClose());
	m_closeBinAction->setEnabled(bin->canClose());
	m_deleteBinAction->setEnabled(bin->canClose());
	ItemBase *itemBase = bin->selectedItemBase();
	bool enabled = (itemBase != NULL);
	m_editPartNewAction->setEnabled(enabled && itemBase->canEditPart());

    bool enableAnyway = false;
#ifndef QT_NO_DEBUG
    enableAnyway = true;
#endif
	m_exportPartAction->setEnabled(enabled && (!itemBase->modelPart()->isCore() || enableAnyway));
	m_removePartAction->setEnabled(enabled && bin->allowsChanges());
	m_findPartAction->setEnabled(enabled);
}

void BinManager::createCombinedMenu() 
{
	m_combinedMenu = new QMenu(tr("Bin"), this);

    m_openAction = new QAction(tr("Import..."), this);
	m_openAction->setStatusTip(tr("Load a Fritzing part (.fzpz), or a Fritzing parts bin (.fzb, .fzbz)"));
	connect(m_openAction, SIGNAL(triggered()), this, SLOT(mainLoad()));

	m_newBinAction = new QAction(tr("New Bin..."), this);
	m_newBinAction->setStatusTip(tr("Create a new parts bin"));
	connect(m_newBinAction, SIGNAL(triggered()),this, SLOT(newBinIn()));

	m_closeBinAction = new QAction(tr("Close Bin"), this);
	m_closeBinAction->setStatusTip(tr("Close parts bin"));
	connect(m_closeBinAction, SIGNAL(triggered()),this, SLOT(closeBin()));

	m_deleteBinAction = new QAction(tr("Delete Bin"), this);
	m_deleteBinAction->setStatusTip(tr("Delete parts bin"));
	connect(m_deleteBinAction, SIGNAL(triggered()),this, SLOT(deleteBin()));

	m_saveBinAction = new QAction(tr("Save Bin"), this);
	m_saveBinAction->setStatusTip(tr("Save parts bin"));
	connect(m_saveBinAction, SIGNAL(triggered()),this, SLOT(saveBin()));

	m_saveBinAsAction = new QAction(tr("Save Bin As..."), this);
	m_saveBinAsAction->setStatusTip(tr("Save parts bin as..."));
	connect(m_saveBinAsAction, SIGNAL(triggered()),this, SLOT(saveBinAs()));

	m_saveBinAsBundledAction = new QAction(tr("Export Bin..."), this);
	m_saveBinAsBundledAction->setStatusTip(tr("Save parts bin in compressed format..."));
	connect(m_saveBinAsBundledAction, SIGNAL(triggered()),this, SLOT(saveBundledBin()));

	m_renameBinAction = new QAction(tr("Rename Bin..."), this);
	m_renameBinAction->setStatusTip(tr("Rename parts bin..."));
	connect(m_renameBinAction, SIGNAL(triggered()),this, SLOT(renameBin()));

	m_copyToSketchAction = new QAction(tr("Copy to Sketch"), this);
	m_copyToSketchAction->setStatusTip(tr("Copy all the parts in the bin to a sketch"));
	connect(m_copyToSketchAction, SIGNAL(triggered()),this, SLOT(copyToSketch()));

	m_copyAllToSketchAction = new QAction(tr("Copy all to Sketch"), this);
	m_copyAllToSketchAction->setStatusTip(tr("Copy all loaded parts to the sketch"));
	connect(m_copyAllToSketchAction, SIGNAL(triggered()),this, SLOT(copyAllToSketch()));

	m_showListViewAction = new QAction(tr("Show Bin in List View"), this);
	m_showListViewAction->setCheckable(true);
	m_showListViewAction->setStatusTip(tr("Display parts as a list"));
	connect(m_showListViewAction, SIGNAL(triggered()),this, SLOT(toListView()));

	m_showIconViewAction = new QAction(tr("Show Bin in Icon View"), this);
	m_showIconViewAction->setCheckable(true);
	m_showIconViewAction->setStatusTip(tr("Display parts as icons"));
	connect(m_showIconViewAction, SIGNAL(triggered()),this, SLOT(toIconView()));

    m_combinedMenu->addAction(m_openAction);
	m_combinedMenu->addSeparator();

	m_combinedMenu->addAction(m_newBinAction);
	m_combinedMenu->addAction(m_closeBinAction);
	m_combinedMenu->addAction(m_deleteBinAction);
	m_combinedMenu->addAction(m_saveBinAction);
	m_combinedMenu->addAction(m_saveBinAsAction);
	m_combinedMenu->addAction(m_saveBinAsBundledAction);
	m_combinedMenu->addAction(m_renameBinAction);
#ifndef QT_NO_DEBUG
	m_combinedMenu->addAction(m_copyToSketchAction);
	m_combinedMenu->addAction(m_copyAllToSketchAction);
#endif
	m_combinedMenu->addSeparator();
	m_combinedMenu->addAction(m_showIconViewAction);
	m_combinedMenu->addAction(m_showListViewAction);

	m_editPartNewAction = new QAction(tr("Edit Part (new parts editor)..."),this);
	m_exportPartAction = new QAction(tr("Export Part..."),this);
	m_removePartAction = new QAction(tr("Remove Part"),this);
	m_findPartAction = new QAction(tr("Find Part in Sketch"),this);

	connect(m_editPartNewAction, SIGNAL(triggered()),this, SLOT(editSelectedNew()));
	connect(m_exportPartAction, SIGNAL(triggered()),this, SLOT(exportSelected()));
	connect(m_removePartAction, SIGNAL(triggered()),this, SLOT(removeSelected()));
	connect(m_findPartAction, SIGNAL(triggered()),this, SLOT(findSelected()));

	connect(m_combinedMenu, SIGNAL(aboutToShow()), this, SLOT(updateBinCombinedMenuCurrent()));

	m_combinedMenu->addSeparator();
	m_combinedMenu->addAction(m_editPartNewAction);
	m_combinedMenu->addAction(m_exportPartAction);
	m_combinedMenu->addAction(m_removePartAction);
	m_combinedMenu->addAction(m_findPartAction);

}

void BinManager::createContextMenus() {
	m_binContextMenu = new QMenu(this);
	m_binContextMenu->addAction(m_closeBinAction);
	m_binContextMenu->addAction(m_deleteBinAction);
	m_binContextMenu->addAction(m_saveBinAction);
	m_binContextMenu->addAction(m_saveBinAsAction);
	m_binContextMenu->addAction(m_saveBinAsBundledAction);
	m_binContextMenu->addAction(m_renameBinAction);

	m_partContextMenu = new QMenu(this);
	m_partContextMenu->addAction(m_editPartNewAction);
	m_partContextMenu->addAction(m_exportPartAction);
	m_partContextMenu->addAction(m_removePartAction);
	m_partContextMenu->addAction(m_findPartAction);
}

void BinManager::closeBin() {
	closeBinIn(-1);
}

void BinManager::deleteBin() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	QMessageBox::StandardButton answer = QMessageBox::question(
		this,
		tr("Delete bin"),
		tr("Do you really want to delete bin '%1'?  This action cannot be undone.").arg(bin->title()),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No
	);
	// TODO: make button texts translatable
	if (answer != QMessageBox::Yes) return;

	QString filename = bin->fileName();
	closeBinIn(-1);
	QFile::remove(filename);
}

void BinManager::importPartToMineBin(const QString & filename) {

	if (!filename.isEmpty() && !filename.isNull()) {
        PartsBinPaletteWidget * bin = getOrOpenBin(MyPartsBinLocation, MyPartsBinTemplateLocation);
        if (bin == NULL) return;

        setAsCurrentTab(bin);
        importPart(filename, bin);
    }
}

void BinManager::importPartToCurrentBin(const QString & filename) {

	if (!filename.isEmpty() && !filename.isNull()) {
        PartsBinPaletteWidget * bin = currentBin();
        if (bin == NULL) return;

        importPart(filename, bin);
	}
}

void BinManager::importPart(const QString & filename, PartsBinPaletteWidget * bin) {
	ModelPart *mp = m_mainWindow->loadBundledPart(filename, !bin->allowsChanges());
	if (bin->allowsChanges()) {
		addPartTo(bin, mp, true);
	}
}

void BinManager::editSelectedNew() {
	editSelectedPartNewFrom(currentBin());
}

void BinManager::renameBin() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	if (!currentBin()->allowsChanges()) {
		// TODO: disable menu item instead
		QMessageBox::warning(this, tr("Read-only bin"), tr("This bin cannot be renamed."));
		return;
	}

	bool ok;
	QString newTitle = QInputDialog::getText(
		this,
		tr("Rename bin"),
		tr("Please choose a name for the bin:"),
		QLineEdit::Normal,
		bin->title(),
		&ok
	);
	if(ok) {
		bin->setTitle(newTitle);
		m_stackTabWidget->stackTabBar()->setTabToolTip(m_stackTabWidget->currentIndex(), newTitle);
		bin->addPartToMeAction()->setText(newTitle);
		updateTitle(bin, newTitle);
	}
}

void BinManager::saveBin() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	bool result = bin->save();
	if (result) setDirtyTab(currentBin(),false);
}

void BinManager::saveBinAs() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	bin->saveAs();
}


void BinManager::updateViewChecks(bool iconView) {
    if (m_showListViewAction == NULL) return;
    if (m_showIconViewAction == NULL) return;

	if (iconView) {
		m_showListViewAction->setChecked(false);
		m_showIconViewAction->setChecked(true);
	}
	else {
		m_showListViewAction->setChecked(true);
		m_showIconViewAction->setChecked(false);
	}
}

QMenu * BinManager::binContextMenu(PartsBinPaletteWidget * bin) {
	updateBinCombinedMenu(bin);
	return m_binContextMenu;
}

QMenu * BinManager::partContextMenu(PartsBinPaletteWidget * bin) {
	updateBinCombinedMenu(bin);
	return m_partContextMenu;
}

QMenu * BinManager::combinedMenu(PartsBinPaletteWidget * bin) {
	updateBinCombinedMenu(bin);
	return m_combinedMenu;
}

QMenu * BinManager::combinedMenu() {
	return m_combinedMenu;
}

bool BinManager::removeSelected() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return false;

	ModelPart * mp = bin->selectedModelPart();
	if (mp == NULL) return false;

    if (m_mainWindow->anyUsePart(mp->moduleID())) {
        QMessageBox::warning(this, tr("Remove from Bin"), tr("Unable to remove part '%1'--it is in use in a sketch").arg(mp->title()));
        return false;
    }

	QMessageBox::StandardButton answer = QMessageBox::question(
		this,
		tr("Remove from bin"),
		tr("Do you really want to remove '%1' from the bin? This operation cannot be undone.").arg(mp->title()),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No
	);
	// TODO: make button texts translatable
	if(answer != QMessageBox::Yes) return false;


	m_undoStack->push(new QUndoCommand("Parts bin: part removed"));
	bin->removePart(mp->moduleID(), mp->path());
	bin->setDirty();

	return true;
}

void BinManager::findSelected() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	ModelPart * mp = bin->selectedModelPart();
	if (mp == NULL) return;

    m_mainWindow->selectPartsWithModuleID(mp);
}


void BinManager::exportSelected() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	ModelPart * mp = bin->selectedModelPart();
	if (mp == NULL) return;

	emit savePartAsBundled(mp->moduleID());
}

void BinManager::saveBundledBin() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	bin->saveBundledBin();
}

void BinManager::setTabIcon(PartsBinPaletteWidget* w, QIcon * icon) 
{
	if (m_stackTabWidget != NULL) {
		int tabIdx = m_stackTabWidget->indexOf(w);
		m_stackTabWidget->setTabIcon(tabIdx, *icon);
	}
}

void BinManager::copyFilesToContrib(ModelPart * mp, QWidget * originator) {
	PartsBinPaletteWidget * bin = qobject_cast<PartsBinPaletteWidget *>(originator);
	if (bin == NULL) return;

	if (bin->fileName().compare(m_tempPartsBinLocation) != 0) return;				// only copy from temp bin

	QString path = mp->path();
	if (path.isEmpty()) return;

	QFileInfo info(path);
	QFile fzp(path);
	
    QString parts = FolderUtils::getUserPartsPath();
    FolderUtils::slamCopy(fzp, parts + "/contrib/" + info.fileName());
	QString prefix = parts + "/svg/contrib/";

	QDir dir = info.absoluteDir();
	dir.cdUp();
	dir.cd("svg");
	dir.cd("contrib");

	QList<ViewLayer::ViewID> viewIDs;
	viewIDs << ViewLayer::IconView << ViewLayer::BreadboardView << ViewLayer::SchematicView << ViewLayer::PCBView;
	foreach (ViewLayer::ViewID viewID, viewIDs) {
		QString fn = mp->hasBaseNameFor(viewID);
		if (!fn.isEmpty()) {
			QFile svg(dir.absoluteFilePath(fn));
            FolderUtils::slamCopy(svg, prefix + fn);
		}
	}
}

bool BinManager::isTempPartsBin(PartsBinPaletteWidget * bin) {
    return bin->fileName().compare(m_tempPartsBinLocation) == 0;
}

void BinManager::setTempPartsBinLocation(const QString & name) {
    m_tempPartsBinLocation = name;
    //StandardBinIcons.insert(m_tempPartsBinLocation, "Temp.png");
}

void BinManager::mainLoad() {
	QString path = m_defaultSaveFolder;

	QString fileName = FolderUtils::getOpenFileName(
			this,
			tr("Select a Fritzing File to Open"),
			path,
			tr("Fritzing Files (*%1 *%2 *%3);;Fritzing Part (*%1);;Fritzing Bin (*%2);;Fritzing Shareable Bin (*%3)")
                .arg(FritzingBundledPartExtension)
                .arg(FritzingBinExtension)
                .arg(FritzingBundledBinExtension)
		);

    if (fileName.isEmpty()) return;

    if (fileName.endsWith(FritzingBundledPartExtension)) {
        importPartToCurrentBin(fileName);
        return;
    }

    if (fileName.endsWith(FritzingBinExtension) || fileName.endsWith(FritzingBundledBinExtension)) {
        openBinIn(fileName, false);
    }
}

void BinManager::hideTabBar()
{
    m_stackTabWidget->stackTabBar()->hide();
}

void BinManager::reloadPart(const QString & moduleID) {
	PartsBinView::removePartReference(moduleID);
	for(int j = 0; j < m_stackTabWidget->count(); j++) {
		PartsBinPaletteWidget *bin = qobject_cast<PartsBinPaletteWidget*>(m_stackTabWidget->widget(j));
		if (bin == NULL) continue;

		bin->reloadPart(moduleID);
	}
}

void BinManager::copyToSketch() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

    QList<ModelPart *> modelParts = bin->getAllParts();
    if (modelParts.count() == 0) return;

    if (m_mainWindow) {
        m_mainWindow->addToSketch(modelParts);
    }
}

void BinManager::copyAllToSketch() {
    QList<ModelPart *> modelParts;

    if (m_mainWindow) {
        m_mainWindow->addToSketch(modelParts);
    }
}

