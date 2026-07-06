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
make test           # full suite: ~2500 golden-diff cases vs bash --posix
cd tests && ./tester redir pipe   # run only specific category files
cd tests && ./tester -v <file>    # verbose: show each case's diff
cd tests && ./verify_alloc.sh     # build BOTH heaps, prove output parity + no leaks
make bench          # speed vs bash --posix (always rebuilds OPT=1; ROUNDS=7 BENCH=micro)
make agnostic-bench # cross-shell speed matrix vs 8 shells, in docker (ROUNDS=… TIMEOUT_S=…)
make norm           # 42 norminette over src/ incs/ tests/
make docker-test    # build + smoke-test from source on Alpine/Debian/Ubuntu/Arch
make cd-zsh-test    # docker diff of the zsh-style `cd old new` extension vs real zsh
make cd-posix-test  # host-side check that --posix gates that extension off
```

Run it: `./build/bin/hellish [script.sh]`, `-c 'cmd'`, or pipe into it (non-TTY). Debug views compose: `--debug=lexer --debug=parser --debug=ast`.

## Test model (read before "fixing" a failure)

- A test category is a plain file in `tests/`, one command per line. The harness runs each line through `hellish -c` AND `bash --posix` and diffs **stdout + exit status + any files written**. stderr text is NOT diffed — error wording is free, exit codes are not.
- Adding a test = append a line to the right category file (or a new file), then `cd tests && ./tester <file>`. Larger programs go in `tests/scripts/*.sh` and `tests/hard/*.sh`, run as whole scripts vs bash.
- Every fix ships with a test — non-negotiable (CONTRIBUTING.md). Cover the neighbouring cases too.
- Leak checking: ASan/LSan is only meaningful on `SAFE=1`. On `SAFE=0` use the allocator's own oracle: `HELLISH_ALLOC_STATS=1 ./build/bin/hellish script.sh` (prints live bytes at exit).
- Gotcha: the tester `chmod 000`s `tests/test_files/invalid_permission` at startup and leaves it that way. Run `chmod 755 tests/test_files/invalid_permission` before git operations or they fail on the unreadable file.

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

Other modules: `builtins` (47 builtins, O(1) hash dispatch), `environment` (`t_vec_env` is the truth; `get_envp()` materializes `char **` for execve on demand), `arith` (self-contained `$((…))` lexer/parser/eval), `glob` (called by the expander), `alias`, `job_control` (`state->job_table` behind `jobs`/`fg`/`bg`/`wait`), `completion` + `editing` (readline tab-completion, vi/emacs modes), `helpers` (the canonical free routines), `infrastructure` (input driver, prompt, history, AST debug printers, centralized `error.c` messages).

### Allocator discipline

Every allocation goes through the `xmalloc` / `xcalloc` / `xfree` macro family (vendor/libft `ft_memory.h`), resolved at **compile time** to libc or `ft_malloc` via a Makefile-generated config header. Never call raw `malloc`/`free`; never let a pointer cross heaps. There is no libc-signature `xrealloc` (the wrapper takes an old-size argument).

### Builtins

Dispatch is `builtin_func(name)` in `src/builtins/hash_builtins_dispatch.c`. Wiring a new builtin: (1) implement `int builtin_x(t_shell *, t_vec argv)`, (2) prototype it in `incs/ft_builtins.h`, (3) add one `hash_set` line in `fill_builtin_hash1/2`. A builtin runs in-process only when it may mutate the parent shell (`modify_parent_ctx`); inside a pipeline it runs in the forked child. Lookup precedence: assignment-only → function → builtin → external.

### Cleanup discipline (ASan is a gate)

`repl_shell` frees the AST, redirects, input buffer, and heredoc scratch every iteration — memory stays flat over long scripts. Session teardown is the single path `free_all_state()` (`src/helpers/free_utils.c`) with deliberate ordering (env index freed late, word slab last). `exit_clean()` does full cleanup + history save only in the original process — a forked child holding a copy of `t_shell` must not free parent state. `off()` runs the EXIT trap *before* freeing the env (the trap may read variables).

## Code style — 42 Norm + house rules

- 42 Norm: ≤ 5 functions per file, ≤ 80 columns, the standard 42 header on every file, no `for` loops or ternaries. `make norm` must be clean on touched files.
- **Comments go outside function bodies only** — block comments before a function or at file scope, never inside. Write the *why*, the trick, the gotcha, in the house voice; use a Doxygen block for genuinely subtle functions.
- **No new global variables.** New state goes in `t_shell` (or a struct you thread through). The few legacy globals (word slab, env index, readline completion iterators) are a known cleanup target, not a precedent — readline's callback signature is the one painful exception.
- Bugs rooted in `vendor/libft` or `ft_malloc` are fixed in the submodule's own repository (then the pointer is bumped here) — never patched around from the shell.

## Git workflow

- Branch from `develop`; PRs target `develop` (`main` and `develop` are protected). Branch names: `type/short-description`, e.g. `fix/heredoc-eof`, `perf/cmdsub-fast-path`.
- Conventional Commits, enforced by the commit-msg hook (`./vendor/scripts/install-hooks.sh`): `type(scope): imperative subject` with type ∈ feat/fix/refactor/perf/test/docs/chore/style and scope = the module (lexer, parser, expander, executor, builtins, alloc, glob, heredoc, job-control, …). **No AI co-author trailers.**
- Pre-PR gates, all green from a clean tree: `make re` AND `make re OPT=1` build clean; `make test` green; `cd tests && ./verify_alloc.sh` green on both heaps; no ASan/LSan reports on your cases; `make norm` clean.
