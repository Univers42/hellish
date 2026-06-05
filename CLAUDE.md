# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`hellish` is a from-scratch, POSIX-shaped interactive shell written in C, built as a 42 school project (a `42sh`/`minishell` extension). It is structured as a pipeline of API-like modules: input → lexer → parser (AST) → word reparser → heredoc prescan → expander → executor. The single source of truth for a running instance is `t_shell` (`incs/shell.h`), threaded through every subsystem; see [src/core/README.md](src/core/README.md) for a field-by-field breakdown.

The produced binary is **`build/bin/hellish`** (the Makefile's `BAPTIZE_SHELL`). README references to `minishell` are stale — the binary is `hellish`.

## Build & run

```sh
make                 # default: debug + AddressSanitizer + LeakSanitizer, -O0 -g3, single-job
make OPT=1           # optimized: -O3 -flto -ffast-math, parallel (-j nproc), NDEBUG — use for benchmarks/daily use
make re              # fclean + build
make clean / fclean  # remove objects / objects+binary+libft+build
./build/bin/hellish  # run it
```

Key build facts:
- **Default build runs under ASan+LeakSanitizer.** Leaks and UB surface at runtime; several test runners count any `LeakSanitizer` report as a failure. Build `OPT=1` for a clean, fast binary.
- **Debug and OPT use separate object trees** (`build/obj` vs `build/obj-opt`) but share the binary path. Make won't relink on a flag-only change, so `make test`/`make bench` force-remove the binary first to guarantee the right build is timed/tested.
- `-Werror`, `-D_XOPEN_SOURCE=700`. Links against `-lreadline`.
- **libft is a git submodule** (`vendor/libft`, plus `vendor/scripts`). The root build compiles it first. If it appears empty/wiped, run `git submodule update --init --recursive` (helper scripts exist under `vendor/scripts/fix-libft-submodule*.sh`).

## Tests

The test model is a **golden diff against `bash --posix`**: each case runs `hellish -c "<cmd>"` and `bash --posix -c "<cmd>"`, strips ANSI/prompt/`exit` noise, and compares stdout, exit status, and any files written under `outfiles/`.

```sh
make test                       # relink debug build, then run the parallel harness
cd tests && ./tester            # run the default category set directly
cd tests && ./tester redir pipe # run specific category files (each is a file of test lines)
cd tests && ./tester -v redir   # verbose: show the diff for failures
```

- Category files live directly in `tests/` (e.g. `redir`, `pipe`, `arith`, `builtins`, `globbing`, `crazy1`, `regress_hellish`). The default set is listed at the top of [tests/tester](tests/tester).
- `tests/scripts/*.sh` are realistic small POSIX programs; `tests/hard/*.sh` are large ones. Both feed the benchmark corpus.
- `make bench [ROUNDS=7] [BENCH=micro|corpus|hard]` always builds `OPT=1` and reports per-test and geometric-mean speed ratios vs `bash --posix` (ratio > 1.00 ⇒ hellish faster). See [tests/benchmark](tests/benchmark).

## Norm (42)

This is a 42 project: the **42 Norm** applies to `src/`, `incs/`, and `tests/`. Run `make norm` (wraps `norminette`) and keep it passing. Practical constraints: functions ≤ 25 lines, ≤ 5 functions per file, no `for`/ternaries, specific brace/indent style. Every `.c`/`.h` carries the 42 header block — preserve it when editing and add it to new files. This is why logic is split across many small numbered files (`utils.c`, `utils2.c`, `expand_word3.c`, …) rather than fewer large ones; follow that pattern.

## Architecture: the processing pipeline

Input flows through these stages (each is a module with its own `README.md`):

1. **`src/core`** — entry (`shell.c` → `main`/`repl_shell`), `t_shell` init/teardown (`on.c`/`init.c`/`off`), CLI option parsing into the `option_flags` bitmask (`opt.c`). The REPL reinitializes `input`, calls `parse_and_execute_input`, then frees per-command state (AST, redirects, input) each iteration.
2. **`src/infrastructure`** — buffered readline wrapper, multi-line input, prompt rendering (ANSI/multibyte-aware), history file in `$HOME`, input-mode handling (TTY / file / `-c` / non-TTY).
3. **`src/lexer`** — cursor-based tokenizer with a longest-match operator table (`||`, `&&`, `<<-`, `<(`, `>(`, `[[`, …). `--debug=lexer` prints token tables.
4. **`src/parsing`** — hand-written recursive parser building a `t_ast_node` tree (lists, pipelines, commands, subshells, redirects, `if`/`for`/`while`/`case`/functions, process subs). Uses a `parse_stack` + `RES_GETMOREINPUT`/`RES_ERR` protocol to drive multi-line prompts. `--debug=parser` / `--debug=ast`.
5. **`src/word_splitting`** — reparses coarse `AST_WORD` nodes into quote/envvar subtokens (`TT_SQWORD`, `TT_DQWORD`, `TT_ENVVAR`, …) and re-tags `NAME=value` as `AST_ASSIGNMENT_WORD`.
6. **`src/heredoc`** — pre-scans the AST for `<<`/`<<-`, reads bodies (optionally expanding `$`), wires temp files into redirects before execution.
7. **`src/expander`** — walks the AST applying shell semantics in classic order: brace expansion, tilde, parameter/env expansion, command substitution (`src/expander/expand_cmd_sub.c`), arithmetic substitution (delegating to `src/arith`), word splitting (IFS), and globbing (delegating to `src/glob`). Produces `t_executable_cmd` structs.
8. **`src/execution`** — runs the tree: pipelines, simple commands, builtins vs `execve`, subshells, control flow, functions (`func_scope`), background jobs, process-substitution lifecycle. `find_cmd_path*.c` resolves PATH (with the `hash` cache in `src/builtins`).

Supporting modules: `src/environment` (the `t_vec_env` view, decoupled from global `environ`), `src/glob`, `src/arith`, `src/alias`, `src/job_control`, `src/completion`, `src/editing`, `src/helpers`.

## Common change patterns

- **New builtin:** add the name→function entries in `fill_builtin_hash1`/`fill_builtin_hash2` in [src/builtins/hash_builtins_dispatch.c](src/builtins/hash_builtins_dispatch.c), implement under `src/builtins/`, and declare the prototype in `builtins_private.h` / `incs/ft_builtins.h`. Lookup goes through `builtin_func(name)`.
- **New syntax/operator:** extend the lexer operator/keyword tables (`src/lexer/tables.c`, `keywords*.c`) and the parser grammar (`src/parsing/`).
- **New expansion behavior:** patch `src/expander` (or the `src/word_splitting` reparser); avoid touching parse/exec.
- **New field on `t_shell`:** follow the ownership/lifetime rules in [src/core/README.md §8](src/core/README.md) — zero-init via `shell_init()`, set non-zero defaults in `on()`, free per-command in `repl_shell()` or per-session in `off()`/`exit_clean()`, and add a `free_*` call so ASan stays clean.

Process-aware cleanup: `exit_clean()` compares the live PID against `state->pid` so a forked child that inherited `t_shell` exits without freeing/saving the parent's global state or history.
