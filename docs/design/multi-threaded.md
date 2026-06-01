# Multithreading Fritzing — Scope, Pitfalls, Phased Plan

> Status: design analysis, not yet implemented. Written 2026‑05‑30 in
> response to "What would it take to make the app multi‑threaded so it
> uses all CPUs / load‑balancing?"

This document is a candid assessment, not a sales pitch. Some of what
Fritzing does is genuinely parallelizable (Gerber export, autorouting,
panelizer rendering); other parts (the editing surface, the parts
library, the undo stack) are tied to the Qt main thread by design and
cannot be moved without invasive surgery.

---

## 1. The blunt summary

Fritzing today is effectively a single‑threaded Qt 5 application:

- Every `QGraphicsItem` (every part, wire, connector, board outline,
  legend element) lives on the **GUI thread**. The `QGraphicsScene` /
  `QGraphicsView` family in Qt is **not thread‑safe** and Qt explicitly
  forbids touching scene items from another thread.
- The Undo stack (`WaitPushUndoStack`) is a `QUndoStack`, also GUI‑thread
  only.
- The reference model (`ReferenceModel`) is loaded once at startup and
  then read from a single thread; some load paths already use a worker
  thread, but consumers assume the loaded model is steady‑state.
- The SVG normalizer and `FSvgRenderer` cache live in module‑local
  static state without locking.
- `DebugDialog::debug()` writes to a `QFile` from the GUI thread; no
  guards.

The result: there is no *single switch* you can flip. "Multi‑threaded
Fritzing" means picking the workloads that can move off the GUI thread,
giving each one a worker, and adding the smallest possible synchroniza-
tion seam — *one piece at a time, with verification at each step*.

---

## 2. Where threading actually pays off

These are the workloads that genuinely benefit from parallelism, ranked
by ROI:

### 2.1 Gerber / PDF / SVG export pipeline   ⭐⭐⭐⭐⭐

Today: `GerberGenerator::exportToGerber()` and `FabExporter::export-
FromPanel()` walk N layers and serialize each one synchronously. With
~9 layers per board × M boards on a panel, this is the most obviously
embarrassingly‑parallel workload in the codebase.

Plan:

- Move each per‑layer render (`GerberGenerator::doEnd()` + writes) into
  a `QtConcurrent::map()` over the layer list.
- Output SVGs already live as `QString` blobs in `Panelizer::makeSVGs()`
  — they are detached from `QGraphicsScene` once captured, so they are
  safe to hand to a worker.
- Single write barrier at the end (collect file paths, hand to the
  preview widget on the GUI thread).
- Expected wall‑clock improvement: ~4× on a 4‑core box for a typical
  panel.

Cost: low. ~150 lines of refactor in `gerbergenerator.cpp` and
`fabexporter.cpp`. No GUI surgery required.

### 2.2 Panelizer board probe + layout   ⭐⭐⭐⭐

Today: `runPanelize()` opens each unique `.fzz` in a hidden
`MainWindow` to probe board size. With N unique boards in a blend
panel, that's N sequential window loads on the GUI thread, blocking
the wizard.

Plan:

- Probe pass cannot move off the GUI thread (it instantiates `MainWindow`
  and `QGraphicsScene`), but the *layout search* (`PanelizerEngine::
  layout()`'s binary‑search fit loop) is pure geometry and **can**
  run in a worker.
- Use a `QFutureWatcher<bool>` to drive the wizard's progress dialog
  determinate instead of indeterminate.

Cost: medium. The layout engine has to be audited for shared mutable
state (currently uses static-scope tile lists from the legacy router —
those are on the cleanup list anyway).

### 2.3 Autorouter   ⭐⭐⭐⭐

The current autorouter (`src/autoroute/`) is a single‑threaded greedy
A\* over a tile grid. Both the JumpingHotWires router and the
cm‑router lend themselves to parallelization at the *net* level:
multiple nets can be routed concurrently as long as they don't share
tiles in their search frontier.

Plan:

- Partition the netlist into independent‑frontier batches.
- Route each batch on a `QThreadPool` worker.
- Merge passes happen on the GUI thread.

Cost: **high**. This is real work — the routers have substantial
shared state (occupancy grid, DRC graph). Worth doing but only after
the export pipeline lands so the team has experience with the project's
threading idioms first.

### 2.4 Parts library load   ⭐⭐⭐

Today: `ReferenceModel::loadAll()` parses ~thousands of `.fzp` XML
files and their SVG icons on startup. Already partially threaded
(SVG icon load runs on a worker), but XML parse is serial.

Plan:

- Parse the `.fzp` files in parallel batches via `QtConcurrent::map`.
- Build a per‑thread intermediate map, merge under one mutex at the
  end. The `ReferenceModel` itself stays single‑writer.

Cost: medium. Watch out for the `FSvgRenderer` cache (currently
unlocked static state).

### 2.5 SVG rendering for export   ⭐⭐⭐

`FSvgRenderer::loadSvg()` and `render()` are CPU‑heavy and called per
layer during export. Today they share a module‑level cache that is GUI‑
thread‑only. Moving to a `QReadWriteLock`‑protected cache would let
multiple export workers share renderers safely.

Cost: low‑medium, but only meaningful once §2.1 is done.

---

## 3. What CANNOT (or should not) move off the GUI thread

These are non‑starters in Qt 5 and would require either a full Qt 6
move or replacing the affected subsystem wholesale:

- **`QGraphicsScene` / `QGraphicsView`** — Qt explicitly forbids
  multithreaded scene mutation. The Breadboard/Schematic/PCB sketches
  must stay on the GUI thread.
- **`ItemBase` and all subclasses** — `QGraphicsItem`‑derived;
  same constraint as scene.
- **`WaitPushUndoStack` / `QUndoCommand`** — `QUndoStack` is GUI‑thread‑
  only. Commands themselves can do expensive work in their constructor
  (off‑thread) but `redo()`/`undo()` must run on the GUI thread.
- **`MainWindow` and dock widgets** — all `QWidget` subclasses; Qt
  widgets are GUI‑thread‑only.
- **`FSvgRenderer` cache mutations** — see §2.5; can be made thread‑
  safe but only with explicit locking.
- **`DebugDialog::debug()`** — must be either (a) made thread‑safe with
  a `QMutex` around the file write, or (b) routed through a queued
  signal to a GUI‑thread sink. The latter is the Qt‑idiomatic choice.

---

## 4. Phased implementation plan

> Each phase is independently shippable. Do not start phase N+1 until
> phase N has been merged and shaken out for a release cycle.

### Phase 0 — Foundations (1 PR, no behavior change)

- Add `QMutex` (or replace with a queued signal pipeline) to
  `DebugDialog::debug()`. Required for *any* future worker to log
  safely.
- Add a `QReadWriteLock` around the `FSvgRenderer` cache. No callers
  see a behavior change; the lock is uncontested today.
- Add a benchmark harness under `tests/auto/perf/` that measures
  baseline export wall‑clock for a known panel. Required to prove
  later phases actually win.

### Phase 1 — Parallel Gerber export (1 PR, user-visible win)

- Refactor `GerberGenerator::exportToGerber()` to take a `QStringList`
  of layer SVGs and run them through `QtConcurrent::mapped()`.
- Same refactor in `FabExporter::renderRealPanelLayers()`.
- Wire a determinate `QProgressDialog` to a `QFutureWatcher` so the
  panelizer wizard shows real per‑layer progress instead of the
  current indeterminate spinner.
- Add a unit test that runs an N=4 panel and asserts identical Gerber
  byte output as the serial path.

### Phase 2 — Parts library load on multiple cores (1 PR)

- Move `.fzp` parse to `QtConcurrent::mapped()` with a thread‑local
  intermediate `QHash`, merged on the main thread.
- Verify the splash‑screen progress callback is invoked from queued
  connections, not direct calls into widgets.

### Phase 3 — Autorouter parallelization (multi‑PR effort)

- Audit `JumpingHotWiresRouter` and `CMRouter` for shared mutable
  state. Document every static, every singleton, every cache.
- Introduce a `Router::Context` struct that carries per‑route state.
- Partition nets by frontier overlap; run independent batches on
  `QThreadPool::globalInstance()`.
- Each phase here should ship behind a `CONFIG += parallel_autoroute`
  qmake flag for one release before becoming default.

### Phase 4 — Panelizer layout search (1 PR, low risk)

- Move the binary‑search fit loop in `PanelizerWizard::runPanelize()`
  into a worker via `QtConcurrent::run()`.
- GUI shows determinate progress driven by `QFutureWatcher`.

### Phase 5 — Long term: Qt 6 + `QPromise`

Qt 5 → Qt 6 is a separate, larger conversation (see `AGENTS.md`:
"Qt 6 NOT supported"). When that happens, `QPromise` /
`QFuture::then()` give a much cleaner async pipeline than the current
`QFutureWatcher` + signal/slot dance. But this is a project‑wide
upgrade, not a multithreading change in itself.

---

## 5. Risks, gotchas, and non‑negotiables

- **`QObject` thread affinity:** any `QObject` created on a worker
  thread belongs to that thread. Cross‑thread signal connections must
  be `Qt::QueuedConnection`. Mistakes here produce silent corruption,
  not crashes.
- **`QPixmap` and `QImage`:** `QPixmap` is GUI‑thread‑only;
  `QImage` is thread‑safe. The export path uses `QImage` already, so
  Phase 1 is safe.
- **DRC graph mutation during export:** export currently calls into the
  DRC graph (`ConnectorItem` topology) while it walks layers. That
  graph is GUI‑thread‑owned — workers must operate on **snapshots**,
  not live `ConnectorItem` pointers.
- **Plugin code / Eagle import / Code view (`src/program/`):** these
  shell out to external processes. They already run via `QProcess`
  which is intrinsically off‑thread; no change needed.
- **Translation files:** `lupdate` does not parse worker‑thread code
  differently; no impact on `tr()` strings.
- **Stress test on Windows:** Qt's `QFile` flush semantics differ on
  Windows vs. POSIX. Any parallel‑write code path must be exercised
  on Windows before merge.

---

## 6. Effort estimate

| Phase | Scope | Confidence | Reviewer cost |
|-------|-------|------------|---------------|
| 0 | Foundations | High | Low |
| 1 | Parallel export | High | Medium |
| 2 | Parts load | Medium | Medium |
| 3 | Autorouter | Low | High |
| 4 | Layout search | High | Low |
| 5 | Qt 6 migration | N/A | N/A (separate effort) |

Phases 0, 1, 2, 4 together would deliver most of the user‑visible
"feels faster" wins without touching the autorouter. Phase 3 is the
real engineering project and should be costed independently.

---

## 7. What this means for the panelizer specifically

The user‑visible "the wizard hangs for several seconds on Next" issue
that prompted this document is solved by **Phases 0 + 1 + 4** in
combination:

1. **Phase 0** makes worker logging safe.
2. **Phase 1** parallelizes the actual Gerber writes — the biggest
   chunk of wall‑clock time in `runPanelize()`.
3. **Phase 4** moves the layout binary search off the GUI thread, so
   the progress dialog updates while the search runs.

Until those land, the current `QProgressDialog` with indeterminate
busy indicator (just added) is the right interim UX — it tells the
user *something is happening* without lying about how far along the
work is.

---

## 8. Bottom line

Yes, Fritzing can be made meaningfully multi‑threaded, but not in a
single PR and not "all CPUs at once" the way a render farm would be.
The realistic shape is:

- **Easy wins (Phases 0, 1, 4):** worth doing soon, ~3 focused PRs.
- **Medium effort (Phase 2):** parts library load, one PR with
  benchmark proof.
- **Real engineering project (Phase 3):** parallel autorouter, a
  multi‑release effort.
- **The GUI itself stays single‑threaded forever** — that's a Qt
  constraint, not a Fritzing choice.

Start with Phase 1. It's the highest‑ROI work and it directly improves
the workflow the user just asked about.
