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

#include "wasmstorage.h"

#include <emscripten.h>

// Only one FS.syncfs() may run at a time; a request made while one is running
// starts another one when it finishes.
EM_ASYNC_JS(void, wasmStorageMount, (), {
	const home = '/home/web_user';
	FS.mkdirTree(home);
	FS.mount(IDBFS, {}, home);
	await new Promise((resolve) => FS.syncfs(true, (err) => {
		if (err) console.error('Fritzing: could not load stored files', err);
		resolve();
	}));

	let running = false;
	let again = false;
	let timer = null;
	const sync = () => {
		if (running) {
			again = true;
			return;
		}
		running = true;
		FS.syncfs(false, (err) => {
			if (err) console.error('Fritzing: could not store files', err);
			running = false;
			if (again) {
				again = false;
				sync();
			}
		});
	};
	Module.fritzingStorageSync = () => {
		clearTimeout(timer);
		timer = setTimeout(sync, 500);
	};
	setInterval(sync, 5000);
	addEventListener('pagehide', sync);
	document.addEventListener('visibilitychange', () => {
		if (document.visibilityState === 'hidden') sync();
	});

	// ask the browser not to evict the stored files under storage pressure
	if (navigator.storage && navigator.storage.persist) {
		navigator.storage.persist().catch(() => {});
	}
});

EM_JS(void, wasmStorageScheduleSync, (), {
	if (Module.fritzingStorageSync) Module.fritzingStorageSync();
});

void WasmStorage::mount()
{
	wasmStorageMount();
}

void WasmStorage::scheduleSync()
{
	wasmStorageScheduleSync();
}
