# Benchmark & conformance methodology

Every choice that affects fairness, listed so anyone can rerun and argue
with the numbers instead of the setup.  Rerun with `make perf` and
`make conformance`.

## Shells under test

| label | invocation | why these flags |
|---|---|---|
| hellish | `bench/.bin/hellish --posix` | extensions off — same posture as bash below |
| bash | `/bin/bash --norc --posix` | `--norc` so startup doesn't bill bash for the user's rc file; `--posix` because that's the dialect hellish diffs against |
| dash | `/usr/bin/dash` | dash IS the POSIX baseline; it has no rc file or posix flag to toggle |

The hellish binary is the `make OPT=1` build (`-O3 -flto`, `ft_malloc`
heap) — the shipping configuration.  It is copied to `bench/.bin/` before
each run so mid-run relinks can't swap the binary underneath the bench.

## Performance

- **Tool**: hyperfine 1.19 with `-N` (no intermediary shell — commands are
  exec'd directly).  The only wrappers are `taskset` (identical for every
  shell) and, in the configure dimension only, `env` to set `CONFIG_SHELL`
  (also identical for every shell).  Neither is a shell.
- **Pinning**: every measured process runs under `taskset -c 2`
  (`BENCH_CPU` to override) to kill core-migration noise.
- **Warmup/runs**: `--warmup 10 --min-runs 30` for dimensions a–d.
  The configure dimension uses `--warmup 1 --min-runs 5` — a deliberate
  deviation, documented here, because each run costs ~10s+ and the run
  time is long enough that scheduler noise amortizes.
- **Governor**: the harness *refuses to run* unless the CPU governor is
  `performance` (`sudo cpupower frequency-set -g performance`).
  `BENCH_LAX=1` overrides for machines where that isn't possible; the
  report then carries a prominent flag.  Absolute times measured under
  `powersave` are pessimistic and noisier; cross-shell *ratios* remain
  broadly meaningful because all shells suffer the same governor.
- **Reliability signal**: the report computes CV = stddev/median per cell
  and flags anything over 3% as unreliable rather than hiding it.
- **Same bytes for everyone**: each dimension feeds the identical script
  file to all three shells; scripts are strict POSIX sh, generated
  deterministically by `lib/gen_workloads.sh` (no randomness, no
  timestamps).
- **Known asymmetry, disclosed**: in `fork_cmdsub` (`$(true)` ×1000),
  hellish legitimately skips the fork via its eligibility-checked forkless
  substitution path — the same optimization ksh93 is famous for.  That is
  the feature being measured, not an unfair harness.  `fork_cmdsub_ext`
  (`$(/bin/true)`) exists precisely to force a real fork+exec on every
  shell, hellish included.

### Dimensions

a. **startup** — `<shell> -c true`: process init + teardown, nothing else.
b. **parse50k** — a generated 50k-line script of realistic constructs with
   `set -n` on line 1: everything is lexed/parsed, nothing executes.
c. **loop_*** — 100k-iteration while-loops isolating arithmetic
   (`$((i+1))`), string concat, the `:` builtin, and `read` over a
   100k-line file.
d. **fork_*** — 1k iterations of `$(true)`, `$(/bin/true)`, and a 3-stage
   pipeline (`printf | cat | wc -l`).
e. **configure** — GNU hello 2.12.1 `./configure --quiet` with
   `CONFIG_SHELL` pointed at the shell under test, in a scratch dir wiped
   between runs (`--prepare`, unmeasured).  The classic real-world shell
   torture: thousands of forks, case statements, string ops.

## Conformance

- **Oils spec tests**: the 79 `spec/*.test.sh` files whose
  `## compare_shells:` includes dash — i.e. the corpus Oils itself uses to
  compare POSIX-ish shells.  Runner: `test/sh_spec.py`, ported to python3
  in-place by `lib/port_oils_py3.sh` (mechanical port; judgement logic
  untouched).  Per-case timeout 5s; a whole file gets 600s.
- **Scoring**: pass-rate counts `pass` + `ok`.  `ok` = matched a per-shell
  annotation in the spec file.  Such annotations exist for bash and dash
  but *cannot* exist for hellish (the upstream spec files don't know it),
  so hellish's rate is structurally conservative — the bias runs against
  us, not for us.
- **Consensus divergence**: hellish fails while bash --posix AND dash both
  pass.  When the two referees agree and we differ, that's a bug, not an
  opinion.  When the referees disagree among themselves, the behaviour is
  unspecified and no shell is charged for it.
- **mksh check.t**: mksh's own harness (`check.pl -P -p '<shell> <flags>'
  -t 10`).  Most mksh-specific cases fail for every foreign shell alike;
  the comparison of pass counts across the three shells is the signal,
  not the absolute number.
- **Gate**: `make conformance` fails if hellish's pass count drops below
  `bench/baseline/conformance.json` on either suite.
  `UPDATE_BASELINE=1` accepts the current counts.

## Known limitations

- **hellish does not yet complete GNU autoconf's `configure`.** The
  configure dimension times only the shells that produce `config.status`
  (bash, dash); hellish is excluded and flagged in results.md. The
  remaining blocker is that configure's `exec 5>>config.log` never opens
  config.log in hellish's context (confirmed by strace: no openat for
  config.log, while bash/dash open it), so every `>&5` write fails with
  EBADF and the compiler probe aborts. A first, real bug in this area was
  already fixed — internal fd save/restore used plain `dup()` and grabbed
  fds 5/6/7 out from under the script (see `save_fd()` in
  `src/execution/utils.c`) — which moved configure from rc 77 in 0.6s to
  running the full compiler-probe sequence; the config.log open itself is
  a separate, still-open bug tracked for a focused session.
- bash's own test suite and the Smoosh corpus are not wired in yet; both
  are natural extensions of `lib/run_*.sh`.
- Only bash --posix and dash are referees; yash (a useful third POSIX
  referee) is not installed on the bench host.
- The Oils runner executes cases sequentially; a full three-shell sweep
  takes several minutes.
- `bench/suites/` and `bench/workloads/` are gitignored scratch; only the
  harness, the reports, and the baseline are tracked.
