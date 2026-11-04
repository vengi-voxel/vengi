#!/usr/bin/env python3
"""Local fixture for ClientTest.testHttpResponse and test-http-browser.html."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import time


class Handler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        super().end_headers()

    def do_GET(self):
        if self.path == "/slow":
            time.sleep(0.2)
            self.send_response(200)
            self.end_headers()
            try:
                self.wfile.write(b"slow")
            except (BrokenPipeError, ConnectionResetError):
                pass
        elif self.path == "/missing":
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"missing")
        else:
            super().do_GET()

    def do_POST(self):
        body = self.rfile.read(int(self.headers.get("Content-Length", "0")))
        self.send_response(404)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("X-Test", self.headers.get("X-Test", "missing"))
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *_):
        pass


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", type=int)
    parser.add_argument("--directory", default=".")
    args = parser.parse_args()
    handler = partial(Handler, directory=args.directory)
    ThreadingHTTPServer(("127.0.0.1", args.port), handler).serve_forever()
