#!/usr/bin/env python3
"""Regression test: updating a ROOT-INSTALLED hellish -- issue #75.

`make my_shell` puts the binary in /usr/bin, owned by root. When the shell
later notices a release and you accept the update, it failed with

    hellish: update failed — the download failed
      the installed binary is unchanged.

which is a false diagnosis: the network was fine. update_apply staged the
download at `<target>.hellish-update` -- i.e. INSIDE /usr/bin -- so curl was
asked to create a file in a directory the user cannot write. It died at
step 2 (download) before the elevation logic it already had was ever
reached.

The elevation itself was never the missing piece: update_needs_sudo() spots
the case and move_into_place() already shells out to `sudo install`. Only the
staging path was wrong, and it made a permission problem look like a network
one -- which is why the report describes "an error of script".

Staging now follows the same decision: when elevation is needed, the download
goes to TMPDIR and `sudo install` places it (which is cross-filesystem safe,
unlike rename). Without elevation it still lands beside the target, so the
replacement stays an atomic same-directory rename.

The reproduction needs no root: a directory the caller cannot write is
enough, and mode 0555 does that for its own owner.

Usage: python3 update_sudo_test.py [/path/to/hellish]
"""
import hashlib
import http.server
import os
import pty
import select
import time
import shutil
import stat
import subprocess
import sys
import tempfile
import threading

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


class Publisher:
    """Stand-in for GitHub Releases: metadata, an asset, its checksum."""

    def __init__(self, tag, payload):
        self.tag, self.payload = tag, payload
        self.sha = hashlib.sha256(payload).hexdigest()
        self.asset = "hellish-linux-" + os.uname().machine
        pub = self

        class H(http.server.BaseHTTPRequestHandler):
            def log_message(self, *a):
                pass

            def do_GET(self):
                if self.path.endswith("/releases/latest"):
                    body = ('{"tag_name": "v%s", "name": "r"}'
                            % pub.tag).encode()
                elif self.path.endswith(".sha256"):
                    body = ("%s  %s\n" % (pub.sha, pub.asset)).encode()
                elif self.path.endswith(pub.asset):
                    body = pub.payload
                else:
                    return self.send_error(404)
                self.send_response(200)
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

        self.srv = http.server.HTTPServer(("127.0.0.1", 0), H)
        self.port = self.srv.server_address[1]
        threading.Thread(target=self.srv.serve_forever, daemon=True).start()

    def env(self, home):
        base = "http://127.0.0.1:%d" % self.port
        return {"HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
                "HELLISH_UPDATE_API": base + "/releases/latest",
                "HELLISH_UPDATE_DL": base + "/download",
                "HELLISH_NO_BANNER": "1", "HELLISH_NO_ANIM": "1",
                "HELLISH_UPDATE_TTL": "0", "ASAN_OPTIONS": "detect_leaks=0",
                "TERM": "dumb"}

    def stop(self):
        self.srv.shutdown()


def run_update_pty(binary, env):
    """Drive `update --now` to completion through a pty.

    Two interactions have to be answered or the update never starts, and
    neither happens on a non-tty stdin -- which is why an earlier version of
    this test passed against the broken build:

      1. a menu asking HOW to update (the system-binary row is preselected
         and marked "(detected)", so Enter takes it);
      2. the elevation confirmation, "proceed? [y/N]".
    """
    pid, fd = pty.fork()
    if pid == 0:
        os.execve(binary, [binary, "-c", "update --now"], env)
        os._exit(1)
    out = b""
    end = time.time() + 90
    picked = False
    confirmed = False
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


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)
    if os.geteuid() == 0:
        print("skip (running as root: every directory is writable, so the "
              "condition under test cannot exist)")
        sys.exit(0)

    tmp = tempfile.mkdtemp()
    home = os.path.join(tmp, "home")
    bindir = os.path.join(tmp, "bin")
    os.makedirs(home)
    os.makedirs(bindir)
    installed = os.path.join(bindir, "hellish")
    shutil.copy2(SHELL, installed)
    os.chmod(installed, 0o755)
    # read+execute only: the caller owns it but cannot create files in it,
    # exactly like /usr/bin after `make my_shell`.
    os.chmod(bindir, 0o555)

    pub = Publisher("9.9.9", open(SHELL, "rb").read())
    try:
        check("the install directory really is unwritable",
              not os.access(bindir, os.W_OK),
              "the test cannot reproduce anything if it is writable")

        # A REAL pty, and an answered confirmation. Both matter: with a
        # non-tty stdin the shell short-circuits at "needs elevation; run
        # from a terminal" and never reaches update_apply, so the bug cannot
        # show. An earlier version of this test did exactly that and passed
        # against the broken build.
        text = run_update_pty(installed, pub.env(home))

        # The core of #75: whatever happens, it must not be blamed on the
        # download. Staging in an unwritable directory is a permission
        # problem and has to be reported as one.
        check("a root-installed binary is not reported as a download failure",
              "the download failed" not in text,
              "still misdiagnosed:\n    " + text.strip()[:300])

        # And the shell must say that elevation is what is needed.
        check("the message names elevation as the reason",
              ("elevation" in text or "sudo" in text or "not writable" in text),
              "got:\n    " + text.strip()[:300])

        # Never leave litter in a directory we could not write anyway.
        stray = [f for f in os.listdir(bindir) if f != "hellish"]
        check("no temp file is left in the install directory",
              stray == [], "found %r" % stray)

        # The installed binary must be untouched on every failure path.
        check("the installed binary is unchanged",
              os.path.isfile(installed)
              and os.stat(installed).st_size == os.stat(SHELL).st_size,
              "the binary was damaged by a failed update")
    finally:
        pub.stop()
        os.chmod(bindir, 0o755)
        shutil.rmtree(tmp, ignore_errors=True)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
