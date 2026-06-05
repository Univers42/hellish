# Performance & Robustness

> **Fast enough to not think about, hard enough to trust.**
> Status legend: ✅ shipped · 🚧 in progress · 📋 planned

Speed is hellish's credibility. Most new/clean shells are *slower* than bash; hellish is at
**parity with bash** while carrying a far richer feature set than the minimalist speed kings. This
page shows the numbers (real, measured) and the engineering discipline behind them.

---

## vs. bash — at parity, faster by throughput ✅

From the in-repo benchmark harness (`make -C vendor/42sh bench`), hellish (OPT, `-O3 -flto`) vs.
`bash --posix`:

| Suite | geomean (per-task) | wall (throughput) | W/T/L |
|---|---|---|---|
| **Overall** (57 tasks) | **1.011×** | **1.189× faster** | 25 / 15 / 17 |
| corpus (real scripts) | 1.043× | 1.112× | 15 / 12 / 6 |
| hard (math/text heavy) | 1.037× | 1.144× | 8 / 2 / 4 |
| micro (tight loops) | 0.877× | 1.207× | 2 / 1 / 7 |

- **geomean = equal weight per task; wall = total time to run everything.**
- hellish *wins on real work* (command substitution, math, text processing) and trails only on
  synthetic tight loops of near-empty commands — and even there it finishes the wall-clock faster.
- Coverage: faster on **56%** of tasks, parity on 12%, slower on 32% (avg −15% when slower,
  on the cheapest micro-ops).

> Why the micro-loop gap exists is well understood: it's per-command *work volume* (each command
> does more setup than a barebones shell), not a hot-path inefficiency. That's the honest ceiling,
> and it's the target of the startup/exec work below.

## vs. dash — the minimalist speed floor

dash is the deliberately feature-barren `/bin/sh` speed king (no `[[ ]]`, no arrays, no process
substitution, no interactive editing). It marks the floor, not a feature competitor. Quick
single-run measurements on this host:

| Metric | dash | hellish | note |
|---|---|---|---|
| Binary size | 130 KB | 387 KB | dash: libc only; hellish: + readline/tinfo |
| Startup (`sh -c :`) | 0.46 ms | 0.74 ms | ~1.6× — **mostly readline/banner init** 🚧 |
| POSIX loop → 200k | 0.13 s | 0.42 s | per-command overhead (the micro ceiling) |

hellish *does dash's job* (it ran an entire LFS build's `./configure` scripts) — it just does far
more per command. bash is also several times slower than dash, so "slower than dash" is the club
every full-featured shell belongs to.

---

## Making it faster 🚧 *(Week 4)*

- **Script-mode startup**: when invoked non-interactively (`-c`, a file, or a pipe), skip readline
  init, the welcome banner, and lazy-init history/completion. Most of the ~1.6× dash startup gap is
  init that a non-interactive run never needs. Target: meaningfully narrow it; numbers recorded
  before/after.
- **Never regress speed**: the benchmark geomean is a release gate (must stay **≥ 1.0**).

---

## Robustness — the trust story ✅

Speed means nothing if it crashes. hellish is held to a strict, automated bar:

- **2481+ tests** (`make test`) — a self-built suite that grows with every fix; all green.
- **Conformance gate** — every shell construct diffed against `bash --posix`, **0 divergences**.
- **Per-fix regression cases** — each bug fix lands with a permanent test *and* a conformance case,
  so it can never silently come back.
- **Memory-clean** — debug builds run under **AddressSanitizer + leak detector**; valgrind-clean on
  exercised paths.
- **42 norminette-clean** — enforced small functions and consistent style across the whole tree.
- **Reproducible** — the same gates run in Docker; the shell binary is published per-commit.

🚧 *(Week 3)* **Crash-hardening pass**: malformed input must produce a graceful error, never an
abort. (E.g. an unterminated `${…}` that currently asserts gets turned into a clean syntax error,
verified by a fuzz-style malformed-input sweep under ASan.)

---

## Reproduce it yourself

```sh
make -C vendor/42sh OPT=1 all     # the optimized binary that's benchmarked
make -C vendor/42sh bench         # vs bash --posix (geomean + wall + per-task)
make -C vendor/42sh test          # the full suite
make -C vendor/42sh norm          # norminette
```

See also: **[Interactive Experience](interactive.md)** · **[Bash Compatibility & Scripting](scripting.md)** · **[What hellish is + Install](product.md)**
