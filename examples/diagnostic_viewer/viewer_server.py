#!/usr/bin/env python3

import argparse
import http.server
import json
import subprocess
import sys
import time
import urllib.parse
from pathlib import Path
from typing import Optional


SCRIPT_PATH = Path(__file__).resolve()
REPO_ROOT = SCRIPT_PATH.parents[2]
DEFAULT_BINARY = REPO_ROOT / "build/linux-debug/examples/diag_diagnostic_viewer_device"
STATIC_DIR = SCRIPT_PATH.with_name("static")
MAX_POST_BODY_BYTES = 256


class DiagnosticViewerHandler(http.server.BaseHTTPRequestHandler):
    binary = DEFAULT_BINARY
    phase_seconds = 4.0
    started_at = time.monotonic()

    def log_message(self, fmt: str, *args: object) -> None:
        sys.stderr.write("diagnostic_viewer: " + fmt % args + "\n")

    def do_GET(self) -> None:
        path = urllib.parse.urlsplit(self.path).path

        if path == "/":
            self._serve_static("index.html", "text/html; charset=utf-8")
            return

        if path == "/app.css":
            self._serve_static("app.css", "text/css; charset=utf-8")
            return

        if path == "/app.js":
            self._serve_static("app.js", "application/javascript; charset=utf-8")
            return

        if path == "/healthz":
            self._write_text(200, "ok\n", "text/plain; charset=utf-8")
            return

        if path == "/api/snapshot":
            self._write_snapshot(clear_dtc=None)
            return

        self._write_text(404, "not found\n", "text/plain; charset=utf-8")

    def do_POST(self) -> None:
        path = urllib.parse.urlsplit(self.path).path

        if path != "/api/clear":
            self._write_text(404, "not found\n", "text/plain; charset=utf-8")
            return

        body = self._read_json_body()
        if body is None:
            return

        try:
            payload = json.loads(body)
            clear_dtc = str(payload["dtc_id"])
        except (KeyError, TypeError, ValueError, json.JSONDecodeError):
            self._write_text(400, "expected JSON body with dtc_id\n", "text/plain; charset=utf-8")
            return

        self._write_snapshot(clear_dtc=clear_dtc)

    def _serve_static(self, name: str, content_type: str) -> None:
        try:
            body = (STATIC_DIR / name).read_bytes()
        except OSError:
            self._write_text(404, "asset not found\n", "text/plain; charset=utf-8")
            return

        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _read_json_body(self) -> Optional[str]:
        length_text = self.headers.get("Content-Length")

        if length_text is None:
            self._write_text(411, "Content-Length required\n", "text/plain; charset=utf-8")
            return None

        try:
            length = int(length_text)
        except ValueError:
            self._write_text(400, "invalid Content-Length\n", "text/plain; charset=utf-8")
            return None

        if length < 0 or length > MAX_POST_BODY_BYTES:
            self._write_text(413, "request body too large\n", "text/plain; charset=utf-8")
            return None

        return self.rfile.read(length).decode("utf-8")

    def _write_snapshot(self, clear_dtc: Optional[str]) -> None:
        command = [
            str(self.binary),
            "--json",
            "--scenario-step",
            str(self._scenario_step()),
        ]
        if clear_dtc:
            command.extend(["--clear", clear_dtc])

        try:
            completed = subprocess.run(
                command,
                check=True,
                capture_output=True,
                text=True,
                timeout=5.0,
            )
        except subprocess.CalledProcessError as exc:
            body = exc.stderr if exc.stderr else "diagnostic viewer device failed\n"
            self._write_text(502, body, "text/plain; charset=utf-8")
            return
        except subprocess.TimeoutExpired:
            self._write_text(504, "diagnostic viewer device timed out\n", "text/plain; charset=utf-8")
            return
        except OSError as exc:
            self._write_text(500, f"cannot run diagnostic viewer device: {exc}\n", "text/plain; charset=utf-8")
            return

        self._write_text(200, completed.stdout, "application/json; charset=utf-8")

    @classmethod
    def _scenario_step(cls) -> int:
        elapsed = time.monotonic() - cls.started_at
        return int(elapsed / cls.phase_seconds)

    def _write_text(self, status: int, body: str, content_type: str) -> None:
        encoded = body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(encoded)))
        self.end_headers()
        self.wfile.write(encoded)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Serve the diagnostics browser viewer.")
    parser.add_argument("--binary", default=str(DEFAULT_BINARY))
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8088)
    parser.add_argument("--self-test", action="store_true")
    return parser.parse_args()


def self_test(binary: Path) -> int:
    completed = subprocess.run(
        [str(binary), "--json", "--scenario-step", "3"],
        check=True,
        capture_output=True,
        text=True,
        timeout=5.0,
    )
    snapshot = json.loads(completed.stdout)
    if snapshot["summary"]["active_dtcs"] < 1:
        raise RuntimeError("expected at least one active DTC in the scenario")
    if len(snapshot["dtcs"]) != 3:
        raise RuntimeError("expected three registered DTCs")
    return 0


def main() -> int:
    args = parse_args()
    binary = Path(args.binary)

    if args.self_test:
        return self_test(binary)

    DiagnosticViewerHandler.binary = binary
    server = http.server.ThreadingHTTPServer((args.host, args.port), DiagnosticViewerHandler)
    print(f"diagnostic_viewer: open http://{args.host}:{args.port}", flush=True)
    server.serve_forever()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
