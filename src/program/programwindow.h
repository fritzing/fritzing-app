/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2016 Fritzing

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by\
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


#ifndef PROGRAMWINDOW_H_
#define PROGRAMWINDOW_H_

#include <QMainWindow>
#include <QPointer>
#include <QPushButton>
#include <QFrame>
#include <QTextEdit>
#include <QProcess>
#include <QTabWidget>
#include <QComboBox>
#include <QActionGroup>
// #include <QSerialPort>
#include <QtSerialPort/qserialportinfo.h>
#include <QtSerialPort/qserialport.h>

#include "platform.h"
#include "syntaxer.h"

#include "../mainwindow/fritzingwindow.h"

struct LinkedFile {
	enum FileFlag {
		NoFlag = 0,
		SameMachineFlag = 1,
		ObsoleteFlag = 2,
		InBundleFlag = 4,
		ReadOnlyFlag = 8
	};
	Q_DECLARE_FLAGS(FileFlags, FileFlag)

	QString linkedFilename;
    QString platform;
	FileFlags fileFlags;
};

class PTabWidget : public QTabWidget 
{
	Q_OBJECT

protected slots:
    void tabChanged(int index);

protected:
    int m_lastTabIndex;

public:
	PTabWidget(QWidget * parent);
	QTabBar * tabBar();
};

class ProgramWindow : public FritzingWindow
{
Q_OBJECT

public:
	ProgramWindow(QWidget *parent=0);
	~ProgramWindow();

	void setup();
    void initMenus(QMenuBar * menubar);
	void linkFiles(const QList<LinkedFile *> &, const QString & alternativePath);
	const QString defaultSaveFolder();

    QList<QSerialPortInfo> getSerialPorts();
    QList<Platform *> getAvailablePlatforms();
    bool hasPlatform(const QString &platformName);
    Platform *getPlatformByName(const QString &platformName);
    const QMap<QString, QString> getBoards();
    bool hasPort(const QString &portName);
	void loadProgramFileNew();
	bool alreadyHasProgram(const QString &);
    void updateLink(const QString & filename, Platform *platform, bool addlink, bool strong);
    void showMenus(bool);
    void createViewMenuActions(QList<QAction *> &);
    void print();

public slots:
    void saveAll();

signals:
	void closed();
    void changeActivationSignal(bool activate, QWidget * originator);
    void linkToProgramFile(const QString & filename, Platform * platform, bool addlink, bool strong);

protected slots:
	void loadProgramFile();
    class ProgramTab * addTab();
    void closeCurrentTab();
    void closeTab(int index);
    void saveCurrentTab();
    void tabSave(int);
    void tabSaveAs(int);
    void tabRename(int);
    void duplicateTab();
	void tabBeforeClosing(int, bool & ok);
    void tabDelete(int index, bool deleteFile);
    void updateMenu(bool programEnable, bool undoEnable, bool redoEnable,
                    bool cutEnable, bool copyEnable, bool pasteEnable,
                    Platform * platform, const QString & port, const QString & board, const QString & filename);
	void updateSerialPorts();
	void portProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void portProcessReadyRead();
    void updateBoards();

    // The following methods just forward events on to the current tab
    void setPlatform(QAction*);
    void setPort(QAction*);
    void setBoard(QAction*);
	void rename();
	void undo();
	void redo();
	void cut();
	void copy();
	void paste();
	void selectAll();
    void serialMonitor();
	void sendProgram();

protected:
    bool event(QEvent * event);
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *object, QEvent *event);

	QFrame * createHeader();
	QFrame * createCenter();

	const QString untitledFileName();
	const QString fileExtension();

    void cleanUp();
    int &untitledFileCount();
    void setTitle();
    void setTitle(const QString & filename);
	bool saveAsAux(const QString & fileName);
	bool prepSave(class ProgramTab *, bool saveAsFlag);
    bool beforeClosingTab(int index, bool showCancel);
    QAction * addBoard(const QString & name, const QString & definition);
    QAction * addPort(QSerialPortInfo port);
	inline ProgramTab * currentWidget();
    inline ProgramTab * indexWidget(int index);
	QString getExtensionString();
	QStringList getExtensions();
	bool beforeClosing(bool showCancel, bool & discard); // returns true if close, false if cancel
	QStringList getSerialPortsAux();

protected:
    static void initPlatforms();

public:
	static const QString LocateName;
    static QString NoBoardName;
    QAction *m_newAction;
    QAction *m_openAction;
    QAction *m_saveAction;
    QAction *m_monitorAction;
    QAction *m_programAction;

protected:
    static QList<Platform *> m_platforms;

protected:
	QPointer<PTabWidget> m_tabWidget;
	QPointer<QPushButton> m_addButton;
    QPointer<class ProgramTab> m_savingProgramTab;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_selectAction;
    QAction *m_printAction;
    QAction *m_preferencesAction;
    QMap<Platform *, QAction *> m_platformActions;
    QMap<QString, QAction *> m_portActions;
    QMap<QString, QAction *> m_boardActions;
    QActionGroup * m_platformActionGroup;
    QActionGroup * m_boardActionGroup;
    QActionGroup * m_serialPortActionGroup;
    QMenu * m_platformMenu;
    QMenu * m_boardMenu;
	QMenu * m_serialPortMenu;
	QStringList m_ports;				// temporary storage for linux
    QMenu* m_editMenu;
    QMenu* m_viewMenu;
    QList<QAction *> m_viewMenuActions;
    QMenu * m_programMenu;
};

#endif /* ProgramWindow_H_ */
