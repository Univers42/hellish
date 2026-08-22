#!/usr/bin/env python3
"""Every symbol we call must be DEFINED somewhere -- proved on Linux.

The report, from the macOS arm64 rung:

    Undefined symbols for architecture arm64:
      "_get_original_tty_job_signals", referenced from:
          _initialize_traps in initialize_traps.o
    ld: symbol(s) not found for architecture arm64

`get_original_tty_job_signals` was declared in trap.h and called by
`initialize_traps()`, and never defined -- by anyone, on any platform. It
had been that way for months and every Linux job stayed green, because
the two are not the same question:

  * an EXECUTABLE must resolve everything, so a symbol on a path the
    linker pulls in is caught. Archives only contribute the members that
    are actually needed, and nothing needed initialize_traps.o.
  * a SHARED LIBRARY on GNU ld may keep undefined symbols and hope the
    loader finds them later. libft.so linked happily with a hole in it.
  * on Darwin, a dylib may NOT. ld resolves everything or fails.

So the difference is not "macOS is stricter about C". It is that GNU ld
defaults to `--allow-shlib-undefined` and Apple's does not, and we had
been shipping a library whose contract was a lie -- a call that would
have been a runtime crash the first time job control asked for it.

Passing `-Wl,--no-undefined` to GNU ld asks the SAME question Apple's
linker asks by default, so the failure reproduces here in about a second:

    cc -shared -o /dev/null -Wl,--no-undefined \
       -Wl,--whole-archive libft.a -Wl,--no-whole-archive -lpthread -lm

`--whole-archive` matters as much as `--no-undefined`: without it the
linker skips the members nothing references, which is exactly how the
hole stayed hidden. Whole-archive is also what a dylib build does.

This is a portability test that needs no Mac. It fails on the commit that
introduced the hole and passes on the one that filled it.

Only the archives are checked, not build/obj: the shell is an EXECUTABLE,
so its own objects already have to resolve every symbol at every build or
there is no binary. The archives are the blind spot, and the reason is
worth keeping in mind when adding to this file -- a check that repeats
what `make` already proves is a check nobody learns anything from.

Usage: python3 link_closure_test.py /path/to/hellish
"""
import os
import platform
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FAILS = []

# What the shell itself is linked against (Makefile LDLIBS). A symbol that
# only these provide is fine; anything else has to come from our own code.
SYSLIBS = ["-lreadline", "-lpthread", "-lm", "-ldl"]


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def cc():
    return os.environ.get("CC", "cc")


def strict_link(objects, extra_libs):
    """Link `objects` the way a Darwin dylib link would, and report holes.

    Returns (ok, stderr). /dev/null as the output is deliberate: nothing
    here wants the artifact, only the linker's verdict on it.
    """
    cmd = [cc(), "-shared", "-o", "/dev/null", "-Wl,--no-undefined",
           "-Wl,--whole-archive"] + objects + ["-Wl,--no-whole-archive"]
    cmd += extra_libs
    p = subprocess.run(cmd, capture_output=True)
    return (p.returncode == 0, p.stderr.decode(errors="replace"))


def missing_symbols(stderr):
    """Pull the symbol names out of a GNU ld undefined-reference report."""
    return sorted(set(re.findall(r"undefined reference to [`'\"]([^`'\"]+)",
                                 stderr)))


def report(label, ok, err):
    syms = missing_symbols(err)
    detail = ""
    if syms:
        detail = "\n        undefined: " + ", ".join(syms[:12])
        if len(syms) > 12:
            detail += " (+%d more)" % (len(syms) - 12)
    elif not ok:
        detail = "\n        " + err.strip()[-600:]
    check(label, ok, detail)


def archives():
    """Every static library the build produces, whichever heap was built.

    build-libc is SAFE=1 (system malloc), build-ft is SAFE=0 (ft_malloc).
    Both are checked when both are present, because they are different
    compilations of the same tree and only one of them is usually built.
    """
    out = []
    for tag in ("libc", "ft"):
        d = os.path.join(ROOT, "vendor", "libft", "build-%s" % tag, "lib")
        if not os.path.isdir(d):
            continue
        for f in sorted(os.listdir(d)):
            if f.endswith(".a"):
                out.append(("%s/%s" % (tag, f), os.path.join(d, f)))
    return out


def test_archives():
    found = archives()
    check("a built archive exists to check", bool(found),
          "no vendor/libft/build-*/lib/*.a -- run make")
    for label, a in found:
        ok, err = strict_link([a], SYSLIBS)
        report("%s has no undefined symbols" % label, ok, err)


def main():
    if platform.system() != "Linux":
        # --no-undefined is a GNU ld spelling, and on Darwin the real
        # linker already enforces this -- there is nothing to emulate.
        print("skipped: this test emulates Darwin's linker using GNU ld")
        sys.exit(0)
    ok, _ = strict_link([], ["-lm"])
    if not ok:
        print("skipped: this toolchain does not accept -Wl,--no-undefined")
        sys.exit(0)
    test_archives()
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
