#!/usr/bin/env python3
# Copyright (c) 2026 Fritzing GmbH
#
# Compiles Arduino sketches for the Fritzing web app (docker/wasm). The browser cannot run
# a compiler, so the app sends the sketch here (through nginx at /api/compile), gets the
# .hex back and uploads it to the board over Web Serial.
#
#   POST /compile   {"fqbn": "arduino:avr:uno", "sketch": "<source of the .ino>"}
#   ->              {"ok": true|false, "log": "<compiler output>", "hex": "<Intel HEX>"|null}
#   GET  /health    -> {"ok": true}
#
# Only the AVR boards listed in src/program/platformarduino.cpp are accepted. Requests are
# limited in size, time and concurrency; the container has no network access at runtime.

import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

# Board types from PlatformArduino::PlatformArduino() that use the arduino:avr core
ALLOWED_FQBNS = frozenset([
    "arduino:avr:uno",
    "arduino:avr:yun",
    "arduino:avr:mega",
    "arduino:avr:diecimila",
    "arduino:avr:nano",
    "arduino:avr:megaADK",
    "arduino:avr:leonardo",
    "arduino:avr:micro",
    "arduino:avr:Esplora",
    "arduino:avr:mini",
    "arduino:avr:ethernet",
    "arduino:avr:fio",
    "arduino:avr:bt",
    "arduino:avr:LilyPadUSB",
    "arduino:avr:lilypad",
    "arduino:avr:pro",
    "arduino:avr:atmegang",
    "arduino:avr:robotControl",
    "arduino:avr:robotMotor",
    "arduino:avr:gemma",
])

MAX_BODY_BYTES = 256 * 1024
COMPILE_TIMEOUT_SECONDS = 120
MAX_LOG_CHARS = 64 * 1024
MAX_CONCURRENT_COMPILES = 2
QUEUE_WAIT_SECONDS = 30

ARDUINO_CLI = os.environ.get("ARDUINO_CLI", "arduino-cli")

_compile_slots = threading.BoundedSemaphore(MAX_CONCURRENT_COMPILES)


class RequestError(Exception):
    def __init__(self, status, message):
        super().__init__(message)
        self.status = status
        self.message = message


def parse_request(body):
    """Returns (fqbn, sketch) from a request body, or raises RequestError."""
    try:
        data = json.loads(body.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError):
        raise RequestError(400, "The request is not valid JSON.")
    if not isinstance(data, dict):
        raise RequestError(400, "The request must be a JSON object.")
    fqbn = data.get("fqbn")
    sketch = data.get("sketch")
    if fqbn not in ALLOWED_FQBNS:
        raise RequestError(400, "Unsupported board: %s" % (fqbn,))
    if not isinstance(sketch, str) or not sketch.strip():
        raise RequestError(400, "The sketch is empty.")
    return fqbn, sketch


def compile_sketch(fqbn, sketch):
    """Compiles the sketch and returns the response dictionary."""
    with tempfile.TemporaryDirectory(prefix="fritzing-compile-") as work:
        # arduino-cli wants the .ino in a folder with the same name
        sketch_dir = os.path.join(work, "sketch")
        build_dir = os.path.join(work, "build")
        out_dir = os.path.join(work, "out")
        os.makedirs(sketch_dir)
        with open(os.path.join(sketch_dir, "sketch.ino"), "w", encoding="utf-8") as f:
            f.write(sketch)

        command = [ARDUINO_CLI, "compile", "--no-color", "--fqbn", fqbn,
                   "--build-path", build_dir, "--output-dir", out_dir, sketch_dir]
        try:
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    timeout=COMPILE_TIMEOUT_SECONDS, cwd=work)
        except subprocess.TimeoutExpired:
            return {"ok": False, "log": "Compiling took longer than %d seconds." % COMPILE_TIMEOUT_SECONDS, "hex": None}

        log = result.stdout.decode("utf-8", errors="replace").replace(work, "")
        if len(log) > MAX_LOG_CHARS:
            log = log[:MAX_LOG_CHARS] + "\n[output truncated]"
        hex_path = os.path.join(out_dir, "sketch.ino.hex")
        if result.returncode != 0 or not os.path.exists(hex_path):
            return {"ok": False, "log": log, "hex": None}
        with open(hex_path, "r", encoding="ascii") as f:
            return {"ok": True, "log": log, "hex": f.read()}


class Handler(BaseHTTPRequestHandler):
    server_version = "FritzingCompile/1"

    def send_json(self, status, data):
        body = json.dumps(data).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path == "/health":
            self.send_json(200, {"ok": True})
        else:
            self.send_json(404, {"ok": False, "log": "Not found."})

    def do_POST(self):
        if self.path != "/compile":
            self.send_json(404, {"ok": False, "log": "Not found."})
            return
        try:
            length = int(self.headers.get("Content-Length", ""))
        except ValueError:
            self.send_json(411, {"ok": False, "log": "Content-Length is required."})
            return
        if length < 0 or length > MAX_BODY_BYTES:
            self.send_json(413, {"ok": False, "log": "The sketch is too large."})
            return
        try:
            fqbn, sketch = parse_request(self.rfile.read(length))
        except RequestError as e:
            self.send_json(e.status, {"ok": False, "log": e.message})
            return

        if not _compile_slots.acquire(timeout=QUEUE_WAIT_SECONDS):
            self.send_json(503, {"ok": False, "log": "The compile service is busy; please try again."})
            return
        try:
            self.send_json(200, compile_sketch(fqbn, sketch))
        finally:
            _compile_slots.release()


def main():
    if shutil.which(ARDUINO_CLI) is None:
        sys.exit("arduino-cli not found")
    port = int(os.environ.get("PORT", "8000"))
    ThreadingHTTPServer(("0.0.0.0", port), Handler).serve_forever()


if __name__ == "__main__":
    main()
