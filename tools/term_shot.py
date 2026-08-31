#!/usr/bin/env python3
"""Render a captured terminal session (ANSI colors and all) into an SVG
"screenshot" for the docs.

    script -qec "cmd" /dev/null | ...   # capture with colors
    python3 tools/term_shot.py --title 'hxp list' -o out.svg < capture.txt

Why SVG and not a PNG of a terminal window: the capture is REAL output from
the real shell (so a doc screenshot can be regenerated the day the output
changes), it diffs like text, weighs kilobytes, and renders crisp at every
zoom on the docs site. The SGR subset understood is what the shell and the
plugin framework actually emit: reset, bold, dim, 16-color, 256-color and
truecolor foregrounds.
"""
import argparse
import html
import re
import sys

BASIC = {30: "#3f4451", 31: "#e05561", 32: "#8cc265", 33: "#d18f52",
         34: "#4aa5f0", 35: "#c162de", 36: "#42b3c2", 37: "#d7dae0",
         90: "#72767d", 91: "#ff616e", 92: "#a5e075", 93: "#f0a45d",
         94: "#4dc4ff", 95: "#de73ff", 96: "#4cd1e0", 97: "#ffffff"}
DEFAULT_FG = "#d7dae0"
BG = "#1e2127"


def c256(n):
    if n < 16:
        return BASIC.get(30 + n if n < 8 else 90 + n - 8, DEFAULT_FG)
    if n >= 232:
        v = 8 + (n - 232) * 10
        return "#%02x%02x%02x" % (v, v, v)
    n -= 16
    r, g, b = n // 36, (n // 6) % 6, n % 6
    lv = [0, 95, 135, 175, 215, 255]
    return "#%02x%02x%02x" % (lv[r], lv[g], lv[b])


SGR = re.compile(r"\x1b\[([0-9;]*)m")
DROP = re.compile(r"\x1b\][^\x07]*\x07|\x1b\[[0-9;?]*[A-LN-Za-ln-z]|\r")


def parse(text):
    """-> list of lines, each a list of (fg, bold, dim, text) runs."""
    lines = []
    for raw in DROP.sub("", text).split("\n"):
        runs, fg, bold, dim = [], DEFAULT_FG, False, False
        pos = 0
        for m in SGR.finditer(raw):
            if m.start() > pos:
                runs.append((fg, bold, dim, raw[pos:m.start()]))
            pos = m.end()
            p = [int(x) if x else 0 for x in (m.group(1) or "0").split(";")]
            i = 0
            while i < len(p):
                c = p[i]
                if c == 0:
                    fg, bold, dim = DEFAULT_FG, False, False
                elif c == 1:
                    bold = True
                elif c == 2:
                    dim = True
                elif c in (22, 39):
                    fg, bold, dim = (fg, False, False) if c == 22 \
                        else (DEFAULT_FG, bold, dim)
                elif c in BASIC:
                    fg = BASIC[c]
                elif c == 38 and i + 2 < len(p) and p[i + 1] == 5:
                    fg = c256(p[i + 2])
                    i += 2
                elif c == 38 and i + 4 < len(p) and p[i + 1] == 2:
                    fg = "#%02x%02x%02x" % (p[i + 2], p[i + 3], p[i + 4])
                    i += 4
                i += 1
        if pos < len(raw):
            runs.append((fg, bold, dim, raw[pos:]))
        lines.append(runs)
    while lines and not any(t.strip() for *_, t in lines[-1]):
        lines.pop()
    return lines


def render(lines, title):
    ch_w, ln_h, pad, top = 8.4, 19, 14, 40
    cols = max((sum(len(t) for *_, t in ln) for ln in lines), default=10)
    w = int(pad * 2 + max(cols, len(title) + 10) * ch_w)
    h = int(top + len(lines) * ln_h + pad)
    out = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" '
           'font-family="SFMono-Regular,Menlo,Consolas,monospace" '
           'font-size="14">' % (w, h),
           '<rect width="100%%" height="100%%" rx="8" fill="%s"/>' % BG,
           '<circle cx="22" cy="20" r="6" fill="#e05561"/>'
           '<circle cx="42" cy="20" r="6" fill="#d18f52"/>'
           '<circle cx="62" cy="20" r="6" fill="#8cc265"/>',
           '<text x="50%%" y="25" text-anchor="middle" fill="#72767d">'
           '%s</text>' % html.escape(title)]
    y = top + 14
    for ln in lines:
        x = pad
        parts = []
        for fg, bold, dim, t in ln:
            if not t:
                continue
            style = ' font-weight="bold"' if bold else ""
            if dim:
                style += ' opacity="0.6"'
            parts.append('<tspan x="%.1f" fill="%s"%s>%s</tspan>'
                         % (x, fg, style, html.escape(t)))
            x += len(t) * ch_w
        out.append('<text y="%d" xml:space="preserve">%s</text>'
                   % (y, "".join(parts)))
        y += ln_h
    out.append("</svg>")
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--title", default="hellish")
    ap.add_argument("-o", required=True)
    a = ap.parse_args()
    text = sys.stdin.buffer.read().decode("utf-8", "replace")
    with open(a.o, "w") as f:
        f.write(render(parse(text), a.title))
    print("wrote %s" % a.o)


main()
