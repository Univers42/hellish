#!/usr/bin/env python3
"""zsh array semantics: the counting base, $#name, the element splice, shift.

WHY THIS EXISTS AS ITS OWN SUITE. Every case here is a WRONG ANSWER risk
rather than a missing-feature risk, and the two need different tests. A
missing feature says "bad substitution" and stops; a wrong counting base
returns a DIFFERENT ELEMENT and reports nothing. `${a[$#a]}` -- "the last
element", which is how every zsh plugin pops a stack -- reads element 3 of a
3-element array under bash's base, finds nothing there, and hands back the
empty string. oh-my-zsh's dirhistory then concludes someone overwrote its
variable and resets its history. Nothing anywhere says so.

So the assertions are two-sided:

    zsh mode   arrays count from 1, $#a is the ELEMENT COUNT, a[i]=(...)
               splices, shift takes an array name
    bash mode  every one of those keeps bash's answer, byte for byte

The bash half is not padding. These changes touch expand_array.c and
assignment_to_env.c, which the 3790 golden cases run through constantly, and
the whole design is that one flag decides which dialect applies. A test that
only checked the zsh side could not tell "gated correctly" from "changed the
default dialect and the golden suite has not noticed yet".

ORACLE. The expected values were taken from zsh 5.9 and bash 5.3.9 rather
than reasoned out; four of them came back the opposite of the intuitive
answer, `a[9]=(x)` padding an array to nine elements among them.

Usage: python3 zsh_arrays_test.py [/path/to/hellish]
"""
import os
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


def run_zsh(script, after=""):
    """Run under the zsh dialect. Sourcing a *.zsh file is how a plugin gets
    the dialect, and it is what the corpus actually exercises -- `set -o zsh`
    inside the same -c string arms the mode AFTER that text was lexed.

    `after` runs in the CALLER once the source returns. That is the only way
    to tell "the file stopped" from "the shell died", which is exactly the
    distinction the bad-subscript cases below are about."""
    with tempfile.NamedTemporaryFile("w", suffix=".zsh", delete=False) as f:
        f.write(script + "\n")
        path = f.name
    try:
        cmd = "source " + path
        if after:
            cmd += "; " + after
        p = subprocess.run([SHELL, "-c", cmd],
                           capture_output=True, timeout=25, env=ENV)
        return (p.stdout + p.stderr).decode().strip(), p.returncode
    finally:
        os.unlink(path)


def run_bash_mode(script):
    p = subprocess.run([SHELL, "-c", script], capture_output=True,
                       timeout=25, env=ENV)
    return (p.stdout + p.stderr).decode().strip(), p.returncode


# (script, expected stdout) -- values from zsh 5.9.
ZSH_CASES = [
    # The counting base, read.
    ('a=(1 2 3); echo "[${a[1]}][${a[2]}][${a[3]}]"', "[1][2][3]"),
    ('a=(1 2 3); echo "[${a[0]}][${a[4]}]"', "[][]"),
    ('a=(1 2 3); echo "[${a[-1]}][${a[-3]}]"', "[3][1]"),
    ('a=(1 2 3); echo "[$a[1]][$a[2]]"', "[1][2]"),
    # A scalar subscript is a CHARACTER in zsh, not the whole value.
    ('x=hello; echo "[${x[1]}][${x[2]}][${x[0]}]"', "[h][e][]"),
    # $#name, braced and bare, in a word and inside arithmetic.
    ('a=(x y z); echo "[$#a][${#a}]"', "[3][3]"),
    ('a=(x y z); echo "[${#a[1]}]"', "[1]"),
    ('set -- p q; echo "[$#][$#a]"', "[2][0]"),
    ('a=(1 2 3); echo $(( $#a + 1 ))', "4"),
    ('a=(1 2 3); echo "${a[$#a]}"', "3"),
    ('typeset -A m; m[k]=v; echo "[$#m][${+m[k]}][${+m[no]}]"', "[1][1][0]"),
    # ${+name[key]} on a table we do not have is 0, which is also what real
    # zsh answers without the module -- not an error, and not a guess.
    ('echo "[${+terminfo[kcub1]}]"', "[0]"),
    # The element splice.
    ('a=(1 2 3); a[2]=(); echo "[${a[@]}] n=$#a"', "[1 3] n=2"),
    ('a=(1 2 3); a[1]=(); echo "[${a[@]}] n=$#a"', "[2 3] n=2"),
    ('a=(1 2 3); a[3]=(); echo "[${a[@]}] n=$#a"', "[1 2] n=2"),
    ('a=(1 2 3); a[2]=(x y); echo "[${a[@]}] n=$#a"', "[1 x y 3] n=4"),
    ('a=(1 2 3); a[-1]=(); echo "[${a[@]}] n=$#a"', "[1 2] n=2"),
    ('a=(1 2 3); a[$#a]=(); echo "[${a[@]}] n=$#a"', "[1 2] n=2"),
    ('a=(1 2 3); a[$((1+1))]=(); echo "[${a[@]}]"', "[1 3]"),
    ('a=(); a[1]=(); echo "[${a[@]}] n=$#a"', "[] n=0"),
    # Assigning past the end pads with empties. Counter-intuitive, and it is
    # what zsh does: nine elements, five of them empty.
    ('a=(1 2 3); a[9]=(x); echo "n=$#a"', "n=9"),
    # A scalar assignment to an element is NOT a splice.
    ('a=(1 2 3); a[2]=9; echo "[${a[@]}]"', "[1 9 3]"),
    # shift on an array.
    ('a=(1 2 3); shift a; echo "[${a[@]}]"', "[2 3]"),
    ('a=(1 2 3); shift 2 a; echo "[${a[@]}]"', "[3]"),
    ('a=(1 2); shift 0 a; echo "[${a[@]}]"', "[1 2]"),
    ('shift nosuch; echo "rc=$?"', "rc=0"),
    ('x=str; shift x; echo "rc=$? [$x]"', "rc=0 [str]"),
    ('set -- p q r; shift; echo "[$*]"', "[q r]"),
    ('set -- p q r; shift 2; echo "[$*]"', "[r]"),
    # ksh_arrays turns the base back to 0 -- the option plugins name in
    # `setopt localoptions no_ksh_arrays` to be sure of the default. If it
    # were accepted and ignored, that reassurance would be a lie.
    ('setopt ksharrays; a=(1 2 3); echo "[${a[0]}][${a[1]}][$#a]"',
     "[1][2][1]"),
    ('setopt ksh_arrays; unsetopt ksh_arrays; a=(1 2); echo "[${a[1]}]"',
     "[1]"),
    # ... slices included: the whole zsh array dialect is what the option
    # switches off, so `a[1,2]` goes back to bash's comma operator.
    ('setopt ksharrays; a=(x y z); echo "[${a[1,2]}]"', "[z]"),
]

A5 = "a=(one two three four five); "

# ${a[lo,hi]} -- the SLICE, and the reason it is worth its own block.
# `lo,hi` is a perfectly good arithmetic expression: the comma operator
# evaluates both sides and yields the right one, so with no slice `a[2,3]`
# reads element 3, ONE element comes back, and nothing anywhere reports a
# problem. A plugin slicing an array gets a single element that looks
# exactly like data.
#
# Every value below came from zsh 5.9 rather than from reasoning. The two
# that look like typos are the interesting ones: `a[0,2]` and `a[-6,2]`
# both name position 0 and they answer differently.
SLICE_CASES = [
    (A5 + 'echo "[${a[2,3]}]"', "[two three]"),
    (A5 + 'echo "[$a[2,3]]"', "[two three]"),
    (A5 + 'echo "[${a[2,-1]}]"', "[two three four five]"),
    (A5 + 'echo "[${a[-2,-1]}]"', "[four five]"),
    (A5 + 'echo "[${a[1,$#a]}]"', "[one two three four five]"),
    (A5 + 'echo "[${a[1+1,2+1]}]"', "[two three]"),
    (A5 + 'i=1; j=3; echo "[${a[i,j]}]"', "[one two three]"),
    # hi < lo is empty, not everything.
    (A5 + 'echo "[${a[3,2]}]"', "[]"),
    # 0 clamps UP to the first element ...
    (A5 + 'echo "[${a[0,2]}]"', "[one two]"),
    # ... but a negative reaching past the start VOIDS the range.
    (A5 + 'echo "[${a[-6,2]}]"', "[]"),
    (A5 + 'echo "[${a[-5,2]}]"', "[one two]"),
    (A5 + 'echo "[${a[-99,2]}]"', "[]"),
    (A5 + 'echo "[${a[1,-5]}]"', "[one]"),
    (A5 + 'echo "[${a[1,-6]}]"', "[]"),
    # Past the end is taken quietly, no complaint.
    (A5 + 'echo "[${a[2,99]}]"', "[two three four five]"),
    # The count is of ELEMENTS, not the width of the joined string.
    (A5 + 'echo "[${#a[2,3]}]"', "[2]"),
    # Quoted joins with IFS[0]; unquoted goes through word splitting, the
    # same route "${a[*]}" already takes.
    (A5 + 'IFS=:; echo "[${a[2,3]}]"', "[two:three]"),
    (A5 + "printf '<%s>' ${a[2,3]}; echo", "<two><three>"),
    # A scalar slices by CHARACTER.
    ('x=hello; echo "[${x[2,3]}][${x[2,-1]}][${#x[2,3]}]"', "[el][ello][2]"),
    ('x=; echo "[${x[1,2]}]"', "[]"),
    # Writing a range: the run is replaced and the array RENUMBERS.
    (A5 + 'a[2,3]=(X Y); echo "[${a[@]}] n=$#a"', "[one X Y four five] n=5"),
    (A5 + 'a[2,3]=(); echo "[${a[@]}] n=$#a"', "[one four five] n=3"),
    (A5 + 'a[2,3]=X; echo "[${a[@]}] n=$#a"', "[one X four five] n=4"),
    (A5 + 'a[2,2]=(P Q); echo "[${a[@]}] n=$#a"',
     "[one P Q three four five] n=6"),
    # An empty range is a POSITION, not an error: this INSERTS.
    (A5 + 'a[3,2]=(P); echo "[${a[@]}] n=$#a"',
     "[one two P three four five] n=6"),
    # Past the end pads, exactly as a plain a[9]= does.
    (A5 + 'a[9,10]=(P); echo "n=$#a"', "n=9"),
    (A5 + 'a[-2,-1]=(P); echo "[${a[@]}] n=$#a"', "[one two three P] n=4"),
    (A5 + 'a[2,99]=(X); echo "[${a[@]}] n=$#a"', "[one X] n=2"),
    ('unset a; a[1,2]=(X Y); echo "[${a[@]}] n=$#a"', "[X Y] n=2"),
    # A comma only separates at the TOP level, which is what leaves an
    # associative key with a comma in it alone.
    ('typeset -A M; M[a,b]=z; echo "[${M[a,b]}]"', "[z]"),
    # A subscript is WORD-EXPANDED before it is evaluated, so $(( )) and
    # $( ) work inside one -- on both sides of a slice's comma too. The read
    # path used to go straight to arithmetic, which resolves $name but
    # neither of those, so `a[$((n+1))]=v` assigned happily while
    # `${a[$((n+1))]}` answered "arithmetic error". tests/array_subscript
    # pins the bash half of the same fix.
    (A5 + 'echo "[${a[$((1+1))]}]"', "[two]"),
    (A5 + 'n=1; echo "[${a[$((n+1))]}]"', "[two]"),
    (A5 + 'echo "[${a[$(echo 2)]}]"', "[two]"),
    (A5 + 'echo "[${a[$((1)),$((3))]}]"', "[one two three]"),
    (A5 + 'echo "[${a[$(echo 2),$(echo 4)]}]"', "[two three four]"),
    (A5 + 'echo "[${#a[$((1+1))]}]"', "[3]"),
    (A5 + 'a[$((1+1))]=(Z); echo "[${a[@]}] n=$#a"',
     "[one Z three four five] n=5"),
    ('x=hello; echo "[${x[$((2))]}]"', "[e]"),
    # ... and a comma INSIDE $(( )) stays the arithmetic comma operator, so
    # this is one index and not a range.
    (A5 + 'echo "[${a[$((1,2))]}]"', "[two]"),
]

# The same constructs in the DEFAULT dialect, against bash's answers.
BASH_CASES = [
    ('a=(1 2 3); echo "[${a[0]}][${a[1]}][${a[2]}]"', "[1][2][3]"),
    ('a=(1 2 3); echo "[${a[-1]}]"', "[3]"),
    ('a=(xxxx yy z); echo "[${#a}]"', "[4]"),
    ('set -- p q r; echo "[$#][$#a]"', "[3][3a]"),
    ('a=(1 2 3); i=1; echo "[${a[i+1]}]"', "[3]"),
    ('a=(1 2 3); a[-1]=x; echo "[${a[@]}]"', "[1 2 x]"),
    ('set -- p q r; shift; echo "[$*]"', "[q r]"),
    ('x=hello; echo "[${x[0]}][${x[1]}]"', "[hello][]"),
]


def zsh_cases():
    for script, want in ZSH_CASES:
        out, _ = run_zsh(script)
        check("zsh/" + script[:52], out, want)


def slice_cases():
    for script, want in SLICE_CASES:
        out, _ = run_zsh(script)
        check("slice/" + script[len(A5):][:48], out, want)
    # bash has no slice and DOES mean the comma operator, so the same text
    # must keep answering bash's way with the dialect off. Without this the
    # suite could not tell "gated correctly" from "changed the default".
    out, _ = run_bash_mode('a=(one two three four five); echo "[${a[2,3]}]"')
    check("slice/bash-mode-is-the-comma-operator", out, "[four]")
    out, _ = run_bash_mode('a=(1 2 3); a[1,2]=x; echo "[${a[@]}]"')
    check("slice/bash-mode-writes-one-element", out, "[1 2 x]")
    # KNOWN DIVERGENCE, recorded rather than matched. ${#a[lo,hi]} on a
    # range that starts past the end: zsh 5.9 answers by how WIDE the range
    # is rather than by how much of it exists --
    #     a=();  ${#a[1,1]} -> 0   ${#a[1,2]} -> 1   ${#a[2,3]} -> 1
    #     a=(x); ${#a[2,3]} -> 1
    # -- so an empty slice reports one element whenever it spans more than
    # one position. That is not a rule anything can rely on; it looks like
    # an off-by-one in how zsh materialises an out-of-range slice. hellish
    # answers "how many elements the slice covers", which agrees with zsh
    # everywhere the slice is in range. Pinned so the choice stays visible.
    out, _ = run_zsh('a=(); echo "[${#a[1,1]}][${#a[1,2]}][${#a[2,3]}]"')
    check("slice/empty-array-count-diverges-from-zsh", out, "[0][0][0]")


def bash_cases():
    """Every one of these must match bash 5.3.9, which is what the 3790
    golden cases pin. A failure here means the dialect leaked."""
    for script, want in BASH_CASES:
        out, _ = run_bash_mode(script)
        check("bash/" + script[:52], out, want)


def localoptions_cases():
    """`setopt localoptions extendedglob` (oh-my-zsh's extract) must not
    escape the function. extendedglob changes how every later pattern
    PARSES, so leaking it would silently re-interpret a glob typed at the
    prompt half an hour afterwards, with nothing connecting the two."""
    out, _ = run_zsh('f() { setopt localoptions extendedglob; }\n'
                     'f\n'
                     'shopt extglob\n')
    check("localoptions/reverts-on-return", "off" in out, True)
    # Without localoptions, zsh's options ARE global and must survive.
    out, _ = run_zsh('f() { setopt extendedglob; }\nf\nshopt extglob\n')
    check("localoptions/absent-means-global", "on" in out, True)


def bad_subscript_cases():
    """An out-of-range subscript must not write somewhere else -- assigning
    to a different element than the script named is the one outcome worse
    than stopping. But HOW FAR the refusal stops is MEASURED, not assumed:

        -c 'unset a; a[-1]=x; echo R'   bash and zsh both abort, rc 1
        inside a sourced file           zsh abandons the FILE (source returns
                                        126), bash abandons only the rest of
                                        that command list -- NEITHER of them
                                        kills the shell

    The second row is the one that cost something. Killing the shell there
    took oh-my-zsh's git plugin from 201 aliases to nothing: four lines from
    the end of its file it writes `aliases[$name]=`, `aliases` is a zsh
    special we do not have, so the subscript is invalid and the whole
    `-c 'source ...'` died before anything could see what it had defined.

    The same policy governs a readonly assignment, so it is pinned here too
    rather than left to be rediscovered from another plugin."""
    out, rc = run_bash_mode('unset a; a[-1]=x; echo REACHED')
    check("bad-sub/bash-negative-past-start",
          (rc, "REACHED" in out, "bad array subscript" in out),
          (1, False, True))
    out, _ = run_zsh('a=(1 2 3); a[0]=(x); echo REACHED')
    check("bad-sub/zsh-a0-is-refused",
          ("REACHED" in out, "invalid subscript range" in out),
          (False, True))
    out, rc = run_zsh('a=(1 2 3)\na[0]=(x)\necho IN_FILE\n',
                      after='echo ALIVE')
    check("bad-sub/refusal-stops-the-file-not-the-shell",
          (rc, "IN_FILE" in out, "ALIVE" in out), (0, False, True))
    out, rc = run_zsh('readonly r=1\nr=2\necho IN_FILE\n', after='echo ALIVE')
    check("bad-sub/readonly-stops-the-file-not-the-shell",
          (rc, "IN_FILE" in out, "ALIVE" in out, "readonly" in out),
          (0, False, True, True))
    out, rc = run_zsh('a=(1 2); shift 5 a; echo "rc=$? [${a[@]}]"')
    check("bad-sub/shift-past-end-leaves-array-alone",
          "rc=1 [1 2]" in out, True)


def churn_cases():
    """Allocation pressure, because the bug that started all of this
    (t_scope_save) needed it to show and no one-line case reproduced it."""
    # Two elements pushed per turn so the array is never empty: `a[$#a]` on
    # an empty array is a[0], which zsh refuses because it counts from 1.
    script = ("a=()\ni=0\n"
              "while [ $i -lt 300 ]; do\n"
              "  a+=($i $i)\n"
              "  a[1]=()\n"
              "  a[$#a]=(x y)\n"
              "  shift a\n"
              "  i=$((i+1))\n"
              "done\n"
              'echo "done n=$#a"\n')
    out, rc = run_zsh(script)
    check("churn/300-splices-clean", rc == 0 and "done" in out, True)
    check("churn/no-sanitizer-report",
          "AddressSanitizer" not in out and "LeakSanitizer" not in out, True)
    # The same pressure through the SLICE, which reaches arr_splice over a
    # range instead of one element and grows and shrinks the array by two
    # every turn rather than staying the same size.
    script = ("a=(0 1 2 3)\ni=0\n"
              "while [ $i -lt 300 ]; do\n"
              "  a[2,3]=($i $i x)\n"
              "  a[1,2]=()\n"
              "  a+=(p q)\n"
              "  i=$((i+1))\n"
              "done\n"
              'echo "done n=$#a [${a[1,2]}]"\n')
    out, rc = run_zsh(script)
    check("churn/300-slice-splices-clean", rc == 0 and "done" in out, True)
    check("churn/slice-no-sanitizer-report",
          "AddressSanitizer" not in out and "LeakSanitizer" not in out, True)
    for bad in ('a=(1); a[]=()', 'a=(1); a[x]=()', 'a=(1); shift a a a a',
                'a=(1); a[999999]=()', 'shift 1 2 3 4', 'a=(1); a[-99]=()',
                'a=(1); echo "${a[,]}"', 'a=(1); echo "${a[1,]}"',
                'a=(1); echo "${a[,2]}"', 'a=(1); echo "${a[1,2,3]}"',
                'a=(1); echo "${a[-9,-9]}"', 'a=(1); a[,]=(x)',
                'a=(1); a[999999,999999]=(x)', 'x=s; echo "${x[9,9]}"'):
        out, rc = run_zsh(bad + "; echo SURVIVED")
        check("churn/no-crash: " + bad[:26],
              "AddressSanitizer" not in out and rc in (0, 1, 2, 127), True)


def main():
    if not os.path.exists(SHELL):
        print("no shell at", SHELL)
        return 1
    zsh_cases()
    slice_cases()
    bash_cases()
    localoptions_cases()
    bad_subscript_cases()
    churn_cases()
    print("\n%d failed" % len(FAILS) if FAILS else "\nall passed")
    return 1 if FAILS else 0


sys.exit(main())
