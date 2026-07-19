# bench/ — conformance & speed vs the field

Two questions, two harnesses:

- **`make conformance`** — "does it behave like a real shell?"
  Runs two independent third-party suites — the Oils spec tests and mksh's
  `check.t` — against hellish, `bash --posix`, and dash, writes
  [conformance.md](conformance.md), and fails if hellish's pass count ever
  drops below [baseline/](baseline/) (the regression gate;
  `UPDATE_BASELINE=1` to accept new counts).

- **`make perf`** — "is it actually faster, and where?"
  Dimension-split hyperfine benchmark (startup, parse, loops, forks, a
  real `./configure`), writes [results.md](results.md).  Requires the
  `performance` CPU governor, or `BENCH_LAX=1` to run flagged.

Every fairness decision is documented in [METHODOLOGY.md](METHODOLOGY.md).
First run fetches ~100MB of suites into `bench/suites/` (gitignored);
`lib/fetch_suites.sh` is idempotent.

Layout: `lib/` runners and report generators · `suites/` vendored
third-party corpora (scratch) · `workloads/` generated scripts + GNU hello
(scratch) · `.artifacts/` raw TSV/JSON results (scratch) · `baseline/`
tracked pass counts the gate defends.
