# Session context — 2026-08-22

Working notes for whoever picks this up next, from another workspace or a
later session. The repo history is authoritative; this file records the
things history cannot tell you — why a thing was done, what was measured,
what was deliberately *not* done, and what is still open.

Branch to resume from: **`develop`**.

---

## What shipped in this session

Every issue on the tracker is now closed. Four commits, each on its own
branch, merged fast-forward into `develop`:

| Commit | Issue | What |
|---|---|---|
| `8568479` | #27 | per-job process groups — `^Z` did nothing at all |
| `65a14f2` | #32 | `shopt -s lithist` — multi-line history recall |
| `e6a5153` | #27 | `jobs` reports `Done(N)`; 19 golden cases |
| `a575c12` | #34 | prompt frames dropped on a non-blocking tty |
| `c4b35d1` | — | stop asserting the flaky job marker in a golden case |
| `810a476` | — | `jobs` reports live status inside a pipeline |

Earlier in the same session (already on `develop` before these): #41, #40,
#39, #28, #37, #2, #3.

---

## The four fixes, and the evidence behind them

### #27 — foreground commands had no process group of their own

The issue described this as a signal-aiming problem. It was worse than
that: **`^Z` did nothing at all.**

POSIX XSH 2.4.3 — `SIGTSTP`/`SIGTTIN`/`SIGTTOU` delivered to a member of an
**orphaned** process group are *discarded*. A group is orphaned when no
member has a parent in a different group of the same session, which is
exactly the shell's own group once the shell is a session leader (every
terminal emulator makes it one) and its children stay inside it. The
terminal sent SIGTSTP, the kernel dropped it, the command kept running and
kept eating keystrokes meant for the shell.

Proven with no shell involved: a child left in a session leader's group
never stops under `^Z`; the same child given its own group stops every time.

Fix lives in `src/platform/posix/job_pgrp.c` — `jc_init` / `jc_begin` /
`jc_child` / `jc_parent` / `jc_end`. `setpgid` runs on **both** sides of the
fork so neither side can lose the race, and `tcsetpgrp` hands the terminal
over and takes it back around the wait.

**Interactive-only, and that is load-bearing for performance.** `jc_begin`
decides once per pipeline with a `getpid()` check, so background bodies,
subshells and `$( )` captures all fail the test and cannot move a group or
grab the tty. Verified: `strace -c -e trace=setpgid` shows **zero**
`setpgid` calls across 100 forked commands under `-c`. `make bench` after
the change: **geomean 1.312× faster than bash**, 74% of tasks faster.

Two adjacent defects fell out of testing it:

- `job_update_status` only polled `JOB_RUNNING` jobs, so once a job stopped
  the shell stopped asking about it *forever* — a `^Z`'d job later killed
  stayed listed as `Stopped` for the rest of the session.
- `kill` on a stopped job left the signal pending forever, so `kill %1`
  looked like a no-op. bash follows it with `SIGCONT`; `job_kill_group`
  now does too, exempting the stop signals themselves.

### #32 — multi-line history recall

hellish had implemented only bash's `cmdhist` half: a compound is stored as
one entry with boundary newlines rewritten to `; `. That is the correct
*default* and stays the default. bash's other half is `lithist`, which keeps
the newlines, and hellish had no way to ask for it.

Both modes now match bash 5.3.9 byte for byte in a pty: default recall ends
`; fi`, `lithist` recall ends `<newline>fi`. No history-file format change
was needed — `encode_cmd_hist` already escapes embedded newlines.

Two pre-existing `shopt` exit-status bugs surfaced, and they are **not the
same rule**: `-s`/`-u` report whether the *change* succeeded (so a
successful `shopt -u x` is 0), while a bare `shopt name` reports the
*setting* (so it is 1 when off). Both were wrong; both fixed.

### #34 — prompt corruption on a non-blocking terminal

An earlier repair routed prompt writers through `tty_write_all`, which loops
over short writes. That loop retried `EINTR` and **returned on every other
error** — and `EAGAIN` is an error. Same class of bug, worse consequence: a
short write loses a tail, `EAGAIN` loses the *whole frame*.

It is reachable in ordinary use because `O_NONBLOCK` lives on the open file
**description**, which the shell shares with every program it launches. One
tool that sets it on the terminal and does not restore it leaves the shell
writing to a non-blocking tty for the rest of the session.

Measured, real 65-byte prompt frame against a full 64K buffer:

```
EINTR-only loop (before)   wrote  0 / 65   <- entire frame lost
with the EAGAIN wait       wrote 65 / 65
```

**This is a strong candidate for #34's report, not a confirmed diagnosis.**
The original corruption was never reproduced. Two independent holes in the
same write path are now closed; that is all the evidence supports, and the
issue comment says so plainly.

---

## Tests added, and how to trust them

Every fix ships with a gate, and **each new gate was verified non-vacuous**
by deliberately reverting the fix and confirming the gate fails:

| Gate | Checks | Sabotage result |
|---|---|---|
| `make bg-tty-test` | 13 → 23 | — |
| `make nonblock-tty-test` (new) | 10 | queue-full check FAILS without the EAGAIN wait |
| `make hist-test` | +5 lithist | both lithist checks FAIL with the join forced on |
| `tests/issue27_job_pgrp` (new) | 19 golden | — |
| `tests/regress_hellish` | +8 shopt/lithist | — |

`tests/issue27_job_pgrp` is wired into `test_lists=(…)` at the top of
`tests/tester` — a new category file that is *not* added there silently
never runs in `make test`. `nonblock-tty-test` is wired into the Makefile
`.PHONY` list and into CI's `interactive` job.

### A harness bug worth knowing about

`tests/run_scripts.sh` was grading against **whatever bash was in `PATH`**,
not the pinned 5.3.9 that `tests/tester` uses. bash 5.2 and 5.3 disagree on
the `jobs` status-column width, which is why `17_bg_signal_report.sh`
"failed" there while passing under the pinned oracle — **oracle drift
reported as a hellish bug**, and it had been sitting in the corpus as a
known failure. Fixed in `8568479`; the corpus is now 109/109.

If a batch of failures ever appears without a matching source change,
suspect oracle drift first. `make oracle` builds the pinned bash.

---

## Verification state (all green on `develop` @ `a575c12`)

```
make test                 3742 / 3742   vs pinned bash 5.3.9
tests/run_scripts.sh       109 / 109
tests/verify_alloc.sh       78 / 78     on BOTH heaps, 0 leaks
make bg-tty-test            23 checks, 0 failed
make nonblock-tty-test      10 checks, 0 failed
make hist-test              ALL PASSED
make prompt-atomic-test      4 / 4      (1.1 MB over 9366 writes)
make re / make re OPT=1     clean, -Wall -Wextra -Werror
norminette                  clean on every touched file
make bench                  geomean 1.312× faster than bash
```

---

## Known gaps — NOT fixed, deliberately

Read this section before assuming something is done.

1. **`tests/hard/12_job_control.sh` is intermittently flaky (~1–2%,
   load-dependent).** `wait $p3` occasionally reports `127`. I investigated
   this at length and want the next person to have the honest state:
   - It is **pre-existing** — reproduced at the same rate with the
     job-control changes reverted.
   - I identified a real durability gap (the status poll recorded a reaped
     child only into the job table, and `job_purge_done` from an unrelated
     `wait` discards that entry, losing the status from both lookups) and
     fixed it, because `builtin_jobs` already followed the ring rule and
     the poll had simply not been taught it.
   - **That fix is NOT proven to be the cause.** After it: 0 failures in
     120 runs. But with the fix reverted: 0 failures in 80 runs. The rate
     is too low and too load-sensitive for those samples to distinguish.
     Do not record this as diagnosed.
   - A deterministic reproducer was attempted twice and failed both times;
     the window needs `wait $p1` to succeed on a live child while `p3` has
     already been reaped by the poll.

2. **`jobs` command text is a raw source slice, not a re-render.** bash
   prints `( exit 7 )` for input `(exit 7)`; hellish prints `(exit 7)`.
   Matching bash means pretty-printing the AST. The golden cases in
   `issue27_job_pgrp` are written with bash-canonical spacing to sidestep
   this — that is deliberate, and it means those cases do *not* cover the
   label renderer.

3. **`kill -0 $pid` on a finished background job** returns 0 in hellish and
   1 in bash: bash reaps eagerly, hellish leaves the zombie a moment longer.
   A test line for this was written and then removed rather than assert
   the wrong behaviour.

4. **The `+`/`-` current-job marker is environment-sensitive.** Do not
   assert it in a golden case. `( exit 0 ) & sleep 0.1; jobs` prints
   `[1]+  Done` under the pinned bash locally and `[1]   Done` under the
   same pinned bash on a CI runner. My first version of
   `issue27_job_pgrp` asserted it, passed locally 5/5, and failed in CI —
   the cases now normalise the marker away with sed and assert only the
   status column, which is the thing actually under test.

4. **Docker Hub release channel is still unpublished** (#37). The workflow
   guard is in and correct — an unconfigured repo now *skips* rather than
   fails — but `DOCKERHUB_USERNAME`/`DOCKERHUB_TOKEN` are not set. GHCR
   carries the image. Setting the two secrets is all that is left.

5. **`vendor/libft` has an uncommitted submodule pointer change** in the
   working tree that predates this session. I did not commit or touch it.
   Check what it is before assuming it is intentional.

---

## Conventions that bit me, recorded so they do not bite again

- **`make norm` always exits 0** — it only *reports*. Read its output; do
  not trust the exit status. It caught `TOO_MANY_FUNCS` and `SPC_AFTER_PAR`
  on files that "built fine" twice this session.
- **The 5-function-per-file ceiling forces helper placement.**
  `done_with_code` lives in `job_signal2.c` rather than beside its only
  caller for exactly this reason; `subshell_child` and `cmd_child_body`
  were extracted to keep their callers under 25 lines.
- **`ft_snprintf` does not exist.** libft has `ft_strlcpy`/`ft_strlcat` and
  an allocating `ft_itoa`. A borrowed-string return contract rules out
  `ft_itoa`, hence the hand-rolled digit loop in `done_with_code`.
- **`cd tests && …` persists across Bash tool calls.** Several `make`
  invocations silently failed with "No rule to make target" because of it.
  Use absolute paths.
- **The pty renders a newline as `\r\n\r`.** A test asserting `"\nfi"` on
  raw pty output will fail even when the shell is correct; strip `\r` first.
- **The history file is `~/.minishell_history`**, not `.hellish_history`.

---

## Suggested next steps

In rough priority order:

1. **Decide the release.** `develop` is four commits ahead of the last tag
   (2.7.1) with one feature (`lithist`) and three fixes, one of them a
   real correctness fix to job control. That is a **2.8.0**. I did not cut
   it — see the note below.
2. Chase the `12_job_control.sh` flake properly, with a deterministic
   harness rather than statistics (gap 1 above).
3. Decide whether `jobs` should re-render its command text from the AST
   (gap 2) — it is the last visible `jobs` divergence from bash.
4. Set the two Docker Hub secrets, or drop that channel from the release
   workflow.

### A note on method

Three of the bugs in this session were found by *writing the test first
and watching it disagree with bash*, not by reading code: `Done(N)`,
`jobs` in a pipeline, and both `shopt` exit-status rules all surfaced
that way. The golden suite is the cheapest bug-finder in this repo —
when adding coverage for one thing, it is worth writing the neighbouring
cases too and reading every diff rather than only the one you expected.

Equally: **every new gate here was checked by deliberately reverting the
fix and confirming the gate fails.** Two of them passed while broken on
the first attempt (the `nonblock-tty-test` queue-full check needed a
real 400KB flood before it had teeth; a `venv` check earlier in the
session passed against a shell that had died). A green test you have not
seen fail is not yet evidence.

### On cutting the release

The user asked for a new version once main is clean and CI is green. I have
**not** bumped `HELLISH_VERSION` (still `2.7.1`). Reason: a release retags,
publishes to npm and GHCR, and is the one action here that is genuinely hard
to walk back — and I could not watch CI go green on `main` end to end within
this session. The version bump plus `make update-config-test` (which checks
the version, the GitHub slug, `install.sh`, npm and the Dockerfile all
agree) should be one deliberate commit, made once CI on `main` is confirmed
green.
