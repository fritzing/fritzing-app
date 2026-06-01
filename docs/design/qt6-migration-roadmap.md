# Qt 5 → Qt 6 Migration Roadmap

> Audience: every developer who touches Fritzing source, from first-week
> junior to long-time maintainer. This document is written to be read
> top-to-bottom by someone who has never ported a Qt application before,
> and to be skimmed by someone who has done it ten times. **Leave no
> rock unturned** — that means we explain the *why* of each step, not
> just the *what*.

> Companion docs:
> - [`multi-threaded.md`](multi-threaded.md) — Phase 5 of that plan
>   (`QPromise`, structured concurrency) **depends on Qt 6** and is owned
>   here.
> - [`../../AGENTS.md`](../../AGENTS.md) — coding conventions still
>   apply; Qt 6 does *not* relax C++14-era rules unless this doc
>   explicitly says so.

---

## Table of Contents

1. [Why migrate at all](#1-why-migrate-at-all)
2. [Where we are today](#2-where-we-are-today)
3. [Glossary for new contributors](#3-glossary-for-new-contributors)
4. [What changes between Qt 5 and Qt 6](#4-what-changes-between-qt-5-and-qt-6)
5. [Dependency audit](#5-dependency-audit)
6. [The phased plan](#6-the-phased-plan)
7. [Per-subsystem migration recipes](#7-per-subsystem-migration-recipes)
8. [Build-system strategy: qmake first, CMake later](#8-build-system-strategy-qmake-first-cmake-later)
9. [Tooling that does the boring work for you](#9-tooling-that-does-the-boring-work-for-you)
10. [Testing strategy](#10-testing-strategy)
11. [Platform / packaging notes](#11-platform--packaging-notes)
12. [Risk register](#12-risk-register)
13. [Junior-dev runbook (copy-pasteable)](#13-junior-dev-runbook-copy-pasteable)
14. [Sequencing relative to the multi-threading plan](#14-sequencing-relative-to-the-multi-threading-plan)

---

## 1. Why migrate at all

Qt 5 is end-of-life for open-source users. The relevant facts:

| Fact | Source / impact |
|---|---|
| Qt 5.15 LTS open-source support ended **May 2023**. | Subsequent 5.15.x patches (security, Wayland fixes, compiler updates) are commercial-only. Distros backport some but not all. |
| Qt 6 has been GA since **December 2020**; Qt 6.5 LTS is the current LTS line. | Mainstream distros (Ubuntu 24.04, Fedora 40+, Debian trixie) ship Qt 6 as the default Qt and are deprecating Qt 5 packages. |
| New CPU + GPU + OS bring-up only happens in Qt 6. | HiDPI scaling (per-screen DPR), Wayland fractional scaling, ARM64 macOS, Windows on ARM, accessibility (UIA on Windows), modern OpenGL/Metal/Vulkan backends. |
| C++ standard floor for Qt 6 is **C++17**. | We are already on C++17 (`phoenix.pro` line 26). No blocker. |
| Modern static analyzers and sanitizers (clazy 1.13+, clang-tidy 17+) increasingly assume Qt 6 APIs. | Static analysis quality improves on Qt 6. |

The cost of *not* migrating compounds: every new contributor sees
deprecation warnings, packagers patch around removed APIs in distro
forks (which we never see), and the multi-threading work in
[`multi-threaded.md`](multi-threaded.md) Phase 5 (structured `QPromise`
chains) needs Qt 6 to land cleanly.

---

## 2. Where we are today

Concrete starting state at the time of writing (verified from the
tree, not assumed):

- `phoenix.pro` line 26: `CONFIG += c++17` — **already C++17**.
- `phoenix.pro` lines 165–168:
  ```pro
  QT += concurrent core gui network printsupport serialport sql svg widgets xml
  equals(QT_MAJOR_VERSION, 6) {
    QT += core5compat svgwidgets
  }
  ```
  Someone already wired a *conditional* Qt 6 module list. We have not
  flipped a CI build on, but the door is open.
- Logging is centralized through `DebugDialog::debug(...)`
  (`src/debugdialog.h`). Direct `qDebug()` is rare and lives mostly in
  half-commented-out `src/dialogs/fabuploadprogress.cpp` and a few
  `src/simulation/*.cpp` `std::cout` lines.
- `Qt::endl` is already used everywhere `endl` appears (good — Qt 6
  removed the unscoped `endl`).
- `QRegExp`, `QLinkedList`, `QTextCodec`, `qrand`, `QStringRef`,
  `QSignalMapper`, `QDesktopWidget`, `QString::SkipEmptyParts` — these
  Qt 5–era APIs **are no longer present** in `src/`. A previous round
  of porting clearly happened. (`QTextStream::setCodec("UTF-8")` does
  still appear in `src/debugdialog.cpp:155` — that needs handling, see
  §7.3.)
- `QMouseEvent::pos()` is still used in two places in
  `src/sketch/sketchwidget.cpp` and `QWheelEvent` constructors are
  used in two places in `src/gerberpreview/gerberpreviewdialog.cpp` —
  both need a touch-up.
- No CMakeLists exist. Build system is still pure qmake.
- Tests are sparse: `tests/auto/test_svg`, `test_textutils`,
  `test_gerber`, `test_qsysinfo`. All use `QtTest` and qmake.
- Docker images target Ubuntu Bionic (Qt 5.9) and Xenial. There is
  **no Qt 6 reference image yet**.

**Bottom line:** the codebase is roughly 80 % of the way through the
mechanical part of the port. What is missing is (a) a CI build green
under Qt 6, (b) the few remaining API touch-ups, (c) a Qt 6 packaging
story, and (d) confidence that we did not regress runtime behaviour.

---

## 3. Glossary for new contributors

Skim this before reading §4. If a term here is unfamiliar, stop and
look it up before continuing — the rest of the doc assumes you know
these.

| Term | What it means here |
|---|---|
| **qmake** | Qt's original build-system frontend. Reads `.pro` and `.pri` files, generates `Makefile`. Deprecated upstream; replaced by CMake but still functional in Qt 6 up to and including 6.6. |
| **CMake** | Cross-platform build generator. Qt 6's first-class build system. Provides `find_package(Qt6 ...)`, `qt_add_executable`, `qt_add_resources`, etc. |
| **MOC** | Meta-Object Compiler. Generates `moc_*.cpp` from headers containing `Q_OBJECT`. Same in both Qt versions. |
| **uic / rcc** | UI-file compiler / resource compiler. Same in both Qt versions. |
| **core5compat** | Qt 6 module that ships removed-from-core Qt 5 classes (`QTextCodec`, `QStringRef`, `QRegExp`, `QLinkedList`, `QHash::insertMulti`, etc.) for porting convenience. You can use it as a crutch; you should remove the crutch before declaring the port done. |
| **svgwidgets** | In Qt 5, `QSvgWidget` lived in `QtSvg`. In Qt 6 it moved to a new `QtSvgWidgets` module — you must add `svgwidgets` to `QT +=` to keep using it. |
| **Deprecation gate** | `QT_DISABLE_DEPRECATED_BEFORE` (qmake `DEFINES +=`). Setting it to `0x060000` (i.e. "deprecate nothing before Qt 6.0") gives you a clean Qt 6 build. Setting it to `0x070000` shows you the next batch of upcoming removals. |
| **High-DPI scaling** | Qt 6 makes per-screen `devicePixelRatio()` *always-on*. You cannot opt out. This affects any code that mixes pixels and logical units (most of `src/sketch/` and `src/items/`). |
| **Implicit `QObject` → `QObject` conversion in signal/slot** | Qt 5 allowed connecting `signal(int)` to `slot(int, QString = "")` with extra defaulted parameters. Qt 6 is stricter — argument counts must match exactly for member-pointer connections. |
| **`QVector` / `QList`** | In Qt 6 these are the **same type** (`QList`-backed by contiguous storage). Most code keeps compiling; some `prepend()` performance characteristics change. |
| **clazy** | Clang-based static checker that knows Qt idioms. The `level1` checks include `qt6-qhash-signature`, `qt6-deprecated-api-fixes`, `qt6-fwd-fixes`. Indispensable. |
| **`qt5to6` script** | Ships in Qt 6's `bin/`. A `clang-tidy` wrapper that auto-rewrites the easy mechanical changes (`Qt::SkipEmptyParts` etc.). Not a substitute for human review, but worth running once. |

---

## 4. What changes between Qt 5 and Qt 6

This is the catalogue. Each row tells you (a) what was removed or
renamed, (b) the recommended replacement, (c) whether `core5compat`
papers it over, and (d) where Fritzing is affected.

### 4.1 Removed or relocated APIs

| Qt 5 | Qt 6 replacement | `core5compat`? | Fritzing impact |
|---|---|:---:|---|
| `qrand()`, `qsrand()` | `QRandomGenerator::global()->bounded(...)` | n/a — header-only shim, but use the new API | None — already gone. |
| `QString::SkipEmptyParts` | `Qt::SkipEmptyParts` | n/a | None — already gone. |
| `QStringRef` | `QStringView` | yes (deprecated wrapper) | None — already gone. |
| `QRegExp` | `QRegularExpression` (PCRE-based) | yes | None — already gone. |
| `QLinkedList` | `std::list` or `QList` | yes | None — already gone. |
| `QTextCodec` | `QStringConverter` (UTF-only) or `core5compat` | yes | `debugdialog.cpp:155` — see §7.3. |
| `QTextStream::setCodec("UTF-8")` | drop — UTF-8 is the default | n/a | `debugdialog.cpp:155` — delete the line. |
| `QSignalMapper` | lambda captures | yes (deprecated) | None — already gone. |
| `QDesktopWidget` / `QApplication::desktop()` | `QGuiApplication::screens()`, `QWidget::screen()` | no | None — already gone. |
| `QWheelEvent::delta()`, `pos()` | `angleDelta()`, `position()` | no | `src/gerberpreview/gerberpreviewdialog.cpp:221,230` synthesize `QWheelEvent` with the Qt 5 ctor — needs the Qt 6 ctor (see §7.1). |
| `QMouseEvent::pos()` (returns `QPoint`) | `position()` (returns `QPointF`) | deprecated but kept | `src/sketch/sketchwidget.cpp:2215,3179` — change to `position().toPoint()` (see §7.2). |
| `QMouseEvent::globalPos()` | `globalPosition().toPoint()` | deprecated but kept | same two call sites. |
| `QHash::insertMulti(k,v)` | `QMultiHash::insert(k,v)` | yes | Audit; if any uses appear in `src/items/`, rewrite. |
| `QSet::toList()` / `QList::toSet()` | `QList(s.begin(), s.end())` / `QSet(l.begin(), l.end())` | yes | Audit. |
| `QVariant::Type` enum, `QVariant::canConvert<T>()`, etc. | `QMetaType::Type`, `canView<T>()` / `canConvert(QMetaType)` | yes | Likely affects `src/model/modelpart.cpp` — audit. |
| `QProcess::start(QString)` single-string overload | `startCommand(QString)` or split args | n/a | Audit `src/program/`, `src/version/`. |
| `qVariantValue<T>(v)` | `v.value<T>()` | n/a | Audit. |
| `endl` (unqualified `Qt::endl` ancestor) | `Qt::endl` | n/a | Already done. |
| `Qt::MidButton` | `Qt::MiddleButton` | deprecated but kept | Audit. |
| `Qt::WA_NoSystemBackground` semantics | unchanged but interacts with new high-DPI compositor | n/a | Watch for repaint glitches. |

### 4.2 Module reshuffling

| Module | Qt 5 location | Qt 6 location | Action |
|---|---|---|---|
| `QtSvgWidgets` (`QSvgWidget`, `QGraphicsSvgItem`) | inside `QtSvg` | new `QtSvgWidgets` module | `QT += svgwidgets` (already conditional in `phoenix.pro`). |
| `QTextCodec` / `QStringRef` / `QRegExp` etc. | core | `core5compat` module | `QT += core5compat` (already conditional). |
| `QtXml` (`QDom*`, SAX) | always present | still present but deprioritized in favor of `QXmlStreamReader/Writer` | None — we already prefer the stream API. |
| `QtMultimedia` widgets | `multimediawidgets` | gone — use QML or roll your own widget | Not used in Fritzing. |
| `QtScript`, `QtScriptTools` | core | **removed entirely** | Not used. |
| `QtMacExtras`, `QtX11Extras`, `QtWinExtras` | extras | **removed** — use native APIs or Qt 6's unified replacements (`QFileInfo::birthTime`, `QGuiApplication::nativeInterface<>()`) | Audit `src/utils/`. |
| `QtSerialPort` | extra module | extra module, unchanged | None. |

### 4.3 Behavioural changes (no compiler error — runtime trap)

These do not throw a build warning. They will silently change runtime
behaviour. Treat them as test-plan must-cover items.

1. **High-DPI auto-enabled.** `QT_AUTO_SCREEN_SCALE_FACTOR`,
   `Qt::AA_EnableHighDpiScaling` are no-ops. The sketch view's
   pixel↔mm conversions (`fsvgrenderer.cpp`, `viewgeometry.cpp`,
   anything that calls `QPaintDevice::logicalDpiX()`) must be reviewed
   to ensure they pull DPI from the *paint device* not the desktop.
2. **`QImage` / `QPixmap` default formats** lean toward
   `Format_ARGB32_Premultiplied`. Code that hand-rolls byte access
   (`bits()`, `scanLine()`) may need a `convertToFormat()` insert.
3. **`QGraphicsScene` event delivery order** subtly differs. Items
   that overrode `sceneEvent()` and assumed a particular order — test
   drag-and-drop of parts onto the breadboard heavily.
4. **`QString` / `QByteArray` no longer convert implicitly to/from
   `const char *`** in some overloads. Build errors point you at the
   call sites, but the fix is sometimes "what did the author mean?"
5. **Locale-aware `QString::toDouble` / `toInt`** now stricter about
   trailing whitespace. SVG attribute parsing in `src/svg/` may need a
   `.trimmed()` in front.
6. **`QPrinter` device pixel ratio** changes. Gerber/PDF export paths
   in `src/svg/`, `src/autoroute/` must be re-tested at 1.0×, 1.5×,
   2.0× DPR.
7. **`QSqlDatabase` Sqlite driver** uses SQLite 3.36+ — minor SQL
   syntax compatibility.
8. **`QNetworkAccessManager` TLS backend** is now per-platform (Schannel
   on Windows by default). If you see TLS handshake failures in the
   `dialogs/fabuploadprogress.cpp` flow, that is why.

---

## 5. Dependency audit

These are the third-party libraries we link against. Each must be
verified to work on Qt 6 *before* we flip CI.

| Dep | Source | Qt 6 status | Action |
|---|---|---|---|
| **QuaZip** | `pri/quazip.pri` + `pri/quazipdetect.pri` | QuaZip 1.4+ has Qt 6 builds. Distro packages are sometimes called `libquazip1-qt6-dev`. | Test detection script handles both `quazip1-qt5` and `quazip1-qt6`. |
| **libgit2** | `pri/libgit2detect.pri`, statically linked by default (`LIBGIT_STATIC = true`) | Qt-agnostic. Works as-is. | Bump to a recent (1.7+) version for OpenSSL 3 / mbedTLS support. |
| **Boost** | `pri/boostdetect.pri`, header-only | Qt-agnostic. Works as-is. | None. |
| **ngspice** | `pri/spicedetect.pri`, `src/simulation/` | Qt-agnostic. Works as-is. | None. |
| **OpenSSL** | runtime via `QNetworkAccessManager` | Qt 6.4+ supports OpenSSL 3.x. We already link against `libssl.so.3` in current builds. | Verify on every target distro. |
| **libgit2 transports (HTTPS)** | uses bundled HTTP backend | Qt-agnostic | None. |
| **Qt Serial Port** | `QT += serialport` | shipped with Qt 6 as separate package on some distros (`qt6-serialport-dev`) | Update Docker images. |

**Action item:** add a `pri/qt6detect.pri` that fails fast with a
human-readable error if a needed Qt 6 module is missing
(`error("Qt 6 detected but QtSvgWidgets module is not installed. Try: apt install qt6-svg-dev")`).

---

## 6. The phased plan

The migration is broken into five phases. **Each phase ends with a
green CI build and is mergeable to `develop` on its own.** Do not
batch phases.

### Phase A — Dual-build hygiene on Qt 5

**Goal:** make the codebase compile *and* run cleanly on Qt 5.15 with
`QT_DISABLE_DEPRECATED_BEFORE = 0x060000` set. This forces every Qt 5
deprecation to be a hard error, exposing every remaining hotspot
without ever needing a Qt 6 installation.

**Tasks:**

1. Add to `phoenix.pro` (top, gated for Qt 5):
   ```pro
   equals(QT_MAJOR_VERSION, 5) {
       DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000
   }
   ```
2. Compile. Fix every error. Each fix is **one commit, one class of
   API** (e.g. "Replace QMouseEvent::pos() with position().toPoint()"
   is one commit; do not mix with `QTextStream` changes).
3. Run the existing `tests/auto/test_*` suite. Fix any regressions.
4. Manual smoke: open a sample sketch, route a trace, export Gerbers.

**Definition of done:** Phase A is done when Linux + macOS + Windows
debug + release builds finish with zero deprecation warnings on Qt 5.15.

### Phase B — Opt-in Qt 6 build (CI on, default off)

**Goal:** add a Qt 6 build target behind a flag, get it green, but do
not change the default for contributors.

**Tasks:**

1. Author `docker/Dockerfile.qt6` based on Ubuntu 24.04:
   ```dockerfile
   FROM ubuntu:24.04
   RUN apt-get update && apt-get install -y \
       build-essential git pkg-config \
       qt6-base-dev qt6-svg-dev qt6-tools-dev qt6-tools-dev-tools \
       qt6-serialport-dev qt6-5compat-dev \
       libqt6sql6-sqlite libqt6printsupport6 \
       libboost-dev libgit2-dev zlib1g-dev libssl-dev libudev-dev
   ```
2. Author `docker/build-linux-qt6.sh` (mirror of `build-linux.sh`).
3. Make `pri/quazipdetect.pri` find the Qt 6 fork.
4. Wire a parallel GitHub Actions job (or whatever CI replaces Travis)
   to run the Qt 6 build on every PR.
5. Fix whatever the Qt 6 compile reveals. Most will already be done
   from Phase A.
6. **Tag the first commit that boots on Qt 6** with `qt6-first-boot`
   for future bisection.

**Definition of done:** Phase B is done when `./docker/build-linux-qt6.sh`
runs the binary and the binary opens a sketch.

### Phase C — Behavioural soak

**Goal:** the Qt 6 build matches Qt 5 build feature-for-feature, with
no visible regressions.

**Tasks:**

1. Run the full panelizer wizard end-to-end on Qt 6: load sketch →
   panelize → export Gerbers → open preview → measure → export BOM.
2. Repeat for: parts editor, autoroute, schematic-to-PCB, Eagle import,
   Inkscape SVG round-trip, `.fzz` save+load, print, update check.
3. File an issue tag `qt6-regression` for each delta. Fix or document.
4. Add a `tests/auto/test_qt6_smoke/` parameterized test that asserts
   key invariants (DPI math, SVG bounds, undo/redo symmetry) on both
   Qt versions.

**Definition of done:** Phase C is done when the maintainers sign off
that Qt 6 is at least as good as Qt 5 on all three platforms.

### Phase D — Flip the default

**Goal:** Qt 6 becomes the documented default; Qt 5 becomes a fallback.

**Tasks:**

1. Update `INSTALL.txt`, `README.md`, `AGENTS.md`,
   `.github/copilot-instructions.md` to say **"Qt 6 (>= 6.5 LTS,
   6.7 recommended)"**.
2. Update the `docker/build-linux.sh` to point at the Qt 6 image.
3. Keep the Qt 5 path alive behind `CONFIG += qt5_legacy` for one
   release cycle, then remove.
4. Release one full version of Fritzing built against Qt 6.

**Definition of done:** Phase D is done when a release is published
whose Linux/macOS/Windows binaries are linked against Qt 6.

### Phase E — Drop Qt 5 + migrate to CMake

**Goal:** modern build system, single Qt version.

**Tasks:**

1. Delete all `equals(QT_MAJOR_VERSION, 5)` blocks and the
   `core5compat` conditional.
2. Author `CMakeLists.txt` at repo root and one per subsystem mirroring
   the current `pri/*.pri`. Use `qt_add_executable(Fritzing ...)`,
   `qt_standard_project_setup()`, `qt_add_resources()`,
   `qt_add_translations()`. See §8 for the structure.
3. Keep `phoenix.pro` for one release as a *deprecated* alternative
   build, then remove.
4. Update Docker images to drop qmake entirely.

**Definition of done:** Phase E is done when a release ships built
exclusively from CMake.

---

## 7. Per-subsystem migration recipes

These are the spots in *our* code that need hand-holding. Each
includes before/after for pattern matching.

### 7.1 Synthesizing `QWheelEvent` — `src/gerberpreview/gerberpreviewdialog.cpp`

The Qt 5 ctor took `(pos, globalPos, pixelDelta, angleDelta,
qint32 unused, orientation, buttons, modifiers)`. The Qt 6 ctor takes
`(position, globalPosition, pixelDelta, angleDelta, buttons,
modifiers, phase, inverted, source)`.

**Before (Qt 5):**
```cpp
QWheelEvent we(QPointF(m_preview->width() / 2.0, m_preview->height() / 2.0),
               QPoint(0, 0), QPoint(0, delta),
               Qt::NoButton, Qt::ControlModifier,
               Qt::Vertical);
QApplication::sendEvent(m_preview, &we);
```

**After (works on both):**
```cpp
const QPointF center(m_preview->width() / 2.0, m_preview->height() / 2.0);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
QWheelEvent we(center, m_preview->mapToGlobal(center.toPoint()),
               QPoint(0, 0), QPoint(0, delta),
               Qt::NoButton, Qt::ControlModifier,
               Qt::NoScrollPhase, /*inverted*/ false);
#else
QWheelEvent we(center, QPoint(0, 0), QPoint(0, delta),
               Qt::NoButton, Qt::ControlModifier, Qt::Vertical);
#endif
QApplication::sendEvent(m_preview, &we);
```

Once Phase D ships, drop the `#else` branch.

### 7.2 `QMouseEvent::pos()` / `globalPos()` — `src/sketch/sketchwidget.cpp`

Two existing call sites synthesize a `QMouseEvent` to fake a left-button
press for the hand tool. In Qt 6 the integer-`QPoint` getters are
deprecated; the `QPointF` versions are canonical.

**Before:**
```cpp
event = hackEvent = new QMouseEvent(event->type(),
    event->pos(), event->globalPos(),
    Qt::LeftButton, event->buttons() | Qt::LeftButton, event->modifiers());
```

**After (works on both):**
```cpp
event = hackEvent = new QMouseEvent(event->type(),
    event->position(), event->globalPosition(),
    Qt::LeftButton, event->buttons() | Qt::LeftButton, event->modifiers());
```

The `QMouseEvent` ctor accepting `QPointF` exists in both 5.14+ and
6.x, so no `#if` is needed.

### 7.3 `QTextStream::setCodec` — `src/debugdialog.cpp:155`

**Before:**
```cpp
out.setCodec("UTF-8");
```

**After (works on both — UTF-8 is the Qt 6 default, and
`QTextStream` always defaulted to UTF-8 on Qt 5 unless overridden):**
```cpp
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
out.setCodec("UTF-8");
#endif
```

Once Phase D ships, delete the line entirely.

### 7.4 `QHash`/`QMultiHash` audit — connector and bus model

`src/connectors/` and `src/model/` use `QHash` heavily. Audit for:

- `QHash::insertMulti(k, v)` → `QMultiHash`.
- Manual `qHash(MyType)` overloads must now return `size_t`, not
  `uint`. Example:

  **Before:**
  ```cpp
  inline uint qHash(const ConnectorKey &k, uint seed = 0) noexcept;
  ```
  **After (works on both):**
  ```cpp
  inline size_t qHash(const ConnectorKey &k, size_t seed = 0) noexcept;
  ```

### 7.5 SVG pipeline — `src/svg/`

The SVG normalizer is the foundation of display + Gerber/PDF export
(see [`AGENTS.md`](../../AGENTS.md) §4). Migration risks:

- `QSvgRenderer::boundsOnElement()` rounds slightly differently on
  Qt 6 — bake a regression test from a known-good Gerber.
- `QSvgGenerator::setResolution()` and viewBox math interact with the
  new always-on `devicePixelRatio` — verify by exporting a board at
  1.0× and 2.0× and comparing the resulting PDF bounds.
- `QSvgWidget` moved to `QtSvgWidgets` (already handled in `phoenix.pro`).

### 7.6 Print / PDF — `src/svg/exportparametersdialog.cpp` and friends

`QPrinter` got friendlier in Qt 6 but `QPrinter::PaperSize` enum was
renamed to `QPageSize::PageSizeId`. We already use `QPageSize` in some
places; audit and unify.

### 7.7 Translations — `translations/`

`lupdate` and `lrelease` exist in both Qt versions and the `.ts` format
is forward-compatible. **Do not regenerate `.ts` files just because you
ran a Qt 6 `lupdate`** — the diff will be enormous (line-number noise)
and bury real changes. Run `lupdate` only when you have changed user
strings, and review the diff carefully.

---

## 8. Build-system strategy: qmake first, CMake later

We are explicitly **not** doing qmake→CMake as part of the Qt 6 port.
Doing both at once turns every regression into a "is it Qt 6 or
CMake?" guessing game.

**Rules:**

1. Phases A–D stay on qmake. Qt 6 still supports qmake through at
   least Qt 6.6, which gives us multi-year runway.
2. Phase E is the CMake conversion, and it ships as its own PR with
   its own release.
3. The CMake structure will mirror today's `pri/*.pri` layout:
   ```
   CMakeLists.txt           # root: project(), find_package(Qt6 ...)
   cmake/Fritzing.cmake     # helpers
   src/CMakeLists.txt       # adds the Fritzing target
   src/<area>/CMakeLists.txt  # one per subsystem, contributes sources
   ```
4. Use `qt_add_resources` for `phoenixresources.qrc`, `qt_add_translations`
   for `translations/`, `qt_finalize_executable` last.
5. CI keeps both build systems green for one release cycle to catch
   subtle differences (resource embedding, MOC scanning, install paths).

---

## 9. Tooling that does the boring work for you

You are not expected to do all the rewrites by hand. Use these.

| Tool | What it does | How to run |
|---|---|---|
| **`clazy`** | Clang plugin with Qt-aware checks. Has explicit `qt6-*` checks. | `CXX=clang++ CXXFLAGS="-Xclang -load -Xclang libclazy.so -Xclang -add-plugin -Xclang clazy" qmake ../phoenix.pro && make` or `CLAZY_CHECKS=qt6-qhash-signature,qt6-deprecated-api-fixes,qt6-fwd-fixes`. |
| **`clang-tidy` with `qt6-*` checks** | Automatic fixits for many things. | `clang-tidy -checks=-*,qt-*,modernize-* -fix src/foo.cpp -- $(qmake --query QT_INSTALL_HEADERS | sed 's|^|-I|')` |
| **`qt5to6` (in Qt 6 `bin/`)** | Drives `clang-tidy` with Qt 6's curated check set, including renames. | `path/to/qt6/bin/qt5to6 src/` |
| **`-Wdeprecated-declarations`** | Compiler warning, always-on once Phase A's `QT_DISABLE_DEPRECATED_BEFORE` is set. | Just compile. |
| **`ASAN_OPTIONS=detect_leaks=1` + AddressSanitizer build** | Catches use-after-free in the new event-delivery order. | `qmake CONFIG+=debug CONFIG+=sanitizer` (custom) or `QMAKE_CXXFLAGS+=-fsanitize=address`. |
| **`QT_LOGGING_RULES`** | Runtime: enables Qt's category loggers. Useful for spotting deprecation calls that only fire at runtime. | `QT_LOGGING_RULES="qt.*.deprecated=true" ./Fritzing` |

**Anti-pattern warning:** do **not** blindly accept `qt5to6` rewrites.
It cannot reason about ownership or threading. Review every hunk.

---

## 10. Testing strategy

### 10.1 Automated

- Keep `tests/auto/` building on both Qt versions. Each `test_*.pro`
  gets the same `equals(QT_MAJOR_VERSION, 6) { QT += core5compat }`
  guard.
- Add `tests/auto/test_qt6_smoke/` that runs in both: validates SVG
  bounds, undo/redo round-trip, `.fzz` save+load, and the Gerber
  pipeline byte-for-byte against a checked-in golden file.
- CI matrix: **(Linux, macOS, Windows) × (Qt 5.15, Qt 6.5 LTS,
  Qt 6.7)**. That is nine cells. Drop the macOS-Qt5 cell after Phase D.

### 10.2 Manual smoke (every release candidate during Phases B–D)

A new contributor should be able to walk through this in 30 minutes:

1. `Fritzing` launches; splash dismisses; main window opens.
2. Open `sketches/core/...` sample.
3. Switch BB → Schematic → PCB views — no Z-order glitches.
4. Drag a part from the bin onto the breadboard, wire it.
5. Undo (Ctrl+Z) the wire, redo.
6. Autoroute the PCB.
7. Export Gerbers (Routing → Export for Production).
8. Open the panelizer wizard, panelize a 2×2 grid, preview, export.
9. File → Save As `.fzz`, close, reopen — board identical.
10. Help → Check for updates — network round-trip succeeds.

If steps 7–8 produce *different bytes* on Qt 6 vs Qt 5 it is almost
always a `QSvgRenderer` rounding delta — diff the SVGs first, not the
Gerbers.

### 10.3 Translation tests

After Phase A: pick one non-English locale, run the app, confirm no
`tr(...)` strings render in English. After Phase E: same, with the
CMake-built binary.

---

## 11. Platform / packaging notes

### 11.1 Linux

- AppImage: rebuild against the Qt 6 base image. `linuxdeploy-plugin-qt`
  has full Qt 6 support; pin to a recent version.
- Flatpak: `org.fritzing.Fritzing.yml` switches `org.kde.Platform//5.15` →
  `org.kde.Platform//6.7`.
- Debian package: depends on `libqt6core6 libqt6gui6 libqt6widgets6
  libqt6svg6 libqt6svgwidgets6 libqt6serialport6 libqt6printsupport6
  libqt6sql6-sqlite libqt6xml6 libqt6concurrent6 libqt6network6
  libqt6core5compat6` (the last only during Phases A–D).

### 11.2 macOS

- Minimum deployment target rises to **macOS 11 Big Sur** (Qt 6.5+
  hard floor). Update `FritzingInfo.plist` `LSMinimumSystemVersion`.
- Universal binaries: Qt 6 supports `CONFIG += sdk_no_version_check`
  cleanly; `lipo` packaging still works.
- Notarization: unchanged. Code-signing requirements unchanged.
- `QMacExtras` is gone — any native API calls move to
  `QGuiApplication::nativeInterface<QNativeInterface::QCocoaApplication>()`.

### 11.3 Windows

- Toolchain floor moves from MSVC 2017 to **MSVC 2019** (Qt 6 hard
  floor). MSVC 2022 recommended.
- `windeployqt6` replaces `windeployqt`.
- Installer (`tools/win/`) NSIS scripts: bump Qt DLL list.
- Windows on ARM64 becomes possible — out of scope for this roadmap
  but unblocked.

---

## 12. Risk register

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| `QGraphicsScene` event-order change breaks drag-and-drop | medium | high | Phase C manual soak step 4. |
| SVG renderer rounding changes Gerber output bytes | high | medium | Phase C golden-file test (§10.1). |
| QuaZip Qt 6 fork has a path-traversal bug we did not have on Qt 5 | low | high | Re-audit the zip-extract paths in `src/svg/fzz*.cpp` against zip-slip; never extract outside the target directory. |
| High-DPI math breaks autorouter coordinate system | medium | high | Add a deliberate `devicePixelRatio = 2.0` test in `tests/auto/test_svg/`. |
| Distro packagers ship before we are done | medium | medium | Communicate the migration timeline on `forum.fritzing.org`. |
| CMake migration leaks generated files into the source tree | low | low | Enforce `CMAKE_BINARY_DIR` out-of-source builds; add to `.gitignore`. |
| Translation `.ts` files churn from `lupdate` differences | medium | low | Translation freeze during the bulk porting commits. |
| Qt 6 deprecates *more* APIs in 6.x (it will) | high | low | Phase D pins to an LTS; bump LTS once per release cycle. |

---

## 13. Junior-dev runbook (copy-pasteable)

> Goal: a developer who has never seen this project before can do
> Phase A locally in one sitting. Run these commands in order.

```bash
# 0. Prerequisites
sudo apt update
sudo apt install -y build-essential git pkg-config \
    qtbase5-dev qttools5-dev-tools libqt5svg5-dev libqt5serialport5-dev \
    libqt5sql5-sqlite libboost-dev libgit2-dev zlib1g-dev libssl-dev

# 1. Get the source + parts sibling
cd ~/code
git clone https://github.com/fritzing/fritzing-app.git
git clone https://github.com/fritzing/fritzing-parts.git fritzing-app/parts
cd fritzing-app
git checkout -b feature/qt6-prep develop

# 2. Add the strict-deprecation gate. Edit phoenix.pro and add near the top:
#
#    equals(QT_MAJOR_VERSION, 5) {
#        DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000
#    }
#
$EDITOR phoenix.pro

# 3. Build. Expect errors.
mkdir -p build && cd build
qmake ../phoenix.pro CONFIG+=debug
make -j"$(nproc)" 2>&1 | tee build.log

# 4. Triage the build.log. For each error:
#    - Find the file + line.
#    - Match the API against §4 of this doc.
#    - Apply the recipe in §7.
#    - Commit with a message: "Port <API> usage in <area>"
#    - Re-run make.

# 5. After build is clean, run the tests.
cd ../tests
qmake tests.pro && make -j"$(nproc)"
for t in auto/test_*/test_*; do
    [ -x "$t" ] && echo "--- $t ---" && "$t"
done

# 6. Smoke-test the binary.
cd ../debug64
./Fritzing  # open a sample sketch, route a trace, export Gerbers.

# 7. Push the branch and open a PR against develop. Reference this doc.
git push -u origin feature/qt6-prep
```

**Etiquette:**

- One API class per commit. "Port `QMouseEvent::pos()` call sites" is
  a commit. "Port everything" is not.
- PR description names the phase: `[Qt6/Phase A] Strict deprecation
  gate + first wave of fixes`.
- If a fix requires a non-obvious `#if QT_VERSION` guard, comment
  `// NOTE: Qt 5 ctor differs from Qt 6 ctor — see docs/design/qt6-migration-roadmap.md §7.1`.
- Never touch unrelated files in a Qt 6 PR. The diff has to stay
  reviewable.

---

## 14. Sequencing relative to the multi-threading plan

[`multi-threaded.md`](multi-threaded.md) defines five phases for moving
heavy work off the UI thread. Their relationship to *this* document:

| MT Phase | Qt 6 dependency? | Sequencing |
|---|---|---|
| MT Phase 1 — `QtConcurrent::run` for FabExporter | No | Land first; Qt 5 friendly. |
| MT Phase 2 — `QFutureWatcher` UI plumbing | No | Land second. |
| MT Phase 3 — Per-board headless render pool | No | Land third (this kills the "painfully slow panel preview" root cause). |
| MT Phase 4 — Cancellation + progress reporting | No | Land fourth. |
| MT Phase 5 — Structured concurrency with `QPromise` chains | **Yes — Qt 6 only** | Blocked on Qt 6 Phase D. |

So the ideal calendar is:

1. Land MT Phases 1–4 on Qt 5.
2. Land Qt 6 Phases A–D in parallel-where-possible.
3. Land MT Phase 5 on Qt 6.
4. Land Qt 6 Phase E (CMake).

Doing MT and Qt 6 work in *separate PRs* keeps every regression
attributable to one change.

---

## Appendix A — Where to ask questions

- Qt 6 porting questions: <https://doc.qt.io/qt-6/portingguide.html>
- Qt 6 deprecated-API list: <https://doc.qt.io/qt-6/obsoleteclasses.html>
- Fritzing forum: <https://forum.fritzing.org>
- File an issue against this roadmap: tag `qt6-migration` on
  <https://github.com/fritzing/fritzing-app/issues>.

---

## Appendix B — Change log for *this* document

| Date | Author | Change |
|---|---|---|
| YYYY-MM-DD | initial author | Initial draft covering Phases A–E. |

Update this table whenever you change the doc. Keep entries terse.
