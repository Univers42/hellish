#!/usr/bin/env python3
"""Whole-file parse must scale linearly (issue #101).

A script that is one large compound command (a monolithic rc whose whole
body is a single if/else, like hellishrc_plugins' theme file) parsed in
O(n^2)+: the batch reader only batched a cycle's FIRST delivery, so once
the compound tripped one hazard byte (the word "alias" in a comment was
enough) every remaining line was delivered alone and the accumulated
construct was re-lexed, re-parsed and re-freed per line. 2000 lines took
seconds; bash and dash do the same file in ~0ms. `source FILE` was always
fine -- it hands the parser the whole buffer at once -- which is exactly
the hidden-superlinearity trap this test pins.

Guards, machine-independent (shape, not absolute speed):
  1. hazard-compound file, `-n`:  t(4N) must be < RATIO_MAX * t(N),
     or just plain fast in absolute terms (fast-pass short-circuit).
  2. same file piped through stdin (the INP_NOTTY 8K-block path).
  3. a themes-like mix (case/cmdsub bodies + hazard comments sprinkled).
  4. a 20000-statement linear file stays under a generous absolute cap
     (catches a catastrophic regression of batching itself).

Usage: python3 parse_scaling_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []

N_SMALL = 600
N_BIG = 2400
RATIO_MAX = 9.0     # linear ~4, quadratic ~16 for a 4x size step
FAST_PASS = 1.5     # a t(4N) this small cannot be the quadratic path
ABS_CAP = 120.0     # even the small run must finish inside this


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name
          + ("  " + detail if not ok else ""))
    if not ok:
        FAILS.append(name)


def hazard_compound(n):
    """One giant if-compound with a single hazard byte near the top --
    the minimal shape that forced per-line continuation delivery."""
    body = "".join("  echo line %d > /dev/null\n" % i for i in range(n))
    return ("if true; then\n"
            "  # keep the aliases below in sync\n" + body + "fi\n")


def themes_like(n):
    """The realistic shape: one monolithic if/else whose body mixes case
    dispatch, command substitution and hazard words in comments."""
    out = ["if true; then\n"]
    for i in range(n // 4):
        if i % 20 == 0:
            out.append("  # alias table, sourced by the loader\n")
        out.append('  case "$%d" in a*) x=A ;; *) x=B ;; esac\n' % (i % 10))
        out.append("  v=$(printf %%s hi)\n")
        out.append('  p="${PWD##*/}:${HOME:-none}"\n')
        out.append("  n=$((n + %d))\n" % i)
    out.append("fi\n")
    return "".join(out)


def word_heavy(n):
    """Many short functions, dense with words: every position of every word
    asked whether a zsh alternation group starts there, and the question
    first measured the rest of the input with strlen -- quadratic in the
    size of the file, however it arrived (#135). 6000 such lines took
    630ms to parse where bash takes 32ms, and 16000 took seconds."""
    return "".join('f%d() { local a=$1 b="$2"; echo "$a" x y z '
                   '| grep -q q && printf "%%s\\n" "$b"; }\n' % i
                   for i in range(n))


def timed(args, stdin_path=None, timeout=300):
    best = None
    for _ in range(2):
        t0 = time.monotonic()
        with open(stdin_path) if stdin_path else open(os.devnull) as f:
            subprocess.run(args, stdin=f, capture_output=True,
                           timeout=timeout)
        dt = time.monotonic() - t0
        if best is None or dt < best:
            best = dt
    return best


def write_tmp(text):
    f = tempfile.NamedTemporaryFile("w", suffix=".sh", delete=False)
    f.write(text)
    f.close()
    return f.name


def scaling_check(name, gen, via_stdin=False, via_source=False):
    small = write_tmp(gen(N_SMALL))
    big = write_tmp(gen(N_BIG))
    try:
        if via_source:
            t1 = timed([SHELL, "-c", ". " + small])
            t4 = timed([SHELL, "-c", ". " + big])
        elif via_stdin:
            t1 = timed([SHELL, "-n"], stdin_path=small)
            t4 = timed([SHELL, "-n"], stdin_path=big)
        else:
            t1 = timed([SHELL, "-n", small])
            t4 = timed([SHELL, "-n", big])
    except subprocess.TimeoutExpired:
        check(name, False, "timed out -- parse is grossly superlinear")
        return
    finally:
        os.unlink(small)
        os.unlink(big)
    detail = "t(%d)=%.2fs t(%d)=%.2fs ratio=%.1f" % (
        N_SMALL, t1, N_BIG, t4, t4 / max(t1, 0.005))
    print("     " + name + ": " + detail)
    if t4 <= FAST_PASS:
        check(name, True)
        return
    check(name, t1 <= ABS_CAP and t4 / max(t1, 0.02) < RATIO_MAX, detail)


WORD_N = 2000
WORD_RATIO_MAX = 20.0   # an 8x size step: linear ~8, quadratic ~64


def word_scaling_check(name, argv):
    """t(8N) against t(N), no fast pass. The step is 8x, not 4x: under
    ASan the linear part is heavy enough that a 4x step put the quadratic
    at a ratio of ~10 -- too close to linear to call on a busy runner."""
    small = write_tmp(word_heavy(WORD_N))
    big = write_tmp(word_heavy(WORD_N * 8))
    try:
        run = (lambda p: argv[:-1] + [argv[-1] + p]) if argv[-1] == ". " \
            else (lambda p: argv + [p])
        t1 = timed(run(small))
        t4 = timed(run(big))
    except subprocess.TimeoutExpired:
        check(name, False, "timed out -- parse is grossly superlinear")
        return
    finally:
        os.unlink(small)
        os.unlink(big)
    detail = "t(%d)=%.3fs t(%d)=%.3fs ratio=%.1f" % (
        WORD_N, t1, WORD_N * 8, t4, t4 / max(t1, 0.005))
    print("     " + name + ": " + detail)
    check(name, t4 / max(t1, 0.01) < WORD_RATIO_MAX, detail)


def main():
    scaling_check("hazard compound via -n FILE", hazard_compound)
    scaling_check("hazard compound via piped stdin", hazard_compound,
                  via_stdin=True)
    scaling_check("themes-like monolith via -n FILE", themes_like)
    scaling_check("themes-like monolith via source (v2.8.6: interior "
                  "hazards must not re-parse the open construct)",
                  themes_like, via_source=True)

    # Word-heavy text is cheap per line, so the quadratic stayed under the
    # fast pass at the sizes above: this one compares 2000 and 16000 lines
    # outright, both ways the text can arrive.
    word_scaling_check("word-heavy file via -n FILE", [SHELL, "-n"])
    word_scaling_check("word-heavy file via source", [SHELL, "-c", ". "])

    # Re-sourcing an rc that installed aliases, functions and a DEBUG
    # trap must stay as cheap as the first load: hellish once disabled
    # the forkless $() path on the first alias and fired the DEBUG trap
    # per sourced statement, so every re-source of a theme rc forked
    # hundreds of times (issue #108, wave 3 -- "source got slow again").
    rc = write_tmp("trap 'preexec_probe=$((${preexec_probe:-0}+1))' DEBUG\n"
                   "alias _rp='echo rp'\n"
                   + "".join("rf_%d(){ v=$(printf %d); }\n" % (i, i)
                             for i in range(60))
                   + "".join("x%d=$(printf a%d)\n" % (i, i)
                             for i in range(120)))
    try:
        t1 = timed([SHELL, "-c", ". " + rc])
        t5 = timed([SHELL, "-c", ". %s; . %s; . %s; . %s; . %s"
                    % (rc, rc, rc, rc, rc)])
        detail = "t1=%.2fs t5=%.2fs ratio=%.1f" % (t1, t5, t5 / max(t1, 0.005))
        print("     re-source with aliases+DEBUG trap: " + detail)
        check("re-source stays as cheap as first load",
              t5 <= 1.0 or t5 / max(t1, 0.02) < 9.0, detail)
    finally:
        os.unlink(rc)

    linear = write_tmp("".join("echo l%d > /dev/null\n" % i
                               for i in range(20000)))
    try:
        t = timed([SHELL, "-n", linear], timeout=120)
        check("20000-statement linear file under 60s", t < 60.0,
              "took %.2fs" % t)
    except subprocess.TimeoutExpired:
        check("20000-statement linear file under 60s", False, "timed out")
    finally:
        os.unlink(linear)

    if FAILS:
        print("\n%d FAILED: %s" % (len(FAILS), ", ".join(FAILS)))
        sys.exit(1)
    print("\nall clear")


main()
