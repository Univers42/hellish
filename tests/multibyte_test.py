#!/usr/bin/env python3
"""Regression test: string operations count CHARACTERS in a multibyte
locale, as bash does -- issue #120.

`${#var}` returned the byte length, so "café" measured 5 where bash says 4;
the same byte arithmetic ran through ${var:off:len} (which cut é in half),
the `?` and `[...]` of every pattern (`caf?` did not match café, ${x%?}
left a stray byte), read -n/-N, and ${x^^} (which could not upper-case é).
Box-drawing and column-padding code that aligns by ${#var} was off by one
per accented character.

The golden suite runs under LC_ALL=C, where bytes ARE characters, so it
cannot see any of this. This test runs the same snippets through hellish
and bash under C.UTF-8 and requires identical output and status; it runs
them again under C, where both must still agree byte for byte -- the
single-byte path must not have moved. The oracle is the pinned bash 5.3.9
when present (HELLISH_ORACLE or ~/bash-5.3.9), else the bash on PATH.

Usage: python3 multibyte_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    ROOT, "build", "bin", "hellish")
ORACLE = os.environ.get("HELLISH_ORACLE") or os.path.expanduser(
    "~/bash-5.3.9/bin/bash")
if not os.path.isfile(ORACLE):
    ORACLE = "bash"
FAILS = []

CASES = [
    'x=café; echo ${#x}',
    'x=café; echo "${x:1:2}|${x:3}|${x: -1}|${x:0:-1}|${x:(-2)}"',
    'x=café; echo "${x%?}|${x#?}|${x%%??}|${x##???}"',
    'x=café; echo "${x/?/_}|${x//?/_}|${x/%?/_}|${x/#?/_}"',
    'x=café; case $x in caf?) echo yes;; *) echo no;; esac',
    'x=café; [[ $x == caf? ]] && echo dyes || echo dno',
    'x=café; [[ $x == caf[é] ]] && echo byes || echo bno',
    'x=café; [[ $x == caf[^é] ]] && echo nyes || echo nno',
    'x=cafè; [[ $x == caf[é] ]] && echo wrong || echo bno2',
    'x=café; [[ $x == caf[a-z] ]] && echo ryes || echo rno',
    'x=café; echo "${x^^}|${x,,}|${x^}|${x~~}"',
    'x=ÉCOLE; echo "${x,,}|${x,}|${x~}"',
    'x=café; echo "${x@U}|${x@L}|${x@u}"',
    'a=(café thé 日本); echo "${#a[0]} ${#a[1]} ${#a[2]} ${#a[@]}"',
    'printf \'héllo\' | { read -n 2 x; echo "[$x]"; }',
    'printf \'héllo\' | { read -N 3 x; echo "[$x]"; }',
    'printf \'héllo\\n\' | { read -n 1 x; read -n 1 y; echo "[$x][$y]"; }',
    'x=日本語; echo "${#x} ${x:1:1} ${x:2} ${x%?}"',
    'x=$\'a\\xffb\'; echo "${#x} ${x:1:1}" | od -c | head -1',
    'x=café; echo "${x:5:1}|${x:4:1}"; echo rc=$?',
    'x=café; echo "${x//[é]/E}"',
    'for w in café thé; do echo "${#w}"; done',
    'x=naïve; echo "$(( ${#x} ))"',
    'x=café; y=${x:1}; echo "${#y}"',
    'x=aéb; echo "${x/[é]/X}|${x//[^a]/-}"',
    'echo caf? | { read -r p; [[ café == $p ]] && echo pat-yes || echo pat-no; }',
    'x=café; printf \'[%6s][%-6s]\\n\' "$x" "$x"',
    'x=caféé; echo "${x%%é*}|${x#*é}|${x##*é}"',
    'case é in [[:alpha:]]) echo alpha;; ?) echo one;; *) echo other;; esac',
    'x=ab; echo "${x:1:5}|${x:2}|${#x}"',
    'x=ééé; printf \'%s\\n\' "${x:1:1}" "${#x}" "${x^}"',
    'printf \'%-8s|\\n\' "$(x=été; echo "${x^^}")"',
]


def run(shell, loc, script):
    env = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"), "LC_ALL": loc,
           "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
           "HELLISH_NO_ANIM": "1"}
    return subprocess.run([shell, "-c", script], capture_output=True,
                          env=env, timeout=30)


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)
    for loc in ("C.UTF-8", "C"):
        for script in CASES:
            h = run(SHELL, loc, script)
            b = run(ORACLE, loc, script)
            ok = (h.stdout == b.stdout and h.returncode == b.returncode)
            check("%-7s %s" % (loc, script), ok,
                  "hellish=%r rc=%d  bash=%r rc=%d"
                  % (h.stdout, h.returncode, b.stdout, b.returncode))
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
