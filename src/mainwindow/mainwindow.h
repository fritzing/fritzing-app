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

$Revision: 6999 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-28 14:14:07 +0200 (So, 28. Apr 2013) $

********************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QUndoView>
#include <QUndoGroup>
#include <QRadioButton>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QStackedWidget>
#include <QSizeGrip>
#include <QPointer>
#include <QProcess>
#include <QDockWidget>
#include <QXmlStreamWriter>
#include <QRegExp>
#include <QProxyStyle>
#include <QStyle>
#include <QStylePainter>
#include <QPrinter>

#include "fritzingwindow.h"
#include "sketchareawidget.h"
#include "../viewlayer.h"
#include "../program/programwindow.h"
#include "../svg/svg2gerber.h"
#include "../routingstatus.h"

QT_BEGIN_NAMESPACE
class QAction;
class QListWidget;
class QMenu;
QT_END_NAMESPACE

class FSizeGrip;

typedef class FDockWidget * (*DockFactory)(const QString & title, QWidget * parent);

bool sortPartList(class ItemBase * b1, class ItemBase * b2);

static const QString ORDERFABENABLED = "OrderFabEnabled";

class FTabWidget : public QTabWidget {
    Q_OBJECT
public:
    FTabWidget(QWidget * parent = NULL);

    //int addTab(QWidget * page, const QIcon & icon, const QIcon & hoverIcon, const QIcon & inactiveIcon, const QString & label);

protected slots:
    //void tabIndexChanged(int index);

protected:
    //QList<QIcon> m_inactiveIcons;
    //QList<QIcon> m_hoverIcons;
    //QList<QIcon> m_icons;
};

class FTabBar : public QTabBar {
    Q_OBJECT
public:
    FTabBar();

    void paintEvent(QPaintEvent *);

protected:
    bool m_firstTime;
};

class SwapTimer : public QTimer
{
Q_OBJECT

public:
	SwapTimer();

	void setAll(const QString & family, const QString & prop, QMap<QString, QString> &  propsMap, ItemBase *);
	const QString & family();
	const QString & prop();
	QMap<QString, QString> propsMap();
	ItemBase * itemBase();

protected:
	QString m_family;
	QString m_prop;
	QMap<QString, QString> m_propsMap;
	QPointer <ItemBase> m_itemBase;
};

struct GridSizeThing
{
	QLineEdit * lineEdit;
	QDoubleValidator * validator;
	QRadioButton * mmButton;
	QRadioButton * inButton;
	double defaultGridSize;
    QString gridSizeText;
	QString viewName;
	QString shortName;
    
    GridSizeThing(const QString & viewName, const QString & shortName, double defaultSize, const QString & gridSizeText);
};

class GridSizeDialog : public QDialog {
    Q_OBJECT

public:
    GridSizeDialog(GridSizeThing *);
    GridSizeThing * gridSizeThing();

protected:
    GridSizeThing * m_gridSizeThing;
};


struct TraceMenuThing {
    int boardCount;
    int boardSelectedCount;
	bool jiEnabled;
	bool exEnabled;
	bool viaEnabled;
	bool exChecked;
	bool gfrEnabled;
    bool gfsEnabled;

    TraceMenuThing() {
        boardCount = 0;
        boardSelectedCount = 0;
        gfsEnabled = false;
        jiEnabled = false;
	    exEnabled = false;
	    exChecked = true;
	    viaEnabled = false;
	    gfrEnabled = false;
    }
};

class MainWindow : public FritzingWindow
{
    Q_OBJECT
    Q_PROPERTY(int fireQuoteDelay READ fireQuoteDelay WRITE setFireQuoteDelay DESIGNABLE true)

public:
    MainWindow(class ReferenceModel *referenceModel, QWidget * parent);
    MainWindow(QFile & fileToLoad);
	~MainWindow();

    void mainLoad(const QString & fileName, const QString & displayName, bool checkObsolete);
	bool loadWhich(const QString & fileName, bool setAsLastOpened, bool addToRecent, bool checkObsolete, const QString & displayName);
	void notClosableForAWhile();
	QAction *raiseWindowAction();
	QSizeGrip *sizeGrip();
	QStatusBar *realStatusBar();
    void enableCheckUpdates(bool enabled);

	void getPartsEditorNewAnd(ItemBase * fromItem);
	void addDefaultParts();
    void init(ReferenceModel *referenceModel, bool lockFiles);
	void showFileProgressDialog(const QString & path);
	void setFileProgressPath(const QString & path);
	void clearFileProgressDialog();
	class FileProgressDialog * fileProgressDialog();

	const QString &selectedModuleID();

	void saveDocks();
	void restoreDocks();

	void redrawSketch();

	// if we consider a part as the smallest ("atomic") entity inside
	// fritzing, then this functions may help with the bundle tasks
	// on the complex entities: sketches, bins, modules (?)
	void saveBundledNonAtomicEntity(QString &filename, const QString &extension, Bundler *bundler, const QList<ModelPart*> &partsToSave, bool askForFilename, const QString & destFolderPath, bool saveModel, bool deleteLeftovers);
	bool loadBundledNonAtomicEntity(const QString &filename, Bundler *bundler, bool addToBin, bool dontAsk);
	void saveAsShareable(const QString & path, bool saveModel);

	
	void setCurrentFile(const QString &fileName, bool addToRecent, bool setAsLastOpened);
	void setReportMissingModules(bool);
	QList<class SketchWidget *> sketchWidgets();
    ProgramWindow * programmingWidget();
	void setCloseSilently(bool);
	class PCBSketchWidget * pcbView();
	void noBackup();
	void swapSelectedAux(ItemBase * itemBase, const QString & moduleID, bool useViewLayerPlacement, ViewLayer::ViewLayerPlacement, QMap<QString, QString> & propsMap);
    void swapLayers(ItemBase * itemBase, int layers, const QString & msg, int delay);
	bool saveAsAux(const QString & fileName);
	void swapObsolete(bool displayFeedback, QList<ItemBase *> &);
	QList<ItemBase *> selectAllObsolete(bool displayFeedback);
	void hideTempPartsBin();
	const QString & fritzingVersion();
	void removeGroundFill(ViewLayer::ViewLayerID, QUndoCommand * parentCommand);
	void groundFill(ViewLayer::ViewLayerID);
	void copperFill(ViewLayer::ViewLayerID);
    bool hasAnyAlien();
    void exportSvg(double res, bool selectedItems, bool flatten, const QString & filename);
    void setCurrentView(ViewLayer::ViewID);
    bool usesPart(const QString & moduleID);
    bool anyUsePart(const QString & moduleID);
    bool updateParts(const QString & moduleID, QUndoCommand * parentCommand);
	void updatePartsBin(const QString & moduleID);
	bool hasCustomBoardShape();
    void selectPartsWithModuleID(ModelPart *);
    void addToSketch(QList<ModelPart *> &);
	QStringList newDesignRulesCheck(bool showOkMessage);
    int fireQuoteDelay();
    void setFireQuoteDelay(int);
    void setInitialTab(int);
    void noSchematicConversion();

public:
	static void initNames();
	static MainWindow * newMainWindow(ReferenceModel *referenceModel, const QString & displayPath, bool showProgress, bool lockFiles, int initialTab);
	static void setAutosavePeriod(int);
	static void setAutosaveEnabled(bool);

signals:
	void alienPartsDismissed();
	void mainWindowMoved(QWidget *);
	void changeActivationSignal(bool activate, QWidget * originator);
	void externalProcessSignal(QString & name, QString & path, QStringList & args);

public slots:
	void ensureClosable();
	ModelPart* loadBundledPart(const QString &fileName, bool addToBin);
	void acceptAlienFiles();
	void statusMessage(QString message, int timeout);
    void showPCBView();
	void groundFill();
	void removeGroundFill();
	void copperFill();
	void setOneGroundFillSeed();
	void setGroundFillSeeds();	
	void clearGroundFillSeeds();
	void changeBoardLayers(int layers, bool doEmit);
	void selectAllObsolete();
	void swapObsolete();
    void swapBoardImageSlot(SketchWidget * sketchWidget, ItemBase * itemBase, const QString & filename, const QString & moduleID, bool addName);
	void updateTraceMenu();
    virtual void updateExportMenu();
	virtual void updateFileMenu();
    void showStatusMessage(const QString &);
    void orderFabHoverEnter();
    void orderFabHoverLeave();
    void setGroundFillKeepout();
    void oldSchematicsSlot(const QString & filename, bool & useOldSchematics);
    void showWelcomeView();

protected slots:
	void mainLoad();
	void revert();
	void openRecentOrExampleFile();
	void openRecentOrExampleFile(const QString & filename, const QString & actionText);
    void print();
    void doExport();
	void exportEtchable();
    void about();
	void tipsAndTricks();
	void firstTimeHelp();
    void copy();
    void cut();
    void paste();
	void pasteInPlace();
    void duplicate();
    void doDelete();
    void doDeleteMinus();
    void selectAll();
    void deselect();
    void zoomIn();
    void zoomOut();
    void fitInWindow();
    void actualSize();
	void hundredPercentSize();
    void alignToGrid();
    void showGrid();
    void setGridSize();
    void setBackgroundColor();
    void colorWiresByLength();
    void showBreadboardView();
    void showSchematicView();
    void showProgramView();
	void showPartsBinIconView();
	void showPartsBinListView();
    virtual void updateEditMenu();
    virtual void updateLayerMenu(bool resetLayout = false);
    void updatePartMenu();
    virtual void updateWireMenu();
    void updateTransformationActions();
	void updateRecentFileActions();
    virtual void tabWidget_currentChanged(int index);
    void createNewSketch();
    void minimize();
    void toggleToolbar(bool toggle);
    void togglePartLibrary(bool toggle);
    void toggleInfo(bool toggle);
    void toggleUndoHistory(bool toggle);
	void toggleDebuggerOutput(bool toggle);
	void openHelp();
	void openDonate();
	void openExamples();
	void openPartsReference();
	void visitFritzingDotOrg();
	void partsEditorHelp();
	virtual void updateWindowMenu();
	void pageSetup();
	void sendToBack();
	void sendBackward();
	void bringForward();
	void bringToFront();
    void alignLeft();
    void alignRight();
    void alignVerticalCenter();
    void alignTop();
    void alignHorizontalCenter();
    void alignBottom();
	void rotate90cw();
	void rotate90ccw();
	void rotate180();
	void rotate45ccw();
	void rotate45cw();
	void rotateIncCCW();
	void rotateIncCW();
	void rotateIncCCWRubberBand();
	void rotateIncCWRubberBand();
	void flipHorizontal();
	void flipVertical();
	void showAllLayers();
	void hideAllLayers();
	void addBendpoint();
	void convertToVia();
	void convertToBendpoint();
	void flattenCurve();
	void disconnectAll();

	void openInPartsEditorNew();
	void openNewPartsEditor(class PaletteItem *);

	void updateZoomSlider(double zoom);
	void updateZoomOptionsNoMatterWhat(double zoom);
	void updateViewZoom(double newZoom);

	void setInfoViewOnHover(bool infoViewOnHover);
	void updateItemMenu();

	void newAutoroute();
	void orderFab();
	void activeLayerTop();
	void activeLayerBottom();
	void activeLayerBoth();
	void toggleActiveLayer();
	void createTrace();
	void excludeFromAutoroute();
	void selectAllTraces();
	void showUnrouted();
    void selectAllCopperFill();
	void updateRoutingStatus();
	void selectAllExcludedTraces();
	void selectAllIncludedTraces();
	void selectAllJumperItems();
	void selectAllVias();

	void shareOnline();
	void saveBundledPart(const QString &moduleId=___emptyString___);
	QStringList saveBundledAux(ModelPart *mp, const QDir &destFolder);

	void binSaved(bool hasAlienParts);
	void routingStatusSlot(class SketchWidget *, const RoutingStatus &);

	void applyReadOnlyChange(bool isReadOnly);

	void raiseAndActivate();
	void activateWindowAux();
	void showPartLabels();
	void addNote();
	void reportBug();
	void enableDebug();
	void tidyWires();
	void changeWireColor(bool checked);

	void startSaveInstancesSlot(const QString & fileName, ModelPart *, QXmlStreamWriter &);
	void loadedViewsSlot(class ModelBase *, QDomElement & views);
	void loadedRootSlot(const QString & filename, ModelBase *, QDomElement & views);
    void obsoleteSMDOrientationSlot();
	void exportNormalizedSVG();
	void exportNormalizedFlattenedSVG();
    void dumpAllParts();
    void testConnectors();

	void launchExternalProcess();
	bool externalProcess(QString & name, QString & path, QStringList & args);
	void processError(QProcess::ProcessError processError);
	void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void processReadyRead();
	void processStateChanged(QProcess::ProcessState newState);
    void throwFakeException();

	void dropPaste(SketchWidget *);

	void openProgramWindow();
    void linkToProgramFile(const QString & filename, Platform * platform, bool addLink, bool strong);
	QStringList newDesignRulesCheck();
	void subSwapSlot(SketchWidget *, ItemBase *, const QString & newModuleID, ViewLayer::ViewLayerPlacement, long & newID, QUndoCommand * parentCommand);
	void updateLayerMenuSlot();
	bool save();
	bool saveAs();
	virtual void backupSketch();
	void undoStackCleanChanged(bool isClean);
	void autosaveNeeded(int index = 0);
	void changeTraceLayer();
	void routingStatusLabelMousePress(QMouseEvent*);
	void routingStatusLabelMouseRelease(QMouseEvent*);
	void selectMoveLock();
	void moveLock();
	void setSticky();
	void autorouterSettings();
	void boardDeletedSlot();
	void cursorLocationSlot(double, double, double=0.0, double=0.0);
	void locationLabelClicked();
	void swapSelectedMap(const QString & family, const QString & prop, QMap<QString, QString> & currPropsMap, ItemBase *);
	void swapSelectedDelay(const QString & family, const QString & prop, QMap<QString, QString> & currPropsMap, ItemBase *);
	void swapSelectedTimeout();
	void filenameIfSlot(QString & filename);
	void openURL();
	void setActiveWire(class Wire *);
	void setActiveConnectorItem(class ConnectorItem *);
	void gridUnits(bool);
	void restoreDefaultGrid();
    void checkLoadedTraces();

    void keepMargins();
	void dockChangeActivation(bool activate, QWidget * originator);
    void addToMyParts(ModelPart *);
    void hidePartSilkscreen();
    void fabQuote();
    void findPartInSketch();
    void fireQuote();
    void setViewFromBelowToggle();
    void setViewFromBelow();
    void setViewFromAbove();
    void updateWelcomeViewRecentList(bool doEmit = true);
    virtual void initZoom();

protected:
	void initSketchWidget(SketchWidget *);
    virtual void initProgrammingWidget();

    virtual void createActions();
    virtual void createFileMenuActions();
    void createExportActions();
    void createRaiseWindowActions();
    void createOrderFabAct();
    void createOpenExampleMenu();
	void createActiveLayerActions();
    void populateMenuFromXMLFile(QMenu *parentMenu, QStringList &actionsTracker, const QString &folderPath, const QString &indexFileName);
    QHash<QString, struct SketchDescriptor *> indexAvailableElements(QDomElement &domElem, const QString &srcPrefix, QStringList & actionsTracker, const QString & localeName);
    void populateMenuWithIndex(const QHash<QString, struct SketchDescriptor *> &, QMenu * parentMenu, QDomElement &domElem, const QString & localeName);
    void populateMenuFromFolderContent(QMenu *parentMenu, const QString &path);
    void createOpenRecentMenu();
    void createEditMenuActions();
    void createPartMenuActions();
    virtual void createViewMenuActions(bool showWelcome);
    void createWindowMenuActions();
    void createHelpMenuActions();
    virtual void createMenus();
    void createStatusBar();
	virtual void connectPairs();
	void connectPair(SketchWidget * signaller, SketchWidget * slotter);
	void closeEvent(QCloseEvent * event);
	void saveAsAuxAux(const QString & fileName);
	void printAux(QPrinter &printer, bool removeBackground, bool paginate);
	void exportAux(QString fileName, QImage::Format format, int quality, bool removeBackground);
	void notYetImplemented(QString action);
	bool eventFilter(QObject *obj, QEvent *event);
	void setActionsIcons(int index, QList<QAction *> &);
	void exportToEagle();
	void exportToGerber();
    void exportBOM();
    void exportNetlist();
    void exportSpiceNetlist();
    void exportSvg(double res, bool selectedItems, bool flatten);
	void exportSvgWatermark(QString & svg, double res);
	void exportEtchable(bool wantPDF, bool wantSVG);

	virtual QList<QWidget*> getButtonsForView(ViewLayer::ViewID viewId);
	const QString untitledFileName();
	int &untitledFileCount();
	const QString fileExtension();
	QString getExtensionString();
	QStringList getExtensions();
	const QString defaultSaveFolder();
	bool undoStackIsEmpty();

	void createTraceMenuActions();
	void hideShowTraceMenu();
    void hideShowProgramMenu();
    void updatePCBTraceMenu(QGraphicsItem *, TraceMenuThing &);

	QList<ModelPart*> moveToPartsFolder(QDir &unzipDir, MainWindow* mw, bool addToBin, bool addToAlien, const QString & prefixFolder, const QString &destFolder, bool importingSinglePart);
	QString copyToSvgFolder(const QFileInfo& file, bool addToAlien, const QString & prefixFolder, const QString &destFolder);
	ModelPart* copyToPartsFolder(const QFileInfo& file, bool addToAlien, const QString & prefixFolder, const QString &destFolder);

	void closeIfEmptySketch(MainWindow* mw);
	bool whatToDoWithAlienFiles();
	void backupExistingFileIfExists(const QString &destFilePath);
	void recoverBackupedFiles();
	void resetTempFolder();
    void saveLastTabList();

	virtual QMenu *breadboardItemMenu();
	virtual QMenu *schematicItemMenu();
	virtual QMenu *pcbItemMenu();
	virtual QMenu *pcbWireMenu();
	virtual QMenu *schematicWireMenu();
	virtual QMenu *breadboardWireMenu();

	QMenu *viewItemMenuAux(QMenu* menu);

	void createZoomOptions(SketchAreaWidget* parent);
	class SketchToolButton *createRotateButton(SketchAreaWidget *parent);
	SketchToolButton *createShareButton(SketchAreaWidget *parent);
	SketchToolButton *createFlipButton(SketchAreaWidget *parent);
	SketchToolButton *createAutorouteButton(SketchAreaWidget *parent);
    SketchToolButton *createOrderFabButton(SketchAreaWidget *parent);
	QWidget *createActiveLayerButton(SketchAreaWidget *parent);
	QWidget *createViewFromButton(SketchAreaWidget *parent);
	class ExpandingLabel * createRoutingStatusLabel(SketchAreaWidget *);
	SketchToolButton *createExportEtchableButton(SketchAreaWidget *parent);
	SketchToolButton *createNoteButton(SketchAreaWidget *parent);
	QWidget *createToolbarSpacer(SketchAreaWidget *parent);
	SketchAreaWidget *currentSketchArea();
	const QString fritzingTitle();

	virtual void updateRaiseWindowAction();

	void moveEvent(QMoveEvent * event);
	bool event(QEvent *);
	void resizeEvent(QResizeEvent * event);
	QString genIcon(SketchWidget *, LayerList &  partViewLayerIDs, LayerList & wireViewLayerIDs);

	bool alreadyOpen(const QString & fileName);
	void svgMissingLayer(const QString & layername, const QString & path);
	long swapSelectedAuxAux(ItemBase * itemBase, const QString & moduleID, ViewLayer::ViewLayerPlacement viewLayerPlacement, QMap<QString, QString> & propsMap, QUndoCommand * parentCommand);
	bool swapSpecial(const QString & prop, QMap<QString, QString> & currPropsMap);

	void enableAddBendpointAct(QGraphicsItem *);
	class FileProgressDialog * exportProgress();
	QString constructFileName(const QString & differentiator, const QString & extension);
	bool isGroundFill(ItemBase * itemBase);

	QString getBoardSvg(ItemBase * board, int res, LayerList &);
	QString mergeBoardSvg(QString & svg, ItemBase * board, int res, bool flip, LayerList &);

	void updateActiveLayerButtons();
	int activeLayerIndex();
	bool hasLinkedProgramFiles(const QString & filename, QStringList & linkedProgramFiles);
	void pasteAux(bool pasteInPlace);

	void routingStatusLabelMouse(QMouseEvent*, bool show);
	class Wire * retrieveWire();
	class ConnectorItem * retrieveConnectorItem();
	QString getBomProps(ItemBase *);
	ModelPart * findReplacedby(ModelPart * originalModelPart);
	void groundFillAux(bool fillGroundTraces, ViewLayer::ViewLayerID viewLayerID);
    void groundFillAux2(bool fillGroundTraces);
	void connectStartSave(bool connect);
	void loadBundledSketch(const QString &fileName, bool addToRecent, bool setAsLastOpened, bool checkObsolete);
    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void mainLoadAux(const QString & fileName);
	QWidget * createGridSizeForm(GridSizeThing *);
    void massageOutput(QString & svg, bool doMask, bool doSilk, bool doPaste, QString & maskTop, QString & maskBottom, const QString & fileName, ItemBase * board, int dpi, const LayerList &);
    virtual void initLockedFiles(bool lockFiles);
    virtual void initSketchWidgets(bool withIcons);
	virtual void initWelcomeView();
    virtual void initDock();
    virtual void initMenus();
    virtual void moreInitDock();
    virtual void setInitialView();
    virtual void createFileMenu();
    virtual void createEditMenu();
    virtual void createPartMenu();
    virtual void createViewMenu();
    virtual void createWindowMenu();
    virtual void createTraceMenus();
    virtual void createHelpMenu();
    virtual void createRotateSubmenu(QMenu * parentMenu);
    virtual void createZOrderSubmenu(QMenu * parentMenu);
  //  virtual void createZOrderWireSubmenu(QMenu * parentMenu);
    virtual void createAlignSubmenu(QMenu * parentMenu);
    virtual void createAddToBinSubmenu(QMenu * parentMenu);
	virtual void populateExportMenu();

    // dock management
	void createDockWindows();
	void dontKeepMargins();
	class FDockWidget * makeDock(const QString & title, QWidget * widget, int dockMinHeight, int dockDefaultHeight, Qt::DockWidgetArea area = Qt::RightDockWidgetArea, DockFactory dockFactory = NULL);
	class FDockWidget * dockIt(FDockWidget* dock, int dockMinHeight, int dockDefaultHeight, Qt::DockWidgetArea area = Qt::RightDockWidgetArea);
	FDockWidget *newTopWidget();
	FDockWidget *newBottomWidget();
	void removeMargin(FDockWidget* dock);
	void addTopMargin(FDockWidget* dock);
	void addBottomMargin(FDockWidget* dock);
	void dockMarginAux(FDockWidget* dock, const QString &name, const QString &style);
    void initStyleSheet();
    virtual QString getStyleSheetSuffix();
	virtual QWidget * createTabWidget();
	virtual void addTab(QWidget * widget, const QString & label);
	virtual void addTab(QWidget * widget, const QString & iconPath, const QString & label, bool withIcon);
	virtual int currentTabIndex();
	virtual void setCurrentTabIndex(int);
	virtual QWidget * currentTabWidget();
	virtual bool activeLayerWidgetAlwaysOn();
    bool copySvg(const QString & path, QFileInfoList & svgEntryInfoList);
    void checkSwapObsolete(QList<ItemBase *> &, bool includeUpdateLaterMessage);
    QMessageBox::StandardButton oldSchematicMessage(const QString & filename);
    MainWindow * revertAux();

protected:
	static void removeActionsStartingAt(QMenu *menu, int start=0);
	static void setAutosave(int, bool);

protected:

	QUndoGroup *m_undoGroup;
	QUndoView *m_undoView;

	QPointer<SketchAreaWidget> m_breadboardWidget;
	QPointer<class BreadboardSketchWidget> m_breadboardGraphicsView;

	QPointer<SketchAreaWidget> m_schematicWidget;
	QPointer<class SchematicSketchWidget> m_schematicGraphicsView;

	QPointer<SketchAreaWidget> m_pcbWidget;
	QPointer<class PCBSketchWidget> m_pcbGraphicsView;

	QPointer<SketchAreaWidget> m_welcomeWidget;
	class WelcomeView * m_welcomeView;

    QPointer<class BinManager> m_binManager;
	QPointer<QWidget> m_tabWidget;
    QPointer<ReferenceModel> m_referenceModel;
    QPointer<class SketchModel> m_sketchModel;
    QPointer<class HtmlInfoView> m_infoView;
    QPointer<QToolBar> m_toolbar;

    bool m_closing;
	bool m_dontClose;
    bool m_firstOpen;

    QPointer<SketchAreaWidget> m_currentWidget;
    QPointer<SketchWidget> m_currentGraphicsView;

    //QToolBar *m_fileToolBar;
    //QToolBar *m_editToolBar;

    QAction *m_raiseWindowAct;

    // Fritzing Menu
    QMenu *m_fritzingMenu;
    QAction *m_aboutAct;
    QAction *m_preferencesAct;
    QAction *m_quitAct;
    QAction *m_exceptionAct;

    // File Menu
    enum { MaxRecentFiles = 10 };

	QMenu *m_fileMenu;
	QAction *m_newAct;
	QAction *m_openAct;
	QAction *m_revertAct;
	QMenu *m_openRecentFileMenu;
    QAction *m_openRecentFileActs[MaxRecentFiles];
	QMenu *m_openExampleMenu;
	QAction *m_saveAct;
	QAction *m_saveAsAct;
	QAction *m_pageSetupAct;
	QAction *m_printAct;
	QAction *m_shareOnlineAct;

	QAction * m_launchExternalProcessAct;

	QMenu *m_zOrderMenu;
	QMenu *m_zOrderWireMenu;
	QAction *m_bringToFrontAct;
	QAction *m_bringForwardAct;
	QAction *m_sendBackwardAct;
	QAction *m_sendToBackAct;
	class WireAction *m_bringToFrontWireAct;
	class WireAction *m_bringForwardWireAct;
	class WireAction *m_sendBackwardWireAct;
	class WireAction *m_sendToBackWireAct;

    QMenu *m_alignMenu;
    QAction * m_alignVerticalCenterAct;
    QAction * m_alignHorizontalCenterAct;
    QAction * m_alignTopAct;
    QAction * m_alignLeftAct;
    QAction * m_alignBottomAct;
    QAction * m_alignRightAct;

	// Export Menu
	QMenu *m_exportMenu;
	QAction *m_exportJpgAct;
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	QAction *m_exportPsAct;
#endif
	QAction *m_exportPngAct;
	QAction *m_exportPdfAct;
	QAction *m_exportEagleAct;
	QAction *m_exportGerberAct;
	QAction *m_exportEtchablePdfAct;
	QAction *m_exportEtchableSvgAct;
	QAction *m_exportBomAct;
	QAction *m_exportNetlistAct;
	QAction *m_exportSpiceNetlistAct;
	QAction *m_exportSvgAct;

    // Edit Menu
    QMenu *m_editMenu;
    QAction *m_undoAct;
    QAction *m_redoAct;
    QAction *m_cutAct;
    QAction *m_copyAct;
    QAction *m_pasteAct;
    QAction *m_pasteInPlaceAct;
    QAction *m_duplicateAct;
    QAction *m_deleteAct;
    QAction *m_deleteMinusAct;
    class WireAction *m_deleteWireAct;
    class WireAction *m_deleteWireMinusAct;
    QAction *m_selectAllAct;
    QAction *m_deselectAct;
	QAction *m_addNoteAct;

    // Part Menu
    QMenu *m_partMenu;
	QAction *m_infoViewOnHoverAction;
	QAction *m_exportNormalizedSvgAction;
	QAction *m_exportNormalizedFlattenedSvgAction;
	QAction *m_dumpAllPartsAction;
	QAction *m_testConnectorsAction;
    QAction *m_openInPartsEditorNewAct;
    QMenu *m_addToBinMenu;


	QMenu *m_rotateMenu;
	QAction *m_rotate90cwAct;
	QAction *m_rotate180Act;
	QAction *m_rotate90ccwAct;
	QAction *m_rotate45ccwAct;
	QAction *m_rotate45cwAct;

	QAction *m_moveLockAct;
	QAction *m_stickyAct;
	QAction *m_selectMoveLockAct;
	QAction *m_flipHorizontalAct;
	QAction *m_flipVerticalAct;
	QAction *m_showPartLabelAct;
	QAction *m_saveBundledPart;
	QAction *m_disconnectAllAct;
	QAction *m_selectAllObsoleteAct;
	QAction *m_swapObsoleteAct;
    QAction *m_findPartInSketchAct;
	QAction * m_openProgramWindowAct;

	QAction *m_addBendpointAct;
	QAction *m_convertToViaAct;
	QAction *m_convertToBendpointAct;
    QAction * m_convertToBendpointSeparator;
	QAction *m_flattenCurveAct;
    QAction *m_showAllLayersAct;
	QAction *m_hideAllLayersAct;

    QAction *m_hidePartSilkscreenAct;
    QAction *m_regeneratePartsDatabaseAct;


    // View Menu
    QMenu *m_viewMenu;
    QAction *m_zoomInAct;
    QAction *m_zoomInShortcut;
    QAction *m_zoomOutAct;
    QAction *m_fitInWindowAct;
    QAction *m_actualSizeAct;
    QAction *m_100PercentSizeAct;
    QAction *m_alignToGridAct;
    QAction *m_showGridAct;
    QAction *m_setGridSizeAct;
    QAction *m_setBackgroundColorAct;
    QAction *m_colorWiresByLengthAct;
    QAction *m_showWelcomeAct;
    QAction *m_showBreadboardAct;
    QAction *m_showSchematicAct;
    QAction *m_showProgramAct;
    QAction *m_showPCBAct;
	QAction *m_showPartsBinIconViewAct;
	QAction *m_showPartsBinListViewAct;
    //QAction *m_toggleToolbarAct;
    int m_numFixedActionsInViewMenu;

    // Window Menu
	QMenu *m_windowMenu;
	QAction *m_minimizeAct;
	QAction *m_togglePartLibraryAct;
	QAction *m_toggleInfoAct;
	QAction *m_toggleUndoHistoryAct;
	QAction *m_toggleDebuggerOutputAct;
	QAction *m_windowMenuSeparator;

	// Trace Menu
	QMenu *m_pcbTraceMenu;
	QMenu *m_schematicTraceMenu;
	QMenu *m_breadboardTraceMenu;
	QAction *m_newAutorouteAct;
	QAction *m_orderFabAct;
	QAction *m_activeLayerTopAct;
	QAction *m_activeLayerBottomAct;
	QAction *m_activeLayerBothAct;
	QAction *m_viewFromBelowToggleAct;
	QAction *m_viewFromBelowAct;
	QAction *m_viewFromAboveAct;
	class WireAction *m_createTraceWireAct;
	class WireAction *m_createWireWireAct;
	QAction *m_createJumperAct;
	QAction *m_changeTraceLayerAct;
	class WireAction *m_changeTraceLayerWireAct;
	QAction *m_excludeFromAutorouteAct;
	class WireAction *m_excludeFromAutorouteWireAct;
    QAction * m_showUnroutedAct;
	QAction *m_selectAllTracesAct;
	QAction *m_selectAllWiresAct;
	QAction *m_selectAllCopperFillAct;
	QAction *m_updateRoutingStatusAct;
	QAction *m_selectAllExcludedTracesAct;
	QAction *m_selectAllIncludedTracesAct;
	QAction *m_selectAllJumperItemsAct;
	QAction *m_selectAllViasAct;
	QAction *m_groundFillAct;
	QAction *m_removeGroundFillAct;
	QAction *m_copperFillAct;
	class ConnectorItemAction *m_setOneGroundFillSeedAct;
	QAction *m_setGroundFillSeedsAct;
	QAction *m_clearGroundFillSeedsAct;
    QAction *m_setGroundFillKeepoutAct;
	QAction *m_newDesignRulesCheckAct;
	QAction *m_autorouterSettingsAct;
	QAction *m_fabQuoteAct;
	QAction *m_tidyWiresAct;
	QAction *m_checkLoadedTracesAct;

	// Help Menu
    QMenu *m_helpMenu;
    QAction *m_openHelpAct;
    QAction *m_openDonateAct;
    QAction *m_examplesAct;
    QAction *m_partsRefAct;
    QAction *m_visitFritzingDotOrgAct;
    QAction *m_checkForUpdatesAct;
    QAction *m_aboutQtAct;
    QAction *m_reportBugAct;
	QAction *m_enableDebugAct;
	QAction *m_partsEditorHelpAct;
    QAction *m_tipsAndTricksAct;
    QAction *m_firstTimeHelpAct;

	// Wire Color Menu
	QMenu * m_breadboardWireColorMenu;
	QMenu * m_schematicWireColorMenu;

    // Dot icons
    QIcon m_dotIcon;
    QIcon m_emptyIcon;

	QList<SketchToolButton*> m_rotateButtons;
	QList<SketchToolButton*> m_flipButtons;
	QStackedWidget * m_activeLayerButtonWidget;
	QStackedWidget * m_viewFromButtonWidget;

    bool m_comboboxChanged;
    bool m_restarting;

    QStringList m_alienFiles;
    QString m_alienPartsMsg;
    QStringList m_filesReplacedByAlienOnes;

    QStringList m_openExampleActions;

	QPointer<class FSizeGrip> m_sizeGrip;

	QTimer m_setUpDockManagerTimer;
	QPointer<class FileProgressDialog> m_fileProgressDialog;
	QPointer<class ZoomSlider> m_zoomSlider;
	QPointer<QLabel> m_locationLabel;
	QString m_locationLabelUnits;

	QByteArray m_externalProcessOutput;

	QPointer<class LayerPalette> m_layerPalette;
	QPointer<class ProgramWindow> m_programWindow;
	QPointer<class ProgramWindow> m_programView;
	QList<LinkedFile *>  m_linkedProgramFiles;
	QString m_backupFileNameAndPath;
	QTimer m_autosaveTimer;
    QTimer m_fireQuoteTimer;
	bool m_autosaveNeeded;
	bool m_backingUp;
	QString m_bundledSketchName;
	RoutingStatus m_routingStatus;
	bool m_orderFabEnabled;		
	bool m_closeSilently;
	QString m_fzzFolder;
	QHash<QString, struct LockedFile *> m_fzzFiles;
	SwapTimer m_swapTimer;
	QPointer<Wire> m_activeWire;
	QPointer<ConnectorItem> m_activeConnectorItem;
    bool m_addedToTemp;
    QString m_settingsPrefix;
    bool m_convertedSchematic;
    bool m_useOldSchematic;
    bool m_noSchematicConversion;
    int m_initialTab;

    // dock management
	QList<FDockWidget*> m_docks;
	FDockWidget* m_topDock;
	FDockWidget* m_bottomDock;
	QString m_oldTopDockStyle;
	QString m_oldBottomDockStyle;
	bool m_dontKeepMargins;
    QPointer<QDialog> m_rolloverQuoteDialog;
    bool m_obsoleteSMDOrientation;
    QWidget * m_orderFabButton;
    int m_fireQuoteDelay;

public:
	static int AutosaveTimeoutMinutes;
	static bool AutosaveEnabled;
	static QString BackupFolder;
    static const int DockMinWidth;
    static const int DockMinHeight;

protected:
	static const QString UntitledSketchName;
	static int UntitledSketchIndex;
	static int CascadeFactorX;
	static int CascadeFactorY;
    static QRegExp GuidMatcher;
};

#endif
