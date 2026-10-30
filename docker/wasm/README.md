# Fritzing in the browser (Qt for WebAssembly)

`docker/wasm/Dockerfile` compiles Fritzing with Qt for WebAssembly and serves the result
as static files from nginx.

```sh
docker build -f docker/wasm/Dockerfile -t fritzing-wasm .
docker run --rm -p 8080:80 fritzing-wasm
# open http://localhost:8080
```

Run the build from the repository root (the build context must include `.git`, which
`pri/gitversion.pri` reads).

## How it is built

| Stage | Purpose |
|---|---|
| `sources` | Downloads boost 1.85, svgpp 1.3.1, Clipper 6.4.2, libgit2 1.7.1, QuaZip (stachenov/quazip#199) and a pinned `fritzing-parts` commit. All archives are checked against SHA-256 sums. |
| `native` | Builds the regular desktop app (Qt 6.8.3 from aqtinstall) and runs `Fritzing -db` to generate `parts.db`, just like `tools/linux_release_script/release.sh`. This stage also shows that the desktop build still works. |
| `wasm` | Installs emsdk 3.1.56 and Qt 6.8.3 `wasm_singlethread`, builds Clipper and QuaZip with Emscripten, and builds Fritzing with `pri/wasm.pri`. Translations, help, example sketches and the parts library (with `parts.db`) are packed into `Fritzing.data` and preloaded as `/fritzing`. |
| final | `nginx:alpine` serving `index.html`, `Fritzing.{js,wasm,data}` and `qtloader.js`, with pre-gzipped copies. |

Build arguments: `QT_VERSION` (default `6.8.3`), `EMSDK_VERSION` (must match the Emscripten
version of that Qt release) and `PARTS_COMMIT`.

emsdk only ships x86_64 Linux binaries, so on arm64 hosts (Apple Silicon) the `wasm`
stage runs emulated and takes considerably longer. The `native` stage runs on the host
architecture.

## What differs from the desktop app

All source changes are behind `#ifdef Q_OS_WASM`, `QT_CONFIG(...)` or the
`FRITZING_NO_SERIAL` define set in `pri/wasm.pri`; desktop builds are unchanged.

- **Opening files** (File > Open, or Open Sketch on the welcome page) uses the browser's file
  picker. The file is copied into the in-memory file system and opened from there. Example
  sketches open as usual from File > Open Example. Other file pickers (importing images,
  opening bins, the parts editor) do nothing in the browser build: the browser only delivers
  file contents asynchronously, and never while a modal Qt event loop is waiting.
- **Saving and exporting** have no save dialog. The file is written to the in-memory file
  system and then offered as a browser download. Gerber and etchable exports download as a
  zip.
- **Not available:** uploading code to a board and the serial monitor (no QtSerialPort or
  QProcess), the ngspice simulator, parts updates (no libgit2), and the online checks at
  startup (update check, welcome-page news) because the browser blocks those cross-origin
  requests.
- **Persistence:** settings are kept by Qt in browser storage. Anything written to the file
  system (user parts, bins, autosave backups) lives in memory and is lost when the page is
  reloaded.
- Rendering uses the raster viewport instead of OpenGL.
- Long operations (ground fill, autorouting, DRC) run on the main thread, since the Qt build
  is single-threaded.

## Notes on the Qt WebAssembly build

Facts about Qt 6.8.3 `wasm_singlethread` that shaped `pri/wasm.pri` and the guards:

- The `wasm-emscripten` mkspec sets `QMAKE_PLATFORM = wasm unix`, so `unix {}` scopes also
  apply to it; native-only blocks in `phoenix.pro` use `unix:!wasm` or `!wasm`.
- Disabled features: `thread`, `concurrent`, `future`, `process`, `ssl`. QtSerialPort is not
  available. QtPrintSupport, QtSql with the static `qsqlite` plugin, QtOpenGLWidgets and
  QLibrary are available.
- The prebuilt Qt is not configured with Asyncify. Qt detects it at runtime, so linking with
  `-sASYNCIFY` is enough to make modal `exec()` dialogs work.
- Fritzing relies on C++ exceptions, so everything is compiled and linked with `-fexceptions`.
