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

*********************************************** *********************

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/


#ifndef PARTSBINPALETTEWIDGET_H_
#define PARTSBINPALETTEWIDGET_H_

#include <QFrame>
#include <QToolButton>
#include <QLineEdit>
#include <QStackedWidget>

#include "../model/palettemodel.h"
#include "../model/modelpart.h"
#include "../waitpushundostack.h"
#include "../utils/fileprogressdialog.h"
#include "../utils/bundler.h"
#include "binmanager/binmanager.h"

class PartsBinPaletteWidget : public QFrame, public Bundler {
	Q_OBJECT

	public:
		PartsBinPaletteWidget(class ReferenceModel *referenceModel, class HtmlInfoView *infoView, WaitPushUndoStack *undoStack, class BinManager* manager);
		~PartsBinPaletteWidget();

		QSize sizeHint() const;
		QString title() const;
		void setTitle(const QString &title);

		void loadFromModel(PaletteModel *model);
		void setPaletteModel(PaletteModel *model, bool clear = false);

		void addPart(ModelPart *modelPart, int position = -1);

		bool currentBinIsCore();
		bool beforeClosing();
		bool canClose();

		ModelPart * selectedModelPart();
		ItemBase * selectedItemBase();
		bool hasAlienParts();
        QList<ModelPart *> getAllParts();

		void setInfoViewOnHover(bool infoViewOnHover);
		void addPart(const QString& moduleID, int position);
		void addNewPart(ModelPart *modelPart);
        void removePart(const QString & moduleID, const QString & path);
        void removeParts();
        void load(const QString& filename, QWidget * progressTarget, bool fastLoad);

		bool contains(const QString &moduleID);
		void setDirty(bool dirty=true);

		const QString &fileName();

		class PartsBinView *currentView();
		QAction *addPartToMeAction();
		bool allowsChanges();
		bool readOnly();
		void setAllowsChanges(bool);
		void setReadOnly(bool);
		void focusSearch();
		void setSaveQuietly(bool);
        bool open(QString fileName, QWidget * progressTarget, bool fastLoad);

		bool currentViewIsIconView();
        QIcon icon();
		QIcon monoIcon();
		bool hasMonoIcon();
        void saveBundledBin();	
		QMenu * combinedMenu();
		QMenu * binContextMenu();
		QMenu * partContextMenu();
		bool fastLoaded();
		BinLocation::Location location();
		void copyFilesToContrib(ModelPart *, QWidget * originator);
        ModelPart * root();
        bool isTempPartsBin();
		void reloadPart(const QString & moduleID);

	public slots:
		void addPartCommand(const QString& moduleID);
		void removeAlienParts();
		void itemMoved();
		void toIconView();
		void toListView();
		bool save();
		bool saveAs();
		void changeIconColor();

	protected slots:
		void undoStackCleanChanged(bool isClean);
		void addSketchPartToMe();
		void search();
		void focusSearchAfter();

	signals:
		void saved(bool hasPartsFromBundled);
		void fileNameUpdated(PartsBinPaletteWidget*, const QString &newFileName, const QString &oldFilename);
		void focused(PartsBinPaletteWidget*);

	protected:
		void dragEnterEvent(QDragEnterEvent *event);
		void dragLeaveEvent(QDragLeaveEvent *event);
		void dragMoveEvent(QDragMoveEvent *event);
		void dropEvent(QDropEvent *event);
		void closeEvent(QCloseEvent *event);
		void mousePressEvent(QMouseEvent *event);
		bool eventFilter(QObject *obj, QEvent *event);

		void setupHeader();

		void grabTitle(PaletteModel *model);
		void grabTitle(const QString & title, QString & iconFilename);

		void setView(class PartsBinView *view);
		bool saveAsAux(const QString &filename);

		void afterModelSetted(PaletteModel *model);

		QToolButton* newToolButton(const QString& btnObjName, const QString& imgPath, const QString &text);

		bool loadBundledAux(QDir &unzipDir, QList<ModelPart*> mps);

		void setFilename(const QString &filename);

	protected:
		PaletteModel *m_model;
		ReferenceModel *m_referenceModel;
		bool m_canDeleteModel;
		bool m_orderHasChanged;

		QString m_fileName;
		QString m_defaultSaveFolder;
		QString m_untitledFileName;

		QString m_title;
		bool m_isDirty;

		PartsBinView *m_currentView;
		class PartsBinIconView *m_iconView;
		class PartsBinListView *m_listView;

		QFrame *m_header;
		QLabel * m_binLabel;

		class SearchLineEdit * m_searchLineEdit;

		QToolButton * m_combinedBinMenuButton;

		WaitPushUndoStack *m_undoStack;
		BinManager *m_manager;

		QStringList m_alienParts;
		bool m_allowsChanges;
		bool m_saveQuietly;

		FileProgressDialog * m_loadingProgressDialog;
		QIcon * m_icon;
        QIcon * m_monoIcon;
		QAction *m_addPartToMeAction;
		QStackedWidget * m_stackedWidget;
		QStackedWidget * m_searchStackedWidget;
		bool m_fastLoaded;
		BinLocation::Location m_location;
        QStringList m_removed;

    public:
        static void cleanup();

	public:
		static QString Title;
};

#endif /* PARTSBINPALETTEWIDGET_H_ */
