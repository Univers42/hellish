#!/usr/bin/env python3
"""zsh grammar: `} always { }`, `for a b (...)`, and a `}` that closes a
group without a preceding separator.

Diffed against a real zsh where one is available, because every one of these
has a plausible wrong answer that reads correctly:

    { echo a } always { false }     status 0, not 1. The cleanup block's own
                                    status is DISCARDED. The other rule --
                                    letting it through -- is what you would
                                    write, and it silently rewrites the
                                    result of every body whose cleanup ends
                                    in a test.
    for a b (x)                     runs ONCE with b="", rather than not at
                                    all or wrapping around.
    { echo hi }                     a function body in zsh, a syntax error
                                    in bash, where `}` is reserved only at
                                    the start of a command.

Every case is also checked to still FAIL in the bash dialect, with bash's
own wording. The point of a gated dialect is that the golden suite cannot
regress, and that only holds if the gate actually holds.

Usage: python3 zsh_grammar_test.py [/path/to/hellish]
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

CASES = [
    # ---- } always { } ------------------------------------------------
    ("always-runs", """{ echo body } always { echo cleanup }"""),
    ("always-after-failure",
     """{ false } always { echo ran }; echo "st=$?" """),
    ("always-status-is-the-bodys",
     """{ echo a } always { false }; echo "st=$?" """),
    ("always-body-failure-survives",
     """{ false } always { true }; echo "st=$?" """),
    ("always-both-fail", """{ false } always { false }; echo "st=$?" """),
    ("always-after-return",
     """f() { { echo in; return 3 } always { echo fin } }; f; echo "r=$?" """),
    ("always-after-break",
     """for i in 1 2 3; do { [ $i = 2 ] && break } always { echo "a$i" }; done"""),
    ("always-after-continue",
     """for i in 1 2; do { continue } always { echo "c$i" }; done"""),
    ("always-nested",
     """{ echo out } always { { echo in } always { echo in-fin } }"""),
    ("always-multiline", """{
  echo one
} always {
  echo two
}"""),
    ("always-with-newline-before-brace", """{ echo x } always
{ echo y }"""),
    # ---- for a b (list) ----------------------------------------------
    ("for-two-names", """for a b (x 1 y 2); do echo "$a=$b"; done"""),
    ("for-three-names",
     """for a b c (1 2 3 4 5 6); do echo "$a-$b-$c"; done"""),
    ("for-short-final-group",
     """for a b (x); do echo "a=$a b=[$b]"; done"""),
    ("for-ragged",
     """for a b (1 2 3); do echo "a=$a b=[$b]"; done"""),
    ("for-one-name-paren", """for x (p q r); do echo "$x"; done"""),
    ("for-multiline-list", """for a b (
  m 1
  n 2
); do echo "$a=$b"; done"""),
    ("for-glob-in-list",
     """for a (a1 a2); do echo "$a"; done"""),
    ("for-posix-still-works", """for x in u v; do echo "$x"; done"""),
    ("for-posix-positional",
     """set -- p q; for x; do echo "$x"; done"""),
    # ---- `}` closing without a separator -----------------------------
    ("brace-no-semicolon", """f() { echo hi }; f"""),
    ("brace-two-commands", """g() { echo a; echo b }; g"""),
    ("brace-group-inline", """{ echo grouped }"""),
    ("brace-nested", """h() { { echo inner } }; h"""),
]

# Where hellish is MORE PERMISSIVE than zsh, recorded rather than hidden.
# zsh reserves `}` everywhere and rejects an empty `for` list; hellish
# accepts both. The divergence is one-directional -- everything zsh accepts,
# hellish accepts identically, which is what the oracle cases above pin --
# and closing it would mean reserving `}` in a .zsh file even where it is
# harmless text. Same call as the `function` keyword: do not promote a
# character to a keyword to match a rejection nobody relies on.
PERMISSIVE = [
    ("echo }", b"}\n"),
    ("echo a } b", b"a } b\n"),
    ("for a b (); do echo nope; done; echo done", b"done\n"),
]

# Each of these must still be a syntax error in the BASH dialect, and say so
# the way bash says it.
GATED = [
    "{ echo body } always { echo cleanup }",
    "for a b (x 1 y 2); do echo $a; done",
    "for x (p q); do echo $x; done",
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


def run(argv, timeout=15, **kw):
    try:
        p = subprocess.run(argv, capture_output=True, timeout=timeout, **kw)
        return p.returncode, p.stdout, p.stderr
    except (subprocess.TimeoutExpired, OSError) as e:
        return -1, b"<" + str(e).encode() + b">", b""


def hsh_zsh(script, d):
    """Run under the zsh dialect the way a plugin gets it: from a .zsh file.
    `-c` cannot be used for the grammar cases -- the whole -c string is lexed
    before `set -o zsh` runs, so the dialect would not be armed in time. That
    is not a limitation to work around, it is what makes a plugin's dialect a
    property of its FILE rather than of whatever ran before it."""
    path = os.path.join(d, "case.zsh")
    with open(path, "w") as fh:
        fh.write(script + "\n")
    return run([SHELL, "-c", "source %s" % path])


def oracle_cases(zsh, d):
    for name, script in CASES:
        path = os.path.join(d, "case.zsh")
        with open(path, "w") as fh:
            fh.write(script + "\n")
        _, zout, _ = run([zsh, "-f", path])
        _, hout, herr = hsh_zsh(script, d)
        check("grammar/" + name, hout == zout,
              "zsh=%r hellish=%r %r" % (zout, hout, herr[:100]))


def gate_cases():
    """The bash dialect must still reject all of it, in bash's words."""
    for script in GATED:
        rc, out, err = run([SHELL, "-c", script])
        check("gate/rejects: %s" % script[:34],
              rc != 0 and b"syntax error" in err,
              "rc=%d err=%r" % (rc, err[:100]))
    # ...and names the same token bash names. Only the `for` forms are
    # compared: bash and hellish diagnose the unterminated `{ } always { }`
    # differently (end-of-file vs the newline), and both are right about an
    # input that is invalid either way -- forcing agreement there would mean
    # matching bash's recovery, not its grammar.
    for script in GATED[1:]:
        _, _, err = run([SHELL, "-c", script])
        _, _, berr = run(["bash", "-c", script])
        tok = berr.split(b"unexpected token ")[-1].split(b"\n")[0]
        check("gate/names-the-same-token: %s" % script[:24],
              tok in err, "hellish=%r bash-token=%r" % (err[:80], tok))
    # `always` is an ordinary command name in the bash dialect.
    _, out, _ = run([SHELL, "-c",
                     "always() { echo cmd; }; { echo x; }; always"])
    check("gate/always-is-a-command-name", out == b"x\ncmd\n", "out=%r" % out)
    # ...and so is `}` as an argument.
    _, out, _ = run([SHELL, "-c", "echo }"])
    check("gate/rbrace-is-a-word", out == b"}\n", "out=%r" % out)


def scope_cases(d):
    """A .sh file next to a .zsh one keeps bash rules."""
    sh = os.path.join(d, "helper.sh")
    with open(sh, "w") as fh:
        fh.write("f() { echo hi }\n")
    rc, _, err = run([SHELL, "-c", "source %s" % sh])
    check("scope/sh-file-stays-bash", rc == 2 and b"syntax error" in err,
          "rc=%d err=%r" % (rc, err[:100]))
    # A .zsh file that sources a .sh helper KEEPS zsh rules for the helper,
    # which is what real zsh does -- the dialect is a property of the shell,
    # and the extension only decides when to arm it. A plugin's helper is
    # written for the plugin's dialect, so reverting on the way in would
    # break the common case to serve a naming convention.
    z = os.path.join(d, "outer.zsh")
    with open(z, "w") as fh:
        fh.write("g() { echo ok }\nsource %s\nf\n" % sh)
    rc, out, err = run([SHELL, "-c", "source %s" % z])
    check("scope/nested-sh-inherits-the-dialect",
          rc == 0 and out == b"hi\n", "rc=%d out=%r err=%r"
          % (rc, out, err[:100]))


def permissive_cases(d):
    """Recorded divergences: hellish accepts these, zsh rejects them."""
    for script, want in PERMISSIVE:
        _, out, _ = hsh_zsh(script, d)
        check("permissive/%s" % script[:28], out == want,
              "out=%r want=%r" % (out, want))


def unsupported_cases():
    """A zsh builtin we do not implement must still be KNOWN, so a plugin
    that calls it loads. Left unregistered they produced `command not found`
    and made the whole file exit 127 -- which reads as a broken plugin rather
    than one missing feature.

    zle and bindkey moved OFF this list when the widget layer landed
    (tests/zle_test.py owns them now). compdef, zstyle and zmodload are
    still stubs: they configure the zsh completion system and loadable
    modules, neither of which exists here."""
    for name in ("compdef", "zstyle", "zmodload"):
        rc, _, err = run([SHELL, "-c", "%s foo" % name])
        check("unsupported/%s-is-known" % name,
              b"command not found" not in err and b"not supported" in err,
              "err=%r" % err[:120])
    _, _, err = run([SHELL, "-c", "zstyle a; zstyle b; zstyle c"])
    check("unsupported/says-it-once", err.count(b"not supported") == 1,
          "count=%d" % err.count(b"not supported"))
    # ...and the two that are now real are NOT stubs.
    for name in ("zle", "bindkey"):
        _, _, err = run([SHELL, "-c", "%s" % name])
        check("unsupported/%s-is-implemented-now" % name,
              b"not supported" not in err, "still a stub: %r" % err[:100])


def churn_cases(d):
    path = os.path.join(d, "churn.zsh")
    with open(path, "w") as fh:
        fh.write("i=0\n"
                 "while [ $i -lt 300 ]; do\n"
                 "  { true } always { true }\n"
                 "  for a b (x 1 y 2); do : ; done\n"
                 "  f() { echo $a }\n"
                 "  i=$((i+1))\n"
                 "done\necho done\n")
    rc, out, err = run([SHELL, "-c", "source %s" % path], timeout=60)
    check("churn/300-rounds-clean", rc == 0 and b"done" in out,
          "rc=%d err=%r" % (rc, err[:200]))
    check("churn/no-sanitizer-report",
          b"AddressSanitizer" not in err and b"LeakSanitizer" not in err,
          "err=%r" % err[:300])
    # Malformed forms must report and not crash.
    for bad in ["{ echo a } always", "{ echo a } always echo",
                "for a b (", "for a b (x", "{ always { echo x } }",
                "for a b (x 1); do"]:
        p2 = os.path.join(d, "bad.zsh")
        with open(p2, "w") as fh:
            fh.write(bad + "\n")
        rc, _, err = run([SHELL, "-c", "source %s" % p2])
        check("churn/malformed reports: %s" % bad[:26],
              rc != 0 and b"AddressSanitizer" not in err
              and err.strip() != b"",
              "rc=%d err=%r" % (rc, err[:120]))


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    with tempfile.TemporaryDirectory() as d:
        gate_cases()
        scope_cases(d)
        permissive_cases(d)
        unsupported_cases()
        churn_cases(d)
        zsh = find_zsh()
        if zsh:
            print("--- oracle: %s" % subprocess.run(
                [zsh, "--version"],
                capture_output=True).stdout.decode().strip())
            oracle_cases(zsh, d)
        else:
            print("SKIP grammar/*  (no zsh; run `make zsh-oracle`)")
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())
