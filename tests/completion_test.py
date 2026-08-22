#!/usr/bin/env python3
"""Regression test: tab completion, on the ft_malloc heap on purpose.

issue #40. readline OWNS every string a completion generator returns and
releases it with libc free(). Our generators built theirs with ft_strdup /
ft_substr / ft_strjoin, which compile to ft_malloc on a SAFE=0 build -- so
the first TAB handed libc a pointer libc never allocated:

    ✘130 ❯ free(): invalid size

(or "double free or corruption (out)", depending on what PATH happened to
hold) and the line editor never recovered. This is invisible on SAFE=1,
where xmalloc IS libc malloc and there is no mismatch to find, so this
test is only meaningful against a SAFE=0 binary -- `make completion-test`
builds one for exactly that reason.

Also covers the truncation found alongside it: cmd_gen_dirs closed each
PATH directory after its FIRST match, and readline asks for one match per
call, so every other command in that directory was silently dropped. An
empty TAB listed roughly one command per PATH entry instead of all of them.

Usage: python3 completion_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import select
import shutil
import struct
import sys
import tempfile
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
FAILS = []

CRASH_MARKERS = ("free(): invalid size", "double free", "corrupted",
                 "munmap_chunk", "AddressSanitizer", "not malloc()-ed",
                 "SIGSEGV", "Aborted")


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def run(sends, path_extra=None, settle=1.4):
    env = {
        "HOME": os.environ.get("HOME", "/tmp"),
        "PATH": os.environ["PATH"],
        "TERM": "xterm-256color", "LANG": "C.UTF-8", "PS1": "$ ",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1",
        # leaks are checked by verify_alloc; here we want heap ERRORS loud
        "ASAN_OPTIONS": "detect_leaks=0",
    }
    if path_extra:
        env["PATH"] = path_extra + ":" + env["PATH"]
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        os.execvp(SHELL, [SHELL])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 40, 100, 0, 0))
    time.sleep(0.8)
    out = b""
    for data in sends:
        try:
            os.write(fd, data)
        except OSError:
            break
        end = time.time() + settle
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.1)
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
    return out.decode(errors="replace")


def no_crash(out):
    return [m for m in CRASH_MARKERS if m in out]


def main():
    # 1: the exact report -- TAB on an empty line. 'n' declines readline's
    # "display all N possibilities?", which only appears now that the scan
    # actually reaches every command in PATH.
    out = run([b"\t", b"n", b"\n", b"echo STILL_ALIVE\n"])
    hits = no_crash(out)
    check("TAB on an empty line does not corrupt the heap", not hits,
          "saw %s" % hits)
    check("the shell still runs commands after that TAB",
          out.count("STILL_ALIVE") >= 2, "tail=%r" % out[-200:])

    # 2: completing a builtin still works (the matches are libc-owned now,
    # so this is the path that would abort if rl_dup were reverted).
    out = run([b"expor\t", b"\n"])
    check("TAB completes a builtin name", "export" in out,
          "tail=%r" % out[-200:])
    check("completing a builtin does not corrupt the heap",
          not no_crash(out), "saw %s" % no_crash(out))

    # 3: variable completion -- ft_strjoin's intermediate was the other
    # cross-heap allocation.
    out = run([b"echo $HOM\t", b"\n"])
    check("TAB completes $HOME", "$HOME" in out or os.environ.get("HOME", "")
          in out, "tail=%r" % out[-200:])
    check("variable completion does not corrupt the heap",
          not no_crash(out), "saw %s" % no_crash(out))

    # 4: every match in a PATH directory is offered, not just the first.
    d = tempfile.mkdtemp(prefix="hellish_comp_")
    try:
        for name in ("zzprobe_alpha", "zzprobe_beta", "zzprobe_gamma"):
            p = os.path.join(d, name)
            with open(p, "w") as f:
                f.write("#!/bin/sh\n")
            os.chmod(p, 0o755)
        out = run([b"zzprobe_\t\t", b"\n"], path_extra=d, settle=1.8)
        found = [n for n in ("alpha", "beta", "gamma")
                 if "zzprobe_" + n in out]
        check("all matches in one PATH dir are offered, not just the first",
              len(found) == 3,
              "only %r survived the scan; tail=%r" % (found, out[-300:]))
    finally:
        shutil.rmtree(d, ignore_errors=True)

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
