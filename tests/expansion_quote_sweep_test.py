#!/usr/bin/env python3
"""Sweep: no ${...} operator may crash the shell, whatever quoting it holds.

The report behind this file was a segfault:

    $ x=a/b; echo ${x//"/"/_}
    Segmentation fault (core dumped)

bash prints a_b. expand_subst split the pattern from the replacement at the
FIRST '/', inside the quotes, so the pattern handed to the reparser was a
lone `"`. The reparser trusted the lexer to have balanced every quote, ran
to the end of that one-byte slice, and tripped ft_assert on the missing
closing quote -- and ft_assert is a deliberate null write. So a typo, or a
perfectly valid bash word, took the whole interactive session down.

It crashed only in some builds. The debug build and the release build
`make my_shell` installs (libc malloc) died on signal 11; the ft_malloc
release build survived the same input and printed a wrong answer. A single
hand-written case run on one build can pass while the bug is still there,
which is why this is a SWEEP: every operator, crossed with every quoting
and nesting token, in both quoting contexts, at several positions. It is
the class that must not come back, not the one line.

Checks, per generated case:
  1. the shell is not killed by a signal and no sanitizer report appears;
  2. when bash is available, hellish agrees with it: same stdout and exit
     status as `bash --posix` (the golden suite's oracle) or as plain
     bash, or -- for input both reject -- no output and a failing status.

Why "or plain bash": the two modes disagree on a few of these. In posix
mode bash accepts "${x/'"'}" at parse time and then fails it at expansion
time with "bad substitution: no closing `}'"; plain bash prints the value.
hellish follows the second, which is the consistent reading.

Why "both reject": a syntax error inside a nested $( ) is status 127 in
bash and 2 for an unterminated one; the sweep checks that bad input is
refused, and the golden list (tests/patsub_quoting) pins exact statuses.

Run it against every build configuration -- the debug/ASan build finds the
out-of-bounds reads that a release build survives by luck:

    python3 tests/expansion_quote_sweep_test.py build/bin/hellish

Usage: python3 expansion_quote_sweep_test.py [/path/to/hellish]
"""
import concurrent.futures
import itertools
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
BASH = shutil.which("bash")
FAILS = []

OPS = ["/", "//", "/#", "/%", "#", "##", "%", "%%", "^", "^^", ",", ",,",
       ":-", ":=", ":+", "-", "+", "="]

# Quoting and nesting tokens, balanced and not. The unbalanced ones are the
# point: they are what reached the reparser as a one-byte slice.
TOKENS = ["'", '"', "\\", "`", "$(", "${", "$((", "}", "/",
          "'/'", '"/"', "\\/", "'}'", '"}"', "$(echo /)", "`echo /`",
          "${y:-/}", "\"'\"", "'\"'", "\\'", '\\"']

# {t} is the token. Each template puts it somewhere different relative to
# the operator's own '/' separators.
TEMPLATES = ["{op}{t}", "{op}a{t}b", "{op}{t}/r", "{op}a/{t}", "{op}{t}{t}/{t}"]

VALUE = "x='a/b}c'\"'\"'d'; "


def cases():
    for op, t, tpl in itertools.product(OPS, TOKENS, TEMPLATES):
        body = "${x" + tpl.format(op=op, t=t) + "}"
        yield VALUE + "echo " + body
        yield VALUE + 'echo "' + body + '"'


def run(argv, cmd):
    env = dict(os.environ, LC_ALL="C",
               ASAN_OPTIONS="detect_leaks=0:abort_on_error=0")
    try:
        p = subprocess.run(argv + ["-c", cmd], capture_output=True,
                           timeout=10, env=env, stdin=subprocess.DEVNULL)
    except subprocess.TimeoutExpired:
        return None
    return p


def crashed(p):
    if p is None:
        return "timeout"
    if p.returncode < 0 or p.returncode >= 128:
        return "status %d" % p.returncode
    err = p.stderr.decode("utf-8", "replace")
    for mark in ("AddressSanitizer", "runtime error:", "DEADLYSIGNAL",
                 "LeakSanitizer"):
        if mark in err:
            return "sanitizer: " + err.strip().splitlines()[0][:100]
    return None


def check(cmd):
    got = run([SHELL, "--norc"], cmd)
    why = crashed(got)
    if why:
        return (cmd, "CRASH " + why)
    if not BASH:
        return None
    want = run([BASH, "--posix"], cmd)
    if want is None or agrees(got, want):
        return None
    plain = run([BASH], cmd)
    if plain is not None and agrees(got, plain):
        return None
    return (cmd, "DIFF hellish %r/%d  bash --posix %r/%d" % (
        got.stdout.decode("utf-8", "replace"), got.returncode,
        want.stdout.decode("utf-8", "replace"), want.returncode))


def agrees(got, want):
    if (got.stdout, got.returncode) == (want.stdout, want.returncode):
        return True
    return (not got.stdout and not want.stdout
            and got.returncode != 0 and want.returncode != 0)


def main():
    if not os.access(SHELL, os.X_OK):
        print("FAIL shell not found: %s" % SHELL)
        return 1
    all_cases = list(cases())
    with concurrent.futures.ThreadPoolExecutor(os.cpu_count() or 4) as ex:
        for res in ex.map(check, all_cases):
            if res:
                FAILS.append(res)
    crashes = [f for f in FAILS if f[1].startswith("CRASH")]
    diffs = [f for f in FAILS if f[1].startswith("DIFF")]
    for cmd, why in (crashes + diffs)[:40]:
        print("FAIL %s\n       %s" % (cmd, why))
    print("%s %d cases: %d crashes, %d differences from bash"
          % ("ok  " if not FAILS else "FAIL", len(all_cases),
             len(crashes), len(diffs)))
    return 1 if FAILS else 0


if __name__ == "__main__":
    sys.exit(main())
