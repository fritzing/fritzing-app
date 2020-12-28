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


#ifndef FRITZINGWINDOW_H_
#define FRITZINGWINDOW_H_

#include <QMainWindow>
#include <QDir>
#include <QStatusBar>
#include <QMessageBox>

#include "../waitpushundostack.h"
#include "../utils/misc.h"
#include "../utils/bundler.h"

class FritzingWindow : public QMainWindow, public Bundler
{
	Q_OBJECT

public:
	FritzingWindow(const QString &untitledFileName, int &untitledFileCount, QString fileExt, QWidget * parent = 0, Qt::WindowFlags f = Qt::WindowFlags());
	const QString &fileName();
	void setFileName(const QString &);

	virtual void notClosableForAWhile();
	static bool alreadyHasExtension(const QString &fileName, const QString &extension=___emptyString___);

signals:
	void readOnlyChanged(bool isReadOnly);

protected slots:
	void undoStackCleanChanged(bool isClean);
	virtual bool save();
	virtual bool saveAs();

protected:
	bool save(const QString & filename, bool readOnly);
	bool saveAs(const QString & filename, bool readOnly);
	virtual void setTitle();
	virtual const QString fritzingTitle();
	virtual const QString fileExtension() = 0;
	virtual const QString untitledFileName() = 0;
	virtual int &untitledFileCount() = 0;
	virtual const QString defaultSaveFolder() = 0;
	virtual QString getExtensionString() = 0;
	virtual QStringList getExtensions() = 0;

	virtual bool saveAsAux(const QString & fileName) = 0;
	virtual bool beforeClosing(bool showCancel, bool & discard);			// returns true if close, false if cancel
	QMessageBox::StandardButton beforeClosingMessage(const QString & filename, bool showCancel);
	virtual void setBeforeClosingText(const QString & filename, QMessageBox & messageBox);

	void createCloseAction();

	void setReadOnly(bool readOnly);
	virtual bool anyModified();

protected:
	class WaitPushUndoStack * m_undoStack = nullptr;
	QString m_fwFilename;
	bool m_readOnly = false;
	QAction *m_closeAct = nullptr;
	QDir m_tempDir;
	QStatusBar *m_statusBar = nullptr;

protected:
	static QStringList OtherKnownExtensions;

public:
	static QString ReadOnlyPlaceholder;
	static const QString QtFunkyPlaceholder;
};

#endif /* FRITZINGWINDOW_H_ */
