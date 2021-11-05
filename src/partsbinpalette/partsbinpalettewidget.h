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

class ReferenceModel;
class HtmlInfoView;
class BinManager;
class PartsBinView;
class PartsBinIconView;
class PartsBinListView;
class SearchLineEdit;
class PartsBinPaletteWidget : public QFrame, public Bundler {
	Q_OBJECT

public:
	PartsBinPaletteWidget(ReferenceModel *referenceModel, HtmlInfoView *infoView, WaitPushUndoStack *undoStack, BinManager* manager);
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
	void setDirty(bool dirty = true);

	const QString &fileName();

	PartsBinView *currentView();
	QAction *addPartToMeAction();
	constexpr bool allowsChanges() const noexcept { return m_allowsChanges; }
	constexpr bool readOnly() const noexcept { return !m_allowsChanges; }
	void setAllowsChanges(bool) noexcept;
	void setReadOnly(bool) noexcept;
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

	void setView(PartsBinView *view);
	bool saveAsAux(const QString &filename);

	void afterModelSetted(PaletteModel *model);

	QToolButton* newToolButton(const QString& btnObjName, const QString& imgPath, const QString &text);

	bool loadBundledAux(QDir &unzipDir, QList<ModelPart*> mps);

	void setFilename(const QString &filename);

protected:
	PaletteModel *m_model = nullptr;
	ReferenceModel *m_referenceModel = nullptr;
	bool m_canDeleteModel = false;
	bool m_orderHasChanged = false;

	QString m_fileName;
	QString m_defaultSaveFolder;
	QString m_untitledFileName;

	QString m_title;
	bool m_isDirty = false;

	PartsBinView *m_currentView = nullptr;
	PartsBinIconView *m_iconView = nullptr;
	PartsBinListView *m_listView = nullptr;

	QFrame * m_header = nullptr;
	QLabel * m_binLabel = nullptr;

	SearchLineEdit * m_searchLineEdit = nullptr;

	QToolButton * m_combinedBinMenuButton = nullptr;

	WaitPushUndoStack *m_undoStack = nullptr;
	BinManager *m_manager = nullptr;

	QStringList m_alienParts;
	bool m_allowsChanges = false;
	bool m_saveQuietly = false;

	FileProgressDialog * m_loadingProgressDialog = nullptr;
	QIcon * m_icon = nullptr;
	QIcon * m_monoIcon = nullptr;
	QAction *m_addPartToMeAction = nullptr;
	QStackedWidget * m_stackedWidget = nullptr;
	QStackedWidget * m_searchStackedWidget = nullptr;
	bool m_fastLoaded = false;
	BinLocation::Location m_location;
	QStringList m_removed;

public:
	static void cleanup();

public:
	static QString Title;
};

#endif /* PARTSBINPALETTEWIDGET_H_ */
