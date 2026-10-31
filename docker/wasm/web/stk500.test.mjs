// Copyright (c) 2026 Fritzing GmbH
//
// Tests for the HEX parser and the STK500v1 uploader in fritzing-web.js, against a simulated
// Optiboot bootloader. Run with: node --test docker/wasm/web/stk500.test.mjs

import { test } from 'node:test';
import assert from 'node:assert/strict';

await import('./fritzing-web.js');
const { parseIntelHex, flashStk500 } = globalThis.FritzingWeb;

// Builds an Intel HEX record
function record(type, address, data) {
	const bytes = [data.length, (address >> 8) & 0xff, address & 0xff, type, ...data];
	const checksum = (256 - (bytes.reduce((sum, b) => sum + b, 0) % 256)) % 256;
	return ':' + [...bytes, checksum].map((b) => b.toString(16).padStart(2, '0').toUpperCase()).join('');
}

function toHex(image) {
	const lines = [];
	for (let i = 0; i < image.length; i += 16) lines.push(record(0, i, Array.from(image.subarray(i, i + 16))));
	lines.push(':00000001FF');
	return lines.join('\n') + '\n';
}

// Behaves like Optiboot on an ATmega328P: answers only after a DTR reset, and ignores
// the first few sync requests while it starts
class FakeBootloader {
	constructor({ signature = [0x1e, 0x95, 0x0f], ignoreSyncs = 2, alive = true } = {}) {
		this.flash = new Uint8Array(32768).fill(0xff);
		this.signature = signature;
		this.ignoreSyncs = ignoreSyncs;
		this.alive = alive;
		this.running = false;
		this.address = 0;
		this.buffer = [];
		this.commands = [];
		this.dtr = true;
	}

	async setSignals(signals) {
		if (this.dtr && !signals.dataTerminalReady) this.running = this.alive;	// falling edge resets the board
		this.dtr = signals.dataTerminalReady;
	}

	drain() {
		this.buffer = [];
	}

	async read(count, timeoutMs) {
		if (this.buffer.length < count) {
			await new Promise((resolve) => setTimeout(resolve, Math.min(timeoutMs, 20)));
			throw new Error('Timed out waiting for the board');
		}
		return Uint8Array.from(this.buffer.splice(0, count));
	}

	async write(bytes) {
		if (!this.running) return;
		const cmd = bytes[0];
		assert.equal(bytes[bytes.length - 1], 0x20, 'every command ends with CRC_EOP');
		this.commands.push(cmd);
		const ok = (...data) => this.buffer.push(0x14, ...data, 0x10);
		switch (cmd) {
		case 0x30:
			if (this.ignoreSyncs > 0) {
				this.ignoreSyncs--;
				return;
			}
			ok();
			break;
		case 0x75:
			ok(...this.signature);
			break;
		case 0x50:
		case 0x51:
			ok();
			break;
		case 0x55:
			this.address = (bytes[1] | (bytes[2] << 8)) * 2;
			ok();
			break;
		case 0x64: {
			const length = (bytes[1] << 8) | bytes[2];
			assert.equal(bytes[3], 0x46, 'flash memory');
			this.flash.set(bytes.subarray(4, 4 + length), this.address);
			ok();
			break;
		}
		case 0x74: {
			const length = (bytes[1] << 8) | bytes[2];
			ok(...this.flash.subarray(this.address, this.address + length));
			break;
		}
		default:
			throw new Error('unexpected command ' + cmd);
		}
	}
}

test('parseIntelHex fills gaps and honours extended addresses', () => {
	const hex = [
		record(0, 0x0000, [1, 2, 3]),
		record(0, 0x0010, [4]),
		record(4, 0, [0x00, 0x00]),
		':00000001FF',
	].join('\n');
	const image = parseIntelHex(hex);
	assert.equal(image.length, 0x11);
	assert.deepEqual(Array.from(image.subarray(0, 4)), [1, 2, 3, 0xff]);
	assert.equal(image[0x10], 4);

	const segmented = parseIntelHex([record(2, 0, [0x00, 0x01]), record(0, 0, [9]), ':00000001FF'].join('\n'));
	assert.equal(segmented.length, 0x11);
	assert.equal(segmented[0x10], 9);
});

test('parseIntelHex rejects damaged files', () => {
	assert.throws(() => parseIntelHex(':0300000001020300\n:00000001FF'), /checksum/);
	assert.throws(() => parseIntelHex(record(0, 0, [1])), /end record/);
	assert.throws(() => parseIntelHex('hello\n:00000001FF'), /Invalid HEX/);
});

test('flashStk500 writes and verifies the whole image', async () => {
	const image = new Uint8Array(1000);
	for (let i = 0; i < image.length; i++) image[i] = (i * 7) & 0xff;
	const board = new FakeBootloader();
	const messages = [];
	await flashStk500(board, parseIntelHex(toHex(image)), {
		pageSize: 128,
		signature: [0x1e, 0x95, 0x0f],
		progress: (m) => messages.push(m),
	});
	assert.deepEqual(Array.from(board.flash.subarray(0, image.length)), Array.from(image));
	assert.equal(board.flash[image.length], 0xff);
	assert.equal(board.commands.at(-1), 0x51, 'leaves programming mode');
	assert.equal(board.commands.filter((c) => c === 0x64).length, 8, '8 pages of 128 bytes');
	assert.ok(messages.includes('Writing 100%'));
	assert.ok(messages.some((m) => m.startsWith('Verified')));
});

test('flashStk500 refuses a different microcontroller', async () => {
	const board = new FakeBootloader({ signature: [0x1e, 0x98, 0x01] });	// ATmega2560
	await assert.rejects(
		flashStk500(board, new Uint8Array(10), { signature: [0x1e, 0x95, 0x0f] }),
		/Unexpected device signature 1e 98 01/,
	);
	assert.ok(!board.commands.includes(0x64), 'nothing written');
});

test('flashStk500 reports a board that does not answer', async () => {
	const board = new FakeBootloader({ alive: false });
	await assert.rejects(flashStk500(board, new Uint8Array(10), { syncAttempts: 3 }), (e) => e.noSync === true);
});
