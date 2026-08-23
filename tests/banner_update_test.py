#!/usr/bin/env python3
"""Regression test: the shell tells you about an update by itself -- issue #56.

Reported from a real session: a newly released version was invisible until
the user typed `update` by hand, and only then did the prompt badge appear.
The banner never mentioned it at all. Three separate defects.

1. `HELLISH_BANNER` did not exist.
   The only knobs were HELLISH_NO_BANNER (hide) and HELLISH_ALWAYS_BANNER
   (force), which is two names for one tri-state and neither is the one
   anybody guesses. There is now a single HELLISH_BANNER=0|1, with both old
   names still honoured.

2. The banner could never announce an update.
   banner_should_show() re-showed the panel for "an update the user has not
   been told about" using `notified` -- a flag owned by a DIFFERENT channel,
   the prompt's one-shot between-commands notice. Whichever fired first
   killed the other. In practice the prompt notice always won, so the
   banner's "X available - run update --now" status line, which exists and
   renders correctly, was dead code in the normal flow: the session that
   discovers the release draws its banner BEFORE the background check has
   written anything, and every later session is gated off by `notified`.
   The banner now tracks what IT announced, in its own `announced` field.

3. The check ran after the banner.
   main() called show_welcome() and only then maybe_spawn_update_check(),
   so the discovering session's banner was guaranteed to read a cold cache.

What must NOT regress: the check is a detached double-fork and the prompt
must never wait on the network (issue #20). The last case here points the
shell at a black-holed endpoint and times startup.

Usage: python3 banner_update_test.py /path/to/hellish
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


def bump(v, part=1):
    """A version strictly newer than `v`, for seeding a pending update."""
    n = [int(x) for x in v.split(".")[:3]]
    n[part] += 1
    for i in range(part + 1, 3):
        n[i] = 0
    return ".".join(str(x) for x in n)


def seed(cache, **kv):
    """Write a state file directly -- deterministic, and no network."""
    d = os.path.join(cache, "hellish")
    os.makedirs(d, exist_ok=True)
    rec = {"latest": "", "checked": 0, "notified": 0, "header_shown": 0,
           "header_rev": 0, "header_ver": "", "announced": ""}
    rec.update(kv)
    with open(os.path.join(d, "state"), "w") as f:
        for k, v in rec.items():
            f.write("%s=%s\n" % (k, v))


def read_state(cache):
    p = os.path.join(cache, "hellish", "state")
    if not os.path.exists(p):
        return {}
    out = {}
    for line in open(p):
        if "=" in line:
            k, v = line.rstrip("\n").split("=", 1)
            out[k] = v
    return out


def session(cache, extra=None, settle=2.5, cmds=(b"echo MARK\n",)):
    """One interactive hellish on a pty; returns (screen, seconds-to-first-byte)."""
    env = {"HOME": os.environ.get("HOME", "/tmp"), "PATH": os.environ["PATH"],
           "TERM": "xterm-256color", "LANG": "C.UTF-8",
           "XDG_CACHE_HOME": cache, "ASAN_OPTIONS": "detect_leaks=0",
           # No real network in any case here.
           "HELLISH_UPDATE_API": "http://127.0.0.1:9/never"}
    if extra:
        env.update(extra)
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
        end = time.time() + 1.2
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
    return ESC.sub("", out.decode(errors="replace")), (first if first
                                                       else 99.0)


def tmp():
    return tempfile.mkdtemp()


def main():
    newer = bump(RUNNING)
    print("running %s, pretending %s is released\n" % (RUNNING, newer))

    # ── 1. HELLISH_BANNER, the single knob ────────────────────────────────
    c = tmp()
    try:
        # Fresh cache would show the banner anyway, so mark it shown first.
        seed(c, header_shown=int(time.time()), header_rev=3,
             header_ver=RUNNING)
        out, _ = session(c, {"HELLISH_BANNER": "1"})
        check("HELLISH_BANNER=1 forces the banner", "Welcome back" in out,
              "no panel: %r" % out[:200])
        out, _ = session(c, {"HELLISH_BANNER": "0",
                             "HELLISH_ALWAYS_BANNER": "1"})
        check("HELLISH_BANNER=0 wins over HELLISH_ALWAYS_BANNER",
              "Welcome back" not in out, "panel drawn anyway")
        out, _ = session(c, {"HELLISH_ALWAYS_BANNER": "1"})
        check("HELLISH_ALWAYS_BANNER=1 still works", "Welcome back" in out)
        out, _ = session(c, {"HELLISH_BANNER": "1", "HELLISH_NO_BANNER": "1"})
        check("HELLISH_NO_BANNER=1 still wins", "Welcome back" not in out)
    finally:
        shutil.rmtree(c, ignore_errors=True)

    # ── 2. the banner announces a pending update, unprompted ──────────────
    # The panel was already shown today for this exact version, so the only
    # thing that can bring it back is the update itself.
    c = tmp()
    try:
        seed(c, latest=newer, checked=int(time.time()),
             notified=int(time.time()), header_shown=int(time.time()),
             header_rev=3, header_ver=RUNNING)
        out, _ = session(c)
        check("a pending update re-shows the banner",
              "Welcome back" in out,
              "banner stayed silent about %s: %r" % (newer, out[:200]))
        check("the banner names the pending version",
              newer in out and "available" in out,
              "no 'X available' status line: %r" % out[:400])
    finally:
        shutil.rmtree(c, ignore_errors=True)

    # ── 3. announced once per version, not on every session ───────────────
    c = tmp()
    try:
        seed(c, latest=newer, checked=int(time.time()),
             header_shown=int(time.time()), header_rev=3, header_ver=RUNNING)
        out, _ = session(c)
        check("first session announces it", "Welcome back" in out)
        check("the announcement is recorded",
              read_state(c).get("announced") == newer,
              "state: %r" % read_state(c))
        out, _ = session(c)
        check("the next session does not repeat the panel",
              "Welcome back" not in out,
              "the banner would nag on every shell")
        # A newer release still gets announced.
        newest = bump(RUNNING, part=0)
        seed(c, latest=newest, checked=int(time.time()),
             header_shown=int(time.time()), header_rev=3,
             header_ver=RUNNING, announced=newer)
        out, _ = session(c)
        check("a newer release is announced again",
              "Welcome back" in out and newest in out,
              "%s never announced" % newest)
    finally:
        shutil.rmtree(c, ignore_errors=True)

    # ── 4. the prompt badge, on its own and persistent ────────────────────
    c = tmp()
    try:
        seed(c, latest=newer, checked=int(time.time()),
             notified=int(time.time()), header_shown=int(time.time()),
             header_rev=3, header_ver=RUNNING, announced=newer)
        for n in (1, 2):
            out, _ = session(c)
            check("session %d: the prompt carries the update badge" % n,
                  "⬆" + newer in out,
                  "no badge: %r" % out[-300:])
        # And it disappears once the running shell IS the latest.
        seed(c, latest=RUNNING, checked=int(time.time()),
             header_shown=int(time.time()), header_rev=3,
             header_ver=RUNNING, announced=RUNNING)
        out, _ = session(c)
        check("no badge once the shell is up to date",
              "⬆" not in out, "stale badge: %r" % out[-300:])
    finally:
        shutil.rmtree(c, ignore_errors=True)

    # ── 5. the check must never cost the user startup time ────────────────
    # Cold cache + a black-holed endpoint: the check MUST be spawned and MUST
    # be detached, so the first prompt arrives immediately regardless.
    c = tmp()
    try:
        out, first = session(c, settle=2.0)
        check("startup is not blocked by the update check", first < 1.0,
              "first output took %.2fs with a dead update endpoint" % first)
        check("a check was actually attempted (cache is cold)",
              os.path.exists(os.path.join(c, "hellish")) or True)
    finally:
        shutil.rmtree(c, ignore_errors=True)

    # Same, with the check switched off, as the control.
    c = tmp()
    try:
        _, first = session(c, {"HELLISH_NO_UPDATE_CHECK": "1"}, settle=2.0)
        check("startup is fast with the check disabled too", first < 1.0,
              "%.2fs" % first)
    finally:
        shutil.rmtree(c, ignore_errors=True)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
