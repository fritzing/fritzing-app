/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2014 Fachhochschule Potsdam - http://fh-potsdam.de

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

$Revision: 6112 $:
$Author: cohen@irascible.com $:
$Date: 2012-06-28 00:18:10 +0200 (Do, 28. Jun 2012) $

********************************************************************/


#ifndef PROGRAMTAB_H_
#define PROGRAMTAB_H_

#include <QMainWindow>
#include <QPointer>
#include <QPushButton>
#include <QFrame>
#include <QTextEdit>
#include <QProcess>
#include <QTabWidget>
#include <QComboBox>
#include <QTabWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QLabel>
#include <QPrinter>

#include "programwindow.h"

class SerialPortComboBox : public QComboBox
{
	Q_OBJECT

public:
	SerialPortComboBox();

protected:
	void showPopup();

signals:
	void aboutToShow();
};

class ProgramTab : public QFrame 
{
	Q_OBJECT

public:
    ProgramTab(QString & filename, QWidget * parent);
    ~ProgramTab();

    void showEvent(QShowEvent *event);

	bool isModified();
	const QString & filename();
	void setFilename(const QString &);
	const QStringList & extensions();
	const QString & extensionString();
	bool readOnly();
	void setClean();
	void setDirty();
	bool save(const QString & filename);
	bool loadProgramFile(const QString & filename, const QString & altFilename, bool noUpdate);
    void print(QPrinter & printer);
    void setText(QString text);
    QString text();
	void updateProgrammers();
	bool setProgrammer(const QString & path);
	const QString & programmer();
	const QString & language();
	void setLanguage(const QString &, bool updateLink);
	void appendToConsole(const QString &);

public slots:
    void setLanguage(const QString &);
    void setPort(const QString &);
    bool loadProgramFile();
	void textChanged();
    void undo();
    void enableUndo(bool enable);
    void redo();
    void enableRedo(bool enable);
    void cut();
    void enableCut(bool enable);
    void copy();
    void enableCopy(bool enable);
    void paste();
    void selectAll();
	void deleteTab();
	void save();
    void saveAs();
    void rename();
	void sendProgram();
	void chooseProgrammerTimed(int);
	void chooseProgrammerTimeout();
	void chooseProgrammer(const QString &);
	void programProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void programProcessReadyRead();
    void updateMenu();
	void updateSerialPorts();

signals:
	// TODO: since ProgramTab has m_programWindow most/all of these signals could be replaced by direct
	// calls to ProgramWindow public functions
	void wantToSave(int);
	void wantToSaveAs(int);
    void wantToRename(int);
	void wantToDelete(int, bool deleteFile);
    void programWindowUpdateRequest(bool programEnable, bool undoEnable, bool redoEnable,
                            bool cutEnable, bool copyEnable, const QString & language,
                            const QString & port, const QString & programmer, const QString & filename);

protected:
    QFrame * createFooter();
	void chooseProgrammerAux(const QString & programmer, bool updateLink);
	void updateProgrammerComboBox(const QString & programmer);
	void enableProgramButton();

protected:
	QPointer<QPushButton> m_saveAsButton;
	QPointer<QPushButton> m_saveButton;
    QPointer<QPushButton> m_cancelCloseButton;
	QPointer<QPushButton> m_programButton;
    QPointer<SerialPortComboBox> m_portComboBox;
	QPointer<QComboBox> m_languageComboBox;
	QPointer<QComboBox>  m_programmerComboBox;
	QPointer<QTextEdit> m_textEdit;
	QPointer<QTextEdit> m_console;
	QPointer<QTabWidget> m_tabWidget;
    QPointer<QLabel> m_unableToProgramLabel;
	bool m_updateEnabled;

    QPointer<ProgramWindow> m_programWindow;

    // Store the status of selected text, undo, and redo actions
    // so this tab can emit the proper status of these actions
    // on activation
    bool m_canUndo;
    bool m_canRedo;
    bool m_canCopy;
    bool m_canCut;
    QString m_language;
    QString m_port;

	QPointer<class Highlighter> m_highlighter;
	QString m_filename;
	QString m_programmerPath;

};

class DeleteDialog : public QDialog {
	Q_OBJECT

public:
	DeleteDialog(const QString & title, const QString & text, bool deleteFileCheckBox, QWidget * parent = 0, Qt::WindowFlags f = 0);

	bool deleteFileChecked();

protected slots:
	void buttonClicked(QAbstractButton * button);

protected:
	QDialogButtonBox * m_buttonBox;
	QCheckBox * m_checkBox;

};

#endif /* PROGRAMTAB_H_ */
