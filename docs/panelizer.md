# Panelizer

> File → Export → for Production → **Panelize…**

The Panelizer turns a single Fritzing PCB into a multi-board **panel** —
several copies (and optionally other boards) tiled together inside one
rectangular outline with fab-ready separations — and exports it as
RS-274X Gerbers plus an Excellon drill file ready to upload to a board
house.

It is designed for the common small-shop case: "I drew one board, I want
N of them on a panel my fab can cut apart."

---

## The two windows

Panelizing happens in two steps, on purpose — the first window is tiny so
the entry point stays trivial, and the second window holds *everything*
else so you never hunt through pages of questions.

### 1. Start dialog

Asks only the two things you must decide first:

| Field | Meaning |
|-------|---------|
| **Quantity** | How many copies of the current board to place. |
| **Allow 90° rotation** | Let the packer turn boards sideways for a tighter fit. |
| **Boards on the panel** | *Just this board* (step & repeat), or *Blend in other boards* — add other `.fzz` files to mix onto the same panel. |

Click **Continue →** to open the interactive window.

### 2. Interactive window

One window, everything visible at once:

- **Left column — options**
  - **Panel size** — pick a standard fab size from the dropdown (50×50 up
    to 300×400 mm, including Eurocard formats) or type a custom
    width/height. Set the board-to-board gap and the rail/border width,
    and toggle top & bottom rails.
  - **Board separation** — *None*, *V-cut* (V-score line width + layer),
    or *Mouse bites* (tab width, holes per tab, hole diameter & pitch).
  - **Extras** — fiducials (diameter + clearance) and tooling holes.
  - **Output** — the folder the Gerbers are written to.
- **Centre — Arrange tab**
  - A drag-to-place canvas seeded from the auto-layout. Move boards
    freely; they snap to a 1 mm grid and to the draggable **alignment
    guides**. Buttons: *Rotate 90°*, *Flip*, *Auto-arrange* (re-seed from
    the packer), *Zoom to fit*, and an *Alignment guides* toggle.
- **Gerber Preview tab**
  - The composited panel rendered in fab-house colours after you
    generate.
- **Bottom bar**
  - **Generate Gerbers** runs the full pipeline, overwrites the output
    folder, and loads the result into the preview tab **without closing
    the window** — so you can rearrange and regenerate as many times as
    you like.
  - **Save & Close** finishes (enabled once you have generated at least
    once); **Cancel** discards.

A status line and a *Loading boards…* / *Generating…* dialog keep you
informed during the slow steps (the panelizer briefly opens hidden,
off-screen Fritzing windows to measure each source board's true size).

---

## How it works under the hood

The UI is a thin shell over `PanelizerEngine`
(`src/autoroute/panelizerengine.h`), which does the real work in two
phases:

1. **`layout()`** — corner-stitching bin-packing of the source boards
   into the panel. Pure geometry, no file I/O. The arrange canvas seeds
   itself from this result, and your hand placements override it
   one-for-one on Generate.
2. **`emitPanel()`** — renders the placed boards (through the standard
   SVG normalizer and `GerberGenerator`), adds the frame, separations,
   fiducials, and tooling holes, and writes the Gerber/drill set.

Everything inside the engine is in **inches**; the dialog presents
millimetres and converts at the boundary (`kMmToIn` / `kInToMm`).

### Key source files

| File | Role |
|------|------|
| `src/autoroute/panelizerstartdialog.*` | The small quantity/blend start dialog. |
| `src/autoroute/panelizerinteractivedialog.*` | The single interactive window (options + arrange + preview + generate). |
| `src/autoroute/panelizerpages/panellayouteditor.*` | The drag-to-arrange `QGraphicsView` with grid + guide snapping. |
| `src/autoroute/panelizerengine.*` | Headless layout + emit pipeline. |
| `src/gerberpreview/gerberpreviewwidget.*` | The embeddable Gerber preview. |
| `src/mainwindow/mainwindow_export.cpp` | `MainWindow::showPanelizerWizard()` chains the two dialogs. |

---

## Tips & limitations

- **Doesn't fit?** The status line tells you when the boards won't fit;
  pick a larger standard size, reduce the border/gap, or lower the
  quantity.
- **Blend mode** packs each blended board at its true measured size, but
  the arrange canvas currently draws every instance at the *primary*
  board's footprint. Positions are still correct in the exported panel;
  heterogeneous on-canvas rendering is a known TODO.
- Always open the **Gerber Preview** tab and eyeball the outline,
  separations, and silk before sending to a fab.
