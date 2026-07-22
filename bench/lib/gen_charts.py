#!/usr/bin/env python3
"""Render bench/.artifacts/dashboard.json into the SVG charts the README shows.

SVG, not PNG, on purpose: no matplotlib/numpy dependency to install on a dev
box or in CI, files are ~4KB instead of ~100KB, they stay sharp on a HiDPI
screen, and a re-render produces a readable diff instead of an opaque binary
blob.  Each chart carries an embedded prefers-color-scheme stylesheet so it
reads correctly against GitHub's light and dark themes from one file.

Honesty rules baked into the rendering, not left to the caller:
  * a benchmark whose worst CV exceeds the limit is drawn hatched and flagged,
    never silently averaged in with clean measurements;
  * if the CPU governor was not `performance`, every chart gets a provisional
    banner, because absolute milliseconds off a throttling core are not a
    result;
  * bars are labelled with their real value, so the picture can never imply
    something the number does not say.

Usage:  python3 bench/lib/gen_charts.py   (after collect_data.py)
Output: bench/charts/*.svg
"""
import datetime
import json
import os
import sys

BENCH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(BENCH, '.artifacts')
OUT = os.path.join(BENCH, 'charts')

W = 880
PAD_L, PAD_R = 210, 116
ROW_H, BAR_H, GRP_PAD = 26, 15, 14

# hellish owns the warm brand colour; the competition gets cool neutrals so a
# glance at any chart tells you which bar is ours without reading the legend.
COLORS = {
    'hellish': '#e2543a',
    'bash':    '#5b7fa6',
    'dash':    '#6f9e78',
    'zsh':     '#9a76b8',
    'mksh':    '#b8925c',
    'ksh':     '#7f8fa0',
    'yash':    '#8aa05c',
    'busybox': '#a0708a',
    'fish':    '#5f9ea0',
}
FALLBACK = '#8b8b8b'

STYLE = """
  .bg{fill:#ffffff}
  .card{fill:#f6f8fa;stroke:#d0d7de}
  .title{fill:#1f2328;font:600 17px system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
  .sub{fill:#59636e;font:400 12px system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
  .lbl{fill:#1f2328;font:500 12px system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
  .val{fill:#1f2328;font:600 11px ui-monospace,SFMono-Regular,Menlo,monospace}
  .muted{fill:#59636e;font:400 10.5px system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
  .grid{stroke:#d8dee4;stroke-width:1}
  .axis{stroke:#8c959f;stroke-width:1}
  .warn{fill:#9a6700;font:600 11px system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
  .warnbox{fill:#fff8c5;stroke:#d4a72c}
  .win{fill:#1a7f37;font:600 11px ui-monospace,SFMono-Regular,Menlo,monospace}
  .lose{fill:#cf222e;font:600 11px ui-monospace,SFMono-Regular,Menlo,monospace}
  .tie{fill:#59636e;font:600 11px ui-monospace,SFMono-Regular,Menlo,monospace}
  @media (prefers-color-scheme: dark){
    .bg{fill:#0d1117}
    .card{fill:#161b22;stroke:#30363d}
    .title{fill:#e6edf3}
    .sub{fill:#9198a1}
    .lbl{fill:#e6edf3}
    .val{fill:#e6edf3}
    .muted{fill:#9198a1}
    .grid{stroke:#30363d}
    .axis{stroke:#6e7681}
    .warn{fill:#d29922}
    .warnbox{fill:#282215;stroke:#9e6a03}
    .win{fill:#3fb950}
    .lose{fill:#f85149}
    .tie{fill:#9198a1}
  }
"""


def esc(s):
    return (str(s).replace('&', '&amp;').replace('<', '&lt;')
            .replace('>', '&gt;').replace('"', '&quot;'))


def fmt_time(sec):
    if sec is None:
        return 'n/a'
    if sec < 1e-3:
        return '%.0f us' % (sec * 1e6)
    if sec < 1.0:
        return '%.1f ms' % (sec * 1e3)
    return '%.2f s' % sec


def fmt_mem(kib):
    if kib is None:
        return 'n/a'
    if kib < 1024:
        return '%d KB' % kib
    return '%.1f MB' % (kib / 1024.0)


def clip(s, n):
    """Truncate on a word boundary. Cutting mid-word ('under `set -n` (')
    reads as a rendering bug rather than as an abbreviation."""
    if len(s) <= n:
        return s
    cut = s[:n].rsplit(' ', 1)[0]
    return (cut or s[:n]).rstrip(' ,(') + '…'


class Svg:
    """A tiny append-only SVG builder.  Deliberately not a general chart
    library: every method here exists because one of the five README charts
    needs it, which keeps the whole file readable."""

    def __init__(self, width, height, title, subtitle):
        self.w, self.h = width, height
        self.parts = []
        self.parts.append(
            '<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" '
            'viewBox="0 0 %d %d" role="img" aria-label="%s">'
            % (width, height, width, height, esc(title)))
        self.parts.append('<style>%s</style>' % STYLE)
        self.parts.append('<rect class="bg" width="%d" height="%d" rx="8"/>'
                          % (width, height))
        self.parts.append('<text class="title" x="20" y="30">%s</text>'
                          % esc(title))
        if subtitle:
            self.parts.append('<text class="sub" x="20" y="50">%s</text>'
                              % esc(subtitle))

    def rect(self, x, y, w, h, fill, opacity=1.0, rx=2, cls=None):
        c = ' class="%s"' % cls if cls else ''
        f = ' fill="%s"' % fill if fill else ''
        self.parts.append(
            '<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" rx="%d"%s%s '
            'opacity="%.2f"/>' % (x, y, max(w, 0), h, rx, f, c, opacity))

    def text(self, x, y, s, cls='lbl', anchor='start'):
        self.parts.append(
            '<text class="%s" x="%.1f" y="%.1f" text-anchor="%s">%s</text>'
            % (cls, x, y, anchor, esc(s)))

    def line(self, x1, y1, x2, y2, cls='grid'):
        self.parts.append(
            '<line class="%s" x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f"/>'
            % (cls, x1, y1, x2, y2))

    def hatch_def(self):
        self.parts.append(
            '<defs><pattern id="hatch" width="6" height="6" '
            'patternTransform="rotate(45)" patternUnits="userSpaceOnUse">'
            '<rect width="6" height="6" fill="#000" opacity="0.001"/>'
            '<line x1="0" y1="0" x2="0" y2="6" stroke="#ffffff" '
            'stroke-opacity="0.45" stroke-width="3"/></pattern></defs>')

    def banner(self, y, msg):
        self.rect(20, y, self.w - 40, 24, None, cls='warnbox', rx=5)
        self.text(30, y + 16, msg, cls='warn')
        return y + 34

    def legend(self, y, shells):
        x = 20
        for s in shells:
            self.rect(x, y - 9, 11, 11, COLORS.get(s, FALLBACK), rx=2)
            self.text(x + 17, y, s, cls='muted')
            x += 22 + 7.2 * len(s)
        return y

    def footer(self, meta, extra=''):
        bits = [meta.get('platform', '').split('-with-')[0]]
        bits.append('governor=%s' % meta.get('governor', '?'))
        if extra:
            bits.append(extra)
        self.text(20, self.h - 12, '  ·  '.join(b for b in bits if b),
                  cls='muted')

    def save(self, path):
        self.parts.append('</svg>')
        with open(path, 'w') as f:
            f.write('\n'.join(self.parts) + '\n')
        return path


def grouped_bars(path, title, subtitle, rows, meta, unit='time',
                 note='', warn_unreliable=True, per_row=False):
    """The workhorse: one group per benchmark, one bar per shell.

    `rows` = [{'title', 'values': {shell: number}, 'reliable', 'note'}].
    Values are raw (seconds or KiB); the winner in each group is the SMALLEST,
    since every quantity here is a cost.

    `per_row` rescales each group to its own maximum instead of a shared axis.
    That is required whenever the rows span orders of magnitude -- the execution
    set runs from a 21ms command substitution to a 14s ./configure, and on one
    shared axis every row but the slowest collapses to a sliver, which hides
    exactly the comparison the chart exists to show.  The trade is that bar
    LENGTH is then only meaningful within a row, so the axis is dropped and
    each row prints its own scale; cross-row comparison is what the companion
    ratio chart is for."""
    shells = []
    for r in rows:
        for s in r['values']:
            if s not in shells and r['values'][s] is not None:
                shells.append(s)
    n_bars = len(shells)
    grp_h = n_bars * (BAR_H + 3) + GRP_PAD
    top = 74
    if not meta.get('governor_ok'):
        top += 34
    height = top + len(rows) * grp_h + 58
    svg = Svg(W, height, title, subtitle)
    svg.hatch_def()

    y = 66
    if not meta.get('governor_ok'):
        y = svg.banner(y, 'PROVISIONAL — CPU governor was "%s", not "performance". '
                          'Ratios hold; absolute values are inflated.'
                       % meta.get('governor', '?'))
    svg.legend(y + 4, shells)
    y += 18

    fmt = fmt_time if unit == 'time' else fmt_mem
    gmax = max((v for r in rows for v in r['values'].values() if v), default=1)
    plot_w = W - PAD_L - PAD_R

    if not per_row:
        for gi in range(5):
            gx = PAD_L + plot_w * gi / 4.0
            svg.line(gx, y + 6, gx, height - 40)
            svg.text(gx, height - 26, fmt(gmax * gi / 4.0),
                     cls='muted', anchor='middle')

    for r in rows:
        vals = {s: v for s, v in r['values'].items() if v is not None}
        best = min(vals.values()) if vals else None
        vmax = (max(vals.values()) if vals else 1) if per_row else gmax
        svg.text(PAD_L - 10, y + grp_h / 2 + 2, r['title'], cls='lbl',
                 anchor='end')
        if r.get('blurb'):
            svg.text(PAD_L - 10, y + grp_h / 2 + 15, clip(r['blurb'], 44),
                     cls='muted', anchor='end')
        by = y + 8
        for s in shells:
            v = r['values'].get(s)
            if v is None:
                svg.text(PAD_L + 4, by + BAR_H - 3, 'n/a', cls='muted')
                by += BAR_H + 3
                continue
            bw = plot_w * (v / vmax) if vmax else 0
            svg.rect(PAD_L, by, bw, BAR_H, COLORS.get(s, FALLBACK),
                     opacity=1.0 if v == best else 0.80)
            if warn_unreliable and not r.get('reliable', True):
                svg.rect(PAD_L, by, bw, BAR_H, 'url(#hatch)')
            label = fmt_time(v) if unit == 'time' else fmt_mem(v)
            svg.text(PAD_L + bw + 7, by + BAR_H - 3, label, cls='val')
            by += BAR_H + 3
        if warn_unreliable and not r.get('reliable', True):
            svg.text(W - 22, y + grp_h / 2 + 2, 'noisy', cls='warn',
                     anchor='end')
        y += grp_h
        if r is not rows[-1]:
            svg.line(PAD_L, y - GRP_PAD / 2, W - PAD_R + 40, y - GRP_PAD / 2)

    svg.footer(meta, note)
    return svg.save(path)


def ratio_chart(path, title, subtitle, rows, meta, note=''):
    """Speedup relative to hellish on a log-symmetric axis centred at 1.0, with
    a bootstrap 95% confidence interval drawn on every bar.

    A ratio chart is the honest way to show 'how much faster', because a plain
    bar of ms lets one slow benchmark flatten every other row into invisibility.
    Bars grow right when hellish wins, left when it loses; the centre line is
    parity.

    The whisker is what makes it trustworthy. Marking rows by absolute CV was
    actively misleading: on a throttling core every shell shows 5-20% CV, so a
    CV gate stripes a 36x win and a 0.99x coin-flip identically and the reader
    concludes the whole page is noise. The CI asks the question that actually
    matters -- could this difference be zero? -- so only genuinely undecidable
    rows get flagged, and a wide-but-decisive bar reads as the win it is."""
    top = 74
    if not meta.get('governor_ok'):
        top += 34
    height = top + len(rows) * ROW_H + 62
    svg = Svg(W, height, title, subtitle)
    svg.hatch_def()

    y = 66
    if not meta.get('governor_ok'):
        y = svg.banner(y, 'PROVISIONAL — CPU governor was "%s", not "performance".'
                       % meta.get('governor', '?'))
    y += 16

    import math
    span = max((abs(math.log2(r['ratio'])) for r in rows if r['ratio'] > 0),
               default=1.0)
    span = max(span, 1.0) * 1.15
    plot_w = W - PAD_L - PAD_R
    mid = PAD_L + plot_w / 2.0

    for k in (-1.0, -0.5, 0.0, 0.5, 1.0):
        gx = mid + (plot_w / 2.0) * k
        svg.line(gx, y, gx, height - 40, 'axis' if k == 0 else 'grid')
        mult = 2 ** (k * span)
        svg.text(gx, height - 26,
                 ('%.2gx' % mult) if mult < 1 else ('%.3gx' % mult),
                 cls='muted', anchor='middle')
    svg.text(mid, y - 6, 'parity', cls='muted', anchor='middle')
    svg.text(PAD_L, y - 6, '← hellish slower', cls='muted')
    svg.text(W - PAD_R, y - 6, 'hellish faster →', cls='muted', anchor='end')

    def px(v):
        """ratio -> x, on the log-symmetric axis."""
        f = max(-1.0, min(1.0, math.log2(v) / span)) if v > 0 else 0.0
        return mid + (plot_w / 2.0) * f

    for r in rows:
        ratio = r['ratio']
        ci = r.get('ci')
        undecided = bool(ci) and not ci.get('significant', True)
        cy = y + 8
        bx = px(ratio)
        bw = bx - mid
        color = COLORS['hellish'] if ratio >= 1 else '#8b95a1'
        svg.rect(min(mid, bx), cy, abs(bw), BAR_H, color,
                 opacity=0.5 if undecided else 1.0)
        if undecided:
            svg.rect(min(mid, bx), cy, abs(bw), BAR_H, 'url(#hatch)')
        # 95% CI whisker: the honest width of the claim.
        if ci:
            lo, hi = px(ci['lo']), px(ci['hi'])
            mid_y = cy + BAR_H / 2.0
            svg.line(lo, mid_y, hi, mid_y, 'axis')
            for ex in (lo, hi):
                svg.line(ex, cy + 2, ex, cy + BAR_H - 2, 'axis')
        svg.text(PAD_L - 10, cy + BAR_H - 3, r['title'], cls='lbl',
                 anchor='end')
        if undecided:
            cls, tag = 'tie', 'no difference'
        else:
            cls = 'win' if ratio > 1.0 else 'lose'
            tag = '%.2fx' % ratio
        tx = max(bx, px(ci['hi'])) + 8 if ci and bw >= 0 else \
            (min(bx, px(ci['lo'])) - 8 if ci else bx + (8 if bw >= 0 else -8))
        svg.text(tx, cy + BAR_H - 3, tag, cls=cls,
                 anchor='start' if bw >= 0 else 'end')
        y += ROW_H

    svg.footer(meta, note)
    return svg.save(path)


def scoreboard_chart(path, perf, meta):
    """Where hellish actually stands, in one picture.

    Every other chart on the page answers one benchmark at a time, which is how
    a reader ends up concluding 'we lose' from a wall of individually-striped
    rows. This one states the record: per opponent, how many dimensions hellish
    wins, ties and loses, and by how much in the median. It exists because the
    detail charts, read quickly, gave exactly the wrong impression."""
    opponents = ['bash', 'dash']
    have = [o for o in opponents
            if any(r['ratios'].get(o) for r in perf)]
    if not have:
        return None
    import math
    height = 96 + len(have) * 76 + 54
    svg = Svg(W, height, 'Scoreboard — hellish against the field',
              'One row per shell we race. Counts are benchmarks where the 95% '
              'CI clears parity; ties are differences too small to call.')
    y = 80
    plot_w = W - PAD_L - PAD_R

    for opp in have:
        win = lose = tie = 0
        ratios = []
        for r in perf:
            v = r['ratios'].get(opp)
            if not v:
                continue
            ratios.append(v)
            ci = (r.get('ci') or {}).get(opp)
            if ci and not ci.get('significant'):
                tie += 1
            elif v > 1.0:
                win += 1
            else:
                lose += 1
        n = win + lose + tie
        if not n:
            continue
        med = sorted(ratios)[len(ratios) // 2]
        geo = math.exp(sum(math.log(x) for x in ratios if x > 0) / len(ratios))
        svg.text(PAD_L - 10, y + 20, 'vs %s' % opp, cls='lbl', anchor='end')
        svg.text(PAD_L - 10, y + 35, '%d benchmarks' % n, cls='muted',
                 anchor='end')
        x = PAD_L
        for count, color, lab in ((win, COLORS['hellish'], 'hellish faster'),
                                  (tie, '#8b95a1', 'no difference'),
                                  (lose, '#cf6b5e', '%s faster' % opp)):
            bw = plot_w * (count / n)
            if bw <= 0:
                continue
            svg.rect(x, y + 6, bw, 26, color, rx=2)
            if bw > 26:
                svg.parts.append(
                    '<text class="val" x="%.1f" y="%.1f" text-anchor="middle" '
                    'fill="#ffffff">%d</text>' % (x + bw / 2, y + 24, count))
            x += bw
        # Verdict from the RECORD and the MEDIAN, never the geomean. Against
        # dash the geomean reads 1.13x -- "hellish ahead" -- off a 3-win,
        # 7-loss card, because one 8x win on the read loop outweighs seven
        # moderate losses. A summary that inverts the result it summarises is
        # worse than no summary, so the headline is what the median row did.
        if win > lose and med > 1.0:
            verdict = 'hellish ahead: wins %d of %d' % (win, n)
        elif lose > win and med < 1.0:
            verdict = '%s ahead: hellish wins only %d of %d' % (opp, win, n)
        else:
            verdict = 'split: %dW / %dT / %dL' % (win, tie, lose)
        svg.text(PAD_L, y + 50,
                 'median %.2fx   ·   geomean %.2fx   ·   %s'
                 % (med, geo, verdict), cls='muted')
        y += 76

    svg.text(20, height - 30,
             'Ratios are other/hellish: above 1.00x means hellish is faster. '
             'Every benchmark is weighted equally, so one huge win does not '
             'carry the row.', cls='muted')
    svg.footer(meta)
    return svg.save(path)


def conformance_chart(path, data, meta):
    """Pass-rate per suite.  Drawn as a proportion of the SAME total for every
    shell (each suite runs an identical case list), so bar length is directly
    the pass count and the eye is not being asked to compare different
    denominators."""
    suites = data['suites']
    rows = sum(len(s['shells']) for s in suites)
    height = 90 + rows * (BAR_H + 6) + len(suites) * 34 + 48
    svg = Svg(W, height, 'Conformance — third-party suites',
              'Oils spec tests and mksh check.t, same case list for every shell. '
              'Higher is better.')
    y = 76
    plot_w = W - PAD_L - PAD_R

    for suite in suites:
        total = max((v['total'] or 0) for v in suite['shells'].values())
        hdr = suite['title']
        if suite.get('files'):
            hdr += '  (%d files, %d cases)' % (suite['files'], total)
        else:
            hdr += '  (%d cases)' % total
        svg.text(20, y, hdr, cls='lbl')
        y += 12
        order = sorted(suite['shells'].items(),
                       key=lambda kv: -(kv[1]['good'] or 0))
        for name, v in order:
            good, rate = v['good'] or 0, v['rate'] or 0.0
            bw = plot_w * (good / total) if total else 0
            svg.rect(PAD_L, y, plot_w, BAR_H, None, cls='card', rx=2)
            svg.rect(PAD_L, y, bw, BAR_H, COLORS.get(name, FALLBACK))
            svg.text(PAD_L - 10, y + BAR_H - 3, name, cls='lbl', anchor='end')
            svg.text(PAD_L + plot_w + 8, y + BAR_H - 3,
                     '%d/%d  %.1f%%' % (good, total, rate), cls='val')
            y += BAR_H + 6
        y += 22

    svg.text(20, height - 30,
             'hellish is scored conservatively: the `ok` bucket needs a '
             'per-shell annotation in the spec file, which exists for '
             'bash/dash and cannot exist for hellish.', cls='muted')
    svg.footer(meta)
    return svg.save(path)


def vs_bash_chart(path, data, meta):
    """`make bench` over the real-program corpus, as a win/parity/loss band per
    class plus the class geomean.

    A distribution rather than a single average, because 'geomean 1.1x' hides
    whether that came from uniform small wins or from one huge win paying for
    a pile of losses -- and those are very different claims."""
    classes = data.get('classes') or {}
    if not classes:
        return None
    order = [c for c in ('micro', 'corpus', 'hard') if c in classes]
    order += [c for c in classes if c not in order]
    height = 96 + len(order) * 46 + 66
    svg = Svg(W, height, 'Real programs — hellish vs bash --posix',
              'Each class of tests/scripts and tests/hard, timed best-of-N. '
              'Bar shows how the individual tasks landed.')
    y = 78
    plot_w = W - PAD_L - PAD_R
    svg.legend(y, ['hellish'])
    svg.parts.append(
        '<rect x="%d" y="%.1f" width="11" height="11" rx="2" fill="#8b95a1"/>'
        % (110, y - 9))
    svg.text(127, y, 'parity', cls='muted')
    svg.parts.append(
        '<rect x="%d" y="%.1f" width="11" height="11" rx="2" fill="#cf6b5e"/>'
        % (180, y - 9))
    svg.text(197, y, 'bash faster', cls='muted')
    y += 16

    for cls in order:
        c = classes[cls]
        n = c['n'] or 1
        svg.text(PAD_L - 10, y + 18, cls, cls='lbl', anchor='end')
        svg.text(PAD_L - 10, y + 31, '%d tasks' % c['n'], cls='muted',
                 anchor='end')
        x = PAD_L
        for count, color in ((c['faster'], COLORS['hellish']),
                             (c['parity'], '#8b95a1'),
                             (c['slower'], '#cf6b5e')):
            bw = plot_w * (count / n)
            if bw > 0:
                svg.rect(x, y + 6, bw, 22, color, rx=2)
                if bw > 34:
                    svg.parts.append(
                        '<text class="val" x="%.1f" y="%.1f" '
                        'text-anchor="middle" fill="#ffffff">%d</text>'
                        % (x + bw / 2, y + 21, count))
            x += bw
        svg.text(PAD_L + plot_w + 8, y + 21, '%.2fx geo' % c['geomean'],
                 cls='win' if c['geomean'] > 1.05
                 else ('lose' if c['geomean'] < 0.95 else 'tie'))
        y += 46

    ov = data.get('overall') or {}
    svg.text(20, height - 32,
             'overall: n=%d  ·  geomean %.2fx  ·  median %.2fx   '
             '(>1 means hellish is faster)'
             % (ov.get('n', 0), ov.get('geomean', 0), ov.get('median', 0)),
             cls='muted')
    svg.footer(meta)
    return svg.save(path)


def agnostic_chart(path, data, meta):
    ranking = data.get('ranking') or []
    if not ranking:
        return None
    height = 96 + len(ranking) * (BAR_H + 8) + 48
    svg = Svg(W, height, 'Cross-shell ranking — geometric mean over the matrix',
              'Every shell in its own natural mode on the same POSIX workloads; '
              'lower is faster. Output-mismatched runs excluded.')
    y = 82
    plot_w = W - PAD_L - PAD_R
    vmax = max(r['geomean_us'] for r in ranking)
    for r in ranking:
        name = r['shell']
        bw = plot_w * (r['geomean_us'] / vmax) if vmax else 0
        svg.rect(PAD_L, y, bw, BAR_H, COLORS.get(name, FALLBACK),
                 opacity=1.0 if name == 'hellish' else 0.8)
        svg.text(PAD_L - 10, y + BAR_H - 3, '%d. %s' % (r['place'], name),
                 cls='lbl', anchor='end')
        svg.text(PAD_L + bw + 8, y + BAR_H - 3,
                 '%.1f ms  (n=%d)' % (r['geomean_us'] / 1000.0, r['n']),
                 cls='val')
        y += BAR_H + 8
    svg.footer(meta)
    return svg.save(path)


def main():
    dash = os.path.join(ART, 'dashboard.json')
    try:
        with open(dash) as f:
            data = json.load(f)
    except OSError:
        print('no dashboard.json -- run bench/lib/collect_data.py first',
              file=sys.stderr)
        return 1
    meta = data['meta']
    os.makedirs(OUT, exist_ok=True)
    made = []

    perf = data.get('perf') or []
    by_dim = {}
    for r in perf:
        by_dim.setdefault(r['dimension'], []).append(r)

    def rows_for(dim):
        return [{
            'title': r['title'], 'blurb': r['blurb'],
            'values': {s: (v['median_s'] if v else None)
                       for s, v in r['shells'].items()},
            # Bars are struck through only when the comparison is undecidable,
            # not when the machine was busy -- see ratio_chart's docstring.
            'reliable': r.get('conclusive', True),
        } for r in by_dim.get(dim, [])]

    if perf:
        p = scoreboard_chart(os.path.join(OUT, 'scoreboard.svg'), perf, meta)
        if p:
            made.append(p)

    if by_dim.get('initialization'):
        made.append(grouped_bars(
            os.path.join(OUT, 'initialization.svg'),
            'Initialization — shell startup cost',
            'fork+exec the shell, run `true`, exit. Lower is better. '
            'This is what every $(...) and every #!/bin/sh script pays.',
            rows_for('initialization'), meta,
            note='%d runs' % (perf[0]['shells']['hellish']['runs'])))

    if by_dim.get('parsing'):
        made.append(grouped_bars(
            os.path.join(OUT, 'parsing.svg'),
            'Parsing — lexer + parser throughput',
            'A 50k-line script under `set -n`: fully lexed and parsed, nothing '
            'executed. Lower is better.',
            rows_for('parsing'), meta))

    if by_dim.get('execution'):
        made.append(grouped_bars(
            os.path.join(OUT, 'execution.svg'),
            'Execution — loops, builtins, forks and a real workload',
            'Same script fed to every shell. Lower is better. Each row is '
            'scaled to its own slowest bar (the set spans ms to seconds), so '
            'compare shells within a row, not lengths between rows.',
            rows_for('execution'), meta, per_row=True))
        for opp in ('bash', 'dash'):
            rrows = []
            for r in (by_dim.get('initialization', [])
                      + by_dim.get('parsing', []) + by_dim['execution']):
                if r['ratios'].get(opp):
                    rrows.append({'title': r['title'],
                                  'ratio': r['ratios'][opp],
                                  'ci': (r.get('ci') or {}).get(opp)})
            if rrows:
                made.append(ratio_chart(
                    os.path.join(OUT, 'speedup-vs-%s.svg' % opp),
                    'Speedup vs %s, per dimension'
                    % ('bash --posix' if opp == 'bash' else opp),
                    'Bar = %s median / hellish median; right of centre means '
                    'hellish is faster. The whisker is the bootstrap 95%% CI '
                    'on that ratio — bars whose interval crosses parity are '
                    'struck through and reported as no difference.' % opp,
                    rrows, meta))

    res = data.get('resources')
    if res:
        rows = [{
            'title': w['title'],
            'values': w['shells'],
            'reliable': True,
        } for w in res['workloads']]
        made.append(grouped_bars(
            os.path.join(OUT, 'resources.svg'),
            'Resources — peak resident memory',
            'Peak RSS (kernel high-water mark) for the same workloads. '
            'Lower is better. The ~%s floor is mapped libc, paid by any process.'
            % fmt_mem(res.get('floor_kib')),
            rows, meta, unit='mem',
            note='%d rounds  ·  floor %s' % (res.get('rounds') or 0,
                                             fmt_mem(res.get('floor_kib'))),
            warn_unreliable=False))

    conf = data.get('conformance')
    if conf:
        made.append(conformance_chart(
            os.path.join(OUT, 'conformance.svg'), conf, meta))

    vsb = data.get('vs_bash')
    if vsb:
        p = vs_bash_chart(os.path.join(OUT, 'real-programs.svg'), vsb, meta)
        if p:
            made.append(p)

    agn = data.get('agnostic')
    if agn:
        p = agnostic_chart(os.path.join(OUT, 'agnostic.svg'), agn, meta)
        if p:
            made.append(p)

    for p in made:
        print('  %s' % os.path.relpath(p, os.path.dirname(BENCH)),
              file=sys.stderr)
    print('%d charts -> bench/charts/' % len(made), file=sys.stderr)
    return 0


if __name__ == '__main__':
    sys.exit(main())
