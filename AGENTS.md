# AGENTS.md — Fritzing App

> Agent + contributor playbook for the Fritzing desktop application
> (`fritzing/fritzing-app`). This file is read by AI coding agents
> (Copilot, Claude, Cursor, etc.) **and** by human contributors. Treat
> every rule below as load-bearing: this is a large, public, GPLv3 EDA
> application with downstream packagers — sloppy contributions break
> distros.

---

## 1. Project at a Glance

| Item | Value |
|---|---|
| Language | C++14 (hard requirement, see [phoenix.pro](phoenix.pro#L31)) |
| Framework | Qt 5 (>= 5.9, **5.12 recommended**) or Qt 6 (>= 6.5 LTS) |
| Build system | qmake (`.pro` + `.pri` includes), **not** CMake |
| License (code) | GPL-3.0-or-later |
| License (docs / parts) | CC-BY-SA 3.0 |
| Default branch | `develop` (PRs target this, **not** `master`) |
| Companion repo | [`fritzing/fritzing-parts`](https://github.com/fritzing/fritzing-parts) — **must be cloned as `parts/` sibling** |

The application is split into many `.pri` submodules wired together by
[phoenix.pro](phoenix.pro). Each `pri/*.pri` corresponds 1:1 to a
`src/<area>/` directory — see [pri/](pri/) and [src/](src/).

---

## 13. Qt 6 Migration Status

Fritzing has begun the migration to Qt 6. The project now supports building with both Qt 5.15 and Qt 6.5+.

The Qt 6 migration is being tracked in the [Qt 6 migration roadmap](docs/design/qt6-migration-roadmap.md) and follows a phased approach:

1. **Phase A** - Dual-build hygiene on Qt 5 (Completed)
2. **Phase B** - Opt-in Qt 6 build (In Progress)
3. **Phase C** - Behavioural soak (Planned)
4. **Phase D** - Flip the default (Planned)
5. **Phase E** - Drop Qt 5 + migrate to CMake (Planned)

The migration is designed to be non-breaking, with proper version guards ensuring compatibility with both Qt versions.

---

## 2. Repository Layout (what lives where)

```
src/
  main.cpp, fapplication.*    -> entry point + QApplication subclass
  mainwindow/                 -> top-level window, menus, file IO orchestration
  sketch/                     -> the QGraphicsScene/View editing surface
  items/                      -> all schematic/breadboard/PCB graphics items
  connectors/                 -> connector + bus model (electrical topology)
  model/                      -> ModelPart / ModelBase (parts data model)
  referencemodel/             -> the in-memory parts library
  partsbinpalette/            -> parts bin dock UI
  partseditor/                -> the standalone parts editor
  autoroute/                  -> PCB autorouting (DRC + routers)
  svg/                        -> SVG read/normalize/render pipeline
  infoview/, dock/, dialogs/  -> auxiliary UI panels
  eagle/                      -> Eagle import
  program/                    -> "Code" view (Arduino, etc.)
  utils/, lib/                -> shared helpers; lib/ has third-party shims
  version/                    -> version + update-check
pri/                          -> qmake .pri include files (one per src/ area)
resources/                    -> images, fonts, templates, system icons, bins
parts/        (NOT in repo)   -> clone of fritzing-parts; required at runtime
sketches/                     -> sample sketches shipped with the app
help/                         -> in-app HTML help
translations/                 -> Qt linguist .ts/.qm
tests/auto/                   -> QtTest unit tests (currently: test_svg, test_textutils)
tools/                        -> packaging + release scripts (mac/win/linux)
docker/                       -> reference Linux build environment
config.tests/                 -> qmake feature probes (boost)
```

Do **not** invent new top-level folders. New source belongs in an
existing `src/<area>/` and must be referenced from the matching
[pri/<area>.pri](pri/).

---

## 3. Environment Setup (Linux — primary dev target here)

The canonical reference is [docker/Dockerfile.bionic](docker/Dockerfile.bionic).
Mirror those packages on a host install.

### 3.1 System dependencies (Debian/Ubuntu)

```bash
sudo apt update
sudo apt install -y \
  build-essential git pkg-config \
  qt5-default qttools5-dev-tools \
  libqt5serialport5-dev libqt5svg5-dev libqt5sql5-sqlite \
  libqt5printsupport5 libqt5xml5 libqt5sql5 \
  libboost-dev \
  libgit2-dev \
  zlib1g-dev libssl-dev libudev-dev \
  libjpeg-dev libpng-dev libncurses5-dev
```

Notes:
- Boost is detected by [pri/boostdetect.pri](pri/boostdetect.pri) via
  [config.tests/boost/](config.tests/boost/). If not found, qmake fails.
- libgit2 defaults to **static** linking (`LIBGIT_STATIC = true` in
  [phoenix.pro](phoenix.pro#L171)). Toggle to `false` to use the system
  shared lib.

### 3.2 Clone with companion repos

The `parts/` directory is **not** in this repo and the app will refuse
to start without it. Translations are also a sibling repo when
contributing translation work.

```bash
# from the parent directory of fritzing-app
git clone https://github.com/fritzing/fritzing-app.git
git clone https://github.com/fritzing/fritzing-parts.git fritzing-app/parts
```

### 3.3 Build (qmake + make, out-of-source)

```bash
cd fritzing-app
mkdir -p build && cd build
qmake ../phoenix.pro CONFIG+=debug    # or CONFIG+=release
make -j"$(nproc)"
```

Binaries land in `../debug64/` or `../release64/` (see DESTDIR rules in
[phoenix.pro](phoenix.pro#L72-L97)). Run with:

```bash
./Fritzing            # from the DESTDIR; needs ../parts/ resolvable
```

### 3.4 Containerized build (zero host setup)

```bash
./docker/build-linux.sh    # see docker/build-linux.sh
```

Uses the `fritzing/build:bionic` image — guaranteed-reproducible.

### 3.5 Tests

```bash
cd tests
qmake tests.pro && make -j"$(nproc)"
# then run each test_* binary under tests/auto/<name>/
```

Test framework is **QtTest**; current coverage is small
([tests/auto/test_svg](tests/auto/test_svg/),
[tests/auto/test_textutils](tests/auto/test_textutils/)). New utility
code should ship with a QtTest addition under `tests/auto/`.

### 3.6 Pre-flight checklist before any code change

Agents must verify all of the following before claiming a build works:

1. `qmake --version` reports Qt 5.9+ (5.12+ preferred).
2. `parts/` exists as a sibling/subdir and contains `core/`, `bins/`, etc.
3. Clean qmake + make completes with **zero new warnings** in changed files.
4. `tests/` builds; relevant test binaries pass.
5. `git status` shows only intended files (no `Makefile`, `*.o`,
   `moc_*.cpp`, `ui_*.h`, `qrc_*.cpp` — these are generated; see
   `.gitignore`).

---

## 4. Architecture (the "why" — read before editing)

- **`FApplication`** ([src/fapplication.cpp](src/fapplication.cpp)) is the
  `QApplication` subclass. It owns startup, command-line parsing, the
  splash screen, the reference model load, and the service (headless)
  mode used by the gerber/export CLI.
- **`MainWindow`** ([src/mainwindow/](src/mainwindow/)) hosts three
  `SketchWidget`s — Breadboard, Schematic, PCB — each a
  `QGraphicsView` over a shared sketch model. Edits flow through
  **`QUndoCommand`** subclasses defined in
  [src/commands.h](src/commands.h). **Always** mutate sketch state via
  a command; never mutate items directly from UI handlers.
- **`ReferenceModel`** ([src/referencemodel/](src/referencemodel/)) is
  the loaded parts library; **`ModelBase` / `ModelPart`**
  ([src/model/](src/model/)) is the per-sketch document model.
- **`ItemBase`** and subclasses ([src/items/](src/items/)) are the
  on-canvas graphics items; **`Connector` / `ConnectorItem`**
  ([src/connectors/](src/connectors/)) carry the electrical topology
  and the rats-nest / DRC graph.
- **SVG pipeline** ([src/svg/](src/svg/)) normalizes part SVGs, renders
  them with `FSvgRenderer` ([src/fsvgrenderer.h](src/fsvgrenderer.h)),
  and is the basis for both display and Gerber/PDF export.
- **Logging**: do not use `qDebug()` directly in app code. Use
  `DebugDialog::debug(...)` ([src/debugdialog.h](src/debugdialog.h#L41-L57))
  with an explicit `DebugLevel`.

---

## 5. Code Style & Conventions

These are **non-negotiable** because they match thousands of existing
files. Match the file you are editing; do not reformat existing code.

### 5.1 Formatting

- **Indentation: hard tabs** (verify with `cat -A`). Existing files use
  tabs; editors must be configured accordingly. Do not introduce spaces.
- Brace style: K&R / Qt — opening brace on same line for functions in
  most files, but match the surrounding file.
- Line endings: LF.
- Encoding: UTF-8, no BOM.
- One class per header/source pair, named lowercase
  (e.g. `partsbinpalettewidget.cpp`).

### 5.2 Required file header (every new `.cpp` / `.h`)

Copy verbatim, updating the year. Missing or altered headers will be
rejected upstream:

```cpp
/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-<YEAR> Fritzing

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
```

Header guards use `#ifndef <NAME>_H / #define / #endif` — **not**
`#pragma once` (matches existing convention).

### 5.3 Naming

- Classes: `UpperCamelCase` (Qt-flavored — `SketchWidget`, `ItemBase`).
- Methods / members: `lowerCamelCase`. Members are typically prefixed
  `m_` *only* in newer files — match the surrounding file.
- Enums: `UpperCamelCase` for both the type and the values
  (e.g. `DebugLevel::Warning`).
- Qt signals/slots: `slotFooBar()` is rare; prefer descriptive verb
  names (`partsBinSelected`).

### 5.4 Qt-specific rules

- Every `QObject` subclass needs `Q_OBJECT` and parent-pointer
  ownership. Do **not** mix `std::unique_ptr` with `QObject` ownership
  unless the parent is `nullptr`.
- Use `QString`, `QList`, `QHash` over std equivalents in code that
  touches Qt APIs (the rest of the codebase does).
- New widgets must add `.ui` files to the matching `pri/*.pri` if used
  via Designer; otherwise build them in code.
- Translations: every user-visible string must be wrapped in `tr(...)`
  (or `QObject::tr` / `QCoreApplication::translate`). Update
  [translations/](translations/) only via `lupdate`/`lrelease`; do not
  hand-edit `.qm`.

### 5.5 Annotation policy (mandatory for new/modified code)

The user’s rule for this fork: **every contributed change must be
annotated thoroughly enough for a junior developer to follow it
without external context.** Concretely:

1. **Header comments on every new public method**: a short Doxygen-style
   block describing intent, inputs, outputs, side-effects, and
   threading expectations. Example:
   ```cpp
   /**
    * @brief Converts a connector's scene-space position into the
    *        sketch's logical grid coordinates.
    * @param connector The ConnectorItem to query; must not be null.
    * @param snap When true, the result is rounded to the active grid.
    * @return Logical grid coordinates in millimeters.
    * @note Caller must hold the sketch's edit lock.
    */
   QPointF connectorToGrid(ConnectorItem * connector, bool snap);
   ```
2. **Inline comments** at every non-obvious branch, every `QUndoCommand`
   redo/undo pair, and every place that interacts with the SVG
   normalizer or the autorouter graph.
3. **`// TODO(<github-handle>): ...`** for deferred work — never leave
   anonymous TODOs.
4. **`// NOTE:`** for any workaround, Qt version quirk, or platform
   `#ifdef` reasoning.
5. Do **not** add comments restating what the code obviously does. The
   bar is "would a new contributor need this to avoid a footgun?".
6. Do not retro-document files you did not otherwise change (keeps PR
   diffs reviewable).

### 5.6 Undo/Redo

All sketch mutations go through a `QUndoCommand` subclass in
[src/commands.h](src/commands.h) pushed onto the
`WaitPushUndoStack` ([src/waitpushundostack.h](src/waitpushundostack.h)).
If you add a new mutation, add a paired command and write its
`undo()` to be exactly inverse — review will reject asymmetric pairs.

---

## 6. Build / qmake Module Wiring

To add a new source file:

1. Drop it under the correct `src/<area>/`.
2. Append it to the matching `pri/<area>.pri` under both `HEADERS +=`
   and `SOURCES +=` (alphabetical within the list).
3. Re-run `qmake` (incremental `make` will not re-scan `.pri` changes).

To add a brand-new area, create `pri/<area>.pri` *and* add an
`include(pri/<area>.pri)` line at the bottom of [phoenix.pro](phoenix.pro).

---

## 7. Git / PR Workflow

- Branch from and PR into **`develop`**. `master` only receives release
  merges.
- Branch naming: `feature/<short-desc>`, `fix/<issue-#>-<short>`,
  `refactor/<area>`.
- Commits: imperative mood, present tense
  (`Add gerber export DPI option`, not `Added`/`Adds`).
- Reference the issue in the body: `Fixes #1234`.
- Keep PRs focused; mechanical reformat must be its own PR.
- Do **not** commit generated files: `Makefile`, `moc_*`, `ui_*.h`,
  `qrc_*.cpp`, `*.o`, IDE folders. The repo currently has stray
  `ui_*.h` / `Makefile` artifacts at the root — **do not add more**;
  do not delete the existing ones in unrelated PRs.
- Sign-off / DCO is not currently required, but include a meaningful
  PR description per [.github/ISSUE_TEMPLATE/](.github/ISSUE_TEMPLATE/).

---

## 8. Security & Safety Rules for Agents

- **Never** modify files under `parts/` from this repo — that is the
  separate `fritzing-parts` repository.
- **Never** edit `translations/*.qm` directly (binary, generated by
  `lrelease`).
- **Never** commit anything under `release64/`, `debug64/`, `build/`,
  or generated `moc_*` / `ui_*.h` / `qrc_*.cpp` files.
- **Never** edit `LICENSE.*` files.
- File / network IO from new code goes through `FolderUtils` / Qt’s
  `QStandardPaths`; do not hard-code paths.
- This is a desktop app handling user-supplied SVG, ZIP (`.fzz`,
  `.fzpz`), and XML (`.fzp`). Treat all of it as untrusted input:
  - Use `QuaZip` for archive reads (already wrapped, see
    [pri/quazip.pri](pri/quazip.pri)); do not unzip into arbitrary
    paths (zip-slip).
  - XML must be parsed with `QXmlStreamReader`; do not enable external
    entity expansion.
  - SVG inputs flow through the normalizer in [src/svg/](src/svg/);
    do not bypass it.
- Networking (update check, parts download): only via the existing
  `version/` and `mainwindow/` paths — do not introduce new outbound
  endpoints without discussion.

---

## 9. Audit Notes (state of the tree at time of writing)

These are observations, not action items — useful context for any
agent picking up work:

- **Stray generated files at repo root**: `ui_consolesettings.h`,
  `ui_consolewindow.h`, `ui_modfiledialog.h` are committed but are
  qmake-generated artifacts. They should normally be regenerated;
  leave them alone unless explicitly cleaning them up in a dedicated
  PR.
- **Travis badges in [README.md](README.md)** point at a no-longer-active
  `travis-ci.org` instance. CI is effectively the Docker scripts in
  [docker/](docker/).
- **Default branch on GitHub is `develop`**, but the local clone is on
  `master`. Always rebase work onto `develop` before opening a PR.
- **Tests are sparse** — only `test_svg` and `test_textutils`. New
  pure-utility code should add coverage; UI/graphics code historically
  is not unit-tested here.
- **Qt 6 is not supported.** Do not introduce Qt 6-only APIs.
- **C++17/20** features may compile on some toolchains but the
  declared standard is C++14 ([phoenix.pro](phoenix.pro#L31)). Stay in
  C++14 unless coordinating a project-wide bump.
- **No `.clang-format` / `.editorconfig`** is checked in. Style is
  enforced by reviewer eyeballs and the rules in §5 above.
- **No `CONTRIBUTING.md`** exists; this `AGENTS.md` is currently the
  most complete contributor reference in the tree.

---

## 10. Quick Command Cheatsheet

```bash
# Configure + build (Linux, debug)
mkdir -p build && cd build && qmake ../phoenix.pro CONFIG+=debug && make -j$(nproc)

# Build tests
cd tests && qmake tests.pro && make -j$(nproc)

# Run app (from DESTDIR, e.g. ../debug64/)
./Fritzing

# Containerized build (no host Qt needed)
./docker/build-linux.sh

# Update translations after string changes
lupdate phoenix.pro
# (do not commit .qm; release pipeline runs lrelease)

# Clean
make distclean && rm -rf ../debug64 ../release64
```

---

## 11. When in Doubt

1. Read the analogous existing file in the same `src/<area>/`.
2. Read the matching `pri/<area>.pri` to understand its surface area.
3. Check `git log -p -- <file>` for the rationale of the last change.
4. Open an issue at <https://github.com/fritzing/fritzing-app/issues>
   or ask on <https://forum.fritzing.org> before large refactors.

End of AGENTS.md — keep this file current as conventions evolve.
