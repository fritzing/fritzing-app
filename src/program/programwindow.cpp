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

/////////////////////////////////////////////
//
// TODO
//
//  integrate dirty
//  remove old program window
//  enable all buttons, and give error messages (i.e. where is IDE)


#include "programwindow.h"
#include "highlighter.h"
#include "syntaxer.h"
#include "programtab.h"
#include "platformarduino.h"
#include "platformpicaxe.h"

#include "../debugdialog.h"
#include "../waitpushundostack.h"
#include "../utils/folderutils.h"

#include <QFileInfoList>
#include <QFileInfo>
#include <QRegExp>
#include <QSettings>
#include <QFontMetrics>
#include <QTextStream>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QApplication>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QPrinter>
#include <QPrintDialog>
#include <QtSerialPort/qserialportinfo.h>
#include <QtSerialPort/qserialport.h>

///////////////////////////////////////////////

PTabWidget::PTabWidget(QWidget * parent) : QTabWidget(parent) {
        m_lastTabIndex = -1;

        connect(tabBar(), SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
}

QTabBar * PTabWidget::tabBar() {
	return QTabWidget::tabBar();
}

void PTabWidget::tabChanged(int index) {
    // Hide the close button on the old tab
    if (m_lastTabIndex >= 0) {
        QAbstractButton *tabButton = qobject_cast<QAbstractButton *>(tabBar()->tabButton(m_lastTabIndex, QTabBar::LeftSide));
        if (!tabButton) {
			tabButton = qobject_cast<QAbstractButton *>(tabBar()->tabButton(m_lastTabIndex, QTabBar::RightSide));
        }

        if (tabButton) {
            tabButton->hide();
        }
    }

    m_lastTabIndex = index;

    // Show the close button on the new tab
    if (m_lastTabIndex >= 0) {
        QAbstractButton *tabButton = qobject_cast<QAbstractButton *>(tabBar()->tabButton(m_lastTabIndex, QTabBar::LeftSide));
        if (!tabButton) {
            tabButton = qobject_cast<QAbstractButton *>(tabBar()->tabButton(m_lastTabIndex, QTabBar::RightSide));
        }
        if (tabButton) {
            tabButton->show();
        }
    }
}

///////////////////////////////////////////////

static int UntitledIndex = 1;
QList<Platform *> ProgramWindow::m_platforms;
QString ProgramWindow::NoBoardName;

ProgramWindow::ProgramWindow(QWidget *parent)
    : FritzingWindow("", untitledFileCount(), "", parent)
{
    QFile styleSheet(":/resources/styles/programwindow.qss");

    this->setObjectName("programmingWindow");
    if (!styleSheet.open(QIODevice::ReadOnly)) {
        qWarning("Unable to open :/resources/styles/programwindow.qss");
    } else {
        QString ss = styleSheet.readAll();
#ifdef Q_OS_MAC
                int paneLoc = 4;
                int tabBarLoc = 0;
#else
                int paneLoc = -1;
                int tabBarLoc = 5;
#endif
                ss = ss.arg(paneLoc).arg(tabBarLoc);
        this->setStyleSheet(ss);
    }

    if (m_platforms.count() == 0) {
        initPlatforms();
	}

    if (NoBoardName.isEmpty()) {
        NoBoardName = tr("No boards available");
    }

	m_savingProgramTab = NULL;
	UntitledIndex--;						// incremented by FritzingWindow
	ProgramWindow::setTitle();				// set to something weird by FritzingWindow
}

ProgramWindow::~ProgramWindow()
{
}

void ProgramWindow::setup()
{
    if (parentWidget() == NULL) {
        resize(500,700);
        setAttribute(Qt::WA_DeleteOnClose, true);
    }

    QFrame * mainFrame =  new QFrame(this);

	QFrame * headerFrame = createHeader();
	QFrame * centerFrame = createCenter();

	layout()->setMargin(0);
	layout()->setSpacing(0);

	QGridLayout *layout = new QGridLayout(mainFrame);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(headerFrame,0,0);
	layout->addWidget(centerFrame,1,0);

	setCentralWidget(mainFrame);

    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

	QSettings settings;
	if (!settings.value("programwindow/state").isNull()) {
		restoreState(settings.value("programwindow/state").toByteArray());
	}
	if (!settings.value("programwindow/geometry").isNull()) {
		restoreGeometry(settings.value("programwindow/geometry").toByteArray());
	}

	installEventFilter(this);
}

void ProgramWindow::initMenus(QMenuBar * menubar) {
    QAction *currentAction;

    m_editMenu = menubar->addMenu(tr("&Edit"));

    m_undoAction = new QAction(tr("Undo"), this);
    m_undoAction->setShortcuts(QKeySequence::Undo);
    m_undoAction->setEnabled(false);
    connect(m_undoAction, SIGNAL(triggered()), this, SLOT(undo()));
    m_editMenu->addAction(m_undoAction);

    m_redoAction = new QAction(tr("Redo"), this);
    m_redoAction->setShortcuts(QKeySequence::Redo);
    m_redoAction->setEnabled(false);
    connect(m_redoAction, SIGNAL(triggered()), this, SLOT(redo()));
    m_editMenu->addAction(m_redoAction);

    m_editMenu->addSeparator();

    m_cutAction = new QAction(tr("&Cut"), this);
    m_cutAction->setShortcut(QKeySequence::Cut);
    m_cutAction->setStatusTip(tr("Cut selection"));
    m_cutAction->setEnabled(false);
    connect(m_cutAction, SIGNAL(triggered()), this, SLOT(cut()));
    m_editMenu->addAction(m_cutAction);

    m_copyAction = new QAction(tr("&Copy"), this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setStatusTip(tr("Copy selection"));
    m_copyAction->setEnabled(false);
    connect(m_copyAction, SIGNAL(triggered()), this, SLOT(copy()));
    m_editMenu->addAction(m_copyAction);

    m_pasteAction = new QAction(tr("&Paste"), this);
    m_pasteAction->setShortcut(QKeySequence::Paste);
    m_pasteAction->setStatusTip(tr("Paste clipboard contents"));
    // TODO: Check clipboard status and disable appropriately here
    connect(m_pasteAction, SIGNAL(triggered()), this, SLOT(paste()));
    m_editMenu->addAction(m_pasteAction);

    m_editMenu->addSeparator();

    m_selectAction = new QAction(tr("&Select All"), this);
    m_selectAction->setShortcut(QKeySequence::SelectAll);
    m_selectAction->setStatusTip(tr("Select all text"));
    connect(m_selectAction, SIGNAL(triggered()), this, SLOT(selectAll()));
    m_editMenu->addAction(m_selectAction);

    m_editMenu->addSeparator();

    m_preferencesAction = new QAction(tr("&Preferences..."), this);
    m_preferencesAction->setStatusTip(tr("Show the application's about box"));
    connect(m_preferencesAction, SIGNAL(triggered()), QApplication::instance(), SLOT(preferences()));
    m_editMenu->addAction(m_preferencesAction);

    m_programMenu = menubar->addMenu(tr("&Code"));

    m_newAction = new QAction(tr("&New Tab"), this);
    m_newAction->setShortcut(QKeySequence::AddTab);
    m_newAction->setStatusTip(tr("Create a new program tab"));
    connect(m_newAction, SIGNAL(triggered()), this, SLOT(addTab()));
    m_programMenu->addAction(m_newAction);

    m_openAction = new QAction(tr("&Import Code..."), this);
    m_openAction->setShortcut(tr("Alt+Ctrl+I"));
    m_openAction->setStatusTip(tr("Import a program from a file"));
    connect(m_openAction, SIGNAL(triggered()), this, SLOT(loadProgramFile()));
    m_programMenu->addAction(m_openAction);

    m_saveAction = new QAction(tr("&Save Tab"), this);
    m_saveAction->setShortcut(tr("Alt+Ctrl+S"));
    m_saveAction->setStatusTip(tr("Save the current program tab"));
    connect(m_saveAction, SIGNAL(triggered()), this, SLOT(saveCurrentTab()));
    m_programMenu->addAction(m_saveAction);

    currentAction = new QAction(tr("&Rename Tab"), this);
    currentAction->setShortcut(tr("Alt+Ctrl+R"));
    currentAction->setStatusTip(tr("Rename the current program tab"));
    connect(currentAction, SIGNAL(triggered()), this, SLOT(rename()));
    m_programMenu->addAction(currentAction);

    currentAction = new QAction(tr("Close Tab"), this);
    currentAction->setShortcut(tr("Alt+Ctrl+W"));
    currentAction->setStatusTip(tr("Remove the current program tab from the sketch"));
    connect(currentAction, SIGNAL(triggered()), this, SLOT(closeCurrentTab()));
    m_programMenu->addAction(currentAction);

    m_programMenu->addSeparator();

    m_platformMenu = new QMenu(tr("Platform"), this);
    m_programMenu->addMenu(m_platformMenu);
    QSettings settings;
    QString currentPlatform = settings.value("programwindow/platform", "").toString();
    QList<Platform *> platforms = getAvailablePlatforms();
    m_platformActionGroup = new QActionGroup(this);
    foreach (Platform * platform, platforms) {
        currentAction = new QAction(platform->getName(), this);
        currentAction->setCheckable(true);
        m_platformActions.insert(platform, currentAction);
        m_platformActionGroup->addAction(currentAction);
        m_platformMenu->addAction(currentAction);
        if (!currentPlatform.isEmpty()) {
            if (platform->getName().compare(currentPlatform) == 0) {
				currentAction->setChecked(true);
			}
		}
    }
    connect(m_platformMenu, SIGNAL(triggered(QAction*)), this, SLOT(setPlatform(QAction*)));

    m_boardMenu = new QMenu(tr("Board"), this);
    m_programMenu->addMenu(m_boardMenu);
    m_boardActionGroup = new QActionGroup(this);
    updateBoards();
    connect(m_boardMenu, SIGNAL(triggered(QAction*)), this, SLOT(setBoard(QAction*)));

    m_serialPortMenu = new QMenu(tr("Port"), this);
    m_programMenu->addMenu(m_serialPortMenu);
    m_serialPortActionGroup = new QActionGroup(this);
    updateSerialPorts();
    connect(m_serialPortMenu, SIGNAL(triggered(QAction*)), this, SLOT(setPort(QAction*)));
    connect(m_serialPortMenu, SIGNAL(aboutToShow()), this, SLOT(updateSerialPorts()), Qt::DirectConnection);

    m_programMenu->addSeparator();

    m_monitorAction = new QAction(tr("Serial Monitor"), this);
    m_monitorAction->setShortcut(tr("Ctrl+M"));
    m_monitorAction->setStatusTip(tr("Monitor the serial port communication"));
    m_monitorAction->setEnabled(false);
    connect(m_monitorAction, SIGNAL(triggered()), this, SLOT(serialMonitor()));
    m_programMenu->addAction(m_monitorAction);

    m_programAction = new QAction(tr("Upload"), this);
    m_programAction->setShortcut(tr("Ctrl+U"));
    m_programAction->setStatusTip(tr("Upload the current program onto a microcontroller"));
    m_programAction->setEnabled(false);
    connect(m_programAction, SIGNAL(triggered()), this, SLOT(sendProgram()));
    m_programMenu->addAction(m_programAction);

    m_viewMenu = menubar->addMenu(tr("&View"));
    foreach (QAction * action, m_viewMenuActions) {
        m_viewMenu->addAction(action);
    }

    addTab(); // the initial ProgramTab must be created after all actions are set up
}

void ProgramWindow::showMenus(bool show) {
    if (m_editMenu) {
        m_editMenu->menuAction()->setVisible(show);
        m_editMenu->setEnabled(show);
        m_undoAction->setEnabled(show);
        m_redoAction->setEnabled(show);
        m_cutAction->setEnabled(show);
        m_copyAction->setEnabled(show);
        m_pasteAction->setEnabled(show);
        m_selectAction->setEnabled(show);
    }
    if (m_programMenu) {
        m_programMenu->menuAction()->setVisible(show);
        m_programMenu->setEnabled(show);
    }
    if (m_viewMenu) {
        m_viewMenu->menuAction()->setVisible(show);
        m_viewMenu->setEnabled(show);
    }
}

void ProgramWindow::createViewMenuActions(QList<QAction *> & actions) {
    m_viewMenuActions = actions;
}

void ProgramWindow::linkFiles(const QList<LinkedFile *> & linkedFiles, const QString & alternativePath) {
    if (linkedFiles.isEmpty()) return;

    bool firstTime = true;
    foreach (LinkedFile * linkedFile, linkedFiles) {
        ProgramTab * programTab = NULL;
        if (firstTime) {
            firstTime = false;
            programTab = indexWidget(0);
        }
        else {
            programTab = addTab();
        }
        QDir dir(alternativePath);
        QFileInfo fileInfo(linkedFile->linkedFilename);
        programTab->loadProgramFile(linkedFile->linkedFilename, dir.absoluteFilePath(fileInfo.fileName()), false);
		if ((linkedFile->fileFlags & LinkedFile::InBundleFlag) && ((linkedFile->fileFlags & LinkedFile::ReadOnlyFlag) == 0)) { 
			if (linkedFile->fileFlags & LinkedFile::SameMachineFlag) {
				programTab->appendToConsole(tr("File '%1' was restored from the .fzz file; the local copy was not found.").arg(fileInfo.fileName()));
			}
			else {
				programTab->appendToConsole(tr("File '%1' was restored from the .fzz file; save a local copy to work with an external editor.").arg(fileInfo.fileName()));
			}
		}
        if (hasPlatform(linkedFile->platform)) {
            programTab->setPlatform(linkedFile->platform, false);
		}
		else {
            linkedFile->platform.clear();
		}
    }
}

QFrame * ProgramWindow::createHeader() {
	QFrame * headerFrame = new QFrame();
	headerFrame->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed));
	headerFrame->setObjectName("header");

	return headerFrame;
}

QFrame * ProgramWindow::createCenter() {

    QFrame * centerFrame = new QFrame(this);
	centerFrame->setObjectName("center");

	m_tabWidget = new PTabWidget(centerFrame);
	m_tabWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
	m_tabWidget->setMovable(true);
    m_tabWidget->setTabsClosable(true);
	m_tabWidget->setUsesScrollButtons(false);
	m_tabWidget->setElideMode(Qt::ElideLeft);
     connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

    //addTab();

	QGridLayout *tabLayout = new QGridLayout(m_tabWidget);
	tabLayout->setMargin(0);
	tabLayout->setSpacing(0);

	QGridLayout *mainLayout = new QGridLayout(centerFrame);
	mainLayout->setMargin(0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(m_tabWidget,0,0,1,1);

	return centerFrame;
}

void ProgramWindow::cleanUp() {
}

/**
 * eventFilter here is used to catch the keyboard shortcuts that trigger the close event
 * for ProgramWindow. If there are more than one tab widget the standard close shortcut
 * should only close the current tab instead of closing the whole window. Every other
 * case is ignored and handled by closeEvent() like normal.
 */
bool ProgramWindow::eventFilter(QObject * object, QEvent * event) {
        if (object == this && event->type() == QEvent::ShortcutOverride) {
                QKeyEvent *keyEvent = dynamic_cast<QKeyEvent*>(event);
                if(keyEvent && keyEvent->matches(QKeySequence::Close) && m_tabWidget->count() > 1 ) {
                        return true;
                }
        }
        return QMainWindow::eventFilter(object, event);
}

/**
 * Reimplement closeEvent to save any modified documents before closing.
 */
void ProgramWindow::closeEvent(QCloseEvent *event) {
	bool discard;
	if(beforeClosing(true, discard)) {
		cleanUp();
		QMainWindow::closeEvent(event);
		emit closed();
	} else {
		event->ignore();
	}

	QSettings settings;
	settings.setValue("programwindow/state",saveState());
	settings.setValue("programwindow/geometry",saveGeometry());
}

const QString ProgramWindow::untitledFileName() {
	return "Untitled";
}

const QString ProgramWindow::fileExtension() {
	return "";
}

const QString ProgramWindow::defaultSaveFolder() {
	return FolderUtils::openSaveFolder();
}

bool ProgramWindow::event(QEvent * e) {
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

int & ProgramWindow::untitledFileCount() {
	return UntitledIndex;
}

void ProgramWindow::setTitle() {
    setWindowTitle(tr("Code Window"));
}

void ProgramWindow::setTitle(const QString & filename) {
        setWindowTitle(tr("Code Window - %1").arg(filename));
}

/**
 * Create and open a new tab within the PTabWidget child.
 * This function handled connecting all the appropriate signals
 * and setting an appropriate filename.
 */
ProgramTab * ProgramWindow::addTab() {
    QString name = (UntitledIndex == 1) ? untitledFileName() : tr("%1%2").arg(untitledFileName()).arg(UntitledIndex);
    ProgramTab * programTab = new ProgramTab(name, m_tabWidget);
    connect(programTab, SIGNAL(wantToSave(int)), this, SLOT(tabSave(int)));
    connect(programTab, SIGNAL(wantToSaveAs(int)), this, SLOT(tabSaveAs(int)));
    connect(programTab, SIGNAL(wantToRename(int)), this, SLOT(tabRename(int)));
    connect(programTab, SIGNAL(wantToDelete(int, bool)), this, SLOT(tabDelete(int, bool)), Qt::DirectConnection);
    connect(programTab, 
        SIGNAL(programWindowUpdateRequest(bool, bool, bool, bool, bool, bool, Platform *, const QString &, const QString &, const QString &)),
		this, 
        SLOT(updateMenu(bool, bool, bool, bool, bool, bool, Platform *, const QString &, const QString &, const QString &)));
	int ix = m_tabWidget->addTab(programTab, name);
    m_tabWidget->setCurrentIndex(ix);
    programTab->initMenus();
	UntitledIndex++;

	return programTab;
}

/**
 * A function for closing the current displayed tab.
 * I'm using a ProgramWindow method instead of calling deleteTab()
 * directly as I believe it's more apropos.
 */
void ProgramWindow::closeCurrentTab() {
        closeTab(m_tabWidget->currentIndex());
}

void ProgramWindow::closeTab(int index) {
        ProgramTab * pTab = indexWidget(index);
        if (pTab) {
            emit linkToProgramFile(pTab->filename(), NULL, false, true);
                pTab->deleteTab();
        }
}

/**
 * This slot is for updating the tab-dependent menu items.
 *  - program
 *  - undo/redo
 *  - cut/copy
 */
void ProgramWindow::updateMenu(bool programEnable, bool undoEnable, bool redoEnable, bool cutEnable, bool copyEnable, bool pasteEnable,
                              Platform* platform, const QString & port, const QString & board, const QString & filename)
{
	ProgramTab * programTab = currentWidget();
	m_saveAction->setEnabled(programTab->isModified());
    m_monitorAction->setEnabled(!port.isEmpty());
    m_programAction->setEnabled(programEnable);
    m_undoAction->setEnabled(undoEnable);
    m_redoAction->setEnabled(redoEnable);
    m_cutAction->setEnabled(cutEnable);
    m_copyAction->setEnabled(copyEnable);
    m_pasteAction->setEnabled(pasteEnable);
    QAction *lang = m_platformActions.value(platform);
    if (lang) {
        lang->setChecked(true);
    }
    QAction *portAction = m_portActions.value(port);
    if (portAction) {
        portAction->setChecked(true);
    }
    QAction *boardAction = m_boardActions.value(board);
    if (boardAction) {
        boardAction->setChecked(true);
    }

    setTitle(filename);
}

void ProgramWindow::setPlatform(QAction* action) {
    currentWidget()->setPlatform(action->text());
}

void ProgramWindow::setPort(QAction* action) {
    currentWidget()->setPort(action->text());
}

void ProgramWindow::setBoard(QAction* action) {
    currentWidget()->setBoard(action->text());
}

bool ProgramWindow::beforeClosing(bool showCancel, bool & discard) {
	discard = false;
	for (int i = 0; i < m_tabWidget->count(); i++) {
		if (!beforeClosingTab(i, showCancel)) {
			return false;
		}
	}

	return true;
}

bool ProgramWindow::beforeClosingTab(int index, bool showCancel) 
{
	ProgramTab * programTab = indexWidget(index);
	if (programTab == NULL) return true;

	if (!programTab->isModified()) return true;

	QMessageBox::StandardButton reply = beforeClosingMessage(programTab->filename(), showCancel);
	if (reply == QMessageBox::Save) {
		return prepSave(programTab, false);
	} 
	
	if (reply == QMessageBox::Discard) {
		return true;
	}
 		
	return false;
}

void ProgramWindow::print() {
#ifndef QT_NO_PRINTER
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
        currentWidget()->print(printer);
    } 
#endif
}


// overrides MainWindow::saveAsAux
bool ProgramWindow::saveAsAux(const QString & fileName) {
	if (!m_savingProgramTab) return false;

    bool result = m_savingProgramTab->save(fileName);
	m_savingProgramTab = NULL;
	return result;
}

void ProgramWindow::tabDelete(int index, bool deleteFile) {
	ProgramTab * programTab = indexWidget(index);
    QString fname = programTab->filename();
	m_tabWidget->removeTab(index);
	if (m_tabWidget->count() == 0) {
		addTab();
	}

	if (deleteFile) {
		QFile file(fname);
		file.remove();
	}
}

void ProgramWindow::saveAll() {
    for (int i= 0; i < m_tabWidget->count(); i++)
        tabSave(i);
}

void ProgramWindow::saveCurrentTab() {
    tabSave(m_tabWidget->currentIndex());
}

void ProgramWindow::tabSave(int index) {
	ProgramTab * programTab =indexWidget(index);
	if (programTab == NULL) return;

    prepSave(programTab, false);
}

void ProgramWindow::tabSaveAs(int index) {
	ProgramTab * programTab = indexWidget(index);
    if (programTab == NULL) return;

    prepSave(programTab, true);
}

void ProgramWindow::tabRename(int index) {
    ProgramTab * programTab = indexWidget(index);
    if (programTab == NULL) return;

    QString oldFileName = programTab->filename();
    if (prepSave(programTab, true)) {
		if (programTab->filename() != oldFileName) {
			QFile oldFile(oldFileName);
			if (oldFile.exists()) {
				oldFile.remove();
                emit linkToProgramFile(oldFileName, NULL, false, true);
			}
		}
    }
}

void ProgramWindow::duplicateTab() {
        ProgramTab * oldTab = currentWidget();
        if (oldTab == NULL) return;

        ProgramTab * newTab = addTab();

        newTab->setText(oldTab->text());
}

void ProgramWindow::tabBeforeClosing(int index, bool & ok) {
	ok = beforeClosingTab(index, true);
}

bool ProgramWindow::prepSave(ProgramTab * programTab, bool saveAsFlag) 
{
	m_savingProgramTab = programTab;				// need this for the saveAsAux call
    if (!programTab->isModified()) return false;

	bool result = (saveAsFlag) 
		? saveAs(programTab->filename(), programTab->readOnly())
		: save(programTab->filename(), programTab->readOnly());

    if (result) {
		programTab->setClean();
        emit linkToProgramFile(programTab->filename(), programTab->platform(), true, true);
	}
	return result;
}

void ProgramWindow::initPlatforms() {
    QDir dir(FolderUtils::getApplicationSubFolderPath("translations"));
    Highlighter::loadStyles(dir.absolutePath().append("/syntax/styles.xml"));

    m_platforms << new PlatformArduino() << new PlatformPicaxe();
}

QList<Platform *> ProgramWindow::getAvailablePlatforms() {
    return m_platforms;
}

Platform * ProgramWindow::getPlatformByName(const QString & platformName) {
    foreach (Platform * platform, getAvailablePlatforms()) {
        if (platform->getName().compare(platformName, Qt::CaseInsensitive) == 0)
            return platform;
    }
    return NULL;
}

bool ProgramWindow::hasPlatform(const QString & platformName) {
    return getPlatformByName(platformName) != NULL;
}

const QMap<QString, QString> ProgramWindow::getBoards() {
    if (currentWidget() && currentWidget()->platform())
        return currentWidget()->platform()->getBoards();

    QMap<QString, QString> boards;
    boards.insert(NoBoardName, NoBoardName);
    return boards;
}

QAction * ProgramWindow::addBoard(const QString & name, const QString & definition)
{
    QAction * currentAction = new QAction(name, this);
    currentAction->setCheckable(true);
    currentAction->setData(definition);
    m_boardActions.insert(name, currentAction);
    m_boardMenu->addAction(currentAction);
    m_boardActionGroup->addAction(currentAction);
    return currentAction;
}

void ProgramWindow::updateBoards() {
    QMap<QString, QString> boards = getBoards();

    m_boardActions.clear();
    foreach (QAction * action, m_boardActionGroup->actions())
        m_boardActionGroup->removeAction(action);
    m_boardMenu->clear();

    QMapIterator<QString, QString> i(boards);
     while (i.hasNext()) {
         i.next();
         addBoard(i.key(), i.value());
     }
}

void ProgramWindow::loadProgramFile() {
    DebugDialog::debug("loading program file");
	currentWidget()->loadProgramFile();
	
}

void ProgramWindow::loadProgramFileNew() {
	ProgramTab * programTab = addTab();
	if (programTab) {
		if (!programTab->loadProgramFile()) {
			delete programTab;
		}
	}
}

void ProgramWindow::rename() {
	 currentWidget()->rename();
}

void ProgramWindow::undo() {
	 currentWidget()->undo();
}

void ProgramWindow::redo() {
	 currentWidget()->redo();
}

void ProgramWindow::cut() {
	 currentWidget()->cut();
}

void ProgramWindow::copy() {
	 currentWidget()->copy();
}

void ProgramWindow::paste() {
	 currentWidget()->paste();
}

void ProgramWindow::selectAll() {
	 currentWidget()->selectAll();
}

void ProgramWindow::serialMonitor() {
     currentWidget()->serialMonitor();
}

void ProgramWindow::sendProgram() {
	 currentWidget()->sendProgram();
}

ProgramTab * ProgramWindow::currentWidget() {
	return qobject_cast<ProgramTab *>(m_tabWidget->currentWidget());
}

ProgramTab * ProgramWindow::indexWidget(int index) {
	return qobject_cast<ProgramTab *>(m_tabWidget->widget(index));
}

bool ProgramWindow::alreadyHasProgram(const QString & filename) {
    DebugDialog::debug("already has program");
	for (int i = 0; i < m_tabWidget->count(); i++) {
		ProgramTab * tab = indexWidget(i);
		if (tab->filename() == filename) {
			m_tabWidget->setCurrentIndex(i);
			return true;
		}
	}

	return false;
}

QString ProgramWindow::getExtensionString() {
	ProgramTab * pt = currentWidget();
	if (pt == NULL) return "";

	return pt->extensionString();
}

QStringList ProgramWindow::getExtensions() {
	ProgramTab * pt = currentWidget();
	if (pt == NULL) return QStringList();

	return pt->extensions();
}

QList<QSerialPortInfo> ProgramWindow::getSerialPorts() {
    QList<QSerialPortInfo> ports;
    ports = QSerialPortInfo::availablePorts();

    /*
    // on the pc, handy for testing the UI when there are no serial ports
    ports.removeOne("COM0");
    ports.removeOne("COM1");
    ports.removeOne("COM2");
    ports.removeOne("COM3");
    */

    return ports;
}

void ProgramWindow::updateSerialPorts() {
    QList<QSerialPortInfo> ports = getSerialPorts();

    m_portActions.clear();
    foreach (QAction * action, m_serialPortActionGroup->actions())
        m_serialPortActionGroup->removeAction(action);
    m_serialPortMenu->clear();

    foreach (QSerialPortInfo port, ports) {
        addPort(port);
    }
}

QAction * ProgramWindow::addPort(QSerialPortInfo port)
{
    QAction * currentAction = new QAction(port.portName(), this);
    currentAction->setCheckable(true);
    currentAction->setData(port.systemLocation());
    m_portActions.insert(port.portName(), currentAction);
    m_serialPortMenu->addAction(currentAction);
    m_serialPortActionGroup->addAction(currentAction);
    return currentAction;
}

bool ProgramWindow::hasPort(const QString & portName) {
    foreach (QSerialPortInfo port, getSerialPorts()) {
        if (port.portName().compare(portName) == 0)
            return true;
    }
    return false;
}

void ProgramWindow::updateLink(const QString & filename, Platform * platform, bool addlink, bool strong)
{
    DebugDialog::debug("updating link");
    emit linkToProgramFile(filename, platform, addlink, strong);
}

void ProgramWindow::portProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
	DebugDialog::debug(QString("process finished %1 %2").arg(exitCode).arg(exitStatus));

	// parse the text and update the combo box

	sender()->deleteLater();
}

void ProgramWindow::portProcessReadyRead() {
	m_ports.clear();

	QByteArray byteArray = qobject_cast<QProcess *>(sender())->readAllStandardOutput();
    QTextStream textStream(byteArray, QIODevice::ReadOnly);
    while (true) {
        QString line = textStream.readLine();
        if (line.isNull()) break;

        if (!line.contains("tty")) continue;
        if (!line.contains("serial", Qt::CaseInsensitive)) continue;

        QStringList candidates = line.split(" ");
        foreach (QString candidate, candidates) {
            if (candidate.contains("tty")) {
                m_ports.append(candidate);
                break;
            }
        }
    }
}
