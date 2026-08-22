#!/usr/bin/env python3
"""Linux-only kernel interfaces must live behind one door, not be sprinkled.

The report, from the macOS arm64 rung, once the build finally succeeded:

    39 ok / 1 failed
    FAIL process substitution
          want ps
          got  ''

`<(cmd)` and `>(cmd)` re-exec the shell to run the inner command, and they
did it by exec'ing the literal string "/proc/self/exe" -- twice, under two
names (PATH_HELLISH and PROC_SELF_EXE) that expanded to the same string, so
the second attempt was dead code. procfs is not in POSIX and Darwin has no
/proc at all, so on macOS both execs hit a path that is not there, the
child exited 127, and process substitution silently produced nothing.

The same assumption was in three other places: the ENOEXEC script-
interpreter fallback, and both halves of the update machinery's "where am
I?" (which made every macOS install classify as ORIGIN_BINARY, so `update`
would offer to overwrite a source checkout as though it were a downloaded
binary). All four now go through self_exe_path(), which uses
/proc/self/exe on Linux and _NSGetExecutablePath on Darwin.

So the invariant this file gates is not "does procsub work" -- the golden
suite and the smoke both cover that, on Linux, where it always worked. It
is that a Linux-only interface appears in exactly ONE place, the platform
file whose job is to know about it. That is the property that failed here:
the knowledge was copied into four files, and porting meant finding all
four. The next one has to be found too, and nobody will remember.

Usage: python3 linux_only_apis_test.py [ignored]
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FAILS = []

# interface -> the single file allowed to name it, and what to use instead.
# Add a row when a new Linux-only path or syscall enters the tree; the
# point of the table is that adding the row is a DECISION someone makes,
# not something that happens by copy-paste.
GATED = {
    "/proc/self/exe": ("src/platform/posix/self_exe.c", "self_exe_path()"),
}

# System headers that cannot be included ANYWHERE in this tree, and why.
# These are not style rules -- each one is a compile error we have already
# paid for, on a platform most of us cannot build locally.
FORBIDDEN_INCLUDES = {
    "mach-o/dyld.h":
        "it declares `enum DYLD_BOOL { FALSE, TRUE };` and libft's\n"
        "        ft_stddef.h already has `enum e_bool { FALSE, TRUE }`.\n"
        "        Two enums cannot define the same enumerator names in one\n"
        "        translation unit, so including both is a hard error in\n"
        "        either order. Declare the one symbol you need instead:\n"
        "        `int _NSGetExecutablePath(char *buf, uint32_t *bufsize);`",
}

# Comments may discuss these freely -- that is where the reasoning lives,
# and a rule that forbids explaining itself is a rule people route around.
# Block comments are tracked properly rather than matched line by line: the
# interesting mentions are on CONTINUATION lines of a /* */ paragraph, which
# start with plain spaces and match no "is this a comment" pattern at all.
# A first cut here used ^\s*(/\*|\*|//) and flagged five prose lines.
LINE_COMMENT = re.compile(r"^\s*//")


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def sources():
    out = subprocess.run(["git", "-C", ROOT, "ls-files", "src", "incs"],
                         capture_output=True)
    return [f for f in out.stdout.decode().split("\n")
            if f.endswith((".c", ".h"))]


def code_hits(path, needle):
    """Lines of `path` that mention `needle` in CODE, not in a comment.

    Not a C parser -- it only has to be right about whether a line is
    inside /* */, and it treats a line that opens and closes a block on
    itself as code plus a comment, which is the common `x = 1; /* why */`.
    A string containing "/*" would fool it; there are none in this tree,
    and the failure mode is a false negative on one line, not a wrong
    verdict on the file.
    """
    hits = []
    inblock = False
    full = os.path.join(ROOT, path)
    try:
        f = open(full, errors="replace")
    except OSError:
        return hits
    with f:
        for i, line in enumerate(f, 1):
            was = inblock
            if not inblock and "/*" in line and "*/" not in line:
                inblock = True
            elif inblock and "*/" in line:
                inblock = False
                was = True
            if was or LINE_COMMENT.match(line):
                continue
            code = line.split("/*")[0]
            if needle in code:
                hits.append("%s:%d" % (path, i))
    return hits


def test_forbidden_includes(files):
    """Headers that do not collide on Linux and do on the target.

    This one cost a CI round trip to find, which is the argument for the
    check: the collision is invisible here -- no Linux toolchain ships
    mach-o/dyld.h, so nothing locally can even attempt it -- and the only
    machine that can see it is the one we get to consult every twenty
    minutes. A grep is a poor substitute for a compiler and a very good
    substitute for a queue.
    """
    for header, why in FORBIDDEN_INCLUDES.items():
        stray = []
        for f in files:
            stray += code_hits(f, "<%s>" % header)
        check("%s is never included" % header, not stray,
              "\n        " + why + "\n        offending: "
              + ", ".join(stray[:6]))


def main():
    files = sources()
    check("there are sources to scan", len(files) > 100,
          "found %d" % len(files))
    for needle, (owner, instead) in GATED.items():
        stray = []
        for f in files:
            if f == owner:
                continue
            stray += code_hits(f, needle)
        check("%s is named only by %s" % (needle, owner), not stray,
              "\n        use %s instead of hardcoding it:\n        " % instead
              + "\n        ".join(stray[:10]))
        # ... and the owner must actually still use it, or the rule above
        # is passing because the interface quietly disappeared.
        check("%s still lives in %s" % (needle, owner),
              bool(code_hits(owner, needle)),
              "the owner no longer mentions it -- is the GATED table stale?")
    test_forbidden_includes(files)
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
