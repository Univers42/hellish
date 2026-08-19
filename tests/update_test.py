#!/usr/bin/env python3
"""The update path, driven end to end against a LOCAL fake release server.

Issue #20 §12 is explicit that this must not be tested by compiling
development builds into the system. So the whole flow is pointed at a
throwaway HTTP server via HELLISH_UPDATE_API / HELLISH_UPDATE_DL: the
shell does real discovery, a real download over a real socket, a real
sha256 check and a real atomic replace -- only the publisher is fake.

Covered (numbering follows the issue):
  1  no update           latest == installed  -> no notification
  2  update available    latest >  installed  -> offered
  4  source unreachable  -> shell unaffected, no ugly error
  5  no sudo             user-writable target -> update succeeds
  7  corrupt release     wrong checksum       -> REJECTED, binary intact
  8  interrupted update  truncated asset      -> binary intact
 10  after installing    the new binary reports the new version
 plus: startup is not delayed by a hanging update server.

Usage: python3 tests/update_test.py [path/to/hellish]
"""
import hashlib
import http.server
import os
import shutil
import subprocess
import sys
import tempfile
import threading
import time

ROOT = os.path.dirname(os.path.abspath(__file__))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "..", "build", "bin", "hellish"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


class Publisher:
    """A stand-in for GitHub Releases: metadata, an asset, its checksum."""

    def __init__(self, tag, payload, sha_override=None, hang=False):
        self.tag = tag
        self.payload = payload
        self.sha = sha_override or hashlib.sha256(payload).hexdigest()
        self.hang = hang
        self.asset = "hellish-linux-" + os.uname().machine
        pub = self

        class H(http.server.BaseHTTPRequestHandler):
            def log_message(self, *a):
                pass

            def do_GET(self):
                if pub.hang:
                    time.sleep(30)
                    return
                if self.path.endswith("/releases/latest"):
                    body = ('{"tag_name": "v%s", "name": "r"}'
                            % pub.tag).encode()
                elif self.path.endswith(".sha256"):
                    body = ("%s  %s\n" % (pub.sha, pub.asset)).encode()
                elif self.path.endswith(pub.asset):
                    body = pub.payload
                else:
                    self.send_error(404)
                    return
                self.send_response(200)
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

        self.srv = http.server.HTTPServer(("127.0.0.1", 0), H)
        self.port = self.srv.server_address[1]
        threading.Thread(target=self.srv.serve_forever, daemon=True).start()

    def env(self, home):
        base = "http://127.0.0.1:%d" % self.port
        return {
            "HOME": home,
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "HELLISH_UPDATE_API": base + "/releases/latest",
            "HELLISH_UPDATE_DL": base + "/download",
            "HELLISH_NO_BANNER": "1", "HELLISH_NO_ANIM": "1",
            "HELLISH_UPDATE_TTL": "0",
            "ASAN_OPTIONS": "detect_leaks=0", "TERM": "dumb",
        }

    def stop(self):
        self.srv.shutdown()


def run(binary, env, args, timeout=30, stdin=b""):
    p = subprocess.run([binary] + args, env=env, input=stdin,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       timeout=timeout)
    return p.returncode, p.stdout.decode("utf-8", "replace")


def installed_copy(home):
    """A user-writable install, the no-sudo case of the issue."""
    d = os.path.join(home, ".local", "bin")
    os.makedirs(d, exist_ok=True)
    dest = os.path.join(d, "hellish")
    shutil.copy2(SHELL, dest)
    return dest


def version_of(binary, env):
    _, out = run(binary, env, ["-c", "update --version"])
    return out.strip().split()[-1]


def main():
    home = tempfile.mkdtemp(prefix="hellish_upd_")
    binary = installed_copy(home)
    cur = version_of(binary, {"HOME": home, "PATH": os.environ["PATH"],
                              "HELLISH_NO_BANNER": "1"})
    # A real, runnable dummy release: the same binary with its version
    # string overwritten in place by one of the SAME LENGTH. The updater
    # insists a download reports the version it was advertised as, so the
    # fake release has to genuinely report it -- a copy that still said the
    # old version is exactly what that check exists to reject.
    newer = "9" + cur[1:]
    if newer == cur:
        newer = "8" + cur[1:]
    payload = open(SHELL, "rb").read().replace(cur.encode(), newer.encode())
    check("dummy release binary was actually re-stamped",
          payload != open(SHELL, "rb").read())

    # 1 -- nothing newer published
    pub = Publisher(cur, payload)
    rc, out = run(binary, pub.env(home), ["-c", "update"])
    check("no update: reports latest, exit 0", rc == 0 and "latest" in out,
          repr(out[:200]))
    check("no update: offers nothing", "[Update]" not in out, repr(out[:200]))
    pub.stop()

    # 2 -- a newer release is published
    pub = Publisher(newer, payload)
    rc, out = run(binary, pub.env(home), ["-c", "update"])
    check("update available: announced", newer in out and "available" in out,
          repr(out[:200]))

    # 5 + 10 -- install it without sudo, new binary reports the new version
    rc, out = run(binary, pub.env(home), ["-c", "update --now"])
    check("no-sudo install succeeds", rc == 0 and "updated" in out,
          repr(out[:300]))
    check("the installed binary now reports the new version",
          os.access(binary, os.X_OK)
          and version_of(binary, pub.env(home)) == newer,
          "got %s want %s" % (version_of(binary, pub.env(home)), newer))
    check("no temp file left behind",
          not os.path.exists(binary + ".hellish-update"))
    pub.stop()

    # 7 -- the checksum does not match what was downloaded.
    # The installed copy is now `newer`, so these cases have to advertise
    # something higher again or the updater rightly says "already latest".
    before = open(binary, "rb").read()
    higher = newer.split(".")[0] + "." + "9" * len(newer.split(".")[1]) \
        + "." + "9" * len(newer.split(".")[2])
    pub = Publisher(higher, payload, sha_override="0" * 64)
    rc, out = run(binary, pub.env(home), ["-c", "update --now"])
    check("bad checksum is rejected", rc != 0 and "checksum" in out,
          repr(out[:300]))
    check("bad checksum leaves the binary untouched",
          open(binary, "rb").read() == before)
    pub.stop()

    # 8 -- a truncated download must not be installed
    pub = Publisher(higher, payload[:512])
    rc, out = run(binary, pub.env(home), ["-c", "update --now"])
    check("truncated download is rejected", rc != 0, repr(out[:300]))
    check("truncated download leaves the binary untouched",
          open(binary, "rb").read() == before)
    pub.stop()

    # 4 -- the release source is simply not there
    env = {"HOME": home, "PATH": os.environ["PATH"],
           "HELLISH_UPDATE_API": "http://127.0.0.1:9/nope",
           "HELLISH_NO_BANNER": "1", "ASAN_OPTIONS": "detect_leaks=0"}
    rc, out = run(binary, env, ["-c", "update"])
    check("unreachable source: clean message, no curl noise",
          "could not reach" in out and "curl:" not in out, repr(out[:200]))
    rc, out = run(binary, env, ["-c", "echo alive"])
    check("unreachable source: shell still works", out.strip() == "alive")

    # startup must not wait on a hanging update server
    pub = Publisher(newer, payload, hang=True)
    t0 = time.time()
    rc, out = run(binary, pub.env(home), ["-c", "echo fast"], timeout=20)
    dt = time.time() - t0
    check("a hanging update server does not delay the shell",
          out.strip() == "fast" and dt < 3.0, "took %.1fs" % dt)
    pub.stop()

    shutil.rmtree(home, ignore_errors=True)
    print("\n%d check(s) failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
