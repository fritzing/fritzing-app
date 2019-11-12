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


#ifndef PEMAINWINDOW_H_
#define PEMAINWINDOW_H_


#include "../mainwindow/mainwindow.h"
#include "../model/modelpartshared.h"
#include "../sketch/sketchwidget.h"
#include "peconnectorsview.h"

class IconSketchWidget : public SketchWidget
{
	Q_OBJECT

public:
	IconSketchWidget(ViewLayer::ViewID, QWidget *parent=0);

	void addViewLayers();
};

struct ViewThing {
	ItemBase * itemBase = nullptr;
	QDomDocument * document = nullptr;
	int svgChangeCount = 0;
	bool everZoomed = false;
	SketchWidget * sketchWidget = nullptr;
	QString referenceFile;
	QString originalSvgPath;
	bool firstTime = false;
	bool busMode = false;
};

class ReferenceModel;
class PaletteItem;
class PEMetadataView;
class PEConnectorsView;
class PEToolView;
class PESvgView;
class PEGraphicsItem;
class Wire;
class WireAction;
class IconSketchWidget;
class PEMainWindow : public MainWindow
{
	Q_OBJECT

public:
	PEMainWindow(ReferenceModel * referenceModel, QWidget * parent);
	~PEMainWindow();

	bool setInitialItem(PaletteItem *);
	void changeTags(const QStringList &, bool updateDisplay);
	void changeProperties(const QHash<QString, QString> &, bool updateDisplay);
	void changeMetadata(const QString & name, const QString & value, bool updateDisplay);
	void changeConnectorMetadata(ConnectorMetadata *, bool updateDisplay);
	void changeSvg(SketchWidget *, const QString & filename, int changeDirection);
	void relocateConnectorSvg(SketchWidget *, const QString & id, const QString & terminalID, const QString & oldGorn, const QString & oldGornTerminal, const QString & newGorn, const QString & newGornTerminal, int changeDirection);
	void moveTerminalPoint(SketchWidget *, const QString & id, QSizeF, QPointF, int changeDirection);
	void restoreFzp(const QString & filename);
	bool editsModuleID(const QString &);
	void addBusConnector(const QString & busID, const QString & connectorID);
	void removeBusConnector(const QString & busID, const QString & connectorID, bool display);
	void changeSMD(const QString & smd, const QString & filename, int changeDirection);
	void changeReferenceFile(ViewLayer::ViewID viewID, const QString referenceFile);
	void changeOriginalFile(ViewLayer::ViewID viewID, const QString originalFile, int changeCount);

signals:
	void addToMyPartsSignal(ModelPart *, const QStringList &);

public slots:
	void metadataChanged(const QString & name, const QString & value);
	void propertiesChanged(const QHash<QString, QString> &);
	void tagsChanged(const QStringList &);
	void connectorMetadataChanged(struct ConnectorMetadata *);
	void removedConnectors(QList<ConnectorMetadata *> &);
	void highlightSlot(PEGraphicsItem *);
	void pegiMousePressed(PEGraphicsItem *, bool & ignore);
	void pegiMouseReleased(PEGraphicsItem *);
	void pegiTerminalPointMoved(PEGraphicsItem *, QPointF);
	void pegiTerminalPointChanged(PEGraphicsItem *, QPointF before, QPointF after);
	void switchedConnector(int);
	void removedConnector(const QDomElement &);
	void terminalPointChanged(const QString & how);
	void terminalPointChanged(const QString & coord, double value);
	void getSpinAmount(double & amount);
	void pickModeChanged(bool);
	void busModeChanged(bool);
	void connectorCountChanged(int);
	void deleteBusConnection();
	void newWireSlot(Wire *);
	void wireChangedSlot(Wire*, const QLineF & oldLine, const QLineF & newLine, QPointF oldPos, QPointF newPos, ConnectorItem * from, ConnectorItem * to);
	void connectorsTypeChanged(Connector::ConnectorType);
	void smdChanged(const QString &);
	void showing(SketchWidget *);
	void updateExportMenu();
	void updateEditMenu();
	void s2sMessageSlot(const QString & message);

protected:
	void closeEvent(QCloseEvent * event);
	bool event(QEvent *);
	void initLockedFiles(bool lockFiles);
	void initSketchWidgets(bool withIcons);
	void initProgrammingWidget();
	void initDock();
	void moreInitDock();
	void setInitialView();
	void createActions();
	void createMenus();
	QList<QWidget*> getButtonsForView(ViewLayer::ViewID);
	void connectPairs();
	QMenu *breadboardItemMenu();
	QMenu *schematicItemMenu();
	QMenu *pcbItemMenu();
	QMenu *pcbWireMenu();
	QMenu *schematicWireMenu();
	QMenu *breadboardWireMenu();
	void setTitle();
	void createViewMenuActions(bool showWelcome);
	void createFileMenuActions();
	void createViewMenu();
	void createEditMenu();
	QHash<QString, QString> getOldProperties();
	QDomElement findConnector(const QString & id, int & index);
	void changeConnectorElement(QDomElement & connector, ConnectorMetadata *);
	void initSvgTree(SketchWidget *, ItemBase *, QDomDocument &);
	void initConnectors(bool updateConnectorsView);
	QString createSvgFromImage(const QString &origFilePath);
	// QString makeSvgPath(const QString & referenceFile, SketchWidget * sketchWidget, bool useIndex);
	QString makeSvgPath2(SketchWidget * sketchWidget);
	QString saveFzp();
	void reload(bool firstTime);
	void createFileMenu();
	void updateChangeCount(SketchWidget * sketchWidget, int changeDirection);
	PEGraphicsItem * findConnectorItem();
	void terminalPointChangedAux(PEGraphicsItem * pegi, QPointF before, QPointF after);
	void showInOS(QWidget *parent, const QString &pathIn);
	void switchedConnector(int, SketchWidget *);
	PEGraphicsItem * makePegi(QSizeF size, QPointF topLeft, ItemBase * itemBase, QDomElement & element, double z);
	QRectF getPixelBounds(FSvgRenderer & renderer, QDomElement & element);
	bool canSave();
	bool saveAs(bool overWrite);
	void setBeforeClosingText(const QString & filename, QMessageBox & messageBox);
	QString getPartTitle();
	void killPegi();
	bool loadFzp(const QString & path);
	void removedConnectorsAux(QList<QDomElement> & connectors);
	//QString getFzpReferenceFile();
	QString getSvgReferenceFile(const QString & filename);
	QString makeDesc(const QString & referenceFile);
	void insertDesc(const QString & referenceFile, QString & svg);
	void updateRaiseWindowAction();
	bool writeXml(const QString & path, const QString & text, bool temp);
	void displayBuses();
	QDomElement findBus(const QDomElement &, const QString &);
	QDomElement findNodeMemberBus(const QDomElement & buses, const QString & connectorID);
	void replaceProperty(const QString & key, const QString & value, QDomElement & properties);
	QWidget * createTabWidget();
	void addTab(QWidget * widget, const QString & label);
	int currentTabIndex();
	void setCurrentTabIndex(int);
	QWidget * currentTabWidget();
	void changeSpecialProperty(const QString & name, const QString & value);
	bool eventFilter(QObject *, QEvent *);
	void relocateConnector(PEGraphicsItem * pegi);
	void clearPickMode();
	void deleteBuses();
	QList<PEGraphicsItem *> getPegiList(SketchWidget *);
	void fillInMetadata(const QDomElement & connector, ConnectorMetadata & cmd);
	void setConnectorSMD(bool toSMD, QDomElement & connector);
	bool activeLayerWidgetAlwaysOn();
	void updateFileMenu();
	void reuseImage(ViewLayer::ViewID);
	void setImageAttribute(QDomElement & layers, const QString & image);
	void setImageAttribute(QDomElement & fzpRoot, const QString & svgPath, ViewLayer::ViewID viewID);
	QString makeNewVariant(const QString & family);
	void connectorWarning();
	bool anyMarquee();
	bool anyVisible();
	QString makeDirName();
	void initWelcomeView();

protected slots:
	void initZoom();
	void showMetadataView();
	void showConnectorsView();
	void showIconView();
	void loadImage();
	bool save();
	bool saveAs();
	void showInOS();
	void tabWidget_currentChanged(int index);
	void backupSketch();
	void updateWindowMenu();
	void updateWireMenu();
	void closeLater();
	void resetNextPick();
	void reuseBreadboard();
	void reuseSchematic();
	void reusePCB();
	void convertToTenth();
	void hideOtherViews();
	void updateLayerMenu(bool resetLayout = false);
	void updateAssignedConnectors();
	void itemAddedSlot(ModelPart *, ItemBase *, ViewLayer::ViewLayerPlacement, const ViewGeometry &, long id, SketchWidget * dropOrigin);
	void itemMovedSlot(ItemBase *);
	void clickedItemCandidateSlot(QGraphicsItem *, bool & ok);
	void resizedSlot(ItemBase *);


protected:
	QDomDocument m_fzpDocument;
	QDomDocument m_iconDocument;
	QDomDocument m_breadboardDocument;
	QDomDocument m_schematicDocument;
	QDomDocument m_pcbDocument;

	QAction * m_showMetadataViewAct = nullptr;
	QAction * m_showConnectorsViewAct = nullptr;
	QAction * m_showIconAct = nullptr;
	QAction * m_showInOSAct = nullptr;
	WireAction * m_deleteBusConnectionAct = nullptr;
	QAction * m_reuseBreadboardAct = nullptr;
	QAction * m_reuseSchematicAct = nullptr;
	QAction * m_reusePCBAct = nullptr;
	QAction * m_hideOtherViewsAct = nullptr;
	QAction * m_convertToTenthAct = nullptr;

	QPointer<SketchAreaWidget> m_iconWidget;
	QPointer<IconSketchWidget> m_iconGraphicsView;
	PEMetadataView * m_metadataView = nullptr;
	PEConnectorsView * m_connectorsView = nullptr;
	PEToolView * m_peToolView = nullptr;
	PESvgView * m_peSvgView = nullptr;
	QString m_guid;
	QString m_prefix;
	int m_fileIndex;
	QHash<ViewLayer::ViewID, ViewThing *> m_viewThings;
	QString m_userPartsFolderPath;
	QString m_userPartsFolderSvgPath;
	bool m_canSave;
	QString m_originalFzpPath;
	QString m_originalModuleID;
	bool m_gaveSaveWarning;
	QStringList m_filesToDelete;
	QList< QPointer<QWidget> > m_inFocusWidgets;
	bool m_inPickMode;
	bool m_useNextPick;
	QList<QDomElement> m_connectorList;

	QString m_removedConnector;
};


#endif /* PEMAINWINDOW_H_ */
