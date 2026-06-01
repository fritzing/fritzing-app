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

#include "panelpreviewpage.h"
#include "../../gerberpreview/gerberpreviewdialog.h"
#include "../../gerberpreview/gerberpreviewwidget.h"
#include "../../gerberpreview/layermanagerwidget.h"

#include <QLineEdit>
#include <QCheckBox>
#include <QFileDialog>
#include <QPushButton>
#include <QLabel>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDir>
#include <QHash>
#include <QMetaEnum>
#include <QSettings>
#include <QVector>

PanelPreviewPage::PanelPreviewPage(QWidget *parent)
	: QWizardPage(parent)
	, m_preview(nullptr)
	, m_browseButton(nullptr)
	, m_outputEdit(nullptr)
	, m_saveFzzCheck(nullptr)
	, m_layerManager(nullptr)
	, m_statusLabel(nullptr)
	, m_openFullButton(nullptr)
{
	setTitle(tr("Preview & Export"));
	setSubTitle(tr("Review the panel layout and choose the output location"));

	m_preview = new GerberPreviewWidget(this);
	// Embedded preview is intentionally generous — the wizard itself
	// is resized to ~1100x800 by PanelizerWizard so this fits without
	// being cropped. The user can still click "Open in full window"
	// for a free-floating resizable preview.
	m_preview->setMinimumHeight(480);
	m_preview->setMinimumWidth(640);

	m_saveFzzCheck = new QCheckBox(tr("Also save panel as .fzz"), this);
	m_saveFzzCheck->setChecked(true);
	m_statusLabel  = new QLabel(this);

	// Output folder field lives on the Sources page now (so the user
	// picks it before the render kicks off when this page is entered).
	// These widgets are kept null so the rest of the page-internal API
	// continues to compile without changes.
	m_outputEdit   = nullptr;
	m_browseButton = nullptr;

	// Full per-layer manager: one row per loaded layer with a
	// visibility checkbox, a colour swatch (click to pick), and an
	// opacity slider. Replaces the old fixed four-checkbox row so the
	// user can recolour any layer right in the wizard preview. Colours
	// are persisted (and shared with the standalone preview dialog) via
	// the same QSettings keys.
	m_layerManager = new LayerManagerWidget(this);
	m_layerManager->setMaximumHeight(180);

	// "Open in full window" pops the standalone GerberPreviewDialog
	// on the same set of files. Disabled until showGerbers() has run.
	m_openFullButton = new QPushButton(tr("Open in full preview window..."), this);
	m_openFullButton->setEnabled(false);
	m_openFullButton->setToolTip(tr("Open the rendered Gerbers in a larger, resizable preview window with full layer controls."));

	auto *layerRow = new QHBoxLayout;
	layerRow->addWidget(m_layerManager, 1);

	auto *buttonRow = new QHBoxLayout;
	buttonRow->addStretch(1);
	buttonRow->addWidget(m_openFullButton);

	auto *outer = new QVBoxLayout;
	outer->addWidget(m_preview, 1);
	outer->addLayout(layerRow);
	outer->addLayout(buttonRow);
	outer->addWidget(m_saveFzzCheck);
	outer->addWidget(m_statusLabel);
	setLayout(outer);

	registerField("panel.saveFzz",    m_saveFzzCheck);

	connect(m_layerManager, &LayerManagerWidget::visibilityChanged,
	        this, &PanelPreviewPage::onLayerVisibilityChanged);
	connect(m_layerManager, &LayerManagerWidget::colorChanged,
	        this, &PanelPreviewPage::onLayerColorChanged);
	// Mirror on-canvas legend interactions into the side manager +
	// persistence so the corner key and the row list never disagree.
	connect(m_preview, &GerberPreviewWidget::legendVisibilityToggled,
	        this, [this](GerberPreviewWidget::LayerKind k, bool on) {
		if (m_layerManager != nullptr) m_layerManager->setInitialVisibility(k, on);
		onLayerVisibilityChanged(k, on);
	});
	connect(m_preview, &GerberPreviewWidget::legendColorChanged,
	        this, [this](GerberPreviewWidget::LayerKind k, const QColor &c) {
		if (m_layerManager != nullptr) m_layerManager->setRowColor(k, c);
		onLayerColorChanged(k, c);
	});
	connect(m_openFullButton, &QPushButton::clicked,   this, &PanelPreviewPage::onOpenFullPreview);
}

PanelPreviewPage::~PanelPreviewPage()
{
}

bool PanelPreviewPage::isComplete() const
{
	// Output folder validation happens on the Sources page; this page
	// is informational once the render has completed.
	return true;
}

QString PanelPreviewPage::outputDir() const
{
	// Kept for backward compatibility - the wizard now reads the
	// output folder directly from the Sources page field. Returning
	// the field value here lets older call sites keep working.
	return field("panel.outputDir").toString();
}

bool PanelPreviewPage::savePanelFzz() const
{
	return m_saveFzzCheck->isChecked();
}

void PanelPreviewPage::onBrowseOutput()
{
	// No-op: output folder picker moved to the Sources page.
}

void PanelPreviewPage::onPreviewReady()
{
}

QString PanelPreviewPage::settingsKeyFor(
	GerberPreviewWidget::LayerKind kind, const QString &suffix) const
{
	// Mirror GerberPreviewDialog::settingsKeyFor so colours chosen in the
	// inline preview and the standalone window share one persisted store.
	const QMetaEnum me = QMetaEnum::fromType<GerberPreviewWidget::LayerKind>();
	const char *name = me.valueToKey(kind);
	return QStringLiteral("layer.%1.%2").arg(name ? name : "Unknown", suffix);
}

void PanelPreviewPage::onLayerVisibilityChanged(
	GerberPreviewWidget::LayerKind kind, bool on)
{
	if (m_preview != nullptr) m_preview->setLayerVisible(kind, on);
	QSettings s;
	s.beginGroup("preview/gerber");
	s.setValue(settingsKeyFor(kind, "visible"), on);
	s.endGroup();
}

void PanelPreviewPage::onLayerColorChanged(
	GerberPreviewWidget::LayerKind kind, const QColor &color)
{
	if (m_preview != nullptr) m_preview->setLayerColor(kind, color);
	QSettings s;
	s.beginGroup("preview/gerber");
	s.setValue(settingsKeyFor(kind, "color"), color);
	s.endGroup();
}

void PanelPreviewPage::showGerbers(const QStringList &paths)
{
	if (!m_preview) return;
	m_lastPaths = paths;
	m_preview->loadFiles(paths);

	// Rebuild the layer-manager rows for whatever kinds the renderer
	// actually loaded, seeding each row's colour + visibility from the
	// persisted store (shared with the standalone preview dialog) and
	// pushing those values straight into the renderer so the preview
	// reflects the user's saved preferences immediately.
	if (m_layerManager != nullptr) {
		const QVector<GerberPreviewWidget::LayerKind> kinds = m_preview->loadedKinds();
		QSettings s;
		s.beginGroup("preview/gerber");
		QHash<GerberPreviewWidget::LayerKind, QColor> initialColors;
		for (const auto k : kinds) {
			const QVariant cv = s.value(settingsKeyFor(k, "color"));
			const QColor col = cv.isValid() ? cv.value<QColor>() : m_preview->layerColor(k);
			initialColors.insert(k, col);
			m_preview->setLayerColor(k, col);
			const bool vis = s.value(settingsKeyFor(k, "visible"), true).toBool();
			m_preview->setLayerVisible(k, vis);
		}
		m_layerManager->setLayers(kinds, initialColors);
		for (const auto k : kinds) {
			const bool vis = s.value(settingsKeyFor(k, "visible"), true).toBool();
			m_layerManager->setInitialVisibility(k, vis);
		}
		s.endGroup();
	}

	m_preview->zoomToFit();
	m_statusLabel->setText(tr("Loaded %1 layer(s).").arg(m_preview->layerCount()));
	if (m_openFullButton != nullptr) {
		m_openFullButton->setEnabled(!paths.isEmpty());
	}
}

void PanelPreviewPage::onOpenFullPreview()
{
	// Parent the standalone dialog to the wizard itself for now — it
	// stays open even after the wizard closes because it is non-modal
	// and uses WA_DeleteOnClose, but Qt will reparent it to the main
	// window automatically on wizard destruction.
	if (m_lastPaths.isEmpty()) return;

	// Single-instance guard: re-clicking the button while a preview
	// window is already open must NOT spawn another one. Loading
	// Gerbers is expensive (parse + render + layout) and opening five
	// of them in parallel will slow the whole system to a crawl.
	// QPointer auto-nulls when the dialog is destroyed, so checking
	// !m_fullPreviewDialog is enough to know we need a fresh one.
	if (m_fullPreviewDialog) {
		if (m_fullPreviewDialog->isMinimized()) {
			m_fullPreviewDialog->showNormal();
		} else {
			m_fullPreviewDialog->show();
		}
		m_fullPreviewDialog->raise();
		m_fullPreviewDialog->activateWindow();
		return;
	}

	// Debounce the button itself for a moment so a rapid double-click
	// cannot slip a second dialog through before the first one has
	// finished its ctor + openFiles() (which spins the event loop).
	if (m_openFullButton != nullptr) {
		m_openFullButton->setEnabled(false);
	}

	auto * dlg = new GerberPreviewDialog(this);
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	m_fullPreviewDialog = dlg;
	dlg->openFiles(m_lastPaths, tr("Panel Preview"));
	dlg->show();
	dlg->raise();
	dlg->activateWindow();

	// Re-enable the launcher button once the dialog is fully up and
	// once it has gone away — both conditions reuse the same lambda
	// because the button just mirrors "can the user open a (new) one".
	if (m_openFullButton != nullptr) {
		QPushButton * btn = m_openFullButton;
		connect(dlg, &QObject::destroyed, btn, [btn]() {
			btn->setEnabled(true);
		});
		btn->setEnabled(true);
	}
}

void PanelPreviewPage::setDefaultOutputDir(const QString &sketchPath)
{
	Q_UNUSED(sketchPath)
	// Output folder lives on the Sources page now; the wizard seeds the
	// default there. This method is kept as a no-op so the wizard ctor
	// continues to compile and link.
}

