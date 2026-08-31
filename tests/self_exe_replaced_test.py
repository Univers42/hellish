#!/usr/bin/env python3
"""Regression test: a session whose own binary was replaced underneath it.

Issue #76, the third bug in that thread and the one with the widest blast
radius. It needs no privileges to reproduce and it happens on EVERY in-place
upgrade -- `make my_shell`, `update --now`, and any `sudo install` over the
running shell.

WHAT HAPPENS. install(1) unlinks the destination and creates a new inode,
deliberately, so the running process keeps executing the old image. From
that moment Linux answers /proc/self/exe with the path plus an English
annotation:

    /usr/bin/hellish (deleted)

self_exe_path() handed that back verbatim, and everything downstream treated
it as a filename. Two failures, both SILENT:

  1. Process substitution re-execs that path to run the body, so
         cat <(echo hi)
     produced NOTHING -- no error, no diagnostic, an empty result -- for the
     rest of the session after any upgrade. Silent wrong output is the worst
     failure mode there is; a user would blame their own script.

  2. The updater installed to a literal file named "hellish (deleted)" beside
     the real one, so the binary it meant to replace was never touched. Note
     that the post-install version check CANNOT catch this: the file it runs
     is the freshly written one, which honestly reports the new version. It
     reports success, and it is wrong.

Plus the confusion that started the report: HELLISH_VERSION is compiled in,
so an already-open session says "you have 2.3.2" while `hellish --version`
in the same terminal says 2.7.6. Both are true -- of different processes.
Accepting the offered update in that state re-ran install.sh, which installs
to /usr/local/bin, AHEAD of /usr/bin on the default Debian/Ubuntu PATH: one
`make my_shell` plus one stale `update` leaves two hellishes with the newer
one shadowed.

Everything below is asserted on a REAL pty against a REAL replacement, in a
throwaway directory. No sudo, no container, no network.

Usage: python3 self_exe_replaced_test.py [/path/to/hellish]
"""
import hashlib
import http.server
import os
import pty
import select
import shutil
import subprocess
import sys
import tempfile
import threading
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []


def check(name, ok, detail=""):
    mark = "\033[32mok\033[0m  " if ok else "\033[31mFAIL\033[0m"
    print("  %s %s" % (mark, name), flush=True)
    if not ok:
        if detail:
            print("       %s" % str(detail).replace("\n", "\n       "))
        FAILS.append(name)


class Publisher:
    """A local stand-in for the release endpoints."""

    def __init__(self, version, payload):
        sha = hashlib.sha256(payload).hexdigest()

        class H(http.server.BaseHTTPRequestHandler):
            def log_message(self, *a):
                pass

            def handle_one_request(self):
                try:
                    http.server.BaseHTTPRequestHandler.handle_one_request(self)
                except (BrokenPipeError, ConnectionResetError):
                    self.close_connection = True

            def do_GET(self):
                if self.path.endswith("/releases/latest"):
                    body = ('{"tag_name": "v%s"}' % version).encode()
                elif self.path.endswith(".sha256"):
                    body = ("%s  hellish-linux-x86_64\n" % sha).encode()
                else:
                    body = payload
                self.send_response(200)
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

        self.srv = http.server.HTTPServer(("127.0.0.1", 0), H)
        self.port = self.srv.server_address[1]
        threading.Thread(target=self.srv.serve_forever, daemon=True).start()

    def env(self, home):
        e = dict(os.environ)
        e.update({
            "HOME": home, "TERM": "dumb", "PS1": "$ ",
            "HELLISH_NO_BANNER": "1", "HELLISH_NO_ANIM": "1",
            "HELLISH_NO_UPDATE_CHECK": "1",
            "HELLISH_UPDATE_API":
                "http://127.0.0.1:%d/releases/latest" % self.port,
            "HELLISH_UPDATE_DL":
                "http://127.0.0.1:%d/download" % self.port,
        })
        return e

    def stop(self):
        self.srv.shutdown()


class Session:
    """An interactive hellish on a real pty, that outlives a replacement."""

    def __init__(self, binary, env):
        self.out = b""
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.execve(binary, [binary, "-i"], env)
            os._exit(1)
        self.drain(2.0)

    def drain(self, t=2.0):
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([self.fd], [], [], 0.2)
            if not r:
                continue
            try:
                d = os.read(self.fd, 65536)
            except OSError:
                return
            if not d:
                return
            self.out += d

    def run(self, line, t=3.0):
        """Send a line and return only what it produced."""
        mark = len(self.out)
        time.sleep(0.4)
        os.write(self.fd, (line + "\n").encode())
        self.drain(t)
        return self.out[mark:].decode("utf-8", "replace")

    def close(self):
        try:
            os.write(self.fd, b"exit\n")
            time.sleep(0.3)
            os.close(self.fd)
        except OSError:
            pass


def restamp(path, old, new):
    """A binary that genuinely reports `new`, by overwriting the version
    string in place with one of the SAME length -- every offset stays valid."""
    blob = open(SHELL, "rb").read().replace(old.encode(), new.encode())
    with open(path, "wb") as f:
        f.write(blob)
    os.chmod(path, 0o755)
    return blob


def version_of(path):
    r = subprocess.run([path, "-c", "update --version"],
                       capture_output=True, text=True)
    return r.stdout.strip().split()[-1] if r.stdout.strip() else ""


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    cur = version_of(SHELL)
    old = "0" + cur[1:]
    if old == cur:
        old = "1" + cur[1:]

    tmp = tempfile.mkdtemp()
    home = os.path.join(tmp, "home")
    os.makedirs(home)
    live = os.path.join(tmp, "hellish")
    newer = os.path.join(tmp, "newer")
    restamp(live, cur, old)
    payload = restamp(newer, cur, cur)
    pub = Publisher(cur, payload)

    try:
        print("\n\033[1;36m▸\033[0m \033[1mbefore the replacement\033[0m")
        s = Session(live, pub.env(home))
        out = s.run("cat <(printf 'PROC%s\\n' SUB)")
        check("process substitution works", "PROCSUB" in out, out[-200:])

        # Exactly what `make my_shell` and the updater both do.
        subprocess.run(["install", "-m", "755", newer, live], check=True)
        check("the binary really was replaced under the running shell",
              version_of(live) == cur and cur != old,
              "on disk %r, session %r" % (version_of(live), old))

        print("\n\033[1;36m▸\033[0m \033[1mafter it (same session)\033[0m")
        out = s.run("cat <(printf 'STILL%s\\n' OK)")
        check("process substitution SURVIVES the replacement",
              "STILLOK" in out,
              "silently produced nothing -- the '(deleted)' path was exec'd:"
              "\n" + out[-300:])

        print("\n\033[1;36m▸\033[0m \033[1m`update` in the stale session\033[0m")
        out = s.run("update", t=10.0)
        # NOT `readlink /proc/$$/exe` -- the KERNEL always appends the
        # annotation, and no shell can change that. What is under test is
        # what hellish RESOLVES it to internally, and the only way to see
        # that is a message built from it. This one is printed straight
        # from update_exe_path(), so a regression shows up here.
        check("the path hellish resolved has no ' (deleted)' in it",
              "(deleted)" not in out,
              "the annotation reached a real code path:\n" + out[-400:])
        check("it says the session is stale rather than only the version",
              "was replaced while it was open" in out, out[-400:])
        check("it names the version now on disk", cur in out, out[-400:])
        check("it tells the user to restart", "exec " in out, out[-400:])
        # The offer is what produced the duplicate install in the report.
        check("it does NOT offer to download anything",
              "[Update]" not in out and "press u" not in out, out[-400:])
        s.close()

        # The ordering above is the FORGIVING one: that first procsub resolves
        # and caches the path while it is still clean, so the cache hides the
        # bug. A session that simply had not used <(...) before the upgrade
        # resolves it for the first time afterwards, with the annotation
        # already attached -- and that is the one that breaks. Both orderings
        # are real; only this one is a test.
        print("\n\033[1;36m▸\033[0m \033[1mcold cache: first <(...) AFTER the"
              " replacement\033[0m")
        cold = os.path.join(tmp, "cold")
        restamp(cold, cur, old)
        c = Session(cold, pub.env(home))
        c.run("echo warmup-without-procsub")
        subprocess.run(["install", "-m", "755", newer, cold], check=True)
        out = c.run("cat <(printf 'COLD%s\\n' OK)")
        check("process substitution works on a never-warmed session",
              "COLDOK" in out,
              "silently produced nothing:\n" + out[-300:])
        c.close()

        print("\n\033[1;36m▸\033[0m \033[1man ordinary session still updates"
              "\033[0m")
        live2 = os.path.join(tmp, "hellish2")
        restamp(live2, cur, old)
        s2 = Session(live2, pub.env(home))
        out = s2.run("update", t=10.0)
        check("a NON-stale session is still offered the update",
              "[Update]" in out or "press u" in out,
              "the stale guard swallowed a real update:\n" + out[-400:])
        check("and it is not wrongly called stale",
              "was replaced while it was open" not in out, out[-400:])
        s2.close()
    finally:
        pub.stop()
        shutil.rmtree(tmp, ignore_errors=True)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
