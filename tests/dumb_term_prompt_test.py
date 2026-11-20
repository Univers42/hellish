#!/usr/bin/env python3
"""Regression test: TERM=dumb with long prompt must not crash readline -- #184.

On TERM=dumb, readline forces horizontal scroll mode (_rl_horizontal_scroll_mode = 1)
because terminal height is <= 1. Under Readline 8.1, when a prompt contains zero-width
invisible markers (\\001 and \\002 from \\[ and \\]) and the visible path exceeds the
terminal width (80 columns), horizontal scroll mode sets lmargin = nleft at the end
of invisible_line, reading off the end of the heap-allocated line buffer and crashing
with an AddressSanitizer heap-buffer-overflow.

Stripping \\001 and \\002 from the line handed to readline when TERM=dumb avoids the
Readline 8.1 buffer calculation bug while preserving all prompt characters.

Usage: python3 dumb_term_prompt_test.py [/path/to/polish]
"""
import os
import pty
import select
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "polish"))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def run_interactive(shell_path, cwd, ps1=None, use_seed=False):
    master, slave = pty.openpty()
    env = dict(os.environ, TERM="dumb", HELLISH_NO_BANNER="1",
               HELLISH_NO_UPDATE_CHECK="1", HELLISH_NO_ANIM="1")
    for v in ("PS1", "PROMPT", "XDG_CONFIG_HOME"):
        env.pop(v, None)

    tmp_home = None
    if use_seed:
        tmp_home = tempfile.mkdtemp()
        env["HOME"] = tmp_home
        seeder = os.path.join(ROOT, "tools", "seed_hellishrc.sh")
        subprocess.run(["sh", seeder], env=dict(os.environ, HOME=tmp_home),
                       capture_output=True, check=True)

    args = [shell_path, "-i"]
    if not use_seed:
        args.append("--norc")

    p = subprocess.Popen(
        args,
        stdin=slave, stdout=slave, stderr=slave,
        env=env, cwd=cwd, close_fds=True
    )
    os.close(slave)

    time.sleep(0.1)
    if ps1 is not None:
        os.write(master, f'PS1="{ps1}"\n'.encode())
        time.sleep(0.1)

    os.write(master, b"echo __ALIVE__\nexit\n")

    out = b""
    end = time.time() + 5
    while time.time() < end:
        r, _, _ = select.select([master], [], [], 0.3)
        if not r:
            continue
        try:
            chunk = os.read(master, 4096)
            if not chunk:
                break
            out += chunk
        except OSError:
            break

    try:
        os.close(master)
    except OSError:
        pass

    p.wait(timeout=5)
    if tmp_home:
        shutil.rmtree(tmp_home, ignore_errors=True)

    return p.returncode, out.decode("utf-8", "replace")


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    tmp_base = tempfile.mkdtemp()
    long_dir = os.path.join(tmp_base, "x" * 110)
    os.makedirs(long_dir, exist_ok=True)

    try:
        # Case 1: Custom colored prompt with \[ and \] in a deep directory (>100 cols)
        rc, out = run_interactive(SHELL, long_dir, ps1="\\[\\033[32m\\]\\w\\[\\033[0m\\] $ ")
        check("custom prompt with invis markers under TERM=dumb exits 0", rc == 0,
              f"exit code {rc}, output: {out[:200]!r}")
        check("custom prompt executes command under TERM=dumb", "__ALIVE__" in out,
              f"missing alive token, output: {out[:200]!r}")
        check("no AddressSanitizer buffer overflow in output", "heap-buffer-overflow" not in out,
              f"ASan triggered: {out[-300:]!r}")

        # Case 2: Seeded default prompt with \w in a deep directory (canonical #184 trigger)
        rc, out = run_interactive(SHELL, long_dir, use_seed=True)
        check("seeded default prompt with deep cwd exits 0", rc == 0,
              f"exit code {rc}, output: {out[:200]!r}")
        check("seeded default prompt executes command under TERM=dumb", "__ALIVE__" in out,
              f"missing alive token, output: {out[:200]!r}")
        check("no ASan buffer overflow with seeded prompt", "heap-buffer-overflow" not in out,
              f"ASan triggered: {out[-300:]!r}")

        # Case 3: Short directory stays functional and clean
        short_dir = tempfile.mkdtemp(dir="/tmp")
        try:
            rc, out = run_interactive(SHELL, short_dir, ps1="\\[\\033[34m\\]\\w\\[\\033[0m\\] > ")
            check("short cwd under TERM=dumb exits 0", rc == 0, f"exit code {rc}")
            check("short cwd executes command", "__ALIVE__" in out)
        finally:
            shutil.rmtree(short_dir, ignore_errors=True)

    finally:
        shutil.rmtree(tmp_base, ignore_errors=True)

    print(f"\n{len(FAILS)} checks failed")
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
