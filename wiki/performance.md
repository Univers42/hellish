# Performance & Robustness

> **Fast enough to not think about, hard enough to trust.**
> Status legend: ✅ shipped · 🚧 in progress · 📋 planned

Speed is hellish's credibility. Most new/clean shells are *slower* than bash; hellish is
**measurably faster than bash** while carrying a far richer feature set than the minimalist speed
kings. This page shows the numbers (real, measured) and the engineering discipline behind them.

---

## vs. bash — faster, across the board ✅

From the in-repo benchmark harness (`make bench`, ROUNDS=7 best-of), hellish (OPT, `-O3 -flto`,
`ft_malloc`) vs. `bash --posix`:

| Suite | geomean (per-task) | wall (throughput) | W/T/L |
|---|---|---|---|
| **Overall** (81 tasks) | **1.333×** | **1.342× faster** | 51 / 18 / 12 |
| micro (tight loops) | **2.010×** | 1.325× | **28 / 0 / 0** |
| corpus (real scripts) | 1.031× | 1.059× | 12 / 15 / 10 |
| hard (math/text heavy) | 1.178× | 1.450× | 11 / 3 / 2 |

- **geomean = equal weight per task; wall = total time to run everything.**
- The micro class — once the weak spot at 0.877× — is now a **clean sweep**: every tight-loop
  task beats bash, 2× on average. Real work (corpus/hard) wins too; standouts: pure-shell
  insertion sort **4.2×**, `${}` string toolkit **2.0×**, log analyzer/math suite ~1.2×.
- Coverage: faster on **69%** of tasks, parity on 9%, slower on 22% — and where it is slower the
  average gap is **−9%**, concentrated in a handful of small fork/signal-bound scripts.

> The old "micro ceiling" was diagnosed and removed in the `perf/micro-hotpath` campaign: the
> pipeline driver dup'd stdin/stdout around **every** command, defeating the redirect fast path —
> 15 fd syscalls per command (a 30k-iteration `:` loop made 1,020,219 syscalls where bash makes
> 171). With std fds passed through untouched, plus a buffered `echo` (one `write(2)` per call,
> not per argument), zero-malloc `$?`/`$_` bookkeeping, slab-allocated `$var` words, block-buffered
> `read` on seekable fds, a validated split-`$PATH` cache, and single-fd unlinked-early heredocs,
> the per-command cost is now below bash's.

## vs. dash — the minimalist speed floor

dash is the deliberately feature-barren `/bin/sh` speed king (no `[[ ]]`, no arrays, no process
substitution, no interactive editing). It marks the floor, not a feature competitor. Quick
single-run measurements on this host:

| Metric | dash | hellish | note |
|---|---|---|---|
| Binary size | 130 KB | 387 KB | dash: libc only; hellish: + readline/tinfo |
| Startup (`sh -c :`) | 1.30 ms | 1.50 ms | ~1.15× — and already faster than bash's |
| POSIX loop → 200k | 0.129 s | **0.103 s** | **hellish now beats dash here** (was 3.2× slower) |

hellish *does dash's job* (it ran an entire LFS build's `./configure` scripts) — and after the
hot-path campaign it beats dash on tight POSIX loops while carrying readline, job control,
`[[ ]]`, process substitution and the rest. (Numbers above re-measured on the same host, best of
5-7 runs.)

---

## How it got faster ✅ *(the `perf/micro-hotpath` campaign)*

Measured-first: every change was driven by strace/callgrind evidence and landed only with the
full conformance suite, both-allocator parity and leak gates green. The big levers, in order of
impact:

- **fd passthrough** — stdin/stdout pass through the pipeline driver untouched; the per-command
  backup/redirect/restore dance only runs when there is actual fd work. (15 → 0 fd syscalls per
  plain command.) Unmasked and fixed a latent `exec 3>f` bug (scratch fd moved to ≥10, bash-style).
- **Buffered `echo`** — one `write(2)` per call instead of one per argument (and per *character*
  under `-e`).
- **Zero-malloc bookkeeping** — `$?` formats into a scratch buffer in `t_shell`; `$_` skips its
  env update when unchanged; simple-`$var` argv words come from the word slab.
- **Block-buffered `read`** — on seekable fds, read 128-byte blocks and `lseek` back over the
  unconsumed tail (bash's trick); pipes/ttys keep POSIX byte-at-a-time.
- **Split-`$PATH` cache** — validated on use by comparing the exact PATH string; also fixed a
  conformance bug (the command hash now flushes when PATH changes, as POSIX requires).
- **Single-fd heredocs** — one `O_RDWR` fd, unlinked eagerly (no `/tmp` litter possible, mode
  0600), rewound with `lseek` instead of re-opened.
- **Custom allocator (`SAFE=0`)**: the whole shell allocates through one compile-time-switchable
  macro family (`xmalloc`/`xcalloc`/`xfree`), so it can run on either libc `malloc` or our own
  `ft_malloc` slab/arena heap with **zero source changes**. `make OPT=1` (the benchmarked build)
  defaults to `SAFE=0` (`ft_malloc`); `make OPT=1 SAFE=1` re-runs the same workloads on libc, so the
  allocator's contribution is measurable in isolation. Both backends are conformance- and ASan-clean.
- **Never regress speed**: the benchmark geomean is a release gate (must stay **≥ 1.0** — currently
  1.333×).

---

## Robustness — the trust story ✅

Speed means nothing if it crashes. hellish is held to a strict, automated bar:

- **2519+ tests** (`make test`) — a self-built suite that grows with every fix; all green.
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
make OPT=1 all          # the optimized binary that's benchmarked (SAFE=0, ft_malloc)
make OPT=1 SAFE=1 all   # same, on libc malloc — A/B the allocator
make bench              # vs bash --posix (geomean + wall + per-task), ROUNDS=7 recommended
make test               # the full suite
make norm               # norminette
```

See also: **[Interactive Experience](interactive.md)** · **[Bash Compatibility & Scripting](scripting.md)** · **[What hellish is + Install](product.md)**
