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

#include "panelizerwizard.h"
#include "panelizerpages/panelmodepage.h"
#include "panelizerpages/panelsourcespage.h"
#include "panelizerpages/panelsizepage.h"
#include "panelizerpages/panelcandidatepage.h"
#include "panelizerpages/panelseparationpage.h"
#include "panelizerpages/panelextrasspage.h"
#include "panelizerpages/panelarrangepage.h"
#include "panelizerpages/panelpreviewpage.h"

#include "panelizerengine.h"
#include "../fapplication.h"
#include "../debugdialog.h"
#include "../gerberpreview/gerberpreviewdialog.h"
#include "../mainwindow/mainwindow.h"
#include "../sketch/pcbsketchwidget.h"
#include "../items/itembase.h"
#include "../utils/graphicsutils.h"

#include <QMessageBox>
#include <QFileInfo>
#include <QProgressDialog>
#include <QApplication>

PanelizerWizard::PanelizerWizard(QWidget *parent, const QString &currentSketchPath)
	: QWizard(parent)
	, m_pageMode(nullptr)
	, m_pageSources(nullptr)
	, m_pagePanelSize(nullptr)
	, m_pageCandidate(nullptr)
	, m_pageSeparation(nullptr)
	, m_pageExtras(nullptr)
	, m_pageArrange(nullptr)
	, m_pagePreview(nullptr)
	, m_idMode(-1)
	, m_idSources(-1)
	, m_idPanelSize(-1)
	, m_idCandidate(-1)
	, m_idSeparation(-1)
	, m_idExtras(-1)
	, m_idArrange(-1)
	, m_idPreview(-1)
	, m_panelizeSuccess(false)
	, m_previewRendered(false)
	, m_currentSketchPath(currentSketchPath)
{
	setWindowTitle(tr("Panelize PCB"));
	// Use Classic style — matches the look of the rest of the
	// Fritzing dialogs and avoids the Aero/Modern banner that
	// looks out of place on Linux.
	setWizardStyle(QWizard::ClassicStyle);

	// The interactive arrange page needs real estate to show the whole
	// panel "front and centre at proper zoom", so open the wizard
	// maximised to the user's screen rather than at QWizard's cramped
	// default size. A generous minimum keeps it usable if un-maximised.
	setMinimumSize(900, 700);
	setWindowState(windowState() | Qt::WindowMaximized);

	m_pageMode       = new PanelModePage(this);
	m_pageSources    = new PanelSourcesPage(this);
	m_pagePanelSize  = new PanelSizePage(this);
	m_pageCandidate  = new PanelCandidatePage(this);
	m_pageSeparation = new PanelSeparationPage(this);
	m_pageExtras     = new PanelExtrasPage(this);
	m_pageArrange    = new PanelArrangePage(this);
	m_pagePreview    = new PanelPreviewPage(this);

	m_idMode       = addPage(m_pageMode);
	m_idSources    = addPage(m_pageSources);
	m_idPanelSize  = addPage(m_pagePanelSize);
	m_idCandidate  = addPage(m_pageCandidate);
	m_idSeparation = addPage(m_pageSeparation);
	m_idExtras     = addPage(m_pageExtras);
	m_idArrange    = addPage(m_pageArrange);
	m_idPreview    = addPage(m_pagePreview);

	// Set default output directory on the Sources page (this is where
	// the field 'panel.outputDir' is registered now; the preview page
	// only displays the rendered Gerbers).
	m_pageSources->setDefaultOutputDir(m_currentSketchPath);

	// Set sketch path for autosize functionality
	m_pagePanelSize->setSketchPath(m_currentSketchPath);
}

PanelizerWizard::~PanelizerWizard()
{
}

void PanelizerWizard::accept()
{
	// runPanelize() is invoked from initializePage() when the preview
	// page is entered, so by the time the user clicks Finish the
	// Gerbers are already on disk and visible in the preview widget.
	// accept() just confirms the bundle path and closes the wizard.
	if (m_previewRendered) {
		const QString outDir = m_lastOutputDir.isEmpty()
			? field("panel.outputDir").toString()
			: m_lastOutputDir;
		// Auto-pop the standalone preview dialog parented to our parent
		// (typically MainWindow) so it survives wizard destruction and
		// gives the user a real, resizable preview window to inspect
		// the artifacts in. Non-modal + WA_DeleteOnClose so they can
		// keep editing the sketch with the preview open alongside.
		QWidget * dlgParent = parentWidget() != nullptr ? parentWidget() : nullptr;
		auto * dlg = new GerberPreviewDialog(dlgParent);
		dlg->setAttribute(Qt::WA_DeleteOnClose);
		dlg->openDirectory(outDir, tr("Panel Preview — %1").arg(QFileInfo(outDir).fileName()));
		dlg->show();
		dlg->raise();

		QMessageBox::information(this, tr("Panelize"),
			tr("Panel artifacts written to:\n%1").arg(outDir));
		m_panelizeSuccess = true;
		QWizard::accept();
		return;
	}

	// Fallback path: user reached Finish without the preview page ever
	// being entered (shouldn't happen with the current page order, but
	// kept as a safety net). Run the pipeline directly.
	QStringList files;
	QString err;
	if (!runPanelize(files, err)) {
		QMessageBox::warning(this, tr("Panelize"), err);
		m_panelizeSuccess = false;
		QWizard::accept();
		return;
	}
	if (m_pagePreview != nullptr && !files.isEmpty()) {
		m_pagePreview->showGerbers(files);
	}
	QMessageBox::information(this, tr("Panelize"),
		tr("Panel artifacts written to:\n%1").arg(m_lastOutputDir));
	m_panelizeSuccess = true;
	QWizard::accept();
}

void PanelizerWizard::initializePage(int id)
{
	// The arrange page needs the probed board footprint before its own
	// initializePage() runs the seed auto-layout, so push it in first
	// (probe is cached, so this is cheap on Back/forward).
	if (id == m_idArrange && m_pageArrange != nullptr) {
		m_pageArrange->setBoardSizeInches(probedBoardSizeInches());
	}

	QWizard::initializePage(id);

	// Seed the candidate page with the probed board size + the current
	// auto-fit inputs before its own initializePage() pulls fresh
	// values from wizard fields. Probe is cached, so repeat entries
	// (Back/forward) don't re-open the .fzz.
	if (id == m_idCandidate && m_pageCandidate != nullptr) {
		m_pageCandidate->setBoardSizeInches(probedBoardSizeInches());
		// The setRequestedCopies/border/gutter/allowRotate values are
		// also re-read inside PanelCandidatePage::initializePage()
		// from the wizard fields, but we set them here too so they
		// reflect a defined state even if the page is queried before
		// QWizard delivers its own initializePage hook.
		m_pageCandidate->setRequestedCopies(field("panel.copies").toInt());
		const double mmToIn = 1.0 / 25.4;
		m_pageCandidate->setBorderInches(field("panel.border").toDouble() * mmToIn);
		m_pageCandidate->setGutterInches(field("panel.gutter").toDouble() * mmToIn);
		m_pageCandidate->setAllowRotate90(field("panel.allowRotate").toBool());
		return;
	}

	// Re-render and load the preview whenever the user enters the
	// preview page (including after Back+Next from earlier pages with
	// changed parameters).
	if (m_pagePreview == nullptr || page(id) != m_pagePreview) return;

	// Heavy-load gate: anything over 50 total copies prompts the user
	// before kicking off the render. This protects against accidental
	// fat-finger input (e.g. 1000 copies of a 50x30 mm board) that
	// would otherwise saturate disk and memory while the wizard sat
	// blocked on emitPanel().
	const int totalCopies = field("panel.copies").toInt();
	const int kHeavyLoadThreshold = 50;
	if (totalCopies > kHeavyLoadThreshold) {
		const auto answer = QMessageBox::question(this, tr("Heavy-load Panelize"),
			tr("You have requested %1 copies. Generating that many boards "
			   "can take several minutes and produce hundreds of megabytes "
			   "of Gerber files.\n\nProceed?").arg(totalCopies),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if (answer != QMessageBox::Yes) {
			m_previewRendered = false;
			// Bounce the user back to the sources page so they can lower
			// the copy count without having to hit Back manually.
			back();
			return;
		}
	}

	// Show an indeterminate progress dialog while the heavy lifting runs.
	// runPanelize() does board-probe -> layout -> emitPanel synchronously
	// on the UI thread; until that is moved to a worker (see
	// docs/design/multi-threaded.md) the best UX we can give is a modal
	// busy indicator with a meaningful caption that updates as we go.
	//
	// NOTE: the caption rolls over from "Generating..." to "Loading
	// preview..." after the writers finish, because loadFiles() below
	// re-parses every emitted Gerber synchronously on the UI thread
	// and that step is just as visible to the user as the write step.
	// Without the rollover, the dialog vanishes while the preview is
	// still building and the app looks frozen for several seconds.
	QProgressDialog progress(tr("Generating panel Gerbers..."),
		QString(), 0, 0, this);
	progress.setWindowTitle(tr("Panelize"));
	progress.setWindowModality(Qt::WindowModal);
	progress.setMinimumDuration(0);
	progress.setCancelButton(nullptr);
	progress.show();
	QApplication::processEvents();

	QStringList files;
	QString err;
	const bool ok = runPanelize(files, err);

	if (!ok) {
		progress.close();
		m_previewRendered = false;
		QMessageBox::warning(this, tr("Panelize"), err);
		return;
	}
	if (!files.isEmpty()) {
		// Roll the caption to the preview-load phase. loadFiles() does
		// parse + bounds-compute + initial render synchronously — keep
		// the modal up so the user sees one continuous "working..."
		// state rather than a flicker-then-frozen-window.
		progress.setLabelText(tr("Loading preview... (%1 layer file(s))").arg(files.size()));
		QApplication::processEvents();
		m_pagePreview->showGerbers(files);
		// One more spin so the freshly-laid-out preview paints before
		// we yank the modal away (otherwise the dialog can close on a
		// frame where the preview widget is still mid-relayout).
		QApplication::processEvents();
	}
	progress.close();
	m_previewRendered = true;
}

void PanelizerWizard::generateInteractivePreview()
{
	// Interactive "Generate" loop driver: run the same pipeline the
	// preview page runs on entry, but stay on the current (Arrange) page
	// and feed the result into a persistent, non-modal preview window the
	// user can leave open while they keep rearranging.

	// Heavy-load gate mirrors the preview-page path so a fat-fingered
	// copy count can't kick off a multi-minute render unprompted.
	const int totalCopies = field("panel.copies").toInt();
	const int kHeavyLoadThreshold = 50;
	if (totalCopies > kHeavyLoadThreshold) {
		const auto answer = QMessageBox::question(this, tr("Heavy-load Panelize"),
			tr("You have requested %1 copies. Generating that many boards "
			   "can take several minutes and produce hundreds of megabytes "
			   "of Gerber files.\n\nProceed?").arg(totalCopies),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if (answer != QMessageBox::Yes) return;
	}

	// Busy indicator while runPanelize() does its synchronous work.
	QProgressDialog progress(tr("Generating panel Gerbers..."),
		QString(), 0, 0, this);
	progress.setWindowTitle(tr("Panelize"));
	progress.setWindowModality(Qt::WindowModal);
	progress.setMinimumDuration(0);
	progress.setCancelButton(nullptr);
	progress.show();
	QApplication::processEvents();

	QStringList files;
	QString err;
	const bool ok = runPanelize(files, err);
	if (!ok) {
		progress.close();
		QMessageBox::warning(this, tr("Panelize"), err);
		return;
	}

	progress.setLabelText(tr("Loading preview..."));
	QApplication::processEvents();

	// Lazily create the reusable preview window. Parent it to the wizard
	// so it stacks above the wizard but is destroyed with it; null our
	// handle if the user closes it so the next Generate re-creates it.
	if (m_livePreview == nullptr) {
		m_livePreview = new GerberPreviewDialog(this);
		m_livePreview->setAttribute(Qt::WA_DeleteOnClose);
		connect(m_livePreview, &QObject::destroyed, this, [this]() {
			m_livePreview = nullptr;
		});
	}

	const QString outDir = m_lastOutputDir.isEmpty()
		? field("panel.outputDir").toString()
		: m_lastOutputDir;
	// openDirectory() re-reads every Gerber from disk, so each Generate
	// click refreshes the window with the freshly-overwritten artifacts.
	m_livePreview->openDirectory(outDir,
		tr("Panel Preview — %1").arg(QFileInfo(outDir).fileName()));
	m_livePreview->show();
	m_livePreview->raise();
	m_livePreview->activateWindow();

	progress.close();
	m_previewRendered = true;
}

bool PanelizerWizard::runPanelize(QStringList & outFiles, QString & outErr)
{
	DebugDialog::debug("[Panelize] runPanelize() entered");

	// Collect parameters from registered fields and build the
	// PanelizerEngine input structs.
	PanelizerEngine::PanelSpec spec;
	// PanelSizePage uses mm; engine wants inches. 1 in = 25.4 mm.
	const double mmToIn = 1.0 / 25.4;
	spec.panelSizeInches = QSizeF(
		field("panel.width").toDouble()  * mmToIn,
		field("panel.height").toDouble() * mmToIn);
	spec.gutterInches = field("panel.gutter").toDouble() * mmToIn;
	spec.borderInches = field("panel.border").toDouble() * mmToIn;
	spec.addRails     = field("panel.addRails").toBool();

	PanelizerEngine::SeparationSpec sep;
	if (field("panel.sep.none").toBool())       sep.kind = PanelizerEngine::Separation::None;
	else if (field("panel.sep.mb").toBool())    sep.kind = PanelizerEngine::Separation::MouseBites;
	else                                        sep.kind = PanelizerEngine::Separation::VCut;
	sep.vcutLineWidthMils = field("panel.sep.vcutWidth").toDouble();
	sep.vcutLayer         = field("panel.sep.vcutLayer").toString();
	sep.tabWidthInches    = field("panel.sep.tabWidth").toDouble() * mmToIn;
	sep.holesPerTab       = field("panel.sep.holesPerTab").toInt();
	sep.holeDiameterMils  = field("panel.sep.holeDiameter").toDouble() * mmToIn * 1000.0;
	sep.holePitchMils     = field("panel.sep.holePitch").toDouble()    * mmToIn * 1000.0;

	PanelizerEngine::ExtrasSpec extras;
	extras.addFiducials              = field("panel.extras.fiducials").toBool();
	extras.fiducialDiameterMils      = field("panel.extras.fiducialDiameter").toDouble();
	extras.fiducialClearMils         = field("panel.extras.fiducialClear").toDouble();
	extras.addToolingHoles           = field("panel.extras.toolingHoles").toBool();
	extras.toolingHoleDiameterInches = field("panel.extras.toolingDiameter").toDouble();

	QList<PanelizerEngine::SourceBoard> sources;
	PanelizerEngine::SourceBoard sb;
	sb.fzzPath       = m_currentSketchPath;
	sb.openWindow    = nullptr;
	sb.copies        = field("panel.copies").toInt();
	sb.allowRotate90 = field("panel.allowRotate").toBool();

	// Use the cached probed board size (computed once by
	// probedBoardSizeInches(); shared with PanelCandidatePage so we
	// don't re-open the .fzz twice in a single wizard run).
	sb.boardSizeInches = probedBoardSizeInches();
	DebugDialog::debug(QString("[Panelize] using board size: %1 x %2 in")
		.arg(sb.boardSizeInches.width()).arg(sb.boardSizeInches.height()));
	sources.append(sb);

	const QString outputDir = field("panel.outputDir").toString();
	DebugDialog::debug(QString("[Panelize] sources=%1 copies=%2 panel=%3x%4in border=%5 gutter=%6 outputDir=%7")
		.arg(sources.size()).arg(sb.copies)
		.arg(spec.panelSizeInches.width()).arg(spec.panelSizeInches.height())
		.arg(spec.borderInches).arg(spec.gutterInches)
		.arg(outputDir));

	// Auto-fit mode is now driven by PanelCandidatePage: when the user
	// ticks panel.autoFit, the wizard inserts the candidate page after
	// PanelSizePage and the user's pick there overwrites panel.width /
	// panel.height before runPanelize is invoked. So by the time we get
	// here, the field values already encode the chosen size and no
	// preset walk is needed. (See PanelizerWizard::nextId().)

	// Pre-validate: binary-search the highest copy count that fits.
	QString errorOut;
	QList<PanelizerEngine::PlacedBoard*> placed;
	if (!PanelizerEngine::layout(sources, spec, placed, &errorOut)) {
		int maxFits = 0;
		QList<PanelizerEngine::SourceBoard> probeSources = sources;
		for (int n = 1; n <= sb.copies; ++n) {
			probeSources[0].copies = n;
			QList<PanelizerEngine::PlacedBoard*> probePlaced;
			QString probeErr;
			const bool ok = PanelizerEngine::layout(probeSources, spec, probePlaced, &probeErr);
			qDeleteAll(probePlaced);
			if (!ok) break;
			maxFits = n;
		}
		const QString hint = (maxFits > 0)
			? tr("\n\nThe current panel fits at most %1 copies of this board.\n"
			     "Either reduce the copy count to %1, or enlarge the panel.").arg(maxFits)
			: tr("\n\nEven a single copy does not fit. Reduce the border, or use a larger panel.");
		DebugDialog::debug(QString("[Panelize] layout FAILED: %1 (maxFits=%2)").arg(errorOut).arg(maxFits));
		outErr = tr("Layout failed: %1").arg(errorOut.isEmpty() ? tr("unknown error") : errorOut) + hint;
		return false;
	}
	DebugDialog::debug(QString("[Panelize] layout OK: %1 boards placed").arg(placed.size()));

	// Honour the interactive arrange editor: when the user hand-placed
	// the boards, their positions/orientations override the auto-layout
	// verbatim. The arrange page seeds itself from the same layout() call
	// above, so the instance count matches one-to-one; if it somehow
	// doesn't (defensive), we fall back to the auto-layout untouched.
	if (m_pageArrange != nullptr) {
		const QList<PanelLayoutEditor::Placement> userPl = m_pageArrange->placements();
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
			DebugDialog::debug(QString("[Panelize] applied %1 user placements from arrange editor")
				.arg(userPl.size()));
		}
	}

	FApplication * app = qobject_cast<FApplication*>(qApp);
	DebugDialog::debug(QString("[Panelize] calling emitPanel into %1").arg(outputDir));
	PanelizerEngine::Result result = PanelizerEngine::emitPanel(
		placed, spec, sep, extras, outputDir, app);
	DebugDialog::debug(QString("[Panelize] emitPanel returned success=%1 gerberDir=%2 files=%3 warnings=%4")
		.arg(result.success).arg(result.gerberDir).arg(result.gerberFiles.size()).arg(result.warnings.size()));

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

void PanelizerWizard::executePanelize()
{
}

int PanelizerWizard::nextId() const
{
	const int cur = currentId();
	// Skip PanelCandidatePage unless the user opted into auto-fit on
	// PanelSizePage. Without this gate, every wizard run would force a
	// candidate-picker screen even when the user already typed an
	// exact W/H pair they care about.
	if (cur == m_idPanelSize) {
		return field("panel.autoFit").toBool() ? m_idCandidate : m_idSeparation;
	}
	return QWizard::nextId();
}

QSizeF PanelizerWizard::probedBoardSizeInches()
{
	if (!m_cachedBoardSizeInches.isEmpty()) return m_cachedBoardSizeInches;

	const double mmToIn = 1.0 / 25.4;
	QSizeF result(50.0 * mmToIn, 30.0 * mmToIn); // legacy fallback

	FApplication * fapp = qobject_cast<FApplication*>(qApp);
	QFileInfo fi(m_currentSketchPath);
	if (fapp != nullptr && fi.exists()) {
		// openWindowForService(false, 3) returns a hidden MainWindow
		// suitable for headless geometry probing. setCloseSilently()
		// suppresses the "save changes?" prompt on tear-down.
		MainWindow * probe = fapp->openWindowForService(false, 3);
		if (probe != nullptr) {
			probe->setCloseSilently(true);
			if (probe->loadWhich(m_currentSketchPath, false, false, false, QString())) {
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
	m_cachedBoardSizeInches = result;
	DebugDialog::debug(QString("[Panelize] probedBoardSizeInches() cached %1 x %2 in")
		.arg(result.width()).arg(result.height()));
	return m_cachedBoardSizeInches;
}

