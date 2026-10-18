#!/usr/bin/env python3
"""The welcome banner's "What's new" block reads whole, and its link lands.

The block is there to be read before the first prompt, so a clipped row is a
defect. At 80 columns, the narrowest terminal the panel is laid out for, the
right column has 32 cells; a row longer than that is cut with an ellipsis
and loses its meaning. The 3.1 rows were written for about 40 and were cut.
Under an ASCII terminal (LANG=C) every framed row must still be exactly the
terminal's width: a UTF-8 byte there cannot be measured, and the identity
line's middle dot pushed the frame out of line.

The banner links to wiki/whats-new.md. That page must exist and be in the
docs site's nav (mkdocs.yml), or the printed URL is a 404, and it must name
every key the banner shows, so the two cannot drift apart.

Usage: python3 banner_news_test.py /path/to/hellish
"""
import fcntl
import os
import pty
import re
import select
import struct
import sys
import tempfile
import termios
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHELL = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else
                        os.path.join(ROOT, "build", "bin", "hellish"))
FAILS = []
ESC = re.compile(r"\x1b\[[0-9;?]*[ -/]*[@-~]|\x1b\][^\x07\x1b]*(\x07|\x1b\\)")
LINK = "univers42.github.io/hellish/whats-new"


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name)
    if not ok:
        FAILS.append(name)
        print("     " + detail)


def banner(cols, lang):
    """The banner as text rows, drawn on a `cols`-wide pty in a fresh HOME
    (so the cwd row is a short `~`)."""
    home = tempfile.mkdtemp(prefix="banner_news_")
    env = {"HOME": home, "PATH": os.environ["PATH"], "TERM": "xterm-256color",
           "LANG": lang, "LC_ALL": lang, "XDG_CACHE_HOME": home,
           "HELLISH_BANNER": "1", "HELLISH_NO_UPDATE_CHECK": "1",
           "ASAN_OPTIONS": "detect_leaks=0"}
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(home)
        os.environ.clear()
        os.environ.update(env)
        os.execv(SHELL, [SHELL])
        os._exit(127)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 50, cols, 0, 0))
    out, quiet, end = b"", time.time() + 1.0, time.time() + 15
    while time.time() < min(quiet, end):
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                out += os.read(fd, 65536)
            except OSError:
                break
            quiet = time.time() + 1.0
    os.kill(pid, 9)
    os.waitpid(pid, 0)
    text = ESC.sub("", out.decode("utf-8", errors="replace"))
    return [ln.strip("\r") for ln in text.split("\n")]


def news_cells(rows, bar):
    """The right-hand cells from the "What's new" heading to the next rule."""
    cells, inside = [], False
    for row in rows:
        parts = row.split(bar)
        if len(parts) != 4:
            continue
        cell = parts[2]
        if "What's new in" in cell:
            inside = True
        elif inside and set(cell.strip()) <= set("─-"):
            if cell.strip():
                break
        if inside:
            cells.append(cell)
    return cells


def panel(name, cols, lang, bar, clip):
    rows = banner(cols, lang)
    framed = [r for r in rows if r.startswith(bar) and r.endswith(bar)]
    check(name + "/banner-drawn", len(framed) > 10, repr(rows[:6]))
    bad = [r for r in framed if len(r) != cols]
    check(name + "/every-row-is-%d-wide" % cols, not bad, repr(bad[:2]))
    cells = news_cells(rows, bar)
    check(name + "/news-block-found", len(cells) > 5, repr(cells))
    cut = [c for c in cells if c.rstrip().endswith(clip)]
    check(name + "/no-news-row-clipped", not cut, repr(cut))
    check(name + "/guide-link-whole", any(LINK in r for r in framed),
          repr([r for r in framed if "univers42" in r]))
    return cells


def doc(cells):
    path = os.path.join(ROOT, "wiki", "whats-new.md")
    check("doc/whats-new-exists", os.path.isfile(path), path)
    if not os.path.isfile(path):
        return
    with open(path, encoding="utf-8") as f:
        page = f.read()
    with open(os.path.join(ROOT, "mkdocs.yml"), encoding="utf-8") as f:
        nav = f.read()
    check("doc/in-the-site-nav", re.search(r":\s*whats-new\.md\s*$", nav,
                                           re.M) is not None)
    # A key row is "  KEY10     what": one cell margin, two of indent, then
    # the key padded to ten columns. The switch row is a command instead.
    for c in cells:
        text = c[3:].rstrip()
        if not text or "What's new in" in c:
            continue
        key = text if text.startswith("export ") else c[3:13].rstrip()
        check("doc/names " + key, key in page, "not in " + path)


def main():
    cells = panel("utf8-80", 80, "C.UTF-8", "│", "…")
    panel("utf8-120", 120, "C.UTF-8", "│", "…")
    panel("ascii-80", 80, "C", "|", "~")
    doc(cells)
    print("%d failed" % len(FAILS))
    return 1 if FAILS else 0


sys.exit(main())
