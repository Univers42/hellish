#!/usr/bin/env python3
"""Regression test: array literals split and glob like bash.

    V="a b c"; arr=($V)     hellish ${#arr[@]} = 1     bash = 3
    g=(*.md)                hellish 1                  bash = 6

Element words were expanded with expand_word_assign_ro, i.e. with
EW_KEEP_AS_ONE | EW_NO_GLOB. The comment in expand_array_assign.c called this
"a documented v1 divergence from bash". It is not a divergence anyone can
live with: `arr=($VAR)` and `files=(*.c)` are the two most common array
idioms in real shell code, and they were silently producing one element.

Silently is the operative word. Nothing errored -- you got an array of length
1 whose single element was the unsplit string, so a loop over it ran once with
a wrong value. Every zsh/bash plugin in tests/plugin_corpus_test.py uses this
shape.

The scalar case must NOT change: POSIX says `VAR=value` does not glob, so
`x=*.md` stays literal. That is the same expand_word_assign_ro, and it keeps
its flags -- only the ELEMENT path moved.

Usage: python3 array_literal_split_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
ORACLE = os.environ.get("HELLISH_ORACLE",
                        os.path.expanduser("~/bash-5.3.9/bin/bash"))
ENV = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
           HELLISH_NO_ANIM="1", ASAN_OPTIONS="detect_leaks=0")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def run(sh, script, cwd=None):
    p = subprocess.run([sh, "-c", script], capture_output=True, text=True,
                       env=ENV, cwd=cwd, timeout=60)
    return p.stdout, p.returncode


def same_as_bash(name, script, cwd=None):
    """The contract is bash's behaviour, so compare rather than assert."""
    got, grc = run(SHELL, script, cwd)
    if not os.path.exists(ORACLE):
        return check(name + " (no oracle, skipped)", True)
    want, wrc = run(ORACLE, script, cwd)
    check(name, got == want and grc == wrc,
          "hellish=%r(rc=%d) bash=%r(rc=%d)" % (got, grc, want, wrc))


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    d = tempfile.mkdtemp()
    for n in ("a.md", "b.md", "c.txt"):
        open(os.path.join(d, n), "w").close()

    # --- the two reported shapes -------------------------------------
    same_as_bash("arr=($V) word-splits",
                 'V="a b c"; arr=($V); echo "${#arr[@]}:${arr[0]}:${arr[2]}"')
    same_as_bash("arr=(*.md) globs", 'a=(*.md); echo "${#a[@]}:${a[0]}"', d)

    # --- splitting details -------------------------------------------
    same_as_bash("quoted element stays one field",
                 'V="a b c"; arr=("$V"); echo "${#arr[@]}"')
    same_as_bash("custom IFS is honoured",
                 'IFS=:; V="a:b:c"; arr=($V); echo "${#arr[@]}"')
    same_as_bash("empty variable yields no element",
                 'V=""; arr=($V); echo "${#arr[@]}"')
    same_as_bash("mixed literal and split",
                 'V="b c"; arr=(a $V d); echo "${#arr[@]}:${arr[3]}"')
    same_as_bash("+= append also splits",
                 'V="b c"; arr=(a); arr+=($V); echo "${#arr[@]}"')

    # --- globbing details --------------------------------------------
    same_as_bash("no match stays literal (nullglob off)",
                 'a=(zz*.md); echo "${#a[@]}:${a[0]}"', d)
    same_as_bash("nullglob drops a non-match",
                 'shopt -s nullglob; a=(zz*.md); echo "${#a[@]}"', d)
    same_as_bash("quoted glob does not expand",
                 'a=("*.md"); echo "${#a[@]}:${a[0]}"', d)

    # --- the scalar case must be UNCHANGED (POSIX: no glob) ----------
    same_as_bash("scalar assignment does not glob", 'x=*.md; echo "$x"', d)
    same_as_bash("scalar assignment does not split",
                 'V="a b"; x=$V; echo "[$x]"')

    # --- churn: allocation pressure, per the t_scope_save lesson ------
    out, rc = run(SHELL,
                  'i=0; while [ $i -lt 300 ]; do V="a b c d e"; '
                  'arr=($V); arr+=($V); i=$((i+1)); done; echo "${#arr[@]}"')
    check("300 rounds of split+append do not crash",
          rc == 0 and out.strip() == "10",
          "rc=%d out=%r" % (rc, out.strip()))

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
