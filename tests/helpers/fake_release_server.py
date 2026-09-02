#!/usr/bin/env python3
"""The fake release channel behind tests/installer_suite.sh.

`python3 -m http.server` served the suite until issue #111: a 42 student's
`curl | sh` stopped at

    Username for 'https://github.com':

because the school image's git 2.34 takes a 401 from GitHub's smart-HTTP
endpoint over HTTP/2 and, with a terminal attached, asks for credentials
instead of failing. A plain file server can never reproduce a 401, so the
scenario had no detector. This one serves the release directory exactly
as before and, for anything under /private/, answers git's smart-HTTP
probes (info/refs, git-upload-pack) with 401 + WWW-Authenticate -- the
shape that makes git prompt -- while still serving the archive tarball
beside it, which is the fallback install.sh must reach.

    python3 tests/helpers/fake_release_server.py PORT DIRECTORY
"""
import functools
import http.server
import sys


class Handler(http.server.SimpleHTTPRequestHandler):
    def _wants_credentials(self):
        return ("/private/" in self.path
                and ("info/refs" in self.path
                     or "git-upload-pack" in self.path))

    def _deny(self):
        self.send_response(401)
        self.send_header("WWW-Authenticate", 'Basic realm="fake-github"')
        self.send_header("Content-Length", "0")
        self.end_headers()

    def do_GET(self):
        if self._wants_credentials():
            return self._deny()
        return super().do_GET()

    def do_POST(self):
        if self._wants_credentials():
            return self._deny()
        self.send_error(404)

    def log_message(self, *args):
        pass


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8377
    root = sys.argv[2] if len(sys.argv) > 2 else "."
    handler = functools.partial(Handler, directory=root)
    http.server.ThreadingHTTPServer(("127.0.0.1", port), handler) \
        .serve_forever()


if __name__ == "__main__":
    main()
