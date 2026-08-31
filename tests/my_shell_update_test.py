#!/usr/bin/env python3
"""`make my_shell`, then the update button -- end to end. Issue #76.

WHAT MAX REPORTED. hellish 2.7.2 installed with `make my_shell`, a 2.7.6
release waiting, the update accepted -- and:

    hellish: /usr/bin/hellish is owned by another user.
      hellish would run: sudo install -m 755 <verified download> /usr/bin/hellish
      proceed? [y/N] y
    hellish: installing 2.7.6 into /usr/bin/hellish…
    hellish: update failed — the download failed
      the installed binary is unchanged.

The first guess was that `make my_shell` had put the binary somewhere PATH
could not reach. It had not, and this test asserts that outright: after
`make my_shell` the binary is exactly where register_shell.sh says, root
owned, listed in /etc/shells, and `command -v hellish` finds it. $PATH was
never the problem.

The problem was a DIFFERENT path -- the one update_apply stages the download
on. It used `<target>.hellish-update`, i.e. inside /usr/bin, so curl was
asked to create a file in a directory the user cannot write; it died at the
download step and blamed the network (#75). Then, once staging was fixed,
`sudo install`'s result was read from the wrong thing entirely -- see
tests/update_sudo_fail_test.py -- so a refused password reported success.

Both are asserted below, on a real root-owned /usr/bin, with a real sudo
that really wants a password.

RUNS AS max, INSIDE docker/Dockerfile.my-shell. Not runnable on a developer
host, by design: it runs `make my_shell`, which rewrites /etc/shells and the
caller's login shell.

Usage: python3 my_shell_update_test.py     (as the non-root user, in the image)
"""
import hashlib
import http.server
import os
import pty
import select
import shutil
import subprocess
import sys
import threading
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEST = "/usr/bin/hellish"
PASSWORD = "hunter2"
FAILS = []


def check(name, ok, detail=""):
    mark = "\033[32mok\033[0m  " if ok else "\033[31mFAIL\033[0m"
    print("  %s %s" % (mark, name), flush=True)
    if not ok:
        if detail:
            print("       %s" % str(detail).replace("\n", "\n       "))
        FAILS.append(name)


def section(title):
    print("\n\033[1;36m▸\033[0m \033[1m%s\033[0m" % title, flush=True)


class Publisher:
    """A local stand-in for the GitHub release endpoints."""

    def __init__(self, version, payload):
        self.sha = hashlib.sha256(payload).hexdigest()
        blob, sha, ver = payload, self.sha, version

        class H(http.server.BaseHTTPRequestHandler):
            def log_message(self, *a):
                pass

            # A dropped connection is the EXPECTED outcome here, not a fault:
            # the v2.7.2 reproduction has curl die mid-download because it
            # cannot write into /usr/bin, which hangs up on us. Letting
            # socketserver print its BrokenPipeError traceback puts a wall of
            # red in the middle of a passing run, and a test that looks
            # broken when it passes is a test people stop believing.
            def handle_one_request(self):
                try:
                    http.server.BaseHTTPRequestHandler.handle_one_request(self)
                except (BrokenPipeError, ConnectionResetError):
                    self.close_connection = True

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

    def env(self):
        e = dict(os.environ)
        e["HELLISH_UPDATE_API"] = (
            "http://127.0.0.1:%d/releases/latest" % self.port)
        e["HELLISH_UPDATE_DL"] = "http://127.0.0.1:%d/download" % self.port
        e["HELLISH_NO_BANNER"] = "1"
        e["HELLISH_NO_ANIM"] = "1"
        return e

    def stop(self):
        self.srv.shutdown()


def run_update_pty(binary, env, password, timeout=180):
    """Drive `update --now` to the end through a REAL pty.

    Three interactions, and every one of them has to be answered or the run
    stalls somewhere that looks like a hang:

      1. the origin menu -- "system binary" is preselected, so Enter takes it
      2. "proceed? [y/N]" -- the elevation confirmation
      3. "[sudo] password for max:" -- sudo asks on /dev/tty, so it reaches
         this pty even though the updater does not route it

    A pipe cannot stand in for any of this: update_selfupdate() refuses to
    elevate at all when stdin is not a tty.
    """
    pid, fd = pty.fork()
    if pid == 0:
        os.execve(binary, [binary, "-c", "update --now"], env)
        os._exit(1)
    out = b""
    end = time.time() + timeout
    picked = confirmed = False
    pw_seen = 0
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
        n = out.count(b"password for")
        if n > pw_seen:
            pw_seen = n
            time.sleep(0.2)
            os.write(fd, password.encode() + b"\n")
            continue
        if b"update failed" in out or b"updated " in out:
            time.sleep(0.5)
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


def sh(cmd, **kw):
    return subprocess.run(cmd, shell=True, capture_output=True, text=True,
                          **kw)


def sha_of(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def version_of(path, env=None):
    r = subprocess.run([path, "-c", "update --version"],
                       capture_output=True, text=True, env=env)
    return r.stdout.strip().split()[-1] if r.stdout.strip() else ""


def have_network():
    """Can we reach the release API? Only the VERSION= phase needs it."""
    return subprocess.run(
        ["curl", "-fsS", "--max-time", "10", "-o", os.devnull,
         "https://api.github.com/repos/Univers42/hellish/releases/latest"],
        capture_output=True).returncode == 0


def prime_sudo():
    """Spend the password once, the way a human does.

    register_shell.sh calls sudo several times (install, tee, chsh) and make
    gives it no tty, so without a live timestamp the first one fails PAM and
    `make my_shell` dies half-installed. -S reads the password from stdin.
    """
    r = subprocess.run(["sudo", "-S", "-v"], input=PASSWORD + "\n",
                       capture_output=True, text=True)
    return r.returncode == 0


def main():
    # A hard stop, not politeness. This test runs `make my_shell`, which
    # installs into /usr/bin, edits /etc/shells and rewrites the caller's
    # passwd entry. Run by accident on a laptop it changes the login shell of
    # whoever typed it. The image sets this variable; nothing else does.
    if os.environ.get("HELLISH_MY_SHELL_TEST") != "1":
        print("refusing to run: this test installs a login shell and changes\n"
              "your passwd entry. Run it with `make my-shell-test`, which\n"
              "builds docker/Dockerfile.my-shell for exactly this purpose.")
        sys.exit(2)
    if os.geteuid() == 0:
        print("error: run as the non-root user (the image's CMD does)")
        sys.exit(2)

    section("make my_shell")
    if not prime_sudo():
        print("error: could not authenticate sudo")
        sys.exit(2)
    r = sh("make my_shell", cwd=ROOT)
    ok = r.returncode == 0
    check("`make my_shell` succeeds", ok,
          (r.stdout[-1500:] + r.stderr[-1500:]) if not ok else "")
    if not ok:
        print("\n%d checks failed" % len(FAILS))
        sys.exit(1)

    section("what my_shell actually installed")
    check("the binary is at %s" % DEST, os.path.isfile(DEST))
    st = os.stat(DEST)
    check("it is owned by root", st.st_uid == 0, "uid %d" % st.st_uid)
    check("it is mode 755", oct(st.st_mode & 0o777) == "0o755",
          oct(st.st_mode & 0o777))
    check("it runs", version_of(DEST) != "", "no version output")

    # The hypothesis this test exists to settle: PATH was never the problem.
    which = sh("command -v hellish").stdout.strip()
    check("`command -v hellish` finds it on PATH", which == DEST,
          "PATH resolved to %r, want %r (PATH=%s)"
          % (which, DEST, os.environ.get("PATH", "")))
    shells = open("/etc/shells").read().splitlines()
    check("it is listed in /etc/shells", DEST in shells, shells)
    passwd_shell = sh("getent passwd max").stdout.strip().split(":")[-1]
    check("it is max's login shell", passwd_shell == DEST, passwd_shell)

    # The condition that broke the update: /usr/bin is root's, so a staging
    # path inside it is unwritable, and the replacement needs elevation.
    check("the install directory is NOT writable by max",
          not os.access("/usr/bin", os.W_OK),
          "then this image cannot reproduce the report at all")

    # The guard-rails my_shell now prints at the end, and `make doctor` runs
    # on demand. On a clean install they must be QUIET -- a report that cries
    # wolf on a correct machine is one nobody reads on a broken one. /bin is a
    # symlink to /usr/bin here, so this also pins the dedup.
    doc = sh("./tools/register_shell.sh --doctor", cwd=ROOT)
    out = doc.stdout + doc.stderr
    check("doctor is clean after a correct install",
          doc.returncode == 0 and "no problems found" in out, out)
    check("doctor does not miscount /bin and /usr/bin as two installs",
          "exists 2 times" not in out, out)
    check("doctor says elevation will be needed for updates",
          "sudo password" in out, out)

    # Drop the sudo ticket my_shell was authorised with. Without this the
    # update below would inherit a live authentication, sudo would not ask
    # for anything, and the password handling under test would go unexercised
    # while every check still passed.
    subprocess.run(["sudo", "-k"], capture_output=True)
    check("the sudo ticket is dropped, so the update must authenticate again",
          subprocess.run(["sudo", "-n", "true"],
                         capture_output=True).returncode != 0)

    cur = version_of(DEST)
    newer = "9" + cur[1:]
    if newer == cur:
        newer = "8" + cur[1:]
    original = open(DEST, "rb").read()
    payload = original.replace(cur.encode(), newer.encode())
    if payload == original:
        print("error: could not re-stamp the version string")
        sys.exit(2)

    section("the update button, elevation REFUSED (wrong password)")
    pub = Publisher(newer, payload)
    try:
        before = sha_of(DEST)
        text = run_update_pty(DEST, pub.env(), "definitely-not-the-password")
        after = sha_of(DEST)
        # #75: a permission problem must never be reported as a network one.
        check("not misreported as a download failure",
              "the download failed" not in text, text[-500:])
        # #76: and a refused elevation must not be reported as a success.
        check("a refused elevation is not reported as an update",
              "updated " not in text, text[-500:])
        check("the failure is stated", "update failed" in text, text[-500:])
        check("sudo's own diagnosis reaches the user",
              "incorrect password" in text or "Sorry, try again" in text,
              text[-500:])
        check("the installed binary is untouched", before == after)
        check("nothing is left behind in /usr/bin",
              [f for f in os.listdir("/usr/bin") if "hellish" in f]
              == ["hellish"])
        check("nothing is left behind in /tmp",
              [f for f in os.listdir("/tmp") if "hellish-update" in f] == [])

        section("the update button, elevation GRANTED (real password)")
        before = sha_of(DEST)
        text = run_update_pty(DEST, pub.env(), PASSWORD)
        after = sha_of(DEST)
        check("the update completes",
              "updated " in text and "update failed" not in text, text[-500:])
        check("the INSTALLED binary really changed", before != after,
              "claimed success over an unchanged file")
        check("and it reports the new version",
              version_of(DEST) == newer,
              "reports %r, want %r" % (version_of(DEST), newer))
        check("it is still root-owned and executable",
              os.stat(DEST).st_uid == 0 and os.access(DEST, os.X_OK))
        check("nothing is left behind in /usr/bin",
              [f for f in os.listdir("/usr/bin") if "hellish" in f]
              == ["hellish"])
        check("nothing is left behind in /tmp",
              [f for f in os.listdir("/tmp") if "hellish-update" in f] == [])
    finally:
        pub.stop()

    section("make my-shell-purge")
    if not prime_sudo():
        print("error: could not re-authenticate sudo")
        sys.exit(2)
    r = sh("make my-shell-purge", cwd=ROOT)
    out = r.stdout + r.stderr
    check("`make my-shell-purge` succeeds", r.returncode == 0, out[-800:])
    check("the binary is gone", not os.path.exists(DEST))
    check("the login shell is restored, not left pointing at a missing file",
          sh("getent passwd max").stdout.strip().split(":")[-1] != DEST)
    check("the /etc/shells entry is gone",
          DEST not in open("/etc/shells").read().splitlines())
    home = os.path.expanduser("~")
    check("the config is purged",
          not os.path.exists(os.path.join(home, ".hellishrc"))
          and not os.path.isdir(os.path.join(home, ".config", "hellish"))
          and not os.path.isdir(os.path.join(home, ".cache", "hellish")))

    section("make my_shell VERSION=… installs a published release")
    # The reproduction loop itself, pinned. `make my_shell VERSION=2.7.2` is
    # how a maintainer stands where a reporter stood -- the bug in #76 lives
    # in the INSTALLED updater, so HEAD cannot reproduce it at any setting.
    # If this silently started installing the working tree instead, the loop
    # would look like it worked and prove nothing.
    #
    # This is the ONLY phase that needs the network: it downloads a real
    # published release. Everything above runs against a local fake server.
    # So it skips rather than fails when github is unreachable -- same rule
    # as the plugin corpus: a runner with no network passes honestly instead
    # of pretending to have checked.
    if not have_network():
        print("  \033[33m~\033[0m skipped: no network — cannot fetch a "
              "published release")
        print("\n%d checks failed" % len(FAILS))
        sys.exit(1 if FAILS else 0)
    if not prime_sudo():
        print("error: could not re-authenticate sudo")
        sys.exit(2)
    old = "2.7.2"
    r = sh("make my_shell VERSION=%s" % old, cwd=ROOT)
    out = r.stdout + r.stderr
    check("`make my_shell VERSION=%s` succeeds" % old, r.returncode == 0,
          out[-1200:])
    check("it installed the PUBLISHED %s, not the working tree" % old,
          version_of(DEST) == old,
          "installed %r, wanted %r" % (version_of(DEST), old))
    check("it says so, so nobody debugs the wrong binary",
          "installed the PUBLISHED" in out, out[-400:])
    check("and it is still a proper root-owned install",
          os.path.isfile(DEST) and os.stat(DEST).st_uid == 0
          and sh("getent passwd max").stdout.strip().split(":")[-1] == DEST)

    # VERSION= and STATIC=1 mean opposite things (fetch vs build); accepting
    # both would silently honour one of them.
    r = sh("make my_shell VERSION=%s STATIC=1" % old, cwd=ROOT)
    check("VERSION= and STATIC=1 are refused together",
          r.returncode != 0 and "mutually exclusive" in (r.stdout + r.stderr))

    section("and the old binary reproduces the original report")
    # The whole point of the loop: v2.7.2 must still fail the way Max's did,
    # with the false "download failed". If this ever passes, the reproduction
    # is no longer reproducing anything and this test should be re-read.
    pub2 = Publisher("9.9.9", payload)
    try:
        before = sha_of(DEST)
        text = run_update_pty(DEST, pub2.env(), PASSWORD)
        check("v%s still fails to update itself (the reported bug)" % old,
              "update failed" in text, text[-500:])
        check("...and still blames the download, which is the #75 symptom",
              "the download failed" in text, text[-500:])
        check("...leaving the installed binary untouched",
              before == sha_of(DEST))
    finally:
        pub2.stop()

    section("cleanup")
    if prime_sudo():
        sh("make my-shell-purge", cwd=ROOT)
    check("the machine is left clean", not os.path.exists(DEST))

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
