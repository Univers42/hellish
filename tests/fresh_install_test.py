#!/usr/bin/env python3
"""Regression test: the FRESH INSTALL boots clean -- issue #88.

A brand-new machine, `curl install.sh | sh`, the plugin framework enabled
with bash-preexec and z -- and every prompt printed

    hellish: _hx_precmd_run__bp_install: command not found
    [1] 77646

plus two whole function bodies dumped over the banner. Three separate
shell bugs stacked into one first impression:

  1. the case/trim matcher had no POSIX character classes, so
     bash-preexec's `${text#"${text%%[![:space:]]*}"}` trimmed every
     PROMPT_COMMAND to "", and its install string was appended to the
     UNtrimmed variable with no separator -- the mashed name above;
  2. `declare -ft fn` fell into the print path instead of silently
     applying the trace attribute, so bash-preexec's closing line dumped
     __bp_install and __bp_hook_preexec_into_debug onto the screen;
  3. `( cmd & )` announced "[1] pid" from inside a subshell -- the exact
     spelling z.sh uses to keep its bookkeeping SILENT; bash keeps job
     control off in subshells.

This test rebuilds that machine: a framework-shaped rc (the same
PROMPT_COMMAND='_hx_precmd_run' convention hellishrc_plugins ships)
sourcing the REAL upstream bash-preexec.sh and z.sh, on a real pty. It
shares the plugin corpus's download cache and its philosophy: vendors
nothing, and the network-dependent half SKIPS cleanly when offline. The
three underlying bugs also have offline coverage: the golden category
tests/pattern_class pins 1 and 2 against the bash oracle, and the
subshell-silence check below needs no network at all.

Usage: python3 fresh_install_test.py [/path/to/hellish]
"""
import os
import pty
import re
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
OFFLINE = os.environ.get("OFFLINE", "") not in ("", "0")
FAILS = []

PLUGINS = [
    ("bash-preexec.sh", "https://raw.githubusercontent.com/rcaloras/"
     "bash-preexec/master/bash-preexec.sh"),
    ("z.sh", "https://raw.githubusercontent.com/rupa/z/master/z.sh"),
]

RC = """# the hellishrc_plugins loader shape, reduced to what issue #88 needs
HX_PRECMD_FUNCS=""
hx_precmd_add() { HX_PRECMD_FUNCS="$HX_PRECMD_FUNCS $1"; }
_hx_precmd_run() {
    local f
    for f in $HX_PRECMD_FUNCS; do
        command -v "$f" >/dev/null 2>&1 && "$f"
    done
    return 0
}
PROMPT_COMMAND='_hx_precmd_run'
PS1='FRESH$ '
. "$HOME/plugins/bash-preexec.sh"
. "$HOME/plugins/z.sh"
"""


def check(name, ok, detail=""):
    print("  %s %s" % ("\033[32mok\033[0m  " if ok else "\033[31mFAIL\033[0m",
                       name), flush=True)
    if not ok:
        if detail:
            print("       %s" % detail.replace("\n", "\n       "))
        FAILS.append(name)


def fetch(name, url):
    os.makedirs(CACHE, exist_ok=True)
    path = os.path.join(CACHE, name)
    if os.path.isfile(path) and os.path.getsize(path) > 0:
        return path
    if OFFLINE:
        return None
    try:
        with urllib.request.urlopen(url, timeout=20) as r:
            data = r.read()
        with open(path, "wb") as f:
            f.write(data)
        return path
    except OSError:
        return None


def session(home, cmds, settle=1.2):
    env = {"HOME": home, "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
           "TERM": "dumb", "LANG": "C.UTF-8", "HELLISH_NO_BANNER": "1",
           "HELLISH_NO_ANIM": "1", "HELLISH_NO_UPDATE_CHECK": "1",
           "ASAN_OPTIONS": "detect_leaks=0"}
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(home)
        os.environ.clear()
        os.environ.update(env)
        os.execv(SHELL, [SHELL, "-i"])
        os._exit(127)
    out = b""

    def drain(t):
        nonlocal out
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.15)
            if not r:
                continue
            try:
                d = os.read(fd, 65536)
            except OSError:
                return
            if not d:
                return
            out += d

    drain(3.0)
    for c in cmds:
        os.write(fd, c + b"\n")
        drain(settle)
    os.write(fd, b"exit\n")
    drain(0.5)
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

    # The no-network half: the subshell-silence bug on its own.
    print("\n\033[1;36m▸\033[0m \033[1msubshell background job is silent"
          "\033[0m")
    home = tempfile.mkdtemp(prefix="fresh88-")
    open(os.path.join(home, ".hellishrc"), "w").write("PS1='BARE$ '\n")
    out = session(home, [b"( sleep 0.1 & ); echo SUBDONE",
                         b"sleep 20 & echo TOPDONE", b"kill %1"])
    check("( cmd & ) announces no job", not re.search(
        r"^\[\d+\] \d+", out.split("TOPDONE")[0], re.M), out[-300:])
    check("a top-level & still announces its job",
          re.search(r"\[\d+\] \d+", out) is not None, out[-300:])
    shutil.rmtree(home, ignore_errors=True)

    # The full machine. Real upstream plugins, cached like the corpus.
    files = [fetch(n, u) for n, u in PLUGINS]
    if not all(files):
        print("\n  \033[33m~\033[0m skipped: bash-preexec/z not cached and "
              "no network -- the golden category tests/pattern_class still "
              "covers the underlying bugs")
        print("\n%d checks failed" % len(FAILS))
        sys.exit(1 if FAILS else 0)

    print("\n\033[1;36m▸\033[0m \033[1mthe #88 machine: framework rc + "
          "bash-preexec + z\033[0m")
    home = tempfile.mkdtemp(prefix="fresh88-")
    os.makedirs(os.path.join(home, "plugins"))
    for f in files:
        shutil.copy2(f, os.path.join(home, "plugins"))
    open(os.path.join(home, ".hellishrc"), "w").write(RC)
    out = session(home, [b"echo E2E-ALIVE",
                         b"printf '<%s>' \"$PROMPT_COMMAND\"",
                         b"cd /tmp", b"cd -"])

    check("no 'command not found' from the prompt hook",
          "command not found" not in out, out[-500:])
    check("no job number announced at any prompt",
          not re.search(r"^\[\d+\] \d+", out, re.M), out[-500:])
    check("no function body dumped on the screen",
          "lastexit" not in out and "__bp_adjust_histcontrol" not in out,
          out[:600])
    check("the shell still runs commands", "E2E-ALIVE" in out, out[-300:])
    check("bash-preexec really installed itself",
          "__bp_precmd_invoke_cmd" in out,
          "PROMPT_COMMAND never got the hook:\n" + out[-400:])
    check("the hooks are separated, not concatenated",
          "_hx_precmd_run__bp" not in out, out[-400:])
    shutil.rmtree(home, ignore_errors=True)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
