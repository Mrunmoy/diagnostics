#!/usr/bin/env python3

import argparse
import http.server
import subprocess
import sys
import urllib.parse
from pathlib import Path

SCRIPT_PATH = Path(__file__).resolve()
if len(SCRIPT_PATH.parents) > 3:
    DEFAULT_BINARY = SCRIPT_PATH.parents[3] / "build/linux-debug/examples/diag_grafana_reader_example"
else:
    DEFAULT_BINARY = SCRIPT_PATH.with_name("diag_grafana_reader_example")


class DiagnosticMetricsHandler(http.server.BaseHTTPRequestHandler):
    binary = DEFAULT_BINARY

    def log_message(self, fmt, *args):
        sys.stderr.write("grafana_reader_service: " + fmt % args + "\n")

    def do_GET(self):
        path = urllib.parse.urlsplit(self.path).path

        if path == "/healthz":
            self._write_text(200, "ok\n", "text/plain; charset=utf-8")
            return

        if path == "/metrics":
            self._run_export("--prometheus", "text/plain; version=0.0.4; charset=utf-8")
            return

        if path == "/snapshot.json":
            self._run_export("--json", "application/json; charset=utf-8")
            return

        self._write_text(404, "not found\n", "text/plain; charset=utf-8")

    def _run_export(self, mode, content_type):
        try:
            completed = subprocess.run(
                [str(self.binary), mode],
                check=True,
                capture_output=True,
                text=True,
                timeout=5.0,
            )
        except subprocess.CalledProcessError as exc:
            body = exc.stderr if exc.stderr else "diagnostic exporter failed\n"
            self._write_text(502, body, "text/plain; charset=utf-8")
            return
        except subprocess.TimeoutExpired:
            self._write_text(504, "diagnostic exporter timed out\n", "text/plain; charset=utf-8")
            return
        except OSError as exc:
            self._write_text(500, f"cannot run diagnostic exporter: {exc}\n", "text/plain; charset=utf-8")
            return

        self._write_text(200, completed.stdout, content_type)

    def _write_text(self, status, body, content_type):
        encoded = body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(encoded)))
        self.end_headers()
        self.wfile.write(encoded)


def parse_args():
    parser = argparse.ArgumentParser(description="Serve grafana_reader diagnostics over HTTP.")
    parser.add_argument("--binary", default=str(DEFAULT_BINARY))
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=9108)
    return parser.parse_args()


def main():
    args = parse_args()
    DiagnosticMetricsHandler.binary = Path(args.binary)
    server = http.server.ThreadingHTTPServer((args.host, args.port), DiagnosticMetricsHandler)
    print(f"grafana_reader_service: listening on http://{args.host}:{args.port}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
