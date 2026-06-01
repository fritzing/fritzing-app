# Copilot Workspace Instructions — fritzing-app

> Auto-loaded by GitHub Copilot Chat in VS Code for this repository.
> This file is the **short** version. The authoritative, full playbook
> is [AGENTS.md](../AGENTS.md) at the repo root — read it before any
> non-trivial change. The selectable agent definition lives in
> [.github/agents/fritzing-app.agent.md](agents/fritzing-app.agent.md).

---

## Project at a Glance

- **Language:** C++14 (hard requirement; see `phoenix.pro` line ~31)
- **Framework:** Qt 5 (>= 5.9, **5.12 recommended**) — **Qt 6 NOT supported**
- **Build system:** qmake (`.pro` + `pri/*.pri` includes) — **NOT CMake**
- **License:** GPL-3.0-or-later (code), CC-BY-SA 3.0 (parts/docs)
- **Default branch:** `develop` — PRs target this, **NOT `master`**
- **Companion repo:** [`fritzing/fritzing-parts`](https://github.com/fritzing/fritzing-parts) — must be cloned as `parts/` sibling; app refuses to start without it

---

## Architecture (must read before editing)

| Subsystem | Path | Role |
|-----------|------|------|
| `FApplication` | `src/fapplication.cpp` | QApplication subclass; startup, CLI, splash, headless/service mode |
| `MainWindow` | `src/mainwindow/` | Hosts three `SketchWidget`s (Breadboard, Schematic, PCB) |
| Sketch | `src/sketch/` | `QGraphicsScene`/`View` editing surface |
| Items | `src/items/` | `ItemBase` + subclasses (on-canvas graphics) |
| Connectors | `src/connectors/` | `Connector` / `ConnectorItem` — electrical topology, rats-nest, DRC graph |
| Model | `src/model/` | `ModelBase` / `ModelPart` (per-sketch document model) |
| Reference model | `src/referencemodel/` | In-memory parts library |
| Parts editor | `src/partseditor/` | Standalone parts editor |
| Autorouter | `src/autoroute/` | PCB autorouting (DRC + routers) |
| SVG pipeline | `src/svg/` | Normalize/render part SVGs; basis for display + Gerber/PDF export |
| Renderer | `src/fsvgrenderer.h` | `FSvgRenderer` |
| Undo stack | `src/waitpushundostack.h` | `WaitPushUndoStack` — **all** sketch mutations go through this |
| Commands | `src/commands.h` | `QUndoCommand` subclasses |
| Logging | `src/debugdialog.h` | `DebugDialog::debug(...)` with explicit `DebugLevel` |

### Hard architectural rules

1. **Never mutate sketch state directly from UI handlers.** Always go
   through a `QUndoCommand` subclass pushed onto `WaitPushUndoStack`.
   Each `redo()` must have an exactly inverse `undo()`. Asymmetric
   pairs are rejected in review.
2. **Never use `qDebug()` directly.** Use `DebugDialog::debug(...)`
   with an explicit `DebugLevel` (see `src/debugdialog.h`).
3. **Never bypass the SVG normalizer** in `src/svg/` — it is the
   foundation of both display and Gerber/PDF export.
4. **Adding a source file:** drop it under the correct `src/<area>/`,
   then append it to the matching `pri/<area>.pri` under both
   `HEADERS +=` and `SOURCES +=` (alphabetically). Re-run `qmake`
   afterwards (incremental `make` does not re-scan `.pri` changes).

---

## Code Style — Non-Negotiable

- **Hard tabs** for indentation (verify with `cat -A`). Never introduce spaces.
- Brace style: K&R/Qt — match the surrounding file.
- Line endings **LF**, encoding **UTF-8 no BOM**.
- One class per `.h`/`.cpp` pair, lowercase filename
  (e.g. `partsbinpalettewidget.cpp`).
- Header guards: `#ifndef <NAME>_H / #define / #endif` —
  **never `#pragma once`**.
- Naming:
  - Classes: `UpperCamelCase` (`SketchWidget`, `ItemBase`)
  - Methods/members: `lowerCamelCase`; newer files prefix members `m_`
  - Enums: `UpperCamelCase` for both type and values (`DebugLevel::Warning`)
- Qt:
  - Every `QObject` subclass needs `Q_OBJECT` and parent-pointer ownership.
  - Do **not** mix `std::unique_ptr` with `QObject` ownership unless
    the parent is `nullptr`.
  - Prefer `QString`/`QList`/`QHash` over std equivalents in Qt-touching code.
  - Every user-visible string must be wrapped in `tr(...)`.
  - Translations: only via `lupdate`/`lrelease` — never hand-edit `.qm`.
- C++14 only — do **not** use C++17/20 features unless coordinating a
  project-wide bump.

### Required file header (every new `.cpp` / `.h`)

Copy verbatim, updating year. Missing/altered headers are rejected upstream:

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

### Annotation policy (mandatory for new/modified code)

Every contributed change must be annotated thoroughly enough for a
junior developer to follow it without external context.

1. **Doxygen-style header** on every new public method — intent,
   inputs, outputs, side-effects, threading expectations:
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
2. **Inline comments** at every non-obvious branch, every
   `QUndoCommand` redo/undo pair, and every SVG-normalizer or
   autorouter-graph interaction.
3. **`// TODO(<github-handle>): ...`** for deferred work — never anonymous TODOs.
4. **`// NOTE:`** for workarounds, Qt version quirks, or `#ifdef` reasoning.
5. Do **not** restate what the code obviously does. The bar is
   "would a new contributor need this to avoid a footgun?".
6. Do **not** retro-document files you didn't otherwise change.

---

## Build / Test Quickstart

```bash
# Configure + build (out-of-source, debug)
mkdir -p build && cd build
qmake ../phoenix.pro CONFIG+=debug
make -j"$(nproc)"

# Run (binaries land in ../debug64/ or ../release64/)
cd ../debug64 && ./Fritzing

# Build tests (QtTest framework)
cd tests && qmake tests.pro && make -j"$(nproc)"
# Then run each test_* binary under tests/auto/<name>/

# Containerized build (no host Qt needed)
./docker/build-linux.sh

# Update translations after string changes
lupdate phoenix.pro     # never commit .qm files
```

Pre-flight checklist before claiming a build works:

1. `qmake --version` reports Qt 5.9+ (5.12+ preferred).
2. `parts/` exists and contains `core/`, `bins/`, etc.
3. Clean qmake + make completes with **zero new warnings** in changed files.
4. Relevant `tests/auto/test_*` binaries pass.
5. `git status` shows only intended files (no `Makefile`, `*.o`,
   `moc_*.cpp`, `ui_*.h`, `qrc_*.cpp`).

---

## Security (untrusted user input)

The app loads user-supplied SVG, ZIP (`.fzz`, `.fzpz`), and XML (`.fzp`).
Treat **all of it** as untrusted:

- **ZIP:** use `QuaZip` (see `pri/quazip.pri`); never unzip into
  arbitrary paths (zip-slip).
- **XML:** parse with `QXmlStreamReader`; do **not** enable external
  entity expansion.
- **SVG:** must flow through the normalizer in `src/svg/` — do not bypass.
- **File / network I/O:** go through `FolderUtils` / `QStandardPaths`;
  no hard-coded paths.
- **Networking:** only via existing `version/` and `mainwindow/`
  paths — no new outbound endpoints without discussion.

---

## Git / PR Workflow

- Branch from and PR into **`develop`** (`master` is release-only).
- Branch naming: `feature/<short-desc>`, `fix/<issue-#>-<short>`,
  `refactor/<area>`.
- Commits: imperative, present tense (`Add gerber export DPI option`,
  not `Added`/`Adds`).
- Reference issues: `Fixes #1234` in body.
- Keep PRs focused — mechanical reformat must be its own PR.
- Do **not** commit generated files: `Makefile`, `moc_*`, `ui_*.h`,
  `qrc_*.cpp`, `*.o`. Stray `ui_*.h`/`Makefile` artifacts already exist
  at the repo root — **do not add more**, do not delete in unrelated PRs.

---

## What NOT to Do

- ❌ Do **not** modify files under `parts/` — that is the separate
  `fritzing-parts` repo.
- ❌ Do **not** edit `translations/*.qm` directly (binary, generated).
- ❌ Do **not** edit `LICENSE.*` files.
- ❌ Do **not** commit into `release64/`, `debug64/`, `build/`, or any
  generated artifact dir.
- ❌ Do **not** introduce Qt 6-only APIs.
- ❌ Do **not** use C++17/20 features (project is C++14).
- ❌ Do **not** use `#pragma once`.
- ❌ Do **not** invent new top-level folders.
- ❌ Do **not** mutate sketch state outside a `QUndoCommand`.
- ❌ Do **not** call `qDebug()` directly in app code.

---

## When in Doubt

1. Read [AGENTS.md](../AGENTS.md) — full playbook.
2. Read the analogous existing file in the same `src/<area>/`.
3. Read the matching `pri/<area>.pri`.
4. Check `git log -p -- <file>` for the rationale of the last change.
5. Open an issue at <https://github.com/fritzing/fritzing-app/issues>
   or ask on <https://forum.fritzing.org> before large refactors.
