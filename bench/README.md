# bench/ — conformance & speed vs the field

Three questions, three harnesses, plus a chart generator that turns all of
them into the images the README shows.

- **`make conformance`** — "does it behave like a real shell?"
  Runs two independent third-party suites — the Oils spec tests and mksh's
  `check.t` — against hellish, `bash --posix`, and dash, writes
  [conformance.md](conformance.md), and fails if hellish's pass count ever
  drops below [baseline/](baseline/) (the regression gate;
  `UPDATE_BASELINE=1` to accept new counts).

- **`make perf`** — "is it actually faster, and where?"
  Dimension-split hyperfine benchmark (startup, parse, loops, forks, a
  real `./configure`), writes [results.md](results.md).  Then runs the
  peak-RSS dimension over the same workloads.  Wants the `performance`
  CPU governor; without it every report and chart is stamped provisional.

- **`make rss`** — the memory half of `make perf`, on its own.
  Peak RSS per workload per shell via GNU `time -f %M`.

- **`make charts`** — "show me."
  Reads every artifact currently on disk, normalizes it into
  `.artifacts/dashboard.json`, and renders [charts/](charts/) as SVG.
  It never runs a benchmark itself: measuring and drawing are separate, so
  a chart can always be regenerated for free, and a missing harness yields
  a missing chart rather than an error.

```sh
make perf conformance   # measure (slow)
make charts             # draw (instant)
```

Every fairness decision is documented in [METHODOLOGY.md](METHODOLOGY.md).
First run fetches ~100MB of suites into `bench/suites/` (gitignored);
`lib/fetch_suites.sh` is idempotent.

## Two traps this harness has already fallen into

Both were silent — they produced plausible numbers, not errors — so they are
written down here rather than only fixed in code:

1. **`timeout` must be GNU coreutils.**  uutils' `timeout` (the Rust rewrite
   now shipping as `/usr/bin/timeout` on some distros) reaps its child on a
   100 ms polling grid, adding up to 100 ms to every measured command and
   rounding the result up to the next 100 ms bucket.  `dash -c true` read
   103 ms instead of 0.5 ms and every sub-second dimension collapsed into
   three indistinguishable buckets.  `run.sh` now probes for GNU and warns
   if it cannot find it.
2. **Don't measure peak RSS from a python parent.**  `ru_maxrss` is a
   lifetime high-water mark, so a forked child inherits the parent's ~10 MB
   image into its own peak before `exec()` replaces it — and every shell
   reads back an identical ~10 MB.  `lib/run_rss.sh` shells out to GNU
   `time` (a 27 KB binary) instead.

Layout: `lib/` runners, report and chart generators · `charts/` generated
SVGs (tracked — the README embeds them) · `suites/` vendored third-party
corpora (scratch) · `workloads/` generated scripts + GNU hello (scratch) ·
`.artifacts/` raw TSV/JSON results (scratch) · `baseline/` tracked pass
counts the gate defends.
