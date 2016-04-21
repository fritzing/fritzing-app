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

$Revision: 6980 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-22 01:45:43 +0200 (Mo, 22. Apr 2013) $

********************************************************************/

/* TO DO ******************************************


    ///////////////////////////////// first release ///////////////////////////////
		        							       
    clicking set bus mode fucks up anchor points

    terminal point area is not highlighting properly
   
    ////////////////////// second release /////////////////////////////////

		icon editor?

        need simplified infoview

        change color of breadboard text item

        for svg import check for flaws:
            multiple connector or terminal ids
			trash any other matching id

		loading tht into smd or v.v.
			can't seem to assign connectors after adding connectors

		why isn't swapping available when a family has new parts with multiple variant values?

		test button with export etchable to make sure the part is right?
			export etchable is disabled without a board

		align to grid when moving?

		restore editable pin labels functionality
			requires storing labels in the part rather than in the sketch

		hidden connectors
			allow multiple hits to the same place and add copies of the svg element if necessary

		restore parts bin

        buses 
			secondary representation (list view)
				display during bus mode instead of connector list
				allow to delete bus, delete nodemember, add nodemember using right-click for now

		use the actual svg shape instead of rectangles
			construct a new svg with the width and height of the bounding box of the element
			translate the element by the current top-left of the element
			set the fill color and kill the stroke width

		properties and tags entries allow duplicates

		delete temp files after crash

        show in OS button only shows folder and not file in linux

        smd vs. tht
            after it's all over remove copper0 if part is all smd
            split copper0 and copper1
            allow mixed tht/smd parts in fritzing but for now disable flipsmd
            allow smd with holes to flip?

        import
            kicad sch files?
				kicad schematic does not match our schematic

        on svg import detect all connector IDs
            if any are invisible, tell user this is obsolete

        sort connector list alphabetically or numerically?

        full svg outline view

        bendable legs
			same as terminalID, but must be a line
			use a radio buttons to distinguish?

        set flippable
        
        connector duplicate op

        add layers:  put everything in silkscreen, then give copper1, copper0 checkbox
            what about breadboardbreadboard or other odd layers?
            if you click something as a connector, automatically move it into copper
                how to distinguish between both and top--default to both, let user set "pad"
			setting layer for top level group sets all children?

        swap connector metadata op

        delete op
    
        move connectors with arrow keys, or typed coordinates
	    drag and drop later

        import
            eagle lbr
            eagle brd

        for schematic view 
            offer lines-or-pins, rects, and a selection of standard schematic icons in the parts bin

        for breadboard view
            import 
            generate ICs, dips, sips, breakouts

        for pcb view
            pads, pins (circle, rect, oblong), holes
            lines and curves?
            import silkscreen

        hybrids

        flip and rotate images w/in the view?

        taxonomy entry like tag entry?

        new schematic layout specs

        give users a family popup with all family names
            ditto for other properties

        matrix problem with move and duplicate (i.e. if element inherits a matrix from far above)
            even a problem when inserting hole, pad, or pin
            eliminate internal transforms, then insert inverted matrix, then use untransformed coords based on viewbox
            

***************************************************/

#include "pemainwindow.h"
#include "pemetadataview.h"
#include "peconnectorsview.h"
#include "pecommands.h"
#include "petoolview.h"
#include "pesvgview.h"
#include "pegraphicsitem.h"
#include "kicadmoduledialog.h"
#include "../debugdialog.h"
#include "../sketch/breadboardsketchwidget.h"
#include "../sketch/schematicsketchwidget.h"
#include "../sketch/pcbsketchwidget.h"
#include "../mainwindow/sketchareawidget.h"
#include "../referencemodel/referencemodel.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../utils/folderutils.h"
#include "../utils/s2s.h"
#include "../mainwindow/fdockwidget.h"
#include "../fsvgrenderer.h"
#include "../partsbinpalette/binmanager/binmanager.h"
#include "../svg/gedaelement2svg.h"
#include "../svg/kicadmodule2svg.h"
#include "../svg/kicadschematic2svg.h"
#include "../sketchtoolbutton.h"
#include "../items/virtualwire.h"
#include "../connectors/connectoritem.h"
#include "../connectors/bus.h"
#include "../installedfonts.h"
#include "../dock/layerpalette.h"
#include "../utils/cursormaster.h"
#include "../infoview/htmlinfoview.h"

#include <QtDebug>
#include <QInputDialog>
#include <QApplication>
#include <QSvgGenerator>
#include <QMenuBar>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QBuffer>
#include <QClipboard>
#include <limits>

////////////////////////////////////////////////////

static bool RubberBandLegWarning = false;

static bool GotZeroConnector = false;

static const QString ReferenceFileString("referenceFile");

static const int IconViewIndex = 3;
static const int MetadataViewIndex = 4;
static const int ConnectorsViewIndex = 5;

const static int PegiZ = 5000;
const static int RatZ = 6000;

static long FakeGornSiblingNumber = 0;

void removeGornAux(QDomElement & element) {
    bool hasGorn = element.hasAttribute("gorn");
    bool hasOld = element.hasAttribute("oldid");
    if (hasGorn && hasOld) {
        element.removeAttribute("gorn");
        QString oldid = element.attribute("oldid");
        if (oldid.isEmpty()) element.removeAttribute("id");
        else element.setAttribute("id", oldid);
        element.removeAttribute("oldid");
    }
    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        removeGornAux(child);
        child = child.nextSiblingElement();
    }
}

QString removeGorn(QString & svg) {
    QDomDocument doc;

    QString errorStr;
    int errorLine;
    int errorColumn;
    if (!doc.setContent(svg, &errorStr, &errorLine, &errorColumn)) {
        // shouldn't happen
        DebugDialog::debug(QString("remove gorn failure: %1 %2 %3 %4").arg(errorStr).arg(errorLine).arg(errorColumn).arg(svg));
        return svg;
    }

    QDomElement root = doc.documentElement();
    removeGornAux(root);

	return doc.toString(4);
}

bool byID(QDomElement & c1, QDomElement & c2)
{
    int c1id = -1;
    int c2id = -1;
	int ix = IntegerFinder.indexIn(c1.attribute("id"));
    if (ix >= 0) c1id = IntegerFinder.cap(0).toInt();
    ix = IntegerFinder.indexIn(c2.attribute("id"));
    if (ix >= 0) c2id = IntegerFinder.cap(0).toInt();

    if (c1id == 0 || c2id == 0) GotZeroConnector = true;
	
	return c1id <= c2id;
}

void removeID(QDomElement & root, const QString & value) {
    if (root.attribute("id") == value) {
        root.removeAttribute("id");
    }

	QDomNodeList nodeList = root.childNodes();
	for (int i = 0; i < nodeList.count(); i++) {
        QDomNode node = nodeList.item(i);
        if (node.nodeType() == QDomNode::ElementNode) {
            QDomElement element = node.toElement();
		    removeID(element, value);
        }
	}
}

////////////////////////////////////////////////////

IconSketchWidget::IconSketchWidget(ViewLayer::ViewID viewID, QWidget *parent)
    : SketchWidget(viewID, parent)
{
	m_shortName = QObject::tr("ii");
	m_viewName = QObject::tr("Icon View");
	initBackgroundColor();
}

void IconSketchWidget::addViewLayers() {
	setViewLayerIDs(ViewLayer::Icon, ViewLayer::Icon, ViewLayer::Icon, ViewLayer::Icon, ViewLayer::Icon);
	addViewLayersAux(ViewLayer::layersForView(ViewLayer::IconView), ViewLayer::layersForViewFromBelow(ViewLayer::IconView));
}

////////////////////////////////////////////////////

ViewThing::ViewThing() {
    itemBase = NULL;
    document = NULL;
    svgChangeCount = 0;
    everZoomed = false;
    sketchWidget = NULL;
	firstTime = true;
    busMode = false;
}

/////////////////////////////////////////////////////

PEMainWindow::PEMainWindow(ReferenceModel * referenceModel, QWidget * parent)
	: MainWindow(referenceModel, parent)
{
    m_viewThings.insert(ViewLayer::BreadboardView, new ViewThing);
	m_viewThings.insert(ViewLayer::SchematicView, new ViewThing);
	m_viewThings.insert(ViewLayer::PCBView, new ViewThing);
	m_viewThings.insert(ViewLayer::IconView, new ViewThing);

	m_useNextPick = m_inPickMode = false;
	m_autosaveTimer.stop();
    disconnect(&m_autosaveTimer, SIGNAL(timeout()), this, SLOT(backupSketch()));
    m_gaveSaveWarning = m_canSave = false;
    m_settingsPrefix = "pe/";
    m_guid = TextUtils::getRandText();
    m_prefix = "prefix0000";
    m_fileIndex = 0;
    m_userPartsFolderPath = FolderUtils::getUserPartsPath()+"/user/";
    m_userPartsFolderSvgPath = FolderUtils::getUserPartsPath()+"/svg/user/";
    m_peToolView = NULL;
    m_peSvgView = NULL;
    m_connectorsView = NULL;
}

PEMainWindow::~PEMainWindow()
{
    // PEGraphicsItems are still holding QDomElement so delete them before m_fzpDocument is deleted
    killPegi();

	// kill temp files
	foreach (QString string, m_filesToDelete) {
		QFile::remove(string);
	}
	QDir dir = QDir::temp();
    dir.rmdir(makeDirName());
}

void PEMainWindow::closeEvent(QCloseEvent *event) 
{
	qDebug() << "close event";

	QStringList messages;

	if (m_inFocusWidgets.count() > 0) {
		bool gotOne = false;
		// should only be one in-focus widget
		foreach (QWidget * widget, m_inFocusWidgets) {
			QLineEdit * lineEdit = qobject_cast<QLineEdit *>(widget);
			if (lineEdit) {
				if (lineEdit->isModified()) {
					lineEdit->clearFocus();
                    lineEdit->setModified(false);
					gotOne = true;
				}
			}
			else {
				QTextEdit * textEdit = qobject_cast<QTextEdit *>(widget);
				if (textEdit) {
					if (textEdit->document()->isModified()) {
						textEdit->clearFocus();
                        textEdit->document()->setModified(false);
						gotOne = true;
					}
				}
			}
		}
		if (gotOne) {
			messages << tr("There is one last edit still pending.");
		}
	}

	QString family = m_metadataView->family();
	if (family.isEmpty()) {
		messages << tr("The 'family' property can not be blank.");
	}

	QStringList keys = m_metadataView->properties().keys();

	if (keys.contains("family", Qt::CaseInsensitive)) {
		messages << tr("A duplicate 'family' property is not allowed");
	}

	if (keys.contains("variant", Qt::CaseInsensitive)) {
		messages << tr("A duplicate 'variant' property is not allowed");
	}

	bool discard = true;
	if (messages.count() > 0) {
	    QMessageBox messageBox(this);
	    messageBox.setWindowTitle(tr("Close without saving?"));
		
		QString message = tr("This part can not be saved as-is:\n\n");
		foreach (QString string, messages) {
			message.append('\t');
			message.append(string);
			messages.append("\n\n");
		}

        message += tr("Do you want to keep working or close without saving?");

	    messageBox.setText(message);
	    messageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	    messageBox.setDefaultButton(QMessageBox::Cancel);
	    messageBox.setIcon(QMessageBox::Warning);
	    messageBox.setWindowModality(Qt::WindowModal);
	    messageBox.setButtonText(QMessageBox::Ok, tr("Close without saving"));
	    messageBox.setButtonText(QMessageBox::Cancel, tr("Keep working"));
	    QMessageBox::StandardButton answer = (QMessageBox::StandardButton) messageBox.exec();

	    if (answer != QMessageBox::Ok) {
			event->ignore();
		    return;
	    }
	}
	else if (!beforeClosing(true, discard)) {
        event->ignore();
        return;
    }

	if (!discard) {
		// if all connectors not found, put up a message
		connectorWarning();
	}

	QSettings settings;
	settings.setValue(m_settingsPrefix + "state", saveState());
	settings.setValue(m_settingsPrefix + "geometry", saveGeometry());

	QMainWindow::closeEvent(event);
}

void PEMainWindow::initLockedFiles(bool) {
}

void PEMainWindow::initSketchWidgets(bool whatever)
{
    Q_UNUSED(whatever);

    MainWindow::initSketchWidgets(false);

	m_iconGraphicsView = new IconSketchWidget(ViewLayer::IconView, this);
	initSketchWidget(m_iconGraphicsView);
	m_iconWidget = new SketchAreaWidget(m_iconGraphicsView,this);
	addTab(m_iconWidget, tr("Icon"));
    initSketchWidget(m_iconGraphicsView);

	ViewThing * viewThing = m_viewThings.value(m_breadboardGraphicsView->viewID());
	viewThing->sketchWidget = m_breadboardGraphicsView;
	viewThing->document = &m_breadboardDocument;

	viewThing = m_viewThings.value(m_schematicGraphicsView->viewID());
	viewThing->sketchWidget = m_schematicGraphicsView;
	viewThing->document = &m_schematicDocument;

	viewThing = m_viewThings.value(m_pcbGraphicsView->viewID());
	viewThing->sketchWidget = m_pcbGraphicsView;
	viewThing->document = &m_pcbDocument;

	viewThing = m_viewThings.value(m_iconGraphicsView->viewID());
	viewThing->sketchWidget = m_iconGraphicsView;
	viewThing->document = &m_iconDocument;
	
    foreach (ViewThing * viewThing, m_viewThings.values()) {
        viewThing->sketchWidget->setAcceptWheelEvents(true);
		viewThing->sketchWidget->setChainDrag(false);				// no bendpoints
        viewThing->firstTime = true;
        viewThing->everZoomed = true;
		connect(viewThing->sketchWidget, SIGNAL(newWireSignal(Wire *)), this, SLOT(newWireSlot(Wire *)));
		connect(viewThing->sketchWidget, SIGNAL(showing(SketchWidget *)), this, SLOT(showing(SketchWidget *)));
		connect(viewThing->sketchWidget, SIGNAL(itemMovedSignal(ItemBase *)), this, SLOT(itemMovedSlot(ItemBase *)));
		connect(viewThing->sketchWidget, SIGNAL(resizedSignal(ItemBase *)), this, SLOT(resizedSlot(ItemBase *)));
		connect(viewThing->sketchWidget, SIGNAL(clickedItemCandidateSignal(QGraphicsItem *, bool &)), this, SLOT(clickedItemCandidateSlot(QGraphicsItem *, bool &)), Qt::DirectConnection);
    	connect(viewThing->sketchWidget, SIGNAL(itemAddedSignal(ModelPart *, ItemBase *, ViewLayer::ViewLayerPlacement, const ViewGeometry &, long, SketchWidget *)),
							 this, SLOT(itemAddedSlot(ModelPart *, ItemBase *, ViewLayer::ViewLayerPlacement, const ViewGeometry &, long, SketchWidget *)));


    }

    m_metadataView = new PEMetadataView(this);
    SketchAreaWidget * sketchAreaWidget = new SketchAreaWidget(m_metadataView, this, false, false);
	addTab(sketchAreaWidget, tr("Metadata"));
    connect(m_metadataView, SIGNAL(metadataChanged(const QString &, const QString &)), this, SLOT(metadataChanged(const QString &, const QString &)), Qt::DirectConnection);
    connect(m_metadataView, SIGNAL(tagsChanged(const QStringList &)), this, SLOT(tagsChanged(const QStringList &)), Qt::DirectConnection);
    connect(m_metadataView, SIGNAL(propertiesChanged(const QHash<QString, QString> &)), this, SLOT(propertiesChanged(const QHash<QString, QString> &)), Qt::DirectConnection);

    m_connectorsView = new PEConnectorsView(this);
    sketchAreaWidget = new SketchAreaWidget(m_connectorsView, this, false, false);
	addTab(sketchAreaWidget, tr("Connectors"));

    connect(m_connectorsView, SIGNAL(connectorMetadataChanged(ConnectorMetadata *)), this, SLOT(connectorMetadataChanged(ConnectorMetadata *)), Qt::DirectConnection);
    connect(m_connectorsView, SIGNAL(connectorsTypeChanged(Connector::ConnectorType)), this, SLOT(connectorsTypeChanged(Connector::ConnectorType)));
    connect(m_connectorsView, SIGNAL(removedConnectors(QList<ConnectorMetadata *> &)), this, SLOT(removedConnectors(QList<ConnectorMetadata *> &)), Qt::DirectConnection);
    connect(m_connectorsView, SIGNAL(connectorCountChanged(int)), this, SLOT(connectorCountChanged(int)), Qt::DirectConnection);
    connect(m_connectorsView, SIGNAL(smdChanged(const QString &)), this, SLOT(smdChanged(const QString &)));

}

void PEMainWindow::initDock()
{
	m_layerPalette = new LayerPalette(this);

    /*
    m_infoView = new HtmlInfoView();
    m_infoView->init(true);

    m_binManager = new BinManager(m_referenceModel, m_infoView, m_undoStack, this);
    m_binManager->openBin(":/resources/bins/pe.fzb");
    m_binManager->hideTabBar();
    */
}

void PEMainWindow::moreInitDock()
{
    static int BinMinHeight = 75;
    static int ToolDefaultHeight = 150;
    static int SvgDefaultHeight = 50;


    m_peToolView = new PEToolView();
    connect(m_peToolView, SIGNAL(getSpinAmount(double &)), this, SLOT(getSpinAmount(double &)), Qt::DirectConnection);
    connect(m_peToolView, SIGNAL(terminalPointChanged(const QString &)), this, SLOT(terminalPointChanged(const QString &)));
    connect(m_peToolView, SIGNAL(terminalPointChanged(const QString &, double)), this, SLOT(terminalPointChanged(const QString &, double)));
    connect(m_peToolView, SIGNAL(switchedConnector(int)), this, SLOT(switchedConnector(int)));
    connect(m_peToolView, SIGNAL(removedConnector(const QDomElement &)), this, SLOT(removedConnector(const QDomElement &)));
    connect(m_peToolView, SIGNAL(pickModeChanged(bool)), this, SLOT(pickModeChanged(bool)));
    connect(m_peToolView, SIGNAL(busModeChanged(bool)), this, SLOT(busModeChanged(bool)));
    connect(m_peToolView, SIGNAL(connectorMetadataChanged(ConnectorMetadata *)), this, SLOT(connectorMetadataChanged(ConnectorMetadata *)), Qt::DirectConnection);
    makeDock(tr("Connectors"), m_peToolView, DockMinWidth, ToolDefaultHeight);
    m_peToolView->setMinimumSize(DockMinWidth, ToolDefaultHeight);

    m_peSvgView = new PESvgView();
    makeDock(tr("SVG"), m_peSvgView, DockMinWidth, SvgDefaultHeight);
    m_peSvgView->setMinimumSize(DockMinWidth, SvgDefaultHeight);

    if (m_binManager) {
	    QDockWidget * dockWidget = makeDock(BinManager::Title, m_binManager, DockMinWidth, BinMinHeight);
        dockWidget->resize(0, 0);
    }

    if (m_infoView) {
        makeDock(tr("Inspector"), m_infoView, InfoViewMinHeight, InfoViewHeightDefault);
        this -> setObjectName("PEInspector");
    }

    makeDock(tr("Layers"), m_layerPalette, DockMinWidth, DockMinHeight)->hide();
    m_layerPalette->setMinimumSize(DockMinWidth, DockMinHeight);
	m_layerPalette->setShowAllLayersAction(m_showAllLayersAct);
	m_layerPalette->setHideAllLayersAction(m_hideAllLayersAct);

}

void PEMainWindow::createFileMenuActions() {
	MainWindow::createFileMenuActions();

	m_reuseBreadboardAct = new QAction(tr("Reuse breadboard image"), this);
	m_reuseBreadboardAct->setStatusTip(tr("Reuse the breadboard image in this view"));
	connect(m_reuseBreadboardAct, SIGNAL(triggered()), this, SLOT(reuseBreadboard()));

	m_reuseSchematicAct = new QAction(tr("Reuse schematic image"), this);
	m_reuseSchematicAct->setStatusTip(tr("Reuse the schematic image in this view"));
	connect(m_reuseSchematicAct, SIGNAL(triggered()), this, SLOT(reuseSchematic()));

	m_reusePCBAct = new QAction(tr("Reuse PCB image"), this);
	m_reusePCBAct->setStatusTip(tr("Reuse the PCB image in this view"));
	connect(m_reusePCBAct, SIGNAL(triggered()), this, SLOT(reusePCB()));

}

void PEMainWindow::createActions()
{
    createFileMenuActions();

	m_openAct->setText(tr("Load image for view..."));
	m_openAct->setStatusTip(tr("Open a file to use as the image for this view of the part."));
	disconnect(m_openAct, SIGNAL(triggered()), this, SLOT(mainLoad()));
	connect(m_openAct, SIGNAL(triggered()), this, SLOT(loadImage()));

	m_showInOSAct = new QAction(tr("Show in Folder"), this);
	m_showInOSAct->setStatusTip(tr("On the desktop, open the folder containing the current svg file."));
	connect(m_showInOSAct, SIGNAL(triggered()), this, SLOT(showInOS()));

    createEditMenuActions();

	m_convertToTenthAct = new QAction(tr("Convert schematic to 0.1 inch standard"), this);
	m_convertToTenthAct->setStatusTip(tr("Convert pre-0.8.6 schematic image to new 0.1 inch standard"));
	connect(m_convertToTenthAct, SIGNAL(triggered()), this, SLOT(convertToTenth()));

	m_deleteBusConnectionAct = new WireAction(tr("Remove Internal Connection"), this);
	connect(m_deleteBusConnectionAct, SIGNAL(triggered()), this, SLOT(deleteBusConnection()));

    createViewMenuActions(false);
    createHelpMenuActions();
    createWindowMenuActions();
	createActiveLayerActions();

}

void PEMainWindow::createMenus()
{
    createFileMenu();
    createEditMenu();
    createViewMenu();
    createWindowMenu();
    createHelpMenu();
}

void PEMainWindow::createFileMenu() {
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_openAct);
    m_fileMenu->addAction(m_reuseBreadboardAct);
    m_fileMenu->addAction(m_reuseSchematicAct);
    m_fileMenu->addAction(m_reusePCBAct);

    //m_fileMenu->addAction(m_revertAct);

    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_closeAct);
    m_fileMenu->addAction(m_saveAct);
    m_fileMenu->addAction(m_saveAsAct);
    m_saveAsAct->setText(tr("Save as new part"));
    m_saveAsAct->setStatusTip(tr("Make a copy of the part and save it in the 'My Parts' Bin"));

    m_fileMenu->addSeparator();
	m_exportMenu = m_fileMenu->addMenu(tr("&Export"));
    //m_fileMenu->addAction(m_pageSetupAct);
    m_fileMenu->addAction(m_printAct);
    m_fileMenu->addAction(m_showInOSAct);

	m_fileMenu->addSeparator();
	m_fileMenu->addAction(m_quitAct);

	populateExportMenu();

    connect(m_fileMenu, SIGNAL(aboutToShow()), this, SLOT(updateFileMenu()));
}

void PEMainWindow::createEditMenu()
{
    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_undoAct);
    m_editMenu->addAction(m_redoAct);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_convertToTenthAct);

    updateEditMenu();
    connect(m_editMenu, SIGNAL(aboutToShow()), this, SLOT(updateEditMenu()));
}

QList<QWidget*> PEMainWindow::getButtonsForView(ViewLayer::ViewID viewID) {

	QList<QWidget*> retval;
	SketchAreaWidget *parent;
	switch(viewID) {
		case ViewLayer::BreadboardView: parent = m_breadboardWidget; break;
		case ViewLayer::SchematicView: parent = m_schematicWidget; break;
		case ViewLayer::PCBView: parent = m_pcbWidget; break;
		default: return retval;
	}

	//retval << createExportEtchableButton(parent);

	switch (viewID) {
		case ViewLayer::BreadboardView:
			break;
		case ViewLayer::SchematicView:
			break;
		case ViewLayer::PCBView:
			// retval << createActiveLayerButton(parent);
			break;
		default:
			break;
	}

	return retval;
}

bool PEMainWindow::activeLayerWidgetAlwaysOn() {
	return true;
}

void PEMainWindow::connectPairs() {
    bool succeeded = true;
	succeeded =  succeeded && connect(qApp, SIGNAL(spaceBarIsPressedSignal(bool)), m_breadboardGraphicsView, SLOT(spaceBarIsPressedSlot(bool)));
	succeeded =  succeeded && connect(qApp, SIGNAL(spaceBarIsPressedSignal(bool)), m_schematicGraphicsView, SLOT(spaceBarIsPressedSlot(bool)));
	succeeded =  succeeded && connect(qApp, SIGNAL(spaceBarIsPressedSignal(bool)), m_pcbGraphicsView, SLOT(spaceBarIsPressedSlot(bool)));

	succeeded =  succeeded && connect(m_pcbGraphicsView, SIGNAL(cursorLocationSignal(double, double, double, double)), this, SLOT(cursorLocationSlot(double, double, double, double)));
	succeeded =  succeeded && connect(m_breadboardGraphicsView, SIGNAL(cursorLocationSignal(double, double, double, double)), this, SLOT(cursorLocationSlot(double, double, double, double)));
	succeeded =  succeeded && connect(m_schematicGraphicsView, SIGNAL(cursorLocationSignal(double, double, double, double)), this, SLOT(cursorLocationSlot(double, double, double, double)));

	connect(m_breadboardGraphicsView, SIGNAL(setActiveWireSignal(Wire *)), this, SLOT(setActiveWire(Wire *)));
	connect(m_schematicGraphicsView, SIGNAL(setActiveWireSignal(Wire *)), this, SLOT(setActiveWire(Wire *)));
	connect(m_pcbGraphicsView, SIGNAL(setActiveWireSignal(Wire *)), this, SLOT(setActiveWire(Wire *)));

}

QMenu *PEMainWindow::breadboardWireMenu() {
	QMenu *menu = new QMenu(QObject::tr("Internal Connections"), this);

	menu->addAction(m_deleteBusConnectionAct);
    connect( menu, SIGNAL(aboutToShow()), this, SLOT(updateWireMenu()));
	return menu;
}
    
QMenu *PEMainWindow::breadboardItemMenu() {
    return NULL;
}

QMenu *PEMainWindow::schematicWireMenu() {
    return breadboardWireMenu();
}
    
QMenu *PEMainWindow::schematicItemMenu() {
    return NULL;
}

QMenu *PEMainWindow::pcbWireMenu() {
    return breadboardWireMenu();
}
    
QMenu *PEMainWindow::pcbItemMenu() {
    return NULL;
}

bool PEMainWindow::setInitialItem(PaletteItem * paletteItem) 
{
    m_pcbGraphicsView->setBoardLayers(2, true);
	m_pcbGraphicsView->setLayerActive(ViewLayer::Copper1, true);
	m_pcbGraphicsView->setLayerActive(ViewLayer::Copper0, true);
	m_pcbGraphicsView->setLayerActive(ViewLayer::Silkscreen1, true);
	m_pcbGraphicsView->setLayerActive(ViewLayer::Silkscreen0, true);

    ModelPart * originalModelPart = NULL;
    if (paletteItem == NULL) {
        // this shouldn't happen
        originalModelPart = m_referenceModel->retrieveModelPart("generic_ic_dip_8_300mil");
    }
    else {
        originalModelPart = paletteItem->modelPart();
    }

    m_originalFzpPath = originalModelPart->path();
    m_originalModuleID = originalModelPart->moduleID();

    QFileInfo info(originalModelPart->path());
    QString basename = info.completeBaseName();
    int ix = GuidMatcher.indexIn(basename);
    if (ix > 1 && basename.at(ix - 1) == '_')  {
        int dix = ix + 32 + 1;
        if (basename.count() > dix) {
            bool gotPrefix = true;
            for (int i = dix; i < basename.count(); i++) {
                if (!basename.at(i).isDigit()) {
                    gotPrefix = false;
                    break;
                }
            }
            if (gotPrefix) {
                m_prefix = basename.left(ix - 1);
            }
        }
    }

    //DebugDialog::debug(QString("%1, %2").arg(info.absoluteFilePath()).arg(m_userPartsFolderPath));
    m_canSave = info.absoluteFilePath().contains(m_userPartsFolderPath);

    if (!loadFzp(originalModelPart->path())) return false;

    QDomElement fzpRoot = m_fzpDocument.documentElement();
    QString referenceFile = fzpRoot.attribute(ReferenceFileString);
    if (referenceFile.isEmpty()) {
        fzpRoot.setAttribute(ReferenceFileString, info.fileName());
    }
    QDomElement views = fzpRoot.firstChildElement("views");
    QDomElement author = fzpRoot.firstChildElement("author");
    if (author.isNull()) {
        author = m_fzpDocument.createElement("author");
        fzpRoot.appendChild(author);
    }
    if (author.text().isEmpty()) {
        TextUtils::replaceChildText(author, QString(getenvUser()));
    }
    QDomElement date = fzpRoot.firstChildElement("date");
    if (date.isNull()) {
        date = m_fzpDocument.createElement("date");
        fzpRoot.appendChild(date);
    }
    TextUtils::replaceChildText(date, QDate::currentDate().toString());

	QDomElement properties = fzpRoot.firstChildElement("properties");
	if (properties.isNull()) {
		properties = m_fzpDocument.createElement("properties");
		fzpRoot.appendChild(properties);
	}
	QHash<QString,QString> props = originalModelPart->properties();
	foreach (QString key, props.keys()) {
		replaceProperty(key, props.value(key), properties);
	}
	// record "local" properties
	foreach (QByteArray byteArray, originalModelPart->dynamicPropertyNames()) {
		replaceProperty(byteArray, originalModelPart->property(byteArray).toString(), properties);
	}

    bool hasLegID = false;
	QDomElement connectors = fzpRoot.firstChildElement("connectors");
	QDomElement connector = connectors.firstChildElement("connector");
	while (!connector.isNull()) {
		QString localName = originalModelPart->connectorLocalName(connector.attribute("id"));
		if (!localName.isEmpty()) {
			connector.setAttribute("name", localName);
		}
        if (!hasLegID) {
            QDomNodeList plist = connector.elementsByTagName("p");
            for (int i = 0; i < plist.count(); i++) {
                QDomElement p = plist.at(i).toElement();
                if (!p.attribute("legId").isEmpty()) {
                    hasLegID = true;
                    break;
                }
            }
        }
		connector = connector.nextSiblingElement("connector");
	}

    if (hasLegID && !RubberBandLegWarning) {
        RubberBandLegWarning = true;
        QMessageBox::warning(NULL, tr("Parts Editor"), 
                                    tr("This part has bendable legs. ") +
                                    tr("This version of the Parts Editor does not yet support editing bendable legs, and the legs may not be displayed correctly in breadboard view . ") +
                                    tr("If you make changes to breadboard view, or change connector metadata, the legs may no longer work. ") +
                                    tr("You can safely make changes to Schematic or PCB view.\n\n") +
                                    tr("This warning will not be repeated in this session of Fritzing")
                                );
    }



	// for now kill the editable pin labels property, otherwise the saved part will try to use the labels that are only found in the sketch
	QDomElement epl = TextUtils::findElementWithAttribute(properties, "name", "editable pin labels");
	if (!epl.isNull()) {
		TextUtils::replaceChildText(epl, "false");
	}

	QDomElement family = TextUtils::findElementWithAttribute(properties, "name", "family");
	if (family.isNull()) {
		replaceProperty("family", m_guid, properties);
	}
	QDomElement variant = TextUtils::findElementWithAttribute(properties, "name", "variant");
	if (variant.isNull()) {
		QString newVariant = makeNewVariant(family.text());
		replaceProperty("variant", newVariant, properties);
	}

    foreach (ViewThing * viewThing, m_viewThings.values()) {
        if (viewThing->sketchWidget == NULL) continue;

        ItemBase * itemBase = originalModelPart->viewItem(viewThing->sketchWidget->viewID());
        if (itemBase == NULL) continue;

        viewThing->referenceFile = getSvgReferenceFile(itemBase->filename());

        if (!itemBase->hasCustomSVG()) {
            QFile file(itemBase->filename());
            if (!file.open(QFile::ReadOnly)) {
                QMessageBox::critical(NULL, tr("Parts Editor"), tr("Unable to load '%1'. Please close the parts editor without saving and try again.").arg(itemBase->filename()));
                continue;
            }

            QString svg = file.readAll();
            insertDesc(viewThing->referenceFile, svg);
            TextUtils::fixMuch(svg, true);
            QString svgPath = makeSvgPath2(viewThing->sketchWidget);
            bool result = writeXml(m_userPartsFolderSvgPath + svgPath, removeGorn(svg), true);
            if (!result) {
                QMessageBox::critical(NULL, tr("Parts Editor"), tr("Unable to write svg to  %1").arg(svgPath));
		        return false;
            }

            continue;
        }

        QHash<QString, QString> svgHash;
		QStringList svgList;
        double factor;
		foreach (ViewLayer * vl, viewThing->sketchWidget->viewLayers().values()) {
			QString string = itemBase->retrieveSvg(vl->viewLayerID(), svgHash, false, GraphicsUtils::StandardFritzingDPI, factor);
			if (!string.isEmpty()) {
				svgList.append(string);
			}
		}

        if (svgList.count() == 0) {
            DebugDialog::debug(QString("pe: missing custom svg %1").arg(originalModelPart->moduleID()));
            continue;
        }


		QString svg;
		if (svgList.count() == 1) {
			svg = svgList.at(0);
		}
		else {
			// deal with copper0 and copper1 layers as parent/child
            // have to remove whitespace in order to compare the two svgs
			QRegExp white("\\s");
			QStringList whiteList;
			foreach (QString string, svgList) {
				string.remove(white);
				whiteList << string;
			}
			bool keepGoing = true;
			while (keepGoing) {
				keepGoing = false;
				for (int i = 0; i < whiteList.count() - 1; i++)  {
					for (int j = i + 1; j < whiteList.count(); j++) {
						if (whiteList.at(i).contains(whiteList.at(j))) {
							keepGoing = true;
							whiteList.removeAt(j);
							svgList.removeAt(j);
							break;
						}
						if (whiteList.at(j).contains(whiteList.at(i))) {
							keepGoing = true;
							whiteList.removeAt(i);
							svgList.removeAt(i);
							break;
						}
					}
					if (keepGoing) break;
				}
			}
			svg = svgList.join("\n");
		}

		QSizeF size = itemBase->size();
        QString header = TextUtils::makeSVGHeader(GraphicsUtils::SVGDPI, GraphicsUtils::StandardFritzingDPI, size.width(), size.height());
        header += makeDesc(viewThing->referenceFile);
		svg = header + svg + "</svg>";
        QString svgPath = makeSvgPath2(viewThing->sketchWidget);
        bool result = writeXml(m_userPartsFolderSvgPath + svgPath, removeGorn(svg), true);
        if (!result) {
            QMessageBox::critical(NULL, tr("Parts Editor"), tr("Unable to write svg to  %1").arg(svgPath));
		    return false;
        }

        QDomElement view = views.firstChildElement(ViewLayer::viewIDXmlName(viewThing->sketchWidget->viewID()));
        QDomElement layers = view.firstChildElement("layers");
        if (layers.isNull()) {
            QMessageBox::critical(NULL, tr("Parts Editor"), tr("Unable to parse fzp file  %1").arg(originalModelPart->path()));
		    return false;
        }

		setImageAttribute(layers, svgPath);
    }

    reload(true);

    foreach (ViewThing * viewThing, m_viewThings.values()) {
        viewThing->originalSvgPath = viewThing->itemBase->filename();
        viewThing->svgChangeCount = 0;
    }

    setTitle();
    createRaiseWindowActions();
    return true;
}

void PEMainWindow::initZoom() {
    if (m_peToolView == NULL) return;
    if (m_currentGraphicsView == NULL) return;
	
	ViewThing * viewThing = m_viewThings.value(m_currentGraphicsView->viewID());
	if (viewThing->itemBase == NULL) return;
			
    if (!viewThing->everZoomed) {
        viewThing->everZoomed = true;
        m_currentGraphicsView->fitInWindow();
    }
		
	m_peSvgView->setFilename(viewThing->referenceFile);
}

void PEMainWindow::setTitle() {
    QString title = tr("Fritzing (New) Parts Editor");
    QString partTitle = getPartTitle();

    QString viewName;
    if (m_currentGraphicsView) viewName = m_currentGraphicsView->viewName();
    else if (currentTabIndex() == IconViewIndex) viewName = tr("Icon View");
    else if (currentTabIndex() == MetadataViewIndex) viewName = tr("Metadata View");
    else if (currentTabIndex() == ConnectorsViewIndex) viewName = tr("Connectors View");

	setWindowTitle(QString("%1: %2 [%3]%4").arg(title).arg(partTitle).arg(viewName).arg(QtFunkyPlaceholder));
}

void PEMainWindow::createViewMenuActions(bool showWelcome) {
    MainWindow::createViewMenuActions(showWelcome);

	m_showIconAct = new QAction(tr("Show Icon"), this);
	m_showIconAct->setShortcut(tr("Ctrl+4"));
	m_showIconAct->setStatusTip(tr("Show the icon view"));
	connect(m_showIconAct, SIGNAL(triggered()), this, SLOT(showIconView()));

	m_showMetadataViewAct = new QAction(tr("Show Metadata"), this);
	m_showMetadataViewAct->setShortcut(tr("Ctrl+5"));
	m_showMetadataViewAct->setStatusTip(tr("Show the metadata view"));
	connect(m_showMetadataViewAct, SIGNAL(triggered()), this, SLOT(showMetadataView()));

    m_showConnectorsViewAct = new QAction(tr("Show Connectors"), this);
	m_showConnectorsViewAct->setShortcut(tr("Ctrl+6"));
	m_showConnectorsViewAct->setStatusTip(tr("Show the connector metadata in a list view"));
	connect(m_showConnectorsViewAct, SIGNAL(triggered()), this, SLOT(showConnectorsView()));

    m_hideOtherViewsAct = new QAction(tr("Make only this view visible"), this);
	m_hideOtherViewsAct->setStatusTip(tr("The part will only be visible in this view and icon view"));
	connect(m_hideOtherViewsAct, SIGNAL(triggered()), this, SLOT(hideOtherViews()));

}
 
void PEMainWindow::createViewMenu() {
    MainWindow::createViewMenu();

    bool afterNext = false;
    foreach (QAction * action, m_viewMenu->actions()) {
        if (action == m_setBackgroundColorAct) {
			afterNext = true;
		}
		else if (afterNext) {
            m_viewMenu->insertSeparator(action);
            m_viewMenu->insertAction(action, m_hideOtherViewsAct);
            break;
        }
    }
	
    afterNext = false;
    foreach (QAction * action, m_viewMenu->actions()) {
        if (action == m_showPCBAct) {
            afterNext = true;
        }
        else if (afterNext) {
            m_viewMenu->insertAction(action, m_showIconAct);
            m_viewMenu->insertAction(action, m_showMetadataViewAct);
            m_viewMenu->insertAction(action, m_showConnectorsViewAct);
            break;
        }
    }
    m_numFixedActionsInViewMenu = m_viewMenu->actions().size();
}

void PEMainWindow::showMetadataView() {
    setCurrentTabIndex(MetadataViewIndex);
}

void PEMainWindow::showConnectorsView() {
    setCurrentTabIndex(ConnectorsViewIndex);
}

void PEMainWindow::showIconView() {
    setCurrentTabIndex(IconViewIndex);
}

void PEMainWindow::changeSpecialProperty(const QString & name, const QString & value)
{
    QHash<QString, QString> oldProperties = getOldProperties();

	if (value.isEmpty()) {
		QMessageBox::warning(NULL, tr("Blank not allowed"), tr("The value of '%1' can not be blank.").arg(name));
		m_metadataView->resetProperty(name, value);
		return;
	}

	if (oldProperties.value(name) == value) {
		return;
	}

    QHash<QString, QString> newProperties(oldProperties);
    newProperties.insert(name, value);
    
    ChangePropertiesCommand * cpc = new ChangePropertiesCommand(this, oldProperties, newProperties, NULL);
    cpc->setText(tr("Change %1 to %2").arg(name).arg(value));
    cpc->setSkipFirstRedo();
    changeProperties(newProperties, false);
    m_undoStack->waitPush(cpc, SketchWidget::PropChangeDelay);
}

void PEMainWindow::metadataChanged(const QString & name, const QString & value)
{
	qDebug() << "metadata changed";

	if (name.compare("family") == 0) {
		changeSpecialProperty(name, value);
        return;
    }

    if (name.compare("variant") == 0) {
		QString family = m_metadataView->family();
		QHash<QString, QString> variants = m_referenceModel->allPropValues(family, "variant");
		QStringList values = variants.values(value);
		if (m_canSave) {
			QString moduleID = m_fzpDocument.documentElement().attribute("moduleId");
			values.removeOne(moduleID);
		}
		if (values.count() > 0) {
			QMessageBox::warning(NULL, tr("Must be unique"), tr("Variant '%1' is in use. The variant name must be unique.").arg(value));
			return;
		}

		changeSpecialProperty(name, value);
        return;
    }

    QString menuText = (name.compare("description") == 0) ? tr("Change description") : tr("Change %1 to '%2'").arg(name).arg(value);

    // called from metadataView
    QDomElement root = m_fzpDocument.documentElement();
    QDomElement element = root.firstChildElement(name);
    QString oldValue = element.text();
	if (oldValue == value) return;

    ChangeMetadataCommand * cmc = new ChangeMetadataCommand(this, name, oldValue, value, NULL);
    cmc->setText(menuText);
    cmc->setSkipFirstRedo();
    changeMetadata(name, value, false);
    m_undoStack->waitPush(cmc, SketchWidget::PropChangeDelay);
}

void PEMainWindow::changeMetadata(const QString & name, const QString & value, bool updateDisplay)
{
    // called from command object
    QDomElement root = m_fzpDocument.documentElement();
    TextUtils::replaceElementChildText(root, name, value);

    //QString test = m_fzpDocument.toString();

    if (updateDisplay) {
        m_metadataView->initMetadata(m_fzpDocument);
    }
}

void PEMainWindow::tagsChanged(const QStringList & newTags)
{

    // called from metadataView
    QDomElement root = m_fzpDocument.documentElement();
    QDomElement tags = root.firstChildElement("tags");
    QDomElement tag = tags.firstChildElement("tag");
    QStringList oldTags;
    while (!tag.isNull()) {
        oldTags << tag.text();
        tag = tag.nextSiblingElement("tag");
    }

    ChangeTagsCommand * ctc = new ChangeTagsCommand(this, oldTags, newTags, NULL);
    ctc->setText(tr("Change tags"));
    ctc->setSkipFirstRedo();
    changeTags(newTags, false);
    m_undoStack->waitPush(ctc, SketchWidget::PropChangeDelay);
}

void PEMainWindow::changeTags(const QStringList & newTags, bool updateDisplay)
{
    QDomElement root = m_fzpDocument.documentElement();
    QDomElement tags = root.firstChildElement("tags");
    QDomElement tag = tags.firstChildElement("tag");
    while (!tag.isNull()) {
        tags.removeChild(tag);
        tag = tags.firstChildElement("tag");
    }

    foreach (QString newTag, newTags) {
        QDomElement tag = m_fzpDocument.createElement("tag");
        tags.appendChild(tag);
        TextUtils::replaceChildText(tag, newTag);
    }

    if (updateDisplay) {
        m_metadataView->initMetadata(m_fzpDocument);
    }
}

void PEMainWindow::propertiesChanged(const QHash<QString, QString> & newProperties)
{	
	qDebug() << "properties changed";

	QStringList keys = newProperties.keys();
	if (keys.contains("family", Qt::CaseInsensitive)) {
		QMessageBox::warning(NULL, tr("Duplicate problem"), tr("Duplicate 'family' property not allowed"));
		return;
	}

	if (keys.contains("variant", Qt::CaseInsensitive)) {
		QMessageBox::warning(NULL, tr("Duplicate problem"), tr("Duplicate 'variant' property not allowed"));
		return;
	}

    // called from metadataView
    QHash<QString, QString> oldProperties = getOldProperties();

    ChangePropertiesCommand * cpc = new ChangePropertiesCommand(this, oldProperties, newProperties, NULL);
    cpc->setText(tr("Change properties"));
    cpc->setSkipFirstRedo();
    changeProperties(newProperties, false);
    m_undoStack->waitPush(cpc, SketchWidget::PropChangeDelay);
}


void PEMainWindow::changeProperties(const QHash<QString, QString> & newProperties, bool updateDisplay)
{
    bool incomingFamily = newProperties.keys().contains("family");
    bool incomingVariant = newProperties.keys().contains("variant");

    QDomElement root = m_fzpDocument.documentElement();
    QDomElement properties = root.firstChildElement("properties");
    QDomElement prop = properties.firstChildElement("property");
    while (!prop.isNull()) {
		QDomElement next = prop.nextSiblingElement("property");
        bool doRemove = true;
        if (prop.attribute("name") == "family" && !incomingFamily) doRemove = false;
        if (prop.attribute("name") == "variant" && !incomingVariant) doRemove = false;
        if (doRemove) {
            properties.removeChild(prop);
        }
        prop = next;
    }

    foreach (QString name, newProperties.keys()) {
        QDomElement prop = m_fzpDocument.createElement("property");
        properties.appendChild(prop);
        prop.setAttribute("name", name);
        TextUtils::replaceChildText(prop, newProperties.value(name));
    }

    if (updateDisplay) {
        m_metadataView->initMetadata(m_fzpDocument);
    }
}

QHash<QString, QString> PEMainWindow::getOldProperties() 
{
    QDomElement root = m_fzpDocument.documentElement();
    QDomElement properties = root.firstChildElement("properties");
    QDomElement prop = properties.firstChildElement("property");
    QHash<QString, QString> oldProperties;
    while (!prop.isNull()) {
        QString name = prop.attribute("name");
        QString value = prop.text();
        oldProperties.insert(name, value);
        prop = prop.nextSiblingElement("property");
    }

    return oldProperties;
}

void PEMainWindow::connectorMetadataChanged(ConnectorMetadata * cmd)
{
    int index;
    QDomElement connector = findConnector(cmd->connectorID, index);
    if (connector.isNull()) return;

    ConnectorMetadata oldcmd;
	fillInMetadata(connector, oldcmd);

    ChangeConnectorMetadataCommand * ccmc = new ChangeConnectorMetadataCommand(this, &oldcmd, cmd, NULL);
    ccmc->setText(tr("Change connector %1").arg(cmd->connectorName));
    bool skipFirstRedo = (sender() == m_connectorsView);
    if (skipFirstRedo) {
        ccmc->setSkipFirstRedo();
        changeConnectorElement(connector, cmd);
        initConnectors(false);
    }
    m_undoStack->waitPush(ccmc, SketchWidget::PropChangeDelay);
}

QDomElement PEMainWindow::findConnector(const QString & id, int & index) 
{
    QDomElement root = m_fzpDocument.documentElement();
    QDomElement connectors = root.firstChildElement("connectors");
    QDomElement connector = connectors.firstChildElement("connector");
    index = 0;
    while (!connector.isNull()) {
        if (id.compare(connector.attribute("id")) == 0) {
            return connector;
        }
        connector = connector.nextSiblingElement("connector");
        index++;
    }

    return QDomElement();
}


void PEMainWindow::changeConnectorMetadata(ConnectorMetadata * cmd, bool updateDisplay) {
    int index;
    QDomElement connector = findConnector(cmd->connectorID, index);
    if (connector.isNull()) return;

    changeConnectorElement(connector, cmd);
    if (updateDisplay) {
        initConnectors(true);
        m_peToolView->setCurrentConnector(index);
    }
}

void PEMainWindow::changeConnectorElement(QDomElement & connector, ConnectorMetadata * cmd)
{
    connector.setAttribute("name", cmd->connectorName);
    connector.setAttribute("id", cmd->connectorID);
    connector.setAttribute("type", Connector::connectorNameFromType(cmd->connectorType));
    TextUtils::replaceElementChildText(connector, "description", cmd->connectorDescription);

#ifndef QT_NO_DEBUG
    QDomElement description = connector.firstChildElement("description");
    QString text = description.text();
    DebugDialog::debug(QString("description %1").arg(text));
#endif

}

void PEMainWindow::initSvgTree(SketchWidget * sketchWidget, ItemBase * itemBase, QDomDocument & svgDocument) 
{
    QString errorStr;
    int errorLine;
    int errorColumn;

    QDomDocument tempSvgDoc;
    QFile file(itemBase->filename());
    if (!tempSvgDoc.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		DebugDialog::debug(QString("unable to parse svg: %1 %2 %3").arg(errorStr).arg(errorLine).arg(errorColumn));
        return;
	}

	if (itemBase->viewID() == ViewLayer::PCBView) {
		QDomElement root = tempSvgDoc.documentElement();
		QDomElement copper0 = TextUtils::findElementWithAttribute(root, "id", "copper0");
		QDomElement copper1 = TextUtils::findElementWithAttribute(root, "id", "copper1");
		if (!copper0.isNull() && !copper1.isNull()) {
			QDomElement parent0 = copper0.parentNode().toElement();
			QDomElement parent1 = copper1.parentNode().toElement();
			if (parent0.attribute("id") == "copper1") ;
			else if (parent1.attribute("id") == "copper0") ;
			else {
    			QMessageBox::warning(NULL, tr("SVG problem"), 
					tr("This version of the new Parts Editor can not deal with separate copper0 and copper1 layers in '%1'. ").arg(itemBase->filename()) +
					tr("So editing may produce an invalid PCB view image"));
			}
		}
	}

    int z = PegiZ;

    FSvgRenderer tempRenderer;
    QByteArray rendered = tempRenderer.loadSvg(tempSvgDoc.toByteArray(), "", false);
    // cleans up the svg
    if (!svgDocument.setContent(rendered, true, &errorStr, &errorLine, &errorColumn)) {
		DebugDialog::debug(QString("unable to parse svg (2): %1 %2 %3").arg(errorStr).arg(errorLine).arg(errorColumn));
        return;
	}

    TextUtils::gornTree(svgDocument);  
    FSvgRenderer renderer;
    renderer.loadSvg(svgDocument.toByteArray(), "", false);
	
	QHash<QString, PEGraphicsItem *> pegiHash;

    QList<QDomElement> traverse;
    traverse << svgDocument.documentElement();
    while (traverse.count() > 0) {
        QDomElement element = traverse.takeFirst();

        // depth first
        QList<QDomElement> next;
        QDomElement child = element.firstChildElement();
        while (!child.isNull()) {
            next << child;
            child = child.nextSiblingElement();
        }
        while (next.count() > 0) {
            traverse.push_front(next.takeLast());
        }

        bool isG = false;
        bool isSvg = false;
        QString tagName = element.tagName();
        if      (tagName.compare("rect") == 0);
        else if (tagName.compare("g") == 0) {
            isG = true;
        }
        else if (tagName.compare("svg") == 0) {
            isSvg = true;
        }
        else if (tagName.compare("circle") == 0);
        else if (tagName.compare("ellipse") == 0);
        else if (tagName.compare("path") == 0);
        else if (tagName.compare("line") == 0);
        else if (tagName.compare("polyline") == 0);
        else if (tagName.compare("polygon") == 0);
        else if (tagName.compare("text") == 0);
        else continue;

        QRectF bounds = getPixelBounds(renderer, element);
        // known Qt bug: boundsOnElement returns zero width and height for text elements.
        if (bounds.width() > 0 && bounds.height() > 0) {
            PEGraphicsItem * pegi = makePegi(bounds.size(), bounds.topLeft(), itemBase, element, z++);
			pegiHash.insert(element.attribute("id"), pegi);

            /* 
            QString string;
            QTextStream stream(&string);
            element.save(stream, 0);
            DebugDialog::debug("........");
            DebugDialog::debug(string);
            */
        }
    }

    QDomElement root = m_fzpDocument.documentElement();
    QDomElement connectors = root.firstChildElement("connectors");
    QDomElement connector = connectors.firstChildElement("connector");
    while (!connector.isNull()) {
		QString svgID, terminalID;
        bool ok = ViewLayer::getConnectorSvgIDs(connector, sketchWidget->viewID(), svgID, terminalID);
		if (ok) {
			PEGraphicsItem * terminalItem = pegiHash.value(terminalID, NULL);
			PEGraphicsItem * pegi = pegiHash.value(svgID);
			if (pegi == NULL) {
				DebugDialog::debug(QString("missing pegi for svg id %1").arg(svgID)); 
			}
			else {
				QPointF terminalPoint = pegi->rect().center();
				if (terminalItem) {
					terminalPoint = terminalItem->pos() - pegi->pos() + terminalItem->rect().center();
				}
				pegi->setTerminalPoint(terminalPoint);
				pegi->showTerminalPoint(true);
			}
		}

        connector = connector.nextSiblingElement("connector");
    }
}

void PEMainWindow::highlightSlot(PEGraphicsItem * pegi) {
    if (m_peToolView) {
        bool enableTerminalPointControls = anyMarquee();
        bool vis = anyVisible();
        m_peToolView->enableConnectorChanges(vis && pegi->showingMarquee(), vis && enableTerminalPointControls, m_connectorList.count() > 0, vis);
    }

    if (m_peSvgView) {
        m_peSvgView->highlightElement(pegi);
    }
}

void PEMainWindow::initConnectors(bool updateConnectorsView) {
    QDomElement root = m_fzpDocument.documentElement();
    QDomElement connectors = root.firstChildElement("connectors");
    QDomElement connector = connectors.firstChildElement("connector");
    m_connectorList.clear();
    while (!connector.isNull()) {
        m_connectorList.append(connector);
        connector = connector.nextSiblingElement("connector");
    }

    qSort(m_connectorList.begin(), m_connectorList.end(), byID);

    if (updateConnectorsView) {
        m_connectorsView->initConnectors(&m_connectorList);
    }
    m_peToolView->initConnectors(&m_connectorList);
	updateAssignedConnectors();
}

void PEMainWindow::switchedConnector(int ix)
{
    if (m_currentGraphicsView == NULL) return;

    switchedConnector(ix, m_currentGraphicsView);
}

void PEMainWindow::switchedConnector(int ix, SketchWidget * sketchWidget)
{
	if (m_connectorList.count() == 0) return;

	QDomElement element = m_connectorList.at(ix);
    if (element.isNull()) return;

	QString svgID, terminalID;
    if (!ViewLayer::getConnectorSvgIDs(element, sketchWidget->viewID(), svgID, terminalID)) return;

    QList<PEGraphicsItem *> pegiList = getPegiList(sketchWidget);
    bool gotOne = false;
    foreach (PEGraphicsItem * pegi, pegiList) {
        QDomElement pegiElement = pegi->element();
        if (pegiElement.attribute("id").compare(svgID) == 0) {
            gotOne = true;
            pegi->showMarquee(true);
            pegi->setHighlighted(true);
			m_peToolView->setTerminalPointCoords(pegi->terminalPoint());
			m_peToolView->setTerminalPointLimits(pegi->rect().size());
            break;
        }
    }

    if (!gotOne) {
        foreach (PEGraphicsItem * pegi, pegiList) {
            pegi->showMarquee(false);
            pegi->setHighlighted(false);        
        }
    }
}

void PEMainWindow::loadImage() 
{
    if (m_currentGraphicsView == NULL) return;

	QStringList extras;
	extras.append("");
	extras.append("");
	QString imageFiles;
	if (m_currentGraphicsView->viewID() == ViewLayer::PCBView) {
		imageFiles = tr("Image & Footprint Files (%1 %2 %3 %4 %5);;SVG Files (%1);;JPEG Files (%2);;PNG Files (%3);;gEDA Footprint Files (%4);;Kicad Module Files (%5)");   // 
		extras[0] = "*.fp";
		extras[1] = "*.mod";
	}
	else {
		imageFiles = tr("Image Files (%1 %2 %3);;SVG Files (%1);;JPEG Files (%2);;PNG Files (%3)%4%5");
	}

    /*
	if (m_currentGraphicsView->viewID() == ViewLayer::SchematicView) {
		extras[0] = "*.lib";
		imageFiles = tr("Image & Footprint Files (%1 %2 %3 %4);;SVG Files (%1);;JPEG Files (%2);;PNG Files (%3);;Kicad Schematic Files (%4)%5");   // 
	}
    */

    QString initialPath = FolderUtils::openSaveFolder();
	ViewThing * viewThing = m_viewThings.value(m_currentGraphicsView->viewID());
    ItemBase * itemBase = viewThing->itemBase;
    if (itemBase) {
        initialPath = itemBase->filename();
    }
	QString origPath = FolderUtils::getOpenFileName(this,
		tr("Open Image"),
		initialPath,
		imageFiles.arg("*.svg").arg("*.jpg *.jpeg").arg("*.png").arg(extras[0]).arg(extras[1])
	);

	if (origPath.isEmpty()) {
		return; // Cancel pressed
	} 

    QString newReferenceFile;
    QString svg; 
	if (origPath.endsWith(".svg")) {
        newReferenceFile = getSvgReferenceFile(origPath);
		QFile origFile(origPath);
		if (!origFile.open(QFile::ReadOnly)) {
    		QMessageBox::warning(NULL, tr("Conversion problem"), tr("Unable to load '%1'").arg(origPath));
			return;
		}

		svg = origFile.readAll();
		origFile.close();
		if (svg.contains("coreldraw", Qt::CaseInsensitive) && svg.contains("cdata", Qt::CaseInsensitive)) {
    		QMessageBox::warning(NULL, tr("Conversion problem"), 
				tr("The SVG file '%1' appears to have been exported from CorelDRAW without the 'presentation attributes' setting. ").arg(origPath) +
				tr("Please re-export the SVG file using that setting, and try loading again.")
			);
			return;
		}

		QStringList availFonts = InstalledFonts::InstalledFontsList.toList();
		if (availFonts.count() > 0) {
			QString destFont = availFonts.at(0);
			foreach (QString f, availFonts) {
				if (f.contains("droid", Qt::CaseInsensitive)) {
					destFont = f;
					break;
				}
			}
            bool reallyFixed = false;
            TextUtils::fixFonts(svg, destFont, reallyFixed);
			if (reallyFixed) {
    			QMessageBox::information(NULL, tr("Fonts"), 
					tr("Fritzing currently only supports OCRA and Droid fonts--these have been substituted in for the fonts in '%1'").arg(origPath));
			}
		}	
    }
    else {
        newReferenceFile = origPath;
        if (origPath.endsWith("png") || origPath.endsWith("jpg") || origPath.endsWith("jpeg")) {
                QString message = tr("You may use a PNG or JPG image to construct your part, but it is better to use an SVG. ") +
                                tr("PNG and JPG images retain their nature as bitmaps and do not look good when scaled--") +                        
                                tr("so for Fritzing parts it is best to use PNG and JPG only as placeholders.")                       
                 ;
                
    		    QMessageBox::information(NULL, tr("Use of PNG and JPG discouraged"), message);

        }

		try {
			svg = createSvgFromImage(origPath);
		}
		catch (const QString & msg) {
    		QMessageBox::warning(NULL, tr("Conversion problem"), tr("Unable to load image file '%1': \n\n%2").arg(origPath).arg(msg));
			return;
		}
	}

    if (svg.isEmpty()) {
        QMessageBox::warning(NULL, tr("Conversion problem"), tr("Unable to load image file '%1'").arg(origPath));
        return;
    }

	QString errorStr;
	int errorLine;
	int errorColumn;
    QDomDocument doc;
	bool result = doc.setContent(svg.toUtf8(), &errorStr, &errorLine, &errorColumn);
    if (!result) {
    	QMessageBox::warning(NULL, tr("SVG problem"), tr("Unable to parse '%1': %2 line:%3 column:%4").arg(origPath).arg(errorStr).arg(errorLine).arg(errorColumn));
        return;
    }

    if (m_currentGraphicsView == m_pcbGraphicsView) {
        QDomElement root = doc.documentElement();
        QDomElement check = TextUtils::findElementWithAttribute(root, "id", "copper1");
        if (check.isNull()) {
            check = TextUtils::findElementWithAttribute(root, "id", "copper0");
        }
        if (check.isNull()) {
            QString message = tr("There are no copper layers defined in: %1. ").arg(origPath) +
                            tr("See <a href=\"http://fritzing.org/learning/tutorials/creating-custom-parts/providing-part-graphics/\">this explanation</a>.") +
                            tr("<br/><br/>This will not be a problem in the next release of the Parts Editor, ") +
                            tr("but for now please modify the file according to the instructions in the link.")                         
                ;
                
    		QMessageBox::warning(NULL, tr("SVG problem"), message);
            return;
        }
    }

    TextUtils::fixMuch(svg, true);
    insertDesc(newReferenceFile, svg);
    QString newPath = m_userPartsFolderSvgPath + makeSvgPath2(m_currentGraphicsView);
    bool success = writeXml(newPath, removeGorn(svg), true);
    if (!success) {
    	QMessageBox::warning(NULL, tr("Copy problem"), tr("Unable to make a local copy of: '%1'").arg(origPath));
		return;
    }

    QFileInfo info(origPath);
    QUndoCommand * parentCommand = new QUndoCommand(QString("Load '%1'").arg(info.fileName()));
	new ChangeSvgCommand(this, m_currentGraphicsView, itemBase->filename(), newPath, parentCommand);
    m_undoStack->waitPush(parentCommand, SketchWidget::PropChangeDelay);
}

QString PEMainWindow::createSvgFromImage(const QString &origFilePath) {
	if (origFilePath.endsWith(".fp")) {
		// this is a geda footprint file
		GedaElement2Svg geda;
		QString svg = geda.convert(origFilePath, false);
		return svg;
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
		return svg;
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
		return svg;
	}

	// deal with png, jpg:
    QBuffer buffer;
	QImage img(origFilePath);
	QSvgGenerator svgGenerator;
	svgGenerator.setResolution(90);
	svgGenerator.setOutputDevice(&buffer);
	QSize sz = img.size();
    svgGenerator.setSize(sz);
	svgGenerator.setViewBox(QRect(0, 0, sz.width(), sz.height()));
	QPainter svgPainter(&svgGenerator);
	svgPainter.drawImage(QPoint(0,0), img);
	svgPainter.end();

	return QString(buffer.buffer());
}

/*
QString PEMainWindow::makeSvgPath(const QString & referenceFile, SketchWidget * sketchWidget, bool useIndex)
{
    QString rf = referenceFile;
    if (!rf.isEmpty()) rf += "_";
    QString viewName = ViewLayer::viewIDNaturalName(sketchWidget->viewID());
    QString indexString;
    if (useIndex) indexString = QString("_%3").arg(m_fileIndex++);
    return QString("%1/%2%3_%1%4.svg").arg(viewName).arg(rf).arg(m_guid).arg(indexString);
}
*/

QString PEMainWindow::makeSvgPath2(SketchWidget * sketchWidget)
{
    QString viewName = ViewLayer::viewIDNaturalName(sketchWidget->viewID());
    return QString("%1/%2_%3_%4_%1.svg").arg(viewName).arg(m_prefix).arg(m_guid).arg(m_fileIndex);
}

void PEMainWindow::changeSvg(SketchWidget * sketchWidget, const QString & filename, int changeDirection) {
    QDomElement fzpRoot = m_fzpDocument.documentElement();
    setImageAttribute(fzpRoot, filename, sketchWidget->viewID());

    foreach (ViewThing * viewThing, m_viewThings.values()) {
        foreach(ItemBase * lk, viewThing->itemBase->layerKin()) {
            delete lk;
        }
        delete viewThing->itemBase;
		viewThing->itemBase = NULL;
    }

    updateChangeCount(sketchWidget, changeDirection);

    reload(false);
}

QString PEMainWindow::makeDirName() {
    return "fritzing_pe_" + m_guid;
}

QString PEMainWindow::saveFzp() {
    QDir dir = QDir::temp();
    QString dirName = makeDirName();
    dir.mkdir(dirName);
    dir.cd(dirName);
    QString fzpPath = dir.absoluteFilePath(QString("%1_%2_%3.fzp").arg(m_prefix).arg(m_guid).arg(m_fileIndex++));   
    DebugDialog::debug("temp path " + fzpPath);
    writeXml(fzpPath, m_fzpDocument.toString(), true);
    return fzpPath;
}

void PEMainWindow::reload(bool firstTime) 
{
	Q_UNUSED(firstTime);

	CursorMaster::instance()->addCursor(this, Qt::WaitCursor);

	QList<ItemBase *> toDelete;

	foreach (ViewThing * viewThing, m_viewThings.values()) {
        if (viewThing->sketchWidget == NULL) continue;

		foreach (QGraphicsItem * item, viewThing->sketchWidget->scene()->items()) {
			ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
			if (itemBase) toDelete << itemBase;
		}
	}

	foreach (ItemBase * itemBase, toDelete) {
		delete itemBase;
	}

    killPegi();

    QString fzpPath = saveFzp();   // needs a document somewhere to set up connectors--not part of the undo stack
    ModelPart * modelPart = new ModelPart(m_fzpDocument, fzpPath, ModelPart::Part);

    long newID = ItemBase::getNextID();
	ViewGeometry viewGeometry;
	viewGeometry.setLoc(QPointF(0, 0));

	QList<ItemBase *> itemBases;
    itemBases << m_iconGraphicsView->addItem(modelPart, m_iconGraphicsView->defaultViewLayerPlacement(modelPart), BaseCommand::SingleView, viewGeometry, newID, -1, NULL);
    itemBases <<  m_breadboardGraphicsView->addItem(modelPart, m_breadboardGraphicsView->defaultViewLayerPlacement(modelPart), BaseCommand::SingleView, viewGeometry, newID, -1, NULL);
    itemBases <<  m_schematicGraphicsView->addItem(modelPart, m_schematicGraphicsView->defaultViewLayerPlacement(modelPart), BaseCommand::SingleView, viewGeometry, newID, -1, NULL);
    itemBases <<  m_pcbGraphicsView->addItem(modelPart, m_pcbGraphicsView->defaultViewLayerPlacement(modelPart), BaseCommand::SingleView, viewGeometry, newID, -1, NULL);
  
	foreach (ItemBase * itemBase, itemBases) {
		ViewThing * viewThing = m_viewThings.value(itemBase->viewID());
		viewThing->itemBase = itemBase;
        viewThing->referenceFile = getSvgReferenceFile(itemBase->filename());
	}

	m_metadataView->initMetadata(m_fzpDocument);

	QList<QWidget *> widgets;
	widgets << m_metadataView << m_peToolView << m_connectorsView;
	foreach (QWidget * widget, widgets) {
		QList<QLineEdit *> lineEdits = widget->findChildren<QLineEdit *>();
		foreach (QLineEdit * lineEdit, lineEdits) {
			lineEdit->installEventFilter(this);
		}
		QList<QTextEdit *> textEdits = widget->findChildren<QTextEdit *>();
		foreach (QTextEdit * textEdit, textEdits) {
			textEdit->installEventFilter(this);
		}
	}

	if (m_currentGraphicsView) {
		showing(m_currentGraphicsView);
	}

	foreach (ViewThing * viewThing, m_viewThings.values()) {
		initSvgTree(viewThing->sketchWidget, viewThing->itemBase, *viewThing->document);
	}

    initConnectors(true);
	m_connectorsView->setSMD(modelPart->flippedSMD());

    foreach (ViewThing * viewThing, m_viewThings.values()) {
        // TODO: may have to revisit this and move all pegi items
        //viewThing->itemBase->setMoveLock(true);
		//viewThing->itemBase->setItemIsSelectable(false);
		viewThing->itemBase->setAcceptsMousePressLegEvent(false);
        viewThing->itemBase->setSwappable(false);
        viewThing->sketchWidget->hideConnectors(true);
        viewThing->everZoomed = false;
   }

    switchedConnector(m_peToolView->currentConnectorIndex());

    // processEventBlocker might be enough?
    QTimer::singleShot(10, this, SLOT(initZoom()));

	CursorMaster::instance()->removeCursor(this);

}

void PEMainWindow::busModeChanged(bool state) {  
	if (m_currentGraphicsView == NULL) return;

	if (!state) {
        foreach (ViewThing * viewThing, m_viewThings.values()) {
            viewThing->busMode = false;
            viewThing->sketchWidget->hideConnectors(true);
        }

		deleteBuses();

        reload(false);
		return;
	}

	QList<PEGraphicsItem *> pegiList = getPegiList(m_currentGraphicsView);

	reload(false);

	QDomElement root = m_fzpDocument.documentElement();
	QDomElement connectors = root.firstChildElement("connectors");

    foreach (ViewThing * viewThing, m_viewThings.values()) {
	    // items on pegiList no longer exist after reload so get them now
	    pegiList = getPegiList(viewThing->sketchWidget);
	    viewThing->sketchWidget->hideConnectors(true);
	    foreach (PEGraphicsItem * pegi, pegiList) {
		    pegi->setVisible(false);
	    }

	    // show connectorItems that have connectors assigned
	    QStringList connectorIDs;
	    QDomElement connector = connectors.firstChildElement("connector");
	    while (!connector.isNull()) {
		    QDomElement p = ViewLayer::getConnectorPElement(connector, viewThing->sketchWidget->viewID());
		    QString id = p.attribute("svgId");
		    foreach (PEGraphicsItem * pegi, pegiList) {
			    QDomElement pegiElement = pegi->element();
			    if (pegiElement.attribute("id").compare(id) == 0) {
				    connectorIDs.append(connector.attribute("id"));
				    break;
			    }
		    }
		    connector = connector.nextSiblingElement("connector");
	    }	

	    if (connectorIDs.count() > 0) {
		    viewThing->itemBase->showConnectors(connectorIDs);
	    }
    }

	displayBuses();
	m_peToolView->enableConnectorChanges(false, false, m_connectorList.count() > 0, false);
}

void PEMainWindow::pickModeChanged(bool state) {  
	if (m_currentGraphicsView == NULL) return;

	// never actually get state == false;
	m_inPickMode = state;
	if (m_inPickMode) {		
		QApplication::setOverrideCursor(Qt::PointingHandCursor);
		foreach (QGraphicsItem * item, m_currentGraphicsView->scene()->items()) {
			PEGraphicsItem * pegi = dynamic_cast<PEGraphicsItem *>(item);
			if (pegi) pegi->setPickAppearance(true);
		}
		qApp->installEventFilter(this);
	}
}

void PEMainWindow::pegiTerminalPointMoved(PEGraphicsItem * pegi, QPointF p)
{
    // called while terminal point is being dragged, no need for an undo operation

    Q_UNUSED(pegi);
    m_peToolView->setTerminalPointCoords(p);
}

void PEMainWindow::pegiTerminalPointChanged(PEGraphicsItem * pegi, QPointF before, QPointF after)
{
    // called at the end of terminal point dragging

	terminalPointChangedAux(pegi, before, after);
}

void PEMainWindow::pegiMousePressed(PEGraphicsItem * pegi, bool & ignore)
{
    ignore = false;

	if (m_useNextPick) {
		m_useNextPick = false;
		relocateConnector(pegi);
		return;
	}

    ViewThing * viewThing = m_viewThings.value(m_currentGraphicsView->viewID());
    if (pegi->itemBase() != viewThing->itemBase) {
        ignore = true;
        return;
    }

	QString id = pegi->element().attribute("id");
	if (id.isEmpty()) return;

	QDomElement current = m_connectorList.at(m_peToolView->currentConnectorIndex());
	if (current.attribute("id").compare(id) == 0) {
		// already there
		return;
	}

	// if a connector has been clicked, make it the current connector
	QDomElement root = m_fzpDocument.documentElement();
	QDomElement connectors = root.firstChildElement("connectors");
	QDomElement connector = connectors.firstChildElement("connector");
	while (!connector.isNull()) {
		QDomElement p = ViewLayer::getConnectorPElement(connector, m_currentGraphicsView->viewID());
		if (p.attribute("svgId") == id || p.attribute("terminalId") == id) {
			m_peToolView->setCurrentConnector(connector);
			return;
		}
		connector = connector.nextSiblingElement("connector");
	}
}

void PEMainWindow::pegiMouseReleased(PEGraphicsItem *) {
}

void PEMainWindow::relocateConnector(PEGraphicsItem * pegi)
{
    QString newGorn = pegi->element().attribute("gorn");
    QDomDocument * svgDoc = m_viewThings.value(m_currentGraphicsView->viewID())->document;
    QDomElement svgRoot = svgDoc->documentElement();
    QDomElement newGornElement = TextUtils::findElementWithAttribute(svgRoot, "gorn", newGorn);
    if (newGornElement.isNull()) {
        return;
    }

	QDomElement fzpRoot = m_fzpDocument.documentElement();
	QDomElement connectors = fzpRoot.firstChildElement("connectors");
	QDomElement currentConnectorElement = m_connectorList.at(m_peToolView->currentConnectorIndex());
    QString svgID, terminalID;
    if (!ViewLayer::getConnectorSvgIDs(currentConnectorElement, m_currentGraphicsView->viewID(), svgID, terminalID)) {
        return;
    }

    QDomElement oldGornElement = TextUtils::findElementWithAttribute(svgRoot, "id", svgID);
    QString oldGorn = oldGornElement.attribute("gorn");
    QString oldGornTerminal;
    if (!terminalID.isEmpty()) {
        QDomElement element = TextUtils::findElementWithAttribute(svgRoot, "id", terminalID);
        oldGornTerminal = element.attribute("gorn");
    }

    if (newGornElement.attribute("gorn").compare(oldGornElement.attribute("gorn")) == 0) {
        // no change
        return;
    }

    RelocateConnectorSvgCommand * rcsc = new RelocateConnectorSvgCommand(this, m_currentGraphicsView, svgID, terminalID, oldGorn, oldGornTerminal, newGorn, "", NULL);
    rcsc->setText(tr("Relocate connector %1").arg(currentConnectorElement.attribute("name")));
    m_undoStack->waitPush(rcsc, SketchWidget::PropChangeDelay);
}

void PEMainWindow::relocateConnectorSvg(SketchWidget * sketchWidget, const QString & svgID, const QString & terminalID,
                const QString & oldGorn, const QString & oldGornTerminal, const QString & newGorn, const QString & newGornTerminal, 
                int changeDirection)
{
    ViewLayer::ViewID viewID = sketchWidget->viewID();
    ViewThing * viewThing = m_viewThings.value(viewID);
    QDomDocument * svgDoc = viewThing->document;
    QDomElement svgRoot = svgDoc->documentElement();

    QDomElement oldGornElement = TextUtils::findElementWithAttribute(svgRoot, "gorn", oldGorn);
    QDomElement oldGornTerminalElement;
    if (!oldGornTerminal.isEmpty()) {
        oldGornTerminalElement = TextUtils::findElementWithAttribute(svgRoot, "gorn", oldGornTerminal);
    }
    QDomElement newGornElement = TextUtils::findElementWithAttribute(svgRoot, "gorn", newGorn);
    QDomElement newGornTerminalElement;
    if (!newGornTerminal.isEmpty()) {
        newGornTerminalElement = TextUtils::findElementWithAttribute(svgRoot, "gorn", newGornTerminal);
    }

    if (!oldGornElement.isNull()) {
        oldGornElement.removeAttribute("id");
        oldGornElement.removeAttribute("oldid");            // remove oldid so removeGorn() call later doesn't restore anything by accident
    }
    if (!oldGornTerminalElement.isNull()) {
        oldGornTerminalElement.removeAttribute("id");
        oldGornTerminalElement.removeAttribute("oldid");
    }
    if (!newGornElement.isNull()) {
        newGornElement.setAttribute("id", svgID);
        newGornElement.removeAttribute("oldid");
    }
    if (!newGornTerminalElement.isNull()) {
        newGornTerminalElement.setAttribute("id", terminalID);
        newGornTerminalElement.removeAttribute("oldid");
    }

	QDomElement fzpRoot = m_fzpDocument.documentElement();
	QDomElement connectors = fzpRoot.firstChildElement("connectors");
    QDomElement connector = connectors.firstChildElement("connector");
    while (!connector.isNull()) {
        QString cSvgID, cTerminalID;
        if (ViewLayer::getConnectorSvgIDs(connector, viewID, cSvgID, cTerminalID)) {
            if (cSvgID == svgID) break;
        }
        connector = connector.nextSiblingElement("connector");
    }
    if (connector.isNull()) return;

    QDomElement p = ViewLayer::getConnectorPElement(connector, viewID);
    if (terminalID.isEmpty()) {
        p.removeAttribute("terminalId");
    }
    else {
        p.setAttribute("terminalId", terminalID);
    }

    // update svg in case there is a subsequent call to reload
	QString newPath = m_userPartsFolderSvgPath + makeSvgPath2(sketchWidget);
	QString svg = TextUtils::svgNSOnly(svgDoc->toString());
    writeXml(newPath, removeGorn(svg), true);
    setImageAttribute(fzpRoot, newPath, viewID);

    foreach (QGraphicsItem * item, sketchWidget->scene()->items()) {
        PEGraphicsItem * pegi = dynamic_cast<PEGraphicsItem *>(item);
        if (pegi == NULL) continue;

        QDomElement element = pegi->element();
        if (element.attribute("gorn").compare(newGorn) == 0) {
            pegi->setHighlighted(true);
			pegi->showMarquee(true);
			pegi->showTerminalPoint(true);
            switchedConnector(m_peToolView->currentConnectorIndex(), sketchWidget);

        }
        else if (element.attribute("gorn").compare(oldGorn) == 0) {
			pegi->showTerminalPoint(false);
            pegi->showMarquee(false);         
        }
    }

	updateAssignedConnectors();
    updateChangeCount(sketchWidget, changeDirection);
}

bool PEMainWindow::save() {
    if (!canSave()) {
        return saveAs(false);
    }

    return saveAs(true);
}

bool PEMainWindow::saveAs() {
    return saveAs(false);
}

bool PEMainWindow::saveAs(bool overWrite)
{
    bool ok = false;
    QString prefix = QInputDialog::getText(
		    this,
		    tr("Filename prefix"),
            tr("<p>Please enter a prefix to help you identify the part files.<br/>"
               "The file names will have the form 'PREFIX_%1'.<br/>"
               "(It is not necessary to change the proposed prefix, since a unique suffix is always added.)</p>").arg(m_guid),
		    QLineEdit::Normal,
		    m_prefix,
		    &ok
	    );
    if (!ok || prefix.isEmpty()) return false;

    if (prefix != m_prefix) overWrite = false;
    m_prefix = prefix;

    QDomElement fzpRoot = m_fzpDocument.documentElement();

    QList<MainWindow *> affectedWindows;
    QList<MainWindow *> allWindows;
    if (overWrite) {
        foreach (QWidget *widget, QApplication::topLevelWidgets()) {
            MainWindow *mainWindow = qobject_cast<MainWindow *>(widget);
		    if (mainWindow == NULL) continue;
			    
		    if (qobject_cast<PEMainWindow *>(mainWindow) != NULL) continue;

			allWindows.append(mainWindow);
            if (mainWindow->usesPart(m_originalModuleID)) {
                affectedWindows.append(mainWindow);
            }
	    }

        if (affectedWindows.count() > 0 && !m_gaveSaveWarning) {
	        QMessageBox messageBox(this);
	        messageBox.setWindowTitle(tr("Sketch Change Warning"));
            QString message;
            if (affectedWindows.count() == 1) {
                message = tr("The open sketch '%1' uses the part you are editing. ").arg(affectedWindows.first()->windowTitle());
                message += tr("Saving this part will make a change to the sketch that cannot be undone.");
            }
            else {
                message =  tr("The open sketches ");
                for (int i = 0; i < affectedWindows.count() - 1; i++) {
                    message += tr("'%1', ").arg(affectedWindows.at(i)->windowTitle());
                }
                message += tr("and '%1' ").arg(affectedWindows.last()->windowTitle());
                message += tr("Saving this part will make a change to these sketches that cannot be undone.");

            }
            message += tr("\n\nGo ahead and save?");

	        messageBox.setText(message);
	        messageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	        messageBox.setDefaultButton(QMessageBox::Cancel);
	        messageBox.setIcon(QMessageBox::Warning);
	        messageBox.setWindowModality(Qt::WindowModal);
	        messageBox.setButtonText(QMessageBox::Ok, tr("Save"));
	        messageBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));
	        QMessageBox::StandardButton answer = (QMessageBox::StandardButton) messageBox.exec();

	        if (answer != QMessageBox::Ok) {
		        return false;
	        }

            m_gaveSaveWarning = true;
        }
    }

    QDomElement views = fzpRoot.firstChildElement("views");

    QHash<ViewLayer::ViewID, QString> svgPaths;

    foreach (ViewLayer::ViewID viewID, m_viewThings.keys()) {
		ViewThing * viewThing = m_viewThings.value(viewID);
        QDomElement view = views.firstChildElement(ViewLayer::viewIDXmlName(viewID));
        QDomElement layers = view.firstChildElement("layers");

        QString currentSvgPath = layers.attribute("image");
        svgPaths.insert(viewID, currentSvgPath);

		QMultiHash<ViewLayer::ViewLayerID, QString> extraSvg;
        QDomDocument writeDoc = viewThing->document->cloneNode(true).toDocument();
        QDomElement svgRoot = writeDoc.documentElement();    
        double svgWidth, svgHeight, vbWidth, vbHeight;
        bool svgOK = TextUtils::getSvgSizes(writeDoc, svgWidth, svgHeight, vbWidth, vbHeight);
        if (svgOK) {
            QHash<QString, QString> svgHash;
            foreach (QGraphicsItem * item, viewThing->sketchWidget->scene()->items()) {
                ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
                if (itemBase == NULL) continue;

                if (itemBase->layerKinChief() == viewThing->itemBase) continue;

                double factor;
			    QString string = itemBase->layerKinChief()->retrieveSvg(itemBase->viewLayerID(), svgHash, false, vbWidth / svgWidth, factor);
			    if (!string.isEmpty()) {
                    QPointF d = itemBase->pos() - viewThing->itemBase->pos();
                    double dx = d.x() * (vbWidth / svgWidth) / GraphicsUtils::SVGDPI;
                    double dy = d.y() * (vbWidth / svgWidth) / GraphicsUtils::SVGDPI;
                    string = QString("<g transform='translate(%1,%2)'>\n%3\n</g>\n").arg(dx).arg(dy).arg(string);
				    extraSvg.insert(itemBase->viewLayerID(), string);
			    }
            }
        }


        // deal with copper1 and copper0:
        //      find innermost
        //      don't save twice
        QDomElement copperChild;
        LayerList sketchLayers = viewThing->sketchWidget->viewLayers().keys();
        if (sketchLayers.contains(ViewLayer::Copper0) && sketchLayers.contains(ViewLayer::Copper1)) {
		    QDomElement copper0 = TextUtils::findElementWithAttribute(svgRoot, "id", "copper0");
		    QDomElement copper1 = TextUtils::findElementWithAttribute(svgRoot, "id", "copper1");
		    if (!copper0.isNull() && !copper1.isNull()) {
			    QDomElement parent0 = copper0.parentNode().toElement();
			    QDomElement parent1 = copper1.parentNode().toElement();
			    if (parent0.attribute("id") == "copper1") {
                    copperChild = copper0;
                }
			    else if (parent1.attribute("id") == "copper0") {
                    copperChild = copper1;
                }
            }
            if (!copperChild.isNull()) {
                if (extraSvg.uniqueKeys().contains(ViewLayer::Copper0)) {
                    extraSvg.remove(ViewLayer::Copper1);
                }
            }
        }

        foreach (ViewLayer::ViewLayerID viewLayerID, extraSvg.uniqueKeys()) {
            QStringList svgList = extraSvg.values(viewLayerID);
            QDomElement svgViewElement = (viewLayerID == ViewLayer::Copper0 && !copperChild.isNull()) 
                        ? copperChild 
                        : TextUtils::findElementWithAttribute(svgRoot, "id", ViewLayer::viewLayerXmlNameFromID(viewLayerID));
            if (svgViewElement.isNull()) {
                svgViewElement = svgRoot;
            }

            foreach (QString svg, svgList) { 
                QDomDocument doc;
                doc.setContent(svg, true);
                QDomElement root = doc.documentElement();
                foreach (ViewLayer::ViewLayerID vlid, sketchLayers) {
                    removeID(root, ViewLayer::viewLayerXmlNameFromID(vlid));
                }
                svgViewElement.appendChild(doc.documentElement());
            }
        }

        QString svg = writeDoc.toString();
        insertDesc(viewThing->referenceFile, svg);

        QString svgPath = makeSvgPath2(viewThing->sketchWidget);  
		setImageAttribute(layers, svgPath);

        QString actualPath = m_userPartsFolderSvgPath + svgPath; 
        bool result = writeXml(actualPath, removeGorn(svg), false);
        if (!result) {
            // TODO: warn user
        }
    }

    QDir dir(m_userPartsFolderPath);
	QString suffix = QString("%1_%2_%3").arg(m_prefix).arg(m_guid).arg(m_fileIndex++);
    QString fzpPath = dir.absoluteFilePath(QString("%1.fzp").arg(suffix));  

    if (overWrite) {
        fzpPath = m_originalFzpPath;
    }
	else {
		fzpRoot.setAttribute("moduleId", suffix);
		QString family = m_metadataView->family();
		QString variant = m_metadataView->variant();
		QHash<QString, QString> variants = m_referenceModel->allPropValues(family, "variant");
		QStringList values = variants.values(variant);
		if (values.count() > 0) {
			QString newVariant = makeNewVariant(family);
			QDomElement properties = fzpRoot.firstChildElement("properties");
			replaceProperty("variant", newVariant, properties);
			m_metadataView->resetProperty("variant", newVariant);
		}
	}

    bool result = writeXml(fzpPath, m_fzpDocument.toString(), false);

	if (!overWrite) {
		m_originalFzpPath = fzpPath;
		m_canSave = true;
		m_originalModuleID = fzpRoot.attribute("moduleId");
	}

    // restore the set of working svg files
    foreach (ViewLayer::ViewID viewID, m_viewThings.keys()) {
        QString svgPath = svgPaths.value(viewID);
        if (svgPath.isEmpty()) continue;

        QDomElement view = views.firstChildElement(ViewLayer::viewIDXmlName(viewID));
        QDomElement layers = view.firstChildElement("layers");
		setImageAttribute(layers, svgPath);
    }

    ModelPart * modelPart = m_referenceModel->retrieveModelPart(m_originalModuleID);
    if (modelPart == NULL) {
	    modelPart = m_referenceModel->loadPart(fzpPath, true);
        emit addToMyPartsSignal(modelPart);
	}
    else {
        m_referenceModel->reloadPart(fzpPath, m_originalModuleID);
        WaitPushUndoStack undoStack;
        QUndoCommand * parentCommand = new QUndoCommand;
        foreach (MainWindow * mainWindow, affectedWindows) {
            mainWindow->updateParts(m_originalModuleID, parentCommand);
        }
        foreach (MainWindow * mainWindow, allWindows) {
            mainWindow->updatePartsBin(m_originalModuleID);
        }
        undoStack.push(parentCommand);
    }

	m_autosaveNeeded = false;
    m_undoStack->setClean();

    return result;
}


void PEMainWindow::updateChangeCount(SketchWidget * sketchWidget, int changeDirection) {
	ViewThing * viewThing = m_viewThings.value(sketchWidget->viewID());
    viewThing->svgChangeCount += changeDirection;
}

PEGraphicsItem * PEMainWindow::findConnectorItem()
{
    if (m_currentGraphicsView == NULL) return NULL;

    QDomElement connector = m_connectorList.at(m_peToolView->currentConnectorIndex());
    if (connector.isNull()) return NULL;

	QString svgID, terminalID;
    bool ok = ViewLayer::getConnectorSvgIDs(connector, m_currentGraphicsView->viewID(), svgID, terminalID);
    if (!ok) return NULL;

    QList<PEGraphicsItem *> pegiList = getPegiList(m_currentGraphicsView);
    foreach (PEGraphicsItem * pegi, pegiList) {
        if (pegi->element().attribute("id") == svgID) return pegi;
    }

    return NULL;
}

void PEMainWindow::terminalPointChanged(const QString & how) {
    PEGraphicsItem * pegi = findConnectorItem();
    if (pegi == NULL) return;

    QRectF r = pegi->rect();
    QPointF p = r.center();
    if (how == "center") {
    }
    else if (how == "N") {
        p.setY(0);
    }
    else if (how == "E") {
        p.setX(r.width());
    }
    else if (how == "S") {
        p.setY(r.height());
    }
    else if (how == "W") {
        p.setX(0);
    }
    terminalPointChangedAux(pegi, pegi->terminalPoint(), p);

    // TODO: UndoCommand which changes fzp xml and svg xml
}

void PEMainWindow::terminalPointChanged(const QString & coord, double value)
{
    PEGraphicsItem * pegi = findConnectorItem();
    if (pegi == NULL) return;

    QPointF p = pegi->terminalPoint();
    if (coord == "x") {
        p.setX(qMax(0.0, qMin(value, pegi->rect().width())));
    }
    else {
        p.setY(qMax(0.0, qMin(value, pegi->rect().height())));
    }
    
    terminalPointChangedAux(pegi, pegi->terminalPoint(), p);
}

void PEMainWindow::terminalPointChangedAux(PEGraphicsItem * pegi, QPointF before, QPointF after)
{
    if (pegi->pendingTerminalPoint() == after) {
        return;
    }

    pegi->setPendingTerminalPoint(after);

    QDomElement currentConnectorElement = m_connectorList.at(m_peToolView->currentConnectorIndex());

    MoveTerminalPointCommand * mtpc = new MoveTerminalPointCommand(this, this->m_currentGraphicsView, currentConnectorElement.attribute("id"), pegi->rect().size(), before, after, NULL);
    mtpc->setText(tr("Move terminal point"));
    m_undoStack->waitPush(mtpc, SketchWidget::PropChangeDelay);
}

void PEMainWindow::getSpinAmount(double & amount) {
    double zoom = m_currentGraphicsView->currentZoom() / 100;
    if (zoom == 0) {
        amount = 1;
        return;
    }

    amount = qMin(1.0, 1.0 / zoom);
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        amount *= 10;
    }
}

void PEMainWindow::moveTerminalPoint(SketchWidget * sketchWidget, const QString & connectorID, QSizeF size, QPointF p, int changeDirection)
{
    // TODO: assumes these IDs are unique--need to check if they are not 

    QPointF center(size.width() / 2, size.height() / 2);
    bool centered = qAbs(center.x() - p.x()) < .05 && qAbs(center.y() - p.y()) < .05;

    QDomElement fzpRoot = m_fzpDocument.documentElement();
    QDomElement connectorElement = TextUtils::findElementWithAttribute(fzpRoot, "id", connectorID);
    if (connectorElement.isNull()) {
        DebugDialog::debug(QString("missing connector %1").arg(connectorID));
        return;
    }

    QDomElement pElement = ViewLayer::getConnectorPElement(connectorElement, sketchWidget->viewID());
    QString svgID = pElement.attribute("svgId");
    if (svgID.isEmpty()) {
        DebugDialog::debug(QString("Can't find svgId for connector %1").arg(connectorID));
        return;
    }

    PEGraphicsItem * connectorPegi = NULL;
	QList<PEGraphicsItem *> pegiList = getPegiList(sketchWidget);
    foreach (PEGraphicsItem * pegi, pegiList) {
        QDomElement pegiElement = pegi->element();
        if (pegiElement.attribute("id").compare(svgID) == 0) {
            connectorPegi = pegi;        
        }
    }
    if (connectorPegi == NULL) return;

    if (centered) {
        pElement.removeAttribute("terminalId");
        // no need to change SVG in this case
    }
    else {
		ViewThing * viewThing = m_viewThings.value(sketchWidget->viewID());
        QDomDocument * svgDoc = viewThing->document;
        QDomElement svgRoot = svgDoc->documentElement();
        QDomElement svgConnectorElement = TextUtils::findElementWithAttribute(svgRoot, "id", svgID);
        if (svgConnectorElement.isNull()) {
            DebugDialog::debug(QString("Unable to find svg connector element %1").arg(svgID));
            return;
        }

        QString terminalID = pElement.attribute("terminalId");
        if (terminalID.isEmpty()) {
            terminalID = svgID;
            if (terminalID.endsWith("pin") || terminalID.endsWith("pad")) {
                terminalID.chop(3);
            }
            terminalID += "terminal";
            pElement.setAttribute("terminalId", terminalID);
        }

        FSvgRenderer renderer;
        renderer.loadSvg(svgDoc->toByteArray(), "", false);
        QRectF svgBounds = renderer.boundsOnElement(svgID);
        double cx = p.x () * svgBounds.width() / size.width();
        double cy = p.y() * svgBounds.height() / size.height();
        double dx = svgBounds.width() / 1000;
        double dy = svgBounds.height() / 1000;

        QDomElement terminalElement = TextUtils::findElementWithAttribute(svgRoot, "id", terminalID);
        if (terminalElement.isNull()) {
            terminalElement = svgDoc->createElement("rect");
        }
        else if (terminalElement.tagName() != "rect" || terminalElement.attribute("fill") != "none" || terminalElement.attribute("stroke") != "none") {
            terminalElement.setAttribute("id", "");
            terminalElement = svgDoc->createElement("rect");
        }
        terminalElement.setAttribute("id", terminalID);
        terminalElement.setAttribute("oldid", terminalID);
        terminalElement.setAttribute("stroke", "none");
        terminalElement.setAttribute("fill", "none");
        terminalElement.setAttribute("stroke-width", "0");
        terminalElement.setAttribute("x", QString::number(svgBounds.left() + cx - dx));
        terminalElement.setAttribute("y", QString::number(svgBounds.top() + cy - dy));
        terminalElement.setAttribute("width", QString::number(dx * 2));
        terminalElement.setAttribute("height", QString::number(dy * 2));
        if (terminalElement.attribute("gorn").isEmpty()) {
            QString gorn = svgConnectorElement.attribute("gorn");
            int ix = gorn.lastIndexOf(".");
            if (ix > 0) {
                gorn.truncate(ix);
            }
            gorn = QString("%1.gen%2").arg(gorn).arg(FakeGornSiblingNumber++);
            terminalElement.setAttribute("gorn", gorn);
        }

        svgConnectorElement.parentNode().insertAfter(terminalElement, svgConnectorElement);

        double oldZ = connectorPegi->zValue() + 1;
        foreach (PEGraphicsItem * pegi, pegiList) {
            QDomElement pegiElement = pegi->element();
            if (pegiElement.attribute("id").compare(terminalID) == 0) {
                DebugDialog::debug("old pegi location", pegi->pos());
                oldZ = pegi->zValue();
                pegiList.removeOne(pegi);
                delete pegi;
                break;
            }
        }

        // update svg in case there is a subsequent call to reload
	    QString newPath = m_userPartsFolderSvgPath + makeSvgPath2(sketchWidget);
	    QString svg = TextUtils::svgNSOnly(svgDoc->toString());
        writeXml(newPath, removeGorn(svg), true);
        setImageAttribute(fzpRoot, newPath, sketchWidget->viewID());

        double invdx = dx * size.width() / svgBounds.width();
        double invdy = dy * size.height() / svgBounds.height();
        QPointF topLeft = connectorPegi->offset() + p - QPointF(invdx, invdy);
        PEGraphicsItem * pegi = makePegi(QSizeF(invdx * 2, invdy * 2), topLeft, viewThing->itemBase, terminalElement, oldZ);
        DebugDialog::debug("new pegi location", pegi->pos());
        updateChangeCount(sketchWidget, changeDirection);
    }

    connectorPegi->setTerminalPoint(p);
    connectorPegi->update();
    m_peToolView->setTerminalPointCoords(p);
}

void PEMainWindow::showInOS(QWidget *parent, const QString &pathIn)
{
    Q_UNUSED(parent);
    FolderUtils::showInFolder(pathIn);
    QClipboard *clipboard = QApplication::clipboard();
	if (clipboard != NULL) {
		clipboard->setText(pathIn);
	}
}

void PEMainWindow::showInOS() {
    if (m_currentGraphicsView == NULL) return;

	ViewThing * viewThing = m_viewThings.value(m_currentGraphicsView->viewID());
    showInOS(this, viewThing->itemBase->filename());
}

PEGraphicsItem * PEMainWindow::makePegi(QSizeF size, QPointF topLeft, ItemBase * itemBase, QDomElement & element, double z) 
{
    PEGraphicsItem * pegiItem = new PEGraphicsItem(0, 0, size.width(), size.height(), itemBase);
	pegiItem->showTerminalPoint(false);
    pegiItem->setPos(itemBase->pos() + topLeft);
    pegiItem->setZValue(z);
    itemBase->scene()->addItem(pegiItem);
    pegiItem->setElement(element);
    pegiItem->setOffset(topLeft);
    connect(pegiItem, SIGNAL(highlightSignal(PEGraphicsItem *)), this, SLOT(highlightSlot(PEGraphicsItem *)));
    connect(pegiItem, SIGNAL(mouseReleasedSignal(PEGraphicsItem *)), this, SLOT(pegiMouseReleased(PEGraphicsItem *)));
    connect(pegiItem, SIGNAL(mousePressedSignal(PEGraphicsItem *, bool &)), this, SLOT(pegiMousePressed(PEGraphicsItem *, bool &)), Qt::DirectConnection);
    connect(pegiItem, SIGNAL(terminalPointMoved(PEGraphicsItem *, QPointF)), this, SLOT(pegiTerminalPointMoved(PEGraphicsItem *, QPointF)));
    connect(pegiItem, SIGNAL(terminalPointChanged(PEGraphicsItem *, QPointF, QPointF)), this, SLOT(pegiTerminalPointChanged(PEGraphicsItem *, QPointF, QPointF)));
    return pegiItem;
}

QRectF PEMainWindow::getPixelBounds(FSvgRenderer & renderer, QDomElement & element)
{
	QSizeF defaultSizeF = renderer.defaultSizeF();
	QRectF viewBox = renderer.viewBoxF();

    QString id = element.attribute("id");
    QRectF r = renderer.boundsOnElement(id);
    QMatrix matrix = renderer.matrixForElement(id);
    QString oldid = element.attribute("oldid");
    if (!oldid.isEmpty()) {
        element.setAttribute("id", oldid);
        element.removeAttribute("oldid");
    }
    QRectF bounds = matrix.mapRect(r);
	bounds.setRect(bounds.x() * defaultSizeF.width() / viewBox.width(), 
						bounds.y() * defaultSizeF.height() / viewBox.height(), 
						bounds.width() * defaultSizeF.width() / viewBox.width(), 
						bounds.height() * defaultSizeF.height() / viewBox.height());
    return bounds;
}

bool PEMainWindow::canSave() {

    return m_canSave;
}

void PEMainWindow::tabWidget_currentChanged(int index) {
    MainWindow::tabWidget_currentChanged(index);

    if (m_peToolView == NULL) return;

    switchedConnector(m_peToolView->currentConnectorIndex());

    bool enabled = index < IconViewIndex;
	m_peSvgView->setChildrenVisible(enabled);
	m_peToolView->setChildrenVisible(enabled);

    if (m_currentGraphicsView == NULL) {
        // update title when switching to connector and metadata view
        setTitle();
    }
    else {
    }

	updateAssignedConnectors();
	updateActiveLayerButtons();
}

void PEMainWindow::backupSketch()
{
}

void PEMainWindow::removedConnector(const QDomElement & element)
{
    QList<QDomElement> connectors;
    connectors.append(element);
    removedConnectorsAux(connectors);
}

void PEMainWindow::removedConnectors(QList<ConnectorMetadata *> & cmdList)
{
    QList<QDomElement> connectors;

    foreach (ConnectorMetadata * cmd, cmdList) {
        int index;
        QDomElement connector = findConnector(cmd->connectorID, index);
        if (connector.isNull()) return;

        cmd->index = index;
        connectors.append(connector);
    }

    removedConnectorsAux(connectors);
}

void PEMainWindow::removedConnectorsAux(QList<QDomElement> & connectors)
{
    QString originalPath = saveFzp();

    foreach (QDomElement connector, connectors) {
        if (m_removedConnector.isEmpty()) {
            QTextStream stream(&m_removedConnector);
            connector.save(stream, 0);
        }
        connector.parentNode().removeChild(connector);
    }

    QString newPath = saveFzp();

    ChangeFzpCommand * cfc = new ChangeFzpCommand(this, originalPath, newPath, NULL);
    QString message;
    if (connectors.count() == 1) {
        message = tr("Remove connector");
    }
    else {
        message = tr("Remove %1 connectors").arg(connectors.count());
    }
    cfc->setText(message);
    m_undoStack->waitPush(cfc, SketchWidget::PropChangeDelay);
}

void PEMainWindow::restoreFzp(const QString & fzpPath) 
{
    if (!loadFzp(fzpPath)) return;

    reload(false);
}

void PEMainWindow::setBeforeClosingText(const QString & filename, QMessageBox & messageBox)
{
    Q_UNUSED(filename);

    QString partTitle = getPartTitle();
    messageBox.setWindowTitle(tr("Save \"%1\"").arg(partTitle));
    messageBox.setText(tr("Do you want to save the changes you made in the part \"%1\"?").arg(partTitle));
    messageBox.setInformativeText(tr("Your changes will be lost if you don't save them."));
}

QString PEMainWindow::getPartTitle() {
    QString partTitle = tr("untitled part");
    QDomElement root = m_fzpDocument.documentElement();
    QDomElement title = root.firstChildElement("title");
    QString candidate = title.text();
    if (!candidate.isEmpty()) return candidate;

    if (m_viewThings.count() > 0) {
		ViewThing * viewThing = m_viewThings.values().at(0);
		if (viewThing->itemBase) {
			candidate = viewThing->itemBase->title();
			if (!candidate.isEmpty()) return candidate;
		}
    }

    return partTitle;
}

void PEMainWindow::killPegi() {
    foreach (ViewThing * viewThing, m_viewThings.values()) {
        if (viewThing->sketchWidget == NULL) continue;

        foreach (QGraphicsItem * item, viewThing->sketchWidget->scene()->items()) {
            PEGraphicsItem * pegi = dynamic_cast<PEGraphicsItem *>(item);
            if (pegi) delete pegi;
        }
    }   
}

bool PEMainWindow::loadFzp(const QString & path) {
    QFile file(path);
	QString errorStr;
	int errorLine;
	int errorColumn;
	bool result = m_fzpDocument.setContent(&file, &errorStr, &errorLine, &errorColumn);
	if (!result) {
        QMessageBox::critical(NULL, tr("Parts Editor"), tr("Unable to load fzp from %1").arg(path));
		return false;
	}

    return true;
}

void PEMainWindow::connectorCountChanged(int newCount) {
    QList<QDomElement> connectorList;
    QDomElement root = m_fzpDocument.documentElement();
    QDomElement connectors = root.firstChildElement("connectors");
    QDomElement connector = connectors.firstChildElement("connector");
    while (!connector.isNull()) {
        connectorList.append(connector);
        connector = connector.nextSiblingElement();
    }

    if (newCount == connectorList.count()) return;

    if (newCount < connectorList.count()) {
        qSort(connectorList.begin(), connectorList.end(), byID);
        QList<QDomElement> toDelete;
        for (int i = newCount; i < connectorList.count(); i++) {
            toDelete.append(connectorList.at(i));
        }

        removedConnectorsAux(toDelete);
        return;
    }

    // add connectors 
    int id = 0;
    foreach (QDomElement connector, connectorList) {
    	int ix = IntegerFinder.indexIn(connector.attribute("id"));
        if (ix >= 0) {
            int candidate = IntegerFinder.cap(0).toInt();
            if (candidate > id) id = candidate;
        }
        // sometimes id = 0 but name = 1, and we are now using name = id
        ix = IntegerFinder.indexIn(connector.attribute("name"));
        if (ix >= 0) {
            int candidate = IntegerFinder.cap(0).toInt();
            if (candidate > id) id = candidate;
        }    
    }

    QString originalPath = saveFzp();

	QDomElement connectorModel;
    QDomDocument tempDoc;
    if (connectorList.count() > 0) connectorModel = connectorList.at(0);
    else {
        tempDoc.setContent(m_removedConnector);
        connectorModel = tempDoc.documentElement();
        if (connectorModel.isNull()) {
            QMessageBox::critical(NULL, tr("Parts Editor"), tr("Unable to create new connector--you may have to start over."));
		    return;
        }
    }
    for (int i = connectorList.count(); i < newCount; i++) {
        id++;
        QDomElement element = connectorModel.cloneNode(true).toElement();         
        connectors.appendChild(element);
        QString newName = QString("pin %1").arg(id);
        element.setAttribute("name", newName);
		QString cid = QString("connector%1").arg(id);
        element.setAttribute("id", cid);
		QString svgid = cid + "pin";
		QDomNodeList nodeList = element.elementsByTagName("p");
		for (int n = 0; n < nodeList.count(); n++) {
			QDomElement p = nodeList.at(n).toElement();
			p.removeAttribute("terminalId");
			p.removeAttribute("legId");
			p.setAttribute("svgId", svgid);
		}
        QDomElement description = element.firstChildElement("description");
        if (description.isNull()) {
            description = m_fzpDocument.createElement("description");
            element.appendChild(description);
        }
        TextUtils::replaceChildText(description, newName);
    }

    QString newPath = saveFzp();

    ChangeFzpCommand * cfc = new ChangeFzpCommand(this, originalPath, newPath, NULL);
    QString message;
    if (newCount - connectorList.count() == 1) {
        message = tr("Add connector");
    }
    else {
        message = tr("Add %1 connectors").arg(newCount - connectorList.count());
    }
    cfc->setText(message);
    m_undoStack->waitPush(cfc, SketchWidget::PropChangeDelay);
}

bool PEMainWindow::editsModuleID(const QString & moduleID) {
    // only to detect whether a user tries to open the parts editor on the same part twice
    return (m_originalModuleID.compare(moduleID) == 0);
}

/*
QString PEMainWindow::getFzpReferenceFile() {
    QString referenceFile = m_fzpDocument.documentElement().attribute(ReferenceFileString);
    if (!referenceFile.isEmpty()) referenceFile += "_";
    return referenceFile;
}
*/

QString PEMainWindow::getSvgReferenceFile(const QString & filename) {
    QFileInfo info(filename);
    QString referenceFile = info.fileName();

    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) return referenceFile;

    QString svg = file.readAll();
    if (!svg.contains(ReferenceFileString)) return referenceFile;

    QXmlStreamReader xml(svg.toUtf8());
	xml.setNamespaceProcessing(false);
    bool inDesc = false;
	while (!xml.atEnd()) {
        switch (xml.readNext()) {
            case QXmlStreamReader::StartElement:
                {
                    QString name = xml.name().toString();
			        if (inDesc && name.compare(ReferenceFileString) == 0) {
				        QString candidate = xml.readElementText().trimmed();
				        if (candidate.isEmpty()) return referenceFile;
                        return candidate;
			        }
			        if (name.compare("desc") == 0) {
				        inDesc = true;
			        }
                }
			    break;
            case QXmlStreamReader::EndElement:
                {
                    QString name = xml.name().toString();
			        if (name.compare("desc") == 0) {
				        inDesc = false;
			        }
                }
			    break;
			
		    default:
			    break;
		}
	}

    return referenceFile;
}

QString PEMainWindow::makeDesc(const QString & referenceFile) 
{
    return QString("\n<desc><%2>%1</%2></desc>\n").arg(referenceFile).arg(ReferenceFileString);
}

void PEMainWindow::updateWindowMenu() {
}

void PEMainWindow::updateRaiseWindowAction() {
    QString title = tr("Fritzing (New) Parts Editor");
    QString partTitle = getPartTitle();
	QString actionText = QString("%1: %2").arg(title).arg(partTitle);
	m_raiseWindowAct->setText(actionText);
	m_raiseWindowAct->setToolTip(actionText);
	m_raiseWindowAct->setStatusTip("raise \""+actionText+"\" window");
}

bool PEMainWindow::writeXml(const QString & path, const QString & xml, bool temp)
{
	bool result = TextUtils::writeUtf8(path, TextUtils::svgNSOnly(xml));
	if (result) {
		if (temp) m_filesToDelete.append(path);
		else m_filesToDelete.removeAll(path);
	}

	return result;
}

void PEMainWindow::displayBuses() {
	deleteBuses();

	QDomElement root = m_fzpDocument.documentElement();
	QDomElement buses = root.firstChildElement("buses");
	QDomElement bus = buses.firstChildElement("bus");
	while (!bus.isNull()) {
		QDomElement nodeMember = bus.firstChildElement("nodeMember");
		QSet<QString> connectorIDs;
		while (!nodeMember.isNull()) {
			QString connectorID = nodeMember.attribute("connectorId");
			if (!connectorID.isEmpty()) {
				connectorIDs.insert(connectorID);
			}
			nodeMember = nodeMember.nextSiblingElement("nodeMember");
		}

		foreach (ViewLayer::ViewID viewID, m_viewThings.keys()) {
			ViewThing * viewThing = m_viewThings.value(viewID);
			QList<ConnectorItem *> connectorItems;
			foreach (QString connectorID, connectorIDs) {
				ConnectorItem * connectorItem = viewThing->itemBase->findConnectorItemWithSharedID(connectorID, viewThing->itemBase->viewLayerPlacement());
				if (connectorItem) connectorItems.append(connectorItem);
			}
			for (int i = 0; i < connectorItems.count() - 1; i++) {
				ConnectorItem * c1 = connectorItems.at(i);
				ConnectorItem * c2 = connectorItems.at(i + 1);
				Wire * wire = viewThing->sketchWidget->makeOneRatsnestWire(c1, c2, false, QColor(0, 255, 0), true);
				wire->setZValue(RatZ);
			}
		}

		bus = bus.nextSiblingElement("bus");
	}
}

void PEMainWindow::updateWireMenu() {
	// assumes update wire menu is only called when right-clicking a wire
	// and that wire is cached by the menu in Wire::mousePressEvent

	Wire * wire = m_activeWire;
	m_activeWire = NULL;

	m_deleteBusConnectionAct->setWire(wire);
	m_deleteBusConnectionAct->setEnabled(true);
}

void PEMainWindow::deleteBusConnection() {
	WireAction * wireAction = qobject_cast<WireAction *>(sender());
	if (wireAction == NULL) return;

	Wire * wire = wireAction->wire();
	if (wire == NULL) return;

	QList<Wire *> wires;
	QList<ConnectorItem *> ends;
	wire->collectChained(wires, ends);
	if (ends.count() != 2) return;

	QString id0 = ends.at(0)->connectorSharedID();
	QString id1 = ends.at(1)->connectorSharedID();

	QDomElement root = m_fzpDocument.documentElement();
	QDomElement buses = root.firstChildElement("buses");
	QDomElement bus = buses.firstChildElement("bus");
	QDomElement nodeMember0;
	QDomElement nodeMember1;
	while (!bus.isNull()) {
		nodeMember0 = TextUtils::findElementWithAttribute(bus, "connectorId", id0);
		nodeMember1 = TextUtils::findElementWithAttribute(bus, "connectorId", id1);
		if (!nodeMember0.isNull() && !nodeMember1.isNull()) break;

		bus = bus.nextSiblingElement("bus");
	}

	QString busID = bus.attribute("id");

	if (nodeMember0.isNull() || nodeMember1.isNull()) {
		QMessageBox::critical(NULL, tr("Parts Editor"), tr("Internal connections are very messed up."));
		return;
	}
	
	QUndoCommand * parentCommand = new QUndoCommand();
	QStringList names;
	names << ends.at(0)->connectorSharedName() << ends.at(1)->connectorSharedName() ;
	new RemoveBusConnectorCommand(this, busID, id0, false, parentCommand);
	new RemoveBusConnectorCommand(this, busID, id1, false, parentCommand);
	if (ends.at(0)->connectedToItems().count() > 1) {
		// restore it
		names.removeAt(0);
		new RemoveBusConnectorCommand(this, busID, id0, true, parentCommand);
	}
	if (ends.at(1)->connectedToItems().count() > 1) {
		// restore it
		new RemoveBusConnectorCommand(this, busID, id1, true, parentCommand);
	}

	parentCommand->setText(tr("Remove internal connection from '%1'").arg(names.at(0)));
	m_undoStack->waitPush(parentCommand, SketchWidget::PropChangeDelay);
}

void PEMainWindow::newWireSlot(Wire * wire) {
	wire->setDisplayBendpointCursor(false);
	disconnect(wire, 0, m_viewThings.value(wire->viewID())->sketchWidget, 0);
	connect(wire, SIGNAL(wireChangedSignal(Wire*, const QLineF & , const QLineF & , QPointF, QPointF, ConnectorItem *, ConnectorItem *)	),
			this, SLOT(wireChangedSlot(Wire*, const QLineF & , const QLineF & , QPointF, QPointF, ConnectorItem *, ConnectorItem *)),
			Qt::DirectConnection);		// DirectConnection means call the slot directly like a subroutine, without waiting for a thread or queue
}

void PEMainWindow::wireChangedSlot(Wire* wire, const QLineF &, const QLineF &, QPointF, QPointF, ConnectorItem * fromOnWire, ConnectorItem * to) {
	wire->deleteLater();

	if (to == NULL) return;

	ConnectorItem * from = wire->otherConnector(fromOnWire)->firstConnectedToIsh();
	if (from == NULL) return;

	QDomElement root = m_fzpDocument.documentElement();
	QDomElement buses = root.firstChildElement("buses");
	if (buses.isNull()) {
		buses = m_fzpDocument.createElement("buses");
		root.appendChild(buses);
	}


	QDomElement fromBus = findNodeMemberBus(buses, from->connectorSharedID());
	QString fromBusID = fromBus.attribute("id");
	QDomElement toBus = findNodeMemberBus(buses, to->connectorSharedID());
	QString toBusID = toBus.attribute("id");

	QString busID = fromBusID.isEmpty() ? toBusID : fromBusID;
	if (busID.isEmpty()) {
		int theMax = std::numeric_limits<int>::max(); 
		for (int ix = 1; ix < theMax; ix++) {
			QString candidate = QString("internal%1").arg(ix);
			QDomElement busElement = findBus(buses, candidate);
			if (busElement.isNull()) {
				busID = candidate;
				break;
			}
		}
	}

	QUndoCommand * parentCommand = new QUndoCommand(tr("Add internal connection from '%1' to '%2'").arg(from->connectorSharedName()).arg(to->connectorSharedName()));
	if (!fromBusID.isEmpty()) {
		// changing the bus for this nodeMember
		new RemoveBusConnectorCommand(this, fromBusID, from->connectorSharedID(), false, parentCommand);
	}
	if (!toBusID.isEmpty()) {
		// changing the bus for this nodeMember
		new RemoveBusConnectorCommand(this, toBusID, to->connectorSharedID(), false, parentCommand);
	}
	new RemoveBusConnectorCommand(this, busID, from->connectorSharedID(), true, parentCommand);
	new RemoveBusConnectorCommand(this, busID, to->connectorSharedID(), true, parentCommand);
	m_undoStack->waitPush(parentCommand, SketchWidget::PropChangeDelay);
}

QDomElement PEMainWindow::findBus(const QDomElement & buses, const QString & id)
{
	QDomElement busElement = buses.firstChildElement("bus");
	while (!busElement.isNull()) {
		if (busElement.attribute("id").compare(id) == 0) {
			return busElement;
		}
		busElement = busElement.nextSiblingElement("bus");
	}

	return QDomElement();
}

QDomElement PEMainWindow::findNodeMemberBus(const QDomElement & buses, const QString & connectorID)
{
	QDomElement bus = buses.firstChildElement("bus");
	while (!bus.isNull()) {
		QDomElement nodeMember = bus.firstChildElement("nodeMember");
		while (!nodeMember.isNull()) {
			if (nodeMember.attribute("connectorId").compare(connectorID) == 0) {
				return bus;
			}
			nodeMember = nodeMember.nextSiblingElement("nodeMember");
		}
		bus = bus.nextSiblingElement("bus");
	}

	return QDomElement();
}

void PEMainWindow::addBusConnector(const QString & busID, const QString & connectorID)
{
	// called from command object
	removeBusConnector(busID, connectorID, false);			// keep the dom very clean

	QDomElement root = m_fzpDocument.documentElement();
	QDomElement buses = root.firstChildElement("buses");
	if (buses.isNull()) {
		m_fzpDocument.createElement("buses");
		root.appendChild(buses);
	}

	QDomElement theBusElement = findBus(buses, busID);
	if (theBusElement.isNull()) {
		theBusElement = m_fzpDocument.createElement("bus");
		theBusElement.setAttribute("id", busID);
		buses.appendChild(theBusElement);
	}

	QDomElement nodeMember = m_fzpDocument.createElement("nodeMember");
	nodeMember.setAttribute("connectorId", connectorID);
	theBusElement.appendChild(nodeMember);
	displayBuses();
}

void PEMainWindow::removeBusConnector(const QString & busID, const QString & connectorID, bool display)
{
	// called from command object
	// for the sake of cleaning, deletes all matching nodeMembers so be careful about the order of deletion and addition within the same parentCommand
	Q_UNUSED(busID);

	QDomElement root = m_fzpDocument.documentElement();
	QDomElement buses = root.firstChildElement("buses");
	QDomElement bus = buses.firstChildElement("bus");
	QList<QDomElement> toDelete;
	while (!bus.isNull()) {
		QDomElement nodeMember = bus.firstChildElement("nodeMember");
		while (!nodeMember.isNull()) {
			if (nodeMember.attribute("connectorId").compare(connectorID) == 0) {
				toDelete.append(nodeMember);
			}
			nodeMember = nodeMember.nextSiblingElement("nodeMember");
		}
		bus = bus.nextSiblingElement("bus");
	}

	foreach (QDomElement element, toDelete) {
		element.parentNode().removeChild(element);
	}

	if (display) displayBuses();
}

void PEMainWindow::replaceProperty(const QString & key, const QString & value, QDomElement & properties)
{
    QDomElement prop = properties.firstChildElement("property");
    while (!prop.isNull()) {
        QString name = prop.attribute("name");
        if (name.compare(key, Qt::CaseInsensitive) == 0) {
            TextUtils::replaceChildText(prop, value);
            return;
        }

        prop = prop.nextSiblingElement("property");
    }
    
	prop = m_fzpDocument.createElement("property");
    properties.appendChild(prop);
    prop.setAttribute("name", key);
    TextUtils::replaceChildText(prop, value);
}

QWidget * PEMainWindow::createTabWidget() {
    QTabWidget * tabWidget = new QTabWidget(this);
    tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tabWidget->setObjectName("pe_tabs");
	return tabWidget;
}

void PEMainWindow::addTab(QWidget * widget, const QString & label) {
	qobject_cast<QTabWidget *>(m_tabWidget)->addTab(widget, label);
    this -> setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

int PEMainWindow::currentTabIndex() {
	return qobject_cast<QTabWidget *>(m_tabWidget)->currentIndex();
}

void PEMainWindow::setCurrentTabIndex(int index) {
	qobject_cast<QTabWidget *>(m_tabWidget)->setCurrentIndex(index);
}

QWidget * PEMainWindow::currentTabWidget() {
	return qobject_cast<QTabWidget *>(m_tabWidget)->currentWidget();
}

bool PEMainWindow::event(QEvent * e) {
	if (e->type() == QEvent::Close) {
		//qDebug() << "event close";
		//if (m_inFocusWidgets.count() > 0) {
		//	e->ignore();
		//	qDebug() << "bail in focus";
		//	return true;
		//}
	}

	return MainWindow::event(e);
}

bool PEMainWindow::eventFilter(QObject *object, QEvent *event)
{
	if (m_inPickMode) {
		switch (event->type()) {
			case QEvent::MouseButtonPress:
				clearPickMode();
				{
					QMouseEvent * mouseEvent = static_cast<QMouseEvent *>(event);
					m_useNextPick = (mouseEvent->button() == Qt::LeftButton);
				}
				QTimer::singleShot(1, this, SLOT(resetNextPick()));
				break;

			case QEvent::ApplicationActivate:
			case QEvent::ApplicationDeactivate:
			case QEvent::WindowActivate:
			case QEvent::WindowDeactivate:
			case QEvent::NonClientAreaMouseButtonPress:
				clearPickMode();
				break;

			case QEvent::KeyPress:
			{
				QKeyEvent * kevent = static_cast<QKeyEvent *>(event);
				if (kevent->key() == Qt::Key_Escape) {
					clearPickMode();
					return true;
				}
			}

			default:
				break;
		}

		return false;
	}

	//qDebug() << "event" << event->type();
    if (event->type() == QEvent::FocusIn) {
        QLineEdit * lineEdit = qobject_cast<QLineEdit *>(object);
		if (lineEdit != NULL) {
			if (lineEdit->window() == this) {
				qDebug() << "inc focus";
				m_inFocusWidgets << lineEdit;
			}
		}
		else {
			QTextEdit * textEdit = qobject_cast<QTextEdit *>(object);
			if (textEdit != NULL && textEdit->window() == this) {
				qDebug() << "inc focus";
				m_inFocusWidgets << textEdit;
			}
		}
    }
    if (event->type() == QEvent::FocusOut) {
        QLineEdit * lineEdit = qobject_cast<QLineEdit *>(object);
		if (lineEdit != NULL) {
			if (lineEdit->window() == this) {
				qDebug() << "dec focus";
				m_inFocusWidgets.removeOne(lineEdit);
			}
		}
		else {
			QTextEdit * textEdit = qobject_cast<QTextEdit *>(object);
			if (textEdit != NULL && textEdit->window() == this) {
				qDebug() << "inc focus";
				m_inFocusWidgets.removeOne(textEdit);
			}
		}
    }
    return false;
}

void PEMainWindow::closeLater()
{
	close();
}

void PEMainWindow::resetNextPick() {
	m_useNextPick = false;
}

void PEMainWindow::clearPickMode() {
	qApp->removeEventFilter(this);
	m_useNextPick = m_inPickMode = false;
	QApplication::restoreOverrideCursor();
	if (m_currentGraphicsView) {
		foreach (QGraphicsItem * item, m_currentGraphicsView->scene()->items()) {
			PEGraphicsItem * pegi = dynamic_cast<PEGraphicsItem *>(item);
			if (pegi) pegi->setPickAppearance(false);
		}
	}
}

QList<PEGraphicsItem *> PEMainWindow::getPegiList(SketchWidget * sketchWidget) {
   // DebugDialog::debug("-----------------------------");

    QList<PEGraphicsItem *> pegiList;
    foreach (QGraphicsItem * item, sketchWidget->scene()->items()) {
        PEGraphicsItem * pegi = dynamic_cast<PEGraphicsItem *>(item);
        if (pegi == NULL) continue;
		
		pegiList.append(pegi);
        /*
        if (pegi->showingTerminalPoint() || pegi->showingMarquee() || !pegi->element().attribute("id").isEmpty()) {
            DebugDialog::debug(QString("pegi m:%1 t:%2 %3")
                .arg(pegi->showingMarquee())
                .arg(pegi->showingTerminalPoint())
                .arg(pegi->element().attribute("id")));
        }
        */
    }

    //DebugDialog::debug("-----------------------------");

	return pegiList;
}

void PEMainWindow::deleteBuses() {
	QList<Wire *> toDelete;
	foreach (ViewThing * viewThing, m_viewThings.values()) {
		foreach (QGraphicsItem * item, viewThing->sketchWidget->scene()->items()) {
			Wire * wire = dynamic_cast<Wire *>(item);
			if (wire == NULL) continue;

			toDelete << wire;
		}
	}

	foreach (Wire * wire, toDelete) {
		delete wire;
	}
}

void PEMainWindow::connectorsTypeChanged(Connector::ConnectorType ct) 
{
	QUndoCommand * parentCommand = NULL;

	QDomElement root = m_fzpDocument.documentElement();
	QDomElement connectors = root.firstChildElement("connectors");
	QDomElement connector = connectors.firstChildElement("connector");
	while (!connector.isNull()) {
		ConnectorMetadata oldCmd;
		fillInMetadata(connector, oldCmd);
		if (oldCmd.connectorType != ct) {
			if (parentCommand == NULL) {
				parentCommand = new QUndoCommand(tr("Change all connectors to %1").arg(Connector::connectorNameFromType(ct)));
			}
			ConnectorMetadata cmd = oldCmd;
			cmd.connectorType = ct;
			new ChangeConnectorMetadataCommand(this, &oldCmd, &cmd, parentCommand);
		}

		connector = connector.nextSiblingElement("connector");
	}

	if (parentCommand) {
		m_undoStack->waitPush(parentCommand, SketchWidget::PropChangeDelay);
	}
}

void PEMainWindow::fillInMetadata(const QDomElement & connector, ConnectorMetadata & cmd) 
{
    cmd.connectorID = connector.attribute("id");
    cmd.connectorType = Connector::connectorTypeFromName(connector.attribute("type"));
    cmd.connectorName = connector.attribute("name");
    QDomElement description = connector.firstChildElement("description");
    cmd.connectorDescription = description.text();
}

void PEMainWindow::smdChanged(const QString & after) {
	QString before;
	bool toSMD = true;
	if (after.compare("smd", Qt::CaseInsensitive) != 0) {
		before = "smd";
		toSMD = false;
	}

	ViewThing * viewThing = m_viewThings.value(ViewLayer::PCBView);
	ItemBase * itemBase = viewThing->itemBase;
	if (itemBase == NULL) return;

	QFile file(itemBase->filename());
	QDomDocument svgDoc;
	svgDoc.setContent(&file);
	QDomElement svgRoot = svgDoc.documentElement();
	QDomElement svgCopper0 = TextUtils::findElementWithAttribute(svgRoot, "id", "copper0");
	QDomElement svgCopper1 = TextUtils::findElementWithAttribute(svgRoot, "id", "copper1");
	if (svgCopper0.isNull() && svgCopper1.isNull()) {
		QMessageBox::critical(NULL, tr("Parts Editor"), tr("Unable to parse '%1'").arg(itemBase->filename()));
		return;
	}

	if (toSMD) {
		if (svgCopper0.isNull() && !svgCopper1.isNull()) {
			// everything is fine
		}
		else if (svgCopper1.isNull()) {
			// just needs a swap
			svgCopper0.setAttribute("id", "copper1");
		}
		else {
			svgCopper0.removeAttribute("id");
		}
	}
	else {
		if (!svgCopper0.isNull() && !svgCopper1.isNull()) {
			// everything is fine
		}
		else if (svgCopper1.isNull()) {
			svgCopper1 = m_pcbDocument.createElement("g");
			svgCopper1.setAttribute("id", "copper1");
			svgRoot.appendChild(svgCopper1);
			QDomElement child = svgCopper0.firstChildElement();
			while (!child.isNull()) {
				QDomElement next = child.nextSiblingElement();
				svgCopper1.appendChild(child);
				child = next;
			}
			svgCopper0.appendChild(svgCopper1);
		}
		else {
			svgCopper0 = m_pcbDocument.createElement("g");
			svgCopper0.setAttribute("id", "copper0");
			svgRoot.appendChild(svgCopper0);
			QDomElement child = svgCopper1.firstChildElement();
			while (!child.isNull()) {
				QDomElement next = child.nextSiblingElement();
				svgCopper0.appendChild(child);
				child = next;
			}
			svgCopper1.appendChild(svgCopper0);
		}
	}

	QString newPath = m_userPartsFolderSvgPath + makeSvgPath2(m_pcbGraphicsView);
	QString svg = TextUtils::svgNSOnly(svgDoc.toString());
    writeXml(newPath, removeGorn(svg), true);

	ChangeSMDCommand * csc = new ChangeSMDCommand(this, before, after, itemBase->filename(), newPath, NULL);
	csc->setText(tr("Change to %1").arg(after));
	m_undoStack->waitPush(csc, SketchWidget::PropChangeDelay);
}

void PEMainWindow::changeSMD(const QString & after, const QString & filename, int changeDirection)
{
	QDomElement root = m_fzpDocument.documentElement();
	QDomElement views = root.firstChildElement("views");
	QDomElement pcbView = views.firstChildElement("pcbView");
	QDomElement layers = pcbView.firstChildElement("layers");
	QDomElement copper0 = TextUtils::findElementWithAttribute(layers, "layerId", "copper0");
	QDomElement copper1 = TextUtils::findElementWithAttribute(layers, "layerId", "copper1");
	bool toSMD = true;
	if (after.compare("smd", Qt::CaseInsensitive) == 0) {
		if (!copper0.isNull()) {
			copper0.parentNode().removeChild(copper0);
		}
	}
	else {
		toSMD = false;
		if (copper0.isNull()) {
			copper0 = m_fzpDocument.createElement("layer");
			copper0.setAttribute("layerId", "copper0");
			layers.appendChild(copper0);
		}
	}
	if (copper1.isNull()) {
		copper1 = m_fzpDocument.createElement("layer");
		copper1.setAttribute("layerId", "copper1");
		layers.appendChild(copper1);
	}

	QDomElement connectors = root.firstChildElement("connectors");
	QDomElement connector = connectors.firstChildElement("connector");
	while (!connector.isNull()) {
		setConnectorSMD(toSMD, connector);
		connector = connector.nextSiblingElement("connector");
	}

	changeSvg(m_pcbGraphicsView, filename, changeDirection);
}

void PEMainWindow::setConnectorSMD(bool toSMD, QDomElement & connector) {
	QDomElement views = connector.firstChildElement("views");
	QDomElement pcbView = views.firstChildElement("pcbView");
	QDomElement copper0 = TextUtils::findElementWithAttribute(pcbView, "layer", "copper0");
	QDomElement copper1 = TextUtils::findElementWithAttribute(pcbView, "layer", "copper1");
	if (copper0.isNull() && copper1.isNull()) {
		// SVG is seriously messed up
		DebugDialog::debug("setting SMD is very broken");
		return;
	}

	if (toSMD) {
		if (copper0.isNull() && !copper1.isNull()) {
			// already correct
			return;
		}
		if (copper1.isNull()) {
			// swap it to copper1
			copper0.setAttribute("layer", "copper1");
			return;
		}
		// remove the extra layer
		copper0.parentNode().removeChild(copper0);
		return;
	}

	if (!copper0.isNull() && !copper1.isNull()) {
		// already correct
		return;
	}

	if (copper1.isNull()) {
		copper1 = copper0.cloneNode(true).toElement();
		copper0.parentNode().appendChild(copper1);
		copper1.setAttribute("layer", "copper1");
		return;
	}

	copper0 = copper1.cloneNode(true).toElement();
	copper1.parentNode().appendChild(copper0);
	copper0.setAttribute("layer", "copper0");
}


void PEMainWindow::reuseBreadboard()
{
	reuseImage(ViewLayer::BreadboardView);
}

void PEMainWindow::reuseSchematic()
{
	reuseImage(ViewLayer::SchematicView);
}

void PEMainWindow::reusePCB()
{
	reuseImage(ViewLayer::PCBView);
}

void PEMainWindow::reuseImage(ViewLayer::ViewID viewID) {
	if (m_currentGraphicsView == NULL) return;

	ViewThing * afterViewThing = m_viewThings.value(viewID);
	if (afterViewThing->itemBase == NULL) return;

	QString afterFilename = afterViewThing->itemBase->filename();

	ViewThing * beforeViewThing = m_viewThings.value(m_currentGraphicsView->viewID());

	ChangeSvgCommand * csc = new ChangeSvgCommand(this, m_currentGraphicsView, beforeViewThing->itemBase->filename(), afterFilename, NULL);
    QFileInfo info(afterFilename);
    csc->setText(QString("Load '%1'").arg(info.fileName()));
    m_undoStack->waitPush(csc, SketchWidget::PropChangeDelay);
}

void PEMainWindow::updateFileMenu() {
	MainWindow::updateFileMenu();

    m_saveAct->setEnabled(canSave());

	/*
	QHash<ViewLayer::ViewID, bool> enabled;
	enabled.insert(ViewLayer::BreadboardView, true);
	enabled.insert(ViewLayer::SchematicView, true);
	enabled.insert(ViewLayer::PCBView, true);
	bool enableAll = true;
	if (m_currentGraphicsView == NULL) {
		enableAll = false;
	}
	else {
		ViewLayer::ViewID viewID = m_currentGraphicsView->viewID();
		enabled.insert(viewID, false);
	}

	m_reuseBreadboardAct->setEnabled(enableAll && enabled.value(ViewLayer::BreadboardView));
	m_reuseSchematicAct->setEnabled(enableAll && enabled.value(ViewLayer::SchematicView));
	m_reusePCBAct->setEnabled(enableAll && enabled.value(ViewLayer::PCBView));
	*/

	bool enabled = m_currentGraphicsView != NULL && m_currentGraphicsView->viewID() == ViewLayer::IconView;
	m_reuseBreadboardAct->setEnabled(enabled);
	m_reuseSchematicAct->setEnabled(enabled);
	m_reusePCBAct->setEnabled(enabled);
}

void PEMainWindow::setImageAttribute(QDomElement & fzpRoot, const QString & svgPath, ViewLayer::ViewID viewID)
{
    QDomElement views = fzpRoot.firstChildElement("views");
    QDomElement view = views.firstChildElement(ViewLayer::viewIDXmlName(viewID));
    QDomElement layers = view.firstChildElement("layers");
    QFileInfo info(svgPath);
    QDir dir = info.absoluteDir();
    QString shortName = dir.dirName() + "/" + info.fileName();
	setImageAttribute(layers, shortName);
}

void PEMainWindow::setImageAttribute(QDomElement & layers, const QString & svgPath)
{
	layers.setAttribute("image", svgPath);
	QDomElement layer = layers.firstChildElement("layer");
	if (!layer.isNull()) return;

	layer = m_fzpDocument.createElement("layer");
	layers.appendChild(layer);
	layer.setAttribute("layerId", ViewLayer::viewLayerXmlNameFromID(ViewLayer::UnknownLayer));
}

void PEMainWindow::updateLayerMenu(bool resetLayout) {
	MainWindow::updateLayerMenu(resetLayout);

	bool enabled = false;
	if (m_currentGraphicsView != NULL) {
		switch (m_currentGraphicsView->viewID()) {
			case ViewLayer::BreadboardView:
			case ViewLayer::SchematicView:
			case ViewLayer::PCBView:
				enabled = true;
			default:
				break;
		}
	}

	m_hideOtherViewsAct->setEnabled(enabled);
}

void PEMainWindow::hideOtherViews() {
	if (m_currentGraphicsView == NULL) return;

	ViewLayer::ViewID afterViewID = m_currentGraphicsView->viewID();
	ItemBase * afterItemBase = m_viewThings.value(afterViewID)->itemBase;
	if (afterItemBase == NULL) return;
	QString afterFilename = afterItemBase->filename();

	QList<ViewLayer::ViewID> viewIDList;
	viewIDList << ViewLayer::BreadboardView << ViewLayer::SchematicView << ViewLayer::PCBView;
	viewIDList.removeOne(afterViewID);

	QString originalPath = saveFzp();

	QString afterViewName = ViewLayer::viewIDXmlName(afterViewID);
	QStringList beforeViewNames;
	foreach (ViewLayer::ViewID viewID, viewIDList) {
		beforeViewNames << ViewLayer::viewIDXmlName(viewID);
	}

	QDomElement root = m_fzpDocument.documentElement();
	QDomElement connectors = root.firstChildElement("connectors");
	QDomElement connector = connectors.firstChildElement("connector");
	while (!connector.isNull()) {
		QDomElement views = connector.firstChildElement("views");
		QDomElement afterView = views.firstChildElement(afterViewName);
		
		foreach (QString name, beforeViewNames) {
			QDomElement toRemove = views.firstChildElement(name);
			if (!toRemove.isNull()) {
				toRemove.parentNode().removeChild(toRemove);
			}
			QDomElement toReplace = afterView.cloneNode(true).toElement();
			toReplace.setTagName(name);
			views.appendChild(toReplace);
		}

		connector = connector.nextSiblingElement("connector");
	}

	QDomElement views = root.firstChildElement("views");
	QDomElement afterView = views.firstChildElement(afterViewName);
	foreach (QString name, beforeViewNames) {
		QDomElement toRemove = views.firstChildElement(name);
		if (!toRemove.isNull()) {
			toRemove.parentNode().removeChild(toRemove);
		}
		QDomElement toReplace = afterView.cloneNode(true).toElement();
		toReplace.setTagName(name);
		views.appendChild(toReplace);
	}
	
    QString newPath = saveFzp();
	ChangeFzpCommand * cfc = new ChangeFzpCommand(this, originalPath, newPath, NULL);
	cfc->setText(tr("Make only %1 view visible").arg(m_currentGraphicsView->viewName()));
	m_undoStack->waitPush(cfc, SketchWidget::PropChangeDelay);
}

QString PEMainWindow::makeNewVariant(const QString & family)
{
	QStringList variants = m_referenceModel->propValues(family, "variant", true);
	int theMax = std::numeric_limits<int>::max(); 
	QString candidate;
	for (int i = 1; i < theMax; i++) {
		candidate = QString("variant %1").arg(i);
		if (!variants.contains(candidate, Qt::CaseInsensitive)) break;
	}

	return candidate;
}

void PEMainWindow::updateAssignedConnectors() {
	if (m_currentGraphicsView == NULL) return;

	QDomDocument * doc = m_viewThings.value(m_currentGraphicsView->viewID())->document;
	if (doc) m_peToolView->showAssignedConnectors(doc, m_currentGraphicsView->viewID());
}

void PEMainWindow::connectorWarning() {
	QHash<ViewLayer::ViewID, int> unassigned;
	foreach (ViewLayer::ViewID viewID, m_viewThings.keys()) {
		unassigned.insert(viewID, 0);
	}
	int unassignedTotal = 0;

	QDomElement fzpRoot = m_fzpDocument.documentElement();
	QDomElement connectors = fzpRoot.firstChildElement("connectors");
	foreach (ViewLayer::ViewID viewID, m_viewThings.keys()) {
		if (viewID == ViewLayer::IconView) continue;
	
		QDomDocument * svgDoc = m_viewThings.value(viewID)->document;
		QDomElement svgRoot = svgDoc->documentElement();
		QDomElement connector = connectors.firstChildElement("connector");
		while (!connector.isNull()) {
			QString svgID, terminalID;
			if (ViewLayer::getConnectorSvgIDs(connector, viewID, svgID, terminalID)) {
				QDomElement element = TextUtils::findElementWithAttribute(svgRoot, "id", svgID);
				if (element.isNull()) {
					unassigned.insert(viewID, 1 + unassigned.value(viewID));
					unassignedTotal++;
				}
			}
			else {
				unassigned.insert(viewID, 1 + unassigned.value(viewID));
				unassignedTotal++;
			}

			connector = connector.nextSiblingElement("connector");
		}
	}

	if (unassignedTotal > 0) {
		int viewCount = 0;
		foreach (ViewLayer::ViewID viewID, unassigned.keys()) {
			if (unassigned.value(viewID) > 0) viewCount++;
		}
		QMessageBox::warning(NULL, tr("Parts Editor"), 
			tr("This part has %n unassigned connectors ", "", unassignedTotal) +
			tr("across %n views. ", "", viewCount) +
			tr("Until all connectors are assigned to SVG elements, the part will not work correctly. ") +
			tr("Exiting the Parts Editor now is fine, as long as you remember to finish the assignments later.")
		);
	}

}

void PEMainWindow::showing(SketchWidget * sketchWidget) {
	ViewThing * viewThing = m_viewThings.value(sketchWidget->viewID());
	if (viewThing->firstTime) {
		viewThing->firstTime = false;
		QPointF offset = viewThing->sketchWidget->alignOneToGrid(viewThing->itemBase);
		if (offset.x() != 0 || offset.y() != 0) {
			QList<PEGraphicsItem *> pegiList = getPegiList(sketchWidget);
			foreach (PEGraphicsItem * pegi, pegiList) {
				pegi->setPos(pegi->pos() + offset);
				pegi->setOffset(pegi->offset() + offset);
			}
		}
	}	
}

bool PEMainWindow::anyMarquee() {
    if (m_currentGraphicsView == NULL) return false;

    QList<PEGraphicsItem *> pegiList = getPegiList(m_currentGraphicsView);
    foreach (PEGraphicsItem * pegi, pegiList) {
        if (pegi->showingMarquee()) {
            return true;
        }
    }

    return false;
}

bool PEMainWindow::anyVisible() {
    if (m_currentGraphicsView == NULL) return false;

    foreach (QGraphicsItem * item, m_currentGraphicsView->scene()->items()) {
        PEGraphicsItem * pegi = dynamic_cast<PEGraphicsItem *>(item);
        if (pegi == NULL) continue;
		
		return pegi->isVisible();
    }

    return false;
}

void PEMainWindow::changeReferenceFile(ViewLayer::ViewID viewID, const QString referenceFile)
{
    ViewThing * viewThing = m_viewThings.value(viewID);
    if (viewThing == NULL) {
        // shouldn't happen
        DebugDialog::debug(QString("missing view thing for %1").arg(viewID));
        return;
    }

    viewThing->referenceFile = referenceFile;
}

void PEMainWindow::insertDesc(const QString & referenceFile, QString & svg) {
    if (svg.contains(ReferenceFileString)) return;

    int ix = svg.indexOf("<svg");
    if (ix >= 0) {
        int jx = svg.indexOf(">", ix);
        if (jx > ix) {
            svg.insert(jx + 1, makeDesc(referenceFile));
        }
    }
}

void PEMainWindow::itemAddedSlot(ModelPart *, ItemBase * itemBase, ViewLayer::ViewLayerPlacement, const ViewGeometry &, long id, SketchWidget *) {
    Q_UNUSED(id);

    if (itemBase == NULL) return;
    if (itemBase->viewID() != m_currentGraphicsView->viewID()) return;

    QDomElement element;
    double z = 0;
    foreach (PEGraphicsItem * pegi, getPegiList(m_currentGraphicsView)) {
        if (pegi->zValue() > z) z = pegi->zValue();
    }

    QRectF bounds = itemBase->boundingRect();
    makePegi(bounds.size(), QPointF(0, 0), itemBase, element, z + 1);
}

void PEMainWindow::itemMovedSlot(ItemBase * itemBase) {
    if (itemBase == NULL) return;
    if (itemBase->viewID() != m_currentGraphicsView->viewID()) return;
  
    foreach (PEGraphicsItem * pegi, getPegiList(m_currentGraphicsView)) {
        if (pegi->itemBase() == itemBase) {
            pegi->setPos(itemBase->pos() + pegi->offset());
        }
    }

}

void PEMainWindow::resizedSlot(ItemBase * itemBase) {
    if (itemBase == NULL) return;
    if (itemBase->viewID() != m_currentGraphicsView->viewID()) return;
  
    foreach (PEGraphicsItem * pegi, getPegiList(m_currentGraphicsView)) {
        if (pegi->itemBase() == itemBase) {
            pegi->setPos(itemBase->pos() + pegi->offset());
            QRectF bounds = itemBase->boundingRect();
            pegi->setRect(bounds);
        }
    }
}

void PEMainWindow::clickedItemCandidateSlot(QGraphicsItem * item, bool & ok) {
    PEGraphicsItem * pegi = dynamic_cast<PEGraphicsItem *>(item);
    if (pegi == NULL) {
        ok = true;
        return;
    }

    ok = pegi->showingMarquee();
}

void PEMainWindow::initProgrammingWidget() {
}

void PEMainWindow::initWelcomeView() {
}

void PEMainWindow::setInitialView() {
    	// do this the first time, since the current_changed signal wasn't sent
	int tab = 0;
	tabWidget_currentChanged(tab+1);
	tabWidget_currentChanged(tab);
}

void PEMainWindow::updateExportMenu() {
    foreach (QAction * action, m_exportMenu->actions()) {
        action->setEnabled(false);
    }
}

void PEMainWindow::convertToTenth() {
    if (m_currentGraphicsView == NULL) return;
    if (m_currentGraphicsView->viewID() != ViewLayer::SchematicView) return;

    QString originalFzpPath = saveFzp();
    QString newFzpPath = saveFzp();

	ViewThing * viewThing = m_viewThings.value(m_currentGraphicsView->viewID());
    QString originalSvgPath = viewThing->itemBase->filename();
    QString newSvgPath = m_userPartsFolderSvgPath + makeSvgPath2(m_currentGraphicsView);
    QFile::copy(originalSvgPath, newSvgPath);

    S2S s2s(false);
    connect(&s2s, SIGNAL(messageSignal(const QString &)), this, SLOT(s2sMessageSlot(const QString &)));
    bool result = s2s.onefzp(newFzpPath, newSvgPath);

    if (!result) return;          // if conversion fails

    QUndoCommand * parentCommand = new QUndoCommand("Convert Schematic");
    new ChangeFzpCommand(this, originalFzpPath, newFzpPath, parentCommand);
    new ChangeSvgCommand(this, m_currentGraphicsView, originalSvgPath, newSvgPath, parentCommand);
    m_undoStack->waitPush(parentCommand, SketchWidget::PropChangeDelay);
}

void PEMainWindow::s2sMessageSlot(const QString & message) {
    QMessageBox::information(this, "Schematic Conversion", message);
}

void PEMainWindow::updateEditMenu() {
    MainWindow::updateEditMenu();
    m_convertToTenthAct->setEnabled(m_currentGraphicsView != NULL && m_currentGraphicsView->viewID() == ViewLayer::SchematicView);
}

