#!/usr/bin/env python3
"""The zsh builtins -- setopt, unsetopt, emulate, print, autoload -- and the
silent-parse-error fix that hunting them down turned up.

Same arrangement as tests/zsh_flags_test.py: `print` is diffed against a
real zsh where one is available, because its DEFAULTS are the part that
differs from echo and defaults are what a hand-written assertion gets wrong.
The rest is asserted directly, since it has no zsh counterpart worth
diffing (option names hellish maps, and hellish's own error wording).

THE SILENT PARSE ERRORS
-----------------------
Chasing why oh-my-zsh's git plugin returned 2 and printed nothing found a
bug with nothing to do with zsh: about a dozen parser productions set
RES_ERR without reporting anything, and neither finish point covered them.
So `for x (a b)` -- and `source` of any file with an unterminated brace --
failed with status 2 and complete silence. bash prints in both cases. The
fix is one `reported` flag on t_parser plus a fallback at the two finish
points; these cases pin it, because a silent failure is the one outcome
that cannot be debugged from the outside.

Usage: python3 zsh_builtins_test.py [/path/to/hellish]
"""
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []

# print's whole point is that its defaults differ from echo's, so these are
# diffed rather than asserted.
PRINT_CASES = [
    ("escapes-on-by-default", r'''print "a\tb"'''),
    ("r-turns-them-off", r'''print -r "a\tb"'''),
    ("n-no-newline", r'''print -n x; print -n y'''),
    ("l-one-per-line", r'''print -l one two three'''),
    ("l-with-n", r'''print -nl a b'''),
    ("dashdash-ends-options", r'''print -- -x'''),
    ("no-args-is-a-newline", r'''print'''),
    ("empty-string", r'''print ""'''),
    ("many-escapes", r'''print "a\nb\tc\\d"'''),
    ("R-is-raw-too", r'''print -R "a\tb"'''),
]


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (("  " + detail)
                                                 if not ok else ""))
    if not ok:
        FAILS.append(name)


def find_zsh():
    env = os.environ.get("ZSH_ORACLE")
    if env and os.path.exists(env):
        return env
    home = os.path.expanduser("~/zsh-5.9/bin/zsh")
    for c in (home, shutil.which("zsh"), "/usr/bin/zsh", "/bin/zsh"):
        if c and os.path.exists(c):
            return c
    return None


def run(argv, script, timeout=15):
    try:
        p = subprocess.run(argv + [script], capture_output=True,
                           timeout=timeout)
        return p.returncode, p.stdout, p.stderr
    except (subprocess.TimeoutExpired, OSError) as e:
        return -1, b"<" + str(e).encode() + b">", b""


def hsh(script, timeout=15):
    return run([SHELL, "-c"], script, timeout)


def print_oracle(zsh):
    for name, script in PRINT_CASES:
        _, zout, _ = run([zsh, "-f", "-c"], script)
        _, hout, herr = hsh(script)
        check("print/" + name, hout == zout,
              "zsh=%r hellish=%r %r" % (zout, hout, herr[:80]))


def print_assertions():
    """The options we refuse, which zsh implements and we do not."""
    for flag in ("z", "s", "u", "p"):
        rc, out, err = hsh('print -%s hi' % flag)
        check("print/-%s-refuses" % flag,
              rc != 0 and b"not supported" in err,
              "rc=%d err=%r" % (rc, err[:100]))
    # -P routes through the same renderer PROMPT uses, so the two agree.
    _, a, _ = hsh('print -rP "%d"')
    _, b, _ = hsh('PROMPT="%d"; print -rP "%d"')
    check("print/-P-renders", a == b and a.strip() != b"", "a=%r b=%r" % (a, b))


def setopt_cases():
    _, out, _ = hsh("setopt nullglob; shopt nullglob")
    check("setopt/maps-to-shopt", b"on" in out, "out=%r" % out)
    _, out, _ = hsh("setopt nullglob; unsetopt nullglob; shopt nullglob")
    check("setopt/unsetopt-clears", b"off" in out, "out=%r" % out)
    # zsh names ignore case and underscores, and a `no` prefix inverts.
    _, out, _ = hsh("setopt NO_NULL_GLOB; shopt nullglob")
    check("setopt/no-prefix-inverts", b"off" in out, "out=%r" % out)
    _, out, _ = hsh("setopt NULLGLOB; shopt nullglob")
    check("setopt/case-insensitive", b"on" in out, "out=%r" % out)
    _, out, _ = hsh("unsetopt nonullglob; shopt nullglob")
    check("setopt/double-negative", b"on" in out, "out=%r" % out)
    # `notify` starts with "no" but is not an inversion of "tify".
    rc, _, err = hsh("setopt notify; echo rc=$?")
    check("setopt/notify-is-not-no-tify", b"no such option" not in err,
          "err=%r" % err[:100])
    # Unknown names get zsh's message and status, and execution CONTINUES.
    rc, out, err = hsh("setopt definitelyNotAnOption; echo after=$?")
    check("setopt/unknown-is-loud", b"no such option" in err, "err=%r" % err[:100])
    check("setopt/unknown-does-not-abort", b"after=1" in out, "out=%r" % out)
    # Real zsh options with no behaviour here are accepted, not refused --
    # most plugins open with `setopt localoptions`.
    rc, out, err = hsh("setopt localoptions; echo rc=$?")
    check("setopt/known-inert-accepted", b"rc=0" in out and not err.strip(),
          "out=%r err=%r" % (out, err[:100]))


def emulate_cases():
    _, out, _ = hsh('emulate zsh; x=hi; echo "${(U)x}"')
    check("emulate/zsh-arms-dialect", out == b"HI\n", "out=%r" % out)
    _, out, err = hsh('emulate zsh; emulate sh; x=hi; echo "${(U)x}"')
    check("emulate/sh-disarms", b"bad substitution" in err, "err=%r" % err[:100])
    rc, _, err = hsh("emulate tcsh")
    check("emulate/unknown-shell-errors", rc != 0 and b"no such shell" in err,
          "rc=%d err=%r" % (rc, err[:100]))
    rc, _, err = hsh("emulate")
    check("emulate/no-args-errors", rc != 0, "rc=%d" % rc)
    # -L is already local, because the call frame restores the dialect on
    # return. That is the whole reason it needs no separate mechanism.
    _, out, err = hsh('f() { emulate -L zsh; }; f; x=hi; echo "${(U)x}"')
    check("emulate/-L-reverts-on-return", b"bad substitution" in err,
          "out=%r err=%r" % (out, err[:100]))


def autoload_cases():
    with tempfile.TemporaryDirectory() as d:
        with open(os.path.join(d, "greetfn"), "w") as fh:
            fh.write("greetfn() { echo hello-from-fpath; }\n")
        _, out, _ = hsh('fpath=%s; autoload -Uz greetfn; greetfn' % d)
        check("autoload/loads-from-fpath", out == b"hello-from-fpath\n",
              "out=%r" % out)
        _, out, _ = hsh('FPATH=%s; autoload -Uz greetfn; greetfn' % d)
        check("autoload/honours-FPATH", out == b"hello-from-fpath\n",
              "out=%r" % out)
    # A name with nothing on fpath is not an error: zsh defers the failure
    # to the call, and a plugin's optional helper must not stop the load.
    rc, _, err = hsh("autoload -Uz definitely_not_there; echo ok")
    check("autoload/missing-is-not-an-error", rc == 0, "rc=%d err=%r" % (rc, err[:100]))


def unsupported_cases():
    rc, _, err = hsh("zstyle ':completion:*' menu select")
    check("zstyle/reports-once", rc != 0 and b"not supported" in err,
          "rc=%d err=%r" % (rc, err[:120]))
    _, _, err = hsh("zstyle a b; zstyle c d; zstyle e f")
    check("zstyle/does-not-spam", err.count(b"not supported") == 1,
          "count=%d" % err.count(b"not supported"))
    rc, _, err = hsh("zmodload zsh/complist")
    check("zmodload/reports", rc != 0 and b"not supported" in err,
          "rc=%d err=%r" % (rc, err[:120]))


def registration_cases():
    """A builtin that dispatch knows must also be in help and completion --
    both are test-enforced elsewhere, so this only checks they agree here."""
    for name in ("setopt", "unsetopt", "emulate", "print", "autoload",
                 "zmodload", "zstyle"):
        _, out, _ = hsh("type %s" % name)
        check("register/%s-is-a-builtin" % name, b"builtin" in out,
              "out=%r" % out)
        _, out, _ = hsh("help %s" % name)
        check("register/%s-has-help" % name, out.strip() != b"",
              "out=%r" % out)


def silent_parse_error_cases():
    """No parse failure may be silent. Each of these set $? to 2 and printed
    nothing before the `reported` flag existed."""
    for label, script in [
            ("for-paren", "for x ( a )"),
            ("for-multi-var", "for a b (x 1 y 2); do :; done"),
            ("while-bad", "while ( ; do :; done"),
            ("case-bad", "case x in ( ;; esac"),
    ]:
        rc, _, err = hsh(script)
        check("silent/%s-reports" % label,
              rc != 0 and b"syntax error" in err,
              "rc=%d err=%r" % (rc, err[:120]))
    # ...and through source and eval, which use a different finish point.
    with tempfile.TemporaryDirectory() as d:
        bad = os.path.join(d, "bad.sh")
        with open(bad, "w") as fh:
            fh.write("echo before\nf() { echo hi }\necho after\n")
        rc, _, err = hsh("source %s" % bad)
        check("silent/source-reports", rc == 2 and b"syntax error" in err,
              "rc=%d err=%r" % (rc, err[:120]))
    rc, _, err = hsh('eval "f() { echo hi"')
    check("silent/eval-reports", rc == 2 and b"syntax error" in err,
          "rc=%d err=%r" % (rc, err[:120]))
    # Wording is bash's, not an approximation of it.
    _, _, err = hsh("for x ( a )")
    check("silent/wording-matches-bash",
          b"syntax error near unexpected token `('" in err, "err=%r" % err)


def churn_cases():
    script = ("i=0\n"
              "while [ $i -lt 300 ]; do\n"
              "  setopt nullglob; unsetopt nullglob\n"
              "  print -l a b > /dev/null\n"
              "  emulate zsh; emulate sh\n"
              "  i=$((i+1))\n"
              "done\necho done\n")
    rc, out, err = hsh(script, timeout=60)
    check("churn/300-rounds-clean", rc == 0 and b"done" in out,
          "rc=%d err=%r" % (rc, err[:200]))
    check("churn/no-sanitizer-report",
          b"AddressSanitizer" not in err and b"LeakSanitizer" not in err,
          "err=%r" % err[:300])


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    setopt_cases()
    emulate_cases()
    autoload_cases()
    unsupported_cases()
    registration_cases()
    print_assertions()
    silent_parse_error_cases()
    churn_cases()
    zsh = find_zsh()
    if zsh:
        print("--- oracle: %s" % subprocess.run(
            [zsh, "--version"], capture_output=True).stdout.decode().strip())
        print_oracle(zsh)
    else:
        print("SKIP print/*  (no zsh; run `make zsh-oracle` or set ZSH_ORACLE)")
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())
