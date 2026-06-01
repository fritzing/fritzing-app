/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2026 Fritzing

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

#ifndef GERBERPREVIEWDIALOG_H
#define GERBERPREVIEWDIALOG_H

#include "gerberpreviewwidget.h"

#include <QColor>
#include <QDialog>
#include <QHash>
#include <QStringList>

class QAction;
class QLabel;
class QMenu;
class QSplitter;
class QToolBar;
class LayerManagerWidget;

/**
 * @brief Standalone, resizable Gerber/Excellon preview window.
 *
 * Layout (QSplitter, horizontal):
 *   [ GerberPreviewWidget canvas ] | [ LayerManagerWidget ]
 *
 * Toolbar across the top: Open / Fit / Zoom +/- / Rotate 90 / Flip H/V /
 * Reset view.
 *
 * State persistence via QSettings under "preview/gerber/":
 *   - dialog geometry
 *   - splitter sizes
 *   - per-layer visibility + color (incl. opacity alpha)
 *   - last loaded file list
 *
 * Non-modal by design so the user can keep editing the sketch while
 * inspecting the exported artifacts.
 */
class GerberPreviewDialog : public QDialog {
	Q_OBJECT

public:
	explicit GerberPreviewDialog(QWidget *parent = nullptr);
	~GerberPreviewDialog() override;

	/**
	 * @brief Replace the layer stack and refit.
	 * @param paths Absolute paths to .gbr / .gtl / .gbl / .drl / etc.
	 * @param title Optional window title suffix (e.g. board name).
	 */
	void openFiles(const QStringList &paths, const QString &title = QString());

	/**
	 * @brief Convenience: scan @p directory for Gerber-ish files
	 *        and openFiles() everything found. Returns the file
	 *        list it ended up loading.
	 */
	QStringList openDirectory(const QString &directory,
	                          const QString &title = QString());

private slots:
	void onOpen();
	void onOpenDir();
	void onRecentSelected();
	void onFit();
	void onZoomIn();
	void onZoomOut();
	void onRotate();
	void onFlipH();
	void onFlipV();
	void onResetView();
	void onToggleMeasure(bool on);
	void onMeasurementChanged(double distMm, double dxMm, double dyMm);
	void onApertureSelected(int dcode, const QString &description, int count);
	void onLayerVisibilityChanged(GerberPreviewWidget::LayerKind kind, bool on);
	void onLayerColorChanged(GerberPreviewWidget::LayerKind kind, const QColor &color);
	void onLoadFinished(int layerCount, const QStringList &warnings);

protected:
	void closeEvent(QCloseEvent *event) override;

private:
	void buildUi();
	void saveState() const;
	void restoreState();
	QString settingsKeyFor(GerberPreviewWidget::LayerKind kind, const QString &suffix) const;
	void applyPersistedLayerSettings();

	/**
	 * @brief Push @p dir to the top of the recent-directories list,
	 *        dedupe, cap at kMaxRecent, persist to QSettings, and
	 *        rebuild the Recent menu.
	 */
	void pushRecentDir(const QString &dir);
	void rebuildRecentMenu();

	GerberPreviewWidget *m_preview;
	LayerManagerWidget  *m_layerMgr;
	QSplitter           *m_split;
	QToolBar            *m_toolbar;
	QAction             *m_measureAction = nullptr; ///< checkable Measure toggle in toolbar
	QMenu               *m_recentMenu    = nullptr; ///< populated by rebuildRecentMenu()
	QAction             *m_recentAction  = nullptr; ///< toolbar entry that exposes m_recentMenu
	QStringList          m_recentDirs;             ///< persisted MRU of opened directories
	QLabel              *m_status;
	QString              m_baseStatus;              ///< last load summary; restored when measurement clears
	QStringList          m_lastPaths;

	// Persisted layer settings loaded at construction; applied to
	// the renderer + manager after each loadFiles() so the user sees
	// the same colors and visibility they last left the dialog with.
	QHash<GerberPreviewWidget::LayerKind, QColor> m_persistedColors;
	QHash<GerberPreviewWidget::LayerKind, bool>   m_persistedVisible;
};

#endif // GERBERPREVIEWDIALOG_H
