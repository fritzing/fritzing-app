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



#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QIcon>
#include <QAction>
#include <QAbstractButton>

#include "fritzingwindow.h"
#include "../debugdialog.h"
#include "../utils/misc.h"
#include "../utils/folderutils.h"
#include "../utils/fileprogressdialog.h"

const QString FritzingWindow::QtFunkyPlaceholder = QLatin1String("[*]");  // this is some weird hack Qt uses in window titles as a placeholder to setr the modified state
QString FritzingWindow::ReadOnlyPlaceholder(" [READ-ONLY] ");
static QString ___fritzingTitle___;
QStringList FritzingWindow::OtherKnownExtensions;

FritzingWindow::FritzingWindow(const QString &untitledFileName, int &untitledFileCount, QString fileExt, QWidget * parent, Qt::WindowFlags f)
	: QMainWindow(parent, f)
{
	___fritzingTitle___ = QObject::tr("Fritzing");

	// Let's set the icon
	this->setWindowIcon(QIcon(QPixmap(":resources/images/fritzing_icon.png")));

	QString fn = untitledFileName;

	if(untitledFileCount > 1) {
		fn += " " + QString::number(untitledFileCount);
	}
	fn += fileExt;
	setFileName(fn);

	untitledFileCount++;

	setTitle();

	m_undoStack = new WaitPushUndoStack(this);
	connect(m_undoStack, SIGNAL(cleanChanged(bool)), this, SLOT(undoStackCleanChanged(bool)) );
}

void FritzingWindow::createCloseAction() {
	m_closeAct = new QAction(tr("&Close Window"), this);
	m_closeAct->setShortcut(tr("Ctrl+W"));
	m_closeAct->setStatusTip(tr("Close the current sketch"));
	connect(m_closeAct, SIGNAL(triggered()), this, SLOT(close()));
}

void FritzingWindow::setTitle() {
	setWindowTitle(tr("%1 - %2")
	               .arg(QFileInfo(m_fwFilename).fileName()+(m_readOnly?ReadOnlyPlaceholder:"")+QtFunkyPlaceholder)
	               .arg(fritzingTitle()));
}

// returns true if the user wanted to save the file
bool FritzingWindow::save() {
	bool result;
	if (FolderUtils::isEmptyFileName(m_fwFilename, untitledFileName())) {
		result = saveAs();
	} else if (m_readOnly) {
		result = saveAs();
	} else {
		result = saveAsAux(m_fwFilename);
	}

	return result;
}

bool FritzingWindow::saveAs() {
	return saveAs(m_fwFilename, m_readOnly);
}

bool FritzingWindow::save(const QString & filename, bool readOnly) {
	if (FolderUtils::isEmptyFileName(filename, untitledFileName())) {
		return saveAs(filename, readOnly);
	} else if (m_readOnly) {
		return saveAs(filename, readOnly);
	} else {
		return saveAsAux(filename);
	}
}

bool FritzingWindow::saveAs(const QString & filename, bool readOnly) {
	DebugDialog::debug(QString("current path: %1").arg(QDir::currentPath()));
	QString fileExt;
	QString path;
	QString untitledBase = untitledFileName();

	if(readOnly) {
		path = defaultSaveFolder() + "/" + QFileInfo(filename).fileName();
	}
	else if(filename.startsWith(untitledBase, Qt::CaseInsensitive)) {
		path = defaultSaveFolder() + "/" + filename;
	}
	else if(filename.isNull() || filename.isEmpty()) {
		path = defaultSaveFolder();
	}
	else {
		path = filename;
	}
	DebugDialog::debug(QString("current file: %1").arg(filename));
	QString newFilename = FolderUtils::getSaveFileName(
	                          this,
	                          tr("Specify a file name"),
	                          path,
	                          getExtensionString(),
	                          &fileExt
	                      );

	if (newFilename.isEmpty()) return false; // Cancel pressed

	if (readOnly && (newFilename.compare(filename, Qt::CaseInsensitive) == 0)) {
		QMessageBox::warning(NULL, QObject::tr("Fritzing"),
		                     QObject::tr("The file '%1' is read-only; please use a different filename.")
		                     .arg(newFilename) );
		return false;

	}

	FileProgressDialog progress("Saving...", 0, this);

	QStringList extensions = getExtensions();
	bool hasExtension = false;
	foreach (QString extension, extensions) {
		if(alreadyHasExtension(newFilename, extension)) {
			hasExtension = true;
			break;
		}
	}

	if (!hasExtension) {
		newFilename += extensions[0];
	}

	return saveAsAux(newFilename);
}

void FritzingWindow::undoStackCleanChanged(bool isClean) {
	setWindowModified(!isClean);
}

bool FritzingWindow::alreadyHasExtension(const QString &fileName, const QString &fileExt) {
	// TODO: Make something prettier to manage all the supported formats at once
	if(!fileExt.isEmpty()) {
		return fileName.endsWith(fileExt);
	} else {
		foreach (QString extension, fritzingExtensions()) {
			if (fileName.endsWith(extension)) return true;
		}
		foreach (QString extension, OtherKnownExtensions) {
			if (fileName.endsWith(extension)) return true;
		}

		return false;
	}
}

bool FritzingWindow::anyModified() {
	return isWindowModified();
}

bool FritzingWindow::beforeClosing(bool showCancel, bool & discard) {
	discard = false;
	if (anyModified()) {
		QMessageBox::StandardButton reply = beforeClosingMessage(m_fwFilename, showCancel);
		if (reply == QMessageBox::Save) {
			return save();
		}
		else if (reply == QMessageBox::Discard) {
			discard = true;
			return true;
		}
		else {
			return false;
		}
	}
	else {
		return true;
	}
}

QMessageBox::StandardButton FritzingWindow::beforeClosingMessage(const QString & filename, bool showCancel)
{

	QMessageBox messageBox(this);
	setBeforeClosingText(filename, messageBox);
	QMessageBox::StandardButtons buttons = QMessageBox::Save | QMessageBox::Discard;
	if (showCancel) {
		buttons |= QMessageBox::Cancel;
	}
	messageBox.setStandardButtons(buttons);
	messageBox.setDefaultButton(QMessageBox::Save);
	if (m_fwFilename.startsWith(untitledFileName())) {
		messageBox.setButtonText(QMessageBox::Save, tr("Save..."));
	}
	else {
		messageBox.setButtonText(QMessageBox::Save, tr("Save"));
	}
	messageBox.setButtonText(QMessageBox::Discard, tr("Don't Save"));
	if (showCancel) {
		messageBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));
	}
	messageBox.setIcon(QMessageBox::Warning);
	messageBox.setWindowModality(Qt::WindowModal);
	messageBox.button(QMessageBox::Discard)->setShortcut(tr("Ctrl+D"));

	return (QMessageBox::StandardButton) messageBox.exec();
}

void FritzingWindow::setBeforeClosingText(const QString & filename, QMessageBox & messageBox)
{
	QString basename = QFileInfo(filename).fileName();
	messageBox.setWindowTitle(tr("Save \"%1\"").arg(basename));
	messageBox.setText(tr("Do you want to save the changes you made in the document \"%1\"?").arg(basename));
	messageBox.setInformativeText(tr("Your changes will be lost if you don't save them."));
}

void FritzingWindow::setReadOnly(bool readOnly) {
	bool hasChanged = m_readOnly != readOnly;
	m_readOnly = readOnly;
	if(hasChanged) {
		emit readOnlyChanged(readOnly);
	}
}

const QString FritzingWindow::fritzingTitle() {
	return ___fritzingTitle___;
}

const QString &  FritzingWindow::fileName() {
	return m_fwFilename;
}

void FritzingWindow::setFileName(const QString & filename) {
	m_fwFilename = filename;
}

void FritzingWindow::notClosableForAWhile() {
}
