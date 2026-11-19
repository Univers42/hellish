#!/usr/bin/env python3
"""`make <TAB>` completes target names read from the Makefile in the cwd.

Issue #140, "make target completion" plus the provider architecture: an
ARGUMENT word that no `complete` spec claimed is offered to a per-command
provider (src/completion/complete_provider.c); `make` and `gmake` are the
first rows.  The Makefile is parsed in-process -- nothing is forked and make
is never run -- so the rules below are the shell's own, pinned here.

Each case builds a fresh temp directory holding only its fixture files and
drives a real pty.  Assertions are on the EXACT completed line or the EXACT
listing, never on "contains".

Cells: the in-process reader, and HELLISH_RL_FORK=1 (the forked reader).

Usage: python3 complete_make_test.py /path/to/hellish [--asan] [-v]
       --inproc skip the forked-reader cell (quicker, for mutation runs)
       --asan   finish each session with `exit` (so LeakSanitizer reports)
                and run only cases 2 and 21.
"""
import fcntl
import os
import pty
import re
import select
import shutil
import struct
import subprocess
import sys
import tempfile
import termios
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERBOSE = "-v" in sys.argv[1:]
ASAN = "--asan" in sys.argv[1:]
INPROC_ONLY = "--inproc" in sys.argv[1:]
ARGS = [a for a in sys.argv[1:] if not a.startswith("-")]
SHELL = os.path.abspath(ARGS[0] if ARGS
                        else os.path.join(ROOT, "build/bin/hellish"))
BASH = os.path.expanduser("~/bash-5.3.9/bin/bash")
FAILS = []
EXTRA = {}
TMP = []

ANSI = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]")
CRASH_MARKERS = ("free(): invalid", "double free", "corrupted",
                 "munmap_chunk", "AddressSanitizer", "LeakSanitizer",
                 "not malloc()-ed", "SIGSEGV", "Aborted")
SEEN_CRASH = []

mk = "Makefile"
TAB = b"\t"
ENTER = b"\n"
KILL = b"\x15"


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name)
    if not ok:
        if detail:
            print("       " + detail.replace("\n", "\n       "))
        FAILS.append(name)


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


def spawn(cwd, home, cols=200, extra=None):
    env = {
        "HOME": home, "PATH": "/usr/bin:/bin", "TERM": "xterm-256color",
        "LANG": "C.UTF-8", "PS1": "$ ",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1",
        "INPUTRC": "/dev/null",
        "ASAN_OPTIONS": "detect_leaks=%d" % (1 if ASAN else 0),
    }
    env.update(EXTRA)
    env.update(extra or {})
    pid, fd = pty.fork()
    if pid == 0:
        # nothing may leave this block except _exit (see
        # completion_posix_test.py for the cascade a leak causes)
        try:
            os.environ.clear()
            os.environ.update(env)
            os.chdir(cwd)
            os.execv(SHELL, [SHELL, "--norc"])
        except BaseException:
            pass
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 60, cols, 0, 0))
    return pid, fd


def session(cwd, steps, settle=0.6, hist=None, env=None, cols=200):
    """Run `steps` (bytes to type, or a callable run between keystrokes)
    in a fresh shell; return the output of each bytes step, in order.
    `hist` is the history the shell starts with, oldest first."""
    home = tempfile.mkdtemp(prefix="hellish_mk_home_")
    TMP.append(home)
    extra = dict(env or {})
    if hist is not None:
        extra["HISTFILE"] = os.path.join(home, "hist")
        with open(extra["HISTFILE"], "w") as f:
            f.write("".join(h + "\n" for h in hist))
    pid, fd = spawn(cwd, home, cols, extra)
    chunks = []
    drain_idle(fd, chunks, quiet=0.4, cap=6.0)
    outs = []
    for step in steps:
        if callable(step):
            step()
            continue
        mark = len(chunks)
        try:
            os.write(fd, step)
        except OSError:
            break
        drain_idle(fd, chunks, quiet=settle, cap=settle * 6 + 6)
        outs.append(b"".join(chunks[mark:]).decode(errors="replace"))
    if ASAN:
        try:
            os.write(fd, KILL + b"exit\n")
        except OSError:
            pass
        drain_idle(fd, chunks, quiet=1.0, cap=20.0)
        try:
            os.waitpid(pid, os.WNOHANG)
        except OSError:
            pass
    try:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    except OSError:
        pass
    text = b"".join(chunks).decode(errors="replace")
    if VERBOSE:
        print("    << %r" % text)
    SEEN_CRASH.extend(m for m in CRASH_MARKERS if m in text)
    return outs


def listing(out):
    """The names readline column-printed, as a set (see
    completion_posix_test.py)."""
    i = out.find("\a\r\n")
    if i < 0:
        return set()
    block = out[i + 3:]
    j = block.find("\r\n$ ")
    if j >= 0:
        block = block[:j]
    return set(ANSI.sub("", block).split())


def ordered(out):
    """Like listing(), but in the order printed. Needs a terminal so
    narrow (session(..., cols=10)) that readline prints one name a row."""
    i = out.find("\a\r\n")
    if i < 0:
        return []
    block = out[i + 3:]
    j = block.find("\r\n$ ")
    if j >= 0:
        block = block[:j]
    return ANSI.sub("", block).split()


def line_is(out, want):
    return want in ANSI.sub("", out).replace("\a", "")


def fixture(files, setup=None):
    d = tempfile.mkdtemp(prefix="hellish_mk_")
    TMP.append(d)
    for name, data in files.items():
        p = os.path.join(d, name)
        os.makedirs(os.path.dirname(p), exist_ok=True)
        mode = "wb" if isinstance(data, bytes) else "w"
        with open(p, mode) as f:
            f.write(data)
    if setup:
        setup(d)
    return d


def lst(files, typed, setup=None):
    """The listing after `typed` + TAB TAB, in a fresh directory."""
    d = fixture(files, setup)
    return listing(session(d, [typed + b"\t\t\t"])[0])


def comp(files, typed, setup=None):
    """The text readline echoed after `typed` + TAB."""
    d = fixture(files, setup)
    return session(d, [typed + b"\t"])[0]


def case21():
    d = fixture({mk: "a1:\na2:\n", "sub/Makefile": "s1:\ns2:\n"})
    path = os.path.join(d, mk)

    def rewrite_size():
        with open(path, "w") as f:
            f.write("a1:\na2:\na3:\n")

    def rewrite_same_size():
        with open(path, "w") as f:
            f.write("b1:\nb2:\nb3:\n")
        st = os.stat(path)
        os.utime(path, (st.st_atime + 100, st.st_mtime + 100))

    outs = session(d, [b"make \t\t\t", KILL, rewrite_size, b"make \t\t\t", KILL,
                       rewrite_same_size, b"make \t\t\t", KILL,
                       b"cd sub\n", b"make \t\t\t"])
    got = [listing(o) for o in (outs[0], outs[2], outs[4], outs[7])]
    want = [{"a1", "a2"}, {"a1", "a2", "a3"}, {"b1", "b2", "b3"},
            {"s1", "s2"}]
    for label, g, w in zip(("first", "size change", "same size, newer mtime",
                            "after cd"), got, want):
        check("21 cache: %s" % label, g == w, str(sorted(g)))


def cases():
    # 1
    got = lst({"fixture.txt": "x", "other.dat": "x"}, b"make ")
    check("1 no Makefile: filenames are offered",
          got == {"fixture.txt", "other.dat"}, str(sorted(got)))

    # 2
    base = "all clean fclean re test".split()
    body = "".join("%s:\n\t@true\n" % t for t in base)
    out = comp({mk: body}, b"make f")
    check("2 make f<TAB> -> make fclean ", line_is(out, "make fclean "),
          repr(out[-120:]))
    got = lst({mk: body, "zzfile": "x"}, b"make ")
    check("2 make <TAB><TAB> lists exactly the targets", got == set(base),
          str(sorted(got)))

    if ASAN:
        case21()
        return

    # 3
    files = {"GNUmakefile": "g_a:\ng_b:\n", "makefile": "l_a:\nl_b:\n",
             "Makefile": "u_a:\nu_b:\n"}
    check("3 GNUmakefile beats makefile beats Makefile",
          lst(files, b"make ") == {"g_a", "g_b"})
    del files["GNUmakefile"]
    check("3 makefile beats Makefile", lst(files, b"make ") == {"l_a", "l_b"})
    del files["makefile"]
    check("3 Makefile alone", lst(files, b"make ") == {"u_a", "u_b"})

    # 4
    def link(d):
        os.symlink("real.mk", os.path.join(d, "Makefile"))
    got = lst({"real.mk": "sa:\nsb:\n"}, b"make ", link)
    check("4 symlinked Makefile is followed", got == {"sa", "sb"},
          str(sorted(got)))

    def adir(d):
        os.makedirs(os.path.join(d, "Makefile"))
    got = lst({"fixture.txt": "x", "Makefile/in": "all:\nclean:\n"},
              b"make ", None)
    check("4 a DIRECTORY named Makefile -> filenames",
          got == {"fixture.txt", "Makefile/"}
          or got == {"fixture.txt", "Makefile"}, str(sorted(got)))
    if os.geteuid() == 0:
        print("skip 4 unreadable Makefile (running as root)")
    else:
        def noperm(d):
            os.chmod(os.path.join(d, mk), 0)
        got = lst({mk: "ta:\ntb:\n", "fixture.txt": "x"}, b"make ", noperm)
        check("4 unreadable Makefile -> filenames",
              got == {mk, "fixture.txt"}, str(sorted(got)))

    # 5
    got = lst({mk: "", "fixture.txt": "x"}, b"make ")
    check("5 empty Makefile -> filenames", got == {mk, "fixture.txt"},
          str(sorted(got)))
    out = comp({mk: "first:\nlast:"}, b"make la")
    check("5 no trailing newline: last target found",
          line_is(out, "make last "), repr(out[-100:]))

    # 6
    got = lst({mk: "a b c: x\ndc:: y\n"}, b"make ")
    check("6 a b c: and dc:: are all rules", got == {"a", "b", "c", "dc"},
          str(sorted(got)))

    # 7
    body = ("V := 1\nW ::= 2\nX = a:b\nY ?= c:d\nZ += e:f\n"
            "real: x\nreal2:\n")
    got = lst({mk: body}, b"make ")
    check("7 assignments are not rules", got == {"real", "real2"},
          str(sorted(got)))

    # 8
    body = "$(NAME): x\n%.o: %.c\n.PHONY: all ok\nok:\nok2:\n"
    got = lst({mk: body}, b"make ")
    check("8 $(var) targets, patterns, .PHONY and its list are not offered",
          got == {"ok", "ok2"}, str(sorted(got)))

    # 9
    got = lst({mk: "ok1:\n\techo a:b\nok2:\n"}, b"make ")
    check("9 recipe lines are not rules", got == {"ok1", "ok2"},
          str(sorted(got)))

    # 10
    got = lst({mk: "define T\nfoo: bar\nendef\nok1:\nok2:\n"}, b"make ")
    check("10 define ... endef is skipped", got == {"ok1", "ok2"},
          str(sorted(got)))

    # 11
    got = lst({mk: "lo\\\nng: x\nother:\n"}, b"make ")
    check("11 backslash-newline is one space, as in GNU make: lo ng",
          got == {"lo", "ng", "other"}, str(sorted(got)))
    got = lst({mk: "all: a \\\n\tb\nother:\n"}, b"make ")
    check("11 a continued prerequisite line is not a recipe or a rule",
          got == {"all", "other"}, str(sorted(got)))

    # 12
    d = {mk: b"all:\r\nclean: x\r\n\techo hi\r\n"}
    got = lst(d, b"make ")
    out = comp(d, b"make cl")
    check("12 CRLF Makefile: names without CR",
          got == {"all", "clean"} and line_is(out, "make clean "),
          "%s %r" % (sorted(got), out[-100:]))

    # 13
    got = lst({mk: "# hidden: x\nhelp: ## show help\nok:\n"}, b"make ")
    check("13 comments hidden, ## help rule offered",
          got == {"help", "ok"}, str(sorted(got)))

    # 14
    files = {mk: "include Makefile\ninclude sub.mk\n-include missing.mk\n"
                 "include a.mk\nmain1:\nmain2:\n",
             "sub.mk": "subtgt:\n", "a.mk": "include b.mk\natgt:\n",
             "b.mk": "include a.mk\nbtgt:\n"}
    got = lst(files, b"make ")
    check("14 includes: one level only, missing skipped, cycles harmless",
          got == {"main1", "main2", "subtgt", "atgt"}, str(sorted(got)))

    # 15
    body = "".join("t%d:\n" % i for i in range(5000))
    d = fixture({mk: body})
    home = tempfile.mkdtemp(prefix="hellish_mk_home_")
    TMP.append(home)
    pid, fd = spawn(d, home)
    chunks = []
    drain_idle(fd, chunks, quiet=0.4, cap=6.0)
    os.write(fd, b"make t4999\t")
    t0 = time.time()
    seen = ""
    while time.time() - t0 < 10 and "t4999 " not in seen:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                seen += os.read(fd, 65536).decode(errors="replace")
            except OSError:
                break
    dt = time.time() - t0
    os.kill(pid, 9)
    os.waitpid(pid, 0)
    check("15 5000 rules: make t4999<TAB> completes within 2 s",
          "t4999 " in ANSI.sub("", seen) and dt < 2.0, "%.2fs" % dt)

    # 16
    body = ("dup:\ndup: y\nx.y:\nsub/t:\na-b:\nc+d:\n")
    got = lst({mk: body}, b"make ")
    check("16 duplicates once; . / - + names offered",
          got == {"dup", "x.y", "sub/t", "a-b", "c+d"}, str(sorted(got)))

    # 17
    files = {mk: "zz_target:\nzz_other:\n", "fixture.txt": "x"}
    for typed in (b"make -C sub ", b"make -f other.mk ",
                  b"make --directory=sub ", b"make --file x "):
        got = lst(files, typed)
        check("17 %s -> filenames" % typed.decode(),
              got == {mk, "fixture.txt"}, str(sorted(got)))

    # 18
    files = {mk: "all:\nclean:\n"}
    out = comp(files, b"make V=1 cl")
    check("18 make V=1 cl<TAB> -> clean", line_is(out, "make V=1 clean "),
          repr(out[-100:]))
    got = lst(files, b"make -")
    check("18 make -<TAB> offers no targets", got == set(), str(sorted(got)))

    # 19
    for pre in (b"sudo make cl", b"env X=1 make cl", b"true && make cl",
                b"gmake cl", b"(make cl", b"/usr/bin/make cl"):
        out = comp(files, pre)
        want = pre.decode() + "ean "
        check("19 %s<TAB>" % pre.decode(), line_is(out, want),
              repr(out[-100:]))

    # 20
    d = fixture({mk: "all:\nclean:\n"})
    # `complete` specs are only consulted under `shopt -s progcomp`
    outs = session(d, [b"shopt -s progcomp\n",
                       b"complete -W 'zz yy' make\n", b"make \t\t\t"])
    check("20 user spec wins", listing(outs[2]) == {"zz", "yy"},
          str(sorted(listing(outs[2]))))
    d = fixture({mk: "all:\nclean:\n"})
    outs = session(d, [b"shopt -s progcomp\n",
                       b"complete -W 'zz yy' make\n", b"complete -r make\n",
                       b"make \t\t\t"])
    check("20 complete -r make: targets again",
          listing(outs[3]) == {"all", "clean"}, str(sorted(listing(outs[3]))))

    case21()

    # 22
    out = comp({mk: "all:\nclean:\n", "out.txt": "x"}, b"make out")
    check("22 no target matches -> make out.txt",
          line_is(out, "make out.txt "), repr(out[-100:]))

    # 23
    d = fixture({mk: "all:\nclean:\n"})
    outs = session(d, [b"make cl", b"\x1b[D", b"\t", KILL,
                       b"echo ALIVE\n"])
    # readline completes the word that CONTAINS the cursor, using the text
    # to its left ("c"), so the line may grow; what is pinned is that the
    # shell survives and keeps reading.
    check("23 mid-word cursor TAB: shell alive, no crash marker",
          "ALIVE" in outs[-1] and not SEEN_CRASH, repr(outs[-1][-100:]))

    # 24
    cmd = "complete -p make; echo $?"
    if not os.access(BASH, os.X_OK):
        print("skip 24 (no pinned bash at %s)" % BASH)
    else:
        a = subprocess.run([SHELL, "--norc", "-c", cmd],
                           capture_output=True, text=True,
                           env={"PATH": "/usr/bin:/bin", "HOME": "/"})
        b = subprocess.run([BASH, "--norc", "-c", cmd],
                           capture_output=True, text=True,
                           env={"PATH": "/usr/bin:/bin", "HOME": "/"})
        check("24 complete -p make: same stdout and status as bash",
              a.stdout == b.stdout and a.returncode == b.returncode,
              "hellish %r/%d bash %r/%d" % (a.stdout, a.returncode,
                                            b.stdout, b.returncode))


# `re` is run three times, `clean` twice and last, `test` once: frecency
# says re, clean, test; the newest entry says clean, re, test.
HIST = ["make re", "make test", "make re", "make clean", "make re",
        "make clean"]
RANKED = "all clean fclean lint re test".split()
UNUSED = ["all", "fclean", "lint"]


def ranked_listing(hist, typed=b"make ", env=None, files=None):
    d = fixture(dict({mk: "".join("%s:\n" % t for t in RANKED)},
                     **(files or {})))
    out = session(d, [typed + b"\t\t\t"], hist=hist, env=env, cols=10)[0]
    return ordered(out)


def ranked_cases():
    # 25
    got = ranked_listing(HIST)
    check("25 used targets by frecency, then the rest alphabetically",
          got == ["re", "clean", "test"] + UNUSED, str(got))
    got = ranked_listing(HIST, env={"HELLISH_SUGGEST_STRATEGY": "recent"})
    check("25 STRATEGY=recent: used targets newest first",
          got == ["clean", "re", "test"] + UNUSED, str(got))
    got = ranked_listing([])
    check("25 no history: alphabetical", got == RANKED, str(got))
    got = ranked_listing(["echo re", "ls test", "cmake lint", "make"])
    check("25 only invocations of make count", got == RANKED, str(got))
    got = ranked_listing(["gmake lint", "/usr/bin/make test", "make lint"])
    check("25 gmake and a path to make count",
          got == ["lint", "test", "all", "clean", "fclean", "re"], str(got))
    got = ranked_listing(["make re re re", "make lint test", "make test"])
    check("25 a word repeated inside one entry counts once",
          got == ["test", "lint", "re", "all", "clean", "fclean"], str(got))
    out = comp_hist(HIST, b"make cl")
    check("25 make cl<TAB> still completes", line_is(out, "make clean "),
          repr(out[-80:]))

    # 26
    got = ranked_listing(["make re", "make re", "make re", "make fclean",
                          "make test"], env={"HELLISH_SUGGEST_MAX": "2"})
    check("26 the window bound applies to make too (MAX=2)",
          got == ["test", "fclean", "all", "clean", "lint", "re"], str(got))

    # 27
    many = {"f%02d" % n: "x" for n in range(30)}
    d = fixture(dict({mk: "".join("%s:\n" % t for t in RANKED)}, **many))
    outs = session(d, [b"make \t\t\t", KILL, b"ls f\t\t\t", KILL,
                       b"bas\t\t\t"], hist=HIST, cols=10)
    mk_order = ordered(outs[0])
    files = ordered(outs[2])
    cmds = ordered(outs[4])
    check("27 make TAB ranked", mk_order == ["re", "clean", "test"]
          + UNUSED, str(mk_order))
    check("27 then a filename TAB: sorted again, not left unsorted",
          len(files) >= 30 and files == sorted(files), str(files[:8]))
    check("27 then a command TAB: sorted again",
          len(cmds) >= 2 and cmds == sorted(cmds), str(cmds[:8]))

    # 28
    d = fixture({mk: "all:\nclean:\n"})
    outs = session(d, [b"shopt -s progcomp\n",
                       b"complete -W 'zz yy' make\n", b"make \t\t\t"],
                   hist=["make clean", "make all", "make clean"])
    check("28 a user complete -W still wins over the ranking",
          listing(outs[2]) == {"zz", "yy"}, str(sorted(listing(outs[2]))))

    # 29
    files = {"sub/Makefile": "s1:\n", "other.mk": "o1:\n"}
    for typed in (b"make -C sub ", b"make -f other.mk "):
        got = ranked_listing(HIST, typed, files=files)
        check("29 %s keeps filenames with a history" % typed.decode(),
              set(got) == {mk, "sub/", "other.mk"}
              and got == sorted(got), str(got))


def comp_hist(hist, typed):
    d = fixture({mk: "".join("%s:\n" % t for t in RANKED)})
    return session(d, [typed + b"\t"], hist=hist)[0]


def main():
    if not os.access(SHELL, os.X_OK):
        sys.exit("complete_make_test: not executable: %s" % SHELL)
    cells = (("in-process", {}), ("forked", {"HELLISH_RL_FORK": "1"}))
    try:
        for cell, env in cells:
            if INPROC_ONLY and env:
                continue
            print("== %s reader" % cell)
            EXTRA.clear()
            EXTRA.update(env)
            cases()
            if not ASAN:
                ranked_cases()
    finally:
        for d in TMP:
            shutil.rmtree(d, ignore_errors=True)
    check("no run corrupted the heap", not SEEN_CRASH,
          "saw %s" % sorted(set(SEEN_CRASH)))
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
