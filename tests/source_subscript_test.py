#!/usr/bin/env python3
"""Regression test: subscripted assignment survives `source` -- issue #71 P0.

`NAME[$var]=value` was not recognised as an assignment when the text arrived
through `source`/`.`, `eval`, `~/.hellishrc`, `/etc/profile` or
PROMPT_COMMAND. It failed as a COMMAND:

    hellish: line 1: M[k]=VAL: command not found

The identical bytes worked by every other route (script, stdin, -c), which
is what made it a blocker for config frameworks specifically: every registry
and lookup table is an associative array written from a helper function, and
every rc file is sourced.

ROOT CAUSE (the issue's own diagnosis was wrong -- it blamed
is_subscript_key(), which handles `$` subscripts correctly): the reparse
pipeline has THREE passes, and `run_parsed()` ran only two of them.

    src/parsing/parse_tokens.c   reparse_subscript_assigns  <- the pre-pass
                                 reparse_words
                                 reparse_assignment_words
    src/execution/exec_string2.c  (pre-pass MISSING)
                                 reparse_words
                                 reparse_assignment_words

Without the pre-pass, reparse_words splits `M[$1]=VAL` on `$`/`[`/`=` first,
so the assignment classifier only ever sees the fragment `M[` and gives up.
A literal subscript (`M[fixed]=v`) survives because it has no `$` to split
on -- which is exactly the asymmetry the reporter observed.

The three passes now go through one `reparse_all()` so the paths cannot
drift apart again; this test pins both the behaviour and that wiring.

Usage: python3 source_subscript_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    ROOT, "build", "bin", "hellish")
ENV = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
           HELLISH_NO_ANIM="1")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def run(argv, stdin=None):
    return subprocess.run([SHELL] + argv, capture_output=True, text=True,
                          env=ENV, timeout=30, input=stdin)


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    body = ('declare -A M\n'
            'pos() { M[$1]=VAL; }\n'
            'pos k\n'
            'echo "result: [${M[k]}]"\n')

    tmp = tempfile.mkdtemp()
    path = os.path.join(tmp, "repro.hsh")
    with open(path, "w") as f:
        f.write(body)

    # The four routes from the issue. All four must agree.
    routes = [
        ("script mode",    lambda: run([path])),
        ("stdin",          lambda: run([], stdin=body)),
        ("-c inline",      lambda: run(["-c", body])),
        ("-c '. file'",    lambda: run(["-c", ". " + path])),
        ("-c 'source file'", lambda: run(["-c", "source " + path])),
        ("eval",           lambda: run(["-c", "eval '" + body.replace(
            "\n", "; ") + "'"])),
    ]
    for label, fn in routes:
        p = fn()
        got = p.stdout.strip()
        check("%-18s -> M[k] is set" % label, got == "result: [VAL]",
              "got %r stderr=%r" % (got, p.stderr.strip()[:120]))

    # Indexed arrays take the identical path.
    p = run(["-c", ". /dev/stdin"], stdin=(
        'A=()\nadd() { A[$1]=x$1; }\nadd 3\necho "idx:[${A[3]}]"\n'))
    check("indexed array A[$i]=v when sourced",
          p.stdout.strip() == "idx:[x3]",
          "got %r %r" % (p.stdout.strip(), p.stderr.strip()[:120]))

    # A literal subscript always worked -- keep it working.
    p = run(["-c", ". /dev/stdin"], stdin=(
        'declare -A M\nlit() { M[fixed]=v; }\nlit\necho "lit:[${M[fixed]}]"\n'))
    check("literal subscript still works when sourced",
          p.stdout.strip() == "lit:[v]", "got %r" % p.stdout.strip())

    # The corollary that makes this nastier than it looks: store_function()
    # deep-clones the ALREADY-REPARSED body, so a function defined in a
    # sourced file carries the misclassified word forever -- even when it is
    # called later from the top level.
    p = run(["-c", ". " + path + "; pos z; echo \"later:[${M[z]}]\""])
    check("a function defined in a sourced file works when called LATER",
          "later:[VAL]" in p.stdout,
          "got %r %r" % (p.stdout.strip(), p.stderr.strip()[:120]))

    # The classic case the issue leads with: a plugin registry.
    reg = ('declare -A REG\n'
           'register() { REG[$1]="$2"; }\n'
           'register git "git plugin"\n'
           'register docker "docker plugin"\n'
           'echo "n=${#REG[@]} git=${REG[git]}"\n')
    p = run(["-c", ". /dev/stdin"], stdin=reg)
    check("a plugin-style registry works when sourced",
          p.stdout.strip() == "n=2 git=git plugin",
          "got %r %r" % (p.stdout.strip(), p.stderr.strip()[:120]))

    # Hostile values must not be re-parsed (the eval workaround the issue
    # had to ship was unsafe-looking; the real fix must not need it).
    p = run(["-c", ". /dev/stdin"], stdin=(
        'declare -A M\nset_it() { M[$1]="$2"; }\n'
        'set_it "k;touch /tmp/pwned_$$" "v`echo no`;x"\n'
        'echo "keys=${#M[@]}"\n'))
    check("hostile keys/values are not re-executed",
          p.stdout.strip() == "keys=1",
          "got %r %r" % (p.stdout.strip(), p.stderr.strip()[:120]))

    # Wiring: the three passes must go through one helper, called by both
    # pipelines. A fourth pass added to one path must not be addable to only
    # one path again.
    with open(os.path.join(ROOT, "src", "execution",
                           "exec_string2.c")) as f:
        es = f.read()
    with open(os.path.join(ROOT, "src", "parsing", "parse_tokens.c")) as f:
        pt = f.read()
    check("exec_string path runs the shared reparse helper",
          "reparse_all(" in es,
          "run_parsed() still open-codes the pass list")
    check("parse_tokens path runs the same helper",
          "reparse_all(" in pt,
          "the two pipelines can still drift apart")
    check("neither path open-codes reparse_subscript_assigns",
          "reparse_subscript_assigns(" not in es
          and "reparse_subscript_assigns(" not in pt,
          "a pass is still called directly, so drift is still possible")

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
