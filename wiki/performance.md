# Performance & Robustness

> **Fast enough to not think about, hard enough to trust.**
> Status legend: ✅ shipped · 🚧 in progress · 📋 planned

Speed is hellish's credibility. Most new/clean shells are *slower* than bash; hellish is
**measurably faster than bash** while carrying a far richer feature set than the minimalist speed
kings. This page shows the numbers (real, measured) and the engineering discipline behind them.

---

## vs. bash — faster, across the board ✅

From the in-repo benchmark harness (`make bench`, best-of-N with h/b rounds interleaved and
both shells pinned to one core — see the fairness note below), hellish (OPT, `-O3 -flto`,
`ft_malloc`) vs. `bash --posix`:

| Suite | geomean (per-task) | wall (throughput) | W/T/L |
|---|---|---|---|
| **Overall** (82 tasks) | **1.453×** | **1.762× faster** | 54 / 22 / 6 |
| micro (tight loops) | **2.408×** | 1.898× | **29 / 0 / 0** |
| corpus (real scripts) | 1.068× | 1.165× | 14 / 19 / 4 |
| hard (math/text heavy) | 1.183× | 1.599× | 11 / 3 / 2 |

Every timed number behind a verdict is appended to `tests/bench_results.txt` — measured, not
claimed. (Individual 2-3 ms corpus scripts still wobble ±15% run to run, which moves per-task
W/T/L; the verdict does not.)

- **geomean = equal weight per task; wall = total time to run everything.**
- The micro class — once the weak spot at 0.877× — is a **clean sweep**: every tight-loop
  task beats bash, 2.4× on average. Real work (corpus/hard) wins too; standouts: pure-shell
  insertion sort **4.4×**, `$(echo …)` substitutions **11.4×**, string toolkit ~1.9×.
- Coverage: faster on **78%** of tasks, parity on 9%, slower on 13% — and where it is slower
  the average gap is **−7%**, concentrated in 2-3 ms scripts whose ratios are run-to-run noise
  plus a couple of fork/signal-bound ones.
- The jump from the previous 1.371× baseline is the **forkless command substitution** fast
  path (`perf/forkless-cmdsub`): a `$( )` whose body is a single side-effect-free builtin
  (`echo`, `printf`, `pwd`, `true`, `:` — no redirects or control operators, no
  `${v=…}`/`$((…))` assignment forms, not under `set -e/-u`) runs in-process with stdout
  parked on an unlinked temp file instead of fork+pipe+waitpid — ksh93's famous trick applied
  to the provably safe subset. `cmdsub 3k` went from 1.25× to **11.4×** vs bash (35 ms vs
  402 ms); ineligible bodies fork exactly as before, and the full suite plus both-allocator
  parity stay green.
- **Harness fairness** (`tests/benchmark`): rounds interleave h,b,h,b so machine-load drift
  hits both shells equally; both are pinned to the same core (`taskset -c 0`, `NOPIN=1`
  opts out); each task gets an untimed warmup; every timing lands in the artifact.

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

## vs. the whole zoo — #2 of 9 ✅

`make agnostic-bench` races hellish against every shell it can install in one docker image
(bash, dash, zsh, mksh, ksh93, yash, busybox ash, fish) on a portable POSIX workload set,
output-checked against bash and ranked by per-workload geomean. After the forkless-cmdsub
campaign:

| # | shell | geomean | vs hellish |
|---|---|---|---|
| 1 | ksh93 | 16.5 ms | 0.67× |
| **2** | **hellish** | **24.8 ms** | **1.00×** |
| 3 | dash | 24.8 ms | 1.00× |
| 4 | busybox ash | 34.4 ms | 1.39× |
| 5 | zsh | 51.3 ms | 2.07× |
| 6 | bash | 60.2 ms | 2.43× |
| 7 | mksh | 66.7 ms | 2.69× |
| 8 | yash | 83.8 ms | 3.38× |
| 9 | fish | 234.6 ms | 9.46× |

hellish moved up from #3 (behind dash) to **#2**, edging dash on geomean and beating it ~7× on
substitution-heavy workloads. Only ksh93 stays ahead — it runs functions and *all* command
substitutions in-process (its `$(fib 18)` is ~94× faster than anyone's fork), a whole-shell
architecture rather than a fast path.

---

## How it got faster ✅ *(the `perf/micro-hotpath` campaign)*

Measured-first: every change was driven by strace/callgrind evidence and landed only with the
full conformance suite, both-allocator parity and leak gates green. The big levers, in order of
impact:

- **fd passthrough** — stdin/stdout pass through the pipeline driver untouched; the per-command
  backup/redirect/restore dance only runs when there is actual fd work. (15 → 0 fd syscalls per
  plain command.) Unmasked and fixed a latent `exec 3>f` bug (scratch fd moved to ≥10, bash-style).
- **Single-stage pipeline direct path** — every plain command is a pipeline of one; it now skips
  the multi-stage scaffolding (results vector, stage copy, finalize pass) entirely. 8.6% fewer
  instructions on the tight-loop microbench.
- **Buffered `echo`** — one `write(2)` per call instead of one per argument (and per *character*
  under `-e`).
- **Zero-malloc bookkeeping** — `$?` formats into a scratch buffer in `t_shell`; `$_` skips its
  env update when unchanged; simple-`$var` argv words come from the word slab.
- **Block-buffered `read`** — on seekable fds, read 128-byte blocks and `lseek` back over the
  unconsumed tail (bash's trick); pipes/ttys keep POSIX byte-at-a-time.
- **Split-`$PATH` cache** — validated on use by comparing the exact PATH string; also fixed a
  conformance bug (the command hash now flushes when PATH changes, as POSIX requires).
- **Pipe-backed heredocs** — bodies ≤ 4 KB get a pipe (exactly like bash ≥ 5.1): zero filesystem
  traffic per materialization; larger bodies use an eagerly-unlinked `O_RDWR` temp file (no
  `/tmp` litter possible, mode 0600). Flipped the heredoc-in-loop benchmark from 0.68× to
  **1.43× faster than bash**.
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

- **2527+ tests** (`make test`) — a self-built suite that grows with every fix; all green.
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
