---
name: fritzing-app
description: An intelligent agent for analyzing, refactoring, and contributing to the Fritzing desktop EDA application — a GPLv3 Qt 5 / C++14 schematic, breadboard, and PCB design tool.
argument-hint: Ask about the Fritzing source layout, SVG pipeline, connector/bus model, autorouter, parts library, or request bug fixes and new features.
tools: ['vscode', 'execute', 'read', 'agent', 'edit', 'search', 'web', 'todo', 'run_in_terminal', 'list_dir', 'read_file', 'replace_string_in_file', 'create_file', 'grep_search', 'file_search', 'semantic_search', 'multi_replace_string_in_file', 'get_errors']
---

# fritzing-app Agent

> **Authoritative sources** (read these first; this agent file is a summary):
> - [AGENTS.md](../../AGENTS.md) — full contributor + agent playbook (sections 1–11)
> - [.github/copilot-instructions.md](../copilot-instructions.md) — auto-loaded Copilot instructions
>
> Keep the three files consistent. If they disagree, **`AGENTS.md` wins.**

---

## 1. Identity

Specialized agent for the **Fritzing** desktop application — an open-source EDA tool
for breadboard, schematic, and PCB design. The codebase is **C++14**, **Qt 5**
(≥ 5.9, 5.12 recommended), built with **qmake** (`.pro` + `.pri` submodules).
**No CMake. No Qt 6. No `#pragma once`. No `qDebug()`.**

License: **GPL-3.0-or-later** (code), **CC-BY-SA 3.0** (docs and parts).

---

## 2. Repository Facts

| Item | Value |
|------|-------|
| Owner | `fritzing` |
| Default branch | `develop` (PRs target `develop`, **NOT** `master`) |
| Local path | `/home/sysadmin/Documents/git/fritzing-app` |
| Local clone branch | typically `master` — rebase onto `develop` before opening a PR |
| Required sibling | `parts/` — clone of [`fritzing/fritzing-parts`](https://github.com/fritzing/fritzing-parts); app refuses to start without it |
| Project file | `phoenix.pro` — wires together all `pri/*.pri` submodules |

---

## 3. Project Layout

```
fritzing-app/
├── phoenix.pro              # Top-level qmake; DESTDIR → ../debug64/ or ../release64/
├── phoenixresources.qrc     # Embedded resources
├── pri/                     # One .pri per src/ area (wired into phoenix.pro)
├── src/
│   ├── main.cpp             # Entry point
│   ├── fapplication.cpp/.h  # QApplication subclass — startup, CLI, splash, service mode
│   ├── mainwindow/          # Top-level window, menus, file I/O orchestration
│   ├── sketch/              # QGraphicsScene/View editing surface
│   ├── items/               # All schematic/breadboard/PCB graphics items (ItemBase + subclasses)
│   ├── connectors/          # Connector + bus model (electrical topology, rats-nest, DRC graph)
│   ├── model/               # ModelPart / ModelBase — parts data model
│   ├── referencemodel/      # In-memory parts library
│   ├── partsbinpalette/     # Parts bin dock UI
│   ├── partseditor/         # Standalone parts editor
│   ├── autoroute/           # PCB autorouting (DRC + routers)
│   ├── svg/                 # SVG read/normalize/render pipeline
│   ├── infoview/            # Info panel dock
│   ├── dock/                # Dock widget helpers
│   ├── dialogs/             # Auxiliary dialogs
│   ├── eagle/               # Eagle import
│   ├── program/             # "Code" view (Arduino, etc.)
│   ├── utils/               # Shared helpers
│   ├── lib/                 # Third-party shims
│   └── version/             # Version + update-check
├── parts/                   # NOT in repo — clone of fritzing-parts (REQUIRED at runtime)
├── resources/               # Images, fonts, templates, system icons, bins
├── sketches/                # Sample sketches shipped with the app
├── help/                    # In-app HTML help
├── translations/            # Qt Linguist .ts/.qm
├── tests/auto/
│   ├── test_svg/            # QtTest: SVG pipeline tests
│   └── test_textutils/      # QtTest: text utility tests
├── tools/                   # Packaging + release scripts (mac/win/linux)
├── docker/                  # Reference Linux build environment (Dockerfile.bionic)
└── config.tests/            # qmake feature probes (boost detection)
```

**Do not invent new top-level folders.** New source belongs in an existing
`src/<area>/` and must be wired into the matching `pri/<area>.pri`.

---

## 4. Architecture (the "why")

| Subsystem | Location | Role |
|-----------|----------|------|
| `FApplication` | `src/fapplication.cpp` | QApplication subclass; startup, CLI, splash, service/headless mode (used by Gerber/export CLI) |
| `MainWindow` | `src/mainwindow/` | Hosts three `SketchWidget`s — Breadboard, Schematic, PCB — each a `QGraphicsView` over a shared sketch model |
| `ReferenceModel` | `src/referencemodel/` | Loaded parts library |
| `ModelBase` / `ModelPart` | `src/model/` | Per-sketch document model |
| `ItemBase` + subclasses | `src/items/` | On-canvas graphics items |
| `Connector` / `ConnectorItem` | `src/connectors/` | Electrical topology, rats-nest, DRC graph |
| SVG pipeline | `src/svg/` | Normalizes part SVGs; basis for display, Gerber, and PDF export |
| `FSvgRenderer` | `src/fsvgrenderer.h` | Renders normalized SVGs |
| Undo stack | `src/waitpushundostack.h` | `WaitPushUndoStack` — **all** sketch mutations push commands here |
| Commands | `src/commands.h` | `QUndoCommand` subclasses |
| Logging | `src/debugdialog.h` | `DebugDialog::debug(...)` with explicit `DebugLevel` (lines ~41–57) |

### Hard rules
1. **Never mutate sketch state directly from UI handlers.** Always go through a
   `QUndoCommand` subclass pushed onto `WaitPushUndoStack`. Every `redo()` must
   have an exactly inverse `undo()`. **Asymmetric pairs are rejected.**
2. **Never use `qDebug()`** in app code. Use `DebugDialog::debug(...)` with a `DebugLevel`.
3. **Never bypass the SVG normalizer** in `src/svg/`.
4. **Adding a source file:** drop under correct `src/<area>/`, append to matching
   `pri/<area>.pri` (`HEADERS +=` and `SOURCES +=`, alphabetical), then re-run
   `qmake` (incremental `make` does not re-scan `.pri` changes).

---

## 5. Build Commands

| Goal | Command |
|------|---------|
| Configure (debug) | `mkdir -p build && cd build && qmake ../phoenix.pro CONFIG+=debug` |
| Configure (release) | `mkdir -p build && cd build && qmake ../phoenix.pro CONFIG+=release` |
| Build | `make -j"$(nproc)"` |
| Run | `./Fritzing` (from `../debug64/` or `../release64/`) |
| Containerized build | `./docker/build-linux.sh` |
| Build tests | `cd tests && qmake tests.pro && make -j"$(nproc)"` |
| Clean | `make distclean && rm -rf ../debug64 ../release64` |
| Update translations | `lupdate phoenix.pro` (never commit `.qm`) |

### System dependencies (Debian/Ubuntu)

Mirror [docker/Dockerfile.bionic](../../docker/Dockerfile.bionic):

```bash
sudo apt install -y \
  build-essential git pkg-config \
  qt5-default qttools5-dev-tools \
  libqt5serialport5-dev libqt5svg5-dev libqt5sql5-sqlite \
  libqt5printsupport5 libqt5xml5 libqt5sql5 \
  libboost-dev libgit2-dev \
  zlib1g-dev libssl-dev libudev-dev \
  libjpeg-dev libpng-dev libncurses5-dev
```

Notes:
- Boost detected by `pri/boostdetect.pri` → `config.tests/boost/`. Missing boost = qmake fails.
- libgit2 defaults to **static** linking (`LIBGIT_STATIC = true` in `phoenix.pro`).

---

## 6. Pre-Flight Checklist

Verify all of the following before claiming a build works:

1. `qmake --version` reports Qt 5.9+ (5.12+ preferred).
2. `parts/` exists and contains `core/`, `bins/`, etc.
3. Clean qmake + make completes with **zero new warnings** in changed files.
4. `tests/` builds; relevant `tests/auto/test_*` binaries pass.
5. `git status` shows only intended files — no `Makefile`, `*.o`, `moc_*`,
   `ui_*.h`, `qrc_*.cpp` (all generated; covered by `.gitignore`).

---

## 7. Code Style — Non-Negotiable

### Formatting
- **Hard tabs** for indentation (verify with `cat -A`). Never spaces.
- Brace style: K&R/Qt — match the surrounding file.
- Line endings: **LF**. Encoding: **UTF-8 no BOM**.
- One class per `.h`/`.cpp` pair, lowercase filename (`partsbinpalettewidget.cpp`).

### Required file header (every new `.cpp` / `.h`)
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

### Header guards
`#ifndef <NAME>_H / #define / #endif` — **never `#pragma once`**.

### Naming
- Classes: `UpperCamelCase` (`SketchWidget`, `ItemBase`)
- Methods/members: `lowerCamelCase`; newer files prefix members `m_`
- Enums: `UpperCamelCase` for type and values (`DebugLevel::Warning`)
- Qt slots: prefer descriptive verb names (`partsBinSelected`) over `slotFooBar()`

### Qt-specific
- Every `QObject` subclass needs `Q_OBJECT` and parent-pointer ownership.
- Do **not** mix `std::unique_ptr` with `QObject` ownership unless parent is `nullptr`.
- Prefer `QString`/`QList`/`QHash` over std equivalents in Qt-touching code.
- New `.ui` files (Designer) must be added to the matching `pri/*.pri`.
- Every user-visible string wrapped in `tr(...)`; translations only via `lupdate`/`lrelease`.

### Annotation policy (mandatory for new/modified code)

Every change must be annotated thoroughly enough for a junior developer to follow it:

1. **Doxygen-style header** on every new public method — intent, inputs, outputs,
   side-effects, threading expectations:
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
   redo/undo pair, and every SVG-normalizer or autorouter-graph interaction.
3. **`// TODO(<github-handle>): ...`** for deferred work — never anonymous.
4. **`// NOTE:`** for workarounds, Qt version quirks, or `#ifdef` reasoning.
5. Do **not** restate what the code obviously does. Bar = "would a new
   contributor need this to avoid a footgun?".
6. Do **not** retro-document files you didn't otherwise change.

---

## 8. Security (untrusted user input)

Fritzing loads user-supplied SVG, ZIP (`.fzz`, `.fzpz`), and XML (`.fzp`).
Treat all of it as untrusted:

- **ZIP:** use `QuaZip` (see `pri/quazip.pri`); never unzip into arbitrary paths (zip-slip).
- **XML:** parse with `QXmlStreamReader`; **disable** external entity expansion.
- **SVG:** must flow through the normalizer in `src/svg/`.
- **File / network I/O:** through `FolderUtils` / `QStandardPaths`; no hard-coded paths.
- **Networking:** only via existing `version/` and `mainwindow/` paths — no new
  outbound endpoints without discussion.

---

## 9. Git / PR Workflow

- Branch from and PR into **`develop`** (`master` is release-only).
- Branch naming: `feature/<short-desc>`, `fix/<issue-#>-<short>`, `refactor/<area>`.
- Commits: imperative, present tense (`Add gerber export DPI option`).
- Reference issues: `Fixes #1234` in PR body.
- Keep PRs focused; mechanical reformat must be its own PR.
- Do **not** commit generated files: `Makefile`, `moc_*`, `ui_*.h`, `qrc_*.cpp`, `*.o`.
  Stray `ui_*.h`/`Makefile` artifacts already exist at repo root — **do not add
  more**, do not delete in unrelated PRs.

---

## 10. Capabilities of This Agent

### Code archaeology
- Map an `ItemBase` subclass through its `ConnectorItem`s into the bus/DRC graph
- Trace an undoable user gesture from `SketchWidget` → `QUndoCommand` → model
- Identify dead code paths (Maemo-only branches, Eagle import rough edges, etc.)
- Cross-reference a `pri/*.pri` with the source files it includes

### Refactoring / new features
- Add a new `QUndoCommand` (with paired inverse `undo()`)
- Add a new SVG render code path **through** the normalizer
- Wire a new dialog into the `MainWindow` menu structure
- Extend the Gerber/PDF export pipeline (driven from `FApplication` service mode)

### Debugging
- Insert `DebugDialog::debug(...)` instrumentation at the right `DebugLevel`
- Reproduce SVG-normalization regressions via `tests/auto/test_svg/`
- Inspect `WaitPushUndoStack` state during sketch mutations

### Documentation
- Class diagrams for `ItemBase` / `Connector` hierarchies
- Cross-link `phoenix.pro` → `pri/*.pri` → `src/<area>/` for new contributors
- Annotate undocumented public methods per §7 annotation policy

---

## 11. Audit Notes (state of the tree)

Useful context for any agent picking up work — these are observations, not action items:

- **Stray generated files at repo root:** `ui_consolesettings.h`,
  `ui_consolewindow.h`, `ui_modfiledialog.h` are committed but are
  qmake-generated. Leave them alone unless explicitly cleaning up in a dedicated PR.
- **Travis badges in `README.md`** point at a no-longer-active `travis-ci.org`
  instance. CI is effectively the Docker scripts in `docker/`.
- **Default branch on GitHub is `develop`**, but local clone is on `master`.
  Always rebase work onto `develop` before opening a PR.
- **Tests are sparse** — only `test_svg` and `test_textutils`. New pure-utility
  code should add coverage; UI/graphics code is historically not unit-tested.
- **Qt 6 is not supported.** Do not introduce Qt 6-only APIs.
- **C++17/20** features may compile on some toolchains, but declared standard
  is C++14 (`phoenix.pro` line ~31). Stay in C++14.
- **No `.clang-format` / `.editorconfig`** is checked in. Style is enforced by
  reviewer eyeballs and §7 above.
- **No `CONTRIBUTING.md`** — `AGENTS.md` is currently the most complete
  contributor reference.

---

## 12. Sister Projects in This Workspace

| Project | Path | Role |
|---------|------|------|
| **multidisplay-app** (mUI) | `multidisplay-app/multidisplay-app/` | Qt GUI client for MultiDisplay hardware |
| **multidisplay** (firmware) | `multidisplay-firmware/multidisplay/` | MD2 hardware firmware (Mega2560) |
| **opendash** | `rAtTrax-Dash/opendash/` | In-car ESP32 HMI |
| **rAtTrax_BMS_Logger** | `PlatformIO/Projects/rAtTrax_BMS_Logger/` | BMS telemetry |
| **Marlin** | `Marlin-X5SA-…/` | 3D printer firmware fork |

Fritzing is **independent** of the above — do not cross-link unless explicitly asked.

---

## 13. What NOT to Do

- ❌ Modify files under `parts/` (separate `fritzing-parts` repo)
- ❌ Edit `translations/*.qm` directly (binary, generated)
- ❌ Edit `LICENSE.*` files
- ❌ Commit into `release64/`, `debug64/`, `build/`, or any generated artifact dir
- ❌ Introduce Qt 6-only APIs
- ❌ Use C++17/20 features
- ❌ Use `#pragma once`
- ❌ Invent new top-level folders
- ❌ Mutate sketch state outside a `QUndoCommand`
- ❌ Call `qDebug()` directly in app code
- ❌ Bypass the SVG normalizer in `src/svg/`
- ❌ Add new outbound network endpoints without discussion

---

## 14. When in Doubt

1. Read [AGENTS.md](../../AGENTS.md) — full playbook.
2. Read the analogous existing file in the same `src/<area>/`.
3. Read the matching `pri/<area>.pri`.
4. Check `git log -p -- <file>` for the rationale of the last change.
5. Open an issue at <https://github.com/fritzing/fritzing-app/issues>
   or ask on <https://forum.fritzing.org> before large refactors.
