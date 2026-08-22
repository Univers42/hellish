#!/usr/bin/env python3
"""Issue #32: every multi-line construct, in both history modes, in a pty.

`hist_multiline_test.py` covers the shapes the cmdhist joiner was written
for. This file is the *matrix*: every way a command can span lines, driven
through a real terminal and compared against the pinned bash 5.3.9 running
the same keystrokes, in BOTH modes.

The two modes are different contracts and both matter:

  default (cmdhist)   the entry is ONE line. Boundary newlines become "; ",
                      or a plain space where a ";" would be a syntax error.
                      This is bash's default and hellish's.
  shopt -s lithist    the entry keeps its newlines exactly as typed.

The thing actually under test is not the text — it is that **the recalled
entry still means what the user typed**. A join that produces
`f(); { echo hi; }` looks close to right and is a syntax error: the
function is never defined. That is the failure this file exists to catch,
and it is why the assertions compare against a live bash rather than
against a table someone wrote by hand.

Continuation shapes covered: backslash-newline (at a word boundary and
mid-word), if/then/elif/else/fi, for, while, until, case/esac, function
definitions, brace groups, subshells, unterminated ' " and `, $( ),
$(( )), here-docs (plain, <<-, quoted tag), and a trailing |, && or ||.

Usage: python3 history_multiline_matrix.py /path/to/hellish [bash]
"""
import os
import pty
import re
import select
import signal
import sys
import tempfile
import time

SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                        else "../build/bin/hellish")
ORACLE = sys.argv[2] if len(sys.argv) > 2 else os.path.expanduser(
    "~/bash-5.3.9/bin/bash")
FAILS = []

# name, keystrokes. Every one of these is a command whose text spans more
# than one input line, for a different structural reason.
CASES = [
    ("backslash continuation",  "echo one \\\nrest\n"),
    ("backslash mid-word",      "echo ab\\\ncd\n"),
    ("if/then/fi",              "if true\nthen\necho yes\nfi\n"),
    ("if with inline then",     "if true; then\necho y\nfi\n"),
    ("if/elif/else",            "if false\nthen\necho a\nelif true\n"
                                "then\necho b\nelse\necho c\nfi\n"),
    ("for/do/done",             "for i in 1 2\ndo\necho $i\ndone\n"),
    ("for with inline do",      "for i in 1 2; do\necho $i\ndone\n"),
    ("while/do/done",           "while false\ndo\necho x\ndone\n"),
    ("until/do/done",           "until true\ndo\necho x\ndone\n"),
    ("case/esac",               "case a in\na)\necho m\n;;\n*)\n"
                                "echo n\n;;\nesac\n"),
    ("function definition",     "f()\n{\necho hi\n}\n"),
    ("brace group",             "{\necho g\n}\n"),
    ("subshell",                "(\necho s\n)\n"),
    ("double-quoted newline",   "echo \"a\nb\"\n"),
    ("single-quoted newline",   "echo 'a\nb'\n"),
    ("backtick newline",        "echo `echo\nhi`\n"),
    ("$( ) newline",            "echo $(echo\nhi)\n"),
    ("$(( )) newline",          "echo $((\n1 +\n2 ))\n"),
    ("here-doc",                "cat <<EOF\nl1\nl2\nEOF\n"),
    ("here-doc <<-",            "cat <<-END\n\tl1\nEND\n"),
    ("here-doc quoted tag",     "cat <<'Q'\nraw $x\nQ\n"),
    ("trailing pipe",           "echo a |\ncat\n"),
    ("trailing &&",             "true &&\necho ok\n"),
    ("trailing ||",             "false ||\necho ok\n"),
    ("nested if inside for",    "for i in 1\ndo\nif true\nthen\n"
                                "echo $i\nfi\ndone\n"),
    ("case with a pipeline",    "case a in\na)\necho x |\ncat\n;;\nesac\n"),
]

NOISE = ("history", "history 1", "history 2", "shopt -s lithist")


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + (" " + detail if not ok
                                                 else ""))
    if not ok:
        FAILS.append(name)


def plain(b):
    return re.sub(rb"\x1b\[[0-9;?]*[a-zA-Z]", b"", b).decode(errors="replace")


def drive(shell, keys, lithist, ask):
    """Type `keys` at a fresh prompt, then `ask`; return the screen."""
    home = tempfile.mkdtemp(prefix="hellish_ml32_")
    os.makedirs(os.path.join(home, ".cache", "hellish"), exist_ok=True)
    open(os.path.join(home, ".cache", "hellish", "seen"), "w").close()
    env = {
        "HOME": home, "PATH": "/usr/bin:/bin", "TERM": "xterm-256color",
        "LANG": "C.UTF-8", "PS1": "$ ", "PS2": "> ", "INPUTRC": "/dev/null",
        "HELLISH_NO_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
        "HELLISH_NO_ANIM": "1", "ASAN_OPTIONS": "detect_leaks=0",
        # bash needs this set or `history` is inert; hellish uses its own file
        "HISTFILE": os.path.join(home, ".bash_history"),
    }
    pid, fd = pty.fork()
    if pid == 0:
        os.environ.clear()
        os.environ.update(env)
        os.execvp(shell, [shell])
        os._exit(127)

    def read(t):
        out = b""
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.06)
            if r:
                try:
                    c = os.read(fd, 65536)
                except OSError:
                    break
                if not c:
                    break
                out += c
        return out

    read(0.9)
    out = b""
    try:
        if lithist:
            os.write(fd, b"shopt -s lithist\n")
            read(0.4)
        os.write(fd, keys.encode())
        read(0.5)
        os.write(fd, ask)
        out = read(0.9)
    except OSError:
        pass
    try:
        os.kill(pid, signal.SIGKILL)
        os.waitpid(pid, 0)
    except OSError:
        pass
    return plain(out).replace("\r\n", "\n").replace("\r", "")


def newest_entry(screen):
    """Text of the newest real entry in a `history` listing.

    Shell-agnostic on purpose: bash records a line BEFORE running it and
    hellish after, so the `history` invocation itself is in one listing and
    not the other. Parse every entry, drop the bookkeeping ones, take the
    last. Continuation rows of a lithist entry carry no number, so they are
    appended to the entry above them.
    """
    lines = screen.split("\n")
    heads = [i for i, l in enumerate(lines) if re.match(r"^\s*\d+\s\s", l)]
    if not heads:
        return None
    entries = []
    for j, i in enumerate(heads):
        body = [re.sub(r"^\s*\d+\s\s", "", lines[i])]
        end = heads[j + 1] if j + 1 < len(heads) else len(lines)
        for l in lines[i + 1:end]:
            if l.startswith("$ ") or l.startswith("> "):
                break
            body.append(l)
        while body and body[-1] == "":
            body.pop()
        entries.append("\n".join(body))
    entries = [e for e in entries if e.strip() not in NOISE]
    return entries[-1] if entries else None


def recalled(screen):
    """What the up-arrow put back on the command line."""
    at = screen.rfind("$ ")
    return screen[at + 2:] if at >= 0 else screen


def compare(mode, lithist):
    print("\n--- %s ---" % mode)
    for name, keys in CASES:
        want = newest_entry(drive(ORACLE, keys, lithist, b"history\n"))
        got = newest_entry(drive(SHELL, keys, lithist, b"history\n"))
        if want is None:
            check("%s: %s [oracle produced nothing]" % (mode, name), False)
            continue
        check("%s: %s" % (mode, name), want == got,
              "\n        bash    %r\n        hellish %r" % (want, got))


def compare_recall():
    """The entry has to come BACK the way it went in.

    Listing and recall are two different code paths in hellish (the
    `history` builtin reads hist_cmds; the arrow keys read readline's own
    list), and issue #32 is a complaint about the second one.
    """
    print("\n--- lithist recall (up-arrow) ---")
    for name, keys in CASES:
        want = recalled(drive(ORACLE, keys, True, b"\x1b[A"))
        got = recalled(drive(SHELL, keys, True, b"\x1b[A"))
        check("recall: %s" % name, want.strip() == got.strip(),
              "\n        bash    %r\n        hellish %r"
              % (want[:120], got[:120]))


def main():
    if not os.path.exists(ORACLE):
        print("oracle %s not found -- run `make oracle`" % ORACLE)
        sys.exit(2)
    compare("cmdhist", False)
    compare("lithist", True)
    compare_recall()
    print("\n%d checks failed" % len(FAILS))
    sys.exit(1 if FAILS else 0)


main()
