#!/usr/bin/env python3
"""3.4.1: an install an old updater upgraded in place gets its polish command.

Every <= 3.3.0 updater replaces the file it knows, .../hellish, with the 3.4
binary and has never heard of polish -- so after `update` the user still had
no `polish` (reported 2026-11-20: "hellish: polish: command not found"). The
updater that did the install cannot be changed; the binary it installed can.
So the 3.4.1 binary adds `polish -> hellish` beside itself:

  * `update` / `update --now`: a user-owned dir gets the link at once (before
    the network, so offline too); a root-owned one is asked about [y/N] on a
    terminal, else the `sudo ln -s` line is printed. `--check`/`--version`
    stay read-only.
  * the welcome banner: a user-owned dir gets the link silently; a
    root-owned one shows "no polish command — run update" in the footer.
  * never: overwrite an existing polish, act when the binary is already
    named polish.

Hermetic: a throwaway HOME and copies of the build in temp dirs; the update
API points at a closed local port. Usage: update_brand_test.py <shell>
"""
import fcntl
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
import time

SHELL = os.path.realpath(sys.argv[1] if len(sys.argv) > 1
                         else "build/bin/polish")
ESC = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]|\x1b\][^\x07]*\x07|\r")
FAILS = []


def check(name, ok, detail=""):
    print("  %s %s" % ("\033[32m✓\033[0m" if ok else "\033[31m✗\033[0m", name))
    if not ok:
        FAILS.append(name)
        if detail:
            print("      " + str(detail).replace("\n", "\n      ")[:900])


ROOT = tempfile.mkdtemp(prefix="update_brand_")
HOME = os.path.join(ROOT, "home")
os.makedirs(HOME)


def env(extra=None):
    e = {"HOME": HOME, "PATH": "/usr/bin:/bin", "TERM": "xterm-256color",
         "LANG": "C.UTF-8", "XDG_CACHE_HOME": os.path.join(HOME, ".cache"),
         "ASAN_OPTIONS": "detect_leaks=0", "HELLISH_NO_UPDATE_CHECK": "1",
         "HELLISH_UPDATE_API": "http://127.0.0.1:9/never",
         "HELLISH_UPDATE_DL": "http://127.0.0.1:9/never"}
    if extra:
        e.update(extra)
    return e


def install(name, case):
    """A copy of the build at <tmp>/<case>/bin/<name>, as a release left it."""
    d = os.path.join(ROOT, case, "bin")
    os.makedirs(d)
    shutil.copy2(SHELL, os.path.join(d, name))
    return d


def run(binary, cmd, stdin=subprocess.DEVNULL):
    return subprocess.run([binary, "-c", cmd], env=env(), stdin=stdin,
                          capture_output=True, text=True, timeout=30)


def pty_session(binary, extra=None, answer=None, cmd=None, settle=2.5):
    """Interactive binary on a pty. `cmd` is typed once the prompt settles,
    `answer` once more after that (the [y/N] prompt). Returns the screen."""
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env(extra))
        os.chdir(HOME)
        os.execv(binary, [binary])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 40, 150, 0, 0))
    out = b""
    for send in (None, cmd, answer):
        if send is not None:
            os.write(fd, send)
        end = time.time() + settle
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.05)
            if r:
                try:
                    out += os.read(fd, 65536)
                except OSError:
                    break
    try:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    except OSError:
        pass
    return ESC.sub("", out.decode(errors="replace"))


def is_link_to_hellish(p):
    return os.path.islink(p) and os.readlink(p) == "hellish"


def main():
    print("update adds the polish command (%s)" % SHELL)

    d = install("hellish", "user")
    link = os.path.join(d, "polish")
    r = run(os.path.join(d, "hellish"), "update")
    check("user dir: `update` adds polish -> hellish",
          is_link_to_hellish(link), r.stdout + r.stderr)
    check("...and says so", "added the polish command" in r.stdout, r.stdout)
    check("...even offline (the release source is unreachable)",
          "could not reach" in r.stderr, r.stderr)
    r = subprocess.run([link, "-c", "update --version"], env=env(),
                       capture_output=True, text=True, timeout=30)
    check("...and `polish` runs the same binary",
          r.stdout.startswith("polish "), r.stdout + r.stderr)
    r = run(os.path.join(d, "hellish"), "update")
    check("a second `update` is a no-op",
          is_link_to_hellish(link) and "added the polish" not in r.stdout,
          r.stdout)

    d = install("hellish", "now")
    run(os.path.join(d, "hellish"), "update --now")
    check("`update --now` adds it too",
          is_link_to_hellish(os.path.join(d, "polish")))

    d = install("hellish", "readonly_flags")
    for flag in ("--check", "--version"):
        run(os.path.join(d, "hellish"), "update " + flag)
        check("`update %s` adds nothing" % flag,
              not os.path.lexists(os.path.join(d, "polish")))

    d = install("hellish", "taken")
    with open(os.path.join(d, "polish"), "w") as f:
        f.write("#!/bin/sh\necho mine\n")
    run(os.path.join(d, "hellish"), "update")
    check("an existing polish is never replaced",
          not os.path.islink(os.path.join(d, "polish"))
          and open(os.path.join(d, "polish")).read().endswith("mine\n"))

    d = install("polish", "newname")
    run(os.path.join(d, "polish"), "update")
    check("a binary already named polish adds nothing",
          os.listdir(d) == ["polish"], os.listdir(d))

    d = install("hellish", "banner_user")
    out = pty_session(os.path.join(d, "hellish"), {"HELLISH_BANNER": "1"})
    check("user dir: the banner adds it, silently",
          is_link_to_hellish(os.path.join(d, "polish"))
          and "no polish command" not in out, out[-600:])
    check("the banner is the 3.4 one", "What's new in 3.4" in out, out[-900:])
    check("the banner names no hellish (a new user never had it)",
          "hellish" not in out, out[-900:])

    if os.geteuid() == 0:
        print("  \033[33m~\033[0m skipped: root writes anywhere — the "
              "root-owned cases need a non-root run")
    else:
        root_owned_cases()

    shutil.rmtree(ROOT, ignore_errors=True)
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


def root_owned_cases():
    """A dir this user cannot write stands in for /usr/bin: no link can be
    made without sudo, so it must ask, or say what to run."""
    d = install("hellish", "system")
    link = os.path.join(d, "polish")
    os.chmod(d, 0o555)
    try:
        r = run(os.path.join(d, "hellish"), "update")
        check("root-owned dir, no terminal: prints the sudo line",
              ("sudo ln -s hellish " + link) in r.stderr, r.stderr)
        check("...and creates nothing", not os.path.lexists(link))
        out = pty_session(os.path.join(d, "hellish"), cmd=b"update\n",
                          answer=b"n\n")
        check("root-owned dir, terminal: asks [y/N] with the exact command",
              "would run: sudo ln -s hellish " + link in out
              and "[y/N]" in out, out[-900:])
        check("...a no leaves it alone and prints the sudo line",
              not os.path.lexists(link)
              and out.count("sudo ln -s hellish " + link) >= 2, out[-900:])
        out = pty_session(os.path.join(d, "hellish"), {"HELLISH_BANNER": "1"})
        check("...and its banner footer says to run update",
              "no polish command" in out and "run update" in out,
              out[-900:])
    finally:
        os.chmod(d, 0o755)


if __name__ == "__main__":
    main()
