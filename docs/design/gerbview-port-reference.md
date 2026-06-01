# Porting KiCad GerbView features into Fritzing's GerberPreviewDialog

> **Purpose**: this document is a deep, file-by-file reference for
> porting features from KiCad's `gerbview/` (the standalone Gerber +
> Excellon viewer that ships with KiCad) into Fritzing's much smaller
> `src/gerberpreview/` subsystem. It is written so a junior contributor
> can pick it up, read it end-to-end, and start cutting code without
> having to first reverse-engineer KiCad.
>
> **Source under analysis**: `/home/sysadmin/Documents/git/kicad-source-mirror/gerbview/`
> at the version cloned on this machine (KiCad master branch, May
> 2026).
>
> **Target**: `src/gerberpreview/` in this repo. The currently shipped
> classes are:
>
> - `GerberPreviewWidget` — embeddable QWidget rendering layers in mm.
> - `GerberPreviewDialog` — standalone resizable QDialog (added 2026-05-30).
> - `GerberParser`, `ExcellonParser`, `GerberAperture`, `GerberDocument`,
>   `GerberRenderer` — clean-room parsers/renderers. No gerbv linkage.
>
> **License constraint**: KiCad is **GPL-3.0-or-later**, Fritzing is
> **GPL-3.0-or-later**. Direct code reuse is *license-compatible* but
> any copy-pasted block must carry a KiCad copyright line in addition
> to the Fritzing header. **When in doubt: re-implement rather than
> copy**, both for cleanliness and because KiCad uses wxWidgets which
> would have to be ported to Qt 5 anyway.

---

## 0. Why we are doing this

The user request driving the port:

> "this wizard needs all the features of say gerbv or the JLCfab
> panelizer/kicad gerber viewer. We are missing tons of options.
> (layers, colors for layers, flipping boards 90/180, mods, etc)
> I just suggested surface level addition while gerbv and kicad
> gerber viewers are intense, I'd like to match their intensity."

KiCad's gerbview is the most feature-rich open-source Gerber viewer
in active development. It ships with KiCad and is what most KiCad
users reach for when verifying fab artifacts. Matching its surface
gives Fritzing users the same "I know exactly what is going to the
fab house" confidence.

We are **not** porting KiCad's render engine. Our `GerberRenderer`
already paints to a `QPainter`; KiCad uses an OpenGL-accelerated
"GAL" (Graphics Abstraction Layer) backend which is a much bigger
piece of machinery than we need. We are porting **UX features**
that sit on top of any rendering engine.

---

## 1. KiCad gerbview file map (~30k LoC)

The `gerbview/` tree breaks down as follows:

### 1.1 Top-level entry / frame (~5k LoC)

| KiCad file | Lines | Fritzing analogue | Port priority |
|---|---|---|---|
| `gerbview.cpp` / `.h` | small | `main.cpp` (not needed; Fritzing has one entry) | N/A |
| `gerbview_frame.cpp` / `.h` | **~3k** | `GerberPreviewDialog` | reference only; we are not building a full MDI frame |
| `gerbview_id.h` | small | (per-action ints; Qt uses signals/slots so unused) | N/A |
| `menubar.cpp` | ~500 | `GerberPreviewDialog::buildUi()` toolbar | port menu→toolbar mappings |
| `toolbars_gerber.cpp` / `.h` | ~700 | `GerberPreviewDialog` toolbar | port action layout |
| `events_called_functions.cpp` | small | (event routing; Qt handles via signals) | N/A |

### 1.2 File loading (~4k LoC)

| KiCad file | Lines | Fritzing analogue | Port priority |
|---|---|---|---|
| `files.cpp` | ~700 | `GerberPreviewDialog::onOpen()` / `openDirectory()` | already covers basic open; **port** the "load N files at once with progress" UX |
| `readgerb.cpp` | ~400 | `GerberParser::parse()` | **already done** clean-room |
| `rs274d.cpp` | **~1.2k** | `GerberParser` D-code dispatch | **already done**; cross-check against KiCad for unhandled commands |
| `rs274x.cpp` | **~1.7k** | `GerberParser` X commands | **already done**; cross-check for AB / TF / TD attribute blocks |
| `rs274_read_XY_and_IJ_coordinates.cpp` | ~200 | `GerberParser` coord parsing | already done; **double-check** trailing-zero suppression |
| `excellon_read_drill_file.cpp` | ~600 | `ExcellonParser::parse()` | already done; **cross-check** routed-slot G85 handling |
| `job_file_reader.cpp` | ~400 | (none yet) | **PORT TARGET**: read `.gbrjob` and use it to auto-classify layers more reliably than filename suffix |

### 1.3 Aperture macros and DCodes (~3k LoC)

| KiCad file | Lines | Fritzing analogue | Port priority |
|---|---|---|---|
| `aperture_macro.cpp` / `.h` | ~700 | `GerberAperture` | partial; **port** missing primitives (Polygon, Thermal, Outline) |
| `am_param.cpp` / `.h` | ~400 | (inline in `GerberAperture`) | reference for variable substitution rules |
| `am_primitive.cpp` / `.h` | **~1.2k** | `GerberAperture::stampMacro*()` | **port** all 7 standard primitives + custom outline |
| `dcode.cpp` / `.h` | ~400 | `GerberAperture` constructors | already covered; cross-check |
| `X2_gerber_attributes.cpp` / `.h` | ~300 | (none) | **PORT TARGET**: parse `%TF`, `%TA`, `%TO`, `%TD` so the viewer can show "this trace is on net VCC" tooltips |

### 1.4 Drawing model (~4k LoC)

| KiCad file | Lines | Fritzing analogue | Port priority |
|---|---|---|---|
| `gbr_layout.cpp` / `.h` | ~600 | `GerberDocument` | already covered |
| `gerber_file_image.cpp` / `.h` | **~1.8k** | `GerberDocument` (per-file image) | **port** the "negative layer" handling — we render everything additive today |
| `gerber_file_image_list.cpp` / `.h` | ~400 | `QVector<GerberLayer>` inside `GerberPreviewWidget` | reference; consider extracting into its own class once we have >10 layers |
| `gerber_draw_item.cpp` / `.h` | **~1.2k** | (none — we paint directly from parsed primitives) | reference for hit-testing implementation (needed for measure tool) |

### 1.5 Rendering (~5k LoC) — **DO NOT PORT WHOLESALE**

| KiCad file | Lines | Fritzing analogue | Port priority |
|---|---|---|---|
| `gerbview_painter.cpp` / `.h` | **~1.5k** | `GerberRenderer::paint*()` | already done in `QPainter`; reference for negative-layer compositing trick |
| `gerbview_draw_panel_gal.cpp` / `.h` | **~1.4k** | (none — would require porting KiCad's GAL) | **skip**; massive engineering effort, marginal benefit |
| `gerber_to_png.cpp` / `.h` | ~300 | (none) | **PORT TARGET**: "Save view as PNG" toolbar action |
| `gerber_to_polyset.cpp` / `.h` | ~600 | (none) | reference only; used by export-to-PCBNew which we won't ship |
| `clear_gbr_drawlayers.cpp` | small | (handled in `GerberPreviewWidget::loadFiles`) | N/A |
| `gerber_collectors.cpp` / `.h` | ~400 | (none) | reference for selection/hit-test |

### 1.6 Layer manager / colors (~2k LoC) — **HIGH ROI**

| KiCad file | Lines | Fritzing analogue | Port priority |
|---|---|---|---|
| `widgets/layer_widget.cpp` / `.h` | **~1.2k** | the row of `QCheckBox` in `GerberPreviewDialog` | **TOP PRIORITY PORT**: a real layer manager with per-layer color picker, opacity slider, draw-order drag-reorder |
| `widgets/gerbview_layer_widget.cpp` / `.h` | ~400 | (none) | sits on top of `layer_widget`; port shape unchanged |
| `widgets/gbr_layer_box_selector.cpp` / `.h` | ~200 | (none) | reference; useful for the future "Map Gerber→PCB layer" dialog |
| `widgets/dcode_selection_box.cpp` / `.h` | ~200 | (none) | **PORT TARGET (Phase 3)**: dropdown that lets the user highlight every primitive drawn with one D-code |
| `dialogs/panel_gerbview_color_settings.cpp` / `.h` | ~300 | (none — colors hard-coded) | **PORT TARGET**: settings-page-style color editor |
| `dialogs/panel_gerbview_display_options.cpp` / `.h` | ~400 | (none) | **PORT TARGET**: "Show pad outlines only", "Negative on/off", "Polar coords readout" |
| `dialogs/panel_gerbview_excellon_settings.cpp` / `.h` | ~300 | (none — we autodetect format) | reference; only needed if we hit a drill file we can't parse |

### 1.7 Tools (~2k LoC) — **MEDIUM ROI**

| KiCad file | Lines | Fritzing analogue | Port priority |
|---|---|---|---|
| `tools/gerbview_actions.cpp` / `.h` | ~600 | (action enums — Qt uses `QAction` objects) | reference for canonical action list |
| `tools/gerbview_control.cpp` / `.h` | ~500 | (none) | reference for view-transform actions (rotate / flip H / flip V) |
| `tools/gerbview_inspection_tool.cpp` / `.h` | ~400 | (none) | **PORT TARGET (Phase 3)**: measure tool, DCode info popup |
| `tools/gerbview_selection.cpp` / `.h` | ~200 | (none) | reference for hit-test interaction |
| `tools/gerbview_selection_tool.cpp` / `.h` | ~300 | (none) | reference for hit-test interaction |

### 1.8 Dialogs (~3k LoC)

| KiCad file | Lines | Fritzing analogue | Port priority |
|---|---|---|---|
| `dialogs/dialog_draw_layers_settings.cpp` / `.h` | ~400 | (none) | reference; advanced |
| `dialogs/dialog_map_gerber_layers_to_pcb.cpp` / `.h` | ~600 | (none) | **SKIP**: we don't have an export-to-Fritzing-PCB workflow |
| `dialogs/dialog_print_gerbview.cpp` | ~300 | (none) | **PORT TARGET (Phase 4)**: "Print preview" of any layer subset |
| `dialogs/dialog_select_one_pcb_layer.cpp` | ~150 | (none) | reference only |

### 1.9 Misc

| KiCad file | Lines | Fritzing analogue | Port priority |
|---|---|---|---|
| `gerber_diff.cpp` / `.h` | ~400 | (none) | **PORT TARGET (Phase 5)**: "diff two Gerber sets" — invaluable for verifying revision changes |
| `gerber_collectors.cpp` / `.h` | ~400 | (none) | reference for selection model |
| `evaluate.cpp` | ~100 | (none) | calculator-style coord eval; reference |
| `export_to_pcbnew.cpp` / `.h` | ~600 | (none) | **SKIP** |
| `gerbview_jobs_handler.cpp` / `.h` | ~300 | (none) | reference for `.gbrjob` integration |
| `gerbview_printout.cpp` / `.h` | ~200 | (none) | print support |
| `gerbview_settings.cpp` / `.h` | ~300 | (none) | reference for persistent settings layout |
| `navlib/` | ~500 | (none) | 3D mouse support; **SKIP** |

### 1.10 Test files

| `gerber_test_files/` | ~1500 | none | **WORTH COPYING** as `tests/auto/test_gerber/` fixtures for our parser unit tests |

---

## 2. Feature prioritization

Cut into shippable phases. Each phase is one PR (or a small stack)
and is independently mergeable.

### Phase 1 — Layer manager UX (~2 weeks)

Replace the current single row of `QCheckBox` in `GerberPreviewDialog`
with a real layer manager modeled on `widgets/layer_widget.cpp`:

- Per-layer row with: visibility checkbox, color swatch (clickable),
  opacity slider, layer name (editable), file source tooltip.
- Drag-reorder for draw order. Bottom of list paints first.
- Right-click "show only this", "show all", "hide all".
- Save/restore via `QSettings` (already partially done — extend the
  `preview/gerber/` group with per-layer color + opacity).

Files to create / modify:

- New: `src/gerberpreview/layermanagerwidget.{h,cpp}` (~600 lines).
- Modify: `GerberPreviewDialog` to swap the checkbox row for the
  new widget, `GerberPreviewWidget` to take per-layer color +
  opacity overrides instead of using `colorFor()`.

KiCad reference: `widgets/layer_widget.cpp` lines 1–650 cover the
core table behavior; `widgets/gerbview_layer_widget.cpp` shows the
glue between the generic layer widget and the renderer.

### Phase 2 — View transforms (rotate / flip) (~1 week)

The toolbar actions are stubbed out today. Wire them:

- `onRotate()`: 90° increments. Multiply `m_world` by a rotation
  transform around the bounds center.
- `onFlipH()` / `onFlipV()`: scale by (-1, 1) / (1, -1).
- Store the cumulative transform in `GerberPreviewWidget` so that
  `zoomToFit()` respects it.

KiCad reference: `tools/gerbview_control.cpp` actions
`mirrorVertically()`, `mirrorHorizontally()`, `rotate()`.

### Phase 3 — DCode highlight + measure tool (~2 weeks)

- DCode dropdown in toolbar (`widgets/dcode_selection_box.cpp`):
  pick a D-code, every primitive drawn with it gets a glow stroke.
- Measure tool: click two points, display distance / dX / dY in
  status row, with mm and inch units.
- Hit-test: needs per-primitive bounding boxes. Add to
  `GerberDocument::primitives()`.

KiCad reference: `widgets/dcode_selection_box.cpp`,
`tools/gerbview_inspection_tool.cpp`.

### Phase 4 — `.gbrjob` parsing + print (~1 week)

- New: `GerberJobFileReader` (`src/gerberpreview/gerberjobfile.{h,cpp}`).
  Parse the JSON `.gbrjob` next to the artifacts; use it to override
  `GerberPreviewWidget::classify()` with a much more reliable
  layer→kind mapping.
- Print preview reusing `QPrinter` + `GerberRenderer::paint()`.

KiCad reference: `job_file_reader.cpp`, `dialogs/dialog_print_gerbview.cpp`.

### Phase 5 — Diff tool + PNG export (~1 week, polish)

- "Save view as PNG..." toolbar action (`gerber_to_png.cpp`).
- "Compare with..." action: pick another directory, diff the layer
  sets, highlight added/removed primitives in red/green
  (`gerber_diff.cpp`).

---

## 3. Implementation notes

### 3.1 wxWidgets → Qt 5 translation cheat sheet

KiCad uses wxWidgets. When porting widgets, translate as follows:

| wxWidgets | Qt 5 |
|---|---|
| `wxFrame` | `QMainWindow` (we use `QDialog`) |
| `wxPanel` | `QWidget` |
| `wxSizer` (vertical/horizontal) | `QVBoxLayout` / `QHBoxLayout` |
| `wxGridSizer` | `QGridLayout` |
| `wxFlexGridSizer` | `QFormLayout` |
| `wxCheckBox::SetValue` | `QCheckBox::setChecked` |
| `wxColour` | `QColor` |
| `wxBitmap` | `QPixmap` (GUI thread only!) |
| `wxString` | `QString` (always `tr()` for user strings) |
| `wxEvent` / `Bind` | Qt signals/slots |
| `wxFileDialog` | `QFileDialog` |
| `wxConfigBase` | `QSettings` |

### 3.2 Coordinate units

KiCad gerbview internally works in **nanometers** (`int64_t`). Our
`GerberRenderer` works in **millimeters** (`double`). When porting:

- Multiply KiCad input values by `1e-6` to get mm.
- Watch for any `IU_PER_MM` constants — they are KiCad's nm→mm scale
  and have no analogue in our code.
- Drill data from `ExcellonParser` is already in mm in our pipeline.

### 3.3 License hygiene

If you keep substantial structural similarity to a KiCad file:

1. Add a comment block at the top of the new Fritzing file:
   ```cpp
   // Structure ported from KiCad's gerbview/widgets/layer_widget.cpp
   // Copyright (c) Jean-Pierre Charras / KiCad developers, GPL-3.0-or-later.
   ```
2. Keep the Fritzing GPL header above it (per `AGENTS.md` §5.2).
3. If you copy a literal block of code, list it in the commit
   message and re-confirm both projects are GPL-3.0-or-later.

If you re-implement from scratch with only the *behavior* matching,
no attribution beyond a `// NOTE(landracer): inspired by KiCad
gerbview <file>.cpp` is required.

### 3.4 Testing strategy

- Copy the smallest 5–10 files from KiCad's `gerber_test_files/` into
  `tests/auto/test_gerber/fixtures/` (small means < 2 KB each; we
  don't need their full regression corpus).
- Each fixture should have a hand-verified expected outcome
  (primitive count, bbox in mm, layer kind).
- New parser features land with a fixture; new UI features land with
  a QTest that drives the widget via `QTest::mouseClick` /
  `QTest::keyClick`.

### 3.5 Performance budget

Today's renderer is GUI-thread synchronous. For panel previews with
many small primitives (e.g. mouse-bite drill arrays × 8 boards × 9
layers) it can hitch on first paint.

- Phase 1's layer manager should preserve current first-paint time
  (\<200 ms for the 100×100 mm 8-copy panel).
- Defer any "cache primitives in a `QPicture`" optimization until
  the layer manager lands — it's much easier to add once we have
  per-layer redraw control.

---

## 4. What we will explicitly NOT port

To keep scope finite:

- **GAL / OpenGL backend** (`gerbview_draw_panel_gal.*`). We do not
  need a hardware-accelerated path for typical Fritzing panel sizes.
- **3D mouse navlib** (`navlib/`). Niche.
- **Map-Gerber-to-PCBNew dialog** (`dialog_map_gerber_layers_to_pcb*`).
  Different workflow; Fritzing doesn't import Gerber back into a sketch.
- **Plot-to-PCBNew export** (`export_to_pcbnew*`).
- **Advanced job-file editor**. We will *read* `.gbrjob`; we will not
  let the user *edit* it from the viewer.

---

## 5. Recommended assignment plan

If you have a junior to throw at this:

1. **First week**: read this document and KiCad's
   `widgets/layer_widget.cpp` (the single most important reference
   file). Produce a one-page design doc for the Fritzing
   `LayerManagerWidget`, including the signal surface, the
   `QSettings` keys, and the test plan. Land that doc as
   `docs/design/layer-manager.md` before touching code.
2. **Weeks 2–3**: implement Phase 1 against that design doc.
3. **Week 4**: ship Phase 2 (view transforms) — these are small and
   make the toolbar finally complete.
4. **Weeks 5–6**: Phase 3 — DCode highlight + measure tool. This is
   the first place the codebase will need a real "selection model"
   inside the renderer.
5. **Weeks 7+**: Phase 4 / 5 are polish; sequence by user demand.

Total estimate: ~6–8 weeks of focused work for one developer to
deliver Phases 1–3, which together close ~80% of the perceived
gap with KiCad gerbview.

---

## 6. Open questions for project lead

These are decisions that should be made before Phase 1 starts:

1. Should the standalone `GerberPreviewDialog` eventually become a
   docked panel inside `MainWindow`, or stay a top-level window?
   (KiCad uses a dedicated MDI frame; we picked dialog for v1.)
2. Should we ship a "preview" entry under `View →` so the user can
   re-open the last set without re-exporting? (Today the dialog
   already remembers `lastPaths` in `QSettings`.)
3. Do we want gerbview's "highlight items net" feature, which
   requires parsing the `%TF` / `%TA` net attributes the
   `GerberGenerator` doesn't currently emit? If yes, add to the
   `GerberGenerator` work-list first.
4. License posture: if we *do* copy substantial code from KiCad, do
   we want to flag those files in `LICENSE.GPL3` or just attribute
   per-file? (Per-file is the typical KiCad-derived-work convention.)

---

End of port reference. Update this file as ports land; it should
always reflect the current best understanding of what is left to do
and what KiCad's source is doing in the area we are about to touch.
