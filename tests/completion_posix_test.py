#!/usr/bin/env python3
"""Regression test: TAB in command position offers COMMANDS.

The report: pressing TAB at an empty prompt offered thousands of matches,
and the list was full of things that are not commands at all -- documents
and folders that merely happen to live in a directory named by $PATH.

    ╰─❯ <TAB><TAB>
    Display all 2790 possibilities? (y or n)

The cause was that the PATH scan in complete_commands.c matched on the
directory ENTRY NAME alone:

    if (ft_strncmp(ent->d_name, text, tlen) == 0)
        return (rl_dup(ent->d_name));

Every readdir() result passed -- a plain 0644 file, a subdirectory, and
"." and ".." from every single PATH element. POSIX defines command search
(XCU 2.9.1.1) as: for each PATH prefix, the concatenation is used only if
it names an EXECUTABLE FILE. Anyone whose PATH holds a directory that also
holds data -- ~/bin, ~/.local/bin, a project's ./scripts -- had those data
files offered as commands. That is the whole report.

Two more divergences from bash live in the same dispatcher and are covered
here, because they are the same question ("is this word a command?") asked
in a different spot:

  * command position was recognised ONLY at column 0, so `ls | <TAB>`,
    `ls; <TAB>`, `true && <TAB>` and a line with leading blanks all fell
    through to filename completion. bash completes a command in every one
    of those; the word after a control operator IS a command word.
  * `start == 0` was tested before `text[0] == '$'`, so `$HOM<TAB>` at the
    start of a line never reached variable completion.

And the guard rails, so the fix cannot overshoot: after a redirection
operator, after `VAR=`, and in ordinary argument position, the completion
must stay a plain FILE completion with no executable filter.

The oracle throughout is bash's own behaviour, recorded against
bash 5.1.16 with PATH set to one fixture directory.

Reproducibility is the point of this file, so nothing here reads the host:
PATH is replaced with a single fixture directory of known contents, HOME
is a scratch directory with no ~/.hellishrc, and the expected builtin set
is derived from src/builtins/hash_builtins*.c rather than typed
out here (the same trick tests/help_test.sh uses -- a list typed twice is
a list that rots). That makes the completion set a CLOSED set: this test
asserts the offered names are EXACTLY the builtins plus the one executable
in the fixture, which is why it catches ".." and a stray document with the
same assertion.

Replacing PATH also buys the safety property tests/completion_test.py had
to work for: a keystroke that escapes onto the command line cannot run a
host program, because there are no host programs on PATH. The Ctrl-U
before every Enter is kept anyway.

Usage: python3 completion_posix_test.py [/path/to/hellish]
"""
import fcntl
import glob
import os
import pty
import re
import select
import shutil
import struct
import sys
import tempfile
import termios
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERBOSE = "-v" in sys.argv[1:]
ARGS = [a for a in sys.argv[1:] if a != "-v"]
SHELL = os.path.abspath(ARGS[0] if ARGS
                        else os.path.join(ROOT, "build/bin/hellish"))
# The dispatch table spans more than one file -- the zsh builtins live in
# hash_builtins_zsh.c -- so this globs rather than naming one. A registration
# file added without being listed here would make the expected set too small
# and the test would report the shell offering "extra" builtins, which is a
# harness bug wearing a shell bug's clothes.
DISPATCH = glob.glob(os.path.join(ROOT, "src/builtins/hash_builtins*.c"))
FAILS = []

# The one executable in the fixture PATH directory. Everything else in
# there is a decoy: a document, a second document with a space in its
# name, a 0644 file with no extension at all, and a subdirectory.
EXE = "zzcmd_run"
DECOYS = ("zzcmd_notes.txt", "zzcmd_report.pdf", "zzcmd_plain",
          "zzcmd_folder")
SPACED = "zzcmd my notes.txt"


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name)
    if not ok:
        if detail:
            print("       " + detail.replace("\n", "\n       "))
        FAILS.append(name)


def builtin_names():
    """The builtin set, read from the dispatch table it is registered in.

    help_test.sh derives its expectations the same way and for the same
    reason: a builtin added without a completion entry should fail a test,
    not go unnoticed until someone types its name and TAB does nothing.
    """
    src = ""
    for path in DISPATCH:
        with open(path) as f:
            src += f.read()
    return set(re.findall(r'hash_set\(h, "([^"]*)"', src))


def drain_idle(fd, chunks, quiet=0.35, cap=8.0):
    """Read until the pty has been SILENT for `quiet` seconds (or `cap`).

    Not a fixed sleep. This file used to give the shell a flat wall-clock
    budget per keystroke -- 0.8s to start, then 1.3s to complete a word --
    and that is a race, not a wait: it passes on an idle machine and fails
    on a busy one. Run alone it was green; run after fifteen other pty
    tests it reported three completion failures that were nothing but an
    expired stopwatch, which is a harness bug wearing a shell bug's
    clothes.

    Waiting for quiet is both more robust under load and faster when idle.
    `cap` is the backstop so a genuinely wedged shell still fails instead
    of hanging.
    """
    last = time.time()
    end = last + cap
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                c = os.read(fd, 65536)
            except OSError:
                break
            if not c:
                break
            chunks.append(c)
            last = time.time()
        elif time.time() - last >= quiet:
            break
    return chunks


def run(sends, path, cwd, home, extra_env=None, settle=1.3):
    env = {
        "HOME": home, "PATH": path, "TERM": "xterm-256color",
        "LANG": "C.UTF-8", "PS1": "$ ",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1",
        # the host's inputrc must not decide whether TAB dings, lists or
        # completes -- that is the thing under test.
        "INPUTRC": "/dev/null",
        "ASAN_OPTIONS": "detect_leaks=0",
    }
    if extra_env:
        env.update(extra_env)
    pid, fd = pty.fork()
    if pid == 0:
        # Everything here is a FORK of this interpreter, so an exception
        # that escapes does not just fail the case: the child unwinds back
        # into main(), runs the rest of the file as a second test process,
        # and hits the fixture cleanup -- deleting the directory tree the
        # real test is still using. The symptom is a cascade of unrelated
        # failures with a Python traceback embedded in the pty capture, and
        # it costs an afternoon to read. execv RAISES rather than returning
        # when the path is wrong, so the bare _exit below is not enough on
        # its own; nothing may leave this block except _exit.
        try:
            os.environ.clear()
            os.environ.update(env)
            os.chdir(cwd)
            os.execv(SHELL, [SHELL])
        except BaseException:
            pass
        os._exit(127)
    # wide, so a column listing stays a handful of lines
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 60, 200, 0, 0))
    # Startup output lands in the same buffer the checks read, exactly as it
    # did when this was a sleep followed by a read.
    chunks = []
    drain_idle(fd, chunks, quiet=0.4, cap=6.0)
    for data in sends:
        try:
            os.write(fd, data)
        except OSError:
            break
        # `settle` is now a floor on how long silence must last, not a
        # countdown the shell has to beat.
        drain_idle(fd, chunks, quiet=min(settle, 0.6), cap=settle * 6 + 6)
    try:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    except OSError:
        pass
    text = b"".join(chunks).decode(errors="replace")
    if VERBOSE:
        print("    << %r" % text)
    SEEN_CRASH.extend(m for m in CRASH_MARKERS if m in text)
    return text


ANSI = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]")

# exec_file_gen() releases strings readline allocated, so this file exercises
# the same ownership handoff that issue #40 was about (see the long note on
# rl_dup in completion.c). completion_test.py is the gate for that -- it
# builds SAFE=0 on purpose, which is the only build where a cross-heap free
# can be detected at all -- but scanning here too costs nothing and means a
# SAFE=0 run of this file reports the abort instead of a puzzling timeout.
CRASH_MARKERS = ("free(): invalid", "double free", "corrupted",
                 "munmap_chunk", "AddressSanitizer", "not malloc()-ed",
                 "SIGSEGV", "Aborted")
SEEN_CRASH = []


def listing(out):
    """The names readline column-printed, as a set.

    readline announces a listing with a bell, then \\r\\n, then the columns,
    then redraws the prompt. So the block we want is everything between the
    first "\\a\\r\\n" and the redraw that follows it.
    """
    i = out.find("\a\r\n")
    if i < 0:
        return set()
    block = out[i + 3:]
    j = block.find("\r\n$ ")
    if j >= 0:
        block = block[:j]
    return set(ANSI.sub("", block).split())


def line_is(out, want):
    """True if `want` ended up on the command line.

    The BEL has to go before the substring test.  readline dings for the
    ambiguity and THEN inserts the character it could agree on, so a line
    that reads "wfile" arrives as "wfil\ae" -- and a naive `"wfile" in out`
    quietly reports that no completion happened when one did, which is the
    wrong answer in exactly the direction that would let this file pass
    against the unfixed shell.
    """
    return want in ANSI.sub("", out).replace("\a", "")


def launched_something(out):
    """A keystroke escaped and ran a full-screen host program."""
    return "\x1b[?1049h" in out or "\x1b[?47h" in out


def make_fixtures():
    root = tempfile.mkdtemp(prefix="hellish_comp_posix_")
    binp = os.path.join(root, "bin")
    work = os.path.join(root, "work")
    home = os.path.join(root, "home")
    for d in (binp, work, home):
        os.makedirs(d)
    p = os.path.join(binp, EXE)
    with open(p, "w") as f:
        f.write("#!/bin/sh\necho RAN_THE_FIXTURE\n")
    os.chmod(p, 0o755)
    for name in DECOYS[:-1] + (SPACED,):
        p = os.path.join(binp, name)
        with open(p, "w") as f:
            f.write("this is a document, not a program\n")
        os.chmod(p, 0o644)
    os.makedirs(os.path.join(binp, DECOYS[-1]))
    # the working directory is deliberately NOT the PATH directory, and
    # holds only unexecutable files: argument completion must still offer
    # them, command completion must never reach them.
    for name in ("wfile.txt", "wfile_two.txt"):
        with open(os.path.join(work, name), "w") as f:
            f.write("x\n")
    return root, binp, work, home


def main():
    if not os.access(SHELL, os.X_OK):
        sys.exit("completion_posix_test: not executable: %s" % SHELL)
    root, binp, work, home = make_fixtures()
    try:
        cases(binp, work, home)
    finally:
        shutil.rmtree(root, ignore_errors=True)
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


def cases(binp, work, home):
    expected = builtin_names() | {EXE}

    # 1. The report itself. PATH is one directory, so the whole offered set
    #    fits on screen and can be compared exactly. Two TABs (one only
    #    dings); the 'y' answers "Display all N possibilities?" if the
    #    builtin count ever grows past readline's query threshold, and is
    #    harmless if the question did not appear because Ctrl-U discards
    #    the line -- and because PATH holds no program it could name.
    out = run([b"\t\t", b"y", b"\x15"], binp, work, home, settle=1.8)
    got = listing(out)
    check("TAB at an empty prompt offers commands, not directory entries",
          got == expected,
          "not commands, but offered: %s\nmissing from the offer: %s"
          % (sorted(got - expected) or "-", sorted(expected - got) or "-"))
    check("'..' is never offered as a command", ".." not in got,
          "readdir's '..' reached the completion list")
    for name in DECOYS:
        check("a non-executable %r on PATH is not a command"
              % name, name not in got, "offered anyway")
    check("a document with a space in its name is not a command",
          SPACED.split()[0] not in got and "notes.txt" not in got,
          "offered: %s" % sorted(got))

    # 2. With the decoys filtered out there is exactly ONE match for the
    #    prefix, so readline completes it outright instead of listing four.
    out = run([b"zzcmd_\t", b"\x15", b"\n"], binp, work, home)
    check("an unambiguous command prefix completes to the executable",
          line_is(out, EXE + " "),
          "the decoys made it ambiguous; tail=%r" % out[-200:])

    # 3. The word after a control operator is a command word (POSIX XCU
    #    2.9.1), and so is the first word of a line that starts with
    #    blanks. Each of these completed as a FILENAME before.
    for label, prefix in (("after a pipe", b"echo hi | "),
                          ("after ';'", b"echo hi; "),
                          ("after '&&'", b"true && "),
                          ("after '||'", b"false || "),
                          ("after '&'", b"true & "),
                          ("inside $( )", b"echo $("),
                          ("after leading blanks", b"   ")):
        out = run([prefix + b"zzcmd_\t", b"\x15", b"\n"], binp, work, home)
        check("a command completes %s" % label, line_is(out, EXE + " "),
              "tail=%r" % out[-200:])

    # 4. ...and the guard rails, so the fix cannot overshoot. After a
    #    redirection operator, after VAR=, and in argument position the
    #    completion is a plain FILE completion: no executable filter, no
    #    PATH.
    for label, sends in (("after '>'", b"cat > wfile_t"),
                         ("after '<'", b"cat < wfile_t"),
                         ("after 'VAR='", b"V=wfile_t"),
                         ("in argument position", b"cat wfile_t")):
        out = run([sends + b"\t", b"\x15", b"\n"], binp, work, home)
        check("a plain file still completes %s" % label,
              line_is(out, "wfile_two.txt"), "tail=%r" % out[-200:])

    # 5. A command word that contains a slash is not searched on PATH; it
    #    is a filename, filtered to what could actually be executed.
    out = run([b"./zzcmd_\t\t", b"\x15"], binp, binp, home, settle=1.8)
    got = listing(out)
    check("'./' in command position offers only what can be run",
          got == {EXE, DECOYS[-1] + "/"},
          "offered %s, wanted %s" % (sorted(got),
                                     sorted({EXE, DECOYS[-1] + "/"})))

    # 5b. The same wrong list arriving by a second route. readline treats a
    #     NULL answer from rl_attempted_completion_function as "not mine"
    #     and retries the word as a plain filename, so a command name that
    #     matched nothing on PATH quietly completed against the current
    #     directory instead. The working directory here holds wfile.txt and
    #     wfile_two.txt and neither is executable: in COMMAND position they
    #     must not be reachable at all, and TAB must simply ding.
    out = run([b"wfil\t\t", b"\x15"], binp, work, home, settle=1.8)
    check("an unknown command name does not fall back to the cwd's files",
          listing(out) == set() and not line_is(out, "wfile"),
          "offered %s; tail=%r" % (sorted(listing(out)), out[-300:]))
    out = run([b"./wfil\t\t", b"\x15"], binp, work, home, settle=1.8)
    check("'./' offers nothing when nothing there can be run",
          listing(out) == set() and not line_is(out, "wfile"),
          "offered %s; tail=%r" % (sorted(listing(out)), out[-300:]))

    # 6. The dispatcher tested start==0 before text[0]=='$', so a variable
    #    at the very start of a line was sent to command completion.
    out = run([b"$ZZPROBE_V\t", b"\x15", b"\n"], binp, work, home,
              extra_env={"ZZPROBE_VARIABLE": "1"})
    check("$VAR completes at the start of a line",
          line_is(out, "$ZZPROBE_VARIABLE"), "tail=%r" % out[-200:])

    # 7. None of the above left the line editor wedged, and no keystroke
    #    this file typed ran a program (the lesson tests/completion_test.py
    #    was written around).
    out = run([b"\t\t", b"y", b"\x15", b"\n", b"echo STILL_ALIVE\n"],
              binp, work, home, settle=1.8)
    check("the shell still runs commands after all that",
          out.count("STILL_ALIVE") >= 2, "tail=%r" % out[-300:])
    check("no keystroke this test typed launched a program",
          not launched_something(out), "tail=%r" % out[-300:])

    # 8. Nothing above tripped the allocator. Only meaningful on a SAFE=0
    #    build, and free either way.
    check("no run in this file corrupted the heap", not SEEN_CRASH,
          "saw %s" % sorted(set(SEEN_CRASH)))


main()
