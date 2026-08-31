#!/usr/bin/env python3
"""Regression test: a FAILED elevation must not report a successful update.

Issue #76. The follow-up to #75, and the worse half of it.

#75 fixed where the download is staged, so a root-installed hellish reached
`sudo install` at all. What nobody checked was whether that install worked:

    st = (int)update_capture(argv, out, sizeof(out));
    return (st >= 0 && access(target, X_OK) == 0);

Neither half can fail. update_capture returns the number of BYTES read from
the child's stdout -- `install` is silent whether it succeeds or not, so
that is 0 either way, and -1 only when fork/pipe itself breaks. And
access(target, X_OK) asks "is something executable at this path", which the
OLD binary already answers yes to.

So with sudo refused -- wrong password three times, or an account that is
not a sudoer -- the shell printed

    ✓ updated 2.7.2 → 2.7.6
      restart hellish (or open a new shell) to run it.

over a binary it had not touched, and called update_mark_notified(), which
takes the ⬆ badge away too. The user is told they are current, the reminder
stops, and they are still on the old version. That is strictly worse than
the failure it replaced, because nothing about it looks wrong.

HOW THIS REPRODUCES IT WITHOUT ROOT. Two facts make it hermetic: the
updater runs `sudo` through execvp, so PATH decides which one, and staging
only needs elevation when the install DIRECTORY is unwritable -- mode 0555
does that to its own owner. So: an unwritable install dir, and a `sudo` on
PATH that fails the way a refused password fails.

Both polarities are asserted. A test that only proves failure is reported
would also pass against a build that can no longer update at all.

Usage: python3 update_sudo_fail_test.py [/path/to/hellish]
"""
import hashlib
import http.server
import os
import pty
import select
import shutil
import stat
import subprocess
import sys
import tempfile
import threading
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []

FAKE_SUDO_FAIL = """#!/bin/sh
# Stands in for a refused elevation: says what sudo says, exits like sudo.
echo "sudo: 3 incorrect password attempts" >&2
exit 1
"""

FAKE_SUDO_OK = """#!/bin/sh
# A granted elevation. `exec "$@"` alone is not enough: it would run install
# as the same unprivileged user, and GNU install unlinks the destination
# first, which a mode-0555 directory refuses. Root is not subject to that
# directory permission, so the stand-in lifts it for the duration and puts it
# back -- the same power, obtained the only way a test without root can.
for a; do t="$a"; done
d=$(dirname "$t")
m=$(stat -c %a "$d")
chmod u+w "$d"
"$@"; rc=$?
chmod "$m" "$d"
exit $rc
"""


def check(name, ok, detail=""):
    print("  %s %s" % ("\033[32mok\033[0m  " if ok else "\033[31mFAIL\033[0m", name))
    if not ok:
        if detail:
            print("       %s" % detail.replace("\n", "\n       "))
        FAILS.append(name)


class Publisher:
    """A local stand-in for the GitHub release endpoints."""

    def __init__(self, version, payload):
        self.sha = hashlib.sha256(payload).hexdigest()
        blob, sha, ver = payload, self.sha, version

        class H(http.server.BaseHTTPRequestHandler):
            def log_message(self, *a):
                pass

            def do_GET(self):
                if self.path.endswith("/releases/latest"):
                    body = ('{"tag_name": "v%s"}' % ver).encode()
                elif self.path.endswith(".sha256"):
                    body = ("%s  hellish-linux-x86_64\n" % sha).encode()
                else:
                    body = blob
                self.send_response(200)
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

        self.srv = http.server.HTTPServer(("127.0.0.1", 0), H)
        self.port = self.srv.server_address[1]
        threading.Thread(target=self.srv.serve_forever, daemon=True).start()

    def env(self, home, sudodir):
        e = dict(os.environ)
        e["HOME"] = home
        e["PATH"] = sudodir + os.pathsep + e.get("PATH", "/usr/bin:/bin")
        e["HELLISH_UPDATE_API"] = (
            "http://127.0.0.1:%d/releases/latest" % self.port)
        e["HELLISH_UPDATE_DL"] = "http://127.0.0.1:%d/download" % self.port
        e["HELLISH_NO_BANNER"] = "1"
        e["HELLISH_NO_ANIM"] = "1"
        return e

    def stop(self):
        self.srv.shutdown()


def write_fake_sudo(dirpath, body):
    p = os.path.join(dirpath, "sudo")
    with open(p, "w") as f:
        f.write(body)
    os.chmod(p, 0o755)
    return p


def run_update_pty(binary, env, timeout=60):
    """Drive `update --now` to the end, answering both prompts."""
    pid, fd = pty.fork()
    if pid == 0:
        os.execve(binary, [binary, "-c", "update --now"], env)
        os._exit(1)
    out = b""
    end = time.time() + timeout
    picked = confirmed = False
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.3)
        if not r:
            continue
        try:
            d = os.read(fd, 65536)
        except OSError:
            break
        if not d:
            break
        out += d
        if not picked and b"How should hellish update?" in out:
            os.write(fd, b"\r")
            picked = True
            continue
        if not confirmed and b"proceed?" in out:
            os.write(fd, b"y\n")
            confirmed = True
            continue
        if b"update failed" in out or b"updated " in out:
            time.sleep(0.4)
            break
    try:
        os.close(fd)
    except OSError:
        pass
    try:
        os.waitpid(pid, 0)
    except ChildProcessError:
        pass
    return out.decode("utf-8", "replace")


def sha_of(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def scenario(pub, sudo_body):
    """Install a copy in an unwritable dir, update it, report what happened."""
    tmp = tempfile.mkdtemp()
    home = os.path.join(tmp, "home")
    bindir = os.path.join(tmp, "bin")
    sudodir = os.path.join(tmp, "sbin")
    for d in (home, bindir, sudodir):
        os.makedirs(d)
    installed = os.path.join(bindir, "hellish")
    shutil.copy2(SHELL, installed)
    os.chmod(installed, 0o755)
    write_fake_sudo(sudodir, sudo_body)
    # read+execute only: the owner cannot create files here, exactly like
    # /usr/bin after `make my_shell`. This is what forces the elevated path.
    os.chmod(bindir, 0o555)
    before = sha_of(installed)
    try:
        text = run_update_pty(installed, pub.env(home, sudodir))
    finally:
        os.chmod(bindir, 0o755)
    after = sha_of(installed)
    stray = [f for f in os.listdir(bindir) if f != "hellish"]
    shutil.rmtree(tmp, ignore_errors=True)
    return text, (before != after), stray


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)
    if os.geteuid() == 0:
        print("skip (running as root: every directory is writable, so the "
              "condition under test cannot exist)")
        sys.exit(0)

    # The "new release" has to genuinely REPORT the new version. update_apply
    # runs the download and asks it (step 4), so a plain copy still saying the
    # old number is rejected there and the elevated path is never reached --
    # a test built that way looks green while proving nothing.
    #
    # So re-stamp the version string in place with one of the SAME LENGTH,
    # which keeps every offset in the binary valid. Same trick as
    # tests/update_test.py.
    cur = subprocess.run([SHELL, "-c", "update --version"],
                         capture_output=True, text=True).stdout.split()[-1]
    newer = "9" + cur[1:]
    if newer == cur:
        newer = "8" + cur[1:]
    original = open(SHELL, "rb").read()
    payload = original.replace(cur.encode(), newer.encode())
    if payload == original:
        print("error: could not re-stamp the version string in the binary")
        sys.exit(2)
    pub = Publisher(newer, payload)

    try:
        print("\n  elevation REFUSED (the #76 false success)")
        text, changed, stray = scenario(pub, FAKE_SUDO_FAIL)

        check("a refused elevation is not reported as a completed update",
              "updated " not in text,
              "claimed success over an untouched binary:\n" + text[-400:])
        check("the binary really was left alone", not changed)
        check("the failure is reported",
              "update failed" in text,
              "said nothing about failing:\n" + text[-400:])
        check("sudo's own diagnosis reaches the user",
              "incorrect password" in text,
              "sudo's stderr was swallowed:\n" + text[-400:])
        check("no temp file is left in the install directory",
              stray == [], "found %r" % stray)

        # The other polarity. Without this, a build that simply refuses to
        # update anything would pass every check above.
        print("\n  elevation GRANTED (the path must still work)")
        text, changed, stray = scenario(pub, FAKE_SUDO_OK)
        check("a granted elevation completes the update",
              "updated " in text and "update failed" not in text,
              text[-400:])
        check("no temp file is left in the install directory",
              stray == [], "found %r" % stray)
    finally:
        pub.stop()

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
