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

#include <QtXml>
#include <QList>
#include <QFileInfo>
#include <QStringList>
#include <QFileInfoList>
#include <QDir>
#include <QLabel>
#include <QTime>
#include <QSettings>
#include <QRegularExpression>
#include <QPaintDevice>
#include <QPixmap>
#include <QTimer>
#include <QStackedWidget>
#include <QXmlStreamReader>
#include <QShortcut>
#include <QStyle>
#include <QFontMetrics>
#include <QApplication>


#include "mainwindow.h"
#include "../debugdialog.h"
#include "fdockwidget.h"
#include "../infoview/htmlinfoview.h"
#include "../waitpushundostack.h"
#include "../sketch/breadboardsketchwidget.h"
#include "../sketch/schematicsketchwidget.h"
#include "../sketch/pcbsketchwidget.h"
#include "../sketch/welcomeview.h"
#include "../utils/folderutils.h"
#include "../utils/fmessagebox.h"
#include "../utils/lockmanager.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"
#include "../items/perfboard.h"
#include "../items/stripboard.h"
#include "../items/partfactory.h"
#include "../items/paletteitem.h"
#include "../items/virtualwire.h"
#include "../processeventblocker.h"
#include "../sketchtoolbutton.h"
#include "../partsbinpalette/binmanager/binmanager.h"
#include "../utils/fsizegrip.h"
#include "../utils/expandinglabel.h"
#include "../utils/autoclosemessagebox.h"
#include "../utils/fileprogressdialog.h"
#include "../utils/clickablelabel.h"
#include "../items/resizableboard.h"
#include "../items/resistor.h"
#include "../utils/zoomslider.h"
#include "../partseditor/pemainwindow.h"
#include "../help/firsttimehelpdialog.h"
#include "../simulation/simulator.h"
#include "../simulation/FProbeStartSimulator.h"
#include "../mainwindow/FProbeDropByModuleID.h"
#include "../mainwindow/FProbeKeyPressEvents.h"
#include "connectors/debugconnectors.h"
#include "connectors/debugconnectorsprobe.h"

FTabWidget::FTabWidget(QWidget * parent) : QTabWidget(parent)
{
	QTabBar * tabBar = new FTabBar;
	tabBar->setObjectName("mainTabBar");
	//connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabIndexChanged(int)));
	setTabBar(tabBar);
}

//int FTabWidget::addTab(QWidget * page, const QIcon & icon, const QIcon & hoverIcon, const QIcon & inactiveIcon, const QString & label)
//{
//   // assumes tabs are not deleted or reordered
//    m_inactiveIcons << inactiveIcon;
//    m_hoverIcons << hoverIcon;
//    m_icons << icon;
//    return QTabWidget::addTab(page, icon, label);
//}

//void FTabWidget::tabIndexChanged(int index) {
//    for (int i = 0; i < this->count(); ++i) {
//        if (i == index) setTabIcon(i, m_icons.at(i));
//        else setTabIcon(i, m_inactiveIcons.at(i));
//    }
//}

FTabBar::FTabBar() : QTabBar()
{
	m_firstTime = true;
}


void FTabBar::paintEvent(QPaintEvent * event) {
	// this is a hack to left-align the tab text by adding spaces to the text
	// center-alignment is hard-coded deep into the way the tab is drawn in qcommonstyle.cpp

	static int offset = 15;  // derived this empirically, no idea where it comes from

	if (m_firstTime) {
		m_firstTime = false;

		// TODO: how to append spaces from the language direction

		for (int i = 0; i < this->count(); ++i) {
			QStyleOptionTab tab;
			initStyleOption(&tab, 0);
			//DebugDialog::debug(QString("state %1").arg(tab.state));
			QString text = tabText(i);
			int added = 0;
			while (true) {
				QRect r = tab.fontMetrics.boundingRect(text);
				if (r.width() + iconSize().width() + offset >= tabRect(i).width()) {
					if (added != 0) {
						text.chop(1);
						setTabText(i, text);
					}
					break;
				}

				text += " ";
				added++;
			}
		}
	}

	QTabBar::paintEvent(event);

	/*
	return;

	// this code mostly lifted from QTabBar::paintEvent

	QStyleOptionTabBarBaseV2 optTabBase;
	optTabBase.init(this);
	optTabBase.shape = this->shape();
	optTabBase.documentMode = this->documentMode();

	QStylePainter p(this);
	int selected = this->currentIndex();

	for (int i = 0; i < this->count(); ++i)
	     optTabBase.tabBarRect |= tabRect(i);

	optTabBase.selectedTabRect = tabRect(selected);

	p.drawPrimitive(QStyle::PE_FrameTabBarBase, optTabBase);

	for (int i = 0; i < this->count(); ++i) {
	    if (i == selected) continue;

	    QStyleOptionTabV3 tab;
	    initStyleOption(&tab, i);
	    drawTab(p, tab, i);
	}

	// Draw the selected tab last to get it "on top"
	if (selected >= 0) {
	    QStyleOptionTabV3 tab;
	    initStyleOption(&tab, selected);
	    drawTab(p, tab, selected);
	}


	*/

}

/*
void FTabBar::drawTab(QStylePainter & p, QStyleOptionTabV3 & tabV3, int index)
{
    //tabV3.iconSize = m_pixmaps.at(index).size();
    p.drawControl(QStyle::CE_TabBarTabShape, tabV3);
    //p.drawPixmap(tabV3.rect.left(), tabV3.rect.top(), m_pixmaps.at(index));


    //QRect tr = tabV3.rect;

    //int alignment = Qt::AlignLeft | Qt::TextShowMnemonic;
    //if (!this->style()->styleHint(QStyle::SH_UnderlineShortcut, &tabV3, this))
    //    alignment |= Qt::TextHideMnemonic;

    //p.drawItemText(tr, alignment, tabV3.palette, tabV3.state & QStyle::State_Enabled, tabV3.text, QPalette::WindowText);


    //p.drawControl(QStyle::CE_TabBarTabLabel, tabV3);

}
*/

///////////////////////////////////////////////

struct MissingSvgInfo {
	QString requestedPath;
	QStringList connectorSvgIds;
	ModelPart * modelPart;
	bool equal;
};

bool byConnectorCount(MissingSvgInfo & m1, MissingSvgInfo & m2)
{
	if (m1.connectorSvgIds.count() == m2.connectorSvgIds.count() && m1.modelPart != m2.modelPart) {
		m1.equal = m2.equal = true;
	}

	return (m1.connectorSvgIds.count() > m2.connectorSvgIds.count());
}

///////////////////////////////////////////////

#define ZIP_PART QString("part.")
#define ZIP_SVG  QString("svg.")

///////////////////////////////////////////////

const QString MainWindow::UntitledSketchName = "Untitled Sketch";
int MainWindow::UntitledSketchIndex = 1;
int MainWindow::CascadeFactorX = 21;
int MainWindow::CascadeFactorY = 19;

static constexpr int MainWindowDefaultWidth = 1024;
static constexpr int MainWindowDefaultHeight = 768;

int MainWindow::AutosaveTimeoutMinutes = 10;   // in minutes
bool MainWindow::AutosaveEnabled = true;
QString MainWindow::BackupFolder;

QRegularExpression MainWindow::GuidMatcher = QRegularExpression("[A-Fa-f0-9]{32}");

/////////////////////////////////////////////

MainWindow::MainWindow(ReferenceModel *referenceModel, QWidget * parent) :
	FritzingWindow(MainWindow::untitledFileName(), MainWindow::untitledFileCount(), MainWindow::fileExtension(), parent)
{
	m_noSchematicConversion = m_useOldSchematic = m_convertedSchematic = false;
	m_initialTab = 1;
	m_rolloverQuoteDialog = nullptr;
	setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
	setDockOptions(QMainWindow::AnimatedDocks);
	m_sizeGrip = new FSizeGrip(this);

	m_topDock = nullptr;
	m_bottomDock = nullptr;
	m_dontKeepMargins = true;

	m_settingsPrefix = "main/";
	m_showWelcomeAct = m_showProgramAct = m_raiseWindowAct = m_showPartsBinIconViewAct = m_showAllLayersAct = m_hideAllLayersAct = m_rotate90cwAct = m_showBreadboardAct = m_showSchematicAct = m_showPCBAct = nullptr;
	m_fileMenu = m_editMenu = m_partMenu = m_windowMenu = m_pcbTraceMenu = m_schematicTraceMenu = m_breadboardTraceMenu = m_viewMenu = nullptr;
	m_infoView = nullptr;
	m_addedToTemp = false;
	setAcceptDrops(true);
	m_activeWire = nullptr;
	m_activeConnectorItem = nullptr;

	m_closeSilently = false;
	m_orderFabAct = nullptr;
	m_viewFromButtonWidget = m_activeLayerButtonWidget = nullptr;
	m_programView = m_programWindow = nullptr;
	m_welcomeView = nullptr;
	m_windowMenuSeparator = nullptr;
	m_schematicWireColorMenu = m_breadboardWireColorMenu = nullptr;
	m_checkForUpdatesAct = nullptr;
	m_fileProgressDialog = nullptr;
	m_currentGraphicsView = nullptr;
	m_comboboxChanged = false;

	// Add a timer for autosaving
	m_backingUp = m_autosaveNeeded = false;
	connect(&m_autosaveTimer, SIGNAL(timeout()), this, SLOT(backupSketch()));
	m_autosaveTimer.start(AutosaveTimeoutMinutes * 60 * 1000);

	resize(MainWindowDefaultWidth, MainWindowDefaultHeight);

	m_backupFileNameAndPath = MainWindow::BackupFolder + "/" + TextUtils::getRandText() + FritzingSketchExtension;
	// Connect the undoStack to our autosave stuff
	connect(m_undoStack, SIGNAL(indexChanged(int)), this, SLOT(autosaveNeeded(int)));
	connect(m_undoStack, SIGNAL(cleanChanged(bool)), this, SLOT(undoStackCleanChanged(bool)));

	// Create dot icons
	m_dotIcon = QIcon(":/resources/images/dot.png");
	m_emptyIcon = QIcon();

	m_currentWidget = nullptr;
	m_firstOpen = true;

	m_statusBar = new QStatusBar(this);
	setStatusBar(m_statusBar);
	m_statusBar->setSizeGripEnabled(false);

	QSettings settings;
	m_locationLabelUnits = settings.value("LocationInches", "in").toString();

	// leave the m_orderFabEnabled check in case we turn off the fab button in the future
	m_orderFabEnabled = true; // settings.value(ORDERFABENABLED, QVariant(false)).toBool();

	m_locationLabel = new ClickableLabel("", this);
	m_locationLabel->setObjectName("LocationLabel");
	connect(m_locationLabel, SIGNAL(clicked()), this, SLOT(locationLabelClicked()));
	m_locationLabel->setCursor(Qt::PointingHandCursor);
	m_statusBar->addPermanentWidget(m_locationLabel);

	m_zoomSlider = new ZoomSlider(ZoomableGraphicsView::MaxScaleValue, m_statusBar);
	connect(m_zoomSlider, SIGNAL(zoomChanged(double)), this, SLOT(updateViewZoom(double)));
	m_statusBar->addPermanentWidget(m_zoomSlider);

	setAttribute(Qt::WA_DeleteOnClose, true);

#ifdef Q_OS_MAC
	//setAttribute(Qt::WA_QuitOnClose, false);					// restoring this temporarily (2008.12.19)
#endif
	m_dontClose = m_closing = false;

	m_referenceModel = referenceModel;
	m_sketchModel = new SketchModel(true);


	auto * shortcut = new QShortcut(QKeySequence(tr("Ctrl+R", "Rotate Clockwise")), this);
	connect(shortcut, SIGNAL(activated()), this, SLOT(rotateIncCW()));
	shortcut = new QShortcut(QKeySequence(tr("Alt+Ctrl+R", "Rotate Clockwise")), this);
	connect(shortcut, SIGNAL(activated()), this, SLOT(rotateIncCWRubberBand()));
	shortcut = new QShortcut(QKeySequence(tr("Meta+Ctrl+R", "Rotate Clockwise")), this);
	connect(shortcut, SIGNAL(activated()), this, SLOT(rotateIncCWRubberBand()));

	shortcut = new QShortcut(QKeySequence(tr("Shift+Ctrl+R", "Rotate Counterclockwise")), this);
	connect(shortcut, SIGNAL(activated()), this, SLOT(rotateIncCCW()));
	shortcut = new QShortcut(QKeySequence(tr("Alt+Shift+Ctrl+R", "Rotate Counterclockwise")), this);
	connect(shortcut, SIGNAL(activated()), this, SLOT(rotateIncCCWRubberBand()));
	shortcut = new QShortcut(QKeySequence(tr("Meta+Shift+Ctrl+R", "Rotate Counterclockwise")), this);
	connect(shortcut, SIGNAL(activated()), this, SLOT(rotateIncCCWRubberBand()));

	shortcut = new QShortcut(QKeySequence(tr("Shift+Ctrl+Tab", "Toggle Active Layer")), this);
	connect(shortcut, SIGNAL(activated()), this, SLOT(toggleActiveLayer()));


	connect(this, SIGNAL(changeActivationSignal(bool, QWidget *)), qApp, SLOT(changeActivation(bool, QWidget *)), Qt::DirectConnection);
	connect(this, SIGNAL(destroyed(QObject *)), qApp, SLOT(topLevelWidgetDestroyed(QObject *)));
	connect(this, SIGNAL(externalProcessSignal(QString &, QString &, QStringList &)),
	        qApp, SLOT(externalProcessSlot(QString &, QString &, QStringList &)),
	        Qt::DirectConnection);
}

QWidget * MainWindow::createTabWidget() {
	//return new QStackedWidget(this);
	auto * tabWidget = new FTabWidget(this);
	tabWidget->setObjectName("sketch_tabs");
	return tabWidget;
}

void MainWindow::addTab(QWidget * widget, const QString & label) {
	//Q_UNUSED(label);
	//qobject_cast<QStackedWidget *>(m_tabWidget)->addWidget(widget);
	qobject_cast<QTabWidget *>(m_tabWidget)->addTab(widget, label);
}


void MainWindow::addTab(QWidget * widget, const QString & iconPath, const QString & label, bool withIcon) {
	if (!withIcon) {
		addTab(widget, label);
		return;
	}

	auto * tabWidget = qobject_cast<FTabWidget *>(m_tabWidget);
	if (tabWidget == nullptr) {
		addTab(widget, label);
		return;
	}

	//Q_UNUSED(label);
	//qobject_cast<QStackedWidget *>(m_tabWidget)->addWidget(widget);
	QIcon icon;
	QPixmap pixmap(iconPath);
	icon.addPixmap(pixmap, QIcon::Normal, QIcon::On);
	QString inactivePath = iconPath;
	inactivePath.replace("Active", "Inactive");
	QPixmap inactivePixmap(inactivePath);
	icon.addPixmap(inactivePixmap, QIcon::Normal, QIcon::Off);
	QString hoverPath = iconPath;
	hoverPath.replace("Active", "Hover");
	QPixmap hoverPixmap(hoverPath);
	icon.addPixmap(hoverPixmap, QIcon::Active, QIcon::On);

	tabWidget->addTab(widget, icon, label);
}

int MainWindow::currentTabIndex() {
	return qobject_cast<QTabWidget *>(m_tabWidget)->currentIndex();
}

void MainWindow::setCurrentTabIndex(int index) {
	qobject_cast<QTabWidget *>(m_tabWidget)->setCurrentIndex(index);
}

QWidget * MainWindow::currentTabWidget() {
	return qobject_cast<QTabWidget *>(m_tabWidget)->currentWidget();
}

void MainWindow::init(ReferenceModel *referenceModel, bool lockFiles) {

	m_tabWidget = createTabWidget(); //   FTabWidget(this);
	setCentralWidget(m_tabWidget);

	m_referenceModel = referenceModel;
	m_restarting = false;

	if (m_fileProgressDialog != nullptr) {
		m_fileProgressDialog->setValue(2);
	}

	initLockedFiles(lockFiles);


	initWelcomeView();
	initSketchWidgets(true);
	initProgrammingWidget();

	m_simulator = new Simulator(this);

	m_undoView = new QUndoView();
	m_undoGroup = new QUndoGroup(this);
	m_undoView->setGroup(m_undoGroup);
	m_undoGroup->setActiveStack(m_undoStack);

	initDock();
	initMenus();
	moreInitDock();

	createZoomOptions(m_breadboardWidget);
	createZoomOptions(m_schematicWidget);
	createZoomOptions(m_pcbWidget);

	m_breadboardWidget->setToolbarWidgets(getButtonsForView(ViewLayer::BreadboardView));
	m_schematicWidget->setToolbarWidgets(getButtonsForView(ViewLayer::SchematicView));
	m_pcbWidget->setToolbarWidgets(getButtonsForView(ViewLayer::PCBView));

	initStyleSheet();

	m_breadboardGraphicsView->setItemMenu(breadboardItemMenu());
	m_breadboardGraphicsView->setWireMenu(breadboardWireMenu());

	m_pcbGraphicsView->setWireMenu(pcbWireMenu());
	m_pcbGraphicsView->setItemMenu(pcbItemMenu());

	m_schematicGraphicsView->setItemMenu(schematicItemMenu());
	m_schematicGraphicsView->setWireMenu(schematicWireMenu());

	if (m_infoView != nullptr) {
		m_breadboardGraphicsView->setInfoView(m_infoView);
		m_pcbGraphicsView->setInfoView(m_infoView);
		m_schematicGraphicsView->setInfoView(m_infoView);
	}

	// make sure to set the connections after the views have been created
	connect(m_tabWidget, SIGNAL(currentChanged ( int )), this, SLOT(tabWidget_currentChanged( int )));

	connectPairs();

	setInitialView();

	this->installEventFilter(this);

	if (m_fileProgressDialog != nullptr) {
		m_fileProgressDialog->setValue(95);
	}

	QSettings settings;

	if(!settings.value(m_settingsPrefix + "state").isNull()) {
		restoreState(settings.value(m_settingsPrefix + "state").toByteArray());
		restoreGeometry(settings.value(m_settingsPrefix + "geometry").toByteArray());
	}

	setMinimumSize(0,0);
	setMinimumHeight(800);
	setMinimumHeight(0);
	m_tabWidget->setMinimumWidth(500);
	m_tabWidget->setMinimumWidth(0);

	connect(this, SIGNAL(readOnlyChanged(bool)), this, SLOT(applyReadOnlyChange(bool)));

	m_setUpDockManagerTimer.setSingleShot(true);
	connect(&m_setUpDockManagerTimer, SIGNAL(timeout()), this, SLOT(keepMargins()));
	m_setUpDockManagerTimer.start(1000);

	if (m_fileProgressDialog != nullptr) {
		m_fileProgressDialog->setValue(98);
	}

	new FProbeStartSimulator(m_simulator);
	auto fProbe = new FProbeDropByModuleID();

	connect(fProbe, &FProbeDropByModuleID::putItemByModuleID, this, &MainWindow::putItemByModuleID);

	auto fProbeKey = new FProbeKeyPressEvents();

	connect(fProbeKey, &FProbeKeyPressEvents::postKeyEvent, this, &MainWindow::postKeyEvent);

#ifndef QT_NO_DEBUG
	m_debugConnectors = new DebugConnectors(m_breadboardGraphicsView, m_schematicGraphicsView, m_pcbGraphicsView);
#endif
	auto fProbeDebugConnectors = new DebugConnectorsProbe(m_breadboardGraphicsView, m_schematicGraphicsView, m_pcbGraphicsView);

	m_projectProperties = QSharedPointer<ProjectProperties>(new ProjectProperties());
//	m_breadboardGraphicsView->setProjectProperties(m_projectProperties);
//	m_schematicGraphicsView->setProjectProperties(m_projectProperties);
//	m_pcbGraphicsView->setProjectProperties(m_projectProperties);
}

MainWindow::~MainWindow()
{
	// Delete backup of this sketch if one exists.
	QFile::remove(m_backupFileNameAndPath);

	delete m_sketchModel;

	dontKeepMargins();
	m_setUpDockManagerTimer.stop();

	Q_FOREACH (LinkedFile * linkedFile, m_linkedProgramFiles) {
		delete linkedFile;
	}
	m_linkedProgramFiles.clear();

	if (!m_fzzFolder.isEmpty()) {
		LockManager::releaseLockedFiles(m_fzzFolder, m_fzzFiles);
		FolderUtils::rmdir(m_fzzFolder);
	}
}

void MainWindow::initLockedFiles(bool lockFiles) {
	LockManager::initLockedFiles("fzz", m_fzzFolder, m_fzzFiles, lockFiles ? LockManager::SlowTime : 0);
	if (lockFiles) {
		QFileInfoList backupList;
		LockManager::checkLockedFiles("fzz", backupList, m_fzzFiles, true, LockManager::SlowTime);
	}
}

void MainWindow::initSketchWidgets(bool withIcons) {
	//DebugDialog::debug("init sketch widgets");

	// all this belongs in viewLayer.xml
	m_breadboardGraphicsView = new BreadboardSketchWidget(ViewLayer::BreadboardView, this);
	initSketchWidget(m_breadboardGraphicsView);
	m_breadboardWidget = new SketchAreaWidget(m_breadboardGraphicsView,this);
	addTab(m_breadboardWidget, ":/resources/images/icons/TabWidgetBreadboardActive_icon.png", tr("Breadboard"), withIcons);

	if (m_fileProgressDialog != nullptr) {
		m_fileProgressDialog->setValue(11);
	}

	m_schematicGraphicsView = new SchematicSketchWidget(ViewLayer::SchematicView, this);
	initSketchWidget(m_schematicGraphicsView);
	m_schematicWidget = new SketchAreaWidget(m_schematicGraphicsView, this);
	addTab(m_schematicWidget, ":/resources/images/icons/TabWidgetSchematicActive_icon.png", tr("Schematic"), withIcons);

	if (m_fileProgressDialog != nullptr) {
		m_fileProgressDialog->setValue(20);
	}

	m_pcbGraphicsView = new PCBSketchWidget(ViewLayer::PCBView, this);
	initSketchWidget(m_pcbGraphicsView);
	m_pcbWidget = new SketchAreaWidget(m_pcbGraphicsView, this);
	addTab(m_pcbWidget, ":/resources/images/icons/TabWidgetPcbActive_icon.png", tr("PCB"), withIcons);


	if (m_fileProgressDialog != nullptr) {
		m_fileProgressDialog->setValue(29);
	}
}

void MainWindow::initMenus() {
	// This is the magic translation that changes all the shortcut text on the menu items
	// to the native language instead of "Ctrl", so the German menu items will now read "Strg"
	// You don't actually have to translate every menu item in the .ts file, you can just leave it as "Ctrl".
	QShortcut::tr("Ctrl", "for naming shortcut keys on menu items");
	QShortcut::tr("Alt", "for naming shortcut keys on menu items");
	QShortcut::tr("Shift", "for naming shortcut keys on menu items");
	QShortcut::tr("Meta", "for naming shortcut keys on menu items");

	//DebugDialog::debug("create menus");

	createActions();
	createMenus();

	//DebugDialog::debug("create toolbars");

	createStatusBar();

	//DebugDialog::debug("after creating status bar");

	if (m_fileProgressDialog != nullptr) {
		m_fileProgressDialog->setValue(91);
	}
}

void MainWindow::initSketchWidget(SketchWidget * sketchWidget) {
	sketchWidget->setSketchModel(m_sketchModel);
	sketchWidget->setReferenceModel(m_referenceModel);
	sketchWidget->setUndoStack(m_undoStack);
	sketchWidget->setChainDrag(true);			// enable bend points
	sketchWidget->initGrid();
	sketchWidget->addViewLayers();
}

void MainWindow::connectPairs() {
	connectPair(m_breadboardGraphicsView, m_schematicGraphicsView);
	connectPair(m_breadboardGraphicsView, m_pcbGraphicsView);
	connectPair(m_schematicGraphicsView, m_breadboardGraphicsView);
	connectPair(m_schematicGraphicsView, m_pcbGraphicsView);
	connectPair(m_pcbGraphicsView, m_breadboardGraphicsView);
	connectPair(m_pcbGraphicsView, m_schematicGraphicsView);

	connect(m_pcbGraphicsView, SIGNAL(groundFillSignal()), this, SLOT(groundFill()));
	connect(m_pcbGraphicsView, SIGNAL(copperFillSignal()), this, SLOT(copperFill()));

	connect(m_pcbGraphicsView, SIGNAL(swapBoardImageSignal(SketchWidget *, ItemBase *, const QString &, const QString &, bool)),
	        this, SLOT(swapBoardImageSlot(SketchWidget *, ItemBase *, const QString &, const QString &, bool)));


	connect(m_breadboardGraphicsView, SIGNAL(setActiveWireSignal(Wire *)), this, SLOT(setActiveWire(Wire *)));
	connect(m_schematicGraphicsView, SIGNAL(setActiveWireSignal(Wire *)), this, SLOT(setActiveWire(Wire *)));
	connect(m_pcbGraphicsView, SIGNAL(setActiveWireSignal(Wire *)), this, SLOT(setActiveWire(Wire *)));

	connect(m_breadboardGraphicsView, SIGNAL(setActiveConnectorItemSignal(ConnectorItem *)), this, SLOT(setActiveConnectorItem(ConnectorItem *)));
	connect(m_schematicGraphicsView, SIGNAL(setActiveConnectorItemSignal(ConnectorItem *)), this, SLOT(setActiveConnectorItem(ConnectorItem *)));
	connect(m_pcbGraphicsView, SIGNAL(setActiveConnectorItemSignal(ConnectorItem *)), this, SLOT(setActiveConnectorItem(ConnectorItem *)));

	bool succeeded = connect(m_pcbGraphicsView, SIGNAL(routingStatusSignal(SketchWidget *, const RoutingStatus &)),
	                         this, SLOT(routingStatusSlot(SketchWidget *, const RoutingStatus &))) != nullptr;
	succeeded =  succeeded && (connect(m_schematicGraphicsView, SIGNAL(routingStatusSignal(SketchWidget *, const RoutingStatus &)),
	                                  this, SLOT(routingStatusSlot(SketchWidget *, const RoutingStatus &))) != nullptr);
	succeeded =  succeeded && (connect(m_breadboardGraphicsView, SIGNAL(routingStatusSignal(SketchWidget *, const RoutingStatus &)),
	                                  this, SLOT(routingStatusSlot(SketchWidget *, const RoutingStatus &))) != nullptr);

	succeeded =  succeeded && (connect(m_breadboardGraphicsView, SIGNAL(swapSignal(const QString &, const QString &, QMap<QString, QString> &, ItemBase *)),
									  this, SLOT(swapSelectedMap(const QString &, const QString &, QMap<QString, QString> &, ItemBase *))) != nullptr);
	succeeded =  succeeded && (connect(m_schematicGraphicsView, SIGNAL(swapSignal(const QString &, const QString &, QMap<QString, QString> &, ItemBase *)),
									  this, SLOT(swapSelectedMap(const QString &, const QString &, QMap<QString, QString> &, ItemBase *))) != nullptr);
	succeeded =  succeeded && (connect(m_pcbGraphicsView, SIGNAL(swapSignal(const QString &, const QString &, QMap<QString, QString> &, ItemBase *)),
									  this, SLOT(swapSelectedMap(const QString &, const QString &, QMap<QString, QString> &, ItemBase *))) != nullptr);

	succeeded =  succeeded && (connect(m_breadboardGraphicsView, SIGNAL(dropPasteSignal(SketchWidget *)),
	                                  this, SLOT(dropPaste(SketchWidget *))) != nullptr);
	succeeded =  succeeded && (connect(m_schematicGraphicsView, SIGNAL(dropPasteSignal(SketchWidget *)),
	                                  this, SLOT(dropPaste(SketchWidget *))) != nullptr);
	succeeded =  succeeded && (connect(m_pcbGraphicsView, SIGNAL(dropPasteSignal(SketchWidget *)),
	                                  this, SLOT(dropPaste(SketchWidget *))) != nullptr);

	succeeded =  succeeded && (connect(m_pcbGraphicsView, SIGNAL(subSwapSignal(SketchWidget *, ItemBase *, const QString &, ViewLayer::ViewLayerPlacement, long &, QUndoCommand *)),
	                                  this, SLOT(subSwapSlot(SketchWidget *, ItemBase *, const QString &, ViewLayer::ViewLayerPlacement, long &, QUndoCommand *)),
	                                  Qt::DirectConnection) != nullptr);

	succeeded =  succeeded && (connect(m_pcbGraphicsView, SIGNAL(updateLayerMenuSignal()), this, SLOT(updateLayerMenuSlot())) != nullptr);
	succeeded =  succeeded && (connect(m_pcbGraphicsView, SIGNAL(changeBoardLayersSignal(int, bool )), this, SLOT(changeBoardLayers(int, bool ))) != nullptr);

	succeeded =  succeeded && (connect(m_pcbGraphicsView, SIGNAL(boardDeletedSignal()), this, SLOT(boardDeletedSlot())) != nullptr);

	succeeded =  succeeded && (connect(qApp, SIGNAL(spaceBarIsPressedSignal(bool)), m_breadboardGraphicsView, SLOT(spaceBarIsPressedSlot(bool))) != nullptr);
	succeeded =  succeeded && (connect(qApp, SIGNAL(spaceBarIsPressedSignal(bool)), m_schematicGraphicsView, SLOT(spaceBarIsPressedSlot(bool))) != nullptr);
	succeeded =  succeeded && (connect(qApp, SIGNAL(spaceBarIsPressedSignal(bool)), m_pcbGraphicsView, SLOT(spaceBarIsPressedSlot(bool))) != nullptr);

	succeeded =  succeeded && (connect(m_pcbGraphicsView, SIGNAL(cursorLocationSignal(double, double, double, double)), this, SLOT(cursorLocationSlot(double, double, double, double))) != nullptr);
	succeeded =  succeeded && (connect(m_breadboardGraphicsView, SIGNAL(cursorLocationSignal(double, double, double, double)), this, SLOT(cursorLocationSlot(double, double, double, double))) != nullptr);
	succeeded =  succeeded && (connect(m_schematicGraphicsView, SIGNAL(cursorLocationSignal(double, double, double, double)), this, SLOT(cursorLocationSlot(double, double, double, double))) != nullptr);

	succeeded =  succeeded && (connect(m_breadboardGraphicsView, SIGNAL(filenameIfSignal(QString &)), this, SLOT(filenameIfSlot(QString &)), Qt::DirectConnection) != nullptr);
	succeeded =  succeeded && (connect(m_pcbGraphicsView, SIGNAL(filenameIfSignal(QString &)), this, SLOT(filenameIfSlot(QString &)), Qt::DirectConnection) != nullptr);
	succeeded =  succeeded && (connect(m_schematicGraphicsView, SIGNAL(filenameIfSignal(QString &)), this, SLOT(filenameIfSlot(QString &)), Qt::DirectConnection) != nullptr);



	succeeded =  succeeded && (connect(m_breadboardGraphicsView, SIGNAL(getDroppedItemViewLayerPlacementSignal(ModelPart *, ViewLayer::ViewLayerPlacement &)),
	                                  m_pcbGraphicsView, SLOT(getDroppedItemViewLayerPlacement(ModelPart *, ViewLayer::ViewLayerPlacement &)),
	                                  Qt::DirectConnection) != nullptr);
	succeeded =  succeeded && (connect(m_schematicGraphicsView, SIGNAL(getDroppedItemViewLayerPlacementSignal(ModelPart *, ViewLayer::ViewLayerPlacement &)),
	                                  m_pcbGraphicsView, SLOT(getDroppedItemViewLayerPlacement(ModelPart *, ViewLayer::ViewLayerPlacement &)),
	                                  Qt::DirectConnection) != nullptr);


	if (!succeeded) {
		DebugDialog::debug("connectPair failed");
	}
}

void MainWindow::connectPair(SketchWidget * signaller, SketchWidget * slotter)
{

	bool succeeded = connect(signaller, SIGNAL(itemAddedSignal(ModelPart *, ItemBase *, ViewLayer::ViewLayerPlacement, const ViewGeometry &, long, SketchWidget *)),
	                         slotter, SLOT(itemAddedSlot(ModelPart *, ItemBase *, ViewLayer::ViewLayerPlacement, const ViewGeometry &, long, SketchWidget *))) != nullptr;

	succeeded = succeeded && (connect(signaller, SIGNAL(itemDeletedSignal(long)),
	                                 slotter, SLOT(itemDeletedSlot(long)),
	                                 Qt::DirectConnection) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(clearSelectionSignal()),
	                                 slotter, SLOT(clearSelectionSlot())) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(itemSelectedSignal(long, bool)),
	                                 slotter, SLOT(itemSelectedSlot(long, bool))) != nullptr);
	succeeded = succeeded && (connect(signaller, SIGNAL(selectAllItemsSignal(bool, bool)),
	                                 slotter, SLOT(selectAllItems(bool, bool))) != nullptr);
	succeeded = succeeded && (connect(signaller, SIGNAL(wireDisconnectedSignal(long, QString)),
	                                 slotter, SLOT(wireDisconnectedSlot(long,  QString))) != nullptr);
	succeeded = succeeded && (connect(signaller, SIGNAL(wireConnectedSignal(long,  QString, long,  QString)),
	                                 slotter, SLOT(wireConnectedSlot(long, QString, long, QString))) != nullptr);
	succeeded = succeeded && (connect(signaller, SIGNAL(changeConnectionSignal(long,  QString, long,  QString, ViewLayer::ViewLayerPlacement, bool, bool)),
	                                 slotter, SLOT(changeConnectionSlot(long, QString, long, QString, ViewLayer::ViewLayerPlacement, bool, bool))) != nullptr);
	succeeded = succeeded && (connect(signaller, SIGNAL(copyBoundingRectsSignal(QHash<QString, QRectF> &)),
	                                 slotter, SLOT(copyBoundingRectsSlot(QHash<QString, QRectF> &)),
	                                 Qt::DirectConnection) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(cleanUpWiresSignal(CleanUpWiresCommand *)),
	                                 slotter, SLOT(cleanUpWiresSlot(CleanUpWiresCommand *)) ) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(cleanupRatsnestsSignal(bool)),
					 slotter, SLOT(cleanupRatsnestsForCommand(bool)) ) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(checkStickySignal(long, bool, bool, CheckStickyCommand *)),
					 slotter, SLOT(checkStickyForCommand(long, bool, bool, CheckStickyCommand *)) ) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(disconnectAllSignal(QList<ConnectorItem *>, QHash<ItemBase *, SketchWidget *> &, QUndoCommand *)),
	                                 slotter, SLOT(disconnectAllSlot(QList<ConnectorItem *>, QHash<ItemBase *, SketchWidget *> &, QUndoCommand *)),
	                                 Qt::DirectConnection) != nullptr);
	succeeded = succeeded && (connect(signaller, SIGNAL(setResistanceSignal(long, QString, QString, bool)),
	                                 slotter, SLOT(setResistance(long, QString, QString, bool))) != nullptr);
	succeeded = succeeded && (connect(signaller, SIGNAL(makeDeleteItemCommandPrepSignal(ItemBase *, bool, QUndoCommand * )),
	                                 slotter, SLOT(makeDeleteItemCommandPrepSlot(ItemBase *, bool, QUndoCommand * ))) != nullptr);
	succeeded = succeeded && (connect(signaller, SIGNAL(makeDeleteItemCommandFinalSignal(ItemBase *, bool, QUndoCommand * )),
	                                 slotter, SLOT(makeDeleteItemCommandFinalSlot(ItemBase *, bool, QUndoCommand * ))) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(setPropSignal(long,  const QString &,  const QString &, bool, bool)),
	                                 slotter, SLOT(setProp(long,  const QString &,  const QString &, bool, bool))) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(setInstanceTitleSignal(long, const QString &, const QString &, bool, bool )),
					 slotter, SLOT(setInstanceTitleForCommand(long, const QString &, const QString &, bool, bool ))) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(setVoltageSignal(double, bool )),
	                                 slotter, SLOT(setVoltage(double, bool ))) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(showLabelFirstTimeSignal(long, bool, bool )),
					 slotter, SLOT(showLabelFirstTimeForCommand(long, bool, bool ))) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(changeBoardLayersSignal(int, bool )),
	                                 slotter, SLOT(changeBoardLayers(int, bool ))) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(deleteTracesSignal(QSet<ItemBase *> &, QHash<ItemBase *, SketchWidget *> &, QList<long> &, bool, QUndoCommand *)),
	                                 slotter, SLOT(deleteTracesSlot(QSet<ItemBase *> &, QHash<ItemBase *, SketchWidget *> &, QList<long> &, bool, QUndoCommand *)),
	                                 Qt::DirectConnection) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(ratsnestConnectSignal(long, const QString &, bool, bool)),
					 slotter, SLOT(ratsnestConnectForCommand(long, const QString &, bool, bool )),
	                                 Qt::DirectConnection) != nullptr);


	succeeded = succeeded && (connect(signaller, SIGNAL(updatePartLabelInstanceTitleSignal(long)),
	                                 slotter, SLOT(updatePartLabelInstanceTitleSlot(long))) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(changePinLabelsSignal(ItemBase *)),
					 slotter, SLOT(changePinLabelsSlot(ItemBase *))) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(collectRatsnestSignal(QList<SketchWidget *> &)),
	                                 slotter, SLOT(collectRatsnestSlot(QList<SketchWidget *> &)),
	                                 Qt::DirectConnection) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(removeRatsnestSignal(QList<struct ConnectorEdge *> &, QUndoCommand *)),
	                                 slotter, SLOT(removeRatsnestSlot(QList<struct ConnectorEdge *> &, QUndoCommand *)),
	                                 Qt::DirectConnection) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(canConnectSignal(Wire *, ItemBase *, bool &)),
	                                 slotter, SLOT(canConnect(Wire *, ItemBase *, bool &)),
	                                 Qt::DirectConnection) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(swapStartSignal(SwapThing &, bool)),
	                                 slotter, SLOT(swapStart(SwapThing &, bool)),
	                                 Qt::DirectConnection) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(packItemsSignal(int, const QList<long> &, QUndoCommand *, bool)),
					 slotter, SLOT(packItemsForCommand(int, const QList<long> &, QUndoCommand *, bool))) != nullptr);

	succeeded = succeeded && (connect(signaller, SIGNAL(addSubpartSignal(long, long, bool)), slotter, SLOT(addSubpartForCommand(long, long, bool))) != nullptr);

	if (!succeeded) {
		DebugDialog::debug("connectPair failed");
	}

}

void MainWindow::setCurrentFile(const QString &filename, bool addToRecent, bool setAsLastOpened) {
	setFileName(filename);

	if (setAsLastOpened) {
		QSettings settings;
		settings.setValue("lastOpenSketch",filename);

		m_useOldSchematic = false;

		if (m_convertedSchematic) {
			m_convertedSchematic = false;
			QString gridSize = QString("%1in").arg(m_schematicGraphicsView->defaultGridSizeInches());
			m_schematicGraphicsView->setGridSize(gridSize);
			setCurrentTabIndex(2);
			m_schematicGraphicsView->resizeLabels();
			m_schematicGraphicsView->resizeWires();
			m_schematicGraphicsView->updateWires();
			setWindowModified(true);
		}
		else {
			QStringList files = settings.value("lastTabList").toStringList();
			for (int ix = files.count() - 1; ix >= 0; ix--) {
				if (files[ix].mid(1) == filename) {
					bool ok;
					int lastTab = files[ix].left(1).toInt(&ok);
					if (ok) {
						setCurrentTabIndex(lastTab);
					}
				}
			}
		}
	}

	updateRaiseWindowAction();
	setTitle();

	if(addToRecent) {
		QSettings settings;
		QStringList files = settings.value("recentFileList").toStringList();
		files.removeAll(filename);
		files.prepend(filename);
		while (files.size() > MaxRecentFiles)
			files.removeLast();

		settings.setValue("recentFileList", files);
		QTimer::singleShot(1, this, SLOT(updateWelcomeViewRecentList()));

		// TODO: if lastTab file is not on recent list, remove it from the settings
	}

	Q_FOREACH (QWidget *widget, QApplication::topLevelWidgets()) {
		auto *mainWin = qobject_cast<MainWindow *>(widget);
		if (mainWin != nullptr)
			mainWin->updateRecentFileActions();
	}
}


void MainWindow::createZoomOptions(SketchAreaWidget* parent) {

	connect(parent->contentView(), SIGNAL(zoomChanged(double)), this, SLOT(updateZoomSlider(double)));
	connect(parent->contentView(), SIGNAL(zoomOutOfRange(double)), this, SLOT(updateZoomOptionsNoMatterWhat(double)));
}

ExpandingLabel * MainWindow::createRoutingStatusLabel(SketchAreaWidget * parent) {
	auto * routingStatusLabel = new ExpandingLabel(m_pcbWidget);

	connect(routingStatusLabel, SIGNAL(mousePressSignal(QMouseEvent*)), this, SLOT(routingStatusLabelMousePress(QMouseEvent*)));
	connect(routingStatusLabel, SIGNAL(mouseReleaseSignal(QMouseEvent*)), this, SLOT(routingStatusLabelMouseRelease(QMouseEvent*)));

	routingStatusLabel->setTextInteractionFlags(Qt::NoTextInteraction);
	routingStatusLabel->setCursor(Qt::WhatsThisCursor);
	routingStatusLabel->viewport()->setCursor(Qt::WhatsThisCursor);

	routingStatusLabel->setObjectName(SketchAreaWidget::RoutingStateLabelName);
	routingStatusLabel->setToolTip(tr("Click to highlight unconnected parts"));
	parent->setRoutingStatusLabel(routingStatusLabel);
	RoutingStatus routingStatus;
	routingStatus.zero();
	routingStatusSlot(qobject_cast<SketchWidget *>(parent->contentView()), routingStatus);
	return routingStatusLabel;
}

SketchToolButton *MainWindow::createRotateButton(SketchAreaWidget *parent) {
	QList<QAction*> rotateMenuActions;
	rotateMenuActions << m_rotate45ccwAct << m_rotate90ccwAct << m_rotate180Act << m_rotate90cwAct << m_rotate45cwAct;
	auto * rotateButton = new SketchToolButton("Rotate",parent, rotateMenuActions);
	rotateButton->setDefaultAction(m_rotate90ccwAct);
	rotateButton->setText(tr("Rotate"));

	m_rotateButtons << rotateButton;
	return rotateButton;
}

SketchToolButton *MainWindow::createShareButton(SketchAreaWidget *parent) {
	auto *shareButton = new SketchToolButton("Share",parent, m_shareOnlineAct);
	shareButton->setText(tr("Share"));
	shareButton->setObjectName("shareProjectButton");
	shareButton->setEnabledIcon();					// seems to need this to display button icon first time
	return shareButton;
}

SketchToolButton *MainWindow::createFlipButton(SketchAreaWidget *parent) {
	QList<QAction*> flipMenuActions;
	flipMenuActions << m_flipHorizontalAct << m_flipVerticalAct;
	auto *flipButton = new SketchToolButton("Flip",parent, flipMenuActions);
	flipButton->setText(tr("Flip"));

	m_flipButtons << flipButton;
	return flipButton;
}

SketchToolButton *MainWindow::createAutorouteButton(SketchAreaWidget *parent) {
	auto *autorouteButton = new SketchToolButton("Autoroute",parent, m_newAutorouteAct);
	autorouteButton->setText(tr("Autoroute"));
	autorouteButton->setEnabledIcon(); // seems to need this to display button icon first time

	return autorouteButton;
}

SketchToolButton *MainWindow::createOrderFabButton(SketchAreaWidget *parent) {
	auto *orderFabButton = new SketchToolButton("Order",parent, m_orderFabAct);
	orderFabButton->setText(tr("Fabricate"));
	orderFabButton->setObjectName("orderFabButton");
	orderFabButton->setEnabledIcon();// seems to need this to display button icon first time
	connect(orderFabButton, SIGNAL(entered()), this, SLOT(orderFabHoverEnter()));
	connect(orderFabButton, SIGNAL(left()), this, SLOT(orderFabHoverLeave()));

	return orderFabButton;
}

QWidget *MainWindow::createActiveLayerButton(SketchAreaWidget *parent)
{
	QList<QAction *> actions;
	actions << m_activeLayerBothAct << m_activeLayerBottomAct << m_activeLayerTopAct;

	m_activeLayerButtonWidget = new QStackedWidget;
	// m_activeLayerButtonWidget->setMaximumWidth(90);
	m_activeLayerButtonWidget->setObjectName("activeLayerButton");

	auto * button = new SketchToolButton("ActiveLayer", parent, actions);
	button->setDefaultAction(m_activeLayerBottomAct);
	button->setText(tr("Both Layers"));
	m_activeLayerButtonWidget->addWidget(button);

	button = new SketchToolButton("ActiveLayerB", parent, actions);
	button->setDefaultAction(m_activeLayerTopAct);
	button->setText(tr("Bottom Layer"));
	m_activeLayerButtonWidget->addWidget(button);

	button = new SketchToolButton("ActiveLayerT", parent, actions);
	button->setDefaultAction(m_activeLayerBothAct);
	button->setText(tr("Top Layer"));
	m_activeLayerButtonWidget->addWidget(button);

	return m_activeLayerButtonWidget;
}

QWidget *MainWindow::createViewFromButton(SketchAreaWidget *parent)
{
	QList<QAction *> actions;
	actions << m_viewFromAboveAct << m_viewFromBelowAct;

	m_viewFromButtonWidget = new QStackedWidget;
	// m_viewFromButtonWidget->setMaximumWidth(95);
	m_viewFromButtonWidget->setObjectName("viewFromButton");

	auto * button = new SketchToolButton("ViewFromT", parent, actions);
	button->setDefaultAction(m_viewFromBelowAct);
	button->setText(tr("View from Above"));
	button->setEnabledIcon();					// seems to need this to display button icon first time

	m_viewFromButtonWidget->addWidget(button);

	button = new SketchToolButton("ViewFromB", parent, actions);
	button->setDefaultAction(m_viewFromAboveAct);
	button->setText(tr("View from Below"));
	button->setEnabledIcon();					// seems to need this to display button icon first time
	m_viewFromButtonWidget->addWidget(button);

	return m_viewFromButtonWidget;
}

SketchToolButton *MainWindow::createNoteButton(SketchAreaWidget *parent) {
	auto *noteButton = new SketchToolButton("Notes",parent, m_addNoteAct);
	noteButton->setObjectName("noteButton");
	noteButton->setText(tr("Add a note"));
	noteButton->setEnabledIcon();					// seems to need this to display button icon first time
	return noteButton;
}


QWidget *MainWindow::createSimulationButton(SketchAreaWidget *parent) {
	QStackedWidget* widget = new QStackedWidget(parent);
	widget->setObjectName("simulatorbuttonstackwidget");
	widget->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum));

	QToolButton* simulationButton = new QToolButton(widget);
	simulationButton->setObjectName("simulationButton");
	simulationButton->setIconSize(QSize(45,24));
	simulationButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	simulationButton->setDefaultAction(m_startSimulatorAct);
	simulationButton->setText(tr("Simulate"));
	simulationButton->setIcon(QIcon(QPixmap(":/resources/images/icons/toolbarSimulationEnabled_icon.png")));
	widget->addWidget(simulationButton);

	QToolButton* stopSimulationButton = new QToolButton(widget);
	stopSimulationButton->setObjectName("stopSimulationButton");
	stopSimulationButton->setIconSize(QSize(45,24));
	stopSimulationButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	stopSimulationButton->setDefaultAction(m_stopSimulatorAct);
	stopSimulationButton->setText(tr("Stop"));
	stopSimulationButton->setIcon(QIcon(QPixmap(":/resources/images/icons/toolbarStopSimulationEnabled_icon.png")));
	widget->addWidget(stopSimulationButton);

	connect(m_simulator, &Simulator::simulationStartedOrStopped, this, [=](bool running) {
		if (running) {
			widget->setCurrentWidget(stopSimulationButton);
		} else {
			widget->setCurrentWidget(simulationButton);
		}
	});
	connect(m_simulator, &Simulator::simulationEnabled, this, [=](bool enabled) {
		widget->setVisible(enabled);
	});

	emit m_simulator->simulationEnabled(m_simulator->isEnabled());
	emit m_simulator->simulationStartedOrStopped(m_simulator->isSimulating());

	return widget;
}

SketchToolButton *MainWindow::createExportEtchableButton(SketchAreaWidget *parent) {
	QList<QAction*> actions;
	actions << m_exportEtchablePdfAct << m_exportEtchableSvgAct << m_exportGerberAct;
	auto *exportEtchableButton = new SketchToolButton("Diy",parent, actions);
	exportEtchableButton->setObjectName("exportButton");
	exportEtchableButton->setDefaultAction(m_exportEtchablePdfAct);
	exportEtchableButton->setText(tr("Export for PCB"));
	exportEtchableButton->setEnabledIcon();				// seems to need this to display button icon first time
	return exportEtchableButton;
}

QWidget *MainWindow::createToolbarSpacer(SketchAreaWidget *parent) {
	auto *toolbarSpacer = new QFrame(parent);
	auto *spacerLayout = new QHBoxLayout(toolbarSpacer);
	spacerLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding));

	return toolbarSpacer;
}

QList<QWidget*> MainWindow::getButtonsForView(ViewLayer::ViewID viewId) {
	QList<QWidget*> retval;
	SketchAreaWidget *parent;
	switch(viewId) {
	case ViewLayer::BreadboardView:
		parent = m_breadboardWidget;
		break;
	case ViewLayer::SchematicView:
		parent = m_schematicWidget;
		break;
	case ViewLayer::PCBView:
		parent = m_pcbWidget;
		break;
	default:
		return retval;
	}

	switch(viewId) {
	case ViewLayer::BreadboardView:
	case ViewLayer::SchematicView:
		retval << createNoteButton(parent);
	default:
		break;
	}

	retval << createRotateButton(parent);	
	switch (viewId) {
	case ViewLayer::BreadboardView:
		retval << createFlipButton(parent) << createRoutingStatusLabel(parent)
			   << createSimulationButton(parent);
		break;
	case ViewLayer::SchematicView:
		retval << createFlipButton(parent)
		       << createAutorouteButton(parent)
			   << createRoutingStatusLabel(parent)
			   << createSimulationButton(parent);
		break;
	case ViewLayer::PCBView:
		retval << createViewFromButton(parent)
		       << createActiveLayerButton(parent)
		       << createAutorouteButton(parent)
		       << createExportEtchableButton(parent)
		       << createRoutingStatusLabel(parent)
		       ;

		if (m_orderFabEnabled) {
			m_orderFabButton = createOrderFabButton(parent);
			retval << m_orderFabButton;
		}
		break;
	default:
		break;
	}

	retval << createShareButton(parent);
	return retval;
}

void MainWindow::updateZoomSlider(double zoom) {
	m_zoomSlider->setValue(zoom);
}

SketchAreaWidget *MainWindow::currentSketchArea() {
	if (m_currentGraphicsView == nullptr) return nullptr;

	return dynamic_cast<SketchAreaWidget*>(m_currentGraphicsView->parent());
}

void MainWindow::updateZoomOptionsNoMatterWhat(double zoom) {
	m_zoomSlider->setValue(zoom);
}

void MainWindow::updateViewZoom(double newZoom) {
	m_comboboxChanged = true;
	if(m_currentGraphicsView != nullptr) m_currentGraphicsView->absoluteZoom(newZoom);
}


void MainWindow::createStatusBar()
{
	m_statusBar->showMessage(tr("Ready"));
}

void MainWindow::tabWidget_currentChanged(int index) {
	auto * widgetParent = dynamic_cast<SketchAreaWidget *>(currentTabWidget());
	if (widgetParent == nullptr) return;

	m_currentWidget = widgetParent;

	if (m_locationLabel != nullptr) {
		m_locationLabel->setText("");
	}

	QStatusBar *sb = statusBar();
	connect(sb, SIGNAL(messageChanged(const QString &)), this, SLOT(showStatusMessage(const QString &)));
	widgetParent->addStatusBar(m_statusBar);
	if(sb != m_statusBar) sb->hide();

	if (m_breadboardGraphicsView != nullptr) m_breadboardGraphicsView->setCurrent(false);
	if (m_schematicGraphicsView != nullptr) m_schematicGraphicsView->setCurrent(false);
	if (m_pcbGraphicsView != nullptr) m_pcbGraphicsView->setCurrent(false);

	auto *widget = qobject_cast<SketchWidget *>(widgetParent->contentView());

	if(m_currentGraphicsView != nullptr) {
		m_currentGraphicsView->saveZoom(m_zoomSlider->value());
		disconnect(
		    m_currentGraphicsView,
		    SIGNAL(selectionChangedSignal()),
		    this,
		    SLOT(updateTransformationActions())
		);
	}
	m_currentGraphicsView = widget;

	if (m_programView != nullptr) {
		hideShowProgramMenu();
	}

	hideShowTraceMenu();
	updateEditMenu();

	if (m_showBreadboardAct != nullptr) {
		QList<QAction *> actions;
		if (m_welcomeView != nullptr) actions << m_showWelcomeAct;
		actions << m_showBreadboardAct << m_showSchematicAct << m_showPCBAct;
		if (m_programView != nullptr) actions << m_showProgramAct;
		setActionsIcons(index, actions);
	}

	setTitle();

	if (widget == nullptr) {
		m_firstTimeHelpAct->setEnabled(false);
		return;
	}

	m_zoomSlider->setValue(m_currentGraphicsView->retrieveZoom());
	FirstTimeHelpDialog::setViewID(m_currentGraphicsView->viewID());
	m_firstTimeHelpAct->setEnabled(true);

	connect(
	    m_currentGraphicsView,					// don't connect directly to the scene here, connect to the widget's signal
	    SIGNAL(selectionChangedSignal()),
	    this,
	    SLOT(updateTransformationActions())
	);

	updateActiveLayerButtons();

	m_currentGraphicsView->setCurrent(true);

	// !!!!!! hack alert  !!!!!!!
	// this item update loop seems to deal with a qt update bug:
	// if one view is visible and you change something in another view,
	// the change might not appear when you switch views until you move the item in question
	Q_FOREACH(QGraphicsItem * item, m_currentGraphicsView->items()) {
		item->update();
	}

	updateLayerMenu(true);
	updateTraceMenu();
	updateTransformationActions();

	if (m_infoView != nullptr) {
		m_currentGraphicsView->updateInfoView();
	}

	// update issue with 4.5.1?: is this still valid (4.6.x?)
	m_currentGraphicsView->updateConnectors();

	QTimer::singleShot(10, this, SLOT(initZoom()));

}

void MainWindow::setActionsIcons(int index, QList<QAction *> & actions) {
	for (int i = 0; i < actions.count(); i++) {
		// DebugDialog::debug(QString("setting icon %1 %2").arg(i).arg(index == i));
		// setting the icons seems to be broken in Qt 5, so use checkMarks instead
		// note that we used dots instead of checkMarks originally because
		// we hoped it was clearer that the items were mutually exclusive
		// note that using QIcon() instead of m_emptyIcon does no good
		// (and we used the m_emptyIcon to preserve the space at the left)
		// actions[i]->setIcon(index == i ? m_dotIcon : m_emptyIcon);
		actions[i]->setChecked(index == i);
	}
}

void MainWindow::closeEvent(QCloseEvent *event) {
	if (m_dontClose) {
		event->ignore();
		return;
	}

	if (m_programWindow != nullptr) {
		m_programWindow->close();
		if (m_programWindow->isVisible()) {
			event->ignore();
			return;
		}
	}

	if (!m_closeSilently) {
		bool whatWithAliens = whatToDoWithAlienFiles();
		bool discard;
		if(!beforeClosing(true, discard) || !whatWithAliens ||!m_binManager->beforeClosing()) {
			event->ignore();
			return;
		}

		if(whatWithAliens && m_binManager->hasAlienParts()) {
			m_binManager->createIfMyPartsNotExists();
		}
	}

	//DebugDialog::debug(QString("top level windows: %1").arg(QApplication::topLevelWidgets().size()));
	/*
	foreach (QWidget * widget, QApplication::topLevelWidgets()) {
		QMenu * menu = qobject_cast<QMenu *>(widget);
		if (menu) {
			continue;				// QMenus are always top level widgets, even if they have parents...
		}
		DebugDialog::debug(QString("top level widget %1 %2 %3")
			.arg(widget->metaObject()->className())
			.arg(widget->windowTitle())
			.arg(widget->toolTip())
			);
	}
	*/

	m_closing = true;

	int count = 0;
	Q_FOREACH (QWidget *widget, QApplication::topLevelWidgets()) {
		if (widget == this) continue;
		if (qobject_cast<QMainWindow *>(widget) == NULL) continue;

		count++;
	}

	if (count == 0) {
		DebugDialog::closeDebug();
	}

	QSettings settings;
	settings.setValue(m_settingsPrefix + "state",saveState());
	settings.setValue(m_settingsPrefix + "geometry",saveGeometry());

	saveLastTabList();

	QMainWindow::closeEvent(event);
}

void MainWindow::saveLastTabList() {
	QSettings settings;
	QStringList files = settings.value("lastTabList").toStringList();
	for (int ix = files.count() - 1; ix >= 0; ix--) {
		if (files[ix].mid(1) == m_fwFilename) files.removeAt(ix);
	}
	files.prepend(QString("%1%2").arg(this->currentTabIndex()).arg(m_fwFilename));
	while (files.size() > MaxRecentFiles)
		files.removeLast();

	settings.setValue("lastTabList", files);
}

bool MainWindow::whatToDoWithAlienFiles() {
	if (m_alienFiles.size() > 0) {
		QString basename = QFileInfo(m_fwFilename).fileName();
		QMessageBox::StandardButton reply;
		QString	alienPartsMsg = tr("Do you want to keep the imported parts?");
		reply = QMessageBox::question(this, tr("Save %1").arg(basename),
									  alienPartsMsg,
		                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

		if (reply == QMessageBox::Yes) {
			return true;
		} else if (reply == QMessageBox::No) {
			Q_FOREACH(QString pathToRemove, m_alienFiles) {
				QFile::remove(pathToRemove);
			}
			m_alienFiles.clear();
			recoverBackupedFiles();

			Q_EMIT alienPartsDismissed();
			return true;
		}
		else {
			return false;
		}
	} else {
		return true;
	}
}

void MainWindow::acceptAlienFiles() {
	m_alienFiles.clear();
}

bool MainWindow::eventFilter(QObject *object, QEvent *event) {
	if (object == this &&
	        (event->type() == QEvent::KeyPress
	         // || event->type() == QEvent::KeyRelease
	         || event->type() == QEvent::ShortcutOverride))
	{
		updatePartMenu();
		updateTraceMenu();

		// On the mac, the first time the delete key is pressed, to be used as a shortcut for QAction m_deleteAct,
		// for some reason, the enabling of the m_deleteAct in UpdateEditMenu doesn't "take" until the next time the event loop is processed
		// Thereafter, the delete key works as it should.
		// So this call to processEvents() makes sure m_deleteAct is enabled.
		ProcessEventBlocker::processEvents();
	}

#if defined(Q_OS_MAC)

	// Need to process Backspace on Mac to workaround bug in Qt5
	// See http://qt-project.org/forums/viewthread/36174

	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		if (keyEvent->key() == Qt::Key_Backspace) {
			Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
			if (modifiers == Qt::NoModifier) {
				doDelete();
				return true;
			}
			if (modifiers == Qt::AltModifier) {
				doDeleteMinus();
				return true;
			}
		}
	}
#endif

	return QMainWindow::eventFilter(object, event);
}

const QString MainWindow::untitledFileName() {
	return UntitledSketchName;
}

int &MainWindow::untitledFileCount() {
	return UntitledSketchIndex;
}

const QString MainWindow::fileExtension() {
	return FritzingBundleExtension;
}

const QString MainWindow::defaultSaveFolder() {
	return FolderUtils::openSaveFolder();
}

bool MainWindow::undoStackIsEmpty() {
	return m_undoStack->count() == 0;
}

void MainWindow::setInfoViewOnHover(bool infoViewOnHover) {
	m_breadboardGraphicsView->setInfoViewOnHover(infoViewOnHover);
	m_schematicGraphicsView->setInfoViewOnHover(infoViewOnHover);
	m_pcbGraphicsView->setInfoViewOnHover(infoViewOnHover);

	m_binManager->setInfoViewOnHover(infoViewOnHover);
}

QString MainWindow::loadBundledSketch(const QString &fileName, bool addToRecent, bool setAsLastOpened, bool checkObsolete) {

	QString error;
	if(!FolderUtils::unzipTo(fileName, m_fzzFolder, error)) {
		FMessageBox::warning(
		    this,
		    tr("Fritzing"),
		    tr("Unable to open '%1': %2").arg(fileName).arg(error)
		);

		return QString("Unable to unzip '%1': %2").arg(fileName).arg(error);
	}

	QDir dir(m_fzzFolder);
	QString binFileName = dir.absoluteFilePath(QFileInfo(BinManager::TempPartsBinTemplateLocation).fileName());
	m_binManager->setTempPartsBinLocation(binFileName);
	FolderUtils::copyBin(binFileName, BinManager::TempPartsBinTemplateLocation);

	QStringList namefilters;
	namefilters << "*"+FritzingSketchExtension;
	QFileInfoList entryInfoList = dir.entryInfoList(namefilters);
	if (entryInfoList.count() == 0) {
		FMessageBox::warning(
		    this,
		    tr("Fritzing"),
		    tr("No Sketch found in '%1'").arg(fileName)
		);

		return "";
	}

	QFileInfo sketchInfo = entryInfoList[0];

	QString sketchName = dir.absoluteFilePath(sketchInfo.fileName());

	namefilters.clear();
	namefilters << "*" + FritzingPartExtension;
	entryInfoList = dir.entryInfoList(namefilters);

	namefilters.clear();
	namefilters << "*.svg";
	QFileInfoList svgEntryInfoList = dir.entryInfoList(namefilters);

	m_addedToTemp = false;

	QList<MissingSvgInfo> missing;
	QList<ModelPart *> missingModelParts;

	Q_FOREACH (QFileInfo fzpInfo, entryInfoList) {
		QFile file(dir.absoluteFilePath(fzpInfo.fileName()));
		if (!file.open(QFile::ReadOnly)) {
			DebugDialog::debug(QString("unable to open %1: %2").arg(file.fileName()));
			continue;
		}

		// TODO: could be more efficient by using a streamreader
		QString fzp = file.readAll();
		file.close();

		QString moduleID = TextUtils::parseForModuleID(fzp);
		if (moduleID.isEmpty()) {
			DebugDialog::debug("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
			DebugDialog::debug(QString("unable to find module id in %1: %2").arg(file.fileName()).arg(fzp));
			DebugDialog::debug("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
			continue;
		}

		ModelPart * mp = m_referenceModel->retrieveModelPart(moduleID);
		if (mp == nullptr) {
			QDomDocument doc;
			QString errorStr;
			int errorLine;
			int errorColumn;
			if (!doc.setContent(fzp, &errorStr, &errorLine, &errorColumn)) {
				DebugDialog::debug(QString("unable to parse fzp in %1. line: %2 column: %3 error: %4 fzp: %5").arg(file.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr).arg(fzp));
				FMessageBox::warning(
				    this,
				    tr("Fritzing"),
				    tr("unable to parse fzp in %1. line: %2 column: %3 error: %4").arg(file.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr)
				);
				continue;
			}

			mp = copyToPartsFolder(fzpInfo, false, PartFactory::folderPath(), "contrib");
			if (mp == nullptr) {
				DebugDialog::debug(QString("unable to create model part in %1: %2").arg(file.fileName()).arg(fzp));
				continue;
			}

			QDomElement root = doc.documentElement();
			QDomElement views = root.firstChildElement("views");
			QDomElement view = views.firstChildElement();
			while (!view.isNull()) {
				QDomElement layers = view.firstChildElement("layers");
				QString path = layers.attribute("image", "");
				if (!path.isEmpty()) {
					bool copied = copySvg(path, svgEntryInfoList);
					if (!copied) {
						DebugDialog::debug(QString("missing svg %1").arg(path));
						MissingSvgInfo msi;
						msi.equal = false;
						msi.modelPart = mp;
						missingModelParts << mp;
						msi.requestedPath = path;
						ViewLayer::ViewID viewID = ViewLayer::idFromXmlName(view.tagName());
						QDomElement connectors = root.firstChildElement("connectors");
						QDomElement connector = connectors.firstChildElement("connector");
						while (!connector.isNull()) {
							QString id, terminalID;
							ViewLayer::getConnectorSvgIDs(connector, viewID, id, terminalID);
							if (!id.isEmpty()) {
								msi.connectorSvgIds.append(id);
							}
							connector = connector.nextSiblingElement("connector");
						}
						missing << msi;
					}
				}
				view = view.nextSiblingElement();
			}

			mp->setFzz(true);
		}

		if (!missingModelParts.contains(mp)) {
			m_binManager->addToTempPartsBin(mp);
			m_addedToTemp = true;
		}
	}

	std::sort(missing.begin(), missing.end(), byConnectorCount);
	Q_FOREACH (MissingSvgInfo msi, missing) {
		if (msi.equal) {
			// two or more parts have the same number of connectors--so we can't figure out how to assign them
			continue;
		}

		int slash = msi.requestedPath.indexOf("/");
		QString suffix = msi.requestedPath.mid(slash + 1);
		QString prefix = msi.requestedPath.left(slash);
		for (int jx = svgEntryInfoList.count() - 1; jx >= 0; jx--) {
			QFileInfo svgInfo = svgEntryInfoList.at(jx);
			if (!svgInfo.fileName().contains(prefix, Qt::CaseInsensitive)) continue;

			QFile svgfile(svgInfo.absoluteFilePath());
			if (!svgfile.open(QIODevice::ReadOnly)) {
				DebugDialog::debug(QString("Unable to open :%1").arg(svgInfo.absoluteFilePath()));
			}
			QDomDocument svgDoc;
			if (!svgDoc.setContent(&svgfile)) continue;

			QList<QDomElement> elements;
			QDomElement root = svgDoc.documentElement();
			TextUtils::findElementsWithAttribute(root, "id", elements);
			if (elements.count() < msi.connectorSvgIds.count()) continue;

			QStringList ids;
			Q_FOREACH (QDomElement element, elements) {
				ids << element.attribute("id");
			}

			bool allGood = true;
			Q_FOREACH (QString id, msi.connectorSvgIds) {
				if (!ids.contains(id)) {
					allGood = false;
					break;
				}
			}

			if (!allGood) continue;

			QString destPath = copyToSvgFolder(svgInfo, false, PartFactory::folderPath(), "contrib");   // copy file with original name
			if (!destPath.isEmpty()) {
				QFileInfo destInfo(destPath);
				QFile file(destPath);
				DebugDialog::debug(QString("found missing %1").arg(destPath));
				FolderUtils::slamCopy(file, destInfo.absoluteDir().absoluteFilePath(suffix));         // make another copy that has the name used in the fzp file
				svgEntryInfoList.removeAt(jx);
				break;
			}
		}
	}

	Q_FOREACH (ModelPart * mp, missingModelParts) {
		m_binManager->addToTempPartsBin(mp);
		m_addedToTemp = true;
	}

	if (!m_addedToTemp) {
		m_binManager->hideTempPartsBin();
	}

	// the bundled itself
	bool result = this->mainLoad(sketchName, "", checkObsolete);


	setCurrentFile(fileName, addToRecent, setAsLastOpened);
	if (!result) return QString("Unable to load sketch: '%1' filename: %2").arg(sketchName).arg(fileName);
	return "";
}

bool MainWindow::copySvg(const QString & path, QFileInfoList & svgEntryInfoList)
{
	int slash = path.indexOf("/");
	QString subpath = path.mid(slash + 1);
	bool gotOne = false;
	for (int jx = svgEntryInfoList.count() - 1; jx >= 0; jx--) {
		QFileInfo svgInfo = svgEntryInfoList.at(jx);
		if (svgInfo.fileName().contains(subpath)) {
			copyToSvgFolder(svgInfo, false, PartFactory::folderPath(), "contrib");
			svgEntryInfoList.removeAt(jx);
			// jrc 30 oct 2012: not sure why we can't just return at this point--can there be other matching files?
			gotOne = true;
		}
	}

	if (gotOne) return true;

	// deal with a bug in which all the svg files exist in the fzz but the fz file points to the wrong name
	// most of the time it's just a GUID difference

	DebugDialog::debug(QString("svg matching fz path %1 not found").arg(path));
	QRegularExpressionMatch match;
	int guidix = subpath.lastIndexOf(GuidMatcher, -1, &match);
	if (guidix < 0) return false;

	QString originalGuid = match.captured(0);
	QString tryPath = subpath;
	tryPath.replace(guidix, originalGuid.length(), "%%%%");
	for (int jx = svgEntryInfoList.count() - 1; jx >= 0; jx--) {
		QFileInfo svgInfo = svgEntryInfoList.at(jx);
		QString tempPath = svgInfo.fileName();
		QRegularExpressionMatch match;
		guidix = tempPath.lastIndexOf(GuidMatcher, -1, &match);
		if (guidix < 0) continue;

		tempPath.replace(guidix, match.captured(0).length(), "%%%%");
		if (!tempPath.contains(tryPath)) continue;

		QString destPath = copyToSvgFolder(svgInfo, false, PartFactory::folderPath(), "contrib");
		if (!destPath.isEmpty()) {
			QFile file(destPath);
			match = QRegularExpressionMatch();
			guidix = destPath.lastIndexOf(GuidMatcher, -1, &match);
			destPath.replace(guidix, match.captured(0).length(), originalGuid);
			FolderUtils::slamCopy(file, destPath);
			DebugDialog::debug(QString("found matching svg %1").arg(destPath));
			svgEntryInfoList.removeAt(jx);
			return true;
		}
	}

	return false;

}


bool MainWindow::loadBundledNonAtomicEntity(const QString &fileName, Bundler* bundler, bool addToBin, bool dontAsk) {
	QDir destFolder = QDir::temp();

	FolderUtils::createFolderAndCdIntoIt(destFolder, TextUtils::getRandText());
	QString unzipDirPath = destFolder.path();

	QString error;
	if(!FolderUtils::unzipTo(fileName, unzipDirPath, error)) {
		FMessageBox::warning(
		    this,
		    tr("Fritzing"),
		    tr("Unable to open shareable '%1': %2").arg(fileName).arg(error)
		);

		// gotta return now, or loadBundledSketchAux will crash
		return false;
	}

	QDir unzipDir(unzipDirPath);

	if (bundler->preloadBundledAux(unzipDir, dontAsk)) {
		QList<ModelPart*> mps = moveToPartsFolder(unzipDir,this,addToBin,true,FolderUtils::getUserPartsPath(), "contrib", false);
		// the bundled itself
		bundler->loadBundledAux(unzipDir,mps);
	}

	FolderUtils::rmdir(unzipDirPath);

	return true;
}

/*
void MainWindow::loadBundledPartFromWeb() {
	QMainWindow *mw = new QMainWindow();
	QString url = "http://localhost:8081/parts_gen/choose";
	QWebView *view = new QWebView(mw);
	mw->setCentralWidget(view);
	view->setUrl(QUrl(url));
	mw->show();
	mw->raise();
}
*/

QList<ModelPart*> MainWindow::loadPart(const QString &fzpFile, bool addToBin) {
	QDir destFolder = QDir::temp();

	FolderUtils::createFolderAndCdIntoIt(destFolder, TextUtils::getRandText());
	QString tmpDirPath = destFolder.path();

	QDir tmpDir(tmpDirPath);

	QList<ModelPart*> mps;
	QMap<QString, QString> map;

	if ( QFileInfo::exists(fzpFile)) {
		map = TextUtils::parseFileForViewImages(fzpFile);
		if(map.count() != 4) {
			FMessageBox::warning(
				this,
				tr("Fritzing"),
				tr("Local part '%1' incomplete, only '%2' layers.").arg(fzpFile).arg(map.count())
			);
		}
		QDir path = QFileInfo(fzpFile).path();
		Q_FOREACH(QString key, map.keys()) {
			QString file = map[key];
			if(!file.startsWith(key + "/")) {
				// We can add the missing "svg." prefix here.
				// However, we can not add the view prefix, as we would have to modify the part file.
				// Idea: Why not let Fritzing derive the folder from the view id or a real folder name
				// instead of the using a filename prefix? Like we do it in the above call to
				// parseFileForViewImages(). Doing this would require some
				// changes and a lot of tests, but might simplify creating parts in the future.
				FMessageBox::warning(
					this,
					tr("Fritzing"),
					tr("View '%1' should be prefixed with '%2/'. Trying to continue.").arg(file, key)
				);
			}

			if (key == "icon" && file.startsWith("breadboard")) {
				// We make an exception for icon files, allow to reuse the breadboard view.
				continue;
			}

			if(!QFileInfo::exists(path.filePath(file))) {
				//tr("File '%1' for view '%2' not found in subfolder, trying prefix instead of folder.").arg(file, key)
				// e.g. "breadboard_whatever/image.svg" -> "breadboard.image.svg"
				file = key + "." + QFileInfo(file).fileName();
			}

			if(QFileInfo::exists(path.filePath(file))) {
				if (!QFile::copy(path.filePath(file), tmpDir.filePath("svg." + file))) {
					FMessageBox::warning(
						this,
						tr("Fritzing"),
						tr("Could not copy subfile '%1' to '%2'").arg(path.filePath(file), tmpDir.filePath("svg." + file))
					);
					return QList<ModelPart*>();
				}
			} else {
				FMessageBox::warning(
					this,
					tr("Fritzing"),
					tr("Local part '%1' incomplete, subfile not found '%2'").arg(fzpFile, file)
				);
				return QList<ModelPart*>();
			}
		}
		QString fzpTemp = QFileInfo(fzpFile).fileName();
		if(!fzpTemp.startsWith("part.")) {
			fzpTemp = "part." + fzpTemp;
		}
		QFile::copy(fzpFile, tmpDir.filePath(fzpTemp));
	} else {
		FMessageBox::warning(
			this,
			tr("Fritzing"),
			tr("Unable to open local part '%1'").arg(fzpFile)
		);
		return QList<ModelPart*>();
	}


	try {
		mps = moveToPartsFolder(tmpDir, this, addToBin, true, FolderUtils::getUserPartsPath(), "user", true);
	}
	catch (const QString & msg) {
		FMessageBox::warning(
			this,
			tr("Fritzing"),
			msg
		);
		return QList<ModelPart*>();
	}

	if (mps.count() < 1) {
		// if this fails, that means that the bundling failed
		FMessageBox::warning(
			this,
			tr("Fritzing"),
			tr("Unable to load part from '%1'").arg(fzpFile)
		);
		return QList<ModelPart*>();
	}

	FolderUtils::rmdir(tmpDirPath);

	return mps;
}

QList<ModelPart*> MainWindow::loadBundledPart(const QString &fileName, bool addToBin) {
	QDir destFolder = QDir::temp();

	FolderUtils::createFolderAndCdIntoIt(destFolder, TextUtils::getRandText());
	QString unzipDirPath = destFolder.path();

	QString error;
	if(!FolderUtils::unzipTo(fileName, unzipDirPath, error)) {
		FMessageBox::warning(
		    this,
		    tr("Fritzing"),
		    tr("Unable to open shareable part '%1': %2").arg(fileName).arg(error)
		);
		return QList<ModelPart*>();
	}

	QDir unzipDir(unzipDirPath);

	QList<ModelPart*> mps;
	try {
		mps = moveToPartsFolder(unzipDir, this, addToBin, true, FolderUtils::getUserPartsPath(), "user", true);
	}
	catch (const QString & msg) {
		FMessageBox::warning(
		    this,
		    tr("Fritzing"),
		    msg
		);
		return QList<ModelPart*>();
	}

	if (mps.count() < 1) {
		// if this fails, that means that the bundled was wrong
		FMessageBox::warning(
		    this,
		    tr("Fritzing"),
		    tr("Unable to load part from '%1'").arg(fileName)
		);
		return QList<ModelPart*>();
	}

	FolderUtils::rmdir(unzipDirPath);

	return mps;
}

void MainWindow::saveBundledPart(const QString &moduleId) {
	QString modIdToExport;
	ModelPart* mp;

	if(moduleId.isEmpty()) {
		if (m_currentGraphicsView == nullptr) return;
		PaletteItem *selectedPart = m_currentGraphicsView->getSelectedPart();
		mp = selectedPart->modelPart();
		modIdToExport = mp->moduleID();
	} else {
		modIdToExport = moduleId;
		mp = m_referenceModel->retrieveModelPart(moduleId);
	}
	QString partTitle = mp->title();

	QString fileExt;
	QString path = defaultSaveFolder()+"/"+partTitle+FritzingBundledPartExtension;
	QString bundledFileName = FolderUtils::getSaveFileName(
	                              this,
	                              tr("Specify a file name"),
	                              path,
	                              tr("Fritzing Part (*%1)").arg(FritzingBundledPartExtension),
	                              &fileExt
	                          );

	if (bundledFileName.isEmpty()) return; // Cancel pressed

	if(!alreadyHasExtension(bundledFileName, FritzingBundledPartExtension)) {
		bundledFileName += FritzingBundledPartExtension;
	}

	QDir destFolder = QDir::temp();

	FolderUtils::createFolderAndCdIntoIt(destFolder, TextUtils::getRandText());
	QString dirToRemove = destFolder.path();

	QString aux = QFileInfo(bundledFileName).fileName();
	QString destPartPath = // remove the last "z" from the extension
	    destFolder.path()+"/"+aux.left(aux.size()-1);
	DebugDialog::debug("saving part temporarily to "+destPartPath);

	//bool wasModified = isWindowModified();
	//QString prevFileName = m_fileName;
	//saveAsAux(destPartPath);
	//m_fileName = prevFileName;
	//setWindowModified(wasModified);
	setTitle();

	saveBundledAux(mp, destFolder);

	QStringList skipSuffixes;
	if(!FolderUtils::createZipAndSaveTo(destFolder, bundledFileName, skipSuffixes)) {
		FMessageBox::warning(
		    this,
		    tr("Fritzing"),
		    tr("Unable to export %1 to shareable sketch").arg(bundledFileName)
		);
	}

	FolderUtils::rmdir(dirToRemove);
}

QStringList MainWindow::saveBundledAux(ModelPart *mp, const QDir &destFolder) {
	QStringList names;
	QString partPath = mp->path();
	QFile file(partPath);
	QString fn = ZIP_PART + QFileInfo(partPath).fileName();
	names << fn;
	FolderUtils::slamCopy(file, destFolder.path()+"/"+fn);

	QList<ViewLayer::ViewID> viewIDs;
	viewIDs << ViewLayer::IconView << ViewLayer::BreadboardView << ViewLayer::SchematicView << ViewLayer::PCBView;
	Q_FOREACH (ViewLayer::ViewID viewID, viewIDs) {
		QString basename = mp->hasBaseNameFor(viewID);
		if (basename.isEmpty()) continue;

		QString filename = PartFactory::getSvgFilename(mp, basename, true, true);
		if (filename.isEmpty()) continue;

		QFile file(filename);
		basename.replace("/", ".");
		QString fn = ZIP_SVG + basename;
		names << fn;
		FolderUtils::slamCopy(file, destFolder.path()+"/"+fn);
	}

	return names;
}

QList<ModelPart*> MainWindow::moveToPartsFolder(QDir &unzipDir, MainWindow* mw, bool addToBin, bool addToAlien, const QString & prefixFolder, const QString &destFolder, bool importingSinglePart) {
	QStringList namefilters;
	QList<ModelPart*> retval;

	if (mw == nullptr) {
		throw "MainWindow::moveToPartsFolder mainwindow missing";
	}

	namefilters.clear();
	namefilters << ZIP_PART+"*";
	QList<QFileInfo> partEntryInfoList = unzipDir.entryInfoList(namefilters);

	if (importingSinglePart && partEntryInfoList.count() > 0) {
		QString moduleID = TextUtils::parseFileForModuleID(partEntryInfoList[0].absoluteFilePath());
		if (!moduleID.isEmpty() && (m_referenceModel->retrieveModelPart(moduleID) != nullptr)) {
			throw QString("There is already a part with id '%1' loaded into Fritzing.").arg(moduleID);
		}
	}


	namefilters.clear();
	namefilters << ZIP_SVG+"*";
	Q_FOREACH(QFileInfo file, unzipDir.entryInfoList(namefilters)) { // svg files
		//DebugDialog::debug("unzip svg " + file.absoluteFilePath());
		mw->copyToSvgFolder(file, addToAlien, prefixFolder, destFolder);
	}


	Q_FOREACH(QFileInfo file, partEntryInfoList) { // part files
		//DebugDialog::debug("unzip part " + file.absoluteFilePath());
		ModelPart * mp = mw->copyToPartsFolder(file, addToAlien, prefixFolder, destFolder);
		retval << mp;
		if (addToBin && (mp != nullptr)) {
			// should only be here when adding single new part
			m_binManager->addToMyParts(mp);
		}
	}



	return retval;
}

QString MainWindow::copyToSvgFolder(const QFileInfo& file, bool addToAlien, const QString & prefixFolder, const QString &destFolder) {
	QFile svgfile(file.filePath());
	// let's make sure that we remove just the suffix
	QString fileName = file.fileName().remove(QRegularExpression("^"+ZIP_SVG));
	QString viewFolder = fileName.left(fileName.indexOf("."));
	fileName.remove(0, viewFolder.length() + 1);

	QString destFilePath =
	    prefixFolder+"/svg/"+destFolder+"/"+viewFolder+"/"+fileName;

	backupExistingFileIfExists(destFilePath);
	if(FolderUtils::slamCopy(svgfile, destFilePath)) {
		if (addToAlien) {
			m_alienFiles << destFilePath;
		}
		return destFilePath;
	}

	return "";
}

ModelPart* MainWindow::copyToPartsFolder(const QFileInfo& file, bool addToAlien, const QString & prefixFolder, const QString &destFolder) {
	QFile partfile(file.filePath());
	// let's make sure that we remove just the suffix
	QString destFilePath =
	    prefixFolder+"/"+destFolder+"/"+file.fileName().remove(QRegularExpression("^"+ZIP_PART));

	backupExistingFileIfExists(destFilePath);
	if(FolderUtils::slamCopy(partfile, destFilePath)) {
		if (addToAlien) {
			m_alienFiles << destFilePath;
		}
	}
	ModelPart *mp = m_referenceModel->loadPart(destFilePath, true);
	if (mp != nullptr) {
		mp->setAlien(true);
	} else {
		// Part load failed, remove modified files before proceeding.
		Q_FOREACH(QString pathToRemove, m_alienFiles) {
			QFile::remove(pathToRemove);
		}
		m_alienFiles.clear();
		recoverBackupedFiles();
		Q_EMIT alienPartsDismissed();
	}

	return mp;
}

void MainWindow::binSaved(bool hasPartsFromBundled) {
	if(hasPartsFromBundled) {
		// the bin will need those parts, so just keep them
		m_alienFiles.clear();
		resetTempFolder();
	}
}

#undef ZIP_PART
#undef ZIP_SVG


void MainWindow::backupExistingFileIfExists(const QString &destFilePath) {
	if(QFileInfo(destFilePath).exists()) {
		if(m_tempDir.path() == ".") {
			m_tempDir = QDir::temp();
			FolderUtils::createFolderAndCdIntoIt(m_tempDir, TextUtils::getRandText());
			DebugDialog::debug("debug folder for overwritten files: "+m_tempDir.path());
		}

		QString fileBackupName = QFileInfo(destFilePath).fileName();
		m_filesReplacedByAlienOnes << destFilePath;
		QFile file(destFilePath);
		bool alreadyExists = file.exists();
		FolderUtils::slamCopy(file, m_tempDir.path()+"/"+fileBackupName);

		if(alreadyExists) {
			file.remove(destFilePath);
		}
	}
}

void MainWindow::recoverBackupedFiles() {
	Q_FOREACH(QString originalFilePath, m_filesReplacedByAlienOnes) {
		QFile file(m_tempDir.path()+"/"+QFileInfo(originalFilePath).fileName());
		if(file.exists(originalFilePath)) {
			file.remove();
		}
		FolderUtils::slamCopy(file, originalFilePath);
	}
	resetTempFolder();
}

void MainWindow::resetTempFolder() {
	if(m_tempDir.path() != ".") {
		FolderUtils::rmdir(m_tempDir);
		m_tempDir = QDir::temp();
	}
	m_filesReplacedByAlienOnes.clear();
}

void MainWindow::routingStatusSlot(SketchWidget * sketchWidget, const RoutingStatus & routingStatus) {
	m_routingStatus = routingStatus;
	QString theText;
	if (routingStatus.m_netCount == 0) {
		theText = tr("No connections to route");
	} else if (routingStatus.m_netCount == routingStatus.m_netRoutedCount) {
		if (routingStatus.m_jumperItemCount == 0) {
			theText = tr("Routing completed");
		}
		else {
			theText = tr("Routing completed using %n jumper part(s)", "", routingStatus.m_jumperItemCount);
		}
	} else {
		theText = tr("%1 of %2 nets routed - %n connector(s) still to be routed", "", routingStatus.m_connectorsLeftToRoute)
		          .arg(routingStatus.m_netRoutedCount)
		          .arg(routingStatus.m_netCount);
	}

	dynamic_cast<SketchAreaWidget *>(sketchWidget->parent())->routingStatusLabel()->setLabelText(theText);

	updateTraceMenu();
}

void MainWindow::applyReadOnlyChange(bool isReadOnly) {
	Q_UNUSED(isReadOnly);
	//m_saveAct->setDisabled(isReadOnly);
}

const QString MainWindow::fritzingTitle() {
	if (m_currentGraphicsView == nullptr) {
		return FritzingWindow::fritzingTitle();
	}

	QString fritzing = FritzingWindow::fritzingTitle();
	return tr("%1 - [%2]").arg(fritzing).arg(m_currentGraphicsView->viewName());
}

QAction *MainWindow::raiseWindowAction() {
	return m_raiseWindowAct;
}

void MainWindow::raiseAndActivate() {
	if(isMinimized()) {
		showNormal();
	}
	raise();
	QTimer::singleShot(20, this, SLOT(activateWindowAux()));
}

void MainWindow::activateWindowAux() {
	activateWindow();
}

void MainWindow::updateRaiseWindowAction() {
	QString actionText;
	QFileInfo fileInfo(m_fwFilename);
	if(fileInfo.exists()) {
		int lastSlashIdx = m_fwFilename.lastIndexOf("/");
		int beforeLastSlashIdx = m_fwFilename.left(lastSlashIdx).lastIndexOf("/");
		actionText = beforeLastSlashIdx > -1 && lastSlashIdx > -1 ? "..." : "";
		actionText += m_fwFilename.right(m_fwFilename.size()-beforeLastSlashIdx-1);
	} else {
		actionText = m_fwFilename;
	}
	m_raiseWindowAct->setText(actionText);
	m_raiseWindowAct->setToolTip(m_fwFilename);
	m_raiseWindowAct->setStatusTip("raise \""+m_fwFilename+"\" window");
}

QSizeGrip *MainWindow::sizeGrip() {
	return m_sizeGrip;
}

QStatusBar *MainWindow::realStatusBar() {
	return m_statusBar;
}

void MainWindow::moveEvent(QMoveEvent * event) {
	FritzingWindow::moveEvent(event);
	Q_EMIT mainWindowMoved(this);
}

bool MainWindow::event(QEvent * e) {
	switch (e->type()) {
	case QEvent::WindowActivate:
		Q_EMIT changeActivationSignal(true, this);
		break;
	case QEvent::WindowDeactivate:
		Q_EMIT changeActivationSignal(false, this);
		break;
	default:
		break;
	}
	return FritzingWindow::event(e);
}

void MainWindow::resizeEvent(QResizeEvent * event) {
	if (m_sizeGrip != nullptr) {
		m_sizeGrip->rearrange();
	}
	FritzingWindow::resizeEvent(event);
}

void MainWindow::enableCheckUpdates(bool enabled)
{
	if (m_checkForUpdatesAct != nullptr) {
		m_checkForUpdatesAct->setEnabled(enabled);
	}
}

void MainWindow::migratePartLabelOffset(QList<ModelPart*> modelParts) {
	QList<QString> migratedParts;

	for (ModelPart* modelPart : modelParts) {
		for(ViewLayer::ViewID id : { ViewLayer::ViewID::SchematicView, ViewLayer::ViewID::PCBView }) {
			ItemBase *item = modelPart->viewItem(id);
			if (item) {
				auto pair = item->migratePartLabel();
				if (pair.second) {
					migratedParts << pair.first;
				}
			}
		}
	}
	DebugDialog::debug(QString("%1 part labels corrected").arg(migratedParts.size()));
}

void MainWindow::swapSelectedMap(const QString & family, const QString & prop, QMap<QString, QString> & currPropsMap, ItemBase * itemBase)
{
	if (itemBase == nullptr) return;

	QString generatedModuleID = currPropsMap.value("moduleID");
	bool logoPadBlocker = false;

	if (generatedModuleID.isEmpty()) {
		if (prop.compare("layer") == 0) {
			if (family.compare("logo") == 0 || family.compare("pad") == 0 || family.contains("blocker", Qt::CaseInsensitive)) {
				QString value = Board::convertToXmlName(currPropsMap.value(prop));
				if (value.contains("copper1") && m_currentGraphicsView->boardLayers() == 1) {
					QMessageBox::warning(
					    this,
					    tr("No copper top layer"),
					    tr("The copper top (copper 1) layer is not available on a one-sided board.  Please switch the board to double-sided or choose the copper bottom (copper 0) layer.")
					);
					return;
				}
				// use the xml name
				currPropsMap.insert(prop, value);
				logoPadBlocker = true;
			}
			else if (itemBase->itemType() == ModelPart::Wire) {
				// assume this option is disabled for a one-sided board, so we would not get here?
				m_pcbGraphicsView->changeTraceLayer(itemBase, false, nullptr);
				return;
			}
		}
	}

	if (generatedModuleID.isEmpty()) {
		if (family.compare("Prototyping Board", Qt::CaseInsensitive) == 0) {
			if (prop.compare("size", Qt::CaseInsensitive) == 0 || prop.compare("type", Qt::CaseInsensitive) == 0) {
				QString size = currPropsMap.value("size");
				QString type = currPropsMap.value("type");
				if (type.compare("perfboard", Qt::CaseInsensitive) == 0) {
					generatedModuleID = Perfboard::genModuleID(currPropsMap);
				}
				else if (type.compare("stripboard", Qt::CaseInsensitive) == 0) {
					generatedModuleID = Stripboard::genModuleID(currPropsMap);
				}
			}
		}
	}

	bool swapLayer = false;
	ViewLayer::ViewLayerPlacement newViewLayerPlacement = ViewLayer::UnknownPlacement;
	if (prop.compare("layer") == 0 && !logoPadBlocker) {
		if (itemBase->modelPart()->flippedSMD() || itemBase->itemType() == ModelPart::Part) {
			ItemBase * viewItem = itemBase->modelPart()->viewItem(ViewLayer::PCBView);
			if (viewItem != nullptr) {
				ViewLayer::ViewLayerPlacement vlp = (currPropsMap.value(prop) == ItemBase::TranslatedPropertyNames.value("bottom") ? ViewLayer::NewBottom : ViewLayer::NewTop);
				if (viewItem->viewLayerPlacement() != newViewLayerPlacement) {
					swapLayer = true;
					newViewLayerPlacement = vlp;
				}
			}
		}
	}

	if (!generatedModuleID.isEmpty()) {
		ModelPart * modelPart = m_referenceModel->retrieveModelPart(generatedModuleID);
		if (modelPart == nullptr) {
			if (m_referenceModel->genFZP(generatedModuleID, m_referenceModel) == nullptr) {
				return;
			}
		}

		swapSelectedAux(itemBase->layerKinChief(), generatedModuleID, swapLayer, newViewLayerPlacement, currPropsMap);
		return;
	}

	if (swapLayer) {
		swapSelectedAux(itemBase->layerKinChief(), itemBase->moduleID(), true, newViewLayerPlacement, currPropsMap);
		return;
	}

	if ((prop.compare("package", Qt::CaseSensitive) != 0) && swapSpecial(prop, currPropsMap)) {
		return;
	}

	Q_FOREACH (QString key, currPropsMap.keys()) {
		QString value = currPropsMap.value(key);
		m_referenceModel->recordProperty(key, value);
	}

	QString moduleID = m_referenceModel->retrieveModuleIdWith(family, prop, true);
	bool exactMatch = m_referenceModel->lastWasExactMatch();

	if (moduleID.isEmpty()) {
		QMessageBox::information(
		    this,
		    tr("Sorry!"),
		    tr(
		        "No part with those characteristics.\n"
		        "We're working to avoid this message, and only let you choose between properties that do exist")
		);
		return;
	}



	itemBase = itemBase->layerKinChief();

	if(!exactMatch) {
		AutoCloseMessageBox::showMessage(this, tr("No exactly matching part found; Fritzing chose the closest match."));
	}

	swapSelectedAux(itemBase, moduleID, false, ViewLayer::UnknownPlacement, currPropsMap);
}

bool MainWindow::swapSpecial(const QString & theProp, QMap<QString, QString> & currPropsMap) {
	ItemBase * itemBase = m_infoView->currentItem();
	QString pinSpacing, resistance;
	int layers = 0;

	Q_FOREACH (QString key, currPropsMap.keys()) {
		if (key.compare("layers", Qt::CaseInsensitive) == 0) {
			if (!Board::isBoard(itemBase)) continue;

			QString value = currPropsMap.value(key, "");
			if (value.compare(Board::OneLayerTranslated) == 0) {
				layers = 1;
			}
			else if (value.compare(Board::TwoLayersTranslated) == 0) {
				layers = 2;
			}
		}

		if (key.compare("resistance", Qt::CaseInsensitive) == 0) {
			resistance = currPropsMap.value(key);
			continue;
		}
		if (key.compare("pin spacing", Qt::CaseInsensitive) == 0) {
			pinSpacing = currPropsMap.value(key);
			continue;
		}
	}

	if (layers != 0) {
		currPropsMap.insert("layers", QString::number(layers));
		if (theProp.compare("layers") == 0) {
			QString msg = (layers == 1) ? tr("Change to single layer pcb") : tr("Change to two layer pcb");
			swapLayers(itemBase, layers, msg);
			return true;
		}
	}

	if (!resistance.isEmpty() || !pinSpacing.isEmpty()) {
		if (theProp.contains("band", Qt::CaseInsensitive)) {
			// swap 4band for 5band or vice versa.
			return false;
		}

		auto * resistor = qobject_cast<Resistor *>(itemBase);
		if (resistor != nullptr) {
			m_currentGraphicsView->setResistance(resistance, pinSpacing);
			return true;
		}
	}

	return false;
}

void MainWindow::swapLayers(ItemBase * itemBase, int layers, const QString & msg) {
	auto* parentCommand = new QUndoCommand(msg);
	new CleanUpWiresCommand(m_breadboardGraphicsView, CleanUpWiresCommand::UndoOnly, parentCommand);
	new CleanUpRatsnestsCommand(m_breadboardGraphicsView, CleanUpWiresCommand::UndoOnly, parentCommand);
	m_pcbGraphicsView->swapLayers(itemBase, layers, parentCommand);
	m_undoStack->push(parentCommand);
}


void MainWindow::swapSelectedAux(ItemBase * itemBase, const QString & moduleID, bool useViewLayerPlacement, ViewLayer::ViewLayerPlacement overrideViewLayerPlacement,  QMap<QString, QString> & propsMap) {

	auto* parentCommand = new QUndoCommand(tr("Swapped %1 with module %2").arg(itemBase->instanceTitle()).arg(moduleID));
	new CleanUpWiresCommand(m_breadboardGraphicsView, CleanUpWiresCommand::UndoOnly, parentCommand);
	new CleanUpRatsnestsCommand(m_breadboardGraphicsView, CleanUpWiresCommand::UndoOnly, parentCommand);

	ViewLayer::ViewLayerPlacement viewLayerPlacement = itemBase->viewLayerPlacement();
	ModelPart * modelPart = m_referenceModel->retrieveModelPart(moduleID);
	if (m_pcbGraphicsView->boardLayers() == 2) {
		if (modelPart->flippedSMD()) {
			//viewLayerPlacement = m_pcbGraphicsView->dropOnBottom() ? ViewLayer::NewBottom : ViewLayer::NewTop;
			if (useViewLayerPlacement) viewLayerPlacement = overrideViewLayerPlacement;
		}
		else if (modelPart->itemType() == ModelPart::Part) {
			//viewLayerPlacement = m_pcbGraphicsView->dropOnBottom() ? ViewLayer::NewBottom : ViewLayer::NewTop;
			if (useViewLayerPlacement) viewLayerPlacement = overrideViewLayerPlacement;
		}
	}
	else {
		if (modelPart->flippedSMD()) {
			viewLayerPlacement = ViewLayer::NewBottom;
		}
		else if (modelPart->itemType() == ModelPart::Part) {
			//viewLayerPlacement = m_pcbGraphicsView->dropOnBottom() ? ViewLayer::NewBottom : ViewLayer::NewTop;
			if (useViewLayerPlacement) viewLayerPlacement = overrideViewLayerPlacement;
		}
	}

	swapSelectedAuxAux(itemBase, moduleID, viewLayerPlacement, propsMap, parentCommand);

	m_undoStack->push(parentCommand);
}

void MainWindow::swapBoardImageSlot(SketchWidget * sketchWidget, ItemBase * itemBase, const QString & filename, const QString & moduleID, bool addName) {

	auto* parentCommand = new QUndoCommand(tr("Change image to %2").arg(filename));
	QMap<QString, QString> propsMap;
	long newID = swapSelectedAuxAux(itemBase, moduleID, itemBase->viewLayerPlacement(), propsMap, parentCommand);

	auto * cmd = new LoadLogoImageCommand(sketchWidget, newID, "", QSizeF(0,0), filename, filename, addName, parentCommand);
	cmd->setRedoOnly();

	// need to defer execution so the content of the info view doesn't change during an event that started in the info view
	m_undoStack->waitPush(parentCommand, SketchWidget::PropChangeDelay);
}


void MainWindow::subSwapSlot(SketchWidget * sketchWidget, ItemBase * itemBase, const QString & newModuleID, ViewLayer::ViewLayerPlacement viewLayerPlacement, long & newID, QUndoCommand * parentCommand) {
	Q_UNUSED(sketchWidget);
	QMap<QString, QString> propsMap;
	newID = swapSelectedAuxAux(itemBase, newModuleID, viewLayerPlacement, propsMap, parentCommand);
}

long MainWindow::swapSelectedAuxAux(ItemBase * itemBase, const QString & moduleID,  ViewLayer::ViewLayerPlacement viewLayerPlacement, QMap<QString, QString> & propsMap, QUndoCommand * parentCommand)
{
	long modelIndex = ModelPart::nextIndex();

	QList<SketchWidget *> sketchWidgets;

	// master view must go last, since it creates the delete command, and possibly has all the local props
	switch (itemBase->viewID()) {
	case ViewLayer::SchematicView:
		sketchWidgets << m_pcbGraphicsView << m_breadboardGraphicsView << m_schematicGraphicsView;
		break;
	case ViewLayer::PCBView:
		sketchWidgets << m_schematicGraphicsView << m_breadboardGraphicsView << m_pcbGraphicsView;
		break;
	default:
		sketchWidgets << m_schematicGraphicsView << m_pcbGraphicsView << m_breadboardGraphicsView;
		break;
	}

	SwapThing swapThing;
	swapThing.firstTime = true;
	swapThing.itemBase = itemBase;
	swapThing.newModelIndex = modelIndex;
	swapThing.newModuleID = moduleID;
	swapThing.viewLayerPlacement = viewLayerPlacement;
	swapThing.parentCommand = parentCommand;
	swapThing.propsMap = propsMap;
	swapThing.bbView = m_breadboardGraphicsView;

	long newID = 0;
	for (int i = 0; i < 3; i++) {
		long tempID = sketchWidgets[i]->setUpSwap(swapThing, i == 2);
		if (newID == 0 && tempID != 0) newID = tempID;
	}

	// TODO:  z-order?

	// Otherwise focus will be on the zoom slider (next in tab order after the deleted package combo box) and ctrl-z won't work.
	this->setFocus();
	return newID;
}

void MainWindow::svgMissingLayer(const QString & layername, const QString & path) {
	QMessageBox::warning(
	    this,
	    tr("Fritzing"),
	    tr("Svg %1 is missing a '%2' layer. "
	       "For more information on how to create a custom board shape, "
	       "see the tutorial at <a href='http://fritzing.org/learning/tutorials/designing-pcb/pcb-custom-shape/'>http://fritzing.org/learning/tutorials/designing-pcb/pcb-custom-shape/</a>.")
	    .arg(path)
	    .arg(layername)
	);
}

void MainWindow::addDefaultParts() {
	if (m_pcbGraphicsView == nullptr) return;

	m_pcbGraphicsView->addDefaultParts();
	m_breadboardGraphicsView->addDefaultParts();
	m_schematicGraphicsView->addDefaultParts();
}

MainWindow * MainWindow::newMainWindow(ReferenceModel *referenceModel, const QString & displayPath, bool showProgress, bool lockFiles, int initialTab) {
	auto * mw = new MainWindow(referenceModel, nullptr);
	if (showProgress) {
		mw->showFileProgressDialog(displayPath);
	}

	if (initialTab >= 0) mw->setInitialTab(initialTab);
	mw->init(referenceModel, lockFiles);

	return mw;
}

void  MainWindow::clearFileProgressDialog() {
	if (m_fileProgressDialog != nullptr) {
		m_fileProgressDialog->close();
		delete m_fileProgressDialog;
		m_fileProgressDialog = nullptr;
	}
}

void MainWindow::setFileProgressPath(const QString & path)
{
	if (m_fileProgressDialog != nullptr) m_fileProgressDialog->setMessage(tr("loading %1").arg(path));
}

FileProgressDialog * MainWindow::fileProgressDialog()
{
	return m_fileProgressDialog;
}

void MainWindow::showFileProgressDialog(const QString & path) {
	m_fileProgressDialog = new FileProgressDialog(tr("Loading..."), 200, this);
	m_fileProgressDialog->setBinLoadingChunk(50);
	if (!path.isEmpty()) {
		setFileProgressPath(QFileInfo(path).fileName());
	}
	else {
		setFileProgressPath(tr("new sketch"));
	}
}

const QString &MainWindow::selectedModuleID() {
	if(m_currentGraphicsView != nullptr) {
		return m_currentGraphicsView->selectedModuleID();
	} else {
		return ___emptyString___;
	}
}

void MainWindow::redrawSketch() {
	if (m_currentGraphicsView == nullptr) return;
	QList<ConnectorItem *> visited;
	Q_FOREACH (QGraphicsItem * item, m_currentGraphicsView->scene()->items()) {
		item->update();
		auto * c = dynamic_cast<ConnectorItem *>(item);
		if (c != nullptr) {
			c->restoreColor(visited);
		}
	}
}

void MainWindow::statusMessage(QString message, int timeout) {
	QStatusBar * sb = realStatusBar();
	if (sb != nullptr) {
		sb->showMessage(message, timeout);
	}
}

void MainWindow::dropPaste(SketchWidget * sketchWidget) {
	Q_UNUSED(sketchWidget);
	pasteInPlace();
}

void MainWindow::updateLayerMenuSlot() {
	updateLayerMenu(true);
}

bool MainWindow::save() {
	bool result = FritzingWindow::save();
	if (result) {
		QSettings settings;
		settings.setValue("lastOpenSketch", m_fwFilename);
	}
	return result;
}

bool MainWindow::saveAs() {
	bool convertSchematic = false;
	if ((m_schematicGraphicsView != nullptr) && m_schematicGraphicsView->isOldSchematic()) {
		QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Schematic conversion"),
		                                    tr("Saving this sketch will convert it to the new schematic graphics standard. Go ahead and convert?"),
		                                    QMessageBox::Yes | QMessageBox::No);
		if (reply != QMessageBox::Yes) {
			return false;
		}
		convertSchematic = true;
	}

	bool result = false;
	result = FritzingWindow::saveAs();
	if (result) {
		QSettings settings;
		settings.setValue("lastOpenSketch", m_fwFilename);
		if (convertSchematic) {
			MainWindow * mw = revertAux();
			QString gridSize = QString("%1in").arg(m_schematicGraphicsView->defaultGridSizeInches());
			mw->m_schematicGraphicsView->setGridSize(gridSize);
			mw->m_schematicGraphicsView->resizeLabels();
			mw->m_schematicGraphicsView->resizeWires();
			mw->m_schematicGraphicsView->updateWires();
			mw->save();
		}
	}
	return result;
}

void MainWindow::changeBoardLayers(int layers, bool doEmit) {
	Q_UNUSED(doEmit);
	Q_UNUSED(layers);
	updateActiveLayerButtons();
	if (m_currentGraphicsView != nullptr) m_currentGraphicsView->updateConnectors();
}

void MainWindow::updateActiveLayerButtons() {
	if (m_activeLayerButtonWidget != nullptr) {
		int index = activeLayerIndex();
		bool enabled = index >= 0;

		m_activeLayerButtonWidget->setCurrentIndex(index);
		m_activeLayerButtonWidget->setVisible(enabled);

		m_activeLayerBothAct->setEnabled(enabled);
		m_activeLayerBottomAct->setEnabled(enabled);
		m_activeLayerTopAct->setEnabled(enabled);

		QList<QAction *> actions;
		actions << m_activeLayerBothAct << m_activeLayerBottomAct << m_activeLayerTopAct;
		setActionsIcons(index, actions);
	}

	if (m_viewFromButtonWidget != nullptr) {
		if (m_pcbGraphicsView != nullptr) {
			bool viewFromBelow = m_pcbGraphicsView->viewFromBelow();
			int index = (viewFromBelow ? 1 : 0);
			m_viewFromButtonWidget->setCurrentIndex(index);
			m_viewFromButtonWidget->setVisible(true);
			m_viewFromBelowToggleAct->setChecked(viewFromBelow);

			m_viewFromBelowAct->setChecked(viewFromBelow);
			m_viewFromAboveAct->setChecked(!viewFromBelow);

			m_viewFromBelowToggleAct->setEnabled(true);
			m_viewFromBelowAct->setEnabled(true);
			m_viewFromAboveAct->setEnabled(true);
		}
	}
}

int MainWindow::activeLayerIndex()
{
	if (m_currentGraphicsView == nullptr) return -1;

	if (m_currentGraphicsView->boardLayers() == 2 || activeLayerWidgetAlwaysOn()) {
		bool copper0Visible = m_currentGraphicsView->layerIsActive(ViewLayer::Copper0);
		bool copper1Visible = m_currentGraphicsView->layerIsActive(ViewLayer::Copper1);
		if (copper0Visible && copper1Visible) {
			return 0;
		}
		else if (copper1Visible) {
			return 2;
		}
		else if (copper0Visible) {
			return 1;
		}
	}

	return -1;
}

bool MainWindow::activeLayerWidgetAlwaysOn() {
	return false;
}

/**
 * A slot for saving a copy of the current sketch to a temp location.
 * This should be called every X minutes as well as just before certain
 * events, such as saves, part imports, file export/printing. This relies
 * on the m_autosaveNeeded variable and the undoStack being dirty for
 * an autosave to be attempted.
 */
void  MainWindow::backupSketch() {
	if (ProcessEventBlocker::isProcessing()) {
		// don't want to autosave during autorouting, for example
		return;
	}

	if (m_autosaveNeeded && !m_undoStack->isClean()) {
		m_autosaveNeeded = false;			// clear this now in case the save takes a really long time

		DebugDialog::debug(QString("%1 autosaved as %2").arg(m_fwFilename).arg(m_backupFileNameAndPath));
		statusBar()->showMessage(tr("Backing up '%1'").arg(m_fwFilename), 2000);
		ProcessEventBlocker::processEvents();
		m_backingUp = true;
		connectStartSave(true);
		m_sketchModel->save(m_backupFileNameAndPath, false);
		connectStartSave(false);
		m_backingUp = false;
	}
}

/**
 * This function is used to trigger an autosave at the next autosave
 * timer event. It is connected to the QUndoStack::indexChanged(int)
 * signal so that any change to the undo stack triggers autosaves.
 * This function can be called independent of this signal and
 * still work properly.
 */
void MainWindow::autosaveNeeded(int index) {
	Q_UNUSED(index);
	//DebugDialog::debug(QString("Triggering autosave"));
	m_autosaveNeeded = true;
}

/**
 * delete the backup file when the undostack is clean.
 */
void MainWindow::undoStackCleanChanged(bool isClean) {
	// DebugDialog::debug(QString("Clean status changed to %1").arg(isClean));
	if (isClean) {
		QFile::remove(m_backupFileNameAndPath);
	}
}

void MainWindow::setAutosavePeriod(int minutes) {
	setAutosave(minutes, AutosaveEnabled);
}

void MainWindow::setAutosaveEnabled(bool enabled) {
	setAutosave(AutosaveTimeoutMinutes, enabled);
}

void MainWindow::setAutosave(int minutes, bool enabled) {
	AutosaveTimeoutMinutes = minutes;
	AutosaveEnabled = enabled;
	Q_FOREACH (QWidget * widget, QApplication::topLevelWidgets()) {
		auto * mainWindow = qobject_cast<MainWindow *>(widget);
		if (mainWindow == nullptr) continue;

		mainWindow->m_autosaveTimer.stop();
		if (qobject_cast<PEMainWindow *>(widget) != nullptr) {
			continue;
		}

		if (AutosaveEnabled) {
			// is there a way to get the current timer offset so that all the timers aren't running in sync?
			// or just add some random time...
			mainWindow->m_autosaveTimer.start(AutosaveTimeoutMinutes * 60 * 1000);
		}
	}
}

bool MainWindow::hasLinkedProgramFiles(const QString & filename, QStringList & linkedProgramFiles)
{
	QFile file(filename);
	file.open(QFile::ReadOnly);
	QXmlStreamReader xml(&file);
	xml.setNamespaceProcessing(false);

	bool done = false;
	while (!xml.atEnd()) {
		switch (xml.readNext()) {
		case QXmlStreamReader::StartElement:
			if (xml.name().toString().compare("program") == 0) {
				linkedProgramFiles.append(xml.readElementText());
				break;
			}
			if (xml.name().toString().compare("views") == 0) {
				done = true;
				break;
			}
			if (xml.name().toString().compare("instances") == 0) {
				done = true;
				break;
			}
		default:
			break;
		}

		if (done) break;
	}

	return linkedProgramFiles.count() > 0;
}

QString MainWindow::getExtensionString() {
	return tr("Fritzing (*%1)").arg(FritzingBundleExtension) + ";;" + \
	       tr("Fritzing uncompressed (*%1)").arg(FritzingSketchExtension);
}

QStringList MainWindow::getExtensions() {
	QStringList extensions;
	extensions.append(FritzingBundleExtension);
	extensions.append(FritzingSketchExtension);
	return extensions;
}

void MainWindow::routingStatusLabelMousePress(QMouseEvent* event) {
	routingStatusLabelMouse(event, true);
}

void MainWindow::routingStatusLabelMouseRelease(QMouseEvent* event) {
	routingStatusLabelMouse(event, false);
}

void MainWindow::routingStatusLabelMouse(QMouseEvent*, bool show) {
	//if (show) DebugDialog::debug("-------");

	if (m_currentGraphicsView == nullptr) return;

	QSet<ConnectorItem *> toShow;
	Q_FOREACH (QGraphicsItem * item, m_currentGraphicsView->scene()->items()) {
		auto * vw = dynamic_cast<VirtualWire *>(item);
		if (vw == nullptr) continue;

		Q_FOREACH (ConnectorItem * connectorItem, vw->connector0()->connectedToItems()) {
			toShow.insert(connectorItem);
		}
		Q_FOREACH (ConnectorItem * connectorItem, vw->connector1()->connectedToItems()) {
			toShow.insert(connectorItem);
		}
	}
	QList<ConnectorItem *> visited;
	Q_FOREACH (ConnectorItem * connectorItem, toShow) {
		//if (show) {
		//	DebugDialog::debug(QString("unrouted %1 %2 %3 %4")
		//		.arg(connectorItem->attachedToInstanceTitle())
		//		.arg(connectorItem->attachedToID())
		//		.arg(connectorItem->attachedTo()->title())
		//		.arg(connectorItem->connectorSharedName()));
		//}

		if (connectorItem->isActive() && connectorItem->isVisible() && !connectorItem->hidden() && !connectorItem->layerHidden()) {
			connectorItem->showEqualPotential(show, visited);
		}
		else {
			connectorItem = connectorItem->getCrossLayerConnectorItem();
			if (connectorItem != nullptr) connectorItem->showEqualPotential(show, visited);
		}
	}

	if (!show && toShow.count() == 0) {
		QMessageBox::information(this, tr("Unrouted connections"),
		                         tr("There are no unrouted connections in this view."));
	}
}

void MainWindow::setReportMissingModules(bool b) {
	if (m_sketchModel != nullptr) {
		m_sketchModel->setReportMissingModules(b);
	}
}

void MainWindow::boardDeletedSlot()
{
	activeLayerBottom();
}

void MainWindow::cursorLocationSlot(double xinches, double yinches, double width, double height)
{
	if (m_locationLabel != nullptr) {
		QString units;
		double x, y, w, h;
		QHash<QString, int> precision;
		precision["mm"] = 1;
		precision["px"] = 0;
		precision["in"] = 3;

		m_locationLabel->setProperty("location", QSizeF(xinches, xinches));

		if (m_locationLabelUnits.compare("mm") == 0) {
			units = "mm";
			x = xinches * 25.4;
			y = yinches * 25.4;
			w = width * 25.4;
			h = height * 25.4;
		}
		else if (m_locationLabelUnits.compare("px") == 0) {
			units = "px";
			x = xinches * GraphicsUtils::SVGDPI;
			y = yinches * GraphicsUtils::SVGDPI;
			w = width * GraphicsUtils::SVGDPI;
			h = height * GraphicsUtils::SVGDPI;
		}
		else {
			units = "in";
			x = xinches;
			y = yinches;
			w = width;
			h = height;
		}

		if ( w*h == 0.0) {
			m_locationLabel->setText(tr("(x,y)=(%1, %2) %3")
			                         .arg(x, 0, 'f', precision[units])
			                         .arg(y, 0, 'f', precision[units])
			                         .arg(units) );
		} else {
			m_locationLabel->setText(tr("(x, y)=(%1, %2)\t(width, height)=(%3, %4) %5")
			                         .arg(x, 0, 'f', precision[units])
			                         .arg(y, 0, 'f', precision[units])
			                         .arg(w, 0, 'f', precision[units])
			                         .arg(h, 0, 'f', precision[units])
			                         .arg(units) );
		}
	}
}

void MainWindow::locationLabelClicked()
{
	if (m_locationLabelUnits.compare("mm") == 0) {
		m_locationLabelUnits = "px";
	}
	else if (m_locationLabelUnits.compare("px") == 0) {
		m_locationLabelUnits = "in";
	}
	else if (m_locationLabelUnits.compare("in") == 0) {
		m_locationLabelUnits = "mm";
	}
	else {
		m_locationLabelUnits = "in";
	}

	if (m_locationLabel != nullptr) {
		QVariant variant =  m_locationLabel->property("location");
		if (variant.isValid()) {
			QSizeF size = variant.toSizeF();
			cursorLocationSlot(size.width(), size.height());
		}
		else {
			cursorLocationSlot(0.0, 0.0);
		}
	}

	QSettings settings;
	settings.setValue("LocationInches", QVariant(m_locationLabelUnits));
}

void MainWindow::filenameIfSlot(QString & filename)
{
	filename = QFileInfo(fileName()).fileName();
}

QList<SketchWidget *> MainWindow::sketchWidgets()
{
	QList<SketchWidget *> list;
	list << m_breadboardGraphicsView << m_schematicGraphicsView << m_pcbGraphicsView;
	return list;
}

void MainWindow::setCloseSilently(bool cs)
{
	m_closeSilently = cs;
}

PCBSketchWidget * MainWindow::pcbView() {
	return m_pcbGraphicsView;
}

void MainWindow::noBackup()
{
	m_autosaveTimer.stop();
}

void MainWindow::hideTempPartsBin() {
	if (m_binManager != nullptr) m_binManager->hideTempPartsBin();
}

void MainWindow::setActiveWire(Wire * wire) {
	m_activeWire = wire;
}

void MainWindow::setActiveConnectorItem(ConnectorItem * connectorItem) {
	m_activeConnectorItem = connectorItem;
}

const QString & MainWindow::fritzingVersion() {
	if (m_sketchModel != nullptr) return m_sketchModel->fritzingVersion();

	return ___emptyString___;
}


void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	const QMimeData* mimeData = event->mimeData();

	if (mimeData->hasUrls()) {
		QStringList pathList;
		QList<QUrl> urlList = mimeData->urls();

		// extract the local paths of the files
		for (int i = 0; i < urlList.size() && i < 32; ++i) {
			QString fn = urlList.at(i).toLocalFile();
			Q_FOREACH (QString ext, fritzingExtensions()) {
				if (fn.endsWith(ext)) {
					event->acceptProposedAction();
					return;
				}
			}

			if (fn.endsWith("txt")) {
				QFile file(fn);
				if (file.open(QFile::ReadOnly)) {
					QTextStream stream(&file);
					while (!stream.atEnd()) {
						QString line = stream.readLine().trimmed();
						Q_FOREACH (QString ext, fritzingExtensions()) {
							if (line.endsWith(ext)) {
								event->acceptProposedAction();
								return;
							}
						}
					}
				}
			}
		}
	}

}

void MainWindow::dropEvent(QDropEvent *event)
{
	const QMimeData* mimeData = event->mimeData();

	if (mimeData->hasUrls()) {
		QStringList pathList;
		QList<QUrl> urlList = mimeData->urls();

		// extract the local paths of the files
		for (int i = 0; i < urlList.size() && i < 32; ++i) {
			mainLoadAux(urlList.at(i).toLocalFile());
		}

		return;
	}
}

bool MainWindow::hasAnyAlien() {
	return m_addedToTemp;
}

void MainWindow::initStyleSheet()
{
	QString suffix = getStyleSheetSuffix();
	QFile styleSheet(QString(":/resources/styles/%1.qss").arg(suffix));
	if (!styleSheet.open(QIODevice::ReadOnly)) {
		DebugDialog::debug(QString("Unable to open :/resources/styles/%1.qss").arg(suffix));
	} else {
		QString platformDependantStyle = "";
		QString platformDependantStylePath;
#ifdef Q_OS_LINUX
		if(style()->metaObject()->className()==QString("OxygenStyle")) {
			QFile oxygenStyleSheet(QString(":/resources/styles/linux-kde-oxygen-%1.qss").arg(suffix));
			if(oxygenStyleSheet.open(QIODevice::ReadOnly)) {
				platformDependantStyle += oxygenStyleSheet.readAll();
			}
		}
		platformDependantStylePath = QString(":/resources/styles/linux-%1.qss").arg(suffix);
#endif

#ifdef Q_OS_MAC
		platformDependantStylePath = QString(":/resources/styles/mac-%1.qss").arg(suffix);
#endif

#ifdef Q_OS_WIN
		platformDependantStylePath = QString(":/resources/styles/win-%1.qss").arg(suffix);
#endif

		QFile platformDependantStyleSheet(platformDependantStylePath);
		if(platformDependantStyleSheet.open(QIODevice::ReadOnly)) {
			platformDependantStyle += platformDependantStyleSheet.readAll();
		}
		setStyleSheet(styleSheet.readAll()+platformDependantStyle);
	}
}

QString MainWindow::getStyleSheetSuffix() {
	return "fritzing";
}

void MainWindow::addToMyParts(ModelPart * modelPart, const QStringList & peAlienFiles)
{
	Q_FOREACH(QString pathToAddFromPe, peAlienFiles) {
		// DebugDialog::debug(QString("addToMyParts adding  %1")
		//.arg(pathToAddFromPe));
		m_alienFiles << pathToAddFromPe;
	}
	m_binManager->addToMyParts(modelPart);
}

bool MainWindow::anyUsePart(const QString & moduleID) {
	Q_FOREACH (QWidget *widget, QApplication::topLevelWidgets()) {
		auto *mainWindow = qobject_cast<MainWindow *>(widget);
		if (mainWindow == nullptr) continue;

		if (mainWindow->usesPart(moduleID)) {
			return true;
		}
	}

	return false;
}

bool MainWindow::usesPart(const QString & moduleID) {
	if (m_currentGraphicsView == nullptr) return false;

	Q_FOREACH (QGraphicsItem * item, m_currentGraphicsView->scene()->items()) {
		auto * itemBase = dynamic_cast<ItemBase *>(item);
		if ((itemBase != nullptr) && itemBase->moduleID().compare(moduleID) == 0) {
			return true;
		}
	}

	return false;
}

bool MainWindow::updateParts(const QString & moduleID, QUndoCommand * parentCommand) {
	if (m_currentGraphicsView == nullptr) return false;

	QSet<ItemBase *> itemBases;
	Q_FOREACH (QGraphicsItem * item, m_currentGraphicsView->scene()->items()) {
		auto * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == nullptr) continue;
		if (itemBase->moduleID().compare(moduleID) != 0) continue;

		itemBases.insert(itemBase->layerKinChief());
	}

	QMap<QString, QString> propsMap;
	Q_FOREACH (ItemBase * itemBase, itemBases) {
		swapSelectedAuxAux(itemBase, moduleID, itemBase->viewLayerPlacement(), propsMap, parentCommand);
	}

	return true;
}

void MainWindow::showStatusMessage(const QString & message)
{
	if (sender() == m_statusBar) {
		return;
	}

	if (message == m_statusBar->currentMessage()) {
		return;
	}

	//DebugDialog::debug("show message " + message);
	m_statusBar->blockSignals(true);
	m_statusBar->showMessage(message);
	m_statusBar->blockSignals(false);
}

void MainWindow::updatePartsBin(const QString & moduleID) {
	m_binManager->reloadPart(moduleID);
}

bool MainWindow::hasCustomBoardShape() {
	if (m_pcbGraphicsView == nullptr) return false;

	return m_pcbGraphicsView->hasCustomBoardShape();
}

void MainWindow::selectPartsWithModuleID(ModelPart * modelPart) {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->selectItemsWithModuleID(modelPart);
}

void MainWindow::addToSketch(QList<ModelPart *> & modelParts) {
	if (m_currentGraphicsView == nullptr) return;

	m_currentGraphicsView->addToSketch(modelParts);
}

void MainWindow::initProgrammingWidget() {
	m_programView = new ProgramWindow(this);

	connect(m_programView, SIGNAL(linkToProgramFile(const QString &, Platform *, bool, bool)),
	        this, SLOT(linkToProgramFile(const QString &, Platform *, bool, bool)));

	m_programView->setup();

	auto * sketchAreaWidget = new SketchAreaWidget(m_programView, this, false, true);
	addTab(sketchAreaWidget, ":/resources/images/icons/TabWidgetCodeActive_icon.png", tr("Code"), true);
}

ProgramWindow *MainWindow::programmingWidget() {
	return m_programView;
}

void MainWindow::orderFabHoverEnter() {
	if ((m_rolloverQuoteDialog != nullptr) && m_rolloverQuoteDialog->isVisible()) return;
	QuoteDialog::setQuoteSucceeded(false);
	QObject::connect(m_pcbGraphicsView, &PCBSketchWidget::fabQuoteFinishedSignal, this, &MainWindow::fireQuote);
	m_pcbGraphicsView->requestQuote();
}

void MainWindow::fireQuote() {
	if (!QuoteDialog::quoteSucceeded()) return;

	m_rolloverQuoteDialog = m_pcbGraphicsView->quoteDialog(m_pcbWidget);
	if (m_rolloverQuoteDialog == nullptr) return;

	//DebugDialog::debug("enter fab button");
	//QWidget * toolbar = m_pcbWidget->toolbar();
	//QRect r = toolbar->geometry();
	//QPointF p = toolbar->parentWidget()->mapToGlobal(r.topLeft());

	Qt::WindowFlags flags = m_rolloverQuoteDialog->windowFlags();
	flags = Qt::Window | Qt::Dialog | Qt::FramelessWindowHint;
	m_rolloverQuoteDialog->setWindowFlags(flags);
	m_rolloverQuoteDialog->show();

	// seems to require setting position after show()

	QRect b = m_orderFabButton->geometry();
	QPoint bt = m_orderFabButton->parentWidget()->mapToGlobal(b.topRight());
	QRect t = m_pcbWidget->toolbar()->geometry();
	QPoint gt = m_pcbWidget->toolbar()->parentWidget()->mapToGlobal(t.topLeft());
	QRect q = m_rolloverQuoteDialog->geometry();

	// I don't understand why--perhaps due to the windowFlags--but q is already in global coordinates
	q.moveBottom(gt.y() - 20);
	q.moveRight(bt.x());
	m_rolloverQuoteDialog->setGeometry(q);
}

void MainWindow::orderFabHoverLeave() {
	QObject::disconnect(m_pcbGraphicsView, &PCBSketchWidget::fabQuoteFinishedSignal, this, &MainWindow::fireQuote);
	if (m_rolloverQuoteDialog != nullptr) {
		m_rolloverQuoteDialog->hide();
	}
}

void MainWindow::initWelcomeView() {
	m_welcomeView = new WelcomeView(this);
	m_welcomeView->setObjectName("WelcomeView");
	auto * sketchAreaWidget = new SketchAreaWidget(m_welcomeView, this, false, false);
	addTab(sketchAreaWidget, ":/resources/images/icons/TabWidgetWelcomeActive_icon.png", tr("Welcome"), true);
}

void MainWindow::setInitialView() {
	// do this the first time, since the current_changed signal wasn't sent
	int tab = 0;

	tabWidget_currentChanged(tab+1);
	tabWidget_currentChanged(tab);

	// default to breadboard view
	this->setCurrentTabIndex(m_initialTab);
}

void MainWindow::updateWelcomeViewRecentList(bool doEmit) {
	if (m_welcomeView != nullptr) {
		m_welcomeView->updateRecent();
		if (doEmit) {
			Q_FOREACH (QWidget *widget, QApplication::topLevelWidgets()) {
				auto *mainWin = qobject_cast<MainWindow *>(widget);
				if ((mainWin != nullptr) && mainWin != this) {
					mainWin->updateWelcomeViewRecentList(false);
				}
			}
		}
	}
}

void MainWindow::initZoom() {
	if (m_currentGraphicsView == nullptr) return;
	if (m_currentGraphicsView->everZoomed()) return;
	if (!m_currentGraphicsView->isVisible()) return;

	bool parts = false;
	Q_FOREACH (QGraphicsItem * item, m_currentGraphicsView->items()) {
		auto * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == nullptr) continue;
		if (!itemBase->isEverVisible()) continue;

		parts = true;
		break;
	}

	if (parts) {
		m_currentGraphicsView->fitInWindow();
	}

	m_currentGraphicsView->setEverZoomed(true);
}

void MainWindow::noSchematicConversion() {
	m_noSchematicConversion = true;
}

void MainWindow::setInitialTab(int tab) {
	m_initialTab = tab;
}

void MainWindow::triggerSimulator() {
	m_simulator->triggerSimulation();
}

QSharedPointer<ProjectProperties> MainWindow::getProjectProperties() {
	return m_projectProperties;
}

bool MainWindow::isSimulatorEnabled() {
	return m_simulator->isEnabled();
}

void MainWindow::enableSimulator(bool enable) {
	if (m_simulator) {
		m_simulator->enable(enable);
	}
}

void MainWindow::putItemByModuleID(const QString & moduleID) {
	if (m_currentGraphicsView != nullptr) {
		m_currentGraphicsView->putItemByModuleID(moduleID);
		m_currentGraphicsView->setFocus();
	}
}

void MainWindow::postKeyEvent(const QString & serializedKeys) {
	QJsonDocument doc = QJsonDocument::fromJson(serializedKeys.toUtf8());
	QJsonArray events = doc.array();

	for (const QJsonValue &event_val : events) {
		QJsonObject event_obj = event_val.toObject();
		QString key = event_obj["key"].toString();
		QJsonArray modifiers = event_obj["modifiers"].toArray();

		int keyCode = key.at(0).unicode(); // Convert the key string to a Qt key code
		Qt::KeyboardModifiers modFlags;

		// Check for modifier keys
		for (const QJsonValue &mod : modifiers) {
			if (mod.toString() == "shift") {
				modFlags |= Qt::ShiftModifier;
			} else if (mod.toString() == "ctrl") {
				modFlags |= Qt::ControlModifier;
			} else if (mod.toString() == "alt") {
				modFlags |= Qt::AltModifier;
			}
		}

		QApplication::postEvent(QApplication::focusWidget(), new QKeyEvent(QEvent::KeyPress, keyCode, modFlags, key.at(0)));
		QApplication::postEvent(QApplication::focusWidget(), new QKeyEvent(QEvent::KeyRelease, keyCode, modFlags, key.at(0)));
	}
}
