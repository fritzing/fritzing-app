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



#include "partseditormainwindow.h"
#include "pcbxml.h"
#include "../debugdialog.h"
#include "../mainwindow/fdockwidget.h"
#include "../waitpushundostack.h"
#include "editabletextwidget.h"
#include "partseditorview.h"
#include "partseditorviewswidget.h"
#include "editabledatewidget.h"
#include "hashpopulatewidget.h"
#include "partspecificationswidget.h"
#include "partconnectorswidget.h"
#include "../model/palettemodel.h"
#include "../model/sketchmodel.h"
#include "../utils/folderutils.h"
#include "../processeventblocker.h"

#include <QtGui>
#include <QCryptographicHash>
#include <QRegExpValidator>
#include <QRegExp>
#include <stdlib.h>


QString PartsEditorMainWindow::TitleFreshStartText;
QString PartsEditorMainWindow::LabelFreshStartText;
QString PartsEditorMainWindow::DescriptionFreshStartText;
QString PartsEditorMainWindow::TaxonomyFreshStartText;
QString PartsEditorMainWindow::TagsFreshStartText;
QString PartsEditorMainWindow::FooterText;
QString PartsEditorMainWindow::UntitledPartName;
QString PartsEditorMainWindow::___partsEditorName___;

int PartsEditorMainWindow::UntitledPartIndex = 1;
QPointer<PartsEditorMainWindow> PartsEditorMainWindow::m_lastOpened = NULL;
int PartsEditorMainWindow::m_closedBeforeCount = 0;
bool PartsEditorMainWindow::m_closeAfterSaving = true;

#ifdef QT_NO_DEBUG
	#define CORE_EDITION_ENABLED false
#else
	#define CORE_EDITION_ENABLED false
#endif


PartsEditorMainWindow::PartsEditorMainWindow(QWidget *parent)
	: FritzingWindow(untitledFileName(), untitledFileCount(), fileExtension(), parent)
{
	m_iconItem = m_breadboardItem = m_schematicItem = m_pcbItem = NULL;
	m_editingAlien = false;
}

PartsEditorMainWindow::~PartsEditorMainWindow()
{
	if (m_iconItem) {
		// m_iconItem was custom-created for the PartsEditor; the other items exist in the sketch so shouldn't be deleted
		delete m_iconItem;
	}

	if (m_sketchModel) {
		// memory leak here, but delete or deleteLater causes a crash if you're editing an already existing part;  a new part seems ok
		delete m_sketchModel;
	}

	if (m_paletteModel) {
		delete m_paletteModel;
	}
}

void PartsEditorMainWindow::initText() {
	UntitledPartName = tr("Untitled Part");
	TitleFreshStartText = tr("Please find a name for me!");
	LabelFreshStartText = tr("Please provide a label");
	DescriptionFreshStartText = tr("You could tell a little bit about this part");
	TaxonomyFreshStartText = tr("Please classify this part");
	TagsFreshStartText = tr("You can add your tags to make searching easier");
	FooterText = tr("<i>created by</i> %1 <i>on</i> %2");
	___partsEditorName___ = tr("Parts Editor");

}

void PartsEditorMainWindow::setup(long id, ModelPart *modelPart, bool fromTemplate, ItemBase * fromItem)
{
	ModelPart * originalModelPart = NULL;
    QFile styleSheet(":/resources/styles/partseditor.qss");
    m_mainFrame = new QFrame(this);
    m_mainFrame->setObjectName("partsEditor");

    m_editingAlien = modelPart? modelPart->isAlien(): false;

    if (!styleSheet.open(QIODevice::ReadOnly)) {
        qWarning("Unable to open :/resources/styles/partseditor.qss");
    } else {
    	m_mainFrame->setStyleSheet(styleSheet.readAll());
    }

    resize(500,700);

	m_id = id;
	m_partUpdated = false;
	m_savedAsNewPart = false;

	setAttribute(Qt::WA_DeleteOnClose, true);

	m_paletteModel = new PaletteModel(false, false);

	if(modelPart == NULL){
		m_lastOpened = this;
		m_updateEnabled = true;
	} else {
		// user only allowed to save parts, once he has saved it as a new one
		m_updateEnabled = modelPart->isCore()?
							CORE_EDITION_ENABLED:
							(modelPart->isContrib() || modelPart->isAlien()? false: true);
		m_fwFilename = modelPart->path();
		setTitle();
		UntitledPartIndex--; // TODO Mariano: not good enough
	}

	if(!fromTemplate) {
		m_sketchModel = new SketchModel(true);
	} else {
		ModelPart * mp = m_paletteModel->loadPart(m_fwFilename, false);
	    // this seems hacky but maybe it's ok
		if (mp == NULL || mp->modelPartShared() == NULL) {
			QMessageBox::critical(this, tr("Parts Editor"),
	                           tr("Error! Cannot load part.\n"),
	                           QMessageBox::Close);
			QTimer::singleShot(60, this, SLOT(close()));
			return;
		}

		QHash<QString,QString> properties = mp->modelPartShared()->properties();
		foreach (QString key, properties.keys()) {
			QVariant prop = modelPart->localProp(key);
			if (!prop.isNull() && prop.isValid()) {
				QString p = prop.toString();
				if (!p.isEmpty()) {
					mp->modelPartShared()->properties()[key] = p;
					if (key.compare("chip label") == 0) {
						mp->modelPartShared()->properties()["family"] = p;
					}
				}
			}
		}

		originalModelPart = modelPart;
		modelPart = mp;
		m_sketchModel = new SketchModel(modelPart);
	}

	ModelPart *mp = fromTemplate ? modelPart : NULL;

	createHeader(mp);
	createCenter(mp, fromItem);
	createFooter();

	// copy connector local names here if any
	if (originalModelPart) {
		foreach (Connector * fromConnector, originalModelPart->connectors()) {
			foreach (Connector * toConnector, mp->connectors()) {
				if (toConnector->connectorSharedID().compare(fromConnector->connectorSharedID()) == 0) {
					toConnector->setConnectorLocalName(fromConnector->connectorLocalName());
				}
			}
		}
	}

	layout()->setMargin(0);
	layout()->setSpacing(0);


	QGridLayout *layout = new QGridLayout(m_mainFrame);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(m_headerFrame,0,0);
	layout->addWidget(m_centerFrame,1,0);
	layout->addWidget(m_footerFrame,2,0);
	setCentralWidget(m_mainFrame);

    if(fromTemplate) {
		m_views->setViewItems(m_breadboardItem, m_schematicItem, m_pcbItem);
    	m_views->loadViewsImagesFromModel(m_paletteModel, m_sketchModel->root());
    }

    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

	QSettings settings;
	if (!settings.value("peditor/state").isNull()) {
		restoreState(settings.value("peditor/state").toByteArray());
	}
	if (!settings.value("peditor/geometry").isNull()) {
		restoreGeometry(settings.value("peditor/geometry").toByteArray());
	}

	installEventFilter(this);

}

void PartsEditorMainWindow::createHeader(ModelPart *modelPart) {
	m_headerFrame = new QFrame();
	m_headerFrame->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed));
	m_headerFrame->setObjectName("header");
	m_headerFrame->setStyleSheet("padding: 2px; padding-bottom: 0;");

	int iconViewSize = 50;
	QGraphicsProxyWidget *startItem = modelPart? NULL: PartsEditorMainWindow::emptyViewItem("icon_icon.png",___emptyString___);
	m_iconViewImage = new PartsEditorView(
		ViewLayer::IconView, createTempFolderIfNecessary(), false, startItem, m_headerFrame, iconViewSize
	);
	m_iconViewImage->setViewItem(m_iconItem);
	m_iconViewImage->setFixedSize(iconViewSize,iconViewSize);
	m_iconViewImage->setObjectName("iconImage");
	m_iconViewImage->setSketchModel(m_sketchModel);
	m_iconViewImage->setUndoStack(m_undoStack);
	m_iconViewImage->addViewLayer(new ViewLayer(ViewLayer::Icon, true, 2.5));
	m_iconViewImage->setViewLayerIDs(ViewLayer::Icon, ViewLayer::Icon, ViewLayer::Icon, ViewLayer::Icon, ViewLayer::Icon);
	if(modelPart) {
		m_iconViewImage->loadFromModel(m_paletteModel, modelPart);
	}

	QString linkStyle = "font-size: 10px; color: #7070ff;";
	QLabel *button = new QLabel(
						QString("<a style='%2' href='#'>%1</a>")
							.arg(tr("Load icon.."))
							.arg(linkStyle),
						this);
	button->setObjectName("iconBrowseButton");
	//button->setFixedWidth(iconViewSize);
	button->setFixedHeight(20);
	//m_iconViewImage->addFixedToBottomRight(button);
	connect(button, SIGNAL(linkActivated(const QString&)), m_iconViewImage, SLOT(loadFile()));

	QString title = modelPart ? modelPart->title() : TitleFreshStartText;
	m_title = new EditableLineWidget(title,m_undoStack,m_headerFrame,"",modelPart,true);
	m_title->setObjectName("partTitle");
	m_title->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::MinimumExpanding));

	QGridLayout *headerLayout = new QGridLayout();
	headerLayout->addWidget(m_iconViewImage,0,0);
	headerLayout->addWidget(m_title,0,1);
	headerLayout->addWidget(button,1,0,1,2);
	headerLayout->setVerticalSpacing(0);
	headerLayout->setMargin(0);
	m_headerFrame->setLayout(headerLayout);
}

void PartsEditorMainWindow::createCenter(ModelPart *modelPart, ItemBase * fromItem) {
	m_moduleId = modelPart ? modelPart->moduleID() : "";
	m_version  = modelPart ? modelPart->version() : "";
	m_uri      = modelPart ? modelPart->uri() : "";

	m_centerFrame = new QFrame();
	m_centerFrame->setObjectName("center");

	QList<QWidget*> specWidgets;

	m_connsInfo = new ConnectorsInfoWidget(m_undoStack,this);
	m_views = new PartsEditorViewsWidget(m_sketchModel, m_undoStack, m_connsInfo, this, fromItem);

	QString label = modelPart ? modelPart->label() : LabelFreshStartText;
	m_label = new EditableLineWidget(label,m_undoStack,this,tr("Label"),modelPart);

	QString description = modelPart ? modelPart->description() : DescriptionFreshStartText;
	m_description = new EditableTextWidget(description,m_undoStack,this,tr("Description"),modelPart);

	/*QString taxonomy = modelPart ? modelPart->modelPartShared()->taxonomy() : TAXONOMY_FRESH_START_TEXT;
	m_taxonomy = new EditableLineWidget(taxonomy,m_undoStack,this,tr("Taxonomy"),modelPart);
	QRegExp regexp("[\\w+\\.]+\\w$");
	m_taxonomy->setValidator(new QRegExpValidator(regexp,this));*/

	QStringList readOnlyKeys;
	readOnlyKeys << "family" << "voltage" << "type";

	QHash<QString,QString> initValues;
	if(modelPart) {
		initValues = modelPart->properties();
	} else {
		initValues["family"] = "";
		//initValues["voltage"] = "";
		//initValues["type"] = "Through-Hole";
	}

	m_properties = new HashPopulateWidget(tr("Properties"),initValues,readOnlyKeys,false, this);
    connect(m_properties, SIGNAL(changed()), this, SLOT(propertiesChanged()));

	QString tags = modelPart ? modelPart->tags().join(", ") : TagsFreshStartText;
	m_tags = new EditableLineWidget(tags,m_undoStack,this,tr("Tags"),modelPart);


	m_author = new EditableLineWidget(
		modelPart ? modelPart->author() : QString(getenvUser()),
		m_undoStack, this, tr("Author"),true);
	connect(
		m_author,SIGNAL(editionCompleted(QString)),
		this,SLOT(updateDateAndAuthor()));

	m_createdOn = new EditableDateWidget(
		modelPart ? modelPart->date() : QDate::currentDate(),
		m_undoStack,this, tr("Created/Updated on"),true);
	connect(
		m_createdOn,SIGNAL(editionCompleted(QString)),
		this,SLOT(updateDateAndAuthor()));

	m_createdByText = new QLabel(FooterText.arg(m_author->text()).arg(m_createdOn->text()));
	m_createdByText->setObjectName("createdBy");

	specWidgets << m_label << m_description /*<< m_taxonomy*/ << m_properties << m_tags << m_author << m_createdOn << m_createdByText;

	m_connsInfo->setViews(m_views);

	connect(m_connsInfo, SIGNAL(repaintNeeded()), m_views, SLOT(repaint()));
	connect(m_connsInfo, SIGNAL(drawConnector(Connector*)), m_views, SLOT(drawConnector(Connector*)));
	connect(
		m_connsInfo, SIGNAL(drawConnector(ViewLayer::ViewIdentifier, Connector*)),
		m_views, SLOT(drawConnector(ViewLayer::ViewIdentifier, Connector*))
	);
	connect(
		m_connsInfo, SIGNAL(setMismatching(ViewLayer::ViewIdentifier, const QString&, bool)),
		m_views, SLOT(setMismatching(ViewLayer::ViewIdentifier, const QString&, bool))
	);
	connect(m_connsInfo, SIGNAL(removeConnectorFrom(const QString&,ViewLayer::ViewIdentifier)),
			m_views, SLOT(removeConnectorFrom(const QString&,ViewLayer::ViewIdentifier)));
	connect(m_views, SIGNAL(connectorSelectedInView(const QString&)),
			m_connsInfo, SLOT(connectorSelectedInView(const QString&)));
	m_views->showTerminalPointsCheckBox()->setChecked(false);

	m_views->connectTerminalRemoval(m_connsInfo);

	connect(
		m_views, SIGNAL(connectorsFoundSignal(QList< QPointer<Connector> >)),
		m_connsInfo, SLOT(connectorsFound(QList< QPointer<Connector> >))
	);

	m_tabWidget = new QTabWidget(m_centerFrame);
	m_tabWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
	m_tabWidget->addTab(new PartSpecificationsWidget(specWidgets,this),tr("Specifications"));
	m_tabWidget->addTab(new PartConnectorsWidget(m_connsInfo,this),tr("Connectors"));

	QGridLayout *tabLayout = new QGridLayout(m_tabWidget);
	tabLayout->setMargin(0);
	tabLayout->setSpacing(0);

	QSplitter *splitter = new QSplitter(Qt::Vertical,this);
	splitter->addWidget(m_views);
	splitter->addWidget(m_tabWidget);

	QGridLayout *mainLayout = new QGridLayout(m_centerFrame);
	mainLayout->setMargin(0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(splitter,0,0,1,1);
}

void PartsEditorMainWindow::connectWidgetsToSave(const QList<QWidget*> &widgets) {
	for(int i=0; i < widgets.size(); i++) {
		connect(m_saveAsNewPartButton,SIGNAL(clicked()),widgets[i],SLOT(informEditionCompleted()));
		connect(this,SIGNAL(saveButtonClicked()),widgets[i],SLOT(informEditionCompleted()));
	}
}

void PartsEditorMainWindow::createFooter() {
	m_footerFrame = new QFrame();
	m_footerFrame->setObjectName("footer");
	m_footerFrame->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);

	m_saveAsNewPartButton = new QPushButton(tr("save as new part"));
	m_saveAsNewPartButton->setObjectName("saveAsPartButton");
	m_saveAsNewPartButton->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

	m_saveButton = new QPushButton(tr("save"));
	m_saveButton->setObjectName("saveAsButton");
	m_saveButton->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

	updateSaveButton();

	QList<QWidget*> editableLabelWidgets;
	editableLabelWidgets << m_title << m_label << m_description /*<< m_taxonomy*/ << m_tags << m_author << m_createdOn << m_connsInfo ;
	connectWidgetsToSave(editableLabelWidgets);

	connect(m_saveAsNewPartButton, SIGNAL(clicked()), this, SLOT(saveAs()));
	connect(m_saveButton, SIGNAL(clicked()), this, SLOT(save()));

	m_cancelCloseButton = new QPushButton(tr("cancel"));
	m_cancelCloseButton->setObjectName("cancelButton");
	m_cancelCloseButton->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
	connect(m_cancelCloseButton, SIGNAL(clicked()), this, SLOT(close()));

	QHBoxLayout *footerLayout = new QHBoxLayout;

	footerLayout->setMargin(0);
	footerLayout->setSpacing(0);
	footerLayout->addSpacerItem(new QSpacerItem(40,0,QSizePolicy::Minimum,QSizePolicy::Minimum));
	footerLayout->addWidget(m_saveAsNewPartButton);
	footerLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Minimum));
	footerLayout->addWidget(m_saveButton);
	footerLayout->addSpacerItem(new QSpacerItem(15,0,QSizePolicy::Minimum,QSizePolicy::Minimum));
	footerLayout->addWidget(m_cancelCloseButton);
	footerLayout->addSpacerItem(new QSpacerItem(40,0,QSizePolicy::Minimum,QSizePolicy::Minimum));
	m_footerFrame->setLayout(footerLayout);
}


QGraphicsProxyWidget *PartsEditorMainWindow::emptyViewItem(QString iconFile, QString text) {
	QLabel *icon = new QLabel();
	icon->setPixmap(QPixmap(":/resources/images/"+iconFile));
	icon->setAlignment(Qt::AlignHCenter);

	QLabel *label = NULL;
	if(!text.isNull() && !text.isEmpty()) {
		label = new QLabel(text);
		label->setAlignment(Qt::AlignHCenter);
	}

	QFrame *container = new QFrame();
	container->setStyleSheet("background-color: transparent;");
	QVBoxLayout *mainLo = new QVBoxLayout(container);
	mainLo->setMargin(0);
	mainLo->setSpacing(0);

	QHBoxLayout *lo1 = new QHBoxLayout();
	lo1->addWidget(icon);
	lo1->setMargin(0);
	lo1->setSpacing(0);
	mainLo->addLayout(lo1);

	if(label) {
		QHBoxLayout *lo2 = new QHBoxLayout();
		lo2->addWidget(label);
		lo2->setMargin(0);
		lo2->setSpacing(0);
		mainLo->addLayout(lo2);
	}

	QGraphicsProxyWidget *item = new QGraphicsProxyWidget();
	item->setWidget(container);

	return item;
}

const QDir& PartsEditorMainWindow::createTempFolderIfNecessary() {
	if(m_tempDir.path() == ".") {
		QString randext = FolderUtils::getRandText();
		m_tempDir = QDir(QDir::tempPath());
		bool dirCreation = m_tempDir.mkdir(randext);
                if (!dirCreation) {
			throw "PartsEditorMainWindow::createTempFolderIfNecessary creation failure";
		}
		if(dirCreation) {
			bool cdResult = m_tempDir.cd(randext);
			if (!cdResult) {
				throw "PartsEditorMainWindow::createTempFolderIfNecessary cd failure";
			}
		} else {
			// will use the standard location of the temp folder
			// instead of a subfolder inside of it
		}
	}
	return m_tempDir;
}

bool PartsEditorMainWindow::save() {
	if(validateMinRequirements() && wannaSaveAfterWarning(false)) {
		bool result = (m_saveButton->isEnabled()) ? FritzingWindow::save() : saveAs();
		if(result) {
			m_cancelCloseButton->setText(tr("close"));
			if(m_closeAfterSaving) close();
		}
		return result;
	} else {
		return false;
	}
}

bool PartsEditorMainWindow::saveAs() {
	if(!validateMinRequirements()) return false;

	QString fileNameBU = m_fwFilename;
	QString moduleIdBU = m_moduleId;

	m_moduleId = ___emptyString___;
	QString title = m_title->text();
	m_fwFilename = !title.isEmpty() ? title+FritzingPartExtension : QFileInfo(m_fwFilename).baseName();

	// TODO Mariano: This folder should be defined in the preferences... some day
	QString partsFolderPath = FolderUtils::getUserDataStorePath("parts")+"/";
	QString userPartsFolderPath = partsFolderPath+"user/";

	bool firstTime = true; // Perhaps the user wants to use the default file name, confirm first
	while(m_fwFilename.isEmpty()
			|| QFileInfo(userPartsFolderPath+m_fwFilename).exists()
			|| (FolderUtils::isEmptyFileName(m_fwFilename,untitledFileName()) && firstTime) ) 
	{
		bool ok;
		m_fwFilename = QInputDialog::getText(
			this,
			tr("Save as new part"),
			tr("There's already a file with this name.\nPlease, specify a new filename"),
			QLineEdit::Normal,
			m_fwFilename,
			&ok
		);
		firstTime = false;
		if (!ok) {
			m_moduleId = moduleIdBU;
			m_fwFilename = fileNameBU;
			return false;
		}
	}

	m_fwFilename.replace(QRegExp("[^A-Za-z0-9\\.]"), "_");

	Qt::CaseSensitivity cs = Qt::CaseSensitive;
#ifdef Q_WS_WIN
	// seems to be necessary for Windows: getUserDataStorePath() returns a string starting with "c:"
	// but the file dialog returns a string beginning with "C:"
	cs = Qt::CaseInsensitive;
#endif

	QString filename = !m_fwFilename.startsWith(userPartsFolderPath, cs)
					&& !m_fwFilename.startsWith(partsFolderPath, cs)
			? userPartsFolderPath+m_fwFilename
			: m_fwFilename;
	QString guid = "__"+FolderUtils::getRandText()+FritzingPartExtension;
	if(!alreadyHasExtension(filename, FritzingPartExtension)) {
		filename += guid;
	} else {
		filename.replace(FritzingPartExtension,guid);
	}

	makeNonCore();
	if (!saveAsAux(filename)) return false;

	if(wannaSaveAfterWarning(true)) {
		m_savedAsNewPart = true;
		m_updateEnabled = true;
		updateButtons();
		if(m_closeAfterSaving) close();
		return true;
	}

	return false;
}

void PartsEditorMainWindow::makeNonCore() {
	QString oldTags = m_tags->text();
	QString newTags = oldTags.replace("fritzing core","user part",Qt::CaseInsensitive);
	m_tags->setText(newTags);
}

bool PartsEditorMainWindow::saveAsAux(const QString & fileName) {
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Fritzing"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }
    file.close();

    updateDateAndAuthor();

    QApplication::setOverrideCursor(Qt::WaitCursor);

    m_sketchModel->root()->setModelPartShared(createModelPartShared());

	QString fileNameAux = QFileInfo(fileName).fileName();
	m_views->copySvgFilesToDestiny(fileNameAux);
	m_iconViewImage->copySvgFileToDestiny(fileNameAux);

	m_sketchModel->save(fileName, true);

    QApplication::restoreOverrideCursor();

    statusBar()->showMessage(tr("Saved '%1'").arg(fileName), 2000);

    m_partUpdated = true;

    setFileName(fileName);
    //setCurrentFile(fileName);

   // mark the stack clean so we update the window dirty flag
    m_undoStack->setClean();
    setTitle();

    if(m_editingAlien) {
    	// FIXME: this will keep ALL the external files, not just the ones that this part uses
    	emit alienPartUsed();
    	m_editingAlien = false;
    }

	return true;
}

void PartsEditorMainWindow::updateDateAndAuthor() {
	m_createdByText->setText(FooterText.arg(m_author->text()).arg(m_createdOn->text()));
}

ModelPartShared* PartsEditorMainWindow::createModelPartShared() {
	ModelPartShared* shared = new ModelPartShared();

	if(m_moduleId.isNull() || m_moduleId.isEmpty()) {
		m_moduleId = FolderUtils::getRandText();
	}

	shared->setModuleID(m_moduleId);
	shared->setUri(m_uri);
	shared->setVersion(m_version);

	shared->setAuthor(m_author->text());
	shared->setTitle(m_title->text());
	shared->setDate(m_createdOn->text());
	shared->setLabel(m_label->text());
	//stuff->setTaxonomy(m_taxonomy->text());
	shared->setDescription(m_description->text());

	QStringList tags = m_tags->text().split(", ");
	shared->setTags(tags);
	shared->setProperties(m_properties->hash());

	m_iconViewImage->aboutToSave(true);
	m_views->aboutToSave();

	// the deal seems to be that an original modelpart is created and becomes the official sketch model (root).
	// the ConnectorsShared from that model are used to seed the PartsEditorConnectorsConnectorItems. 
	// When a new image is loaded into a view, the sketch model's ConnectorsShared are replaced with newly constructed ones.
	// So now the sketch model points to one set of ConnectorsShared, but the original ones are still sitting
	// back in the PartsEditorConnectorsConnectorItems.  So now we replace the new ones with the old ones.
	// I think.

	QList< QPointer<ConnectorShared> > connsShared = m_connsInfo->connectorsShared();
	m_views->updatePinsInfo(connsShared);
	shared->setConnectorsShared(m_connsInfo->connectorsShared());

	return shared;
}

void PartsEditorMainWindow::cleanUp() {
	bool decUntitled = m_fwFilename.startsWith(untitledFileName());
	if(decUntitled) {
		if(m_lastOpened == this) {
			UntitledPartIndex -= m_closedBeforeCount;
			UntitledPartIndex--;
			m_closedBeforeCount = 0;
		} else {
			m_closedBeforeCount++;
		}
	}
	if(m_tempDir.path() != ".") {
		FolderUtils::rmdir(m_tempDir);
		m_tempDir = QDir();
	}
}

void PartsEditorMainWindow::parentAboutToClose() {
	if(beforeClosing(false)) {
		cleanUp();
	}
}

void PartsEditorMainWindow::closeEvent(QCloseEvent *event) {
	if(beforeClosing()) {
		cleanUp();
		QMainWindow::closeEvent(event);
		if(m_partUpdated) {
			emit partUpdated(m_fwFilename, m_id, !m_savedAsNewPart &&
				(m_connsInfo->connectorsCountChanged() || m_views->connectorsPosOrSizeChanged())
			);
		}
		emit closed(m_id);
	} else {
		event->ignore();
	}

	QSettings settings;
	settings.setValue("peditor/state",saveState());
	settings.setValue("peditor/geometry",saveGeometry());
}

const QDir& PartsEditorMainWindow::tempDir() {
	return createTempFolderIfNecessary();
}

const QString PartsEditorMainWindow::untitledFileName() {
	return UntitledPartName;
}

int &PartsEditorMainWindow::untitledFileCount() {
	return UntitledPartIndex;
}

const QString PartsEditorMainWindow::fileExtension() {
	return FritzingPartExtension;
}

const QString PartsEditorMainWindow::defaultSaveFolder() {
	return QDir::currentPath()+"/parts/user/";
}

void PartsEditorMainWindow::updateSaveButton() {
	if(m_saveButton) m_saveButton->setEnabled(m_updateEnabled);
}

bool PartsEditorMainWindow::wannaSaveAfterWarning(bool savingAsNew) {
	bool doEmit = true;
	bool errorFound = false;
	QString msg = "Some errors found:\n";
	if(!m_savedAsNewPart && !savingAsNew && m_connsInfo->connectorsRemoved()) {
		errorFound = true;
		msg += "- The connectors defined in this part have changed.\n"
				"If you save it, you may break other sketches that already use it.\n";
	}

	if(m_connsInfo->hasMismatchingConnectors()) {
		errorFound = true;
		msg += "- Some connectors are not present in all views."
				"If you save now, they will be removed.\n";
	}

	if(errorFound) {
		msg += "\nDo you want to save anyway?";
		QMessageBox::StandardButton btn = QMessageBox::question(this,
			tr("Updating existing part"),
			msg,
			QMessageBox::Yes|QMessageBox::No
		);
		doEmit = (btn == QMessageBox::Yes);
	}

	if(doEmit) emit saveButtonClicked();

	return doEmit;
}

void PartsEditorMainWindow::updateButtons() {
	m_saveAsNewPartButton->setEnabled(false);
	m_cancelCloseButton->setText(tr("close"));
}

bool PartsEditorMainWindow::eventFilter(QObject *object, QEvent *event) {
	if (object == this && event->type() == QEvent::ShortcutOverride) {
		QKeyEvent *keyEvent = dynamic_cast<QKeyEvent*>(event);
		if(keyEvent && keyEvent->matches(QKeySequence::Close)) {
			this->close();
			event->ignore();
			ProcessEventBlocker::processEvents();
#ifdef Q_WS_MAC
			FritzingWindow *parent = qobject_cast<FritzingWindow*>(parentWidget());
			if(parent) {
				parent->notClosableForAWhile();
			}
#endif
			return true;
		}
	}
	return QMainWindow::eventFilter(object, event);
}

const QString PartsEditorMainWindow::fritzingTitle() {
	QString fritzing = FritzingWindow::fritzingTitle();
	return tr("%1 %2").arg(fritzing).arg(___partsEditorName___);
}

bool PartsEditorMainWindow::event(QEvent * e) {
	switch (e->type()) {
		case QEvent::WindowActivate:
			emit changeActivationSignal(true, this);
			break;
		case QEvent::WindowDeactivate:
			emit changeActivationSignal(false, this);
			break;
		default:
			break;
	}
	return FritzingWindow::event(e);
}

bool PartsEditorMainWindow::validateMinRequirements() {
	if(!m_iconViewImage->isEmpty()) {
		return true;
	} else {
		QMessageBox::information(this, tr("Icon needed"), tr("Please, provide an icon image for this part"));
		return false;
	}
}

void PartsEditorMainWindow::setViewItems(ItemBase * iiItem, ItemBase* bbItem, ItemBase* schemItem, ItemBase* pcbItem) {
	m_iconItem = iiItem;
	m_breadboardItem = bbItem;
	m_schematicItem = schemItem;
	m_pcbItem = pcbItem;
}

QString PartsEditorMainWindow::getExtensionString() {
	return tr("Fritzing Parts (*%1)").arg(fileExtension());
}

QStringList PartsEditorMainWindow::getExtensions() {
	QStringList extensions;
	extensions.append(fileExtension());
	return extensions;
}

void PartsEditorMainWindow::propertiesChanged() {
    if (m_undoStack) {
	    m_undoStack->push(new QUndoCommand("dummy parts editor command"));
    }
}
