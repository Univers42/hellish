#!/usr/bin/env python3
"""Regression test: the three build configurations stay distinct.

Raised after a 5.6 MB `build/bin/hellish` was mistaken for the shipped
binary. It was not: that is the DEBUG build, and it is large on purpose --
full DWARF plus the AddressSanitizer runtime. The release artifact is about
620 KB and carries no debug information of ours at all, because release
never compiles -g in. (`strip` on it recovers ~1 KB: DWARF stubs the C
runtime brings, not our code.)

The lesson worth keeping is not "make it smaller", it is that the two jobs
must stay separated at the BUILD level rather than by stripping afterwards.
So this pins the property directly:

  debug            -O0, -g, sanitizer     -- develop against this
  release          optimized, NO -g, no sanitizer, LTO -- ship this
  relwithdebinfo   optimized AND -g, no sanitizer, no LTO -- for the bugs
                   that only appear with optimization on

and that OPT=1, which CI, both install targets, the docker build and
`make bench` all pass, still means release.

Flags are read from `make flags`, which resolves them without compiling, so
this costs nothing and cannot be fooled by a stale object tree.

Usage: python3 build_modes_test.py [/path/to/hellish]   (shell unused)
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def flags(*args):
    p = subprocess.run(["make", "flags", "--no-print-directory"] + list(args),
                       cwd=ROOT, capture_output=True, text=True, timeout=120)
    out = re.sub(r"\x1b\[[0-9;]*[A-Za-z]", "", p.stdout)
    got = {}
    for line in out.splitlines():
        if "=" in line and line.split("=", 1)[0] in (
                "MODE", "CFLAGS", "LDFLAGS", "LDLIBS", "OBJ_DIR"):
            k, v = line.split("=", 1)
            got[k] = v
    return got


def has_g(cf):
    """A real debug-info flag: -g, -g3, -ggdb -- but not -gdwarf-ish noise."""
    return any(re.fullmatch(r"-g\d?|-ggdb\d?", t) for t in cf.split())


def main():
    d = flags("MODE=debug")
    check("debug: mode resolves", d.get("MODE") == "debug", repr(d.get("MODE")))
    check("debug: carries debug info", has_g(d.get("CFLAGS", "")),
          d.get("CFLAGS", ""))
    check("debug: unoptimised", "-O0" in d.get("CFLAGS", ""))
    check("debug: sanitizer on", "-fsanitize" in d.get("CFLAGS", ""),
          "development instrumentation must not be quietly dropped")
    check("debug: sanitizer also linked", "-fsanitize" in d.get("LDFLAGS", ""))

    r = flags("MODE=release")
    check("release: mode resolves", r.get("MODE") == "release")
    check("release: NO debug info", not has_g(r.get("CFLAGS", "")),
          "release would ship DWARF: %s" % r.get("CFLAGS", ""))
    check("release: optimised", "-O3" in r.get("CFLAGS", ""))
    check("release: assertions compiled out", "-DNDEBUG" in r.get("CFLAGS", ""))
    check("release: no sanitizer", "-fsanitize" not in r.get("CFLAGS", "")
          and "-fsanitize" not in r.get("LDFLAGS", ""),
          "the sanitizer runtime is most of the debug build's size")
    check("release: link-time optimisation on", "-flto" in r.get("CFLAGS", ""))

    w = flags("MODE=relwithdebinfo")
    check("relwithdebinfo: mode resolves", w.get("MODE") == "relwithdebinfo")
    check("relwithdebinfo: optimised", "-O2" in w.get("CFLAGS", ""))
    check("relwithdebinfo: AND carries debug info", has_g(w.get("CFLAGS", "")),
          "the whole point is a debuggable optimised build")
    check("relwithdebinfo: no sanitizer",
          "-fsanitize" not in w.get("CFLAGS", ""))
    check("relwithdebinfo: no LTO", "-flto" not in w.get("CFLAGS", ""),
          "LTO turns a stack trace into a list of inlined addresses")

    # The three must actually differ -- a copy-paste that made two modes
    # identical would pass every check above and defeat the purpose.
    check("the three modes are genuinely distinct",
          len({d.get("CFLAGS"), r.get("CFLAGS"), w.get("CFLAGS")}) == 3,
          "two modes resolve to the same flags")

    # OPT=1 is load-bearing: CI, my_shell, user-install, docker and bench.
    o = flags("OPT=1")
    check("OPT=1 still means release", o.get("MODE") == "release",
          "renaming the flag would break CI and both install targets")
    check("OPT=1 matches MODE=release exactly",
          o.get("CFLAGS") == r.get("CFLAGS"))

    # And an unknown mode must fail loudly rather than silently building
    # something nobody asked for.
    p = subprocess.run(["make", "flags", "MODE=nope", "--no-print-directory"],
                       cwd=ROOT, capture_output=True, text=True, timeout=120)
    # Separate flags are worth nothing if the modes share an object tree.
    # make rebuilds on a changed PREREQUISITE, never on a changed flag, so a
    # tree filled by MODE=debug and then reused by MODE=release hands the
    # linker ASan-instrumented objects under a link line with no -fsanitize:
    #
    #   func_retire.o: undefined reference to `__asan_report_load4'
    #
    # That is exactly what happened while this was being written, because
    # OBJ_DIR still keyed on `ifdef OPT` -- which covered the OPT benchmark
    # build and nothing else, leaving release and relwithdebinfo both parked
    # in the debug tree. `make re` hides it; a plain `make MODE=release`
    # after a debug build does not.
    trees = [d.get("OBJ_DIR"), r.get("OBJ_DIR"), w.get("OBJ_DIR")]
    check("each mode gets its own object tree",
          all(trees) and len(set(trees)) == 3, repr(trees))
    check("OPT=1 shares release's object tree",
          o.get("OBJ_DIR") == r.get("OBJ_DIR"),
          "%r != %r" % (o.get("OBJ_DIR"), r.get("OBJ_DIR")))

    # SAFE rides along in the key for the same reason: it decides
    # -DHAVE_ALLOC_ORACLE, which is a compile-time define, so the two
    # allocator backends cannot share objects either.
    rs = flags("MODE=release", "SAFE=1")
    check("the allocator backend also splits the object tree",
          rs.get("OBJ_DIR") not in (None, r.get("OBJ_DIR")),
          "%r == %r" % (rs.get("OBJ_DIR"), r.get("OBJ_DIR")))

    check("an unknown MODE is refused", p.returncode != 0
          and "MODE must be" in (p.stdout + p.stderr))

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
