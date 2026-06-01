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

#include "panelizerinteractivedialog.h"
#include "panelizerpages/panellayouteditor.h"
#include "../gerberpreview/gerberpreviewwidget.h"

#include "../fapplication.h"
#include "../debugdialog.h"
#include "../mainwindow/mainwindow.h"
#include "../sketch/pcbsketchwidget.h"
#include "../items/itembase.h"
#include "../utils/graphicsutils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QTabWidget>
#include <QScrollArea>
#include <QSplitter>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>
#include <QShowEvent>
#include <QSignalBlocker>

// 1 inch = 25.4 mm. The engine speaks inches; every control here speaks
// millimetres, so this constant guards every boundary crossing.
static const double kMmToIn = 1.0 / 25.4;
static const double kInToMm = 25.4;

PanelizerInteractiveDialog::PanelizerInteractiveDialog(QWidget * parent,
                                                       const QString & currentSketchPath,
                                                       int copies,
                                                       bool allowRotate,
                                                       const QStringList & blendPaths)
	: QDialog(parent)
	, m_currentSketchPath(currentSketchPath)
	, m_copies(copies)
	, m_allowRotate(allowRotate)
	, m_blendPaths(blendPaths)
{
	buildUi();
}

// ======================================================================
// UI construction
// ======================================================================

void PanelizerInteractiveDialog::buildUi()
{
	setWindowTitle(tr("Panelize — Arrange && Generate"));

	QVBoxLayout * root = new QVBoxLayout(this);

	// A horizontal splitter: options on the left, the arrange/preview
	// tab stack on the right. The splitter lets power users widen the
	// canvas while keeping every control one glance away.
	QSplitter * splitter = new QSplitter(Qt::Horizontal, this);

	QWidget * options = buildOptionsPanel();
	splitter->addWidget(options);

	// Right side: tabs for {Arrange, Gerber Preview}. Generate switches
	// to the preview tab automatically so the result is front-and-centre.
	m_tabs = new QTabWidget(this);

	m_editor = new PanelLayoutEditor(this);
	m_editor->setGridStepMm(1.0);
	m_tabs->addTab(m_editor, tr("Arrange"));

	m_preview = new GerberPreviewWidget(this);
	m_tabs->addTab(m_preview, tr("Gerber Preview"));

	splitter->addWidget(m_tabs);
	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);
	splitter->setSizes(QList<int>() << 340 << 900);

	root->addWidget(splitter, 1);

	// --- Arrange toolbar (rotate/flip/guides) ----------------------
	QHBoxLayout * arrangeTools = new QHBoxLayout();
	QPushButton * rotateBtn = new QPushButton(tr("Rotate 90°"), this);
	QPushButton * flipBtn   = new QPushButton(tr("Flip"), this);
	QCheckBox *   guidesChk = new QCheckBox(tr("Alignment guides"), this);
	guidesChk->setChecked(true);
	QPushButton * autoBtn   = new QPushButton(tr("Auto-arrange"), this);
	QPushButton * fitBtn    = new QPushButton(tr("Zoom to fit"), this);
	arrangeTools->addWidget(rotateBtn);
	arrangeTools->addWidget(flipBtn);
	arrangeTools->addWidget(autoBtn);
	arrangeTools->addWidget(fitBtn);
	arrangeTools->addStretch();
	arrangeTools->addWidget(guidesChk);
	root->addLayout(arrangeTools);

	connect(rotateBtn, &QPushButton::clicked, m_editor, &PanelLayoutEditor::rotateSelectedClockwise);
	connect(flipBtn,   &QPushButton::clicked, m_editor, &PanelLayoutEditor::flipSelectedHorizontal);
	connect(autoBtn,   &QPushButton::clicked, this,     &PanelizerInteractiveDialog::reseedEditor);
	connect(fitBtn,    &QPushButton::clicked, m_editor, &PanelLayoutEditor::zoomToFit);
	connect(guidesChk, &QCheckBox::toggled,   m_editor, &PanelLayoutEditor::setGuidesVisible);

	// --- Status + bottom buttons -----------------------------------
	m_statusLabel = new QLabel(tr("Adjust the panel, arrange boards, then Generate."), this);
	m_statusLabel->setWordWrap(true);

	QHBoxLayout * bottom = new QHBoxLayout();
	bottom->addWidget(m_statusLabel, 1);

	m_generateButton = new QPushButton(tr("Generate Gerbers"), this);
	m_generateButton->setDefault(true);
	m_saveButton = new QPushButton(tr("Save && Close"), this);
	m_saveButton->setEnabled(false); // nothing to save until first generate
	QPushButton * cancelButton = new QPushButton(tr("Cancel"), this);

	bottom->addWidget(m_generateButton);
	bottom->addWidget(m_saveButton);
	bottom->addWidget(cancelButton);
	root->addLayout(bottom);

	connect(m_generateButton, &QPushButton::clicked, this, &PanelizerInteractiveDialog::onGenerate);
	connect(m_saveButton,     &QPushButton::clicked, this, &QDialog::accept);
	connect(cancelButton,     &QPushButton::clicked, this, &QDialog::reject);

	resize(1280, 800);
}

QWidget * PanelizerInteractiveDialog::buildOptionsPanel()
{
	// Everything sits in a scroll area so the column never forces the
	// window taller than the screen on small displays.
	QScrollArea * scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QWidget * panel = new QWidget(scroll);
	QVBoxLayout * col = new QVBoxLayout(panel);

	// --- Panel size group ------------------------------------------
	QGroupBox * sizeBox = new QGroupBox(tr("Panel size (mm)"), panel);
	QFormLayout * sizeForm = new QFormLayout(sizeBox);

	// Standard fab-shop sizes first, so the common case is one click. The
	// width/height spinboxes stay fully editable for anything bespoke;
	// editing them flips this selector to "Custom".
	m_sizePreset = new QComboBox(sizeBox);
	populateSizePresets();
	sizeForm->addRow(tr("Standard size:"), m_sizePreset);

	m_panelWidth = new QDoubleSpinBox(sizeBox);
	m_panelWidth->setRange(1.0, 2000.0);
	m_panelWidth->setDecimals(2);
	m_panelWidth->setValue(100.0);
	m_panelWidth->setSuffix(tr(" mm"));
	sizeForm->addRow(tr("Width:"), m_panelWidth);

	m_panelHeight = new QDoubleSpinBox(sizeBox);
	m_panelHeight->setRange(1.0, 2000.0);
	m_panelHeight->setDecimals(2);
	m_panelHeight->setValue(100.0);
	m_panelHeight->setSuffix(tr(" mm"));
	sizeForm->addRow(tr("Height:"), m_panelHeight);

	m_gutter = new QDoubleSpinBox(sizeBox);
	m_gutter->setRange(0.0, 50.0);
	m_gutter->setDecimals(2);
	m_gutter->setValue(2.0);
	m_gutter->setSuffix(tr(" mm"));
	sizeForm->addRow(tr("Board gap:"), m_gutter);

	m_border = new QDoubleSpinBox(sizeBox);
	m_border->setRange(0.0, 50.0);
	m_border->setDecimals(2);
	m_border->setValue(5.0);
	m_border->setSuffix(tr(" mm"));
	sizeForm->addRow(tr("Rail/border:"), m_border);

	m_addRails = new QCheckBox(tr("Add top && bottom rails"), sizeBox);
	m_addRails->setChecked(true);
	sizeForm->addRow(QString(), m_addRails);

	col->addWidget(sizeBox);

	// Re-seed the arrange editor whenever the panel geometry changes so
	// the on-canvas layout always reflects the current numbers.
	connect(m_panelWidth,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PanelizerInteractiveDialog::reseedEditor);
	connect(m_panelHeight, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PanelizerInteractiveDialog::reseedEditor);
	connect(m_gutter,      QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PanelizerInteractiveDialog::reseedEditor);
	connect(m_border,      QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PanelizerInteractiveDialog::reseedEditor);
	connect(m_addRails,    &QCheckBox::toggled,                                  this, &PanelizerInteractiveDialog::reseedEditor);

	// Preset selection drives the spinboxes; manual spinbox edits flip the
	// preset back to "Custom". m_applyingPreset breaks the feedback loop.
	connect(m_sizePreset,  QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PanelizerInteractiveDialog::applySizePreset);
	connect(m_panelWidth,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PanelizerInteractiveDialog::markCustomSize);
	connect(m_panelHeight, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PanelizerInteractiveDialog::markCustomSize);

	// --- Separation + extras ---------------------------------------
	col->addWidget(buildSeparationGroup());
	col->addWidget(buildExtrasGroup());

	// --- Output ----------------------------------------------------
	QGroupBox * outBox = new QGroupBox(tr("Output"), panel);
	QVBoxLayout * outLayout = new QVBoxLayout(outBox);
	QHBoxLayout * outRow = new QHBoxLayout();
	m_outputDir = new QLineEdit(outBox);
	m_outputDir->setPlaceholderText(tr("Gerber output folder…"));
	QPushButton * browse = new QPushButton(tr("Browse…"), outBox);
	outRow->addWidget(m_outputDir, 1);
	outRow->addWidget(browse);
	outLayout->addLayout(outRow);
	col->addWidget(outBox);
	connect(browse, &QPushButton::clicked, this, &PanelizerInteractiveDialog::browseOutputDir);

	// Default the output folder next to the source sketch.
	if (!m_currentSketchPath.isEmpty()) {
		const QString dir = QFileInfo(m_currentSketchPath).absolutePath()
			+ QStringLiteral("/panel_gerbers");
		m_outputDir->setText(dir);
	}

	col->addStretch();

	scroll->setWidget(panel);
	scroll->setMinimumWidth(320);
	return scroll;
}

QWidget * PanelizerInteractiveDialog::buildSeparationGroup()
{
	QGroupBox * box = new QGroupBox(tr("Board separation"), this);
	QGridLayout * g = new QGridLayout(box);

	m_sepNone       = new QRadioButton(tr("None"), box);
	m_sepVCut       = new QRadioButton(tr("V-cut (V-score)"), box);
	m_sepMouseBites = new QRadioButton(tr("Mouse bites"), box);
	m_sepVCut->setChecked(true); // matches the legacy default

	g->addWidget(m_sepNone, 0, 0, 1, 2);
	g->addWidget(m_sepVCut, 1, 0, 1, 2);

	m_vcutWidth = new QDoubleSpinBox(box);
	m_vcutWidth->setRange(1.0, 100.0);
	m_vcutWidth->setValue(10.0);
	m_vcutWidth->setSuffix(tr(" mils"));
	g->addWidget(new QLabel(tr("V-cut width:"), box), 2, 0);
	g->addWidget(m_vcutWidth, 2, 1);

	m_vcutLayer = new QComboBox(box);
	m_vcutLayer->addItems(QStringList() << "Edge_Cuts" << "Eco1_User" << "Eco2_User");
	g->addWidget(new QLabel(tr("V-cut layer:"), box), 3, 0);
	g->addWidget(m_vcutLayer, 3, 1);

	g->addWidget(m_sepMouseBites, 4, 0, 1, 2);

	m_mbTabWidth = new QDoubleSpinBox(box);
	m_mbTabWidth->setRange(0.1, 50.0);
	m_mbTabWidth->setDecimals(2);
	m_mbTabWidth->setValue(3.0);
	m_mbTabWidth->setSuffix(tr(" mm"));
	g->addWidget(new QLabel(tr("Tab width:"), box), 5, 0);
	g->addWidget(m_mbTabWidth, 5, 1);

	m_mbHoles = new QSpinBox(box);
	m_mbHoles->setRange(1, 50);
	m_mbHoles->setValue(5);
	g->addWidget(new QLabel(tr("Holes / tab:"), box), 6, 0);
	g->addWidget(m_mbHoles, 6, 1);

	m_mbHoleDia = new QDoubleSpinBox(box);
	m_mbHoleDia->setRange(0.1, 5.0);
	m_mbHoleDia->setDecimals(2);
	m_mbHoleDia->setValue(0.5);
	m_mbHoleDia->setSuffix(tr(" mm"));
	g->addWidget(new QLabel(tr("Hole dia:"), box), 7, 0);
	g->addWidget(m_mbHoleDia, 7, 1);

	m_mbHolePitch = new QDoubleSpinBox(box);
	m_mbHolePitch->setRange(0.1, 5.0);
	m_mbHolePitch->setDecimals(2);
	m_mbHolePitch->setValue(0.8);
	m_mbHolePitch->setSuffix(tr(" mm"));
	g->addWidget(new QLabel(tr("Hole pitch:"), box), 8, 0);
	g->addWidget(m_mbHolePitch, 8, 1);

	connect(m_sepNone,       &QRadioButton::toggled, this, &PanelizerInteractiveDialog::updateSeparationEnabled);
	connect(m_sepVCut,       &QRadioButton::toggled, this, &PanelizerInteractiveDialog::updateSeparationEnabled);
	connect(m_sepMouseBites, &QRadioButton::toggled, this, &PanelizerInteractiveDialog::updateSeparationEnabled);
	updateSeparationEnabled();

	return box;
}

QWidget * PanelizerInteractiveDialog::buildExtrasGroup()
{
	QGroupBox * box = new QGroupBox(tr("Extras"), this);
	QGridLayout * g = new QGridLayout(box);

	m_fiducials = new QCheckBox(tr("Fiducials"), box);
	m_fiducials->setChecked(true);
	g->addWidget(m_fiducials, 0, 0, 1, 2);

	m_fiducialDia = new QDoubleSpinBox(box);
	m_fiducialDia->setRange(1.0, 200.0);
	m_fiducialDia->setValue(40.0);
	m_fiducialDia->setSuffix(tr(" mils"));
	g->addWidget(new QLabel(tr("Fiducial dia:"), box), 1, 0);
	g->addWidget(m_fiducialDia, 1, 1);

	m_fiducialClear = new QDoubleSpinBox(box);
	m_fiducialClear->setRange(1.0, 400.0);
	m_fiducialClear->setValue(80.0);
	m_fiducialClear->setSuffix(tr(" mils"));
	g->addWidget(new QLabel(tr("Fiducial clear:"), box), 2, 0);
	g->addWidget(m_fiducialClear, 2, 1);

	m_toolingHoles = new QCheckBox(tr("Tooling holes"), box);
	m_toolingHoles->setChecked(false);
	g->addWidget(m_toolingHoles, 3, 0, 1, 2);

	m_toolingDia = new QDoubleSpinBox(box);
	m_toolingDia->setRange(0.01, 1.0);
	m_toolingDia->setDecimals(3);
	m_toolingDia->setValue(0.125);
	m_toolingDia->setSuffix(tr(" in"));
	g->addWidget(new QLabel(tr("Tooling dia:"), box), 4, 0);
	g->addWidget(m_toolingDia, 4, 1);

	connect(m_fiducials,    &QCheckBox::toggled, this, &PanelizerInteractiveDialog::updateExtrasEnabled);
	connect(m_toolingHoles, &QCheckBox::toggled, this, &PanelizerInteractiveDialog::updateExtrasEnabled);
	updateExtrasEnabled();

	return box;
}

void PanelizerInteractiveDialog::updateSeparationEnabled()
{
	const bool vcut = m_sepVCut->isChecked();
	const bool mb   = m_sepMouseBites->isChecked();
	m_vcutWidth->setEnabled(vcut);
	m_vcutLayer->setEnabled(vcut);
	m_mbTabWidth->setEnabled(mb);
	m_mbHoles->setEnabled(mb);
	m_mbHoleDia->setEnabled(mb);
	m_mbHolePitch->setEnabled(mb);
}

void PanelizerInteractiveDialog::updateExtrasEnabled()
{
	const bool fid = m_fiducials->isChecked();
	m_fiducialDia->setEnabled(fid);
	m_fiducialClear->setEnabled(fid);
	m_toolingDia->setEnabled(m_toolingHoles->isChecked());
}

// ======================================================================
// Standard panel sizes
// ======================================================================

/**
 * @brief Fill the size dropdown with common fab-shop panel dimensions.
 *
 * Index 0 is always "Custom" so the user can fall back to free-form
 * width/height. Each preset stores its millimetre width in Qt::UserRole
 * and height in Qt::UserRole + 1, read back verbatim by applySizePreset().
 * The list covers the JLC/PCBWay/OSHPark size ladder plus the classic
 * Eurocard formats — the sizes a hobbyist or small shop actually orders.
 */
void PanelizerInteractiveDialog::populateSizePresets()
{
	struct Preset { const char * label; double w; double h; };
	static const Preset presets[] = {
		{ "50 × 50 mm",                  50.0,  50.0 },
		{ "50 × 100 mm",                 50.0,  100.0 },
		{ "80 × 100 mm (Half Eurocard)", 80.0,  100.0 },
		{ "100 × 100 mm",                100.0, 100.0 },
		{ "100 × 160 mm (Eurocard)",     100.0, 160.0 },
		{ "100 × 200 mm",                100.0, 200.0 },
		{ "150 × 150 mm",                150.0, 150.0 },
		{ "150 × 200 mm",                150.0, 200.0 },
		{ "160 × 233 mm (Double Eurocard)", 160.0, 233.35 },
		{ "200 × 200 mm",                200.0, 200.0 },
		{ "200 × 250 mm",                200.0, 250.0 },
		{ "200 × 300 mm",                200.0, 300.0 },
		{ "250 × 250 mm",                250.0, 250.0 },
		{ "300 × 300 mm",                300.0, 300.0 },
		{ "300 × 400 mm",                300.0, 400.0 },
	};

	m_sizePreset->addItem(tr("Custom…"));
	for (const Preset & p : presets) {
		m_sizePreset->addItem(QString::fromUtf8(p.label));
		const int idx = m_sizePreset->count() - 1;
		m_sizePreset->setItemData(idx, p.w, Qt::UserRole);
		m_sizePreset->setItemData(idx, p.h, Qt::UserRole + 1);
	}
}

void PanelizerInteractiveDialog::applySizePreset(int index)
{
	if (index <= 0) return; // "Custom" — leave the spinboxes as-is.

	const double w = m_sizePreset->itemData(index, Qt::UserRole).toDouble();
	const double h = m_sizePreset->itemData(index, Qt::UserRole + 1).toDouble();
	if (w <= 0.0 || h <= 0.0) return;

	// Push the preset into the spinboxes without letting their
	// valueChanged handlers bounce the selector back to "Custom".
	m_applyingPreset = true;
	m_panelWidth->setValue(w);
	m_panelHeight->setValue(h);
	m_applyingPreset = false;

	// One re-seed for the pair, rather than two from the spinbox signals.
	reseedEditor();
}

void PanelizerInteractiveDialog::markCustomSize()
{
	// A genuine user edit means the size no longer matches a preset.
	if (m_applyingPreset) return;
	if (m_sizePreset != nullptr && m_sizePreset->currentIndex() != 0) {
		const QSignalBlocker block(m_sizePreset);
		m_sizePreset->setCurrentIndex(0);
	}
}

// ======================================================================
// Spec builders
// ======================================================================

PanelizerEngine::PanelSpec PanelizerInteractiveDialog::currentPanelSpec() const
{
	PanelizerEngine::PanelSpec spec;
	spec.panelSizeInches = QSizeF(m_panelWidth->value() * kMmToIn,
	                              m_panelHeight->value() * kMmToIn);
	spec.gutterInches = m_gutter->value() * kMmToIn;
	spec.borderInches = m_border->value() * kMmToIn;
	spec.addRails     = m_addRails->isChecked();
	return spec;
}

PanelizerEngine::SeparationSpec PanelizerInteractiveDialog::currentSeparationSpec() const
{
	PanelizerEngine::SeparationSpec sep;
	if (m_sepNone->isChecked())            sep.kind = PanelizerEngine::Separation::None;
	else if (m_sepMouseBites->isChecked()) sep.kind = PanelizerEngine::Separation::MouseBites;
	else                                   sep.kind = PanelizerEngine::Separation::VCut;
	sep.vcutLineWidthMils = m_vcutWidth->value();
	sep.vcutLayer         = m_vcutLayer->currentText();
	sep.tabWidthInches    = m_mbTabWidth->value() * kMmToIn;
	sep.holesPerTab       = m_mbHoles->value();
	// Hole dia/pitch controls are mm; engine wants mils (mm → in → mils).
	sep.holeDiameterMils  = m_mbHoleDia->value()   * kMmToIn * 1000.0;
	sep.holePitchMils     = m_mbHolePitch->value() * kMmToIn * 1000.0;
	return sep;
}

PanelizerEngine::ExtrasSpec PanelizerInteractiveDialog::currentExtrasSpec() const
{
	PanelizerEngine::ExtrasSpec extras;
	extras.addFiducials              = m_fiducials->isChecked();
	extras.fiducialDiameterMils      = m_fiducialDia->value();
	extras.fiducialClearMils         = m_fiducialClear->value();
	extras.addToolingHoles           = m_toolingHoles->isChecked();
	extras.toolingHoleDiameterInches = m_toolingDia->value();
	return extras;
}

QList<PanelizerEngine::SourceBoard> PanelizerInteractiveDialog::currentSources()
{
	QList<PanelizerEngine::SourceBoard> sources;

	// The current sketch gets the requested copy count.
	PanelizerEngine::SourceBoard primary;
	primary.fzzPath        = m_currentSketchPath;
	primary.openWindow     = nullptr;
	primary.copies         = m_copies;
	primary.allowRotate90  = m_allowRotate;
	primary.boardSizeInches = probeBoardSize(m_currentSketchPath);
	sources.append(primary);

	// Each blended-in board gets one copy; its real size is probed so the
	// packer reserves the correct footprint even though the arrange editor
	// draws every instance at the primary board's size.
	// TODO(fritzing): teach PanelLayoutEditor to render heterogeneous
	// board footprints so blended boards look right on the arrange canvas.
	for (const QString & path : m_blendPaths) {
		PanelizerEngine::SourceBoard sb;
		sb.fzzPath        = path;
		sb.openWindow     = nullptr;
		sb.copies         = 1;
		sb.allowRotate90  = m_allowRotate;
		sb.boardSizeInches = probeBoardSize(path);
		sources.append(sb);
	}

	return sources;
}

// ======================================================================
// Board geometry probe (headless)
// ======================================================================

QSizeF PanelizerInteractiveDialog::probeBoardSize(const QString & fzzPath)
{
	// Cache hit: never re-open the same .fzz twice in one session.
	if (m_boardSizeCache.contains(fzzPath)) return m_boardSizeCache.value(fzzPath);

	QSizeF result(50.0 * kMmToIn, 30.0 * kMmToIn); // legacy fallback

	FApplication * fapp = qobject_cast<FApplication*>(qApp);
	QFileInfo fi(fzzPath);
	if (fapp != nullptr && fi.exists()) {
		// openWindowForService(false, 3) returns a hidden MainWindow
		// suitable for headless geometry probing. setCloseSilently()
		// suppresses the "save changes?" prompt on tear-down.
		MainWindow * probe = fapp->openWindowForService(false, 3);
		if (probe != nullptr) {
			// Keep the probe window completely off-screen. WA_DontShowOnScreen
			// guarantees it never flashes into view even if loadWhich()
			// internally calls show()/raise() — the old behaviour the user
			// saw was a second Fritzing window popping up mid-probe.
			probe->setAttribute(Qt::WA_DontShowOnScreen, true);
			probe->hide();
			probe->setCloseSilently(true);
			if (probe->loadWhich(fzzPath, false, false, false, QString())) {
				QList<ItemBase*> boards = probe->pcbView()->findBoard();
				if (!boards.isEmpty() && boards.first() != nullptr) {
					ItemBase * b = boards.first();
					const QRectF sbr = b->layerKinChief()->sceneBoundingRect();
					const QSizeF sz = sbr.size() / GraphicsUtils::SVGDPI;
					if (sz.width() > 0 && sz.height() > 0) {
						result = sz;
					}
				}
			}
			probe->close();
			delete probe;
		}
	}

	m_boardSizeCache.insert(fzzPath, result);
	DebugDialog::debug(QString("[Panelize] probeBoardSize(%1) = %2 x %3 in")
		.arg(fi.fileName()).arg(result.width()).arg(result.height()));
	return result;
}

// ======================================================================
// Seeding + generation
// ======================================================================

void PanelizerInteractiveDialog::showEvent(QShowEvent * event)
{
	QDialog::showEvent(event);
	// Seed the arrange editor once, the first time we become visible, so
	// the probe + initial layout happen after the window is up (keeps the
	// open feeling responsive rather than blocking the click that spawned us).
	if (!m_seeded) {
		m_seeded = true;
		// Probe every source board behind a loading dialog first; the
		// hidden Fritzing windows that probing spins up are the slow part,
		// and the user should see "Loading…" rather than a frozen window.
		preloadBoardSizes();
		reseedEditor();
	}
}

/**
 * @brief Open + measure every source board up-front, with a busy dialog.
 *
 * probeBoardSize() caches per path, so doing them all here means the
 * later reseed/generate calls are instant cache hits and the costly
 * headless-window work happens exactly once, with visible feedback.
 */
void PanelizerInteractiveDialog::preloadBoardSizes()
{
	QStringList paths;
	paths << m_currentSketchPath;
	for (const QString & p : m_blendPaths) {
		if (!p.isEmpty()) paths << p;
	}

	QProgressDialog progress(tr("Loading boards…"), QString(), 0, paths.size(), this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setMinimumDuration(0);
	progress.setValue(0);
	progress.show();
	QApplication::processEvents();

	int done = 0;
	for (const QString & p : paths) {
		progress.setLabelText(tr("Loading %1…").arg(QFileInfo(p).fileName()));
		QApplication::processEvents();
		probeBoardSize(p); // populates m_boardSizeCache
		progress.setValue(++done);
		QApplication::processEvents();
	}
	progress.close();
}

void PanelizerInteractiveDialog::reseedEditor()
{
	if (m_editor == nullptr) return;

	const QList<PanelizerEngine::SourceBoard> sources = currentSources();
	const PanelizerEngine::PanelSpec spec = currentPanelSpec();

	QString err;
	QList<PanelizerEngine::PlacedBoard*> placed;
	const bool ok = PanelizerEngine::layout(sources, spec, placed, &err);

	if (!ok || placed.isEmpty()) {
		// Don't wipe a usable prior layout; just warn. The user can
		// enlarge the panel or drop the copy count and try again.
		qDeleteAll(placed);
		m_statusLabel->setText(tr("⚠ These boards don't fit: %1")
			.arg(err.isEmpty() ? tr("panel too small") : err));
		m_statusLabel->setStyleSheet(QStringLiteral("color:#b00020;"));
		return;
	}

	// Convert engine inches → editor millimetres for seeding.
	QList<QPointF> seedTops;
	QList<bool>    seedRot;
	for (PanelizerEngine::PlacedBoard * pb : placed) {
		seedTops << QPointF(pb->positionInches.x() * kInToMm,
		                    pb->positionInches.y() * kInToMm);
		seedRot  << pb->rotated90;
	}

	const QSizeF boardMm = probeBoardSize(m_currentSketchPath) * kInToMm;
	const QSizeF panelMm(m_panelWidth->value(), m_panelHeight->value());
	const double borderMm = m_border->value();

	m_editor->seed(panelMm, borderMm, boardMm, seedTops, seedRot);
	m_editor->zoomToFit();

	qDeleteAll(placed);

	m_statusLabel->setText(tr("✔ %1 boards fit. Drag to arrange, then Generate.")
		.arg(seedTops.size()));
	m_statusLabel->setStyleSheet(QString());
}

void PanelizerInteractiveDialog::browseOutputDir()
{
	const QString start = m_outputDir->text().isEmpty()
		? QFileInfo(m_currentSketchPath).absolutePath()
		: m_outputDir->text();
	const QString dir = QFileDialog::getExistingDirectory(
		this, tr("Choose Gerber output folder"), start);
	if (!dir.isEmpty()) m_outputDir->setText(dir);
}

void PanelizerInteractiveDialog::onGenerate()
{
	if (m_outputDir->text().trimmed().isEmpty()) {
		QMessageBox::warning(this, tr("Panelize"),
			tr("Choose a Gerber output folder first."));
		return;
	}

	QProgressDialog progress(tr("Generating panel Gerbers…"), QString(), 0, 0, this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setMinimumDuration(0);
	progress.show();
	QApplication::processEvents();

	QStringList files;
	QString err;
	const bool ok = runPanelize(files, err);
	progress.close();

	if (!ok) {
		m_statusLabel->setText(tr("⚠ Generate failed."));
		m_statusLabel->setStyleSheet(QStringLiteral("color:#b00020;"));
		QMessageBox::warning(this, tr("Panelize"), err);
		return;
	}

	// Load the freshly written Gerbers into the embedded preview and
	// bring that tab forward, keeping the window open for another pass.
	m_preview->loadFiles(files);
	m_preview->zoomToFit();
	m_tabs->setCurrentWidget(m_preview);

	m_generatedOnce = true;
	m_saveButton->setEnabled(true);
	m_statusLabel->setText(tr("✔ Generated %1 Gerber files into %2")
		.arg(files.size()).arg(m_lastOutputDir));
	m_statusLabel->setStyleSheet(QString());
}

bool PanelizerInteractiveDialog::runPanelize(QStringList & outFiles, QString & outErr)
{
	DebugDialog::debug("[Panelize] interactive runPanelize() entered");

	const PanelizerEngine::PanelSpec      spec   = currentPanelSpec();
	const PanelizerEngine::SeparationSpec sep    = currentSeparationSpec();
	const PanelizerEngine::ExtrasSpec     extras = currentExtrasSpec();
	QList<PanelizerEngine::SourceBoard>   sources = currentSources();

	const QString outputDir = m_outputDir->text().trimmed();

	QString errorOut;
	QList<PanelizerEngine::PlacedBoard*> placed;
	if (!PanelizerEngine::layout(sources, spec, placed, &errorOut)) {
		qDeleteAll(placed);
		outErr = tr("Layout failed: %1")
			.arg(errorOut.isEmpty() ? tr("the boards do not fit the panel") : errorOut);
		return false;
	}

	// Honour the interactive arrange editor: when the per-instance counts
	// match, the user's hand placements override the auto-layout verbatim.
	const QList<PanelLayoutEditor::Placement> userPl = m_editor->placements();
	if (userPl.size() == placed.size()) {
		for (int i = 0; i < placed.size(); ++i) {
			placed[i]->positionInches    = userPl[i].topLeftInches;
			placed[i]->rotationDegrees   = userPl[i].rotationDegrees;
			placed[i]->flippedHorizontal = userPl[i].flippedHorizontal;
			// Keep the legacy 90°-set selector in sync so the renderer
			// picks the right pre-rendered SVG base set.
			placed[i]->rotated90 =
				(userPl[i].rotationDegrees == 90 || userPl[i].rotationDegrees == 270);
		}
		DebugDialog::debug(QString("[Panelize] applied %1 user placements").arg(userPl.size()));
	}

	FApplication * app = qobject_cast<FApplication*>(qApp);
	PanelizerEngine::Result result =
		PanelizerEngine::emitPanel(placed, spec, sep, extras, outputDir, app);

	qDeleteAll(placed);
	placed.clear();

	if (!result.success) {
		outErr = tr("Emit failed:\n%1").arg(result.warnings.join("\n"));
		return false;
	}

	outFiles = result.gerberFiles;
	m_lastOutputDir = result.gerberDir;
	return true;
}
