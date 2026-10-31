# Fritzing in the browser (Qt for WebAssembly)

`docker/wasm/Dockerfile` compiles Fritzing with Qt for WebAssembly and serves the result
as static files from nginx. `docker/wasm/compose.yaml` adds the Arduino compile service.

```sh
# everything, including Arduino upload
docker compose -f docker/wasm/compose.yaml up --build

# or only the app (everything except Arduino upload)
docker build -f docker/wasm/Dockerfile -t fritzing-wasm .
docker run --rm -p 8080:80 fritzing-wasm
```

Then open http://localhost:8080. Run the commands from the repository root (the build
context must include `.git`, which `pri/gitversion.pri` reads).

Use Chrome or Edge for serial ports and Arduino upload (Web Serial), and for choosing where
downloads are saved. Browsers only allow these on `localhost` or over HTTPS, so to use the
app from other machines, put a TLS-terminating proxy in front of it.

## How it is built

| Stage | Purpose |
|---|---|
| `sources` | Downloads boost 1.85, svgpp 1.3.1, Clipper 6.4.2, libgit2 1.7.1, QuaZip (stachenov/quazip#199), ngspice 42 and the `fritzing-parts` library. All archives are checked against SHA-256 sums. |
| `native` | Builds the regular desktop app (Qt 6.8.3 from aqtinstall) and runs `Fritzing -db` to generate `parts.db`, just like `tools/linux_release_script/release.sh`. This stage also shows that the desktop build still works. |
| `wasm` | Installs emsdk 3.1.56 and Qt 6.8.3 `wasm_singlethread`, builds Clipper, QuaZip and ngspice with Emscripten, and builds Fritzing with `pri/wasm.pri`. Translations, help, example sketches and the parts library (with `parts.db`) are packed into `Fritzing.data` and preloaded as `/fritzing`. |
| final | `nginx:alpine` serving `index.html`, `Fritzing.{js,wasm,data}`, `qtloader.js` and `web/fritzing-web.js`, with pre-gzipped copies. |

Build arguments: `QT_VERSION` (default `6.8.3`), `EMSDK_VERSION` (must match the Emscripten
version of that Qt release) and `PARTS_COMMIT`.

emsdk only ships x86_64 Linux binaries, so on arm64 hosts (Apple Silicon) the `wasm`
stage runs emulated and takes considerably longer. The `native` stage runs on the host
architecture. Linking the app needs several GB of memory; give Docker at least 10 GB.

### Updating parts

The parts library is built into the image. To update it, rebuild with a newer commit:

```sh
docker build -f docker/wasm/Dockerfile -t fritzing-wasm --build-arg PARTS_COMMIT=latest --no-cache-filter sources .
```

`latest` takes the newest commit of fritzing-parts; `parts.db` is regenerated from it. Help >
Check for updates in the app says the same.

## The compile service

`docker/wasm/compile-service` is a small HTTP service (Python standard library) around a
pinned arduino-cli with the `arduino:avr` core, which is installed when the image is built.
nginx forwards `/api/compile` to it. It only accepts the AVR boards listed in
`src/program/platformarduino.cpp`, limits request size (256 KB), time (120 s) and
concurrency (2), and in `compose.yaml` runs without network access, as a non-root user, on
a read-only file system.

It runs a compiler on code sent by anyone who can reach the app, so only expose it to
people you trust.

Tests: `docker run --rm --network none fritzing-compile python3 -m unittest -v test_server`.

## What differs from the desktop app

All source changes are behind `#ifdef Q_OS_WASM` or `QT_CONFIG(...)`, or are files only
`pri/wasm.pri` compiles; desktop builds are unchanged.

- **Files.** File pickers use the browser's file chooser: the chosen file is copied into
  the app's file system and opened from there. Saving and exporting writes the file and
  offers it as a download (Gerber and etchable exports as a zip). Saving a bin keeps it
  with your other bins.
- **Storage.** Your Fritzing folders (user parts, bins, autosave backups, opened files) are
  kept in the browser's IndexedDB and are still there after a reload; settings are kept by
  Qt in browser storage. Clearing the site data removes them.
- **Arduino.** Upload compiles the sketch with the compile service and writes it to the
  board over Web Serial. Boards with an STK500v1 bootloader can be programmed from the
  browser: Arduino UNO, Nano and Pro/Pro Mini. For the other boards the compiled `.hex`
  is downloaded instead. Choose "Choose a port..." in the port list to allow the browser to
  use a board. The serial monitor uses Web Serial too (7 or 8 data bits, no XON/XOFF).
  PICAXE upload is not available.
- **Simulator.** ngspice is compiled into the app. Simulations run in the foreground, so
  the page pauses until a simulation step is finished. XSPICE code models (`analog.cm`)
  cannot be loaded in the browser, so circuits with the "DC Power Supply" (lab power
  supply) part, whose model uses one, do not simulate.
- **Online information.** The welcome page news and the update check go through nginx
  (`/proxy/<host>/...`, only for fritzing.org, blog.fritzing.org, fab.fritzing.org and aisler.net),
  because the browser blocks requests to other sites. fritzing.org's bot protection may
  refuse the server's requests (HTTP 403), in which case the projects list stays empty and
  the update check reports that it could not get update information.
- **Long operations.** The app is single-threaded. Autorouting and DRC keep the page
  responsive (their progress dialogs and Cancel work); ground fill shows its progress
  dialog but the page pauses while the fill is calculated.
- Rendering uses the raster viewport instead of OpenGL.

## Notes on the Qt WebAssembly build

Facts about Qt 6.8.3 `wasm_singlethread` that shaped `pri/wasm.pri` and the guards:

- The `wasm-emscripten` mkspec sets `QMAKE_PLATFORM = wasm unix`, so `unix {}` scopes also
  apply to it; native-only blocks in `phoenix.pro` use `unix:!wasm` or `!wasm`.
- Disabled features: `thread`, `concurrent`, `future`, `process`, `ssl`. QtSerialPort is not
  available; `src/program/wasm/QtSerialPort` maps its includes to
  `src/program/webserialport_wasm.h`. QtPrintSupport, QtSql with the static `qsqlite` plugin,
  QtOpenGLWidgets and QLibrary are available.
- The prebuilt Qt is not configured with Asyncify. Qt detects it at runtime, so linking with
  `-sASYNCIFY` is enough to make modal `exec()` dialogs work.
- The browser only hands over file contents asynchronously, and not while a nested event
  loop waits for them, so file pickers use `FolderUtils::uploadFile()` with a callback.
- A modal dialog opened from a menu that runs its own event loop (`QMenu::exec()`) gets no
  input in the browser, so the app stops responding. Context menus and the parts bin menu
  therefore use `popup()` in the browser.
- `ProcessEventBlocker::processEvents()` briefly waits in an event loop every 50 ms, which
  suspends the app (Asyncify) and lets the browser repaint and deliver input.
- The home folder is an IDBFS mount, loaded in `main()` before the application starts
  (`src/utils/wasmstorage.cpp`) and written back every few seconds, after saves, and when
  the page is hidden.
- Fritzing relies on C++ exceptions, so everything is compiled and linked with `-fexceptions`.
