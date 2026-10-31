# Copyright (c) 2026 Fritzing GmbH
#
# Tests for server.py. The compile test needs arduino-cli with the arduino:avr core, so run
# them in the compile service image:
#
#   docker build -t fritzing-compile docker/wasm/compile-service
#   docker run --rm --network none fritzing-compile python3 -m unittest -v test_server

import json
import shutil
import threading
import unittest
import urllib.error
import urllib.request
from http.server import ThreadingHTTPServer

import server

BLINK = """
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}
"""


class ParseRequestTest(unittest.TestCase):
    def test_valid(self):
        body = json.dumps({"fqbn": "arduino:avr:uno", "sketch": BLINK}).encode()
        self.assertEqual(server.parse_request(body), ("arduino:avr:uno", BLINK))

    def test_rejects_unknown_board(self):
        body = json.dumps({"fqbn": "esp32:esp32:esp32", "sketch": BLINK}).encode()
        with self.assertRaises(server.RequestError) as e:
            server.parse_request(body)
        self.assertEqual(e.exception.status, 400)

    def test_rejects_option_like_board(self):
        body = json.dumps({"fqbn": "--config-file=/etc/passwd", "sketch": BLINK}).encode()
        with self.assertRaises(server.RequestError):
            server.parse_request(body)

    def test_rejects_empty_sketch(self):
        body = json.dumps({"fqbn": "arduino:avr:uno", "sketch": "  "}).encode()
        with self.assertRaises(server.RequestError):
            server.parse_request(body)

    def test_rejects_invalid_json(self):
        for body in [b"not json", b"[1, 2]", b"\xff\xfe"]:
            with self.assertRaises(server.RequestError):
                server.parse_request(body)


class HttpTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.httpd = ThreadingHTTPServer(("127.0.0.1", 0), server.Handler)
        cls.base = "http://127.0.0.1:%d" % cls.httpd.server_address[1]
        threading.Thread(target=cls.httpd.serve_forever, daemon=True).start()

    @classmethod
    def tearDownClass(cls):
        cls.httpd.shutdown()

    def post(self, path, body):
        request = urllib.request.Request(self.base + path, data=body, method="POST",
                                         headers={"Content-Type": "application/json"})
        try:
            with urllib.request.urlopen(request, timeout=300) as response:
                return response.status, json.loads(response.read())
        except urllib.error.HTTPError as e:
            return e.code, json.loads(e.read())

    def test_health(self):
        with urllib.request.urlopen(self.base + "/health") as response:
            self.assertEqual(json.loads(response.read()), {"ok": True})

    def test_rejects_large_body(self):
        status, data = self.post("/compile", b"x" * (server.MAX_BODY_BYTES + 1))
        self.assertEqual(status, 413)
        self.assertFalse(data["ok"])

    def test_rejects_unknown_board(self):
        status, data = self.post("/compile", json.dumps({"fqbn": "x:y:z", "sketch": BLINK}).encode())
        self.assertEqual(status, 400)
        self.assertFalse(data["ok"])

    @unittest.skipUnless(shutil.which(server.ARDUINO_CLI), "arduino-cli is not installed")
    def test_compiles_blink(self):
        status, data = self.post("/compile", json.dumps({"fqbn": "arduino:avr:uno", "sketch": BLINK}).encode())
        self.assertEqual(status, 200, data["log"])
        self.assertTrue(data["ok"], data["log"])
        lines = data["hex"].strip().splitlines()
        self.assertTrue(all(line.startswith(":") for line in lines))
        self.assertEqual(lines[-1], ":00000001FF")

    @unittest.skipUnless(shutil.which(server.ARDUINO_CLI), "arduino-cli is not installed")
    def test_reports_compile_errors(self):
        status, data = self.post("/compile", json.dumps({"fqbn": "arduino:avr:uno", "sketch": "void setup( {"}).encode())
        self.assertEqual(status, 200)
        self.assertFalse(data["ok"])
        self.assertIsNone(data["hex"])
        self.assertIn("error", data["log"])


if __name__ == "__main__":
    unittest.main()
