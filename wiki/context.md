# Session context — 2026-08-22 (portability)

Working notes for whoever picks this up next, from another workspace or a
later session. The repo history is authoritative; this file records the
things history cannot tell you — why a thing was done, what was measured,
what was deliberately *not* done, and what is still open.

Branch to resume from: **`develop`**. Released version: **2.8.2**.

The previous note on this file covered the issue-fixing session (#27, #32,
#34, #42 and friends). All of those are closed; the tracker is empty. This
note replaces it and covers the cross-platform work.

---

## What this session was

One request, repeated: make the CI failures reproducible, gate them, fix
them, then merge and release. The failures were on the two rungs nobody can
build locally — macOS arm64 and WSL — which changes the method more than it
sounds like it should.

**You cannot iterate on a platform you cannot compile for.** Every macOS
defect costs one CI round trip of roughly twenty minutes, and they arrive
strictly one at a time, each invisible until the one before it is fixed. So
the discipline that paid off was: after each fix, ask *what else in the tree
makes the same assumption* — and gate the assumption, not the symptom.

---

## The macOS list, in the order they appeared

Six, each hidden behind the last. The shape of this list is the argument for
the rung existing at all.

| # | Symptom | Actual cause |
|---|---|---|
| 1 | `no member named 'st_mtim'` | Darwin spells it `st_mtimespec` |
| 2 | `undeclared identifier 'SIGPWR'` / `SIGRTMAX` | Linux-only signals; realtime signals are optional in POSIX |
| 3 | `bcopy` conflict | it is a fortified *macro* on Darwin, not a function |
| 4 | `-Werror=sign-compare` in `mascot_anim.c` | `MB_CUR_MAX` is `size_t` on glibc and `int` on Darwin |
| 5 | `Undefined symbols: _get_original_tty_job_signals` | declared and called, **never defined**, for months |
| 6 | `Undefined symbols: _malloc_live_bytes` | weak undefined ref — legal on ELF, a link error on Mach-O |
| 7 | `process substitution` produced `''` | `/proc/self/exe` exec'd literally; no procfs on Darwin |
| 8 | `redefinition of enumerator 'FALSE'` | `<mach-o/dyld.h>` vs libft's `enum e_bool` |

(#5 and #6 are the interesting ones. Neither is a macOS bug.)

### #5 was a real bug everywhere

`get_original_tty_job_signals()` was in libft's `trap.h`, called by
`initialize_traps()`, and defined by no translation unit. It stayed green on
Linux for months because **GNU ld defaults to `--allow-shlib-undefined`**: a
shared library may keep undefined symbols and hope the loader finds them.
Apple's linker does not. So Darwin was simply the first linker that told us
we were shipping a library whose contract was a lie — a crash waiting for
the first caller.

Implemented in libft with bash's semantics. Two details worth keeping:

- The fetch is **non-destructive**. `get_orig_sig()` reads a disposition by
  installing `SIG_DFL` and keeping what came back — fine for SIGQUIT and
  SIGTERM, which are re-armed immediately, and wrong for the tty signals: a
  shell left with `SIGTSTP` at `SIG_DFL` suspends itself the moment a child
  touches the terminal. So: swap, look, swap back.
- Non-interactive shells do not ask the kernel at all. Nobody installed
  these on our behalf, so reading them records whatever the parent left
  behind and propagates it into every child. `SIG_DFL` is both what bash
  records and what a child expects.

### #6 was ELF-only cleverness

`alloc_stats.c` asked "was ft_malloc linked in?" at *link* time — a weak
undefined reference plus `-Wl,-u` to drag the archive member in — so the
file needed no `-D`. On ELF an undefined weak symbol resolves to NULL; on
Mach-O, `__attribute__((weak))` on a **declaration** is a weak *definition*,
so Apple's linker wanted a body.

Now `-DHAVE_ALLOC_ORACLE`, from the Makefile, which already knew the answer.
The `-Wl,-u` went **with** the weak ref rather than staying beside it: a
*strong* reference pulls an archive member on its own; only a weak one does
not.

### #7 was in four places, and only one of them failed

`<(cmd)` / `>(cmd)` re-exec the shell and did it through the literal
`/proc/self/exe` — twice, under two names (`PATH_HELLISH`, `PROC_SELF_EXE`)
that expanded to the same string, so the second attempt had always been dead
code. The same assumption was also in the ENOEXEC script-interpreter
fallback and **both halves of the update machinery's "where am I?"** — which
meant every non-Linux install classified as `ORIGIN_BINARY`, so `update`
would have offered to overwrite a source checkout as though it were a
downloaded binary.

`self_exe_path()` (`src/platform/posix/self_exe.c`) is the one door now.

**`_NSGetExecutablePath` has two traps.** It takes its buffer size *by
pointer* (rewritten with the size needed on overflow), so passing by value
compiles and corrupts the stack. And it does **not** promise an absolute,
resolved path — it returns the path the process was exec'd *through*, which
may be relative. Caching a relative one is a bug with a long fuse: correct
until the first `cd`. Apple's docs say to `realpath()` it; we do.

---

## Tests added, and how to trust them

Every gate here was verified non-vacuous by deliberately reverting the fix
and confirming it fails. Two of them **passed while broken on the first
attempt**, which is the whole reason that step is not optional:

| Gate | What it asks | First attempt |
|---|---|---|
| `tests/link_closure_test.py` | GNU ld the question Apple's ld asks by default (`--no-undefined --whole-archive`) | measured against the **archives** and passed while broken — a leftover SAFE=0 `libft.a` defined the very symbol a SAFE=1 link cannot see. Now measured against the objects alone. |
| `tests/crlf_hygiene_test.py` | nothing executed is stored CRLF, **and** every such file is covered by a rule | the first half alone passes on a repo with *no* `.gitattributes` — it would have passed on the commit that broke WSL. The coverage half is the one with teeth. |
| `tests/linux_only_apis_test.py` | `/proc/self/exe` named in exactly one file; `<mach-o/dyld.h>` never included | its comment filter matched `^\s*(/\*\|\*\|//)` and flagged five prose lines — the interesting mentions are on **continuation** lines of a `/* */` paragraph, which start with plain spaces and match no comment pattern at all. |

All three are discovered automatically by `tests/pty_suite.sh` (it globs
`tests/*.py`), so they need no Makefile or CI wiring.

**The reproduce-on-Linux trick is the point.** Each of these fails on the
unfixed code, on this machine, in about a second, with no Mac and no Windows
runner. A grep is a poor substitute for a compiler and a very good
substitute for a twenty-minute queue.

---

## WSL

Two separate things, and the first masked the second.

1. **CRLF.** GitHub's Windows runners check out with `core.autocrlf=true`,
   so `set -u` arrived as `set -u\r` and bash reported
   `set: - : invalid option` and a syntax error on a brace. `.gitattributes`
   now pins executed file types to `eol=lf`.

   Deliberately **not** `* text=auto eol=lf`: ~100 files here are stored with
   CRLF already, including fixtures under `tests/` whose bytes the golden
   suite compares exactly. `git add --renormalize .` was run to confirm the
   rule changes zero stored bytes.

2. **drvfs.** `$GITHUB_WORKSPACE` is on `D:`, which WSL reaches through a
   bridge out of the VM. A build is ~970 compiler invocations doing thousands
   of round trips; the job was still compiling when the 60-minute timeout
   killed it, and because the cancellation lands on the `cmd.exe` wrapper the
   log ends in `Terminate batch job (Y/N)?` with nothing above it. The tree
   is now copied to the distro's own ext4 first (one `tar | tar`, `.git`
   excluded) and built there, with an explicit in-step timeout so a stall
   produces a message instead of a cancelled batch job.

   A separate step keeps what is actually WSL-specific: running the shell
   against a drvfs directory (glob, read, `pwd`). Deliberately *not* the
   permission checks — `chmod 000` on drvfs without metadata support is a
   no-op, so a red there would be Windows telling the truth.

---

## Two golden cases removed for being environment-dependent

This keeps happening and is worth naming as a class.

- `sleep 0.01 & wait; jobs; echo end` — the arm64 runner's bash printed a
  `Done` line; the pinned bash on x86 printed it **0 / 65 times**. The
  *oracle* was architecture-dependent, so there was no right answer to pin.
  Replaced with `sleep 0.3 & jobs | grep -c Running; wait; echo end`.
- `issue13_bg_pid`: twelve cases racing `sleep 0.4` in the parent against a
  3-second child. One full-suite run in five failed 7/3790 on exactly these,
  with `sleep <defunct>` — the child had exited, so ~3s had passed where the
  script asked for 0.4s.

  **Not reproduced in isolation**: 25/25 clean under CPU saturation, 192/192
  under 64-way concurrency, four other full runs green. So the child's
  nominal life went 3s → 30s as a *widened margin* against the only
  mechanism the evidence fits, not as a diagnosis anyone can claim. Every one
  of the twelve was re-checked against pinned bash individually. Cases with
  no parent sleep were left alone — they have no wall-clock dependency.

---

## Conventions that bit me, recorded so they do not bite again

- **Do not rebuild while a pty suite is running.** I did it three times and
  each run reported failures that were mine, not the shell's. Finish source
  changes, *then* run `make pty-test`.
- **`make norm` always exits 0** — it only reports. Read the output.
- **Norminette rejects `(uint32_t)sizeof(x)`** (`SPC_AFTER_PAR`) and
  `(size_t)-1` (`SPC_BFR_OPERATOR`). The accepted spellings are a named
  constant and `(size_t) - 1` respectively — the latter is already the
  tree-wide idiom for `mbrtowc` error returns, and is what made #4 above a
  one-line fix.
- **A preprocessor block inside a function body** needs a blank line after
  the directive (`NL_AFTER_PREPROC`).
- **Python rewrites of `.c` files silently convert CRLF → LF.**
  `procsub_input.c` and `create_procsub_output.c` flipped this way; the real
  change is ~40 lines and the diff reads as ~170. Left as LF (the right
  direction for C sources) rather than churning them a second time.
- **`cd tests && …` persists across Bash tool calls.** Use absolute paths.
- **GitHub only honours `Closes #N` when a PR merges into the DEFAULT
  branch.** PRs into `develop` do not auto-close anything, which is why
  issues sat open in the previous session. Close them by hand.
- **`concurrency: cancel-in-progress` means every push kills the run you
  were watching.** Several "failures" this session were cancellations. Check
  the conclusion string before believing a red.

---

## Still open

1. **macOS is still `continue-on-error`.** The build now succeeds and the
   smoke reached 39/40; the last gap is fixed but has not yet been confirmed
   green on a real runner. Do not flip the flag until it has — a red required
   check that everyone learns to ignore is worse than an informational one.
2. **The version line is 2.7.x, not 2.8.x.** `2.8.0` and `2.8.1` exist in
   git history and were never tagged, so nothing ever shipped under those
   numbers; both were folded into a single **`v2.7.2`** release. Worth
   knowing if you find the old numbers in a commit message — they are not
   releases anyone can have installed. (Strict semver would have called the
   `pretty` builtin a minor bump; the owner chose to continue the 2.7 line.)
3. **Docker Hub secrets** are still unset; that channel of `release.yml`
   will no-op or fail. Set them or drop the channel.
4. **`tests/test_files/invalid_permission`** shows as permanently modified
   and `git` cannot even hash it (mode 000 by design). Pre-existing; ignore
   it in `git status`, and never `git add -A`.
