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

#include "programtab.h"
#include "highlighter.h"
#include "syntaxer.h"
#include "consolewindow.h"
#include "../debugdialog.h"
#include "../utils/folderutils.h"
#include "../sketchtoolbutton.h"


#include <QFileInfoList>
#include <QFileInfo>
#include <QRegExp>
#include <QSettings>
#include <QFontMetrics>
#include <QTextStream>
#include <QMessageBox>
#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>

static const QChar Quote91Char(0x91);
static QString UnableToProgramMessage;

/////////////////////////////////////////

SerialPortComboBox::SerialPortComboBox() : QComboBox() {
}

void SerialPortComboBox::showPopup() {
	emit aboutToShow();
	QComboBox::showPopup();
}

/////////////////////////////////////////

DeleteDialog::DeleteDialog(const QString & title, const QString & text, bool deleteFileCheckBox, QWidget *parent, Qt::WindowFlags flags) : QDialog(parent, flags)
{
	// we use this dialog instead of the normal QMessageBox because it's currently hard if not impossible to add a checkbox to QMessageBox.
	// supposedly this will be fixed in a future release of Qt.

    this->setWindowTitle(title);
	this->setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |  Qt::WindowCloseButtonHint);

	QVBoxLayout * vlayout = new QVBoxLayout(this);

	QFrame * frame  = new QFrame(this);
	QHBoxLayout * hlayout = new QHBoxLayout(frame);

    QLabel * iconLabel = new QLabel;
	iconLabel->setPixmap(QMessageBox::standardIcon(QMessageBox::Warning));
    iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QLabel * label = new QLabel;
	label->setWordWrap(true);
	label->setText(text);

	hlayout->addWidget(iconLabel);
	hlayout->addWidget(label);

	vlayout->addWidget(frame);

	m_checkBox = NULL;
	if (deleteFileCheckBox) {
		m_checkBox = new QCheckBox(tr("Also delete the file"));
		vlayout->addSpacing(7);
		vlayout->addWidget(m_checkBox);
		vlayout->addSpacing(15);
	}

    m_buttonBox = new QDialogButtonBox;
	m_buttonBox->addButton(QDialogButtonBox::Yes);
	m_buttonBox->addButton(QDialogButtonBox::No);
	m_buttonBox->button(QDialogButtonBox::Yes)->setText(tr("Remove"));
	m_buttonBox->button(QDialogButtonBox::No)->setText(tr("Don't remove"));

    QObject::connect(m_buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
	vlayout->addWidget(m_buttonBox);

    this->setModal(true);
}

void DeleteDialog::buttonClicked(QAbstractButton * button) {
	this->done(m_buttonBox->standardButton(button));
}

bool DeleteDialog::deleteFileChecked() {
	if (m_checkBox == NULL) return false;

	return m_checkBox->isChecked();
}

/////////////////////////////////////////////

QIcon * AsteriskIcon = NULL;

ProgramTab::ProgramTab(QString & filename, QWidget *parent) : QFrame(parent)
{
    if (UnableToProgramMessage.isEmpty()) {
        UnableToProgramMessage = tr("While it is possible to read and edit %1 programming files, it is not yet possible to use Fritzing to compile or upload these programs to a microcontroller.");
    }

    m_tabWidget = NULL;
	while (parent != NULL) {
		QTabWidget * tabWidget = qobject_cast<QTabWidget *>(parent);
		if (tabWidget) {
			m_tabWidget = tabWidget;
			break;
		}
		parent = parent->parentWidget();
    }

	if (AsteriskIcon == NULL) {
		AsteriskIcon = new QIcon(":/resources/images/icons/asterisk.png");
	}

    m_canCopy = false;
    m_canCut = false;
    m_canUndo = false;
    m_canRedo = false;
    m_canPaste = false;
    //m_platform = NULL;
    m_port = "";
    m_board = "";
    m_filename = filename;

	m_updateEnabled = false;
	QGridLayout *editLayout = new QGridLayout(this);
	editLayout->setMargin(0);
    editLayout->setSpacing(0);

    while (m_programWindow == NULL) {
        m_programWindow = qobject_cast<ProgramWindow *>(parent);
        parent = parent->parentWidget();
    }

    // m_textEdit needs to be initialized before createFooter so
    // some signals get connected properly.
    m_textEdit = new QTextEdit;
    m_textEdit->setObjectName("code");
    m_textEdit->setFontFamily("Droid Sans Mono");
    m_textEdit->setLineWrapMode(QTextEdit::NoWrap);
    QFontMetrics fm(m_textEdit->currentFont());
    m_textEdit->setTabStopWidth(fm.averageCharWidth() * 2);
    m_textEdit->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    m_textEdit->setUndoRedoEnabled(true);
    connect(m_textEdit, SIGNAL(textChanged()), this, SLOT(textChanged()));
    connect(m_textEdit, SIGNAL(undoAvailable(bool)), this, SLOT(enableUndo(bool)));
    connect(m_textEdit, SIGNAL(redoAvailable(bool)), this, SLOT(enableRedo(bool)));
    connect(m_textEdit, SIGNAL(copyAvailable(bool)), this, SLOT(enableCopy(bool)));
    connect(m_textEdit, SIGNAL(copyAvailable(bool)), this, SLOT(enableCut(bool))); // Reuse copy signal for cut
    m_highlighter = new Highlighter(m_textEdit);

	QSplitter * splitter = new QSplitter;
	splitter->setObjectName("splitter");
	splitter->setOrientation(Qt::Vertical);
    editLayout->addWidget(splitter, 1, 0);

	splitter->addWidget(m_textEdit);

    m_console = new QPlainTextEdit();
	m_console->setObjectName("console");
	m_console->setReadOnly(true);
    QFont font = m_console->document()->defaultFont();
    font.setFamily("Droid Sans Mono");
    m_console->document()->setDefaultFont(font);

	splitter->addWidget(m_console);

	splitter->setStretchFactor(0, 8);
    splitter->setStretchFactor(1, 2);

    m_toolbar = new QFrame(this);
    m_toolbar->setObjectName("sketchAreaToolbar");
    m_toolbar->setFixedHeight(66);

    QFrame *leftButtons = new QFrame(m_toolbar);
    m_leftButtonsContainer = new QHBoxLayout(leftButtons);
    m_leftButtonsContainer->setMargin(0);
    m_leftButtonsContainer->setSpacing(0);

    QFrame *middleButtons = new QFrame(m_toolbar);
    middleButtons->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::MinimumExpanding);
    m_middleButtonsContainer = new QHBoxLayout(middleButtons);
    m_middleButtonsContainer->setSpacing(0);
    m_middleButtonsContainer->setMargin(0);

    QFrame *rightButtons = new QFrame(m_toolbar);
    m_rightButtonsContainer = new QHBoxLayout(rightButtons);
    m_rightButtonsContainer->setMargin(0);
    m_rightButtonsContainer->setSpacing(0);

    QHBoxLayout *toolbarLayout = new QHBoxLayout(m_toolbar);
    toolbarLayout->setMargin(0);
    toolbarLayout->setSpacing(0);
    toolbarLayout->addWidget(leftButtons);
    toolbarLayout->addWidget(middleButtons);
    toolbarLayout->addWidget(rightButtons);

    editLayout->addWidget(m_toolbar,2,0);

    //createToolBarMenu();
}

ProgramTab::~ProgramTab()
{
}

/**
 * We override the showEvent for the tab to trigger a menu update
 */
void ProgramTab::showEvent(QShowEvent *event) {
    QFrame::showEvent(event);
    m_textEdit->setFocus();
    updateMenu();
}


void ProgramTab::initMenus() {

    m_newButton = new SketchToolButton("NewCode", this, m_programWindow->m_newAction);
    m_newButton->setText(tr("New"));
    m_newButton->setObjectName("newCodeButton");
    m_newButton->setEnabledIcon();					// seems to need this to display button icon first time
    m_leftButtonsContainer->addWidget(m_newButton);

    m_openButton = new SketchToolButton("OpenCode", this, m_programWindow->m_openAction);
    m_openButton->setText(tr("Open"));
    m_openButton->setObjectName("openCodeButton");
    m_openButton->setEnabledIcon();					// seems to need this to display button icon first time
    m_leftButtonsContainer->addWidget(m_openButton);

    m_saveButton = new SketchToolButton("SaveCode", this, m_programWindow->m_saveAction);
    m_saveButton->setText(tr("Save"));
    m_saveButton->setObjectName("saveCodeButton");
    m_saveButton->setEnabledIcon();					// seems to need this to display button icon first time
    m_leftButtonsContainer->addWidget(m_saveButton);

    // Platform selection

    QFrame *platformSelector = new QFrame(m_middleButtonsContainer->parentWidget());
    platformSelector->setObjectName("toolbarSelector");
    QVBoxLayout *platformSelectionContainer = new QVBoxLayout(platformSelector);
    platformSelectionContainer->setSpacing(0);
    platformSelectionContainer->setMargin(0);

    QLabel * platformLabel = new QLabel(tr("Platform"), this);
    m_platformComboBox = new QComboBox();
    m_platformComboBox->setObjectName("toolBarComboBox");
    m_platformComboBox->setEditable(false);
    m_platformComboBox->setEnabled(true);
    foreach (Platform * platform, m_programWindow->getAvailablePlatforms()) {
        m_platformComboBox->addItem(platform->getName());
    }
    QSettings settings;
    QString currentPlatform = settings.value("programwindow/platform", "").toString();
    if (currentPlatform.isEmpty()) {
        currentPlatform = m_platformComboBox->currentText(); // TODO
    }

    platformSelectionContainer->addWidget(m_platformComboBox);
    platformSelectionContainer->addWidget(platformLabel);
    m_rightButtonsContainer->addWidget(platformSelector);

    connect(this, SIGNAL(platformChanged(Platform *)), m_programWindow, SLOT(updateBoards()));
    connect(this, SIGNAL(platformChanged(Platform *)), this, SLOT(updateBoards()));
    setPlatform(currentPlatform, false);

    // Board selection

    QFrame *boardSelector = new QFrame(m_middleButtonsContainer->parentWidget());
    boardSelector->setObjectName("toolbarSelector");
    QVBoxLayout *boardSelectionContainer = new QVBoxLayout(boardSelector);
    boardSelectionContainer->setSpacing(0);
    boardSelectionContainer->setMargin(0);

    QLabel * boardLabel = new QLabel(tr("Board"), this);
    m_boardComboBox = new QComboBox();
    m_boardComboBox->setObjectName("toolBarComboBox");
    m_boardComboBox->setEditable(false);
    m_boardComboBox->setEnabled(true);
    updateBoards();

    QString currentBoard = settings.value("programwindow/board", "").toString();
    if (currentBoard.isEmpty()) {
        currentBoard = m_boardComboBox->currentText();
    }
    else if (!m_programWindow->getBoards().contains(currentBoard)) {
        currentBoard = m_boardComboBox->currentText();
    }
    setBoard(currentBoard);

    boardSelectionContainer->addWidget(m_boardComboBox);
    boardSelectionContainer->addWidget(boardLabel);
    m_rightButtonsContainer->addWidget(boardSelector);

    // Port selection

    QFrame *portSelector = new QFrame(m_middleButtonsContainer->parentWidget());
    portSelector->setObjectName("toolbarSelector");
    QVBoxLayout *portSelectionContainer = new QVBoxLayout(portSelector);
    portSelectionContainer->setSpacing(0);
    portSelectionContainer->setMargin(0);

    QLabel * portLabel = new QLabel(tr("Port"), this);
    m_portComboBox = new SerialPortComboBox();
    m_portComboBox->setObjectName("toolBarComboBox");
    m_portComboBox->setEditable(false);
    m_portComboBox->setEnabled(true);
    QList<QSerialPortInfo> ports = m_programWindow->getSerialPorts();
    foreach (const QSerialPortInfo port, ports)
        m_portComboBox->addItem(port.portName(), port.systemLocation());

    QString currentPort = settings.value("programwindow/port", "").toString();
    if (currentPort.isEmpty()) {
        currentPort = m_portComboBox->currentText();
    }
    else if (!m_programWindow->hasPort(currentPort)) {
        currentPort = m_portComboBox->currentText();
    }
    setPort(currentPort);

    portSelectionContainer->addWidget(m_portComboBox);
    portSelectionContainer->addWidget(portLabel);
    m_rightButtonsContainer->addWidget(portSelector);

	// connect last so these signals aren't triggered during initialization
    connect(m_platformComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(setPlatform(const QString &)));
    connect(m_portComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(setPort(const QString &)));
	connect(m_portComboBox, SIGNAL(aboutToShow()), this, SLOT(updateSerialPorts()), Qt::DirectConnection);
    connect(m_boardComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(setBoard(const QString &)));

    m_monitorButton = new SketchToolButton("MonitorCode", this, m_programWindow->m_monitorAction);
    m_monitorButton->setText(tr("Serial Monitor"));
    m_monitorButton->setObjectName("monitorCodeButton");
    m_monitorButton->setEnabledIcon();					// seems to need this to display button icon first time
    m_rightButtonsContainer->addWidget(m_monitorButton);

    m_programButton = new SketchToolButton("ProgramCode", this, m_programWindow->m_programAction);
    m_programButton->setText(tr("Upload"));
    m_programButton->setObjectName("programCodeButton");
    m_programButton->setEnabledIcon();					// seems to need this to display button icon first time
    m_rightButtonsContainer->addWidget(m_programButton);


    //    QFrame * superFrame = new QFrame();
    //    QVBoxLayout * superLayout = new QVBoxLayout();
    //	  superLayout->setMargin(0);
    //	  superLayout->setSpacing(0);
    //    m_unableToProgramLabel = new QLabel(UnableToProgramMessage.arg(""));
    //    m_unableToProgramLabel->setObjectName("unableToProgramLabel");
    //    superLayout->addWidget(footerFrame);
    //    superLayout->addWidget(m_unableToProgramLabel);
    //    superFrame->setLayout(superLayout);

    setPlatform(currentPlatform, false);

}

void ProgramTab::setPlatform(const QString & newPlatformName) {
    setPlatform(m_programWindow->getPlatformByName(newPlatformName), true);
}

void ProgramTab::setPlatform(const QString & newPlatformName, bool updateLink) {
    setPlatform(m_programWindow->getPlatformByName(newPlatformName), updateLink);
}

void ProgramTab::setPlatform(Platform * newPlatform) {
    setPlatform(newPlatform, true);
}

void ProgramTab::setPlatform(Platform * newPlatform, bool updateLink) {
    DebugDialog::debug(QString("Setting platform to %1").arg(newPlatform->getName()));
    bool isPlatformChanged = newPlatform != m_platform;
    if (updateLink && isPlatformChanged) {
        m_programWindow->updateLink(m_filename, newPlatform, false, false);
	}

    if (m_platform != NULL) m_platform->disconnect(SIGNAL(commandLocationChanged()));
    connect(newPlatform, SIGNAL(commandLocationChanged()), this, SLOT(enableProgramButton()));

    m_platform = newPlatform;
    m_platformComboBox->setCurrentIndex(m_platformComboBox->findText(newPlatform->getName()));
    Syntaxer * syntaxer = m_platform->getSyntaxer();
    m_highlighter->setSyntaxer(syntaxer);
    m_highlighter->rehighlight();
    updateMenu();
	QSettings settings;
    settings.setValue("programwindow/platform", newPlatform->getName());

    //bool canProgram = (syntaxer != NULL && syntaxer->canProgram());
    //m_unableToProgramLabel->setVisible(!canProgram);
    //m_unableToProgramLabel->setText(UnableToProgramMessage.arg(newPlatform.getName()));

    if (updateLink && isPlatformChanged)
        emit platformChanged(newPlatform);
}

void ProgramTab::setPort(const QString & newPort) {
    DebugDialog::debug(QString("Setting port to %1").arg(newPort));
    int ix = m_portComboBox->findText(newPort);
    if (ix >= 0) {
        m_port = newPort;
        m_portComboBox->setCurrentIndex(ix);
        m_portComboBox->setToolTip(newPort);
        updateMenu();
        QSettings settings;
        settings.setValue("programwindow/port", newPort);
    }
}

void ProgramTab::setBoard(const QString & newBoard) {
    DebugDialog::debug(QString("Setting board to %1").arg(newBoard));
    int ix = m_boardComboBox->findText(newBoard);
    if (ix >= 0) {
        m_board = newBoard;
        m_boardComboBox->setCurrentIndex(ix);
        m_boardComboBox->setToolTip(newBoard);
        updateMenu();
        QSettings settings;
        settings.setValue("programwindow/board", newBoard);
    }
}

bool ProgramTab::loadProgramFile() {
	if (isModified() || !m_textEdit->document()->isEmpty()) {
		m_programWindow->loadProgramFileNew();
		return false;
	}

	QString fileName = FolderUtils::getOpenFileName(
						this,
						tr("Select a program file to load"),
                        FolderUtils::openSaveFolder(),
                        m_highlighter->syntaxer()->extensionString()
		);

	if (fileName.isEmpty()) return false;

	return loadProgramFile(fileName, "", true);
}

bool ProgramTab::loadProgramFile(const QString & fileName, const QString & altFileName, bool updateLink) {
    DebugDialog::debug("program tab load program file");
	if (m_programWindow->alreadyHasProgram(fileName)) {
		return false;
	}

    DebugDialog::debug("checking file");

	QFile file(fileName);
	if (!file.open(QFile::ReadOnly)) {
        m_programWindow->updateLink(fileName, m_platform, false, true);
		if (!altFileName.isEmpty()) {
			file.setFileName(altFileName);
			if (!file.open(QFile::ReadOnly)) {
				QFileInfo fileInfo(fileName);
				QString fn = FolderUtils::getOpenFileName(
										NULL,
										tr("Fritzing is unable to find '%1', please locate it").arg(fileInfo.fileName()),
										FolderUtils::openSaveFolder(),
										tr("Code (*.%1)").arg(fileInfo.suffix())
								);
				if (fn.isEmpty()) return false;

				file.setFileName(fn);
				if (!file.open(QFile::ReadOnly)) {
					return false;
				}
			}
		}
	}

    DebugDialog::debug("about to read");

	m_filename = file.fileName();
	QString text = file.readAll();
	// clean out 0x91, mostly due to picaxe files
	for (int i = 0; i < text.count(); i++) {
		if (text[i] == Quote91Char) {
			text[i] = '\'';
		}
	}

    DebugDialog::debug("about to set text");

	m_textEdit->setText(text);
	setClean();
	QFileInfo fileInfo(m_filename);
	m_tabWidget->setTabText(m_tabWidget->currentIndex(), fileInfo.fileName());
	m_tabWidget->setTabToolTip(m_tabWidget->currentIndex(), m_filename);

    DebugDialog::debug("about to update link");

	if (updateLink) {
        m_programWindow->updateLink(m_filename, m_platform, true, true);
	}
	return true;
}

void ProgramTab::textChanged() {
	QIcon tabIcon = m_tabWidget->tabIcon(m_tabWidget->currentIndex());
	bool modified = isModified();

	if (m_saveButton) {
		m_saveButton->setEnabled(modified);
	}

	updateMenu();			// calls enableProgramButton

	if (tabIcon.isNull()) {
		if (modified) {
			m_tabWidget->setTabIcon(m_tabWidget->currentIndex(), *AsteriskIcon);
		}
	}
	else {
		if (!modified) {
			m_tabWidget->setTabIcon(m_tabWidget->currentIndex(), QIcon());
		}
	}
}

void ProgramTab::undo() {
	m_textEdit->undo();
}

void ProgramTab::enableUndo(bool enable) {
        m_canUndo = enable;
		m_saveButton->setEnabled(m_canUndo);
        updateMenu();
}

void ProgramTab::redo() {
    m_textEdit->redo();
}

void ProgramTab::enableRedo(bool enable) {
        m_canRedo = enable;
        updateMenu();
}

void ProgramTab::cut() {
        m_textEdit->cut();
}

void ProgramTab::enableCut(bool enable) {
        m_canCut = enable;
        updateMenu();
}

void ProgramTab::copy() {
        m_textEdit->copy();
}

void ProgramTab::enableCopy(bool enable) {
        m_canCopy = enable;
        updateMenu();
}

void ProgramTab::paste() {
        m_textEdit->paste();
}

void ProgramTab::enablePaste(bool enable) {
        m_canPaste = enable;
        updateMenu();
}

void ProgramTab::selectAll() {
        m_textEdit->selectAll();
}

void ProgramTab::deleteTab() {
	bool deleteFile = false;
	if (!m_textEdit->document()->isEmpty()) {
		QString name = QFileInfo(m_filename).fileName();
		if (name.isEmpty()) {
			name = m_tabWidget->tabText(m_tabWidget->currentIndex());
		}

		DeleteDialog deleteDialog(tr("Remove \"%1\"?").arg(name),
								  tr("Are you sure you want to remove \"%1\" from the sketch?").arg(name),
								  !FolderUtils::isEmptyFileName(m_filename, "Untitled"),
								  NULL, 0);
		int reply = deleteDialog.exec();
 		if (reply != QMessageBox::Yes) {
     		return;
		}

		deleteFile = deleteDialog.deleteFileChecked();
	}

	if (m_tabWidget) {
		emit wantToDelete(m_tabWidget->currentIndex(), deleteFile);
		this->deleteLater();
	}
}

bool ProgramTab::isModified() {
	return m_textEdit->document()->isModified();
}

const QString & ProgramTab::filename() {
	return m_filename;
}

void ProgramTab::setFilename(const QString & name) {
        m_filename = name;
        updateMenu();

        // Here we update the widget's tab label. We need
        // to check that it both has the PTabWidget as its
        // parent and it's already been added to the PTabWidget
        int currentIndex = m_tabWidget->indexOf(this);
        if (currentIndex >= 0) {
            m_tabWidget->setTabText(currentIndex, name.section("/",-1));
        }
        else {
            DebugDialog::debug("Negative index");
        }
}

const QStringList & ProgramTab::extensions() {
    if (m_highlighter == NULL) return ___emptyStringList___;

	Syntaxer * syntaxer = m_highlighter->syntaxer();
	if (syntaxer == NULL) return ___emptyStringList___;

	return syntaxer->extensions();
}

const QString & ProgramTab::extensionString() {
    if (m_highlighter == NULL) return ___emptyString___;

	Syntaxer * syntaxer = m_highlighter->syntaxer();
	if (syntaxer == NULL) return ___emptyString___;

	return syntaxer->extensionString();
}


void ProgramTab::setClean() {
	m_textEdit->document()->setModified(false);
	textChanged();
}

void ProgramTab::setDirty() {
	m_textEdit->document()->setModified(true);
	textChanged();
}

void ProgramTab::save() {
	emit wantToSave(m_tabWidget->currentIndex());
}

void ProgramTab::saveAs() {
	emit wantToSaveAs(m_tabWidget->currentIndex());
}

void ProgramTab::rename() {
        emit wantToRename(m_tabWidget->currentIndex());
}

void ProgramTab::print(QPrinter &printer) {
        m_textEdit->print(&printer);
}

void ProgramTab::setText(QString text) {
        m_textEdit->setPlainText(text);
}

QString ProgramTab::text() {
        return m_textEdit->toPlainText();
}

bool ProgramTab::readOnly() {
	// TODO: return true if it's a sample file
	return false;
}

bool ProgramTab::save(const QString & filename) {
	QFile file(filename);
	if (!file.open(QFile::WriteOnly)) {
		return false;
	}

	QByteArray b = m_textEdit->toPlainText().toLatin1();
	qint64 written = file.write(b);
	file.close();
	bool result = (b.length() == written);
	if (result) {
                setFilename(filename);
	}
	return result;
}

void ProgramTab::serialMonitor() {
    if (m_monitorWindow == NULL) {
        m_monitorWindow = new ConsoleWindow(this);
    }
    m_monitorWindow->show();
    m_monitorWindow->activateWindow();
    m_monitorWindow->raise();
    m_monitorWindow->openSerialPort(m_port);
}

void ProgramTab::sendProgram() {
    const QString commandLoc = m_platform->getCommandLocation();
    if (commandLoc.isEmpty()) {
        m_console->setPlainText(tr("No uploader for %1 specified. Go to Preferences > Code View to configure it.").arg(m_platform->getName()));
        return;
    }
    if (! QFile::exists(commandLoc)) {
        m_console->setPlainText(tr("Uploader configured, but not found at %1").arg(commandLoc));
        return;
    }
    if (isModified()) {
        //QMessageBox::information(this, QObject::tr("Fritzing"), tr("The file '%1' must be saved before it can be sent to the programmer.").arg(m_filename));
        //return;
        save();
    }
    if (m_monitorWindow != NULL) {
        m_monitorWindow->closeSerialPort(m_portComboBox->currentText());
    }

	m_programButton->setEnabled(false);
    m_console->setPlainText("");
    m_platform->upload(this,
                       m_portComboBox->currentData().toString(),
                       m_boardComboBox->currentText(),
                       m_filename);
}

void ProgramTab::programProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
	DebugDialog::debug(QString("program process finished %1 %2").arg(exitCode).arg(exitStatus));
	m_programButton->setEnabled(true);
	sender()->deleteLater();
    if (exitCode == 0) {
        m_console->appendPlainText(tr("Upload finished."));
    } else {
        m_console->appendPlainText(tr("Upload failed with exit code %1, %2").arg(exitCode).arg(exitStatus));
    }
}

void ProgramTab::programProcessReadyRead() {
    QByteArray byteArray = qobject_cast<QProcess *>(sender())->readAllStandardOutput();
    m_console->appendPlainText(byteArray);
}

/**
 * This function emits all the required signals to update the menu of its owning ProgramWindow.
 * It is currently called on the QTabWidget's currentChanged() signal.
 */
void ProgramTab::updateMenu() {
//        DebugDialog::debug(QString("Updating the menu for tab %1").arg(m_tabWidget->currentIndex()));
//        DebugDialog::debug(QString("Undo: %1").arg(m_canUndo));
//        DebugDialog::debug(QString("Redo: %1").arg(m_canRedo));
//        DebugDialog::debug(QString("Cut: %1").arg(m_canCut));
//        DebugDialog::debug(QString("Copy: %1").arg(m_canCopy));
//        DebugDialog::debug(QString("Paste: %1").arg(m_canPaste));
//        DebugDialog::debug(QString("Platform: %1").arg(m_platform.getName()));
//        DebugDialog::debug(QString("Port: %1").arg(m_port));

		enableProgramButton();

        // Emit a signal so that the ProgramWindow can update its own UI.
        emit programWindowUpdateRequest(m_programButton ? m_programButton->isEnabled() : false,
            m_canUndo, m_canRedo, m_canCut, m_canCopy, m_canPaste,
            m_platform, m_port, m_board, m_filename);
}

void ProgramTab::updateBoards() {

    QString currentBoard;
    int index = m_boardComboBox->currentIndex();
    if (index >= 0) {
        currentBoard = m_boardComboBox->itemText(index);
    }

    while (m_boardComboBox->count() > 0) {
        m_boardComboBox->removeItem(0);
    }

    QMap<QString, QString> boardNames = m_programWindow->getBoards();
    foreach (QString name, boardNames.keys()) {
        m_boardComboBox->addItem(name, boardNames.value(name));
    }

    int foundIt = m_boardComboBox->findText(currentBoard);
    if (foundIt >= 0)
        m_boardComboBox->setCurrentIndex(foundIt);
    else
        setBoard(m_platform->getDefaultBoardName());
}

void ProgramTab::updateSerialPorts() {
    QList<QSerialPortInfo> ports = m_programWindow->getSerialPorts();

    QList<QSerialPortInfo> newPorts;
    foreach (QSerialPortInfo port, ports) {
        if (m_portComboBox->findText(port.portName()) < 0) {
			newPorts.append(port);
		}
	}
    QList<int> obsoletePorts;
	for (int i = 0; i < m_portComboBox->count(); i++) {
        QString portName = m_portComboBox->itemText(i);
        if (!m_programWindow->hasPort(portName)) {
			obsoletePorts.append(i);
		}
		if (i == m_portComboBox->currentIndex()) {
			// TODO: what?
		}
	}

	for (int i = obsoletePorts.count() - 1; i >= 0; i--) {
		m_portComboBox->removeItem(obsoletePorts.at(i));
	}
    foreach (QSerialPortInfo port, newPorts) {
        m_portComboBox->addItem(port.portName(), port.systemLocation());
	}

	enableProgramButton();
    enableMonitorButton();
}

Platform * ProgramTab::platform() {
    return m_platform;
}

void ProgramTab::enableMonitorButton() {
    if (m_monitorButton == NULL) return;

    bool enabled = true;
    if (m_portComboBox->count() == 0) {
        enabled = false;
    }

    m_monitorButton->setEnabled(enabled);
}

void ProgramTab::enableProgramButton() {
	if (m_programButton == NULL) return;

	bool enabled = true;
    // always enable, to show helpful error message if no programmer is set up
    /*
    if (m_platform->getCommandLocation().isEmpty()) {
		enabled = false;
	}
    if (enabled && m_portComboBox->count() == 0) {
        enabled = false;
	}
	if (enabled && m_textEdit->document()->isEmpty()) {
		enabled = false;
	}
    */

	m_programButton->setEnabled(enabled);
}

void ProgramTab::appendToConsole(const QString & text)
{
    m_console->appendPlainText(text);
}
