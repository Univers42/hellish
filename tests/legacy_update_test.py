#!/usr/bin/env python3
"""An install from before the polish rename gets `polish` once it updates.

Reported 2026-11-20: a 3.2.0 install ran `update`, got 3.4.0 into
/usr/bin/hellish, and then `polish` was "command not found". Every <= 3.3.0
updater replaces the one file it knows and has never heard of polish; it
cannot be changed after the fact. The binary it installs can, so 3.4.1 adds
`polish -> hellish` beside itself (src/platform/posix/update_brand.c).

Three real installs, each by the real old installer from the published old
release (LEGACY_VERSION: v3.3.0, and v3.2.0 -- the reporter's), served
from 127.0.0.1 (see docker/Dockerfile.legacy-update):

  sami  system mode, sudo   /usr/bin/hellish, chsh'd to it
  bea   user mode, bash     ~/.local/bin/hellish + the ~/.bashrc exec hook
  zoe   user mode, zsh      ~/.local/bin/hellish + the ~/.zshrc exec hook

Each is then updated by its own old updater (`update --now`) to THIS
build, which must reproduce the report (no polish yet), and then reach a
polish command the way that user would:

  sami  the banner footer says "no polish command -- run update"; `update`
        asks [y/N] and runs `sudo ln -s hellish /usr/bin/polish`
  bea   logs in: the hook execs the new binary, whose banner adds it
  zoe   logs in and types `update`, which adds it

Finally every login still lands in the shell, and from there and from the
plain bash/zsh (hook off) `command -v polish` resolves on PATH.

Runs as root inside the container; every step runs as the user it is about.
"""
import fcntl
import hashlib
import http.server
import os
import pty
import pwd
import re
import select
import signal
import struct
import subprocess
import sys
import termios
import threading
import time

NEW = "/srv/new/polish"
OLDVER = os.environ.get("LEGACY_VERSION", "v3.3.0").lstrip("v")
ESC = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]|\x1b\][^\x07]*\x07|\r")
FAILS = []
AUTO = ((b"[y/N]", b"y\n"), (b"How should", b"\r"))


def check(name, ok, detail=""):
    print("  %s %s" % ("\033[32m✓\033[0m" if ok else "\033[31m✗\033[0m", name))
    if not ok:
        FAILS.append(name)
        if detail:
            print("      " + str(detail).replace("\n", "\n      ")[-1200:])


def section(t):
    print("\n\033[1m%s\033[0m" % t)


def sha(p):
    with open(p, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def new_version():
    out = subprocess.run([NEW, "-c", "update --version"], capture_output=True,
                         text=True, timeout=30).stdout.split()
    return out[1]


def serve(newver, blob, newsha):
    """/old/... is the published old release on disk; /new/... answers as
    the GitHub API and download host for THIS build; anything else is 404,
    so the old installer's refresh-from-main falls back to its bundle."""
    class H(http.server.BaseHTTPRequestHandler):
        def log_message(self, *a):
            pass

        def do_GET(self):
            p = self.path
            body = None
            if p.startswith("/old/") and ".." not in p:
                f = "/srv" + p
                if os.path.isfile(f):
                    body = open(f, "rb").read()
            elif p == "/new/releases/latest":
                body = ('{"tag_name": "v%s"}' % newver).encode()
            elif p.startswith("/new/download/v%s/" % newver):
                asset = p.rsplit("/", 1)[1]
                if asset.endswith(".sha256"):
                    body = ("%s  %s\n" % (newsha, asset[:-7])).encode()
                else:
                    body = blob
            if body is None:
                self.send_response(404)
                self.end_headers()
                return
            self.send_response(200)
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

    srv = http.server.ThreadingHTTPServer(("127.0.0.1", 0), H)
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return "http://127.0.0.1:%d" % srv.server_address[1]


def uenv(user, base, extra=None):
    pw = pwd.getpwnam(user)
    e = {"HOME": pw.pw_dir, "USER": user, "LOGNAME": user,
         "SHELL": pw.pw_shell, "TERM": "xterm-256color", "LANG": "C.UTF-8",
         "PATH": "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:"
                 "/bin",
         "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_NO_ANIM": "1",
         "HELLISH_UPDATE_API": base + "/new/releases/latest",
         "HELLISH_UPDATE_DL": base + "/new/download"}
    if extra:
        e.update(extra)
    return e


def become(user):
    pw = pwd.getpwnam(user)
    os.setgroups([])
    os.setgid(pw.pw_gid)
    os.setuid(pw.pw_uid)
    os.chdir(pw.pw_dir)


def run_as(user, argv, env):
    return subprocess.run(argv, env=env, stdin=subprocess.DEVNULL,
                          capture_output=True, text=True, timeout=300,
                          preexec_fn=lambda: become(user))


def drive(user, argv, env, typed=(), settle=2.5, timeout=120):
    """`argv` as `user` on a real pty. Each of `typed` is sent once output
    has been quiet for `settle` seconds; [y/N] and the origin menu are
    answered as they appear. argv[0] is the path, argv[1:] the real argv
    (so "-bash" makes a login shell). Returns the de-escaped screen."""
    pid, fd = pty.fork()
    if pid == 0:
        become(user)
        os.execve(argv[0], argv[1:], env)
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 40, 150, 0, 0))
    out, seen, queue = b"", {}, list(typed)
    last, end = time.time(), time.time() + timeout
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.2)
        if r:
            try:
                d = os.read(fd, 65536)
            except OSError:
                break
            if not d:
                break
            out += d
            last = time.time()
            for pat, resp in AUTO:
                if out.count(pat) > seen.get(pat, 0):
                    seen[pat] = out.count(pat)
                    os.write(fd, resp)
            continue
        if time.time() - last > settle:
            if not queue:
                break
            os.write(fd, queue.pop(0))
            last = time.time()
    try:
        os.kill(pid, signal.SIGKILL)
        os.waitpid(pid, 0)
    except OSError:
        pass
    return ESC.sub("", out.decode(errors="replace"))


def login_argv(user):
    sh = pwd.getpwnam(user).pw_shell
    return [sh, "-" + os.path.basename(sh)]


def old_install(user, mode, base):
    r = run_as(user, ["sh", "/srv/install-old.sh", "--yes", "--plugins=none"],
               uenv(user, base, {"HELLISH_INSTALL_MODE": mode,
                                 "HELLISH_RELEASE_BASE": base + "/old",
                                 "HELLISH_RAW_BASE": base + "/noraw"}))
    check("the real v%s installer succeeds (%s mode)" % (OLDVER, mode),
          r.returncode == 0, r.stdout + r.stderr)


def scenario(user, mode, bindir, base, newver, newsha):
    hel, pol = bindir + "/hellish", bindir + "/polish"
    section("%s: %s install, login %s" % (user, mode,
                                           pwd.getpwnam(user).pw_shell))
    old_install(user, mode, base)
    v = run_as(user, [hel, "-c", "echo $BASH_VERSION; update --version"],
               uenv(user, base)).stdout
    check("%s is the published %s" % (hel, OLDVER), OLDVER in v, v)
    check("...and there is no polish yet", not os.path.lexists(pol))
    if mode == "system":
        check("...and it is the login shell (chsh)",
              pwd.getpwnam(user).pw_shell == hel, pwd.getpwnam(user).pw_shell)
    else:
        rc = os.path.join(pwd.getpwnam(user).pw_dir, ".bashrc" if
                          user == "bea" else ".zshrc")
        check("...and %s execs it" % rc,
              ('exec "%s"' % hel) in open(rc).read())

    out = drive(user, [hel, hel, "-c", "update --now"], uenv(user, base))
    check("the %s updater installs %s" % (OLDVER, newver),
          sha(hel) == newsha and "update failed" not in out, out)
    check("...into the hellish path it knows, adding no polish (the report)",
          not os.path.lexists(pol))
    return hel, pol


def finish(user, bindir, base, newver):
    pol = bindir + "/polish"
    check("%s is a symlink to hellish" % pol,
          os.path.islink(pol) and os.readlink(pol) == "hellish",
          os.path.lexists(pol) and os.readlink(pol))
    if not os.path.lexists(pol):
        return
    r = run_as(user, [pol, "-c", "update --version"], uenv(user, base))
    check("`polish` runs %s" % newver,
          r.stdout.strip() == "polish " + newver, r.stdout + r.stderr)
    out = drive(user, login_argv(user), uenv(user, base,
                                             {"HELLISH_BANNER": "0"}),
                typed=(b"command -v polish; update --version\n", b"exit\n"))
    check("a login lands in the shell, with polish on PATH",
          pol in out and ("polish " + newver) in out, out)
    if pwd.getpwnam(user).pw_shell.endswith("/hellish"):
        return
    out = drive(user, login_argv(user), uenv(user, base,
                                             {"HELLISH_NO_EXEC": "1"}),
                typed=(b"command -v polish\n", b"exit\n"))
    check("the plain %s (hook off) finds polish on PATH too"
          % os.path.basename(pwd.getpwnam(user).pw_shell), pol in out, out)


def main():
    if os.geteuid() != 0 or not os.path.isfile("/srv/install-old.sh"):
        print("run me as root, inside docker/Dockerfile.legacy-update")
        sys.exit(2)
    newver = new_version()
    blob = open(NEW, "rb").read()
    newsha = hashlib.sha256(blob).hexdigest()
    base = serve(newver, blob, newsha)
    print("this build is %s; the old installs are the published v%s"
          % (newver, OLDVER))

    scenario("sami", "system", "/usr/bin", base, newver, newsha)
    out = drive("sami", ["/usr/bin/hellish", "hellish"],
                uenv("sami", base, {"HELLISH_BANNER": "1"}),
                typed=(b"update\n", b"exit\n"))
    check("sami: the banner footer says to run update",
          "no polish command" in out, out)
    check("sami: `update` asks, then runs sudo ln -s",
          "sudo ln -s hellish /usr/bin/polish" in out
          and "added the polish command" in out, out)
    finish("sami", "/usr/bin", base, newver)

    scenario("bea", "user", "/home/bea/.local/bin", base, newver, newsha)
    drive("bea", login_argv("bea"), uenv("bea", base), typed=(b"exit\n",))
    check("bea: logging in once adds it (the banner, silently)",
          os.path.islink("/home/bea/.local/bin/polish"))
    finish("bea", "/home/bea/.local/bin", base, newver)

    scenario("zoe", "user", "/home/zoe/.local/bin", base, newver, newsha)
    out = drive("zoe", login_argv("zoe"),
                uenv("zoe", base, {"HELLISH_BANNER": "0"}),
                typed=(b"update\n", b"exit\n"))
    check("zoe: `update` adds it and says so",
          "added the polish command" in out, out)
    finish("zoe", "/home/zoe/.local/bin", base, newver)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
