#!/usr/bin/env python3
"""zsh's =(cmd): a process substitution to a TEMP FILE, as an assignment value.

WHY THIS EXISTS AS ITS OWN SUITE.  `=(cmd)` landed as a command argument and
worked there -- `cat =(echo hi)` printed hi -- while the same construct on the
right-hand side of an assignment produced two DIFFERENT wrong answers (#83):

    x==(:)                the temp path was handed to the executor as a
                          COMMAND to run, so the shell said
                          "/tmp/hsh...: No such file or directory"
                          and x was left empty                      -- loud
    f() { local t==(:); } the path was simply a second operand that
                          `local` ignored, and t was empty          -- SILENT

The silent one is why this is a suite rather than a line: oh-my-zsh's extract
resolves a name collision with

    local tmp_name==(:); tmp_name="${tmp_name:t}"
    command mv "${content[1]}" "$tmp_name" && ...

and with tmp_name empty the mv fails, the && chain stops, nothing is
destroyed -- and nothing is said either.  The plugin still LOADS, so the
corpus test stays green while its entry point does not work.  Loading is not
running, and only a test that runs it can tell the two apart.

THE BUG WAS NOT ZSH'S.  Measured while fixing it: plain bash `x=<(cmd)` did
exactly the same thing, and no golden case covered it.  The bash half now
lives in tests/procsub_assign (diffable against bash); this file is the zsh
=(cmd) half, which needs the dialect armed and so cannot live there.

Usage: python3 zsh_procsub_test.py [/path/to/hellish]
"""
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []
ENV = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
           HELLISH_NO_ANIM="1")


def check(name, got, want):
    ok = got == want
    print(("ok   " if ok else "FAIL ") + name
          + ("" if ok else "\n       want %r\n       got  %r" % (want, got)))
    if not ok:
        FAILS.append(name)


def run_zsh(script):
    """Sourcing a *.zsh file is how a plugin gets the dialect, and it is what
    the corpus exercises -- `set -o zsh` inside the same -c string arms the
    mode AFTER that text was lexed, so it cannot reach `=(` at all."""
    with tempfile.NamedTemporaryFile("w", suffix=".zsh", delete=False) as f:
        f.write(script + "\n")
        path = f.name
    try:
        p = subprocess.run([SHELL, "-c", "source " + path],
                           capture_output=True, timeout=25, env=ENV)
        return (p.stdout + p.stderr).decode().strip(), p.returncode
    finally:
        os.unlink(path)


# (script, expected stdout).  Temp paths vary per run, so every case asserts
# a DERIVED property -- non-empty, is a file, holds the right bytes -- rather
# than the path itself.
CASES = [
    # The two shapes from #83.  Each asserts non-emptiness, because "empty"
    # is precisely the old wrong answer.
    ('x==(:); [[ -n "$x" ]] && echo nonempty || echo EMPTY', "nonempty"),
    ('f() { local t==(:); [[ -n "$t" ]] && echo nonempty || echo EMPTY; }; f',
     "nonempty"),
    ('f() { typeset t==(:); [[ -n "$t" ]] && echo nonempty; }; f', "nonempty"),
    # ...and that nothing tried to RUN the path.  The old bare form printed
    # "No such file or directory" here; a status of 0 with no output is the
    # whole assertion.
    ('x==(:); echo "rc=$?"', "rc=0"),
    ('f() { local t==(:); echo "rc=$?"; }; f', "rc=0"),
    # The file is real, and holds what the command wrote.
    ('x==(echo body); [[ -f "$x" ]] && echo isfile', "isfile"),
    ('x==(echo body); cat "$x"', "body"),
    ('x==(printf "a\\nb\\n"); wc -l < "$x"', "2"),
    ('x==(:); [[ -f "$x" ]] && wc -c < "$x"', "0"),
    # extract's actual line: :t takes the basename, so an empty value would
    # print nothing at all rather than a name.
    ('f() { local t==(:); t="${t:t}"; [[ -n "$t" ]] && echo named; }; f',
     "named"),
    # Two substitutions in one function get two distinct files.
    ('f() { local a==(echo 1); local b==(echo 2); [[ "$a" != "$b" ]] '
     '&& echo distinct; }; f', "distinct"),
    # Still works as a command ARGUMENT -- the half that already worked, kept
    # here so a fix to the assignment path cannot quietly break it.
    ('cat =(echo hi)', "hi"),
    ('diff =(echo same) =(echo same) && echo identical', "identical"),
    # A separated operand is NOT the assignment's value: `x=` then a path.
    ('x= ; echo "[$x]"', "[]"),
]


def value_cases():
    for script, want in CASES:
        out, _ = run_zsh(script)
        check("value: " + script[:44], out, want)


def temp_file_cases():
    """The temp file must exist while the shell can still see it and be gone
    afterwards -- a =(cmd) that leaked a file per call would fill /tmp on any
    plugin that used it in a loop."""
    out, _ = run_zsh('x==(echo hi); echo "$x"')
    path = out.strip().splitlines()[-1] if out.strip() else ""
    check("temp: path was printed", bool(re.match(r"^/.*", path)), True)
    if path:
        check("temp: cleaned up after exit", os.path.exists(path), False)


def churn_cases():
    """The t_scope_save crash needed allocation pressure to show, so every
    new construct gets a churn case rather than a single line."""
    out, rc = run_zsh(
        'i=0\n'
        'while (( i < 300 )); do\n'
        '  x==(:)\n'
        '  [[ -n "$x" ]] || echo BAD\n'
        '  i=$((i+1))\n'
        'done\n'
        'echo done')
    check("churn: 300 bare assignments", out.splitlines()[-1:], ["done"])
    check("churn: no sanitizer report", "AddressSanitizer" not in out, True)
    check("churn: clean exit", rc, 0)
    out, rc = run_zsh(
        'f() { local t==(echo x); [[ -n "$t" ]] || echo BAD; }\n'
        'i=0; while (( i < 200 )); do f; i=$((i+1)); done; echo done')
    check("churn: 200 local assignments", out.splitlines()[-1:], ["done"])
    check("churn: no sanitizer report (local)",
          "AddressSanitizer" not in out, True)


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    value_cases()
    temp_file_cases()
    churn_cases()
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())
