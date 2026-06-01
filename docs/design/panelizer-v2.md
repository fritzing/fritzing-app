# Panelizer v2 — Design & Implementation Outline

> **Status:** design proposal, ready to hand to a junior dev.
> **Author:** landracer (forum post: https://forum.fritzing.org/t/v-cut-or-panelizer/5416)
> **Reviewer:** Copilot agent (per [AGENTS.md](../../AGENTS.md))
> **Target branch:** `develop`
> **Scope:** integrate a first-class, GUI-driven PCB panelizer into the
> Fritzing desktop app, replacing/augmenting today's headless
> `-p / -pc / -i` command-line workflow described in the wiki and in
> [src/autoroute/panelizer.cpp](../../src/autoroute/panelizer.cpp).

---

## 0. TL;DR for the Jr. Dev

You are not writing a panelizer from scratch. There is **already** a
working panelizer engine in this repo —
[`src/autoroute/panelizer.{h,cpp}`](../../src/autoroute/panelizer.cpp) —
but it is:

1. **Headless only**: invoked exclusively via CLI flags `-p`, `-pc`,
   `-i` parsed in [src/fapplication.cpp](../../src/fapplication.cpp#L496-L520).
2. **XML-driven**: requires a hand-written `panelizer.xml` listing every
   `.fzz` source, panel sizes, costs, etc.
3. **Multi-project oriented**: built around a job-shop model (many
   different boards onto one panel for cost amortization) rather than
   the much more common single-board NxM step-and-repeat that hobbyists
   actually want.
4. **No V-cut / mouse-bites / fiducials / rail support**.
5. **Undocumented and unmaintained** since ~2015 (PR
   [#3083](https://github.com/fritzing/fritzing-app/pull/3083)).

Your job is to **wrap, extend, and surface** this engine — *not* rip it
out. Read `panelizer.cpp` end-to-end before touching anything else.

---

## 1. Goals & Non-Goals

### 1.1 Goals (must have, v2.0)

| # | Goal |
|---|------|
| G1 | A `File → Export → for Production → Panelize…` menu entry that opens a wizard. |
| G2 | **Step-and-repeat** mode: take the *current* sketch and tile it NxM. |
| G3 | **Multi-project blend** mode: pick 2–4 `.fzz` files, auto-fit on a panel. |
| G4 | User-pickable panel size: presets (100×100, 100×150, 160×100, JLCPCB defaults, custom WxH in mm/in). |
| G5 | Configurable **gutter spacing**, **panel border (rail)**, and **board count**. |
| G6 | Choice of separation method: **V-cut** (straight scoring lines) **or** **mouse-bites** (perforated tab routing) **or** **none** (ship as gutter only). |
| G7 | Optional **fiducials** (3 corner, configurable diameter) and **tooling holes** (NPTH, configurable). |
| G8 | Live preview (QGraphicsScene) before commit, with drag-to-reposition for blend mode. |
| G9 | Output: a synthetic `.fzz` panel sketch **plus** a Gerber set produced by the existing `GerberGenerator`. |
| G10 | Backwards-compatible CLI: existing `-p / -pc / -i` flags keep working byte-for-byte. |

### 1.2 Non-goals (explicit, do not scope-creep)

- **Not** a DRC overhaul. We assume each input board already passes its own DRC.
- **Not** a rotation optimizer beyond 0°/90° (the existing tile router
  already handles 90°; arbitrary angle is out of scope).
- **Not** a cost-optimizer rewrite. The `c1`/`c2` cost model in
  [`PanelType`](../../src/autoroute/panelizer.h#L84-L90) stays, but the
  GUI does not expose it in v2.0 — keep it CLI-only.
- **Not** Qt 6, **not** C++17 features. Stay in C++14 ([phoenix.pro#L31](../../phoenix.pro#L31)).

---

## 2. What Already Exists (Inventory)

Read these files before writing a single line:

| File | What it gives you |
|---|---|
| [src/autoroute/panelizer.h](../../src/autoroute/panelizer.h) | Public API: `Panelizer::panelize()`, `inscribe()`, `checkDonuts()`, `checkText()`. Structs: `PanelItem`, `PanelType`, `PanelParams`, `PlanePair`, `LayerThing`. |
| [src/autoroute/panelizer.cpp](../../src/autoroute/panelizer.cpp) | ~1900 lines. Tile-based bin packing (`bestFitOne`, `placeBestFit`), per-board SVG generation (`makeSVGs`, `doOnePanelItem`), and the final Gerber emission. Uses the **corner-stitching tile data structure** from `cmrouter/` — same one the autorouter uses. |
| [src/svg/gerbergenerator.h](../../src/svg/gerbergenerator.h) | `GerberGenerator::exportToGerber(prefix, dir, board, sketchWidget, displayMessageBoxes)` — the single-board entry point you'll call once per output panel. |
| [src/mainwindow/mainwindow_export.cpp](../../src/mainwindow/mainwindow_export.cpp#L1644) | `MainWindow::exportToGerber()` — copy this pattern for the new panel export. |
| [src/mainwindow/mainwindow_menu.cpp](../../src/mainwindow/mainwindow_menu.cpp#L1398-L1402) | Where the `for Production` submenu is built — your new `m_exportPanelAct` goes here. |
| [src/items/resizableboard.h](../../src/items/resizableboard.h) | `Board` and `ResizableBoard` classes — needed to instantiate the synthetic panel outline. |
| [src/fapplication.cpp](../../src/fapplication.cpp#L496-L520) | CLI parsing for `-p / -pc / -i`. **Do not break.** |
| [src/commands.h](../../src/commands.h) | Add a `PanelizeCommand` here so the new panel sketch is undoable. |

### 2.1 Reference (do not commit, just learn from)

- `/home/sysadmin/Documents/git/kicadPanelizer/pcbPanelize` — the
  KiCad-side reference user mentioned. Read `brdPanel()`. It does:
  rotate → translate to origin → for each (x,y) duplicate items into
  the pcb, skipping the original Edge.Cuts → redraw the outer frame
  with mil-spaced lines. **We are doing the equivalent at the SVG
  layer**, not at a board-object layer (because Fritzing has no
  KiCad-style mutable PCB object — it has SVG layers per `ItemBase`).

---

## 3. Architecture: Where the New Code Lives

```
src/autoroute/
    panelizer.{h,cpp}             ← existing; leave the public API alone
    panelizerengine.{h,cpp}       ← NEW: extracted reusable core (see §3.1)
    panelizerwizard.{h,cpp}       ← NEW: QWizard subclass (the GUI)
    panelizerpages/
        panelmodepage.{h,cpp}     ← NEW: step-and-repeat vs blend
        panelsourcespage.{h,cpp}  ← NEW: file picker(s)
        panelsizepage.{h,cpp}     ← NEW: panel dims + spacing + border
        panelseparationpage.{h,cpp} ← NEW: V-cut / mouse-bites / none
        panelextrasspage.{h,cpp}  ← NEW: fiducials + tooling holes
        panelpreviewpage.{h,cpp}  ← NEW: QGraphicsView preview
        panelexportpage.{h,cpp}   ← NEW: output paths + format toggles
    panelizerseparators.{h,cpp}   ← NEW: V-cut + mouse-bite SVG generators
src/items/
    panelboarditem.{h,cpp}        ← NEW: subclass of Board for the panel outline
src/commands.h / commands.cpp     ← add PanelizeCommand
pri/autoroute.pri                 ← add the new files (alphabetical)
tests/auto/test_panelizer/        ← NEW: QtTest for the engine (see §9)
```

### 3.1 Why an "engine" extraction

The current `Panelizer::panelize()` is a 250-line monolith that does
file IO, XML parsing, layout, SVG emission, and Gerber writing. The GUI
needs to call **only the layout + emission steps** with parameters that
come from the wizard, not from XML. So:

```cpp
// src/autoroute/panelizerengine.h
namespace PanelizerEngine {

	struct SourceBoard {
		QString fzzPath;          // absolute path; "" if from open MainWindow
		MainWindow * openWindow;  // non-null for the current sketch
		int copies = 1;           // step-and-repeat multiplier
		bool allowRotate90 = true;
	};

	struct PanelSpec {
		QSizeF panelSizeInches;   // outer dimensions
		double gutterInches = 0.08;       // board-to-board spacing
		double borderInches = 0.2;        // panel rail width
		bool addRails = true;             // top+bottom rails for V-cut
	};

	enum class Separation { None, VCut, MouseBites };

	struct SeparationSpec {
		Separation kind = Separation::VCut;
		// V-cut:
		double vcutLineWidthMils = 10.0;
		QString vcutLayer = "Edge_Cuts"; // emit on outline gerber
		// Mouse-bites:
		double tabWidthInches  = 0.118;  // 3 mm
		int    holesPerTab     = 5;
		double holeDiameterMils = 20.0;
		double holePitchMils    = 31.5;
	};

	struct ExtrasSpec {
		bool addFiducials = true;
		double fiducialDiameterMils = 40.0;
		double fiducialClearMils    = 80.0;
		bool addToolingHoles = false;
		double toolingHoleDiameterInches = 0.125;
	};

	struct Result {
		QString panelFzzPath;     // synthetic panel sketch
		QString gerberDir;        // where the gerbers landed
		QStringList warnings;
		bool success = false;
	};

	// Pure layout — no IO. Fills positions in `outItems`.
	bool layout(const QList<SourceBoard>& sources,
	            const PanelSpec&        spec,
	            QList<PanelItem*>&      outItems,    // owned by caller
	            QString*                errorOut);

	// IO + Gerber. Calls back into the existing Panelizer for SVG work.
	Result emit(const QList<PanelItem*>& laidOut,
	            const PanelSpec&         spec,
	            const SeparationSpec&    sep,
	            const ExtrasSpec&        extras,
	            const QString&           outputDir,
	            FApplication*            app);

} // namespace PanelizerEngine
```

The existing `Panelizer::panelize()` is then refactored to **build
these structs from XML and call** `PanelizerEngine::layout` +
`emit` — same behavior, same outputs, no behavior change for CLI users.
**That refactor must be its own PR**, before any GUI work lands.

---

## 4. UI Flow (QWizard)

Use `QWizard` (Qt 5 native, already a transitive dep — no new libs).
6 pages, all backed by `.ui` files under `src/autoroute/panelizerpages/`.

### Page 1 — Mode

- Radio: **Step-and-repeat current sketch** (default)
- Radio: **Blend multiple projects** (enables Page 2 file list)
- Radio: **Use legacy panelizer.xml** (open file dialog, hand off to
  existing `Panelizer::panelize()` unchanged — no UI for this path)

### Page 2 — Sources

- Step-and-repeat: shows current sketch's board as a thumbnail; spinner
  for `copies` (1–100), checkbox `allow 90° rotation`.
- Blend: `QListWidget` with **Add .fzz**, **Remove**, **Up/Down**.
  Per-row spinner for `copies`, per-row `allowRotate90` checkbox. Cap
  at 4 distinct projects (per user's spec; enforce with
  `wizard()->button(QWizard::NextButton)->setEnabled(...)`).

### Page 3 — Panel Size

- Preset combo: `100×100`, `100×150`, `150×100`, `160×100`, `200×150`,
  `JLCPCB max (400×500)`, `Custom…`.
- Custom: two `QDoubleSpinBox` (width, height) + unit toggle (mm/in).
  Validate against `min(board) + 2×border` to refuse impossible sizes.
- Gutter spacing (mm): default 2.0.
- Border / rail width (mm): default 5.0.
- Checkbox: "Add top+bottom rails (recommended for V-cut)".
- **Live "fits N boards" label** — calls `PanelizerEngine::layout()`
  in a non-modal QFutureWatcher and updates as you type.

### Page 4 — Separation

- Radio: **None / V-cut / Mouse-bites**.
- V-cut sub-fields: line width (mils), edge layer name (default
  `Edge_Cuts`).
- Mouse-bites sub-fields: tab width, holes per tab, hole Ø, pitch.
  Show a small SVG preview of one tab.
- **NOTE**: V-cut lines must extend through the rails edge-to-edge
  (that is what the fab actually scores against). Mouse-bites are local
  to the gutter only.

### Page 5 — Extras

- Fiducials checkbox + diameter/clear spinners. Three corners (TL, TR,
  BR) is the de-facto standard; expose as a 4-checkbox grid.
- Tooling holes checkbox + diameter spinner; place at all 4 rail
  corners with a 5 mm offset.

### Page 6 — Preview & Export

- Embedded `QGraphicsView` showing the laid-out panel SVG (use
  `FSvgRenderer`).
- "Output folder" file picker.
- Checkbox: "Also save panel as .fzz" (default on).
- **Finish** button:
  1. Push a `PanelizeCommand` onto `WaitPushUndoStack` (so opening the
     synthesized sketch is undoable).
  2. Call `PanelizerEngine::emit(...)`.
  3. Show a `FileProgressDialog` (already in the codebase, see
     `MainWindow::exportProgress()`).
  4. On success, offer to open the `.fzz` panel in a new window.

---

## 5. Separation Geometry (the actual hard part)

This is where most KiCad-style panelizers get it wrong. Read this
section twice.

### 5.1 V-cut

For each interior gutter line (vertical *and* horizontal):

1. Compute the gutter centerline in inches → emit a single straight
   `<line>` in SVG into a **new** layer `panel_vcut`.
2. The line must extend `border + epsilon` past the panel outline on
   each end so the fab's V-scoring saw runs off the edge.
3. The `panel_vcut` layer maps to Gerber file suffix `.gko` (board
   outline) — most fabs accept multiple polylines on `.gko` and infer
   "scoring" from straight lines that cross the whole panel.
4. Also emit a separate `<rect>` for the panel outer outline.

Implementation: a free function in `panelizerseparators.cpp`:

```cpp
QString PanelizerSeparators::vcutSvg(const QList<PanelItem*>& items,
                                     const PanelizerEngine::PanelSpec& spec,
                                     const PanelizerEngine::SeparationSpec& sep);
```

### 5.2 Mouse-bites

Per gutter segment between two adjacent boards:

1. Compute the midline of the gutter.
2. Lay out `holesPerTab` non-plated through-holes centered on the
   midline, spaced by `holePitchMils`.
3. For the routing slot: emit two parallel arcs/lines on the outline
   layer that *do not* cross through the holes — this leaves the tab
   intact, holes weaken it for snap-off.
4. Holes go on `.txt` (drill) Gerber, slots go on `.gko`.

The drill output already routes through `GerberGenerator`'s
`doDrill()` family in [src/svg/gerbergenerator.cpp](../../src/svg/gerbergenerator.cpp).
You append synthetic `<circle>` elements to the drill layer SVG before
handing it off — see how `Panelizer::doOnePanelItem()` currently
composes per-layer SVGs.

### 5.3 Fiducials

Concentric: copper pad Ø `fiducialDiameterMils`, mask opening Ø
`fiducialClearMils`. Emit on top copper + top soldermask layers.
**Do not** put them inside the gutter — only on the rails.

---

## 6. CLI Backwards-Compatibility

Required new CLI surface (the GUI flow must also be invocable headless
for CI):

```
fritzing --panelize-simple <input.fzz> \
         --panel-size 100x100mm \
         --copies 4 \
         --gutter 2mm --border 5mm \
         --separation vcut \
         --fiducials \
         -o ./out/
```

Add the parser branch in [src/fapplication.cpp](../../src/fapplication.cpp#L484)
**after** the existing `-p / -pc / -i` blocks. Use a new
`m_serviceType = SimplePanelService;` enum value defined in
[src/fapplication.h](../../src/fapplication.h#L193). Dispatch from
`runService()` to a new `FApplication::runSimplePanelService()` that
constructs the `PanelizerEngine` structs and calls `emit()`.

**Do not modify the existing `PanelizerService` branch.**

---

## 7. Data Model: The Synthetic Panel `.fzz`

A panelized output is itself a Fritzing sketch, so the user can re-open
it, eyeball it, and re-export. Composition:

1. One `PanelBoardItem` (new subclass of `Board` from
   [resizableboard.h](../../src/items/resizableboard.h)) sized to the
   panel outline. Its custom SVG is the V-cut/mouse-bite outline.
2. For each `PanelItem` in the layout: an instance of the original
   board's `ModelPart` placed at `(x, y)` with `rotate90` applied.
   Use `PCBSketchWidget::loadFromModel()` patterns — see how Eagle
   import does it in [src/eagle/](../../src/eagle/).
3. Save via the normal `MainWindow::saveAs()` path so the `.fzz` is a
   valid archive (handled by QuaZip — see [pri/quazip.pri](../../pri/quazip.pri)).

This means the panelizer **must run on the main thread** (because
`MainWindow` and `QGraphicsScene` are not thread-safe). Long phases
(layout, Gerber writing) yield via
[`ProcessEventBlocker`](../../src/processeventblocker.h) like the
existing autorouter does.

---

## 8. Step-by-Step Implementation Order

Do these in order. Each step is its own PR against `develop`. Do not
combine.

| # | PR title | Touches | Verifiable by |
|---|---|---|---|
| 1 | `refactor(panelizer): extract PanelizerEngine namespace` | new `panelizerengine.{h,cpp}`; `panelizer.cpp` calls into it | All existing CLI tests still pass; gerber bytes identical (golden file diff). |
| 2 | `feat(panelizer): synthetic PanelBoardItem` | new `items/panelboarditem.{h,cpp}` + `pri/items.pri` | Open Fritzing, programmatically instantiate one in a scratch sketch. |
| 3 | `feat(panelizer): V-cut + mouse-bite SVG generators` | new `panelizerseparators.{h,cpp}` + `tests/auto/test_panelizer/` | QtTest snapshot tests against committed reference SVGs. |
| 4 | `feat(panelizer): wizard scaffolding + Mode/Sources pages` | wizard + 2 pages, menu wiring | Manual: menu opens wizard, can pick mode + sources. |
| 5 | `feat(panelizer): Size/Separation/Extras pages` | 3 more pages + live "fits N" | Manual: spinners update preview count. |
| 6 | `feat(panelizer): preview page + export pipeline` | preview + emit + Gerber | Manual: produce a Gerber, open in [gerbv](http://gerbv.geda-project.org/) and visually verify. |
| 7 | `feat(panelizer): simple-panelize CLI` | `fapplication.cpp` | New CI smoke test in [tests/auto](../../tests/auto/) generates panel headless. |
| 8 | `docs(panelizer): user-facing help page` | `help/panelizer_help.html` + wiki update | Reviewer reads it cold and can panelize a board. |

---

## 9. Testing Strategy

The repo currently only has [tests/auto/test_svg](../../tests/auto/test_svg/)
and [tests/auto/test_textutils](../../tests/auto/test_textutils/) — both
QtTest. Add `tests/auto/test_panelizer/`:

```
test_panelizer/
    test_panelizer.pro
    test_panelizer.cpp
    fixtures/
        single_board.fzz       (committed; small; <50 KB)
        expected_2x2_vcut.gbr  (golden Gerber)
        expected_2x2_vcut.gko
```

Cases (minimum):

1. **Layout determinism**: 4 copies of `single_board.fzz` on
   100×100 mm with 2 mm gutter → layout returns 4 items at expected
   coordinates (assert exact `QPointF` values).
2. **Layout overflow**: 50 copies on 50×50 mm → returns `false` with
   `errorOut` containing "does not fit".
3. **V-cut SVG**: assert generated SVG contains exactly N+1 vertical
   and M+1 horizontal lines extending past panel bounds.
4. **Mouse-bite count**: NxM panel produces
   `(N-1)*M + (M-1)*N` tab clusters with `holesPerTab` holes each.
5. **CLI smoke**: run the binary with `--panelize-simple` against the
   fixture, diff Gerbers byte-for-byte against golden files.

**Do not** unit-test the Wizard pages — UI is tested manually.

---

## 10. Annotation Standard (per [AGENTS.md §5.5](../../AGENTS.md))

Every new public method gets a Doxygen header. Example for the
junior dev to copy:

```cpp
/**
 * @brief Lays out source boards into a panel using corner-stitching bin packing.
 *
 * Wraps the legacy Panelizer::bestFit pipeline with a parameter struct
 * driven by the GUI wizard rather than panelizer.xml. Pure layout: no
 * file IO, no Gerber emission. Caller owns the PanelItem* objects in
 * @p outItems and must `qDeleteAll()` them.
 *
 * @param sources    Boards to place. Each may request multiple copies
 *                   via SourceBoard::copies. Order matters only for
 *                   tie-breaking — the packer is greedy by area.
 * @param spec       Panel dimensions, gutter, and border. Must satisfy
 *                   spec.panelSizeInches > (largestBoard + 2*border).
 * @param outItems   [out] Filled with placed PanelItem instances.
 *                   Cleared on entry. Empty on failure.
 * @param errorOut   [out, optional] Human-readable failure reason.
 * @return true on success, false if any required board does not fit.
 *
 * @note Must be called on the main (GUI) thread because it instantiates
 *       MainWindow objects to read board geometry from the .fzz files.
 *       Use a QFutureWatcher with QtConcurrent::run + signal hop if you
 *       need it off the UI thread for live preview.
 *
 * @see PanelizerEngine::emit  for the IO half of the pipeline.
 * @see Panelizer::bestFitOne  src/autoroute/panelizer.cpp:495 — the
 *      underlying packer this delegates to.
 */
bool layout(const QList<SourceBoard>& sources,
            const PanelSpec&          spec,
            QList<PanelItem*>&        outItems,
            QString*                  errorOut);
```

Inline comments are required at:

- The boundary where mm/in conversion happens (use
  [`TextUtils::convertToInches`](../../src/utils/textutils.h)) —
  the legacy code mixes units silently; this has burned contributors.
- Every place V-cut lines are extended past the panel outline (explain
  *why* — fabs need overrun for the saw blade).
- Every interaction with the corner-stitching tile data structure.
  The data structure is non-obvious; reference the Ousterhout paper in
  the comment: `// Corner-stitching: see Ousterhout 1984.`

---

## 11. Risks & Open Questions

| Risk | Mitigation |
|---|---|
| Existing CLI users have `panelizer.xml` files in production. | Step 1 PR's golden-file Gerber diff guards against any byte-level regression. |
| `.fzz` files reference the `parts/` repo at runtime — blend mode may pull boards that need parts the user doesn't have. | At source-pick time, eagerly load each `.fzz` and warn (don't fail) on missing modules. Reuse `MainWindow::loadWhich()` error path. |
| Mouse-bite drill holes can collide with board's own copper near the edge. | DRC pass after layout: for each hole, query the placed board's copper SVG and warn if overlap > 0. Optional auto-shift to nearest gutter midline. |
| Synthesized `.fzz` may not survive round-trip through MainWindow's normal save (the panel is not a "real" sketch). | Mark `PanelBoardItem` with a `isPanel="true"` attribute in its `ModelPart` properties; `MainWindow` skips parts-bin operations on panels. |
| User picks panel size smaller than the largest source board. | Page 3 validator hard-blocks Next; show inline error label. |
| Qt 5.9 (minimum supported) lacks `QWizard::setSubTitleFormat(Qt::RichText)` — works but limited. Confirmed acceptable. | Avoid HTML in subtitles; use plain text + `QLabel` widgets in the page body for rich content. |
| Threading: long Gerber emission blocks UI. | `ProcessEventBlocker::processEvents()` inside the per-board emit loop, same as autorouter. |

### Open questions for landracer to answer **before** Step 4 starts

1. **Maximum supported board count per panel** — engine currently has
   no hard cap; suggest 64 for v2.0 with a config override.
2. **Default unit** in the wizard — mm or in? KiCad/JLCPCB users
   expect mm; older Fritzing exports default to in.
3. **Where to store wizard preferences** — `QSettings` under
   `org.fritzing.Fritzing/Panelizer/...`? (Recommended; matches
   existing pattern in `MainWindow::loadSettings()`.)
4. **Fab profiles** — should we ship a `panelizer_fabs.json` with
   JLCPCB / OSH Park / PCBWay defaults? Out of scope for v2.0 but
   design `PanelSpec` so it can be JSON-serialized cleanly.

---

## 12. References

- Existing engine: [src/autoroute/panelizer.cpp](../../src/autoroute/panelizer.cpp)
- Original PR (May 2015, never finished): https://github.com/fritzing/fritzing-app/pull/3083
- Wiki CLI docs: https://github.com/fritzing/fritzing-app/wiki/3.-Command-Line-Options
- Forum post (this proposal's origin): https://forum.fritzing.org/t/v-cut-or-panelizer/5416
- KiCad reference panelizer: `/home/sysadmin/Documents/git/kicadPanelizer/pcbPanelize`
- Corner-stitching paper: Ousterhout, J.K. *Corner Stitching: A Data-Structuring
  Technique for VLSI Layout Tools.* IEEE Trans. on CAD, 1984. (The
  basis for `src/autoroute/cmrouter/tile.h`.)
- Qt `QWizard` docs: https://doc.qt.io/qt-5/qwizard.html

---

## 13. Definition of Done

A PR finishes the panelizer feature only when **all** of these are true:

- [ ] All 8 PRs above are merged into `develop`.
- [ ] `tests/auto/test_panelizer` passes in CI (Docker bionic build).
- [ ] Manual: a brand-new user can open Fritzing, load a sample
      sketch, run `Export → for Production → Panelize…`, accept all
      defaults, and produce a valid Gerber set that opens cleanly in
      `gerbv` with no warnings.
- [ ] Manual: a fab (JLCPCB DFM check) accepts the produced Gerbers.
- [ ] `help/panelizer_help.html` exists, is linked from the wizard's
      `?` button, and a junior dev (you) can follow it without help.
- [ ] All code carries the GPL header from [AGENTS.md §5.2](../../AGENTS.md).
- [ ] Code review by Copilot agent against [AGENTS.md](../../AGENTS.md) annotation rules
      passes with **zero** "missing comment" findings.

End of design. Ping me on the PR for Step 1 before starting Step 2.
