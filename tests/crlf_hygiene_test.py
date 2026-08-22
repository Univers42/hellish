#!/usr/bin/env python3
"""Anything with a shebang must be stored with LF, or Windows breaks it.

The report, from the WSL rung:

    docker/smoke.sh: line 19: set: -
    : invalid option
    docker/smoke.sh: line 20: $'\\r': command not found
    docker/smoke.sh: line 28: syntax error near unexpected token `$'{\\r''

Nothing was wrong with smoke.sh. GitHub's Windows runners check out with
`core.autocrlf=true`, which rewrites LF to CRLF on the way to the working
tree, and bash then reads the trailing CR as part of every token: `set -u`
becomes `set -u\\r`, `expect() {` becomes `expect() {\\r`. Harmless for C,
which the compiler tokenizes on whitespace; fatal for anything executed.

The fix is `.gitattributes` pinning the executed file types to `eol=lf`, so
the rule travels with the repo and protects anyone cloning on Windows --
not just CI. This file is the gate on that rule, and it asserts the two
halves separately because they fail for different reasons:

  1. no tracked script is stored with CRLF in the INDEX. A file that is
     already CRLF in git will be CRLF everywhere, `.gitattributes` or not.
  2. every one of them is COVERED by an `eol=lf` attribute. This is the
     half that matters: today's files are all LF, so check 1 passes on a
     repo with no .gitattributes at all -- it would have passed on the
     commit that broke WSL. Coverage is what stops the next file added.

Deliberately not asserted: that the whole tree is LF. Around a hundred
files here are stored with CRLF already (older headers, and fixtures under
tests/ whose bytes the golden suite compares exactly). Renormalizing those
is a separate decision with real risk, and pretending otherwise would make
this test something nobody can keep green.

Usage: python3 crlf_hygiene_test.py [ignored]
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FAILS = []

# Extensions and names whose content is EXECUTED by an interpreter that
# tokenizes lines. A stray CR is a syntax error in every one of them.
SUFFIXES = (".sh", ".py", ".bash", ".zsh", ".mk")
NAMES = ("Makefile", "tester", "configure")
# Dockerfiles break one layer down: the CR rides along on every RUN line and
# lands inside the command the shell in the image runs.
PREFIXES = ("Dockerfile",)


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def git(*args):
    p = subprocess.run(["git", "-C", ROOT] + list(args), capture_output=True)
    return p.stdout.decode(errors="replace")


def executed_files():
    """Tracked files an interpreter reads line by line.

    Selected by name rather than by reading each file, so a file that is
    MISSING its shebang today is still covered -- the point is the class,
    not the current contents.
    """
    out = []
    for f in git("ls-files", "-z").split("\0"):
        if not f:
            continue
        base = os.path.basename(f)
        if f.endswith(".dockerignore"):
            continue
        if (f.endswith(SUFFIXES) or base in NAMES
                or base.startswith(PREFIXES)):
            out.append(f)
    return sorted(out)


def stored_with_crlf(paths):
    """Which of `paths` contain a CR in the INDEX, not the working tree.

    The index is the thing that travels. A working tree can be CRLF on
    Windows and correct in git, which is exactly what .gitattributes
    arranges, so reading files off disk would ask the wrong question.
    """
    bad = []
    for f in paths:
        p = subprocess.run(["git", "-C", ROOT, "show", ":" + f],
                           capture_output=True)
        if p.returncode == 0 and b"\r\n" in p.stdout:
            bad.append(f)
    return bad


def uncovered(paths):
    """Which of `paths` no `eol=lf` attribute applies to."""
    if not paths:
        return []
    out = git("check-attr", "eol", "--", *paths).split("\n")
    bad = []
    for line in out:
        if not line.strip():
            continue
        # "<path>: eol: <value>" -- the path may itself contain ": "
        head, _, value = line.rpartition(": ")
        if value.strip() != "lf":
            bad.append(head.rsplit(":", 1)[0])
    return bad


def main():
    files = executed_files()
    check("there are executed files to check", len(files) > 10,
          "found only %d" % len(files))
    bad = stored_with_crlf(files)
    check("no executed file is stored with CRLF", not bad,
          "\n        " + "\n        ".join(bad[:10]))
    miss = uncovered(files)
    check("every executed file is covered by an eol=lf attribute",
          not miss,
          "\n        not covered by .gitattributes:\n        "
          + "\n        ".join(miss[:10])
          + ("\n        (+%d more)" % (len(miss) - 10) if len(miss) > 10
             else ""))
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
