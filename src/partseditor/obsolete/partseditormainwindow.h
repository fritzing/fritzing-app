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


#ifndef PARTSEDITORMAINWINDOW_H_
#define PARTSEDITORMAINWINDOW_H_

#include <QMainWindow>
#include <QTabWidget>
#include <QStackedWidget>
#include <QtGui/qwidget.h>

#include "../mainwindow/fritzingwindow.h"
#include "../model/modelpartshared.h"
#include "connectorsinfowidget.h"
#include "editablelinewidget.h"

QT_FORWARD_DECLARE_CLASS(QGraphicsScene)
QT_FORWARD_DECLARE_CLASS(QGraphicsView)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QSplitter)

class PartsEditorMainWindow : public FritzingWindow
{
Q_OBJECT

public:
	PartsEditorMainWindow(QWidget *parent=0);
	~PartsEditorMainWindow();

	void setup(long id, ModelPart *modelPart, bool fromTemplate, class ItemBase * fromItem);
	void setViewItems(class ItemBase *, class ItemBase *, class ItemBase *, class ItemBase *);
	static const QString templatePath;
	const QDir& tempDir();

	bool validateMinRequirements();
	bool save();

signals:
	void partUpdated(const QString &filename, long myId, bool connectorsChanged);
	void closed(long id);
	void changeActivationSignal(bool activate, QWidget * originator);
	void saveButtonClicked();
	void alienPartUsed();

public slots:
	void parentAboutToClose();

protected slots:
	void updateDateAndAuthor();
    void propertiesChanged();

protected:
	bool saveAs();
	bool saveAsAux(const QString & fileName);
	const QDir& createTempFolderIfNecessary();
	void closeEvent(QCloseEvent *event);
	bool eventFilter(QObject *object, QEvent *event);

	void createHeader(ModelPart * = 0);
	void createCenter(ModelPart * = 0, class ItemBase * fromItem = 0);
	void connectWidgetsToSave(const QList<QWidget*> &widgets);
	void createFooter();

	ModelPartShared* createModelPartShared();

	const QString untitledFileName();
	int &untitledFileCount();
	const QString fileExtension();
	QString getExtensionString();
	QStringList getExtensions();
	const QString defaultSaveFolder();

	void updateSaveButton();
	bool wannaSaveAfterWarning(bool savingAsNew);
	void updateButtons();
	const QString fritzingTitle();

	void cleanUp();
	bool event(QEvent *);

	void makeNonCore();

protected:
	long m_id;

	QPointer<class PaletteModel> m_paletteModel;
	QPointer<class SketchModel> m_sketchModel;

	QPointer<class PartsEditorView> m_iconViewImage;
	QPointer<EditableLineWidget> m_title;

	QPointer<class PartsEditorViewsWidget> m_views;

	QPointer<class ItemBase> m_iconItem;
	QPointer<class ItemBase> m_breadboardItem;
	QPointer<class ItemBase> m_schematicItem;
	QPointer<class ItemBase> m_pcbItem;

	QPointer<EditableLineWidget> m_label;
	QPointer<class EditableTextWidget> m_description;
	//QPointer<EditableLineWidget> m_taxonomy;
	QPointer<EditableLineWidget> m_tags;
	QPointer<class HashPopulateWidget> m_properties;
	QPointer<EditableLineWidget> m_author;
	QPointer<class EditableDateWidget> m_createdOn;
	QPointer<QLabel> m_createdByText;

	QPointer<ConnectorsInfoWidget> m_connsInfo;

	QString m_version;
	QString m_moduleId;
	QString m_uri;


	QPointer<QPushButton> m_saveAsNewPartButton;
	QPointer<QPushButton> m_saveButton;
	QPointer<QPushButton> m_cancelCloseButton;

	QPointer<QTabWidget> m_tabWidget;

	QPointer<QFrame> m_mainFrame;
    QPointer<QFrame> m_headerFrame;
    QPointer<QFrame> m_centerFrame;
    QPointer<QFrame> m_footerFrame;

    bool m_updateEnabled;
    bool m_partUpdated;
    bool m_savedAsNewPart;
    bool m_editingAlien;

    static QPointer<PartsEditorMainWindow> m_lastOpened;
    static int m_closedBeforeCount;
    static QString ___partsEditorName___;
    static bool m_closeAfterSaving;

public:
	static QString TitleFreshStartText;
	static QString LabelFreshStartText;
	static QString DescriptionFreshStartText;
	static QString TaxonomyFreshStartText;
	static QString TagsFreshStartText;
	static QString FooterText;

	static QString UntitledPartName;
	static int UntitledPartIndex;

	static QGraphicsProxyWidget *emptyViewItem(QString iconFile, QString text="");
	static void initText();
};
#endif /* PARTSEDITORMAINWINDOW_H_ */
