#!/usr/bin/env python3
"""Render the cross-shell geomean ranking (make agnostic-bench) into an SVG in
the same house style as gen_charts.py -- GitHub palette, embedded
prefers-color-scheme stylesheet, hellish highlighted. Reads the plain-text
matrix artifact (bench/.artifacts/agnostic-matrix.txt) so the chart never
drifts from the numbers actually raced. Output: bench/charts/cross-shell.svg."""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
BENCH = os.path.dirname(HERE)
SRC = os.path.join(BENCH, '.artifacts', 'agnostic-matrix.txt')
OUT = os.path.join(BENCH, 'charts', 'cross-shell.svg')

COLORS = {
    'hellish': '#e2543a', 'bash': '#5b7fa6', 'dash': '#6f9e78',
    'zsh': '#9a76b8', 'mksh': '#b8925c', 'ksh': '#7f8fa0',
    'yash': '#8aa05c', 'busybox': '#a0708a', 'fish': '#5f9ea0',
}
STYLE = """
  .bg{fill:#ffffff}.card{fill:#f6f8fa;stroke:#d0d7de}
  .title{fill:#1f2328;font:600 17px system-ui,-apple-system,Segoe UI,sans-serif}
  .sub{fill:#59636e;font:400 12px system-ui,-apple-system,Segoe UI,sans-serif}
  .lbl{fill:#1f2328;font:600 12.5px system-ui,-apple-system,Segoe UI,sans-serif}
  .val{fill:#1f2328;font:600 11px ui-monospace,SFMono-Regular,Menlo,monospace}
  .muted{fill:#59636e;font:400 10.5px system-ui,-apple-system,sans-serif}
  .us{fill:#e2543a;font:700 12.5px system-ui,-apple-system,Segoe UI,sans-serif}
  .axis{stroke:#8c959f;stroke-width:1}
  @media (prefers-color-scheme:dark){
   .bg{fill:#0d1117}.card{fill:#161b22;stroke:#30363d}
   .title{fill:#e6edf3}.sub{fill:#8b949e}.lbl{fill:#e6edf3}.val{fill:#e6edf3}
   .muted{fill:#8b949e}.axis{stroke:#6e7681}}
  :root[data-theme=dark] .bg{fill:#0d1117}
  :root[data-theme=dark] .card{fill:#161b22;stroke:#30363d}
  :root[data-theme=dark] .title{fill:#e6edf3}:root[data-theme=dark] .sub{fill:#8b949e}
  :root[data-theme=dark] .lbl{fill:#e6edf3}:root[data-theme=dark] .val{fill:#e6edf3}
  :root[data-theme=dark] .muted{fill:#8b949e}
  :root[data-theme=light] .bg{fill:#ffffff}:root[data-theme=light] .card{fill:#f6f8fa;stroke:#d0d7de}
  :root[data-theme=light] .title{fill:#1f2328}:root[data-theme=light] .lbl{fill:#1f2328}
"""


def parse():
    rows, n_wl, ref = [], '?', 'bash'
    for ln in open(SRC):
        m = re.search(r'across matching workloads', ln)
        r = re.match(r'\s*(\d+)\s+(\w+)\s+(\d+)\s+([\d.]+)ms\s+([\d.]+)ms\s+([\d.]+)x', ln)
        if r:
            rows.append({'rank': int(r[1]), 'shell': r[2], 'n': int(r[3]),
                         'geo': float(r[4]), 'wall': float(r[5]), 'ratio': float(r[6])})
        mo = re.search(r'oracle=(\w+)', ln)
        if mo:
            ref = mo[1]
    return rows, ref


def esc(s):
    return s.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')


def main():
    if not os.path.exists(SRC):
        print('skip cross-shell chart: %s absent (run make agnostic-bench)' % SRC)
        return 0
    rows, ref = parse()
    if not rows:
        print('no ranking data in %s' % SRC, file=sys.stderr)
        return 0
    rows.sort(key=lambda r: r['rank'])
    W, PAD_L, PAD_R, TOP, ROW = 880, 150, 118, 92, 34
    H = TOP + ROW * len(rows) + 40
    maxgeo = max(r['geo'] for r in rows)
    x0, x1 = PAD_L, W - PAD_R
    s = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" '
         'viewBox="0 0 %d %d" font-family="system-ui">' % (W, H, W, H)]
    s.append('<style>%s</style>' % STYLE)
    s.append('<rect class="bg" width="%d" height="%d"/>' % (W, H))
    s.append('<rect class="card" x="8" y="8" width="%d" height="%d" rx="10"/>' % (W - 16, H - 16))
    s.append('<text class="title" x="26" y="40">Cross-shell speed ranking '
             '&#8212; 9 shells, one Docker image</text>')
    s.append('<text class="sub" x="26" y="62">Geometric mean of best-of-7 across '
             '17 portable POSIX workloads. Lower is faster; bar = geomean, '
             'label = ratio vs hellish.</text>')
    for r in rows:
        y = TOP + ROW * (r['rank'] - 1)
        cy = y + ROW / 2
        us = r['shell'] == 'hellish'
        bw = (r['geo'] / maxgeo) * (x1 - x0)
        col = COLORS.get(r['shell'], '#8b8b8b')
        s.append('<text class="%s" x="%d" y="%d" text-anchor="end">#%d %s%s</text>'
                 % ('us' if us else 'lbl', x0 - 12, cy + 4, r['rank'],
                    esc(r['shell']), ' &#9666;' if us else ''))
        s.append('<rect x="%d" y="%d" width="%.1f" height="16" rx="3" fill="%s" '
                 'opacity="%s"/>' % (x0, cy - 8, max(bw, 2), col, '1' if us else '.82'))
        s.append('<text class="val" x="%.1f" y="%d">%.1f ms</text>'
                 % (x0 + max(bw, 2) + 8, cy + 4, r['geo']))
        s.append('<text class="muted" x="%d" y="%d" text-anchor="end">%.2f&#215;'
                 '%s</text>' % (x1 + PAD_R - 26, cy + 4, r['ratio'],
                                '' if us else (' slower' if r['ratio'] > 1 else ' faster')))
    s.append('<line class="axis" x1="%d" y1="%d" x2="%d" y2="%d"/>'
             % (x0, TOP - 6, x0, TOP + ROW * len(rows) - 6))
    s.append('<text class="muted" x="26" y="%d">oracle=%s &#183; ksh\'s recursion/'
             'cmdsub rows are builtin-accelerated outliers, not comparable work'
             '</text>' % (H - 20, esc(ref)))
    s.append('</svg>')
    open(OUT, 'w').write('\n'.join(s))
    print('wrote %s (%d shells)' % (OUT, len(rows)))
    workloads_chart()
    return 0



# --- second chart: hellish vs bash vs dash on representative workloads ------
def parse_workloads():
    cols, rows = None, []
    for ln in open(SRC):
        p = ln.split()
        if p[:1] == ['workload']:
            cols = p
        elif cols and len(p) >= len(cols) and re.match(r'[a-z]', ln) \
                and not ln.startswith('geomean') and 'shells:' not in ln:
            name = ' '.join(p[:len(p) - (len(cols) - 1)])
            vals = p[-(len(cols) - 1):]
            d = {}
            for c, v in zip(cols[1:], vals):
                try:
                    d[c] = float(v)
                except ValueError:
                    d[c] = None
            rows.append((name, d))
    return rows


def workloads_chart():
    keep = ['while-arith 50k', 'func-call 5k', 'var-concat 30k',
            'concat-grow 4k', 'cmdsub 3k', 'param-expand 20k',
            'case-loop 20k', 'test-string 20k', 'arith-mix 20k']
    all_rows = {n: d for n, d in parse_workloads()}
    rows = [(n, all_rows[n]) for n in keep if n in all_rows]
    if not rows:
        return
    shells = ['hellish', 'bash', 'dash']
    W, PAD_L, PAD_R, TOP, GRP = 880, 168, 30, 92, 60
    H = TOP + GRP * len(rows) + 44
    x0, x1 = PAD_L, W - PAD_R - 60
    s = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" '
         'viewBox="0 0 %d %d" font-family="system-ui">' % (W, H, W, H)]
    s.append('<style>%s</style>' % STYLE)
    s.append('<rect class="bg" width="%d" height="%d"/>' % (W, H))
    s.append('<rect class="card" x="8" y="8" width="%d" height="%d" rx="10"/>' % (W - 16, H - 16))
    s.append('<text class="title" x="26" y="40">Per-workload: hellish vs bash vs dash</text>')
    s.append('<text class="sub" x="26" y="62">Best-of-7 ms, lower is faster. '
             'Legend: <tspan fill="#e2543a">hellish</tspan> '
             '<tspan fill="#5b7fa6">bash</tspan> '
             '<tspan fill="#6f9e78">dash</tspan></text>')
    for i, (name, d) in enumerate(rows):
        y = TOP + GRP * i
        vals = [d.get(sh) for sh in shells]
        mx = max(v for v in vals if v) or 1
        s.append('<text class="lbl" x="%d" y="%d" text-anchor="end">%s</text>'
                 % (x0 - 12, y + 22, esc(name)))
        for j, sh in enumerate(shells):
            v = d.get(sh)
            by = y + 6 + j * 15
            bw = (v / mx) * (x1 - x0) if v else 2
            s.append('<rect x="%d" y="%d" width="%.1f" height="12" rx="2" '
                     'fill="%s"/>' % (x0, by, max(bw, 2), COLORS[sh]))
            s.append('<text class="val" x="%.1f" y="%d">%s</text>'
                     % (x0 + max(bw, 2) + 6, by + 10,
                        ('%.0f' % v) if v else '--'))
    s.append('</svg>')
    open(os.path.join(BENCH, 'charts', 'cross-shell-workloads.svg'), 'w').write('\n'.join(s))
    print('wrote cross-shell-workloads.svg (%d workloads)' % len(rows))

if __name__ == '__main__':
    sys.exit(main())
