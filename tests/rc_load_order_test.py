#!/usr/bin/env python3
"""Regression test: the rc.d + plugin load path -- issue #70, issue #72 phase 2.

Before this, `~/.hellishrc` was the ONLY entry point. That is why every tool
that wanted to add something to a hellish session had to append to it, and
why the `# >>> managed by ... >>>` marker convention had to exist at all:
there was nowhere else to put anything.

    /etc/hellish/rc.d/*.hsh                 system-wide, lexical order
    $XDG_CONFIG_HOME/hellish/rc.d/*.hsh     yours, lexical order
    $XDG_CONFIG_HOME/hellish/plugins/*/plugin.hsh
    ~/.hellishrc                            LAST, so your own file wins

Three properties this pins, each of which is a bug if it breaks:

  * ORDER IS LEXICAL, not readdir order. readdir returns hash order on ext4,
    so without an explicit sort it is 10-env vs 20-aliases decided by the
    filesystem, differently on each machine. The whole 10-/20-/30- convention
    is worthless if the sort is missing, and it fails silently and
    intermittently, which is the worst way for it to fail.

  * ~/.hellishrc GOES LAST. It is the file people already have and already
    edit; a loader that lets a dropped-in plugin override it is a loader
    people switch off.

  * A PLUGIN CAN FIND ITSELF. ${BASH_SOURCE[0]} inside plugin.hsh names that
    file, so a plugin can load siblings and ship data next to its entry point
    instead of hardcoding $HOME/.hellish. Issue #71 calls this the single
    highest-leverage item for plugins.

The rc is only read for INTERACTIVE shells (the `metinp != INP_RL` guard), so
every case here drives a real pty -- `-c` deliberately reads none of it.

Usage: python3 rc_load_order_test.py [/path/to/hellish]
"""
import os
import pty
import select
import shutil
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(text)


def run_interactive(home, cmd, extra_args=()):
    """Start an interactive hellish with HOME=home, run one command, exit."""
    env = dict(os.environ, HOME=home, HELLISH_NO_BANNER="1",
               HELLISH_NO_UPDATE_CHECK="1", HELLISH_NO_ANIM="1", TERM="dumb")
    env.pop("XDG_CONFIG_HOME", None)
    pid, fd = pty.fork()
    if pid == 0:
        os.execve(SHELL, [SHELL] + list(extra_args), env)
        os._exit(1)
    out = b""
    end = time.time() + 6
    os.write(fd, cmd.encode() + b"\nexit\n")
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.2)
        if not r:
            continue
        try:
            d = os.read(fd, 65536)
        except OSError:
            break
        if not d:
            break
        out += d
    try:
        os.close(fd)
    except OSError:
        pass
    os.waitpid(pid, 0)
    return out.decode("utf-8", "replace")


def marker(out, tag):
    """Pull the one line we asked the shell to print."""
    for line in out.splitlines():
        if line.startswith(tag):
            return line[len(tag):].strip()
    return None


def build_home():
    home = tempfile.mkdtemp()
    cfg = os.path.join(home, ".config", "hellish")
    # deliberately created out of order, and named so that readdir order and
    # lexical order are unlikely to agree
    write(os.path.join(cfg, "rc.d", "30-c.hsh"), 'ORD="${ORD}c"\n')
    write(os.path.join(cfg, "rc.d", "10-a.hsh"), 'ORD="${ORD}a"\n')
    write(os.path.join(cfg, "rc.d", "20-b.hsh"), 'ORD="${ORD}b"\n')
    write(os.path.join(cfg, "plugins", "zed", "plugin.hsh"), 'ORD="${ORD}Z"\n')
    write(os.path.join(cfg, "plugins", "alpha", "plugin.hsh"),
          'ORD="${ORD}A"\n'
          'ALPHA_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"\n')
    write(os.path.join(home, ".hellishrc"), 'ORD="${ORD}rc"\n')
    return home, cfg


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    home, cfg = build_home()
    try:
        out = run_interactive(home, 'echo "ORD=$ORD"')
        got = marker(out, "ORD=")
        check("rc.d is lexical, plugins follow, ~/.hellishrc is LAST",
              got == "abcAZrc",
              "got %r want 'abcAZrc' (a,b,c = rc.d in order; A,Z = plugins; "
              "rc = ~/.hellishrc)" % got)

        out = run_interactive(home, 'echo "DIR=$ALPHA_DIR"')
        want = os.path.join(cfg, "plugins", "alpha")
        check("a plugin can locate its own directory",
              marker(out, "DIR=") == want,
              "got %r want %r" % (marker(out, "DIR="), want))

        # --norc must skip everything, including ~/.hellishrc. This is what
        # lets a test (or a bug report) pin a known configuration instead of
        # inheriting whatever the developer happens to have.
        out = run_interactive(home, 'echo "ORD=[$ORD]"', extra_args=("--norc",))
        check("--norc loads nothing at all", marker(out, "ORD=") == "[]",
              "got %r" % marker(out, "ORD="))

        # --rcfile replaces the whole path with exactly one file
        only = os.path.join(home, "only.hsh")
        write(only, 'ORD="ONLY"\n')
        out = run_interactive(home, 'echo "ORD=$ORD"',
                              extra_args=("--rcfile=" + only,))
        check("--rcfile=F loads F and nothing else",
              marker(out, "ORD=") == "ONLY", "got %r" % marker(out, "ORD="))

        # A missing tree is not an error -- most users have no ~/.config/hellish
        bare = tempfile.mkdtemp()
        out = run_interactive(bare, 'echo "OK=yes"')
        check("an absent config tree is not an error",
              marker(out, "OK=") == "yes", "got %r" % marker(out, "OK="))
        shutil.rmtree(bare, ignore_errors=True)

        # A plugin that fails must not take the shell down with it.
        write(os.path.join(cfg, "plugins", "broken", "plugin.hsh"),
              'this-command-does-not-exist-xyz\nBROKE=survived\n')
        out = run_interactive(home, 'echo "B=$BROKE"')
        check("a failing plugin does not abort startup",
              marker(out, "B=") == "survived", "got %r" % marker(out, "B="))
    finally:
        shutil.rmtree(home, ignore_errors=True)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
