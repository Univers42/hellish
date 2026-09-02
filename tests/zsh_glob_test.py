#!/usr/bin/env python3
"""zsh glob qualifiers and `=(cmd)`, and the dotglob bug they uncovered.

oh-my-zsh's extract needs both:

    content=("${extract_dir}"/*(DNY2))     at most 2 entries, dotfiles too
    local tmp_name==(:)                    a fresh temp FILE, not a pipe

DIFFED AGAINST A REAL ZSH, in a fixture directory of known contents, because
each qualifier has a plausible wrong answer:

    (.)   is lstat, not stat -- a symlink to a regular file is a SYMLINK and
          is excluded, or else (.) and (@) would both be true for it and the
          set would not partition.
    (D)   is not a filter. Whether a dotfile is offered AT ALL is the walk's
          decision, so no post-pass can add one back.
    (Y2)  caps the count. WHICH two is readdir order, a property of the
          filesystem -- so the count is asserted and the names are not.

RECORDED DIVERGENCE: zsh's `nomatch` (on by default) makes a glob with no
matches an ERROR. hellish keeps bash's literal fallback, because adopting
nomatch would change every no-match glob in the shell and not just the ones
carrying a qualifier. Checked against bash, so it stays one-directional.

THE BUG UNDERNEATH (D). `shopt -s dotglob` was mirrored from state->shopt
into a cell by the shopt builtin and by `pretty`, and then NOTHING read it:
dotfiles stayed hidden. Two matchers had to be fixed, not one -- glob_match.c
answers "does this name match" for `case` and `[[ = ]]`, pattern_matcher*.c
drives the directory walk. Fixing one and not the other fixes nothing that
matters, which is exactly what happened on the first attempt.

=(cmd) IS NOT <(cmd). Both substitute a path; `<(cmd)` gives a pipe, which
reads once, front to back, and cannot be seeked or reopened. `=(cmd)` gives
a real file, which can be all of those. A consumer that reads twice works
with one and fails with the other, so implementing =() as <() would produce
a path that works right up until it does not.

Usage: python3 zsh_glob_test.py [/path/to/hellish]
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


def oracle_bash():
    """The bash the golden suite is defined against (5.3.9, built by `make
    oracle`), when it is there; the box's own bash otherwise. `echo .*`
    is where the two disagree: 5.2 turned globskipdots on, so a 22.04 box
    with bash 5.1 answers `. .. .hidden` and blames hellish for agreeing
    with 5.3."""
    env = os.environ.get("HELLISH_ORACLE")
    if env and os.path.exists(env):
        return env
    home = os.path.expanduser("~/bash-5.3.9/bin/bash")
    return home if os.path.exists(home) else "bash"

# Diffed against zsh, run from the fixture directory so both shells see the
# same entries. A `.hidden`, a symlink, a directory and three plain files.
QUAL_CASES = [
    ("plain-N", '''echo *(N)'''),
    ("D-includes-dotfiles", '''echo *(DN)'''),
    ("dot-is-plain-files-only", '''echo *(N.)'''),
    ("slash-is-directories", '''echo *(N/)'''),
    ("at-is-symlinks", '''echo *(N@)'''),
    ("D-and-types-compose", '''echo *(DN.)'''),
    ("no-match-with-N-is-empty", '''echo nomatch*(N)'''),
    ("qualifier-on-a-path", '''echo ./*(N.)'''),
    ("not-a-qualifier-stays-literal", '''echo *(zz)'''),
    ("bare-parens-are-not-a-qualifier", '''echo foo(bar) 2>&1 | head -1'''),
]

# `(Y<n>)` caps the count; WHICH entries is readdir order, which belongs to
# the filesystem and not to the shell. The count is the contract.
LIMIT_CASES = [("Y1", 1), ("Y2", 2), ("Y3", 3)]

# The bash dialect must be unmoved. Compared against bash itself.
BASH_SAME = [
    'echo *',
    'shopt -s dotglob; echo *',
    'shopt -s dotglob; echo a*',
    'echo .*',
    'echo *(N)',
    'cat <(echo hi)',
    'echo x=y',
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


def run(argv, cwd=None, timeout=25):
    try:
        p = subprocess.run(argv, capture_output=True, timeout=timeout, cwd=cwd,
                           env=dict(os.environ, HELLISH_NO_BANNER="1",
                                    HELLISH_NO_UPDATE_CHECK="1",
                                    HELLISH_NO_ANIM="1"))
        return p.returncode, p.stdout, p.stderr
    except (subprocess.TimeoutExpired, OSError) as e:
        return -1, b"<" + str(e).encode() + b">", b""


def fixture(d):
    """A directory whose contents make every qualifier distinguishable."""
    for n in ("a.txt", "b.txt", "c.dat"):
        open(os.path.join(d, n), "w").close()
    open(os.path.join(d, ".hidden"), "w").close()
    os.mkdir(os.path.join(d, "sub"))
    os.symlink("a.txt", os.path.join(d, "link"))


def qual_cases(zsh, d):
    path = os.path.join(d, "case.zsh")
    for name, script in QUAL_CASES:
        with open(path, "w") as fh:
            fh.write(script + "\n")
        _, zout, _ = run([zsh, "-f", path], cwd=d)
        _, hout, herr = run([SHELL, "-c", "source %s" % path], cwd=d)
        check("qual/" + name, hout == zout,
              "zsh=%r hellish=%r %r" % (zout, hout, herr[:100]))


def limit_cases(d):
    """(Y<n>) caps the count. Asserted as a COUNT, not as which entries --
    zsh takes them in readdir order, which is the filesystem's business."""
    path = os.path.join(d, "case.zsh")
    for q, want in LIMIT_CASES:
        with open(path, "w") as fh:
            fh.write("echo *(DN%s) | wc -w\n" % q)
        _, out, _ = run([SHELL, "-c", "source %s" % path], cwd=d)
        try:
            got = int(out.decode().strip())
        except ValueError:
            got = -1
        check("limit/%s-caps-at-%d" % (q, want), got == want, "got %d" % got)
    # ...and a limit above the match count is not a floor.
    with open(path, "w") as fh:
        fh.write("echo *(DNY99) | wc -w\n")
    _, out, _ = run([SHELL, "-c", "source %s" % path], cwd=d)
    check("limit/Y99-does-not-invent", out.decode().strip() == "7",
          "got %r (fixture has 7 entries)" % out)


def dotglob_cases(d):
    """shopt -s dotglob, which was mirrored into a cell nothing read."""
    for script in ('shopt -s dotglob; echo *', 'echo *',
                   'shopt -s dotglob; echo a*', 'echo .*',
                   'shopt -s dotglob; case .x in *) echo m ;; esac',
                   'case .x in *) echo m ;; esac'):
        _, hout, _ = run([SHELL, "-c", script], cwd=d)
        _, bout, _ = run([oracle_bash(), "-c", script], cwd=d)
        check("dotglob/%s" % script[:34], hout == bout,
              "hellish=%r bash=%r" % (hout, bout))


def eqsub_cases(d):
    """=(cmd) gives a real file, in every position <(cmd) works."""
    path = os.path.join(d, "case.zsh")
    for name, script, want in [
            ("reads-back", 'cat =(echo hi)', b"hi\n"),
            ("empty-command", 'cat =(:)', b""),
            ("as-an-argument", 'wc -l < =(printf "a\\nb\\n")', b"2\n"),
            ("is-a-real-file", 'f=$(echo =(echo x)); [ -f "$f" ] '
             '&& echo yes || echo no', b"yes\n"),
            ("is-seekable", 'f=$(echo =(echo abc)); cat "$f"; cat "$f"',
             b"abc\nabc\n")]:
        with open(path, "w") as fh:
            fh.write(script + "\n")
        rc, out, err = run([SHELL, "-c", "source %s" % path], cwd=d)
        check("eqsub/" + name, out == want,
              "got %r want %r err=%r" % (out, want, err[:120]))
    # Two of them must not collide.
    with open(path, "w") as fh:
        fh.write('a=$(echo =(:)); b=$(echo =(:)); '
                 '[ "$a" != "$b" ] && echo unique || echo collision\n')
    _, out, _ = run([SHELL, "-c", "source %s" % path], cwd=d)
    check("eqsub/paths-are-unique", out.strip() == b"unique", "got %r" % out)
    # ...and the file is gone once the session ends.
    with open(path, "w") as fh:
        fh.write('echo =(:)\n')
    _, out, _ = run([SHELL, "-c", "source %s" % path], cwd=d)
    leaked = out.decode().strip()
    check("eqsub/cleans-up-after-itself",
          not leaked or not os.path.exists(leaked),
          "left %r behind" % leaked)


def nomatch_divergence(d):
    """zsh's `nomatch` option, ON by default, makes a glob with no matches an
    ERROR ("no matches found"). bash leaves the pattern literal, and hellish
    is a bash-dialect shell wearing a zsh hat -- turning nomatch on for
    anything in zsh mode would change what every no-match glob does, which
    reaches far beyond the qualifiers.

    So: a qualifier without (N) keeps bash's literal fallback. Recorded, and
    checked against bash rather than zsh, so the divergence is one-directional
    and visible."""
    path = os.path.join(d, "case.zsh")
    with open(path, "w") as fh:
        fh.write("echo nomatch*(.)\n")
    _, hout, _ = run([SHELL, "-c", "source %s" % path], cwd=d)
    check("nomatch/qualifier-no-match-is-literal",
          hout.strip() == b"nomatch*(.)",
          "got %r -- if this is now an error, zsh nomatch was adopted "
          "and the note needs updating" % hout)
    _, bout, _ = run([oracle_bash(), "-c", "echo nomatch*"], cwd=d)
    check("nomatch/bash-is-literal-too", bout.strip() == b"nomatch*",
          "bash changed: %r" % bout)


def known_gap_cases(d):
    """A proc-sub in ASSIGNMENT position attaches to the assignment (#83).

           f=<(echo hi)     bash: f=/dev/fd/63
                            hellish, before: f= , then it RAN /dev/fd/3

    This was recorded here as a KNOWN GAP, at the broken behaviour, so that
    the day it was fixed this file would fail and name it.  It did exactly
    that, and the expectation is now the fixed one.  Pinning a gap only
    works if flipping it is a failure rather than a quiet pass.

    The note that came with the gap said an earlier attempt had been
    reverted -- gluing the path onto the previous argv element aborted with
    `munmap_chunk(): invalid pointer`, because argv holds a MIX of word-slab
    and xmalloc pointers.  That is why the fix frees through word_free(),
    which routes each kind to its own allocator, and why it is exercised on
    the ft_malloc build and not only under ASan.

    The full behaviour lives in tests/procsub_assign (bash `<(cmd)`, diffed
    against bash) and tests/zsh_procsub_test.py (zsh `=(cmd)`); these two
    rows stay here because this is the file that recorded the gap."""
    rc, out, err = run([SHELL, "-c", 'f=<(echo hi); echo "[$f]"'], cwd=d)
    check("procsub-in-assignment/attaches-now",
          b"/dev/fd/" in out and b"No such file" not in err,
          "regressed to the #83 behaviour -- out=%r err=%r" % (out, err[:120]))
    _, bout, _ = run([oracle_bash(), "-c", 'f=<(echo hi); echo "[$f]"'], cwd=d)
    check("procsub-in-assignment/bash-agrees", b"/dev/fd/" in bout,
          "bash changed: %r" % bout)


def gate_cases(d):
    for script in BASH_SAME:
        _, hout, _ = run([SHELL, "-c", script], cwd=d)
        _, bout, _ = run([oracle_bash(), "-c", script], cwd=d)
        check("gate/matches-bash: %s" % script[:30], hout == bout,
              "hellish=%r bash=%r" % (hout, bout))
    # `=(` is only an operator in the dialect; in bash it is a syntax error,
    # and it must STAY one.
    rc, _, err = run([SHELL, "-c", "cat =(echo hi)"], cwd=d)
    _, _, berr = run([oracle_bash(), "-c", "cat =(echo hi)"], cwd=d)
    check("gate/eqsub-is-a-syntax-error-in-bash",
          b"syntax error" in err and b"syntax error" in berr,
          "hellish=%r bash=%r" % (err[:90], berr[:90]))


def churn_cases(d):
    path = os.path.join(d, "churn.zsh")
    with open(path, "w") as fh:
        fh.write("i=0\nwhile [ $i -lt 200 ]; do\n"
                 "  n=$(echo *(DNY2) | wc -w)\n"
                 "  m=$(echo *(N.) | wc -w)\n"
                 "  i=$((i+1))\ndone\necho \"$n/$m\"\n")
    rc, out, err = run([SHELL, "-c", "source %s" % path], cwd=d, timeout=120)
    check("churn/200-rounds-clean", rc == 0 and out.strip() != b"",
          "rc=%d err=%r" % (rc, err[:200]))
    check("churn/no-sanitizer-report",
          b"AddressSanitizer" not in err and b"LeakSanitizer" not in err,
          "err=%r" % err[:300])
    for bad in ["*(", "*(N", "*()", "*(YY)", "*(Y)", "=(", "=()",
                "*(DNY999999999999)"]:
        p2 = os.path.join(d, "bad.zsh")
        with open(p2, "w") as fh:
            fh.write("echo %s\n" % bad)
        rc, _, err = run([SHELL, "-c", "source %s" % p2], cwd=d)
        check("churn/no-crash: %s" % bad[:20],
              b"AddressSanitizer" not in err
              and b"munmap" not in err and rc in (0, 1, 2, 127),
              "rc=%d err=%r" % (rc, err[:120]))


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    with tempfile.TemporaryDirectory() as d:
        fixture(d)
        gate_cases(d)
        dotglob_cases(d)
        limit_cases(d)
        eqsub_cases(d)
        nomatch_divergence(d)
        known_gap_cases(d)
        churn_cases(d)
        zsh = find_zsh()
        if zsh:
            print("--- oracle: %s" % subprocess.run(
                [zsh, "--version"],
                capture_output=True).stdout.decode().strip())
            qual_cases(zsh, d)
        else:
            print("SKIP qual/*  (no zsh; run `make zsh-oracle`)")
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())
