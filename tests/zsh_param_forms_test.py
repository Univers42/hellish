#!/usr/bin/env python3
"""The zsh parameter forms oh-my-zsh's colored-man-pages needs, and the
plugin-standard preamble they exist for.

Every zsh plugin that wants to know where it lives opens with this:

    0="${${ZERO:-${0:#$ZSH_ARGZERO}}:-${(%):-%N}}"
    0="${${(M)0:#/*}:-$PWD/$0}"
    typeset -g my_dir="${0:A:h}"

Four separate features in two lines -- nested expansions as operands, the
`:#` filter, the (M) flag that inverts it, and the `:h`/`:A` modifiers --
plus a `$0` that means something different from bash's. Getting any one of
them subtly wrong produces a PATH, which is the worst possible failure
shape: it looks like a directory, and the plugin loads things from it.

DIFFED AGAINST A REAL ZSH. Four of these had a plausible wrong answer that
a hand-written assertion would have accepted:

    ${x:#pat}      is a FILTER, not a trim -- "foo" with `f*` is empty,
                   not "oo". `${x#f*}` (one colon fewer) IS the trim.
    "${a:#f*}"     joins the array BEFORE filtering, so quoted and unquoted
                   give different answers to the same expression.
    ${p:r}         on "/a/.hidden" is "/a/", not "/a/.hidden" -- a leading
                   dot counts as an extension separator, which is neither
                   what basename does nor what "hidden file" suggests.
    $0             inside a SOURCED FILE is the file, not the shell. bash
                   keeps the shell name; getting this one wrong pointed
                   every plugin's self-locating code at build/bin.

Usage: python3 zsh_param_forms_test.py [/path/to/hellish]
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

# Run from a FILE, not `-c`: the whole -c string is lexed before `set -o zsh`
# executes, so the dialect would not be armed in time for anything the LEXER
# has to decide (`0=`, `$+name[k]`). That is not a limitation to work
# around -- a plugin's dialect is a property of its file.
CASES = [
    # ---- the :# filter, and (M) which inverts it -------------------
    ("hash-drops-a-match", '''x=foo; echo "[${x:#f*}]"'''),
    ("hash-keeps-a-non-match", '''x=foo; echo "[${x:#z*}]"'''),
    ("M-keeps-only-the-match", '''x=foo; echo "[${(M)x:#f*}]"'''),
    ("M-empty-on-no-match", '''x=bar; echo "[${(M)x:#f*}]"'''),
    ("hash-filters-array-unquoted",
     '''a=(foo bar fig); printf "<%s>" ${a:#f*}; echo'''),
    ("M-filters-array-unquoted",
     '''a=(foo bar fig); printf "<%s>" ${(M)a:#f*}; echo'''),
    ("hash-quoted-joins-first", '''a=(foo bar fig); echo "[${a:#f*}]"'''),
    ("M-quoted-joins-first", '''a=(foo bar fig); echo "[${(M)a:#f*}]"'''),
    ("hash-empty-pattern", '''x=foo; echo "[${x:#}]"'''),
    ("hash-on-unset", '''unset q; echo "[${q:#f*}]"'''),
    # ---- modifiers, and they chain ---------------------------------
    ("mod-h", '''p=/a/b/c.txt; echo "[${p:h}]"'''),
    ("mod-t", '''p=/a/b/c.txt; echo "[${p:t}]"'''),
    ("mod-r", '''p=/a/b/c.txt; echo "[${p:r}]"'''),
    ("mod-e", '''p=/a/b/c.txt; echo "[${p:e}]"'''),
    ("mod-chain-t-r", '''p=/a/b/c.txt; echo "[${p:t:r}]"'''),
    ("mod-h-no-slash", '''p=noslash; echo "[${p:h}]"'''),
    ("mod-h-trailing-slash", '''p=/a/b/; echo "[${p:h}]"'''),
    ("mod-leading-dot-is-an-ext",
     '''p=/a/.hidden; echo "[${p:r}][${p:e}]"'''),
    ("mod-dot-in-a-directory", '''p=/a.b/c; echo "[${p:r}][${p:e}]"'''),
    ("mod-no-extension", '''p=/a/b/c; echo "[${p:r}][${p:e}]"'''),
    ("mod-l", '''p=ABC; echo "[${p:l}]"'''),
    ("mod-u", '''p=abc; echo "[${p:u}]"'''),
    ("mod-on-unset", '''unset q; echo "[${q:h}]"'''),
    # ---- nested expansions as operands -----------------------------
    ("nest-plain", '''A=1; echo "[${${A}:-b}]"'''),
    ("nest-unset-takes-default", '''unset B; echo "[${${B}:-fb}]"'''),
    ("nest-then-modifier", '''A=/x/y; echo "[${${A}:t}]"'''),
    ("nest-then-trim", '''A=hello; echo "[${${A}#he}]"'''),
    ("nest-twice", '''A=/a/b/c; echo "[${${${A}:h}:t}]"'''),
    ("nest-with-flags", '''x=abc; echo "[${${(U)x}}]"'''),
    ("nest-flags-then-default",
     '''x=/a/b; echo "[${${(M)x:#/*}:-FALLBACK}]"'''),
    ("nest-flags-default-taken",
     '''x=rel; echo "[${${(M)x:#/*}:-FALLBACK}]"'''),
    ("nest-default-references-a-var",
     '''unset Z; A=fb; echo "[${${Z}:-$A}]"'''),
    # ---- the empty-name default, which is how (%) is applied -------
    ("empty-name-default", '''echo "[${:-literal}]"'''),
    ("flags-on-empty-name-default", '''echo "[${(U):-abc}]"'''),
    # ---- $0 -------------------------------------------------------
    ("zero-is-the-sourced-file", '''echo "[$0]"'''),
    ("zero-assignable", '''0=myname; echo "[$0]"'''),
    ("zero-in-a-function", '''f() { echo "[$0]"; }; f'''),
    ("zero-multi-name-function",
     '''function p q { echo "[$0]"; }; p; q'''),
    # ---- the plugin-standard preamble, end to end ------------------
    ("plugin-standard-preamble", '''0="${${ZERO:-${0:#$ZSH_ARGZERO}}:-${(%):-%N}}"
0="${${(M)0:#/*}:-$PWD/$0}"
echo "[${0:t}]"'''),
]

# $+commands, asserted rather than compared.
#
# The oracle cannot judge these: `zsh -f` runs without the zsh/parameter
# module, so `commands` does not exist there and real zsh answers 0 for
# every one of them -- the same reason ${+terminfo[...]} is 0 in both.
# hellish answers from its own PATH lookup, which is MORE useful and
# therefore not comparable, so the expectations are written out.
#
# The literal form always worked. The variable form asked after a program
# literally named "$m" and answered 0 -- indistinguishable from a program
# that is genuinely missing, which is the whole problem. The loop is the
# idiom every plugin writes it in, and it is why jsontools found no
# interpreter on a machine with three of them installed.
COMMANDS_CASES = [
    ("literal", '''echo "[${+commands[sh]}]"''', "[1]"),
    ("via-variable", '''m=sh; echo "[${+commands[$m]}]"''', "[1]"),
    ("via-braced-variable", '''m=sh; echo "[${+commands[${m}]}]"''', "[1]"),
    ("via-substitution", '''echo "[${+commands[$(echo sh)]}]"''', "[1]"),
    ("missing-literal", '''echo "[${+commands[no_such_xyzzy]}]"''', "[0]"),
    ("missing-via-variable",
     '''m=no_such_xyzzy; echo "[${+commands[$m]}]"''', "[0]"),
    ("loop-idiom", '''for m in no_such_xyzzy sh; do
  (( $+commands[$m] )) && break
  unset m
done
echo "[$m]"''', "[sh]"),
]

# The bash dialect must be unmoved by any of this. Compared against bash
# itself rather than against a hardcoded string, so the check is "hellish
# still agrees with bash here" and not "hellish still produces what I typed".
BASH_UNCHANGED = [
    'p=/a/b; echo "[${p:h}]"',
    '0=x',
    'A=1; echo "[${${A}:-b}]"',
    'x=foo; echo "[${(M)x:#f*}]"',
]

# A PRE-EXISTING bash-parity gap, verified against the previous commit's
# binary before any of this work: `${x:#f*}` in the BASH dialect is read as
# the substring form `${x:offset}`, the offset fails to parse as arithmetic,
# and hellish falls back to offset 0 and returns the whole value. bash
# errors ("operand expected"). Recorded here rather than fixed on this
# branch -- changing arith-error handling inside substrings reaches the
# golden suite, which is a separate change with a separate blast radius.
BASH_KNOWN_GAP = 'x=foo; echo "[${x:#f*}]"'



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


def run(argv, timeout=20):
    try:
        p = subprocess.run(argv, capture_output=True, timeout=timeout,
                           env=dict(os.environ, HELLISH_NO_BANNER="1",
                                    HELLISH_NO_UPDATE_CHECK="1",
                                    HELLISH_NO_ANIM="1"))
        return p.returncode, p.stdout, p.stderr
    except (subprocess.TimeoutExpired, OSError) as e:
        return -1, b"<" + str(e).encode() + b">", b""


def oracle_cases(zsh, d):
    """Both shells run the SAME file, so $0 and %N name the same path and a
    case about them compares like with like."""
    path = os.path.join(d, "case.zsh")
    for name, script in CASES:
        with open(path, "w") as fh:
            fh.write(script + "\n")
        _, zout, _ = run([zsh, "-f", path])
        _, hout, herr = run([SHELL, "-c", "source %s" % path])
        check("param/" + name, hout == zout,
              "zsh=%r hellish=%r %r" % (zout, hout, herr[:100]))


def commands_cases(d):
    """$+commands / ${commands[x]}, including through a variable."""
    path = os.path.join(d, "case.zsh")
    for name, script, want in COMMANDS_CASES:
        with open(path, "w") as fh:
            fh.write(script + "\n")
        _, out, err = run([SHELL, "-c", "source %s" % path])
        check("commands/" + name, out.decode().strip() == want,
              "want %r got %r %r" % (want, out, err[:100]))
    # The VALUE form, which must be a real path to the real program.
    with open(path, "w") as fh:
        fh.write('m=sh; echo "${commands[$m]}"\n')
    _, out, _ = run([SHELL, "-c", "source %s" % path])
    got = out.decode().strip()
    check("commands/value-via-variable-is-a-path",
          got.endswith("/sh") and os.path.exists(got), "got %r" % got)


def gate_cases(d):
    """The bash dialect must be untouched by any of it."""
    sh = os.path.join(d, "case.sh")
    for script in BASH_UNCHANGED:
        with open(sh, "w") as fh:
            fh.write(script + "\n")
        _, out, _ = run([SHELL, "-c", "source %s" % sh])
        _, bout, _ = run(["bash", sh])
        check("gate/matches-bash: %s" % script[:30], out == bout,
              "hellish=%r bash=%r" % (out, bout))
    # The recorded gap. Pinned at the CURRENT behaviour, so the day the
    # arithmetic-error path is fixed this fails and says which case changed
    # rather than the fix silently doing nothing.
    with open(sh, "w") as fh:
        fh.write(BASH_KNOWN_GAP + "\n")
    _, out, _ = run([SHELL, "-c", "source %s" % sh])
    _, bout, berr = run(["bash", sh])
    check("gate/known-gap-bash-still-errors", b"operand expected" in berr,
          "bash no longer errors -- re-check the note; err=%r" % berr[:120])
    check("gate/known-gap-hellish-still-lenient", out.strip() == b"[foo]",
          "hellish changed: %r -- if this is the fix, remove the note" % out)


def plugin_cases(d):
    """colored-man-pages itself: it must define exactly the four functions
    zsh defines -- no more, which is the check that catches a continuation
    backslash being collected as a fifth name."""
    cache = os.path.join(os.environ.get(
        "XDG_CACHE_HOME", os.path.expanduser("~/.cache")),
        "hellish-plugin-corpus", "omz-colored-man.zsh")
    if not os.path.exists(cache):
        print("skip plugin/*  (corpus not cached; run plugin_corpus_test.py)")
        return
    rc, out, err = run([SHELL, "-c", "source %s" % cache])
    check("plugin/loads-silently", rc == 0 and err.strip() == b"",
          "rc=%d err=%r" % (rc, err[:160]))
    rc, out, _ = run([SHELL, "-c",
                      "source %s; declare -F | sort" % cache])
    got = sorted(l.split()[-1] for l in out.decode().split("\n") if l.strip())
    check("plugin/defines-exactly-four",
          got == ["colored", "debman", "dman", "man"], "got %r" % got)
    # The three man wrappers share ONE body and tell themselves apart by $0.
    # If $0 were the shell, all three would colour `man`.
    rc, out, _ = run([SHELL, "-c",
                      "source %s; type man dman debman" % cache])
    check("plugin/all-three-are-functions",
          out.count(b"is a function") == 3, "out=%r" % out[:160])
    # ...and the directory it computes must be the plugin's, not the shell's.
    rc, out, _ = run([SHELL, "-c",
                      'source %s; echo "$__colored_man_pages_dir"' % cache])
    check("plugin/locates-its-own-directory",
          out.strip() == os.path.dirname(cache).encode(),
          "got %r want %r" % (out.strip(), os.path.dirname(cache)))


def churn_cases(d):
    path = os.path.join(d, "churn.zsh")
    with open(path, "w") as fh:
        fh.write("i=0\n"
                 "while [ $i -lt 300 ]; do\n"
                 "  p=/a/b/c.txt; v=\"${${p:h}:t}\"\n"
                 "  x=foo; w=\"${(M)x:#f*}\"\n"
                 "  a=(foo bar); u=\"${a:#f*}\"\n"
                 "  0=n$i\n"
                 "  i=$((i+1))\n"
                 "done\necho \"$v/$w/$u\"\n")
    rc, out, err = run([SHELL, "-c", "source %s" % path], timeout=90)
    check("churn/300-rounds-clean", rc == 0 and out.strip() != b"",
          "rc=%d err=%r" % (rc, err[:200]))
    check("churn/no-sanitizer-report",
          b"AddressSanitizer" not in err and b"LeakSanitizer" not in err,
          "err=%r" % err[:300])
    for bad in ["${${", "${${A}", "${x:#", "${(M)x:#", "${x:", "${x:zz}",
                "${${${${A}}}}", "0="]:
        p2 = os.path.join(d, "bad.zsh")
        with open(p2, "w") as fh:
            fh.write('echo "%s"\n' % bad)
        rc, _, err = run([SHELL, "-c", "source %s" % p2])
        check("churn/no-crash: %s" % bad[:18],
              b"AddressSanitizer" not in err and rc in (0, 1, 2, 127),
              "rc=%d err=%r" % (rc, err[:120]))


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    with tempfile.TemporaryDirectory() as d:
        gate_cases(d)
        commands_cases(d)
        plugin_cases(d)
        churn_cases(d)
        zsh = find_zsh()
        if zsh:
            print("--- oracle: %s" % subprocess.run(
                [zsh, "--version"],
                capture_output=True).stdout.decode().strip())
            oracle_cases(zsh, d)
        else:
            print("SKIP param/*  (no zsh; run `make zsh-oracle`)")
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())
