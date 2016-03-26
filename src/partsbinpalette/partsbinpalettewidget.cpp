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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/


#include <QUndoStack>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QWidgetAction>
#include <QColorDialog>
#include <QBuffer>
#include <QSvgGenerator>

#include "partsbinpalettewidget.h"
#include "partsbincommands.h"
#include "partsbiniconview.h"
#include "partsbinlistview.h"
#include "searchlineedit.h"
#include "../utils/misc.h"
#include "../debugdialog.h"
#include "../infoview/htmlinfoview.h"
#include "../utils/fileprogressdialog.h"
#include "../utils/folderutils.h"
#include "../utils/textutils.h"
#include "../mainwindow/mainwindow.h"          // TODO: PartsBinPaletteWidget should not call MainWindow functions


static QString CustomIconName = "Custom1.png";
static QString CustomIconTitle = "Fritzing Custom Icon";

inline bool isCustomSvg(const QString & string) {
	return string.startsWith("<?xml") && string.contains(CustomIconTitle);
}

static QHash<QString, PaletteModel *> PaletteBinModels;

static QIcon EmptyIcon;

//////////////////////////////////////////////

PartsBinPaletteWidget::PartsBinPaletteWidget(ReferenceModel *referenceModel, HtmlInfoView *infoView, WaitPushUndoStack *undoStack, BinManager* manager) :
	QFrame(manager)
{
    m_binLabel = NULL;
	m_monoIcon = m_icon = NULL;
	m_searchLineEdit = NULL;
	m_saveQuietly = false;
	m_fastLoaded = false;
	m_model = NULL;

	m_loadingProgressDialog = NULL;

	setAcceptDrops(true);
	setAllowsChanges(true);

	m_manager = manager;

	m_referenceModel = referenceModel;
	m_canDeleteModel = false;
	m_orderHasChanged = false;

	Q_UNUSED(undoStack);

	m_undoStack = new WaitPushUndoStack(this);
	connect(m_undoStack, SIGNAL(cleanChanged(bool)), this, SLOT(undoStackCleanChanged(bool)) );

	m_iconView = new PartsBinIconView(m_referenceModel, this);
	m_iconView->setInfoView(infoView);

	m_listView = new PartsBinListView(m_referenceModel, this);
	m_listView->setInfoView(infoView);

	m_stackedWidget = new QStackedWidget(this);
	m_stackedWidget->addWidget(m_iconView);
	m_stackedWidget->addWidget(m_listView);

	QVBoxLayout * vbl = new QVBoxLayout(this);
    vbl->setMargin(3);
    vbl->setSpacing(0);

    m_header = NULL;
    setupHeader();
    if (m_header) {
	    vbl->addWidget(m_header);

	    QFrame * separator = new QFrame();
	    separator->setMaximumHeight(1);
	    separator->setObjectName("partsBinHeaderSeparator");
        separator->setFrameShape(QFrame::HLine);
        separator->setFrameShadow(QFrame::Plain);
	    vbl->addWidget(separator);
    }

	vbl->addWidget(m_stackedWidget);
	this->setLayout(vbl);

	setObjectName("partsBinContainer");
	toIconView();

    m_defaultSaveFolder = FolderUtils::getUserBinsPath();
	m_untitledFileName = tr("Untitled Bin");

	connect(m_listView, SIGNAL(currentRowChanged(int)), m_iconView, SLOT(setSelected(int)));
	connect(m_iconView, SIGNAL(selectionChanged(int)), m_listView, SLOT(setSelected(int)));

	connect(m_listView, SIGNAL(currentRowChanged(int)), m_manager, SLOT(updateBinCombinedMenuCurrent()));
	connect(m_iconView, SIGNAL(selectionChanged(int)), m_manager, SLOT(updateBinCombinedMenuCurrent()));

	connect(m_listView, SIGNAL(informItemMoved(int,int)), m_iconView, SLOT(itemMoved(int,int)));
	connect(m_iconView, SIGNAL(informItemMoved(int,int)), m_listView, SLOT(itemMoved(int,int)));
	connect(m_listView, SIGNAL(informItemMoved(int,int)), this, SLOT(itemMoved()));
	connect(m_iconView, SIGNAL(informItemMoved(int,int)), this, SLOT(itemMoved()));

	if (m_binLabel) m_binLabel->setText(m_title);

	m_addPartToMeAction = new QAction(m_title,this);
	connect(m_addPartToMeAction, SIGNAL(triggered()),this, SLOT(addSketchPartToMe()));

	installEventFilter(this);
}

PartsBinPaletteWidget::~PartsBinPaletteWidget() {
	if (m_canDeleteModel && m_model != NULL) {
		delete m_model;
		m_model = NULL;
	}
   
    if (m_icon) delete m_icon;
    if (m_monoIcon) delete m_monoIcon;
}

void PartsBinPaletteWidget::cleanup() {
    foreach (PaletteModel * paletteModel, PaletteBinModels) {
        delete paletteModel;
    }
    PaletteBinModels.clear();
}


QSize PartsBinPaletteWidget::sizeHint() const {
	return QSize(DockWidthDefault, PartsBinHeightDefault);
}

QString PartsBinPaletteWidget::title() const {
	return m_title;
}

void PartsBinPaletteWidget::setTitle(const QString &title) {
	if(m_title != title) {
		m_title = title;
		if (m_binLabel) m_binLabel->setText(title);
	}
}

void PartsBinPaletteWidget::setupHeader()
{
    QMenu * combinedMenu = m_manager->combinedMenu();
    if (combinedMenu == NULL) return;

	m_combinedBinMenuButton = newToolButton("partsBinCombinedMenuButton", ___emptyString___, ___emptyString___);
	m_combinedBinMenuButton->setMenu(combinedMenu);

	m_binLabel = new QLabel(this);
	m_binLabel->setObjectName("partsBinLabel");
	m_binLabel->setWordWrap(false);

	m_searchLineEdit = new SearchLineEdit(this);
    m_searchLineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	connect(m_searchLineEdit, SIGNAL(returnPressed()), this, SLOT(search()));

	m_searchStackedWidget = new QStackedWidget(this);
	m_searchStackedWidget->setObjectName("searchStackedWidget");

	m_searchStackedWidget->addWidget(m_binLabel);
	m_searchStackedWidget->addWidget(m_searchLineEdit);

	m_header = new QFrame(this);
	m_header->setObjectName("partsBinHeader");
	QHBoxLayout * hbl = new QHBoxLayout();
    hbl->setSpacing(0);
    hbl->setMargin(0);

	hbl->addWidget(m_searchStackedWidget);
	hbl->addWidget(m_combinedBinMenuButton);

	m_header->setLayout(hbl);
}

void PartsBinPaletteWidget::setView(PartsBinView *view) {
	m_currentView = view;
	if(m_currentView == m_iconView) {
		m_stackedWidget->setCurrentIndex(0);
		m_manager->updateViewChecks(true);
	} 
	else {
		m_stackedWidget->setCurrentIndex(1);
		m_manager->updateViewChecks(false);
	}

}

void PartsBinPaletteWidget::toIconView() {
	setView(m_iconView);
}

void PartsBinPaletteWidget::toListView() {
	disconnect(m_listView, SIGNAL(currentRowChanged(int)), m_iconView, SLOT(setSelected(int)));
	setView(m_listView);
	connect(m_listView, SIGNAL(currentRowChanged(int)), m_iconView, SLOT(setSelected(int)));
}

bool PartsBinPaletteWidget::saveAsAux(const QString &filename) {
    FileProgressDialog progress("Saving...", 0, this);

	QString oldFilename = m_fileName;
	setFilename(filename);
	QString title = this->title();
	if(!title.isNull() && !title.isEmpty()) {
		ModelPartSharedRoot * root = m_model->rootModelPartShared();
		if (root) root->setTitle(title);
	}

	if(m_orderHasChanged) {
		m_model->setOrdererChildren(m_iconView->orderedChildren());
	}
	m_model->save(filename, false);
	if(m_orderHasChanged) {
		m_model->setOrdererChildren(QList<QObject*>());
		m_orderHasChanged = false;
	}

	m_undoStack->setClean();

	m_location = BinLocation::findLocation(filename);

	if(oldFilename != m_fileName) {
		emit fileNameUpdated(this,m_fileName,oldFilename);
	}
	emit saved(hasAlienParts());

    foreach (QString path, m_removed) {
        bool result = QFile::remove(path);
        if (!result) {
            DebugDialog::debug("unable to delete '" + path + "' from bin");
        }
    }
    m_removed.clear();

	return true;
}

void PartsBinPaletteWidget::loadFromModel(PaletteModel *model) {
	m_iconView->loadFromModel(model);
	m_listView->setPaletteModel(model);
	afterModelSetted(model);
	m_canDeleteModel = false;				// FApplication is holding this model, so don't delete it
	setFilename(model->loadedFrom());
}

void PartsBinPaletteWidget::setPaletteModel(PaletteModel *model, bool clear) {
	m_iconView->setPaletteModel(model, clear);
	m_listView->setPaletteModel(model, clear);
	afterModelSetted(model);
}

void PartsBinPaletteWidget::afterModelSetted(PaletteModel *model) {
	grabTitle(model);
	m_model = model;
	m_undoStack->setClean();
	setFilename(model->loadedFrom());
}

void PartsBinPaletteWidget::grabTitle(PaletteModel *model) {
	ModelPartSharedRoot * root = model->rootModelPartShared();
	if (!root) return;

	QString iconFilename = root->icon();
	grabTitle(root->title(), iconFilename);
	root->setIcon(iconFilename);

	if (m_searchLineEdit) m_searchLineEdit->setText(root->searchTerm());
}

void PartsBinPaletteWidget::grabTitle(const QString & title, QString & iconFilename)
{
	m_title = title;
	m_addPartToMeAction->setText(m_title);
	if (m_binLabel) m_binLabel->setText(m_title);

	QString temp = BinManager::StandardBinIcons.value(m_fileName, "");
	if (!temp.isEmpty()) {
		iconFilename = temp;
	}
	else if (iconFilename.isEmpty()) {
		iconFilename = CustomIconName;
	}
	
	if (isCustomSvg(iconFilename)) {
		// convert to image
		int w = TextUtils::getViewBoxCoord(iconFilename, 2);
		int h = TextUtils::getViewBoxCoord(iconFilename, 3);
		QImage image(w, h, QImage::Format_ARGB32);
		image.fill(0);
		QRectF target(0, 0, w, h);
		QSvgRenderer renderer(iconFilename.toUtf8());
		QPainter painter;
		painter.begin(&image);
		renderer.render(&painter, target);
		painter.end();	
		//image.save(FolderUtils::getUserDataStorePath("") + "/test icon.png");
		m_icon = new QIcon(QPixmap::fromImage(image));
        m_monoIcon = new QIcon(":resources/bins/icons/Custom1-mono.png");

        // TODO: hack svg to make a mono icon
	}
	else {
        QFileInfo info(m_fileName);
        QDir dir = info.absoluteDir();
        QString path = dir.absoluteFilePath(iconFilename);
        QFile file1(path);
        if (file1.exists()) {
            m_icon = new QIcon(path);
        }
        else {
            path = ":resources/bins/icons/" + iconFilename;
		    QFile file2(path);
		    if (file2.exists()) {
			    m_icon = new QIcon(path);
		    }
        }

        if (m_icon) {
            int ix = path.lastIndexOf(".");
            path.insert(ix, "-mono");
            QFile file3(path);
            if (file3.exists()) {
                m_monoIcon = new QIcon(path);
            }
        }
	}
}

void PartsBinPaletteWidget::addPart(ModelPart *modelPart, int position) {
    if (m_model == NULL) {
        return;
    }

	ModelPart *mp = m_model->addModelPart(m_model->root(),modelPart);

	m_iconView->addPart(mp, position);
	m_listView->addPart(mp, position);

	if(modelPart->isAlien()) {
		m_alienParts << mp->moduleID();
	}
}

QToolButton* PartsBinPaletteWidget::newToolButton(const QString& btnObjName, const QString& imgPath, const QString &text) {
	QToolButton *toolBtn = new QToolButton(this);
	toolBtn->setObjectName(btnObjName);
	toolBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
	toolBtn->setArrowType(Qt::NoArrow);
	if (!text.isEmpty()) {
		toolBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		toolBtn->setText(text);
	}
	toolBtn->setPopupMode(QToolButton::InstantPopup);
	if (!imgPath.isEmpty()) {
		toolBtn->setIcon(QIcon(imgPath));
	}
	toolBtn->setArrowType(Qt::NoArrow);
	return toolBtn;
}

bool PartsBinPaletteWidget::save() {
	bool result = true;
	if (FolderUtils::isEmptyFileName(m_fileName,m_untitledFileName) || currentBinIsCore()) {
		result = saveAs();
	} else {
		saveAsAux(m_fileName);
	}
	return result;
}

bool PartsBinPaletteWidget::saveAs() {
	QString fileExt;
    QString fileName = QFileDialog::getSaveFileName(
		this,
		tr("Specify a file name"),
		(m_fileName.isNull() || m_fileName.isEmpty() || /* it's a resource */ m_fileName.startsWith(":"))?
				m_defaultSaveFolder+"/"+title()+FritzingBinExtension:
				m_fileName,
		tr("Fritzing Bin (*%1)").arg(FritzingBinExtension),
		&fileExt
	  );

    if (fileName.isEmpty()) return false; // Cancel pressed

    if(!FritzingWindow::alreadyHasExtension(fileName, FritzingBinExtension)) {
		fileName += FritzingBinExtension;
	}
    saveAsAux(fileName);
    return true;
}

void PartsBinPaletteWidget::saveBundledBin() {
	bool wasModified = m_isDirty;
	m_manager->mainWindow()->saveBundledNonAtomicEntity(
		m_fileName, FritzingBundledBinExtension, this,
		m_model->root()->getAllNonCoreParts(), true, "", true, false
	);
	setDirty(wasModified);
}

bool PartsBinPaletteWidget::loadBundledAux(QDir &unzipDir, QList<ModelPart*> mps) {
	QStringList namefilters;
	namefilters << "*"+FritzingBinExtension;

	this->load(unzipDir.entryInfoList(namefilters)[0].filePath(), this, false);
	foreach(ModelPart* mp, mps) {
		if(mp->isAlien()) { // double check
			m_alienParts << mp->moduleID();
		}
	}
	setFilename(___emptyString___);

	return true;
}


bool PartsBinPaletteWidget::open(QString fileName, QWidget * progressTarget, bool fastLoad) {
	QFile file(fileName);
	if (!file.exists()) {
       QMessageBox::warning(NULL, tr("Fritzing"),
                             tr("Cannot find file %1.")
                             .arg(fileName));
		return false;
	}

    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(NULL, tr("Fritzing"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    file.close();

    if(fileName.endsWith(FritzingBinExtension)) {
    	load(fileName, progressTarget, fastLoad);
    	m_isDirty = false;
    } else if(fileName.endsWith(FritzingBundledBinExtension)) {
    	return m_manager->mainWindow()->loadBundledNonAtomicEntity(fileName,this,false, false);
    }

    return true;
}

void PartsBinPaletteWidget::load(const QString &filename, QWidget * progressTarget, bool fastLoad) {
	// TODO deleting this local palette reference model deletes modelPartShared held by the palette bin modelParts
	//PaletteModel * paletteReferenceModel = new PaletteModel(true, true);

	m_location = BinLocation::findLocation(filename);

	//DebugDialog::debug("loading bin");

	if (fastLoad) {
		QString binName, iconName;
		if (BinManager::getBinTitle(filename, binName, iconName)) {
			m_fileName = filename;
			grabTitle(binName, iconName);
			m_fastLoaded = true;
		}
		return;
	}

	m_fastLoaded = false;
    PaletteModel * paletteBinModel = PaletteBinModels.value(filename);
    if (paletteBinModel == NULL) {
	    paletteBinModel = new PaletteModel(true, false);
	    //DebugDialog::debug("after palette model");

	    QString name = m_title;
	    if (name.isEmpty()) name = QFileInfo(filename).completeBaseName();

	    bool deleteWhenDone = false;
        if (progressTarget != NULL) {
            //DebugDialog::debug("open progress " + filename);
		    deleteWhenDone = true;
            progressTarget = m_loadingProgressDialog = new FileProgressDialog(tr("Loading..."), 200, progressTarget);
		    m_loadingProgressDialog->setBinLoadingChunk(200);
		    m_loadingProgressDialog->setBinLoadingCount(1);
		    m_loadingProgressDialog->setMessage(tr("loading bin '%1'").arg(name));
		    m_loadingProgressDialog->show();
	    }
	
	    if (progressTarget) {
		    connect(paletteBinModel, SIGNAL(loadingInstances(ModelBase *, QDomElement &)), progressTarget, SLOT(loadingInstancesSlot(ModelBase *, QDomElement &)));
		    connect(paletteBinModel, SIGNAL(loadingInstance(ModelBase *, QDomElement &)), progressTarget, SLOT(loadingInstanceSlot(ModelBase *, QDomElement &)));
		    connect(m_iconView, SIGNAL(settingItem()), progressTarget, SLOT(settingItemSlot()));
		    connect(m_listView, SIGNAL(settingItem()), progressTarget, SLOT(settingItemSlot()));
	    }
	    DebugDialog::debug(QString("loading bin '%1'").arg(name));
	    bool result = paletteBinModel->loadFromFile(filename, m_referenceModel, false);
	    //DebugDialog::debug(QString("done loading bin '%1'").arg(name));

	    if (!result) {
		    QMessageBox::warning(NULL, QObject::tr("Fritzing"), QObject::tr("Fritzing cannot load the parts bin"));
	    }
	    else {
		    m_fileName = filename;
		    setPaletteModel(paletteBinModel,true);
            PaletteBinModels.insert(filename, paletteBinModel);
	    }

	    if (progressTarget) {
            //DebugDialog::debug("close progress " + filename);
		    disconnect(paletteBinModel, SIGNAL(loadingInstances(ModelBase *, QDomElement &)), progressTarget, SLOT(loadingInstancesSlot(ModelBase *, QDomElement &)));
		    disconnect(paletteBinModel, SIGNAL(loadingInstance(ModelBase *, QDomElement &)), progressTarget, SLOT(loadingInstanceSlot(ModelBase *, QDomElement &)));
		    disconnect(m_iconView, SIGNAL(settingItem()), progressTarget, SLOT(settingItemSlot()));
		    disconnect(m_listView, SIGNAL(settingItem()), progressTarget, SLOT(settingItemSlot()));
		    if (deleteWhenDone) {
			    m_loadingProgressDialog->close();
			    delete m_loadingProgressDialog;
		    }
		    m_loadingProgressDialog = NULL;
	    }
    }
    else {
		m_fileName = filename;
		setPaletteModel(paletteBinModel,true);
    }


	//DebugDialog::debug("done loading bin");
	//delete paletteReferenceModel;
}

void PartsBinPaletteWidget::undoStackCleanChanged(bool isClean) {
	if(!isClean && currentBinIsCore()) {
		setFilename(QString::null);
	}
	setWindowModified(!isClean);
	m_manager->setDirtyTab(this,isClean);
}

bool PartsBinPaletteWidget::currentBinIsCore() {
    return m_fileName == BinManager::CorePartsBinLocation;
}

bool PartsBinPaletteWidget::beforeClosing() {
	bool retval;
	if (this->isWindowModified()) {
		QMessageBox::StandardButton reply;
		if (m_saveQuietly) {
			reply = QMessageBox::Save;
		}
		else {
            QMessageBox messageBox(this);
            messageBox.setWindowTitle(tr("Save bin \"%1\"").arg(title()));
            messageBox.setText(tr("Do you want to save the changes you made in the bin \"%1\"?").arg(title()));
            messageBox.setInformativeText(tr("Your changes will be lost if you don't save them."));
            messageBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            messageBox.setDefaultButton(QMessageBox::Save);
            messageBox.setIcon(QMessageBox::Warning);
            messageBox.setWindowModality(Qt::WindowModal);
            messageBox.setButtonText(QMessageBox::Save, tr("Save"));
            messageBox.setButtonText(QMessageBox::Discard, tr("Don't Save"));
            messageBox.button(QMessageBox::Discard)->setShortcut(tr("Ctrl+D"));
            messageBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));

			reply = (QMessageBox::StandardButton)messageBox.exec();
		}

     	if (reply == QMessageBox::Save) {
     		retval = save();
    	} else if (reply == QMessageBox::Discard) {
    		retval = true;
        } else {
         	retval = false;
        }
	} else {
		retval = true;
	}
	return retval;
}

void PartsBinPaletteWidget::closeEvent(QCloseEvent* event) {
	QFrame::closeEvent(event);
}

void PartsBinPaletteWidget::mousePressEvent(QMouseEvent* event) {
	emit focused(this);
	QFrame::mousePressEvent(event);
}

ModelPart * PartsBinPaletteWidget::selectedModelPart() {
	return m_currentView->selectedModelPart();
}

ItemBase * PartsBinPaletteWidget::selectedItemBase() {
	return m_currentView->selectedItemBase();
}

bool PartsBinPaletteWidget::contains(const QString &moduleID) {
	return m_iconView->contains(moduleID);
}

bool PartsBinPaletteWidget::hasAlienParts() {
	return m_alienParts.size() > 0;
}

void PartsBinPaletteWidget::addPart(const QString& moduleID, int position) {
	ModelPart *modelPart = m_referenceModel->retrieveModelPart(moduleID);
	addPart(modelPart, position);
}

void PartsBinPaletteWidget::removePart(const QString & moduleID, const QString & path) {
	m_iconView->removePart(moduleID);
	m_listView->removePart(moduleID);

	// remove the model part from the model last, as this deletes it,
	// and the removePart calls above still need the modelpart
	m_model->removePart(moduleID);
    if (path.contains(FolderUtils::getUserPartsPath())) {
        m_removed << path;
    }
}


void PartsBinPaletteWidget::removeParts() {
    m_iconView->removeParts();
    m_listView->removeParts();

    // remove the model part from the model last, as this deletes it,
    // and the removePart calls above still need the modelpart
    m_model->removeParts();
}

void PartsBinPaletteWidget::removeAlienParts() {
	foreach(QString moduleID, m_alienParts) {
		removePart(moduleID, "");
	}
	m_alienParts.clear();
}

void PartsBinPaletteWidget::setInfoViewOnHover(bool infoViewOnHover) {
	if(m_iconView) m_iconView->setInfoViewOnHover(infoViewOnHover);
	if(m_listView) m_listView->setInfoViewOnHover(infoViewOnHover);
}

void PartsBinPaletteWidget::addPartCommand(const QString& moduleID) {
	/*bool updating = alreadyIn(moduleID);

	QString partTitle = m_referenceModel->partTitle(moduleID);
	if(partTitle.isEmpty()) partTitle = moduleID;

	QString undoStackMsg;

	if(!updating) {
		undoStackMsg = tr("\"%1\" added to bin").arg(partTitle);
	} else {
		undoStackMsg = tr("\"%1\" updated in bin").arg(partTitle);
	}
	QUndoCommand *parentCmd = new QUndoCommand(undoStackMsg);

	int index = m_listView->position(moduleID);
	new PartsBinAddCommand(this, moduleID, index, parentCmd);
	m_undoStack->push(parentCmd);*/

	QMessageBox::StandardButton answer = QMessageBox::question(
		this,
		tr("Add to bin"),
		tr("Do you really want to add the selected part to the bin?"),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::Yes
	);
	// TODO: make button texts translatable
	if(answer == QMessageBox::Yes) {
		int index = m_listView->position(moduleID);
		m_undoStack->push(new QUndoCommand("Parts bin: part added"));
		addPart(moduleID, index);
	}
}

void PartsBinPaletteWidget::itemMoved() {
	m_orderHasChanged = true;
	m_manager->setDirtyTab(this);
}

void PartsBinPaletteWidget::setDirty(bool dirty) {
	m_manager->setDirtyTab(this, dirty);
	m_isDirty = dirty;
}

const QString &PartsBinPaletteWidget::fileName() {
	return m_fileName;
}

bool PartsBinPaletteWidget::eventFilter(QObject *obj, QEvent *event) {
	if (obj == this) {
		if (event->type() == QEvent::MouseButtonPress ||
			event->type() == QEvent::GraphicsSceneDragMove ||
			event->type() == QEvent::GraphicsSceneDrop ||
			event->type() == QEvent::GraphicsSceneMousePress
		) {
			emit focused(this);
		}
	}
	return QFrame::eventFilter(obj, event);
}

PartsBinView *PartsBinPaletteWidget::currentView() {
	return m_currentView;
}

void PartsBinPaletteWidget::dragEnterEvent(QDragEnterEvent *event) {
	QFrame::dragEnterEvent(event);
}

void PartsBinPaletteWidget::dragLeaveEvent(QDragLeaveEvent *event) {
	QFrame::dragLeaveEvent(event);
}

void PartsBinPaletteWidget::dragMoveEvent(QDragMoveEvent *event) {
	QFrame::dragMoveEvent(event);
}

void PartsBinPaletteWidget::dropEvent(QDropEvent *event) {
	QFrame::dropEvent(event);
}


QAction *PartsBinPaletteWidget::addPartToMeAction() {
	return m_addPartToMeAction;
}

void PartsBinPaletteWidget::addSketchPartToMe() {
    m_manager->openBinIn(this->m_fileName, false);
	QString moduleID = m_manager->getSelectedModuleIDFromSketch();
    if (moduleID.isEmpty()) return;

	bool wasAlreadyIn = contains(moduleID);
	addPart(moduleID, -1);
	if(!wasAlreadyIn) {
		setDirty();
	}
}

void PartsBinPaletteWidget::setFilename(const QString &filename) {
	m_fileName = filename;
	if (m_fileName.compare(BinManager::SearchBinLocation) == 0) {
		m_searchStackedWidget->setCurrentIndex(1);
	}
	bool acceptIt = !currentBinIsCore();
	setAcceptDrops(acceptIt);
	m_iconView->setAcceptDrops(acceptIt);
	m_listView->setAcceptDrops(acceptIt);
}

void PartsBinPaletteWidget::search() {
	SearchLineEdit * edit = qobject_cast<SearchLineEdit *>(sender());
	if (edit == NULL) return;

	QString searchText = edit->text();
	if (searchText.isEmpty()) return;

	ModelPartSharedRoot * root = m_model->rootModelPartShared();
	if (root) {
		root->setSearchTerm(searchText);
	}

    m_manager->search(searchText);
}

bool PartsBinPaletteWidget::allowsChanges() {
	return m_allowsChanges;
}

bool PartsBinPaletteWidget::readOnly() {
	return !allowsChanges();
}

void PartsBinPaletteWidget::setAllowsChanges(bool allowsChanges) {
	m_allowsChanges = allowsChanges;
}

void PartsBinPaletteWidget::setReadOnly(bool readOnly) {
	setAllowsChanges(!readOnly);
}

void PartsBinPaletteWidget::focusSearch() {
	if (m_searchLineEdit) {
		QTimer::singleShot(20, this, SLOT(focusSearchAfter()));
	}
}

void PartsBinPaletteWidget::focusSearchAfter() {
	//DebugDialog::debug("focus search after");
	if (m_searchLineEdit->decoy()) {
		m_searchLineEdit->setDecoy(false);
	}
	m_searchLineEdit->setFocus(Qt::OtherFocusReason);
}

void PartsBinPaletteWidget::setSaveQuietly(bool saveQuietly) {
	m_saveQuietly = saveQuietly;
}

bool PartsBinPaletteWidget::currentViewIsIconView() {
	if (m_currentView == NULL) return true;

	return (m_currentView == m_iconView);
}

QIcon PartsBinPaletteWidget::icon() {
	if (m_icon) return *m_icon;

	return EmptyIcon;
}

bool PartsBinPaletteWidget::hasMonoIcon() {
    return m_monoIcon != NULL;
}

QIcon PartsBinPaletteWidget::monoIcon() {
	if (m_monoIcon) return *m_monoIcon;

	return EmptyIcon;
}

QMenu * PartsBinPaletteWidget::combinedMenu()
{
	return m_manager->combinedMenu(this);
}

QMenu * PartsBinPaletteWidget::partContextMenu()
{
	return m_manager->partContextMenu(this);
}

QMenu * PartsBinPaletteWidget::binContextMenu()
{
	QMenu * menu = m_manager->binContextMenu(this);
	if (menu == NULL) return NULL;

	QMenu * newMenu = new QMenu();
	foreach (QAction * action, menu->actions()) {
		newMenu->addAction(action);
	}

	// TODO: need to enable/disable actions based on this bin

	ModelPartSharedRoot * root = (m_model == NULL) ? NULL : m_model->rootModelPartShared();
	if (root) {
		QString iconFilename = root->icon();
		if (iconFilename.compare(CustomIconName) == 0 || isCustomSvg(iconFilename)) {
			newMenu->addSeparator();
			QAction * action = new QAction(tr("Change icon color..."), newMenu);
			action->setToolTip(tr("Change the color of the icon for this bin."));
			connect(action, SIGNAL(triggered()), this, SLOT(changeIconColor()));
			newMenu->addAction(action);
		}
	}
	return newMenu;
}

void PartsBinPaletteWidget::changeIconColor() {
	QImage image(":resources/bins/icons/" + CustomIconName);
	QColor initial(image.pixel(image.width() / 2, image.height() / 2));
	QColor color = QColorDialog::getColor(initial, this, tr("Select a color for this icon"), 0 );
	if (!color.isValid()) return;

	QRgb match = initial.rgba();
	for (int y = 0; y < image.height(); y++) {
		for (int x = 0; x < image.width(); x++) {
			QRgb rgb = image.pixel(x, y);
			if (qRed(rgb) == qRed(match) && qBlue(rgb) == qBlue(match) && qGreen(rgb) == qGreen(match)) {
				image.setPixel(x, y, (color.rgb() & 0xffffff) | (qAlpha(rgb) << 24));
			}
		}
	}

#ifndef QT_NO_DEBUG
    //QFileInfo info(m_fileName);
    //image.save(FolderUtils::getUserDataStorePath("") + "/" + info.completeBaseName() + ".png");
#endif

	delete m_icon;
	m_icon = new QIcon(QPixmap::fromImage(image));

	m_manager->setTabIcon(this, m_icon);
	setDirty();

	ModelPartSharedRoot * root = m_model->rootModelPartShared();

	if (root) {
		QSvgGenerator svgGenerator;
		svgGenerator.setResolution(90);
		svgGenerator.setTitle(CustomIconTitle);
		QBuffer buffer;
		svgGenerator.setOutputDevice(&buffer);
		QSize sz = image.size();
		svgGenerator.setSize(sz);
		svgGenerator.setViewBox(QRect(0, 0, sz.width(), sz.height()));
		QPainter svgPainter(&svgGenerator);
		svgPainter.drawImage(QPoint(0,0), image);
		svgPainter.end();
		QString svg(buffer.buffer());
		root->setIcon(svg);
	}
}

bool PartsBinPaletteWidget::fastLoaded() {
	return m_fastLoaded;
}

BinLocation::Location PartsBinPaletteWidget::location() {
	return m_location;
}

bool PartsBinPaletteWidget::canClose() {
	switch (m_location) {
	case BinLocation::User:
		if (m_fileName.compare(BinManager::SearchBinLocation) == 0) return false;
		if (m_fileName.compare(BinManager::ContribPartsBinLocation) == 0) return false;
		if (m_fileName.compare(BinManager::MyPartsBinLocation) == 0) return false;	
		if (m_manager->isTempPartsBin(this)) return false;	
		return true;
	case BinLocation::More:
		return false;
	case BinLocation::App:
		return false;
	case BinLocation::Outside:
	default:
		return true;
	}
}

void PartsBinPaletteWidget::copyFilesToContrib(ModelPart * mp, QWidget * originator) {
	m_manager->copyFilesToContrib(mp, originator);
}

ModelPart * PartsBinPaletteWidget::root() {
    if (m_model == NULL) return NULL;

    return m_model->root();
}

bool PartsBinPaletteWidget::isTempPartsBin() {
    return m_manager->isTempPartsBin(this);
}

void PartsBinPaletteWidget::reloadPart(const QString & moduleID) {
	m_iconView->reloadPart(moduleID);
	m_listView->reloadPart(moduleID);
}

QList<ModelPart *> PartsBinPaletteWidget::getAllParts() {
    QList<ModelPart*> empty;
    if (m_model == NULL) return empty;

    if (m_model->root() == NULL) return empty;

    return m_model->root()->getAllParts();
}


