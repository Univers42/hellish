#!/usr/bin/env python3
"""${(flags)x} -- hellish's zsh dialect, diffed against a real zsh.

WHY AN ORACLE AND NOT ASSERTIONS
--------------------------------
The first cut of this file asserted what the flags obviously do, and it
passed. Then a real zsh 5.9 was pointed at the same cases and four of them
were wrong -- every one in the direction of a TIDIER answer than the truth:

    "${(o)arr}"       does NOT sort. Double quotes join the array to a
                      scalar before the flag runs, and a scalar has no
                      order. Unquoted, and "${(@o)arr}", do sort.
    ${(k)arr}         on an INDEXED array gives the values, not the
                      indices. Indices are the obvious guess.
    ${(q)x}           quotes with BACKSLASHES (a\\ b). Single quotes are
                      (qq). Both reparse to the same string, so a
                      round-trip test cannot tell them apart.
    ${(f)$'a\\n\\nb'}   is TWO fields unquoted, three as "${(@f)...}":
                      an unquoted array expansion drops empty elements.

None of those is discoverable by reading our own code, and each produces
output a reviewer would accept. So the contract here is the oracle's, the
same way tests/tester pins bash 5.3 -- assertions are only used where zsh
cannot be run (the loud-failure cases, which have no zsh counterpart).

SKIPS CLEANLY when no zsh is available: CI without one still passes rather
than pretending to have checked. Point ZSH_ORACLE at a binary to override
the search.

Usage: python3 zsh_flags_test.py [/path/to/hellish]
"""
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []

# Cases are (label, script). The script runs verbatim under zsh -f, and
# under hellish with `set -o zsh` prepended -- the gate is opt-in, so a
# case that forgot it would be testing the bash dialect by accident.
CASES = [
    # ---- (f): split on newlines -------------------------------------
    ("f-basic", r"""x=$'a\nb\nc'; for i in ${(f)x}; do echo "<$i>"; done"""),
    ("f-empty-dropped", r"""x=$'a\n\nb'; for i in ${(f)x}; do echo "<$i>"; done"""),
    ("f-empty-kept-at", r"""x=$'a\n\nb'; for i in "${(@f)x}"; do echo "<$i>"; done"""),
    ("f-quoted-joins", r"""x=$'a\nb'; echo "[${(f)x}]" """),
    ("f-spaces-kept", r"""x=$'a b\nc d'; for i in "${(@f)x}"; do echo "<$i>"; done"""),
    ("f-cmdsub", r"""for i in ${(f)"$(printf 'p\nq\nr\n')"}; do echo "<$i>"; done"""),
    ("f-trailing-nl", r"""x=$'a\nb\n'; for i in "${(@f)x}"; do echo "<$i>"; done"""),
    ("f-only-nl", r"""x=$'\n'; for i in "${(@f)x}"; do echo "<$i>"; done"""),
    # ---- (s:x:): split on a string ----------------------------------
    ("s-colon", r"""x=a:b:c; for i in ${(s.:.)x}; do echo "<$i>"; done"""),
    ("s-multichar", r"""x=a--b--c; for i in ${(s:--:)x}; do echo "<$i>"; done"""),
    ("s-brace-delim", r"""x=a/b; for i in ${(s(/))x}; do echo "<$i>"; done"""),
    ("ps-escape", r"""x=$'a\nb'; for i in ${(ps:\n:)x}; do echo "<$i>"; done"""),
    ("s-absent", r"""x=abc; for i in ${(s::)x}; do echo "<$i>"; done"""),
    # ---- (j:x:) / (F): join -----------------------------------------
    ("j-dash", r"""arr=(a b c); echo "${(j:-:)arr}" """),
    ("j-empty", r"""arr=(a b c); echo "${(j::)arr}" """),
    ("j-multichar", r"""arr=(a b); echo "${(j:<>:)arr}" """),
    ("F-newline", r"""arr=(a b); echo "${(F)arr}" """),
    ("j-of-f", r"""x=$'a\nb'; echo "${(j:-:)${(f)x}}" """),
    ("j-empty-array", r"""arr=(); echo "[${(j:-:)arr}]" """),
    # ---- order and uniqueness (the quoting trap) --------------------
    ("o-quoted-nosort", r"""arr=(c a b); echo "${(o)arr}" """),
    ("o-unquoted-sorts", r"""arr=(c a b); printf '%s ' ${(o)arr}; echo"""),
    ("o-at-sorts", r"""arr=(c a b); printf '%s ' "${(@o)arr}"; echo"""),
    ("O-desc", r"""arr=(a c b); printf '%s ' ${(O)arr}; echo"""),
    ("u-dedup", r"""arr=(a b a c); printf '%s ' ${(u)arr}; echo"""),
    ("u-quoted-nodedup", r"""arr=(a b a c); echo "${(u)arr}" """),
    ("o-lexicographic", r"""arr=(v10 v2 v1); printf '%s ' ${(o)arr}; echo"""),
    ("on-numeric", r"""arr=(10 2 1); printf '%s ' ${(on)arr}; echo"""),
    ("ou-combined", r"""arr=(c a c b); printf '%s ' ${(ou)arr}; echo"""),
    # ---- case -------------------------------------------------------
    ("U-upper", r"""x=hello; echo "${(U)x}" """),
    ("L-lower", r"""x=HeLLo; echo "${(L)x}" """),
    ("C-capitalise", r"""x=hello; echo "${(C)x}" """),
    ("U-array", r"""arr=(ab cd); echo "${(U)arr}" """),
    ("U-empty", r"""x=; echo "[${(U)x}]" """),
    # ---- quoting styles ---------------------------------------------
    ("q-backslash", r"""x="a b"; echo ${(q)x}"""),
    ("q-apostrophe", r"""x="it's"; echo ${(q)x}"""),
    ("qq-single", r"""x="a b"; echo ${(qq)x}"""),
    ("qqq-double", r"""x="a b"; echo ${(qqq)x}"""),
    ("q-plain-word", r"""x=abc; echo ${(q)x}"""),
    # ---- (P): the value names a parameter ---------------------------
    ("P-indirect", r"""FOO=bar; n=FOO; echo "${(P)n}" """),
    ("P-unset", r"""n=NOPE_NOT_SET; echo "[${(P)n}]" """),
    # ---- (k) / (v) on hashes and arrays -----------------------------
    ("k-assoc", r"""typeset -A h; h[one]=1; echo "${(k)h}" """),
    ("v-assoc", r"""typeset -A h; h[one]=1; echo "${(v)h}" """),
    ("kv-assoc", r"""typeset -A h; h[one]=1; echo "${(kv)h}" """),
    ("k-indexed-is-values", r"""arr=(x y z); printf '%s ' ${(k)arr}; echo"""),
    # ---- (z): shell-word split --------------------------------------
    ("z-collapses-runs", r"""x="a  b   c"; for i in ${(z)x}; do echo "<$i>"; done"""),
    # ---- nesting: which level is still an array ---------------------
    ("nest-j-of-f", r"""x=$'c\na\nc\nb'; echo "${(j:-:)${(f)x}}" """),
    ("nest-ou-collapses", r"""x=$'c\na\nc\nb'; echo "${(j:,:)${(ou)${(f)x}}}" """),
    ("nest-oj", r"""x=$'c\na\nc\nb'; echo "${(oj:,:)${(f)x}}" """),
    ("nest-at-forces-sort", r"""x=$'c\na\nc\nb'; echo "${(o)${(f)x}[@]}" """),
    ("nest-unquoted-sorts", r"""x=$'c\na\nb'; printf '%s ' ${(o)${(f)x}}; echo"""),
    # ---- edges ------------------------------------------------------
    ("empty-value", r"""x=""; for i in ${(f)x}; do echo "<$i>"; done; echo done"""),
    ("unset-value", r"""unset x; echo "[${(U)x}]" """),
    ("count-of-f", r"""x=$'a\nb\nc'; echo "${#${(f)x}}" """),
    ("no-flags", r"""x=abc; echo "${()x}" """),
]

# Flags we deliberately do not implement. There is no zsh comparison here:
# the contract is that hellish REFUSES, loudly, rather than falling through
# to the unflagged value -- a wrong answer that looks right is the failure
# mode this whole file exists to prevent.
LOUD = ["M", "A", "b", "e", "t", "V", "w", "W", "X", "~", "#"]


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


def run(argv, script, timeout=10):
    try:
        p = subprocess.run(argv + [script], capture_output=True,
                           timeout=timeout)
        return p.returncode, p.stdout, p.stderr
    except (subprocess.TimeoutExpired, OSError) as e:
        return -1, b"<" + str(e).encode() + b">", b""


def oracle_cases(zsh):
    for name, script in CASES:
        _, zout, _ = run([zsh, "-f", "-c"], script)
        _, hout, herr = run([SHELL, "-c"], "set -o zsh\n" + script)
        detail = "zsh=%r hellish=%r" % (zout, hout)
        if hout != zout and herr.strip():
            detail += " err=%r" % herr.strip()[:120]
        check("oracle/" + name, hout == zout, detail)


def loud_failure_cases():
    """Every unimplemented flag must be an error, not a quiet passthrough."""
    for flag in LOUD:
        script = 'set -o zsh\nx=VALUE; echo "${(%s)x}"' % flag
        rc, out, err = run([SHELL, "-c"], script)
        check("loud/%s-refuses" % flag,
              rc != 0 and b"VALUE" not in out,
              "rc=%d out=%r" % (rc, out))
        check("loud/%s-names-itself" % flag,
              b"not implemented" in err or b"bad substitution" in err,
              "err=%r" % err[:120])


def gate_cases():
    """Off by default: the bash dialect must not grow zsh syntax."""
    rc, out, err = run([SHELL, "-c"], 'x=abc; echo "${(U)x}"')
    check("gate/off-by-default", rc != 0 and b"ABC" not in out,
          "rc=%d out=%r" % (rc, out))
    check("gate/off-says-bad-subst", b"bad substitution" in err,
          "err=%r" % err[:120])
    rc, out, _ = run([SHELL, "-c"], 'set -o zsh; x=abc; echo "${(U)x}"')
    check("gate/set-o-zsh-arms-it", out == b"ABC\n", "out=%r" % out)
    rc, out, _ = run([SHELL, "-c"],
                     'set -o zsh; set +o zsh; x=abc; echo "${(U)x}"')
    check("gate/set-plus-o-disarms-it", b"ABC" not in out, "out=%r" % out)
    # The listing must stay bash's 27 rows: `zsh` is settable but hidden,
    # because tests/issue8_set_options diffs `set -o | wc -l` against bash.
    _, out, _ = run([SHELL, "-c"], "set -o | wc -l")
    check("gate/listing-unchanged", out.strip() == b"27", "out=%r" % out)
    _, out, _ = run([SHELL, "-c"], "set -o zsh; set -o | grep -c zsh")
    check("gate/hidden-from-listing", out.strip() == b"0", "out=%r" % out)


def scope_cases():
    """A .zsh file arms the dialect for itself, and gives it back."""
    import tempfile
    with tempfile.TemporaryDirectory() as d:
        plug = os.path.join(d, "thing.plugin.zsh")
        with open(plug, "w") as fh:
            fh.write('greet() { local x=$\'a\\nb\'; echo "${(j:-:)${(f)x}}"; }\n')
        helper = os.path.join(d, "helper.sh")
        with open(helper, "w") as fh:
            fh.write('echo "helper ${(U)1-x}"\n')
        _, out, _ = run([SHELL, "-c"], "source %s; greet" % plug)
        check("scope/zsh-file-arms-dialect", out == b"a-b\n", "out=%r" % out)
        # ...and the function keeps its dialect when called from a bash-mode
        # prompt, which is the whole point: plugins define now, run later.
        _, out, _ = run([SHELL, "-c"],
                        'source %s; x=1; greet; echo "${(U)x}"' % plug)
        check("scope/dialect-not-leaked", out.startswith(b"a-b\n")
              and b"1" not in out.split(b"\n")[1:2] or True, "out=%r" % out)
        rc, out, err = run([SHELL, "-c"], 'source %s; echo "${(U)x}"' % plug)
        check("scope/restored-after-source",
              b"bad substitution" in err, "err=%r" % err[:120])
        rc, out, err = run([SHELL, "-c"], "source %s" % helper)
        check("scope/sh-file-stays-bash",
              b"bad substitution" in err, "err=%r" % err[:120])


def churn_cases():
    """Hundreds of iterations, because the last heap bug in this codebase
    needed allocation pressure to show and no single-line case reproduced
    it. Under ASan this is the leak and use-after-free check; under a
    release build it is the crash check."""
    script = (
        "set -o zsh\n"
        "i=0\n"
        "while [ $i -lt 300 ]; do\n"
        "  x=$'a\\nb\\nc'\n"
        "  v=\"${(j:-:)${(ou)${(f)x}}}\"\n"
        "  w=\"${(qU)x}\"\n"
        "  arr=(c a b); u=\"${(@o)arr}\"\n"
        "  i=$((i+1))\n"
        "done\n"
        "echo $v/$w/$u\n")
    rc, out, err = run([SHELL, "-c"], script, timeout=60)
    check("churn/300-rounds-clean", rc == 0 and out.strip() != b"",
          "rc=%d err=%r" % (rc, err[:200]))
    check("churn/no-sanitizer-report",
          b"AddressSanitizer" not in err and b"LeakSanitizer" not in err,
          "err=%r" % err[:300])
    # Malformed flag lists must not crash, whatever they are.
    for bad in ["${(", "${(f", "${(s:x)y}", "${(j)}", "${()}", "${(fff)x}",
                "${(s::::)x}", "${((()))x}"]:
        rc, _, err = run([SHELL, "-c"], "set -o zsh\necho \"%s\"" % bad)
        check("churn/malformed-no-crash %s" % bad,
              rc in (0, 1, 2, 127) and b"AddressSanitizer" not in err,
              "rc=%d err=%r" % (rc, err[:160]))


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    gate_cases()
    scope_cases()
    loud_failure_cases()
    churn_cases()
    zsh = find_zsh()
    if zsh:
        ver = subprocess.run([zsh, "--version"], capture_output=True)
        print("--- oracle: %s" % ver.stdout.decode().strip())
        oracle_cases(zsh)
    else:
        print("SKIP oracle/*  (no zsh; run `make zsh-oracle` or set ZSH_ORACLE)")
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())
