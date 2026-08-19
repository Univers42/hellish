# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`hellish` — a from-scratch, almost-POSIX shell in C (a 42 project grown well past the subject). It diffs byte-for-byte against `bash --posix`, runs under AddressSanitizer, stays 42-norm clean, and ships two compile-time-swappable allocators. The binary lands at `build/bin/hellish`.

Two git submodules are required: `vendor/libft` (stdlib + the `ft_malloc` allocator) and `vendor/scripts` (dev tooling). If a build fails on missing libft: `git submodule update --init --recursive`.

## Build matrix — OPT × SAFE

Everything goes through the root Makefile. Two independent knobs; the build banner announces the active allocator.

| Command | Optimization | Allocator | Sanitizers |
|---|---|---|---|
| `make` | `-O0 -g3` | libc (`SAFE=1`) | ASan + LeakSanitizer |
| `make OPT=1` | `-O3 -flto` | `ft_malloc` (`SAFE=0`) | none |
| `make SAFE=0` | `-O0 -g3` | `ft_malloc` | ASan (blind to ft_malloc) |
| `make OPT=1 SAFE=1` | `-O3 -flto` | libc | none |

- An explicit `SAFE=…` on the command line always wins over the per-mode default.
- libft builds into per-SAFE trees (`vendor/libft/build-libc` vs `build-ft`) and shell objects into per-mode trees (`build/obj` vs `build/obj-opt`), but the binary path is shared — `make test` / `make bench` force a relink so the binary always matches the requested mode.
- Flags are `-Wall -Wextra -Werror`: a warning is a build failure.

## Commands

```sh
make                # debug build (ASan+LSan, libc heap)
make OPT=1          # optimized build (ft_malloc heap)
make re             # fclean + rebuild, race-safe ordering (OPT/SAFE propagate)
make oracle         # build the PINNED bash 5.3.9 the suite is defined against
                    #   (cached in ~/bash-5.3.9; tests/tester auto-uses it)
make test           # full suite: ~3000 golden-diff cases vs bash --posix
make docker-suite   # same suite, hermetic: shell AND oracle from the image
make static         # static musl binary via docker -> dist/hellish-linux-<arch>
make static-verify  #   ... and prove it runs on THIS host
cd tests && ./tester redir pipe   # run only specific category files
cd tests && ./tester -v <file>    # verbose: show each case's diff
cd tests && ./verify_alloc.sh     # build BOTH heaps, prove output parity + no leaks
make bench          # speed vs bash --posix (always rebuilds OPT=1; ROUNDS=7 BENCH=micro)
make agnostic-bench # cross-shell speed matrix vs 8 shells, in docker (ROUNDS=… TIMEOUT_S=…)
make conformance    # third-party suites (Oils spec + mksh check.t) vs bash --posix AND dash
                    #   → bench/conformance.md; FAILS if pass count drops vs bench/baseline/
                    #   (UPDATE_BASELINE=1 to accept an improved count)
make perf           # dimension-split hyperfine bench (startup/parse/loops/forks/configure)
                    #   → bench/results.md; needs performance CPU governor, or BENCH_LAX=1
make geoman         # external 42 "minishell tester" as an independent cross-check (GEOMAN_URL=…)
make cli-opts-test  # shell's own argv parsing (-e, -o name, +c, --, $-) vs bash --posix
make net-redir-test # /dev/tcp and /dev/udp redirections vs bash --posix; brings up its
                    #   own TCP+UDP peer, so it needs python3 but no network access
make login-test     # login shell sources /etc/profile then ~/.profile; non-login sources neither
make update-test    # the whole update path against a LOCAL fake release server
                    #   (discovery, download, sha256, atomic replace, every rejection
                    #   path) plus a pty gate for the lazy header and for a discovered
                    #   update never disturbing a line being typed. No network.
make update-config-test # offline gate on the release/update config: the GitHub slug in
                    #   version.h must match the git remote, install.sh/npm/Dockerfile/docs
                    #   must agree with it, npm version must track HELLISH_VERSION
make norm           # 42 norminette over src/ incs/ tests/ — REPORTS ONLY, always exits 0
make docker-test    # build + smoke-test from source on Alpine/Debian/Ubuntu/Arch
make docker-alpine  # (also -debian/-ubuntu/-arch) interactive hellish in that container
make cd-zsh-test    # docker diff of the zsh-style `cd old new` extension vs real zsh
make cd-posix-test  # host-side check that --posix gates that extension off
make hist-test      # pty-driven check of cmdhist multiline history joining
make readline-test  # pty gate over every libreadline entry point (completion,
                    #   history recall, vi/emacs) — run before touching readline linkage
make anim-test      # pty-driven check that the prompt animation never clobbers pasted input
make prompt-atomic-test # pty gate: the prompt prefix must reach the tty in ONE write.
                    #   Byte-at-a-time let kernel echo of type-ahead split a colour
                    #   escape or a box-drawing glyph — run after touching rl.c
make widechar-test  # pty gate for non-ASCII: the shortened cwd in the prompt, and a
                    #   wrapping TYPED line starting with a 2-byte/1-column character
make git-prompt-test # pty gate: the prompt's git dirty check never blocks a render — cd
                    #   into a slow-scanning repo must prompt instantly, star arrives async
make charts         # regenerate bench/charts/*.svg from whatever harness output is on disk
                    #   (never re-measures — charting and measuring stay separate)
make rss            # peak-RSS dimension alone (needs a prior `make perf` build)
make my_shell       # sudo-install to /usr/bin + register as a login shell (OPT=1 SAFE=1)
make user-install   # the SUDO-LESS route (user-install.sh): ~/.local/bin + a
                    #   marker-delimited `exec hellish` hook in the login shell's rc.
                    #   chsh cannot be used without root (shell must be in /etc/shells),
                    #   so the rc hook is the mechanism. Idempotent; smoke-tests the
                    #   binary BEFORE writing the hook. PREFIX=/RC_TARGET=/STATIC=1.
make user-uninstall #   ... strip the hook and remove the binary
```

Run it: `./build/bin/hellish [script.sh]`, `-c 'cmd'`, or pipe into it (non-TTY). Debug views compose: `--debug=lexer --debug=parser --debug=ast`.

Runtime knobs (env): `HELLISH_ALLOC_STATS=1` (live bytes at exit), `HELLISH_NO_BANNER` / `HELLISH_NO_ANIM` (quiet startup — set these when scripting the shell), `HELLISH_NO_UPDATE_CHECK`, `HELLISH_DBG_SPANS` / `HELLISH_DBG_DQ` / `HELLISH_DBG_CYCLE`, `HELLISH_PROMPT_BENCH`.

Interactive shells (and only interactive shells) source `~/.hellishrc` — never scripts, `-c`, or piped input, so tests stay clean. `hellishrc.example` at the repo root documents the prompt escapes and knobs it can set.

## Test model (read before "fixing" a failure)

- **The bash in PATH is the specification, not an environment detail.** The suite
  diffs against `bash --posix`, and bash changes POSIX-visible behaviour between
  minor releases: 5.1/5.2 vs 5.3 disagree on ~35 of these cases (printf's status
  on numeric overflow and on an empty numeric arg, umask symbolic-mode
  validation, cd's status on too many operands and on an empty operand, whether
  `.*` matches `.` and `..`). Grading against an older bash reports those as
  hellish failures. **Run `make oracle` once**, then `make test`; `tests/tester`
  prefers that pinned 5.3.9 and warns loudly when it is grading against anything
  else. If a batch of failures appears in `builtins`/`globbing`/`cd_posix`
  without a matching source change, suspect oracle drift before touching C.

- A test category is a plain file in `tests/`, one command per line. The harness runs each line through `hellish -c` AND `bash --posix` and diffs **stdout + exit status + any files written**. stderr text is NOT diffed — error wording is free, exit codes are not.
- Adding a test = append a line to the right category file (or a new file), then `cd tests && ./tester <file>`. Cases too big for one line go in `tests/hard/*.sh` as whole programs.
- Gotcha: `make test` runs the default category list **hardcoded in the `test_lists=(…)` array at the top of `tests/tester`** — a brand-new category file must be added there or it silently never runs in the full suite.
- `tests/hard/` has its **own** runner, not wired into `make test`: `tests/hard/run.sh [script.sh …]` diffs each script vs `bash --posix` (output + exit status) and prints a best-of-3 timing next to each PASS. Run it explicitly after touching the executor or expander.
- The harness runs 16 cases in parallel (`xargs -P 16`) in a scratch copy of `tests/test_files` — a test that depends on cwd state left by an earlier line will not behave the way it reads.
- Every fix ships with a test — non-negotiable (CONTRIBUTING.md). Cover the neighbouring cases too.
- Leak checking: ASan/LSan is only meaningful on `SAFE=1`. On `SAFE=0` use the allocator's own oracle: `HELLISH_ALLOC_STATS=1 ./build/bin/hellish script.sh` (prints live bytes at exit).
- Gotcha: the tester `chmod 000`s `tests/test_files/invalid_permission` at startup and leaves it that way. Run `chmod 755 tests/test_files/invalid_permission` before git operations or they fail on the unreadable file.
- Conformance (`bench/`): first run fetches ~100MB of third-party suites into gitignored `bench/suites/` (idempotent). In `bench/conformance.md`, the consensus divergences — cases hellish fails while bash AND dash both pass — are the working repair queue. Update `bench/baseline/` only when the pass count improves. Fairness decisions live in `bench/METHODOLOGY.md`.

## Architecture

Conceptual pipeline: `input → lexer → parser (AST) → word reparser → heredoc → expander → executor`. Each stage is a module under `src/` with its own README. One struct — `t_shell` (`incs/shell.h`) — is the single source of truth; every subsystem takes `t_shell *`. Subshells fork and the child inherits a copy. `shell_init()` returns `(t_shell){0}`; any non-zero default belongs in `on()` (`src/core/on.c`).

| Stage | Entry point | Where |
|---|---|---|
| Lifecycle / REPL | `main` → `on()` → `repl_shell()` → `off()` | `src/core/shell.c` |
| One input cycle | `parse_and_execute_input()` | `src/infrastructure/input_utils2.c` |
| Lexer | `tokenizer()` — tokens are slices into the input buffer, no copies | `src/lexer/tokenizer.c` |
| Parser | `parse_tokens()` — recursive descent → `t_ast_node` tree | `src/parsing/parse_tokens.c` |
| Word reparser | `reparse_words()` — refines WORD nodes into subtokens; runs inside `parse_tokens` | `src/word_splitting/` |
| Heredoc | `gather_heredocs()` — pre-exec pass reads `<<` bodies to tmp files and rewrites them as plain file redirects | `src/heredoc/` |
| Expander | `expand_simple_command()` → `t_executable_cmd` (argv + pre-assigns) | `src/expander/expander_simple_cmd.c` |
| Executor | `execute_top_level()` → `execute_tree_node()` dispatch on node type | `src/execution/` |

**Load-bearing subtlety:** the pipeline is conceptual, not literal. Lexer+parser build the AST once per input line and heredocs are gathered once, but the **expander runs lazily, per simple command, during the executor's tree walk** (`src/execution/execute_simple_command.c`). The AST deliberately carries raw tokens (`incs/ast.h`) so the same subtree re-expands on every loop/function iteration without re-parsing. Every `execute_*` returns a `t_execution_state` (status + ctrl_c) so `$?` and Ctrl-C propagate up the walk.

**Command substitution has two paths.** `capture_subshell_output.c` first tries the forkless fast path `cmdsub_fast()` (`src/expander/cmdsub_fast.c`), which runs the body in-process when it is provably side-effect-free: a single whitelisted builtin (`echo`/`printf`/`pwd`/`true`/`:`), no redirects or control operators, no `${v=…}`, no `$((…))`. NULL means "not eligible" and the caller falls back to the normal fork. A cmdsub bug can live in either path; the eligibility scan (`cmdsub_fast2.c`) deliberately errs toward forking.

Other modules: `builtins` (51 builtin names, O(1) hash dispatch), `environment` (`t_vec_env` is the truth; `get_envp()` materializes `char **` for execve on demand), `arith` (self-contained arithmetic lexer/parser/eval serving `$((…))` plus the `(( ))` command and `for ((;;))` — those two are AST nodes run by `src/execution/execute_arith.c`), `glob` (called by the expander), `alias`, `job_control` (`state->job_table` behind `jobs`/`fg`/`bg`/`wait`), `completion` + `editing` (readline tab-completion, vi/emacs modes), `helpers` (the canonical free routines), `infrastructure` (input driver, prompt, history — including the bash-`cmdhist` multiline joiner in `history_join.c` — AST debug printers, centralized `error.c` messages).

### Allocator discipline

Every allocation goes through the `xmalloc` / `xcalloc` / `xfree` macro family (vendor/libft `ft_memory.h`), resolved at **compile time** to libc or `ft_malloc` via a Makefile-generated config header. Never call raw `malloc`/`free`; never let a pointer cross heaps. There is no libc-signature `xrealloc` (the wrapper takes an old-size argument).

### Builtins

Dispatch is `builtin_func(name)` in `src/builtins/hash_builtins_dispatch.c`. Wiring a new builtin: (1) implement `int builtin_x(t_shell *, t_vec argv)`, (2) prototype it in `incs/ft_builtins.h`, (3) add one `hash_set` line in `fill_builtin_hash1/2`. A builtin runs in-process only when it may mutate the parent shell (`modify_parent_ctx`); inside a pipeline it runs in the forked child. Lookup precedence: assignment-only → function → builtin → external.

### Cleanup discipline (ASan is a gate)

`repl_shell` frees the AST, redirects, input buffer, and heredoc scratch every iteration — memory stays flat over long scripts. Session teardown is the single path `free_all_state()` (`src/helpers/free_utils.c`) with deliberate ordering (env index freed late, word slab last). `exit_clean()` does full cleanup + history save only in the original process — a forked child holding a copy of `t_shell` must not free parent state. `off()` runs the EXIT trap *before* freeing the env (the trap may read variables).

## Code style — 42 Norm + house rules

- 42 Norm: ≤ 5 functions per file, ≤ 80 columns, the standard 42 header on every file, no `for` loops or ternaries. `make norm` must be clean on touched files — but note it only *reports* and always exits 0, so read its output rather than trusting the exit status, and confirm `norminette` is actually installed before believing a pass.
- **Comments go outside function bodies only** — block comments before a function or at file scope, never inside. Write the *why*, the trick, the gotcha, in the house voice; use a Doxygen block for genuinely subtle functions.
- **No new global variables.** New state goes in `t_shell` (or a struct you thread through). The few legacy globals (word slab, env index, readline completion iterators) are a known cleanup target, not a precedent — readline's callback signature is the one painful exception.
- Bugs rooted in `vendor/libft` or `ft_malloc` are fixed in the submodule's own repository (then the pointer is bumped here) — never patched around from the shell.

## Git workflow

- Branch from `develop`; PRs target `develop` (`main` and `develop` are protected). Branch names: `type/short-description`, e.g. `fix/heredoc-eof`, `perf/cmdsub-fast-path`.
- Conventional Commits, enforced by the commit-msg hook (`./vendor/scripts/install-hooks.sh`): `type(scope): imperative subject` with type ∈ feat/fix/refactor/perf/test/docs/chore/style and scope = the module (lexer, parser, expander, executor, builtins, alloc, glob, heredoc, job-control, …). **No AI co-author trailers.**
- Pre-PR gates, all green from a clean tree: `make re` AND `make re OPT=1` build clean; `make test` green; `cd tests && ./verify_alloc.sh` green on both heaps; no ASan/LSan reports on your cases; `make norm` clean.
- CI (`.github/workflows/ci.yml`) re-runs these gates: submodule hygiene, the build matrix, norminette (**enforced there**, unlike the local always-exit-0 `make norm`), and the full suite with leak checks. Releases ship via `release.yml`/`ghcr.yml` and an npm package (`npm/`, published as `hellish-shell`).

## Where the prose lives

Per-module design notes are in `src/<module>/README.md` (13 of them) — read the one for the stage you're touching before changing it. User-facing docs are in `wiki/` (`scripting.md`, `interactive.md`, `performance.md`, `builtins/`, `fixes/`, `troubleshoot/`). Benchmark methodology and fairness calls are in `bench/METHODOLOGY.md`, with known gaps in `bench/KNOWN_ISSUES.md`. `backlog.md` is the running list of what's not done yet.
