#!/usr/bin/env python3
"""#72 phase 4, the second half: `complete` specs are CONSULTED at TAB.

The registry landed first -- `complete -W ... git` stored a spec, `complete
-p` printed it back, and `compgen` generated the same words on demand. None
of that reached the line editor. The completer was a three-way branch on
what the word looked like (`$` -> variable, command position -> command,
anything else -> readline's filenames) and it never asked whether a spec
existed for the command being typed.

That is the failure mode this project treats as the worst one: the shell
answers, the status is 0, and the answer is wrong. `complete -W 'add commit
push' git` returned 0, `complete -p git` echoed it back, and TAB offered the
files in the current directory. A completion script looked installed and was
inert -- which is exactly what left git-completion.bash defining 140
functions and doing nothing.

The two shapes plugins actually use:

    complete -W 'a b c' cmd        a literal word list
    complete -F _fn cmd            a function that fills COMPREPLY

and the function contract bash defines, which scripts genuinely read:
$1 = command, $2 = current word, $3 = previous word, plus COMP_WORDS,
COMP_CWORD, COMP_LINE and COMP_POINT.

The control at the end matters as much as the rest: a command with NO spec
must still get readline's filename completion. A dispatcher that claims
every argument word would break completion everywhere it was already right.

Usage: python3 progcomp_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import select
import shutil
import struct
import sys
import tempfile
import termios
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else os.path.join(os.path.dirname(__file__),
                                          "../build/bin/hellish"))
FAILS = []

CRASH_MARKERS = ("free(): invalid size", "double free", "corrupted",
                 "munmap_chunk", "AddressSanitizer", "not malloc()-ed",
                 "SIGSEGV", "Aborted", "invalid pointer")


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  " + detail))
    if not ok:
        FAILS.append(name)


def run(sends, cwd=None, settle=1.2):
    """Drive an interactive shell and return one output chunk PER SEND.

    Per-send and not one blob, because the setup lines are echoed back by
    the terminal: `complete -W 'alpha beta gamma' pfoo` puts all three words
    on screen before any TAB is pressed. Asserting against the whole
    transcript would pass on the echo of the registration and prove nothing
    about the completer -- three of the cases here did exactly that on the
    first draft, and agreed with a shell that had no dispatch at all.
    """
    env = {
        "HOME": os.environ.get("HOME", "/tmp"),
        "PATH": os.environ["PATH"],
        "TERM": "xterm-256color", "LANG": "C.UTF-8", "PS1": "$ ",
        "HELLISH_NO_BANNER": "1", "HELLISH_BANNER": "0",
        "HELLISH_NO_UPDATE_CHECK": "1", "HELLISH_NO_ANIM": "1",
        # the host's inputrc must not decide whether TAB lists or dings.
        "INPUTRC": "/dev/null",
        "ASAN_OPTIONS": "detect_leaks=0",
    }
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        if cwd:
            os.chdir(cwd)
        os.execv(SHELL, [SHELL, "--norc"])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 40, 100, 0, 0))
    time.sleep(0.9)
    chunks = []
    for data in sends:
        got = b""
        try:
            os.write(fd, data)
        except OSError:
            break
        end = time.time() + settle
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.1)
            if r:
                try:
                    got += os.read(fd, 65536)
                except OSError:
                    break
        chunks.append(got.decode(errors="replace"))
    try:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    except OSError:
        pass
    while len(chunks) < len(sends):
        chunks.append("")
    return chunks


def no_crash(chunks):
    blob = "".join(chunks)
    return [m for m in CRASH_MARKERS if m in blob]


# Every case ends the typed line with Ctrl-U so that whatever TAB did or did
# not insert can never be run as a command -- the lesson completion_test.py
# paid for on a CI runner where the decline key was a real program on PATH.
KILL = b"\x15"


def alive(chunks, name):
    check(name + ": the shell survived the TAB",
          "ALIVE" in chunks[-1], "tail=%r" % chunks[-1][-200:])
    hits = no_crash(chunks)
    check(name + ": no heap corruption", not hits, "saw %s" % hits)


def main():
    if not os.path.exists(SHELL):
        print("no shell at %s" % SHELL)
        return 1

    # 1: -W with a unique prefix. One TAB must finish the word, and the
    # assertion looks only at the chunk the TAB produced.
    c = run([b"shopt -s progcomp\n",
             b"complete -W 'alpha beta gamma' pfoo\n",
             b"pfoo al\t", KILL, b"\n", b"echo ALIVE\n"])
    check("complete -W: TAB finishes a unique match",
          "alpha" in c[1], "tab chunk=%r" % c[1][-300:])
    alive(c, "complete -W")

    # 2: -W, ambiguous, in a directory full of decoys. Two TABs must list
    # the spec's words and NOTHING from the filesystem -- offering the cwd
    # is precisely what the shell did before, with status 0.
    d = tempfile.mkdtemp(prefix="hellish_pc_")
    try:
        for n in ("zzdecoy_one", "zzdecoy_two"):
            open(os.path.join(d, n), "w").close()
        c = run([b"shopt -s progcomp\n",
                 b"complete -W 'alpha beta gamma' pfoo\n",
                 b"pfoo \t\t", KILL, b"\n", b"echo ALIVE\n"],
                cwd=d, settle=1.6)
        listed = [w for w in ("alpha", "beta", "gamma") if w in c[1]]
        check("complete -W: TAB lists the whole word list",
              len(listed) == 3, "only %r; tab chunk=%r" % (listed, c[1][-400:]))
        check("complete -W: the spec replaces filename completion",
              "zzdecoy" not in c[1], "tab chunk=%r" % c[1][-400:])
    finally:
        shutil.rmtree(d, ignore_errors=True)

    # 3: -F, the shape every real completion script uses.
    c = run([b"shopt -s progcomp; _pbar() { COMPREPLY=(zulu zebra); }\n",
             b"complete -F _pbar pbar\n",
             b"pbar z\t\t", KILL, b"\n", b"echo ALIVE\n"], settle=1.6)
    check("complete -F: COMPREPLY reaches readline",
          "zulu" in c[2] and "zebra" in c[2], "tab chunk=%r" % c[2][-400:])
    alive(c, "complete -F")

    # 4: the positional contract -- $1 command, $2 current word, $3 previous.
    c = run([b"shopt -s progcomp\n_pargs() { COMPREPLY=(\"got-$1-$2-$3\"); }\n",
             b"complete -F _pargs pargs\n",
             b"pargs xy\t", KILL, b"\n", b"echo ALIVE\n"])
    check("complete -F: the function gets cmd, cur and prev",
          "got-pargs-xy-pargs" in c[2], "tab chunk=%r" % c[2][-400:])

    # 5: COMP_WORDS / COMP_CWORD, which scripts branch on far more than $2.
    c = run([b"shopt -s progcomp\n_pcw() { COMPREPLY=(\"n${#COMP_WORDS[@]}c$COMP_CWORD\"); }\n",
             b"complete -F _pcw pcw\n",
             b"pcw a b\t", KILL, b"\n", b"echo ALIVE\n"])
    check("complete -F: COMP_WORDS and COMP_CWORD describe the line",
          "n3c2" in c[2], "tab chunk=%r" % c[2][-400:])

    # 6: an action, so -A is not the one registered shape that still does
    # nothing. It routes through compgen, which already knew how.
    c = run([b"shopt -s progcomp; _zzx() { :; }\n_zzy() { :; }\n",
             b"complete -A function pfn\n",
             b"pfn _zz\t\t", KILL, b"\n", b"echo ALIVE\n"], settle=1.6)
    check("complete -A function: offers shell functions",
          "_zzx" in c[2] and "_zzy" in c[2], "tab chunk=%r" % c[2][-400:])

    # 7: THE CONTROL. No spec means readline's filename completion, exactly
    # as before. A dispatcher that claimed every argument word would pass
    # every case above and silently break completion for everything else.
    d = tempfile.mkdtemp(prefix="hellish_pc2_")
    try:
        open(os.path.join(d, "zzunique_file"), "w").close()
        c = run([b"cat zzuniq\t", KILL, b"\n", b"echo ALIVE\n"], cwd=d)
        check("a command with no spec still completes filenames",
              "zzunique_file" in c[0], "tab chunk=%r" % c[0][-400:])
    finally:
        shutil.rmtree(d, ignore_errors=True)

    # 8: THE DEFAULT. progcomp is OFF unless asked for, and the option is a
    # real switch in both directions -- which is why every case above arms
    # it explicitly. bash defaults it on; hellish does not, because the
    # option is the gate /etc/profile.d/bash_completion.sh checks and
    # bash-completion does not run here yet (see incs/shell.h and the
    # corpus row). `shopt progcomp` tells the truth either way.
    d = tempfile.mkdtemp(prefix="hellish_pc3_")
    try:
        open(os.path.join(d, "zzoff_file"), "w").close()
        c = run([b"complete -W 'alpha beta' pfoo\n",
                 b"pfoo zzoff\t", KILL, b"\n", b"echo ALIVE\n"], cwd=d)
        check("the default is off: filenames, not the spec",
              "zzoff_file" in c[1], "tab chunk=%r" % c[1][-400:])
        c = run([b"shopt -s progcomp; shopt -u progcomp\n",
                 b"complete -W 'alpha beta' pfoo\n",
                 b"pfoo zzoff\t", KILL, b"\n", b"echo ALIVE\n"], cwd=d)
        check("shopt -u progcomp turns an armed dispatch back off",
              "zzoff_file" in c[2], "tab chunk=%r" % c[2][-400:])
    finally:
        shutil.rmtree(d, ignore_errors=True)

    # 9: THE ACCEPTANCE TEST. git's own completion script -- 140 functions
    # of real bash -- driven by a real TAB. It is the thing #72 phase 4
    # exists for, and every synthetic case above can pass while this one
    # fails. It did, for four separate reasons, each of them silent:
    #
    #   compgen ... -- "$cur"       "--: invalid option"; the whole idiom
    #   ${cur%%?(\\)=*}              a bare `{` in the pattern ate the `}`
    #   for ((i=0; i<${#W[@]};))    ${#} unknown to (( )); zero iterations
    #   git ${a:+"${a[@]}"} ...     an empty expansion became an empty argv
    #                               slot, so git errored and printed nothing
    #
    # It reuses the plugin corpus's download cache rather than fetching, and
    # skips when that is absent, so an offline run stays green.
    cache = os.environ.get("XDG_CACHE_HOME",
                           os.path.join(os.environ.get("HOME", "/tmp"),
                                        ".cache"))
    gc = os.path.join(cache, "hellish-plugin-corpus", "git-completion.bash")
    if os.path.exists(gc):
        c = run([("shopt -s progcomp; source %s\n" % gc).encode(),
                 b"git che\t\t", KILL, b"\n", b"echo ALIVE\n"], settle=2.4)
        offered = [w for w in ("checkout", "cherry-pick", "cherry")
                   if w in c[1]]
        check("git-completion.bash: TAB offers git's subcommands",
              len(offered) >= 2,
              "only %r; tab chunk=%r" % (offered, c[1][-500:]))
        alive(c, "git-completion")
    else:
        print("skip git-completion (no corpus cache at %s)" % gc)

    print("\n%d checks failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())
