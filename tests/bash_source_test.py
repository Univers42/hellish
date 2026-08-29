#!/usr/bin/env python3
"""Regression test: a sourced file can locate itself -- issue #71 item 6.

Issue #71 calls BASH_SOURCE "the single change that would most improve plugin
portability", and it is the reason: `$0` inside a sourced file is
/usr/bin/hellish, not the file, so a module could not derive its own
directory. Every module in the reporter's config had to hardcode
$HOME/.hellish, which means the tree cannot be relocated or vendored into a
repo. The shell tracked only two int counters (func_depth, source_depth) --
how DEEP it was, never WHERE.

The idiom that has to work is the one every bash/zsh plugin opens with:

    plugin_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BASH_SOURCE[0] names the file where the running code was WRITTEN, not
whatever is being sourced when it is CALLED -- so a helper defined in a
plugin still reports the plugin's own path. That distinction is the whole
point, and it is what t_shell_func.src records at definition time.

Also covers FUNCNAME (stack traces in plugin code) and EUID.

Documented divergence from bash: BASH_SOURCE here has exactly one entry per
live frame rather than bash's FUNCNAME-length-plus-main. Element [0] agrees
with bash, which is what the idiom uses. BASH_LINENO is deliberately absent:
tok_lineno() cannot resolve lines inside sourced text, so it would be a
confident lie.

Usage: python3 bash_source_test.py [/path/to/hellish]
"""
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    ROOT, "build", "bin", "hellish")
ENV = dict(os.environ, HELLISH_NO_BANNER="1", HELLISH_NO_UPDATE_CHECK="1",
           HELLISH_NO_ANIM="1")
FAILS = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("  " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def run(script):
    return subprocess.run([SHELL, "-c", script], capture_output=True,
                          text=True, env=ENV, timeout=30)


def write(d, name, text):
    p = os.path.join(d, name)
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, "w") as f:
        f.write(text)
    return p


def main():
    if not os.path.isfile(SHELL):
        print("error: no shell at %s -- run make" % SHELL)
        sys.exit(2)

    tmp = tempfile.mkdtemp()

    # 1. THE idiom: a sourced file derives its own directory.
    plug = write(tmp, "plugins/git/plugin.hsh",
                 'PLUGIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"\n'
                 'echo "dir=$PLUGIN_DIR"\n')
    p = run(". " + plug)
    want = os.path.join(tmp, "plugins", "git")
    check("a sourced file derives its own directory",
          p.stdout.strip() == "dir=" + want,
          "got %r want dir=%s  stderr=%r"
          % (p.stdout.strip(), want, p.stderr.strip()[:150]))

    # 2. The unbraced form is the same element.
    bare = write(tmp, "plugins/git/b.hsh", 'echo "u=$BASH_SOURCE"\n')
    p = run(". " + bare)
    check("unbraced $BASH_SOURCE is element 0",
          p.stdout.strip() == "u=" + bare, "got %r" % p.stdout.strip())

    # 3. The distinction that matters: a FUNCTION reports where it was
    #    DEFINED, not where it was called from.
    lib = write(tmp, "lib/helper.hsh",
                'whereami() { echo "fn=${BASH_SOURCE[0]}"; }\n')
    caller = write(tmp, "other/caller.hsh",
                   '. ' + lib + '\nwhereami\n')
    p = run(". " + caller)
    check("a function reports its DEFINING file, not the caller's",
          p.stdout.strip() == "fn=" + lib,
          "got %r want fn=%s" % (p.stdout.strip(), lib))

    # 4. FUNCNAME, innermost first.
    p = run('outer() { inner; }; inner() { echo "f=${FUNCNAME[0]}"; }; outer')
    check("FUNCNAME[0] is the running function",
          p.stdout.strip() == "f=inner", "got %r" % p.stdout.strip())
    p = run('outer() { inner; }; inner() { echo "n=${#FUNCNAME[@]} '
            '0=${FUNCNAME[0]} 1=${FUNCNAME[1]}"; }; outer')
    check("FUNCNAME is a stack, innermost first",
          p.stdout.strip() == "n=2 0=inner 1=outer",
          "got %r" % p.stdout.strip())

    # 5. Frames unwind: nothing leaks after the call or the source returns.
    p = run('f() { :; }; f; echo "after=[${FUNCNAME[0]}] n=${#FUNCNAME[@]}"')
    check("FUNCNAME empties when the call returns",
          p.stdout.strip() == "after=[] n=0", "got %r" % p.stdout.strip())
    p = run(". " + plug + '; echo "after=[${BASH_SOURCE[0]}]"')
    check("BASH_SOURCE empties when the source returns",
          "after=[]" in p.stdout, "got %r" % p.stdout.strip())

    # 6. Nesting: a plugin sourcing a sibling still resolves correctly.
    write(tmp, "plugins/deep/inner.hsh", 'echo "in=${BASH_SOURCE[0]}"\n')
    outer = write(tmp, "plugins/deep/outer.hsh",
                  'echo "out=${BASH_SOURCE[0]}"\n'
                  '. "$(dirname "${BASH_SOURCE[0]}")/inner.hsh"\n'
                  'echo "back=${BASH_SOURCE[0]}"\n')
    p = run(". " + outer)
    lines = p.stdout.split()
    check("nested source: inner overrides, then restores",
          lines == ["out=" + outer,
                    "in=" + os.path.join(tmp, "plugins/deep/inner.hsh"),
                    "back=" + outer],
          "got %r" % (lines,))

    # 7. EUID (issue #71 item 6).
    p = run('echo "$EUID"')
    check("EUID is set", p.stdout.strip() == str(os.geteuid()),
          "got %r want %d" % (p.stdout.strip(), os.geteuid()))
    p = run('echo "$UID"')
    check("UID still set", p.stdout.strip() == str(os.getuid()),
          "got %r" % p.stdout.strip())

    # 8. Not a regression: outside any source, these are simply empty rather
    #    than garbage, and the shell does not crash reading them.
    p = run('echo "[${BASH_SOURCE[0]}][${FUNCNAME[0]}]"')
    check("empty at top level, no crash", p.stdout.strip() == "[][]"
          and p.returncode == 0,
          "got %r rc=%d" % (p.stdout.strip(), p.returncode))

    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
