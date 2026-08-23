#!/usr/bin/env python3
"""Regression test: a release published AFTER the last check -- issue #64.

Reported twice, and the second time with a screenshot that says it all:

    hellish 2.7.3 ...
    ✓ 2.7.3 up to date · via user binary · 50m ago

2.7.4 was out. The shell had checked 50 minutes earlier, when 2.7.3 really
was the newest thing there was, and cache_is_fresh() used a flat 24 hour
TTL -- so it would not look again until the next day. Every session in
between confidently reported "up to date". The only way out was typing
`update` by hand, which is exactly what a background update check exists to
save you from.

The interval is now adaptive, because the two states are not the same
question:

  * an update is ALREADY known pending -- there is nothing to learn, the
    badge is on the prompt and the banner has said so. Keep the long
    interval; asking again buys nothing.
  * we believe we are CURRENT -- this is the ONLY state in which a new
    release can exist without us knowing, so it is the only one where
    asking is worth anything. Short interval.

That asymmetry is what makes this cheap: the frequent case is one request
per quarter hour per machine, from a detached child, and it stops entirely
the moment an update is found.

Two hammering guards come with it, because "check more often" must not
become "check on every shell":

  * the attempt is recorded BEFORE the fetch, so a failing network backs
    off like a successful one instead of re-firing on every single startup;
  * and it is recorded by the PARENT, so twenty terminals opened at once
    produce one request rather than twenty.

`attempted` is kept separate from `checked` on purpose: `checked` still
means "last time we successfully learned something", which is what the
banner's "50m ago" reports. A failed attempt must not be able to claim it.

Usage: python3 update_freshness_test.py /path/to/hellish
"""
import fcntl
import http.server
import os
import pty
import re
import select
import shutil
import struct
import subprocess
import sys
import tempfile
import termios
import threading
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "build/bin/hellish")
FAILS = []
ESC = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]|\x1b\][^\x07]*\x07")


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def version():
    out = subprocess.run([SHELL, "--version"], capture_output=True,
                         text=True, timeout=30).stdout
    return out.split("version ", 1)[1].split()[0].strip(" ,")


RUNNING = version()


def bump(v, part=2):
    n = [int(x) for x in v.split(".")[:3]]
    n[part] += 1
    return ".".join(str(x) for x in n)


NEWER = bump(RUNNING)


class Server:
    """A release endpoint that counts how many times it is asked."""

    def __init__(self, tag):
        self.hits = 0
        outer = self

        class H(http.server.BaseHTTPRequestHandler):
            def do_GET(self):
                outer.hits += 1
                body = ('{"tag_name": "v%s", "name": "r"}' % tag).encode()
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

            def log_message(self, *a):
                pass

        self.srv = http.server.HTTPServer(("127.0.0.1", 0), H)
        self.url = "http://127.0.0.1:%d/releases/latest" % \
            self.srv.server_address[1]
        threading.Thread(target=self.srv.serve_forever, daemon=True).start()

    def stop(self):
        self.srv.shutdown()


def seed(cache, **kv):
    d = os.path.join(cache, "hellish")
    os.makedirs(d, exist_ok=True)
    rec = {"latest": "", "checked": 0, "notified": 0, "header_shown": 0,
           "header_rev": 0, "header_ver": "", "announced": "", "attempted": 0}
    rec.update(kv)
    with open(os.path.join(d, "state"), "w") as f:
        for k, v in rec.items():
            f.write("%s=%s\n" % (k, v))


def read_state(cache):
    p = os.path.join(cache, "hellish", "state")
    out = {}
    if os.path.exists(p):
        for line in open(p):
            if "=" in line:
                k, v = line.rstrip("\n").split("=", 1)
                out[k] = v
    return out


def session(cache, api, settle=3.0, cmds=(b"echo MARK\n",)):
    env = {"HOME": os.environ.get("HOME", "/tmp"), "PATH": os.environ["PATH"],
           "TERM": "xterm-256color", "LANG": "C.UTF-8",
           "XDG_CACHE_HOME": cache, "ASAN_OPTIONS": "detect_leaks=0",
           "HELLISH_UPDATE_API": api}
    t0 = time.time()
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        os.execv(SHELL, [SHELL])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 40, 150, 0, 0))
    out = b""
    first = None
    end = time.time() + settle
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                chunk = os.read(fd, 65536)
            except OSError:
                break
            if first is None and chunk.strip():
                first = time.time() - t0
            out += chunk
    for c in cmds:
        os.write(fd, c)
        end = time.time() + 1.5
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.05)
            if r:
                try:
                    out += os.read(fd, 65536)
                except OSError:
                    break
    try:
        os.killpg(pid, 9)
    except OSError:
        pass
    try:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    except OSError:
        pass
    os.close(fd)
    return ESC.sub("", out.decode(errors="replace")), (first or 99.0)


def main():
    print("running %s, pretending %s is released\n" % (RUNNING, NEWER))
    now = int(time.time())

    # ── 1. THE REPORT. Checked 50 minutes ago, believed current, and a new
    #      release has landed since. The shell must look again.
    srv = Server(NEWER)
    cache = tempfile.mkdtemp()
    try:
        seed(cache, latest=RUNNING, checked=now - 3000, attempted=now - 3000,
             header_shown=now, header_rev=3, header_ver=RUNNING,
             announced=RUNNING)
        txt, _ = session(cache, srv.url)
        st = read_state(cache)
        check("a 50-minute-old check does not block a new one",
              srv.hits >= 1, "the endpoint was never asked")
        check("the newer release is discovered", st.get("latest") == NEWER,
              "state still says latest=%s" % st.get("latest"))
        # The session that DISCOVERS the release must say so without being
        # restarted -- the whole complaint is having to find out by hand.
        # Either channel counts: the one-shot notice between commands, or
        # the prompt badge once its few-second cache turns over. Which one
        # wins is a timing detail; being told is not.
        check("the discovering session says so, unprompted",
              "⬆" in txt and NEWER in txt,
              "nothing announced in the session that found it: %r"
              % txt[-300:])
        txt2, _ = session(cache, srv.url, settle=2.0)
        check("and the next session carries the badge", "⬆" + NEWER in txt2,
              "no badge: %r" % txt2[-300:])
    finally:
        srv.stop()
        shutil.rmtree(cache, ignore_errors=True)

    # ── 2. Not hammering. Once an update IS known pending there is nothing
    #      left to learn, so a fresh attempt must not fire every session.
    srv = Server(NEWER)
    cache = tempfile.mkdtemp()
    try:
        seed(cache, latest=NEWER, checked=now - 3000, attempted=now - 3000,
             header_shown=now, header_rev=3, header_ver=RUNNING,
             announced=NEWER)
        session(cache, srv.url, settle=2.0)
        check("a known pending update does not trigger a re-check",
              srv.hits == 0, "asked %d times with nothing to learn" % srv.hits)
    finally:
        srv.stop()
        shutil.rmtree(cache, ignore_errors=True)

    # ── 3. A very recent attempt is still respected -- the short interval is
    #      an interval, not "every startup".
    srv = Server(NEWER)
    cache = tempfile.mkdtemp()
    try:
        seed(cache, latest=RUNNING, checked=now - 10, attempted=now - 10,
             header_shown=now, header_rev=3, header_ver=RUNNING,
             announced=RUNNING)
        session(cache, srv.url, settle=2.0)
        check("a 10-second-old attempt is still fresh", srv.hits == 0,
              "asked %d times seconds after the last attempt" % srv.hits)
    finally:
        srv.stop()
        shutil.rmtree(cache, ignore_errors=True)

    # ── 4. Single flight. Terminals opened together must not each fire.
    srv = Server(NEWER)
    cache = tempfile.mkdtemp()
    try:
        seed(cache, latest=RUNNING, checked=now - 3000, attempted=now - 3000,
             header_shown=now, header_rev=3, header_ver=RUNNING,
             announced=RUNNING)
        ts = [threading.Thread(target=session, args=(cache, srv.url, 2.5))
              for _ in range(6)]
        for t in ts:
            t.start()
        for t in ts:
            t.join()
        check("six shells at once make at most two requests", srv.hits <= 2,
              "%d requests -- one release per terminal" % srv.hits)
    finally:
        srv.stop()
        shutil.rmtree(cache, ignore_errors=True)

    # ── 5. A failing endpoint backs off instead of retrying every startup,
    #      and `checked` is NOT advanced by a failure -- the banner's "Xm
    #      ago" must keep meaning "last time we actually learned something".
    cache = tempfile.mkdtemp()
    try:
        seed(cache, latest=RUNNING, checked=now - 3000, attempted=now - 3000,
             header_shown=now, header_rev=3, header_ver=RUNNING,
             announced=RUNNING)
        _, first = session(cache, "http://127.0.0.1:9/dead", settle=2.5)
        st = read_state(cache)
        check("a failed check still records the attempt",
              int(st.get("attempted", 0)) > now - 100,
              "attempted=%s -- every startup would retry" % st.get("attempted"))
        check("a failed check does not claim a successful one",
              int(st.get("checked", 0)) <= now - 2000,
              "checked=%s was advanced by a failure" % st.get("checked"))
        check("a dead endpoint never delays startup", first < 1.0,
              "first output took %.2fs" % first)
    finally:
        shutil.rmtree(cache, ignore_errors=True)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
