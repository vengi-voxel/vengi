#!/usr/bin/env python3
import argparse
from http.server import HTTPServer, SimpleHTTPRequestHandler
import functools

class COOPHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add COOP/COEP headers for cross-origin isolation
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        super().end_headers()

def run_server(port, directory):
    handler = functools.partial(COOPHandler, directory=directory)
    httpd = HTTPServer(("localhost", port), handler)
    print(f"Serving {directory!r} on http://localhost:{port}")
    httpd.serve_forever()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="HTTP server with COOP/COEP headers")
    parser.add_argument(
        "-d", "--directory",
        default=".",
        help="Directory to serve (default: current directory)"
    )
    parser.add_argument(
        "-p", "--port",
        type=int,
        default=8000,
        help="Port number (default: 8000)"
    )
    args = parser.parse_args()

    run_server(args.port, args.directory)
