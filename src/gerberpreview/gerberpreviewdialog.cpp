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

#include "gerberpreviewdialog.h"
#include "gerberpreviewwidget.h"
#include "layermanagerwidget.h"

#include <QAction>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMetaEnum>
#include <QSettings>
#include <QSplitter>
#include <QStringList>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWheelEvent>

GerberPreviewDialog::GerberPreviewDialog(QWidget *parent)
	: QDialog(parent)
	, m_preview(nullptr)
	, m_layerMgr(nullptr)
	, m_split(nullptr)
	, m_toolbar(nullptr)
	, m_status(nullptr)
{
	// Non-modal so the user can keep editing the sketch while
	// inspecting the exported files.
	setModal(false);
	setWindowTitle(tr("Gerber Preview"));
	setWindowFlags(windowFlags() | Qt::Window);
	resize(1000, 720);

	buildUi();
	restoreState();
}

GerberPreviewDialog::~GerberPreviewDialog()
{
}

void GerberPreviewDialog::buildUi()
{
	// The application-wide stylesheet (resources/styles/*.qss) is tuned
	// for the main window and leaves this auxiliary dialog's toolbar
	// text effectively white-on-white / unreadable. Apply a small,
	// self-consistent light theme scoped to this dialog so it always
	// looks like a normal window regardless of the inherited chrome.
	// NOTE: selectors are prefixed with the class name so they only
	// touch this dialog's widget tree, not the rest of the app.
	setStyleSheet(QStringLiteral(
		"GerberPreviewDialog { background: #ECECEC; }"
		"GerberPreviewDialog QToolBar { background: #E4E4E4; border: 1px solid #C2C2C2;"
		"    spacing: 3px; padding: 2px; }"
		"GerberPreviewDialog QToolButton { color: #202020; background: transparent;"
		"    padding: 4px 9px; border: 1px solid transparent; border-radius: 3px; }"
		"GerberPreviewDialog QToolButton:hover { background: #D4D4D4; border: 1px solid #B4B4B4; }"
		"GerberPreviewDialog QToolButton:pressed,"
		"GerberPreviewDialog QToolButton:checked { background: #BCD6F0; border: 1px solid #6F9BD1; }"
		"GerberPreviewDialog QLabel { color: #202020; }"
		"GerberPreviewDialog QCheckBox { color: #202020; }"));

	m_preview = new GerberPreviewWidget(this);
	m_preview->setMinimumSize(640, 480);
	connect(m_preview, &GerberPreviewWidget::loadFinished,
	        this, &GerberPreviewDialog::onLoadFinished);
	// Keep the side layer-manager + persisted style in sync when the
	// user interacts with the on-canvas legend (corner key).
	connect(m_preview, &GerberPreviewWidget::legendVisibilityToggled,
	        this, [this](GerberPreviewWidget::LayerKind k, bool on) {
		if (m_layerMgr != nullptr) m_layerMgr->setInitialVisibility(k, on);
		m_persistedVisible.insert(k, on);
	});
	connect(m_preview, &GerberPreviewWidget::legendColorChanged,
	        this, [this](GerberPreviewWidget::LayerKind k, const QColor &c) {
		if (m_layerMgr != nullptr) m_layerMgr->setRowColor(k, c);
		m_persistedColors.insert(k, c);
	});

	m_layerMgr = new LayerManagerWidget(this);
	m_layerMgr->setMinimumWidth(260);
	connect(m_layerMgr, &LayerManagerWidget::visibilityChanged,
	        this, &GerberPreviewDialog::onLayerVisibilityChanged);
	connect(m_layerMgr, &LayerManagerWidget::colorChanged,
	        this, &GerberPreviewDialog::onLayerColorChanged);

	// --- Toolbar ---------------------------------------------------
	m_toolbar = new QToolBar(this);
	m_toolbar->addAction(tr("Open..."),  this, &GerberPreviewDialog::onOpen);
	m_toolbar->addAction(tr("Open Folder..."),
	                     this, &GerberPreviewDialog::onOpenDir);
	m_recentMenu = new QMenu(this);
	m_recentAction = m_toolbar->addAction(tr("Recent"));
	m_recentAction->setMenu(m_recentMenu);
	m_recentAction->setToolTip(tr("Re-open a recently-viewed Gerber folder."));
	// QToolBar normally won't trigger a menu on a plain QAction click —
	// we wire the popup explicitly so a single click drops the list.
	connect(m_recentAction, &QAction::triggered, this, [this]() {
		if (m_recentMenu == nullptr || m_recentMenu->actions().isEmpty()) return;
		if (auto * w = m_toolbar->widgetForAction(m_recentAction)) {
			m_recentMenu->exec(w->mapToGlobal(QPoint(0, w->height())));
		}
	});
	m_toolbar->addSeparator();
	m_toolbar->addAction(tr("Fit"),      this, &GerberPreviewDialog::onFit);
	m_toolbar->addAction(tr("Zoom +"),   this, &GerberPreviewDialog::onZoomIn);
	m_toolbar->addAction(tr("Zoom -"),   this, &GerberPreviewDialog::onZoomOut);
	m_toolbar->addSeparator();
	m_toolbar->addAction(tr("Rotate 90"), this, &GerberPreviewDialog::onRotate);
	m_toolbar->addAction(tr("Flip H"),    this, &GerberPreviewDialog::onFlipH);
	m_toolbar->addAction(tr("Flip V"),    this, &GerberPreviewDialog::onFlipV);
	m_toolbar->addAction(tr("Reset view"), this, &GerberPreviewDialog::onResetView);
	m_toolbar->addSeparator();
	m_measureAction = m_toolbar->addAction(tr("Measure"));
	m_measureAction->setCheckable(true);
	m_measureAction->setToolTip(tr("Toggle measurement ruler. Click two points to read "
	                               "distance, \u0394x and \u0394y in mm and inches. "
	                               "Press Escape (or untoggle) to clear."));
	connect(m_measureAction, &QAction::toggled,
	        this, &GerberPreviewDialog::onToggleMeasure);
	connect(m_preview, &GerberPreviewWidget::measurementChanged,
	        this, &GerberPreviewDialog::onMeasurementChanged);
	connect(m_preview, &GerberPreviewWidget::apertureSelected,
	        this, &GerberPreviewDialog::onApertureSelected);

	// --- Splitter: canvas | layer manager --------------------------
	m_split = new QSplitter(Qt::Horizontal, this);
	m_split->addWidget(m_preview);
	m_split->addWidget(m_layerMgr);
	m_split->setStretchFactor(0, 4);
	m_split->setStretchFactor(1, 1);
	m_split->setCollapsible(0, false);

	m_status = new QLabel(this);

	auto * outer = new QVBoxLayout(this);
	outer->setContentsMargins(6, 6, 6, 6);
	outer->addWidget(m_toolbar);
	outer->addWidget(m_split, 1);
	outer->addWidget(m_status);
}

void GerberPreviewDialog::openFiles(const QStringList &paths, const QString &title)
{
	m_lastPaths = paths;
	if (!title.isEmpty()) {
		setWindowTitle(tr("Gerber Preview - %1").arg(title));
	}
	if (m_preview != nullptr) {
		m_preview->loadFiles(paths);
		m_preview->zoomToFit();
	}
	// Track the parent folder of the first file as a recent entry so a
	// user who opened files directly (not via a folder pick) still gets
	// the directory remembered in the MRU.
	if (!paths.isEmpty()) {
		QFileInfo fi(paths.first());
		if (fi.exists()) pushRecentDir(fi.absolutePath());
	}
	// Layer manager + persisted state apply once loadFinished() fires
	// (so we have the actual loaded kinds to drive rows from).
}

QStringList GerberPreviewDialog::openDirectory(const QString &directory,
                                               const QString &title)
{
	QDir d(directory);
	if (!d.exists()) return QStringList();

	QStringList filters;
	filters << "*.gbr" << "*.gtl" << "*.gbl" << "*.gts" << "*.gbs"
	        << "*.gto" << "*.gbo" << "*.gtp" << "*.gbp"
	        << "*.gko" << "*.gm1" << "*.gm2" << "*.gml"
	        << "*.drl" << "*.txt" << "*.xln";

	const QFileInfoList entries = d.entryInfoList(filters, QDir::Files | QDir::Readable);
	QStringList paths;
	paths.reserve(entries.size());
	for (const QFileInfo &fi : entries) {
		paths.append(fi.absoluteFilePath());
	}
	openFiles(paths, title.isEmpty() ? d.dirName() : title);
	pushRecentDir(d.absolutePath());
	return paths;
}

void GerberPreviewDialog::onOpen()
{
	const QStringList paths = QFileDialog::getOpenFileNames(this,
		tr("Open Gerber / Excellon files"),
		m_lastPaths.isEmpty() ? QString() : m_lastPaths.first(),
		tr("Gerber/Excellon (*.gbr *.gtl *.gbl *.gts *.gbs *.gto *.gbo "
		   "*.gtp *.gbp *.gko *.gm1 *.gm2 *.gml *.drl *.txt *.xln);;"
		   "All files (*)"));
	if (paths.isEmpty()) return;
	openFiles(paths);
}

void GerberPreviewDialog::onOpenDir()
{
	const QString seed = !m_recentDirs.isEmpty()
		? m_recentDirs.first()
		: (m_lastPaths.isEmpty() ? QString() : QFileInfo(m_lastPaths.first()).absolutePath());
	const QString dir = QFileDialog::getExistingDirectory(this,
		tr("Open Gerber folder"), seed);
	if (dir.isEmpty()) return;
	openDirectory(dir);
}

void GerberPreviewDialog::onRecentSelected()
{
	auto * act = qobject_cast<QAction*>(sender());
	if (act == nullptr) return;
	const QString dir = act->data().toString();
	if (dir.isEmpty()) return;
	openDirectory(dir);
}

void GerberPreviewDialog::onFit()
{
	if (m_preview != nullptr) m_preview->zoomToFit();
}

void GerberPreviewDialog::onZoomIn()
{
	// Synthesize a wheel event around the widget center. The renderer
	// already handles zoom-around-cursor; emulating it here keeps the
	// behavior consistent between the toolbar button and the wheel.
	if (m_preview == nullptr) return;
	const QPointF center(m_preview->width() / 2.0, m_preview->height() / 2.0);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	QWheelEvent we(center, m_preview->mapToGlobal(center.toPoint()),
	               QPoint(0, 0), QPoint(0, 120),
	               Qt::NoButton, Qt::NoModifier,
	               Qt::NoScrollPhase, /*inverted*/ false);
#else
	QWheelEvent we(center, QPointF(), QPoint(0, 0), QPoint(0, 120),
	               Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
#endif
	QCoreApplication::sendEvent(m_preview, &we);
}

void GerberPreviewDialog::onZoomOut()
{
	if (m_preview == nullptr) return;
	const QPointF center(m_preview->width() / 2.0, m_preview->height() / 2.0);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	QWheelEvent we(center, m_preview->mapToGlobal(center.toPoint()),
	               QPoint(0, 0), QPoint(0, -120),
	               Qt::NoButton, Qt::NoModifier,
	               Qt::NoScrollPhase, /*inverted*/ false);
#else
	QWheelEvent we(center, QPointF(), QPoint(0, 0), QPoint(0, -120),
	               Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
#endif
	QCoreApplication::sendEvent(m_preview, &we);
}

void GerberPreviewDialog::onRotate()
{
	if (m_preview != nullptr) m_preview->rotate90();
}

void GerberPreviewDialog::onFlipH()
{
	if (m_preview != nullptr) m_preview->flipHorizontal();
}

void GerberPreviewDialog::onFlipV()
{
	if (m_preview != nullptr) m_preview->flipVertical();
}

void GerberPreviewDialog::onResetView()
{
	if (m_preview != nullptr) m_preview->resetViewTransform();
}

void GerberPreviewDialog::onToggleMeasure(bool on)
{
	if (m_preview == nullptr) return;
	m_preview->setTool(on
		? GerberPreviewWidget::Tool::Measure
		: GerberPreviewWidget::Tool::Pan);
	if (m_status != nullptr) {
		m_status->setText(on
			? tr("Measure: click and drag between two points. Escape to clear.")
			: m_baseStatus);
	}
}

void GerberPreviewDialog::onMeasurementChanged(double distMm, double dxMm, double dyMm)
{
	if (m_status == nullptr) return;
	// Idle update (start==end or cleared) restores the load summary so
	// the status bar never sits empty after the user cancels.
	if (distMm <= 0.0 && qFuzzyIsNull(dxMm) && qFuzzyIsNull(dyMm)) {
		m_status->setText(m_measureAction != nullptr && m_measureAction->isChecked()
			? tr("Measure: click and drag between two points. Escape to clear.")
			: m_baseStatus);
		return;
	}
	const double mmToIn = 1.0 / 25.4;
	m_status->setText(tr("Measure: %1 mm (%2 in)  \u0394x %3 mm  \u0394y %4 mm")
		.arg(distMm,         0, 'f', 3)
		.arg(distMm * mmToIn, 0, 'f', 4)
		.arg(dxMm,           0, 'f', 3)
		.arg(dyMm,           0, 'f', 3));
}

void GerberPreviewDialog::onApertureSelected(
	int dcode, const QString &description, int count)
{
	if (m_status == nullptr) return;
	if (dcode < 0) {
		// Cleared — don't trample an active measurement readout. If a
		// measurement is live, leave its text alone; otherwise restore
		// the baseline status (or the measure-mode prompt).
		if (m_measureAction != nullptr && m_measureAction->isChecked()) return;
		m_status->setText(m_baseStatus);
		return;
	}
	m_status->setText(tr("D%1: %2  \u2014  %3 occurrence(s) on this layer")
		.arg(dcode).arg(description).arg(count));
}

void GerberPreviewDialog::onLayerVisibilityChanged(
	GerberPreviewWidget::LayerKind kind, bool on)
{
	if (m_preview != nullptr) m_preview->setLayerVisible(kind, on);
}

void GerberPreviewDialog::onLayerColorChanged(
	GerberPreviewWidget::LayerKind kind, const QColor &color)
{
	if (m_preview != nullptr) m_preview->setLayerColor(kind, color);
}

void GerberPreviewDialog::onLoadFinished(int layerCount, const QStringList &warnings)
{
	// Build (or rebuild) the layer-manager rows for whatever kinds
	// the renderer actually loaded. Each row's initial color comes
	// from the persisted store if present, otherwise the renderer's
	// current per-layer color (which equals the palette default for
	// a fresh load).
	if (m_layerMgr != nullptr && m_preview != nullptr) {
		const auto kinds = m_preview->loadedKinds();
		QHash<GerberPreviewWidget::LayerKind, QColor> initialColors;
		for (const auto k : kinds) {
			initialColors.insert(k,
				m_persistedColors.value(k, m_preview->layerColor(k)));
		}
		m_layerMgr->setLayers(kinds, initialColors);
		applyPersistedLayerSettings();
	}

	QString msg = tr("Loaded %1 layer(s)").arg(layerCount);
	if (!warnings.isEmpty()) {
		// Keep the headline brief; full warning text shows in tooltip
		// so the status bar doesn't grow taller than one line.
		msg += tr(" - %1 warning(s)").arg(warnings.size());
		if (m_status != nullptr) m_status->setToolTip(warnings.join("\n"));
	} else if (m_status != nullptr) {
		m_status->setToolTip(QString());
	}
	if (m_status != nullptr) m_status->setText(msg);
	m_baseStatus = msg;
}

void GerberPreviewDialog::applyPersistedLayerSettings()
{
	if (m_preview == nullptr || m_layerMgr == nullptr) return;
	// Push persisted colors + visibility down into the renderer for
	// every kind currently loaded. The manager is also updated so
	// its checkboxes/swatches reflect what the user is seeing.
	const auto kinds = m_preview->loadedKinds();
	for (const auto k : kinds) {
		if (m_persistedColors.contains(k)) {
			m_preview->setLayerColor(k, m_persistedColors.value(k));
		}
		const bool vis = m_persistedVisible.value(k, true);
		m_preview->setLayerVisible(k, vis);
		m_layerMgr->setInitialVisibility(k, vis);
	}
}

void GerberPreviewDialog::closeEvent(QCloseEvent *event)
{
	saveState();
	QDialog::closeEvent(event);
}

QString GerberPreviewDialog::settingsKeyFor(
	GerberPreviewWidget::LayerKind kind, const QString &suffix) const
{
	// Use the Q_ENUM name so the QSettings keys are human-readable
	// in the platform store (e.g. "layer.TopCopper.color"). This
	// also keeps the keys stable across enum-value reorderings.
	const QMetaEnum me = QMetaEnum::fromType<GerberPreviewWidget::LayerKind>();
	const char * name = me.valueToKey(kind);
	return QStringLiteral("layer.%1.%2").arg(name ? name : "Unknown", suffix);
}

void GerberPreviewDialog::saveState() const
{
	QSettings s;
	s.beginGroup("preview/gerber");
	s.setValue("geometry", saveGeometry());
	if (m_split != nullptr) s.setValue("splitter", m_split->saveState());
	s.setValue("lastPaths", m_lastPaths);
	s.setValue("recentDirs", m_recentDirs);

	if (m_layerMgr != nullptr) {
		// Persist visibility + color for every row currently exposed.
		// Kinds not present in the latest load keep whatever value
		// was previously stored (we don't wipe absent kinds, so a
		// user's mask-color preference survives loading a Gerber set
		// that happens not to include a mask layer).
		const auto vis = m_layerMgr->visibilityMap();
		const auto col = m_layerMgr->colorMap();
		for (auto it = vis.constBegin(); it != vis.constEnd(); ++it) {
			s.setValue(settingsKeyFor(it.key(), "visible"), it.value());
		}
		for (auto it = col.constBegin(); it != col.constEnd(); ++it) {
			s.setValue(settingsKeyFor(it.key(), "color"), it.value());
		}
	}
	s.endGroup();
}

void GerberPreviewDialog::restoreState()
{
	QSettings s;
	s.beginGroup("preview/gerber");
	const QByteArray geom = s.value("geometry").toByteArray();
	if (!geom.isEmpty()) restoreGeometry(geom);
	const QByteArray splitterState = s.value("splitter").toByteArray();
	if (!splitterState.isEmpty() && m_split != nullptr) {
		m_split->restoreState(splitterState);
	}
	m_lastPaths = s.value("lastPaths").toStringList();
	m_recentDirs = s.value("recentDirs").toStringList();
	rebuildRecentMenu();

	// Pre-load persisted per-layer style. We can't yet apply it
	// because no files are loaded; applyPersistedLayerSettings()
	// pulls from these caches after each loadFiles().
	const QMetaEnum me = QMetaEnum::fromType<GerberPreviewWidget::LayerKind>();
	for (int i = 0; i < me.keyCount(); ++i) {
		const auto kind = static_cast<GerberPreviewWidget::LayerKind>(me.value(i));
		const QString colorKey = settingsKeyFor(kind, "color");
		const QString visKey   = settingsKeyFor(kind, "visible");
		if (s.contains(colorKey)) {
			m_persistedColors.insert(kind, s.value(colorKey).value<QColor>());
		}
		if (s.contains(visKey)) {
			m_persistedVisible.insert(kind, s.value(visKey).toBool());
		}
	}
	s.endGroup();
}

void GerberPreviewDialog::pushRecentDir(const QString &dir)
{
	if (dir.isEmpty()) return;
	const QString canonical = QFileInfo(dir).absoluteFilePath();
	m_recentDirs.removeAll(canonical);
	m_recentDirs.prepend(canonical);
	// Cap so the menu doesn't grow forever; 8 mirrors the rest of the
	// Fritzing app's File > Recent menus.
	constexpr int kMaxRecent = 8;
	while (m_recentDirs.size() > kMaxRecent) m_recentDirs.removeLast();
	rebuildRecentMenu();
}

void GerberPreviewDialog::rebuildRecentMenu()
{
	if (m_recentMenu == nullptr) return;
	m_recentMenu->clear();
	if (m_recentDirs.isEmpty()) {
		auto * a = m_recentMenu->addAction(tr("(no recent folders)"));
		a->setEnabled(false);
		if (m_recentAction != nullptr) m_recentAction->setEnabled(false);
		return;
	}
	if (m_recentAction != nullptr) m_recentAction->setEnabled(true);
	for (const QString &dir : m_recentDirs) {
		// Display: leaf folder name; full path as tooltip. Long
		// absolute paths in a tight toolbar menu eat horizontal
		// budget fast otherwise.
		QFileInfo fi(dir);
		auto * a = m_recentMenu->addAction(fi.fileName().isEmpty() ? dir : fi.fileName());
		a->setData(dir);
		a->setToolTip(dir);
		connect(a, &QAction::triggered,
		        this, &GerberPreviewDialog::onRecentSelected);
	}
	m_recentMenu->addSeparator();
	auto * clear = m_recentMenu->addAction(tr("Clear list"));
	connect(clear, &QAction::triggered, this, [this]() {
		m_recentDirs.clear();
		rebuildRecentMenu();
	});
}
