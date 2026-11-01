// Copyright (c) 2026 Fritzing GmbH
//
// Browser services for the Fritzing web app (loaded by index.html, see docker/wasm/Dockerfile):
// serial ports through Web Serial, and uploading compiled Arduino sketches to boards with an
// STK500v1 (Optiboot) bootloader. The C++ side (src/program/webserialport_wasm.cpp) calls
// globalThis.FritzingWeb and receives events through the handlers it registers.
//
// Also loaded by stk500.test.mjs under node, so nothing here touches the DOM at load time.

(function () {
	'use strict';

	const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

	// ---------------------------------------------------------------- Intel HEX

	// Returns the program image as bytes from address 0, with gaps filled with 0xFF
	function parseIntelHex(text) {
		const chunks = [];
		let base = 0;
		let end = 0;
		let sawEof = false;
		const lines = text.split(/\r?\n/);
		for (let i = 0; i < lines.length && !sawEof; i++) {
			const line = lines[i].trim();
			if (line === '') continue;
			if (line[0] !== ':' || line.length < 11 || line.length % 2 === 0) {
				throw new Error('Invalid HEX record on line ' + (i + 1));
			}
			const bytes = [];
			for (let j = 1; j < line.length; j += 2) {
				const value = Number.parseInt(line.substr(j, 2), 16);
				if (Number.isNaN(value)) throw new Error('Invalid HEX record on line ' + (i + 1));
				bytes.push(value);
			}
			const length = bytes[0];
			if (bytes.length !== length + 5) throw new Error('Invalid HEX record length on line ' + (i + 1));
			if (bytes.reduce((sum, b) => sum + b, 0) % 256 !== 0) throw new Error('HEX checksum error on line ' + (i + 1));
			const address = (bytes[1] << 8) | bytes[2];
			const data = bytes.slice(4, 4 + length);
			switch (bytes[3]) {
			case 0x00:
				chunks.push({ address: base + address, data });
				end = Math.max(end, base + address + length);
				break;
			case 0x01:
				sawEof = true;
				break;
			case 0x02:
				base = ((data[0] << 8) | data[1]) << 4;
				break;
			case 0x04:
				base = ((data[0] << 8) | data[1]) << 16;
				break;
			default:	// start address records do not matter for flashing
				break;
			}
		}
		if (!sawEof) throw new Error('The HEX file has no end record');
		const image = new Uint8Array(end).fill(0xff);
		for (const chunk of chunks) image.set(chunk.data, chunk.address);
		return image;
	}

	// ---------------------------------------------------------------- STK500v1

	const STK = {
		OK: 0x10, INSYNC: 0x14, CRC_EOP: 0x20,
		GET_SYNC: 0x30, ENTER_PROGMODE: 0x50, LEAVE_PROGMODE: 0x51,
		LOAD_ADDRESS: 0x55, PROG_PAGE: 0x64, READ_PAGE: 0x74, READ_SIGN: 0x75,
	};

	// Boards whose bootloader speaks STK500v1, keyed by the names in PlatformArduino
	const FLASHABLE_BOARDS = {
		'Arduino UNO': { baudRates: [115200], signature: [0x1e, 0x95, 0x0f], pageSize: 128 },
		'Arduino Nano': { baudRates: [115200, 57600], signature: [0x1e, 0x95, 0x0f], pageSize: 128 },	// new and old bootloader
		'Arduino Pro/Pro Mini': { baudRates: [57600], signature: null, pageSize: 128 },	// ATmega328P or ATmega168
	};

	const hex2 = (b) => b.toString(16).padStart(2, '0');
	// details for diagnosing uploads; shown in the browser console with "Verbose" messages
	const debug = (...args) => console.debug('[Fritzing upload]', ...args);

	// transport: { write(Uint8Array), read(count, timeoutMs) -> Uint8Array, drain(), setSignals(signals) }
	async function command(transport, bytes, answerLength, timeoutMs) {
		await transport.write(Uint8Array.from([...bytes, STK.CRC_EOP]));
		const answer = await transport.read(answerLength + 2, timeoutMs || 1000);
		if (answer[0] !== STK.INSYNC || answer[answer.length - 1] !== STK.OK) {
			throw new Error('The bootloader did not acknowledge command 0x' + hex2(bytes[0]));
		}
		return answer.subarray(1, answer.length - 1);
	}

	async function resetBoard(transport) {
		// The DTR line resets the board through a capacitor; the bootloader then waits briefly
		await transport.setSignals({ dataTerminalReady: false, requestToSend: false });
		await sleep(250);
		await transport.setSignals({ dataTerminalReady: true, requestToSend: true });
		await sleep(50);
	}

	async function sync(transport, attempts) {
		for (let i = 0; i < attempts; i++) {
			transport.drain();
			try {
				await command(transport, [STK.GET_SYNC], 0, 300);
				return;
			} catch (e) {
				// the bootloader may still be starting
				debug('sync attempt ' + (i + 1) + ' of ' + attempts + ':', e.message);
			}
		}
		const error = new Error('No answer from the bootloader.');
		error.noSync = true;
		throw error;
	}

	async function loadAddress(transport, byteAddress) {
		const word = byteAddress >> 1;
		await command(transport, [STK.LOAD_ADDRESS, word & 0xff, (word >> 8) & 0xff], 0);
	}

	// Uploads image (bytes from address 0) to a board whose bootloader is waiting after reset
	async function flashStk500(transport, image, options) {
		const pageSize = options.pageSize || 128;
		const progress = options.progress || (() => {});

		debug('resetting the board');
		await resetBoard(transport);
		await sync(transport, options.syncAttempts || 10);
		debug('in sync');

		const signature = await command(transport, [STK.READ_SIGN], 3);
		progress('Device signature ' + Array.from(signature, hex2).join(' '));
		if (options.signature && options.signature.some((b, i) => b !== signature[i])) {
			throw new Error('Unexpected device signature ' + Array.from(signature, hex2).join(' ') + '. Check the board type.');
		}

		await command(transport, [STK.ENTER_PROGMODE], 0);
		const pages = Math.ceil(image.length / pageSize);
		debug('writing ' + image.length + ' bytes in ' + pages + ' pages of ' + pageSize);
		for (let page = 0; page < pages; page++) {
			const address = page * pageSize;
			const data = image.subarray(address, Math.min(address + pageSize, image.length));
			await loadAddress(transport, address);
			await command(transport, [STK.PROG_PAGE, data.length >> 8, data.length & 0xff, 0x46, ...data], 0);
			progress('Writing ' + Math.round(((page + 1) * 100) / pages) + '%');
		}
		for (let page = 0; page < pages; page++) {
			const address = page * pageSize;
			const data = image.subarray(address, Math.min(address + pageSize, image.length));
			await loadAddress(transport, address);
			const read = await command(transport, [STK.READ_PAGE, data.length >> 8, data.length & 0xff, 0x46], data.length);
			for (let i = 0; i < data.length; i++) {
				if (read[i] !== data[i]) {
					throw new Error('Verification failed at address 0x' + (address + i).toString(16));
				}
			}
		}
		progress('Verified ' + image.length + ' bytes');
		await command(transport, [STK.LEAVE_PROGMODE], 0);
		debug('done');
	}

	// ---------------------------------------------------------------- Web Serial

	// An open port. Incoming bytes go to onData if set, otherwise into a buffer for read().
	class Connection {
		constructor(port) {
			this.port = port;
			this.buffer = [];
			this.onData = null;
			this.onClosed = null;
			this.wakeUp = null;
		}

		async open(options) {
			await this.port.open(options);
			this.writer = this.port.writable.getWriter();
			this.reader = this.port.readable.getReader();
			this.readLoop = this.runReadLoop();
		}

		async runReadLoop() {
			let error = null;
			try {
				for (;;) {
					const { value, done } = await this.reader.read();
					if (done) break;
					if (this.onData) {
						this.onData(value);
					} else {
						for (const b of value) this.buffer.push(b);
						if (this.wakeUp) this.wakeUp();
					}
				}
			} catch (e) {
				error = e;
			}
			if (this.onClosed && !this.closing) this.onClosed(error ? String(error.message || error) : 'The port was closed.');
		}

		drain() {
			this.buffer = [];
		}

		async read(count, timeoutMs) {
			const deadline = Date.now() + timeoutMs;
			while (this.buffer.length < count) {
				const left = deadline - Date.now();
				if (left <= 0) throw new Error('Timed out waiting for the board');
				await new Promise((resolve) => {
					this.wakeUp = resolve;
					setTimeout(resolve, left);
				});
				this.wakeUp = null;
			}
			return Uint8Array.from(this.buffer.splice(0, count));
		}

		async write(bytes) {
			await this.writer.write(bytes);
		}

		async setSignals(signals) {
			await this.port.setSignals(signals);
		}

		async close() {
			this.closing = true;
			try {
				await this.reader.cancel();
			} catch (e) {
				// already closed
			}
			await this.readLoop;
			this.reader.releaseLock();
			this.writer.releaseLock();
			await this.port.close();
		}
	}

	const known = [];			// { port, name }
	const connections = new Map();	// name -> Connection
	const handlers = { data() {}, closed() {}, portsChanged() {} };

	function portName(port) {
		let entry = known.find((k) => k.port === port);
		if (!entry) {
			const info = port.getInfo ? port.getInfo() : {};
			const usb = info.usbVendorId !== undefined
				? ' (USB ' + info.usbVendorId.toString(16).padStart(4, '0') + ':' + (info.usbProductId || 0).toString(16).padStart(4, '0') + ')'
				: '';
			entry = { port, name: 'Serial ' + (known.length + 1) + usb };
			known.push(entry);
		}
		return entry.name;
	}

	function findPort(name) {
		const entry = known.find((k) => k.name === name);
		if (!entry) throw new Error('Unknown port ' + name);
		return entry.port;
	}

	async function refreshPorts() {
		if (!navigator.serial) return;
		const ports = await navigator.serial.getPorts();
		for (let i = known.length - 1; i >= 0; i--) {
			if (!ports.includes(known[i].port)) known.splice(i, 1);
		}
		ports.forEach(portName);
		handlers.portsChanged();
	}

	const FritzingWeb = {
		parseIntelHex,
		flashStk500,
		FLASHABLE_BOARDS,

		serialSupported() {
			return typeof navigator !== 'undefined' && !!navigator.serial;
		},

		// called once by the application with { data(name, bytes), closed(name, message), portsChanged() }
		setHandlers(newHandlers) {
			Object.assign(handlers, newHandlers);
			if (FritzingWeb.serialSupported()) {
				navigator.serial.addEventListener('connect', refreshPorts);
				navigator.serial.addEventListener('disconnect', refreshPorts);
				refreshPorts().catch((e) => console.warn('Fritzing: could not list serial ports', e));
			}
		},

		// names of the ports the user has allowed this site to use
		portNames() {
			return known.map((k) => k.name);
		},

		// must be called while handling a click; resolves to the new port's name, or null
		async requestPort() {
			if (!FritzingWeb.serialSupported()) throw new Error('This browser does not support Web Serial. Please use Chrome or Edge.');
			let port;
			try {
				port = await navigator.serial.requestPort();
			} catch (e) {
				return null;	// the user closed the chooser
			}
			const name = portName(port);
			handlers.portsChanged();
			return name;
		},

		async open(name, options) {
			if (connections.has(name)) throw new Error(name + ' is already open');
			const connection = new Connection(findPort(name));
			await connection.open(options);
			connection.onData = (bytes) => handlers.data(name, bytes);
			connection.onClosed = (message) => {
				connections.delete(name);
				handlers.closed(name, message);
			};
			connections.set(name, connection);
		},

		async write(name, bytes) {
			const connection = connections.get(name);
			if (!connection) throw new Error(name + ' is not open');
			await connection.write(bytes);
		},

		async close(name) {
			const connection = connections.get(name);
			if (!connection) return;
			connections.delete(name);
			await connection.close();
		},

		canFlash(board) {
			return Object.prototype.hasOwnProperty.call(FLASHABLE_BOARDS, board);
		},

		// uploads a compiled sketch; progress(message) reports what happens
		async flash(name, board, hexText, progress) {
			const settings = FLASHABLE_BOARDS[board];
			if (!settings) throw new Error('Uploading to ' + board + ' is not supported in the browser.');
			const image = parseIntelHex(hexText);
			await FritzingWeb.close(name);	// e.g. the serial monitor
			const port = findPort(name);
			let lastError = null;
			for (const baudRate of settings.baudRates) {
				const connection = new Connection(port);
				try {
					await connection.open({ baudRate });
				} catch (e) {
					debug('opening ' + name + ' failed:', e);
					throw new Error('Could not open ' + name + ' (' + e.message + '). Close other programs that use the port, ' +
					                'such as the Arduino IDE or another browser tab, and try again.');
				}
				try {
					progress('Connecting at ' + baudRate + ' baud');
					await flashStk500(connection, image, { pageSize: settings.pageSize, signature: settings.signature, progress });
					return;
				} catch (e) {
					if (!e.noSync) throw e;
					lastError = e;		// maybe a different bootloader speed
				} finally {
					await connection.close();
				}
			}
			lastError.message += ' Tried ' + settings.baudRates.join(' and ') + ' baud. Check that the board type matches ' +
				'the board, that ' + name + ' is the board\'s port, and that the board has an Arduino bootloader.';
			throw lastError;
		},
	};

	globalThis.FritzingWeb = FritzingWeb;
})();
