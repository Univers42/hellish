#!/usr/bin/env python3
"""TAB completion through real bash-completion 2.16 (issue #105, wave 2).

Loading bash_completion cleanly is necessary but not sufficient: the
first fix wave made it SOURCE silently, and then the first TAB on a
Debian 13 box ran its completion functions for real and hit three more
gaps -- `[[ ... &&\\n ... ]]` split at the newline ("[[: missing ]]"
once per compat drop-in), `${!ref-}` rejected as a bad substitution in
_comp_get_words, and knock-on `_comp_upvars: invalid option` noise.
Sourcing tests can never see these; only pressing TAB does.

This drives an interactive pty whose ~/.bashrc sources the real
bash-completion 2.16 (cached download, cleanly skipped offline) with a
POPULATED compat dir -- the empty-dir shortcut is exactly what hid the
compat-loop bug -- then presses TAB and asserts the transcript carries
none of the failure signatures and the shell still works afterwards.

Usage: python3 tab_completion_chain_test.py [/path/to/hellish]
"""
import os
import pty
import select
import shutil
import sys
import tempfile
import time
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
CACHE = os.environ.get(
    "PLUGIN_CACHE",
    os.path.join(os.environ.get("XDG_CACHE_HOME",
                                os.path.expanduser("~/.cache")),
                 "hellish-plugin-corpus"))
URL = ("https://raw.githubusercontent.com/scop/bash-completion/2.16.0/"
       "bash_completion")
FAILS = []

BAD_SIGNS = ["missing `]]'", "bad substitution", "invalid option",
             "syntax error", "command not found", "AddressSanitizer"]

DROPIN = """# a compat drop-in, like every distro package installs
_hx_dropin_probe()
{
    return 0
}
complete -F _hx_dropin_probe hxprobe
"""


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def fetch():
    path = os.path.join(CACHE, "bash-completion-2.16.sh")
    if os.path.isfile(path) and os.path.getsize(path) > 0:
        return path
    try:
        os.makedirs(CACHE, exist_ok=True)
        with urllib.request.urlopen(URL, timeout=20) as r:
            data = r.read()
        with open(path, "wb") as f:
            f.write(data)
        return path
    except OSError:
        return None


def main():
    bc = fetch()
    if not bc:
        print("SKIP: no network and no cached bash-completion 2.16")
        return
    home = tempfile.mkdtemp(prefix="hx-tab-")
    compat = os.path.join(home, "compat.d")
    os.mkdir(compat)
    for n in ("one.sh", "two.sh", "three.sh", "four.sh"):
        open(os.path.join(compat, n), "w").write(DROPIN)
    os.mkdir(os.path.join(home, "files"))
    open(os.path.join(home, "files", "lifecycle.txt"), "w").write("x\n")
    with open(os.path.join(home, ".bashrc"), "w") as f:
        f.write("PS1='TAB$ '\n. %s\n" % bc)
    with open(os.path.join(home, ".hellishrc"), "w") as f:
        f.write(". $HOME/.bashrc\n")

    pid, fd = pty.fork()
    if pid == 0:
        env = {"HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
               "TERM": "xterm", "LANG": "C.UTF-8",
               "BASH_COMPLETION_COMPAT_DIR": compat,
               "BASH_COMPLETION_USER_FILE": "/nonexistent-hx",
               "HELLISH_NO_BANNER": "1", "HELLISH_BANNER": "0",
               "HELLISH_NO_ANIM": "1", "HELLISH_NO_UPDATE_CHECK": "1",
               "ASAN_OPTIONS": "detect_leaks=0"}
        os.execve(SHELL, [SHELL], env)
        os._exit(127)
    time.sleep(2.0)
    os.write(fd, b"echo START-OK\r")
    time.sleep(0.6)
    os.write(fd, b"cd $HOME/files\r")
    time.sleep(0.4)
    os.write(fd, b"cat li\t")
    time.sleep(1.5)
    os.write(fd, b"\r")
    time.sleep(0.6)
    os.write(fd, b"ls /tm\t")
    time.sleep(1.5)
    os.write(fd, b"\r")
    time.sleep(0.6)
    os.write(fd, b"hxpro\t")
    time.sleep(1.0)
    os.write(fd, b"\x15echo AFTER-OK\r")
    time.sleep(0.6)
    os.write(fd, b"exit\r")
    out = b""
    end = time.time() + 8
    status = None
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.3)
        if r:
            try:
                d = os.read(fd, 65536)
            except OSError:
                break
            if not d:
                break
            out += d
        else:
            try:
                w, st = os.waitpid(pid, os.WNOHANG)
            except ChildProcessError:
                break
            if w:
                status = st
                break
    try:
        os.close(fd)
    except OSError:
        pass
    if status is None:
        try:
            _, status = os.waitpid(pid, 0)
        except ChildProcessError:
            status = 0
    text = out.decode("utf-8", "replace")
    shutil.rmtree(home, ignore_errors=True)

    check("session started with rc chain", "START-OK" in text,
          repr(text[:400]))
    for sign in BAD_SIGNS:
        check("no %r after TAB" % sign, sign not in text,
              repr(text[-600:]))
    check("filename TAB completed", "lifecycle.txt" in text,
          repr(text[-600:]))
    check("shell alive after completions", "AFTER-OK" in text,
          repr(text[-400:]))
    check("clean exit", status is not None and os.WIFEXITED(status),
          "status=%r" % status)
    if FAILS:
        print("\n%d FAILED: %s" % (len(FAILS), ", ".join(FAILS)))
        sys.exit(1)
    print("\nall clear")


main()
