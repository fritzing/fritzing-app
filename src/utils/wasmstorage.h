/*******************************************************************

Part of the Fritzing project - https://fritzing.org
Copyright (c) 2026 Fritzing GmbH

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

#ifndef WASMSTORAGE_H
#define WASMSTORAGE_H

// Persistent storage for the WebAssembly build (only compiled by pri/wasm.pri).
// The home folder, which holds the user data store and the Fritzing documents
// folder (user parts, bins, autosave backups), lives in IndexedDB.
namespace WasmStorage {

// Mounts the home folder and loads its contents from IndexedDB.
// Call before the application object is created.
void mount();

// Writes changes in the home folder back to IndexedDB soon. Changes are also
// written every few seconds and when the page is hidden or closed.
void scheduleSync();

}

#endif
