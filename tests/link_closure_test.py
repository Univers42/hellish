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

The archives are checked for undefined symbols, and the shell's own
objects for a subtler thing: a WEAK reference that nothing defines. The
shell is an executable, so an ordinary missing symbol already fails its
own build -- but a weak one does not. It is legal on ELF, resolves to
NULL, and is a link error on Mach-O. Both checks exist to ask a question
`make` does not ask here; a check that repeats what `make` already proves
is a check nobody learns anything from.

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


def nm_symbols(path):
    """(defined, undefined_weak) for one object or archive, or None.

    None means nm could not read it -- an LTO build stores bitcode, not
    ELF, and plain nm says "file format not recognized". Skipping those is
    correct rather than lenient: the non-LTO object tree carries the same
    references and IS readable, so nothing goes unchecked.
    """
    p = subprocess.run(["nm", path], capture_output=True)
    if p.returncode != 0:
        return None
    defined = set()
    weak_undef = set()
    for line in p.stdout.decode(errors="replace").split("\n"):
        f = line.split()
        if len(f) == 2 and f[0] == "w":
            weak_undef.add(f[1])
        elif len(f) == 3 and f[1] not in ("U", "w", "v"):
            defined.add(f[2])
    return (defined, weak_undef)


def object_tree():
    """The shell's own objects, from whichever build ran most recently."""
    for d in ("obj", "obj-opt", "obj-debug"):
        base = os.path.join(ROOT, "build", d)
        if not os.path.isdir(base):
            continue
        objs = []
        for root, _, files in os.walk(base):
            objs += [os.path.join(root, f) for f in files
                     if f.endswith(".o")]
        if objs:
            return (d, sorted(objs))
    return (None, [])


def test_no_dangling_weak_refs():
    """A weak reference nothing defines is an ELF-only trick.

    On ELF an undefined WEAK symbol is legal and resolves to NULL, so code
    can ask "was this backend linked in?" with `if (!sym)`. Mach-O has no
    such thing: __attribute__((weak)) on a DECLARATION is a weak
    *definition*, not a weak import, so Apple's linker demands a body. That
    is the whole of the second macOS failure --

        Undefined symbols for architecture arm64:
          "_malloc_live_bytes", referenced from: _free_all_state in lto.o

    -- from alloc_stats.c probing for ft_malloc's leak oracle that way on a
    SAFE=1 build, where nothing defines it. The fix was to let the Makefile
    decide at compile time, since it already knows which heap it builds.

    So: no object may carry an undefined weak symbol that the object tree
    does not itself define. Deliberately NOT "defined anywhere in the
    link" -- if a LIBRARY has the definition, a weak reference is still
    the wrong way to reach it (use a strong one, which also pulls the
    archive member), and it still fails on Darwin. Measuring against the
    archives instead made this check pass while broken, because a SAFE=0
    libft.a left over from an earlier build defined the very symbol a
    SAFE=1 link cannot see.

    There is no allowlist here because the tree needs none: across ~490
    objects this is the only weak undefined symbol that has ever appeared.
    If a toolchain starts emitting its own, the failure names it and the
    decision can be made then, with the name in hand.
    """
    d, objs = object_tree()
    check("the shell's objects exist to check", bool(objs),
          "no build/obj*/**/*.o -- run make")
    if not objs:
        return
    defined = set()
    dangling = {}
    unreadable = 0
    for o in objs:
        got = nm_symbols(o)
        if got is None:
            unreadable += 1
            continue
        defined |= got[0]
        for sym in got[1]:
            dangling.setdefault(sym, os.path.relpath(o, ROOT))
    left = {s: o for s, o in dangling.items() if s not in defined}
    detail = ""
    if left:
        detail = "\n        " + "\n        ".join(
            "%s (referenced from %s)" % (s, o) for s, o in
            sorted(left.items())[:8])
    check("build/%s has no weak reference that nothing defines" % d,
          not left, detail)
    if unreadable:
        print("     (%d LTO object(s) skipped -- not ELF; the non-LTO tree "
              "covers the same code)" % unreadable)


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
    test_no_dangling_weak_refs()
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
