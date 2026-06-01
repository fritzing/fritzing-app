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

#ifndef PANELIZERINTERACTIVEDIALOG_H
#define PANELIZERINTERACTIVEDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QSizeF>
#include <QHash>

#include "panelizerengine.h"

class PanelLayoutEditor;
class GerberPreviewWidget;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QComboBox;
class QRadioButton;
class QLineEdit;
class QLabel;
class QTabWidget;
class QPushButton;

/**
 * @brief The single, all-in-one interactive panelizer window.
 *
 * This deliberately replaces the old multi-page wizard. Everything the
 * user needs lives here at once:
 *   - a left options column (panel size, separation, extras, output),
 *   - a centre arrange surface (PanelLayoutEditor) with alignment guides,
 *   - an embedded Gerber preview (GerberPreviewWidget) on its own tab.
 *
 * "Generate" runs the full PanelizerEngine pipeline (layout honouring the
 * user's hand placements, then emitPanel) and loads the freshly written
 * Gerbers into the embedded preview without closing the window, so the
 * user can iterate: tweak → generate → look → tweak again. "Save & Close"
 * accepts once at least one successful generation has happened.
 *
 * Units: every spin box presents millimetres to the user; the
 * PanelizerEngine boundary is inches, converted at the edge (mmToIn).
 */
class PanelizerInteractiveDialog : public QDialog
{
	Q_OBJECT

public:
	/**
	 * @brief Construct the interactive window.
	 * @param parent             Owning widget (the MainWindow).
	 * @param currentSketchPath  Absolute path to the open .fzz (step board).
	 * @param copies             Copies of the current board (from start dialog).
	 * @param allowRotate        Allow 90° rotation during auto-layout.
	 * @param blendPaths         Extra .fzz boards to blend in (1 copy each).
	 */
	PanelizerInteractiveDialog(QWidget * parent,
	                           const QString & currentSketchPath,
	                           int copies,
	                           bool allowRotate,
	                           const QStringList & blendPaths);
	~PanelizerInteractiveDialog() override = default;

	/// Gerber directory of the last successful generation (empty if none).
	QString lastOutputDir() const { return m_lastOutputDir; }

protected:
	/// Reject re-seeding to fit content on first show.
	void showEvent(QShowEvent * event) override;

private slots:
	/// Re-run the auto-layout and re-seed the arrange editor.
	void reseedEditor();
	/// Apply a chosen standard panel size to the width/height spinboxes.
	void applySizePreset(int index);
	/// Flip the preset selector to "Custom" when the user edits a dimension.
	void markCustomSize();
	/// Run the full pipeline and refresh the embedded preview.
	void onGenerate();
	/// Pick the Gerber output directory.
	void browseOutputDir();
	/// Enable separation sub-controls based on the chosen method.
	void updateSeparationEnabled();
	/// Enable extras sub-controls based on their checkboxes.
	void updateExtrasEnabled();

private:
	void buildUi();
	QWidget * buildOptionsPanel();
	QWidget * buildSeparationGroup();
	QWidget * buildExtrasGroup();

	/// Populate the standard panel-size dropdown (fab-shop sizes + Custom).
	void populateSizePresets();
	/// Open + cache every source board's size behind a loading dialog so the
	/// hidden-probe work happens once, with user feedback, on first show.
	void preloadBoardSizes();

	/// Build a PanelSpec from the current control values.
	PanelizerEngine::PanelSpec currentPanelSpec() const;
	/// Build a SeparationSpec from the current control values.
	PanelizerEngine::SeparationSpec currentSeparationSpec() const;
	/// Build an ExtrasSpec from the current control values.
	PanelizerEngine::ExtrasSpec currentExtrasSpec() const;
	/// Build the engine source-board list (current sketch + any blend boards).
	QList<PanelizerEngine::SourceBoard> currentSources();

	/**
	 * @brief Probe a .fzz file for its PCB board size, in inches.
	 *
	 * Opens a hidden headless MainWindow to read the board geometry.
	 * Results are cached per path so the same file is never opened twice
	 * in a single session. Falls back to 50×30 mm on any failure.
	 */
	QSizeF probeBoardSize(const QString & fzzPath);

	/// Full layout + emit pipeline. Returns false (with reason) on failure.
	bool runPanelize(QStringList & outFiles, QString & outErr);

	// --- Inputs from the start dialog ------------------------------
	QString     m_currentSketchPath;
	int         m_copies;
	bool        m_allowRotate;
	QStringList m_blendPaths;

	// --- State ------------------------------------------------------
	QString                 m_lastOutputDir;
	bool                    m_generatedOnce = false;
	bool                    m_seeded = false;
	QHash<QString, QSizeF>  m_boardSizeCache; // path -> size in inches

	// --- Panel size controls ---------------------------------------
	QComboBox *      m_sizePreset = nullptr;
	QDoubleSpinBox * m_panelWidth = nullptr;
	QDoubleSpinBox * m_panelHeight = nullptr;
	QDoubleSpinBox * m_gutter = nullptr;
	QDoubleSpinBox * m_border = nullptr;
	QCheckBox *      m_addRails = nullptr;
	// Guards the preset<->spinbox two-way binding against feedback loops.
	bool             m_applyingPreset = false;

	// --- Separation controls ---------------------------------------
	QRadioButton *   m_sepNone = nullptr;
	QRadioButton *   m_sepVCut = nullptr;
	QRadioButton *   m_sepMouseBites = nullptr;
	QDoubleSpinBox * m_vcutWidth = nullptr;
	QComboBox *      m_vcutLayer = nullptr;
	QDoubleSpinBox * m_mbTabWidth = nullptr;
	QSpinBox *       m_mbHoles = nullptr;
	QDoubleSpinBox * m_mbHoleDia = nullptr;
	QDoubleSpinBox * m_mbHolePitch = nullptr;

	// --- Extras controls -------------------------------------------
	QCheckBox *      m_fiducials = nullptr;
	QDoubleSpinBox * m_fiducialDia = nullptr;
	QDoubleSpinBox * m_fiducialClear = nullptr;
	QCheckBox *      m_toolingHoles = nullptr;
	QDoubleSpinBox * m_toolingDia = nullptr;

	// --- Output -----------------------------------------------------
	QLineEdit *      m_outputDir = nullptr;
	QLabel *         m_statusLabel = nullptr;

	// --- Interactive surfaces --------------------------------------
	PanelLayoutEditor *   m_editor = nullptr;
	GerberPreviewWidget * m_preview = nullptr;
	QTabWidget *          m_tabs = nullptr;
	QPushButton *         m_generateButton = nullptr;
	QPushButton *         m_saveButton = nullptr;
};

#endif // PANELIZERINTERACTIVEDIALOG_H
