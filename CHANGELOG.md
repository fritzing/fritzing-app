# Changelog

> Conventions: keep entries newest-first. Group by release. Use the
> headings **Added / Changed / Fixed / Removed / Internal**. PR
> references go inline as `(#NNNN)`. When in doubt, write the entry
> from the *end-user's* perspective, not the implementer's.

## Unreleased

### Added

- **Redesigned Panelizer as a single interactive window.** The old
  multi-page wizard is replaced by a two-step flow: a small start
  dialog asks only *quantity* and whether to *blend in other boards*,
  then one interactive window holds everything else at once — panel
  size, board separation, extras, a drag-to-arrange canvas with
  alignment guides, and an embedded Gerber preview. **Generate**
  overwrites the output and refreshes the preview in place, so you can
  tweak → generate → look → tweak again without re-opening anything.
  See `docs/panelizer.md`.
- **Standard panel-size dropdown.** The interactive window lists common
  fab-shop panel sizes (50×50 up to 300×400 mm, including the Eurocard
  formats) for one-click selection; the width/height fields stay fully
  editable for custom sizes and flip the selector to *Custom* when
  edited.
- **Loading feedback.** A *Loading boards…* progress dialog now shows
  while the panelizer measures each source board, and a
  *Generating panel Gerbers…* dialog shows during export, so the tool
  no longer appears frozen during the slow steps.
- **Qt 6 build support.** Fritzing now compiles and runs against
  Qt 6 (6.5+) in addition to Qt 5.15, from a single source tree.
  The Qt version is selected by whichever `qmake` you build with;
  no source changes are required to switch. Qt 5 remains the
  default for release packaging for now. See `INSTALL.txt` and a
  reference Qt 6 container build in `docker/Dockerfile.qt6`.
- **Panelizer wizard end-to-end pipeline.** File → Export → for
  Production → Panelize… now produces real RS-274X Gerbers and an
  Excellon drill file in the chosen output folder, complete with a
  composite Edge_Cuts outline, frame, and (where applicable) V-cut
  or mouse-bite separations between boards. Replaces the previous
  placeholder pipeline that wrote empty files.
- **Standalone Gerber Preview window** (`GerberPreviewDialog`).
  Opens automatically after any Gerber export (single-board or
  panelized) so the user can verify the artifacts in-app before
  shipping to a fab house. Non-modal, remembers geometry and layer
  visibility between sessions via `QSettings`.
- **Auto-fit panel mode.** New checkbox on the Panel Size page:
  when on, the wizard ignores the user-entered width/height and
  walks a vendor preset list smallest-first to find the smallest
  panel that fits the requested copies. Lets new users panelize
  without having to know fab-house tier sizes up front.
- **Vendor panel presets.** The Panel Size combo now lists 19
  preset sizes covering JLCPCB tiers (100×100, 100×150, 100×200,
  150×150, 200×200, 400×500 max), PCBWay (540×510 max), OSH Park
  super-swift (10×10 in), generic intermediates, and imperial
  sizes 5×5 / 5×10 / 10×10 / 10×15 / 15×20 in. Inch presets
  auto-convert to mm.
- **Heavy-load confirmation.** Requesting more than 50 copies pops
  a "this will take a while" confirmation before the render kicks
  off. Cancelling bounces back to the Sources page so the user can
  reduce the count without manual navigation.
- **Progress dialog during panel render.** Indeterminate
  `QProgressDialog` covers the otherwise opaque pause between
  clicking Next on the Extras page and seeing the preview. Will be
  upgraded to determinate once the render pipeline moves off the
  GUI thread (see `docs/design/multi-threaded.md`).

### Changed

- **Panelizer wizard layout.** Output folder picker moved from the
  Preview page to the Sources page so the destination is chosen
  *before* the long-running render kicks off, not after. Default
  remains `<sketchDir>/panel`.
- **Panelizer copy count cap raised** from 100 to 1000. The
  heavy-load confirmation above is the actual UX gate.
- **Panelizer board sizing** now probes the real board outline from
  the source `.fzz` instead of using a hardcoded 50×30 mm placeholder.
  Falls back to the old constant only if the probe fails.
- **Panel layout error messages** now report the maximum copy count
  that *would* have fit, so the user knows what to dial back to.
- **Preview pane height** in the wizard raised from 240 to 360 px.

### Fixed

- **Panelizer crash while arranging boards on large panels.** The
  arrange canvas could recurse without bound when grid + alignment-guide
  snapping disagreed by a sub-pixel amount on non-integer board sizes
  (reproduced on a 300×300 mm panel). A re-entrancy guard stops the
  snap from re-triggering itself.
- **Panelizer no longer pops up a stray second Fritzing window.** The
  hidden window used to measure board geometry is now forced fully
  off-screen, so it never flashes into view while panelizing.
- **Panelizer Gerber preview text is readable.** The preview is now
  embedded directly in the panelizer window, bypassing the dialog
  chrome whose stylesheet rendered some labels white-on-white.
- **Panelizer no longer reports "layout failed" mysteriously** when
  the requested copy count cannot fit. Wizard now runs a
  binary-search pre-validation and surfaces the maximum fittable
  count in the error message.
- **Wizard preview now actually shows the panel.** Previously the
  preview load was triggered from `accept()`, racing the wizard's
  close handler so the user never saw it. Render is now driven by
  `initializePage(previewPage)` so the user gets to inspect before
  hitting Finish.

### Internal

- New `src/gerberpreview/gerberpreviewdialog.{h,cpp}`. Wraps the
  existing `GerberPreviewWidget` in a top-level `QDialog` with
  toolbar, layer toggles, and persisted state.
- `Panelizer::makeSVGs()` promoted to public so `FabExporter` can
  reuse it for the real-Gerber render path.
- New `docs/design/multi-threaded.md` — phased plan for moving the
  Gerber export, parts library load, autorouter, and panelizer
  layout search off the GUI thread.
- New `docs/design/gerbview-port-reference.md` — file-by-file map
  of KiCad's gerbview tree against Fritzing equivalents, with a
  phased port plan for matching gerbview's feature surface.
- New `docs/internal/panelizer-progress.md` — running session log
  of what has landed in the panelizer / preview effort and what is
  still pending.
- **Qt 6 source port (dual-build via `QT_VERSION` guards).** All
  removed/changed Qt APIs replaced with version-agnostic
  equivalents where one exists in both Qt 5.15 and Qt 6, and
  `#if QT_VERSION` guards only where unavoidable:
  - `QMatrix` → `QTransform` (panelizer, cmrouter).
  - `qrand()`/`RAND_MAX` → `QRandomGenerator::global()`.
  - `QPrinter::setPaperSize()`/`paperRect()` → `QPageSize` /
    `paperRect(QPrinter::DevicePixel)`.
  - `QString::arg()` on `QFlags`/enums now passes an explicit
    `static_cast<int>` (commands, connectoritem, itembase,
    sketchwidget, textutils, paletteitembase).
  - `QStandardPaths::DataLocation` → `AppLocalDataLocation`.
  - `QDomDocument::setContent()` `ParseResult` handled via `auto`.
  - `QTextCodec`/`setCodec()` paths guarded; UTF-8 handling falls
    back to `QString::fromUtf8` under Qt 6 (`core5compat`).
- **Qt 6 wiring in `phoenix.pro`.** Adds `core5compat` and
  `svgwidgets` modules when building with Qt 6.
- New `docker/Dockerfile.qt6` and `docker/build-linux-qt6.sh` —
  self-contained reference Qt 6 build (builds sibling libgit2 and
  Qt 6 QuaZip from source, clones the parts repo).
