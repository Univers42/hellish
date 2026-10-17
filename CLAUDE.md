# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

hellish is a from-scratch POSIX(+bashisms, + an opt-in zsh dialect) shell in C, written as a 42 project. Its behaviour is defined by diffing against a **pinned bash 5.3.9** (`bash --posix`) and, for the zsh dialect, a pinned zsh 5.9. In-depth docs: `DEV_DOC.md`, `CONTRIBUTING.md`, and `src/<module>/README.md` for each module.

## Setup

- Submodules are required: `vendor/libft` (libft + the `ft_malloc` allocator) and `vendor/scripts` (dev tooling). If the build can't find libft, run `git submodule update --init --recursive`.
- Build the oracles once: `make oracle` (→ `~/bash-5.3.9`) and `make zsh-oracle` (→ `~/zsh-5.9`). The golden suite compares against these, not the system bash.
- `make` with no target prints the help page, which lists every target with a description.

## Build

| Command | Result |
|---|---|
| `make all` | `MODE=debug`: `-O0 -g3`, ASan+LSan, libc malloc (`SAFE=1`) → `build/bin/hellish` |
| `make OPT=1 all` / `MODE=release` | `-O3 -flto`, `ft_malloc` (`SAFE=0`), no sanitizers |
| `MODE=relwithdebinfo` | `-O2 -g`, `ft_malloc` |
| `make re` / `make re OPT=1` | full rebuild; both must build cleanly before a PR |

- `-Wall -Wextra -Werror`: a warning fails the build. Add flags with `EXTRA_CFLAGS=…`. Passing `CFLAGS=…` replaces the whole flag set.
- Each mode/SAFE combination has its own object dir (`build/obj-<mode>-<alloc>`), but they all link to the same binary path.
- Sources are found with `find src` (excluding `src/platform/`), so a new `.c` file needs no Makefile edit. Only `src/platform/$(TARGET)/` (currently `posix`) is compiled in. Platform-specific code goes in per-file implementations there, not in `#ifdef`s.
- Debug knobs: `--debug=lexer|parser|ast`, `HELLISH_ALLOC_STATS=1` (live bytes at exit, the leak oracle on `SAFE=0`), `HELLISH_BANNER=0`, `HELLISH_NO_UPDATE_CHECK=1`.

## Test

A green `make test` alone isn't enough. The test layers are:

- **Golden suite**: `make test`. Each category in `tests/` is a file with one command per line. Every case runs through `hellish -c` and the pinned `bash --posix`, and stdout, exit status and any written files are diffed.
  - Run one category: `cd tests && ./tester <category>` (`-v` for verbose).
  - **A new category file must be added to `test_lists` in `tests/tester`**, or it silently never runs.
  - The tester `chmod 000`s `tests/test_files/invalid_permission`. If a run is interrupted, `chmod 755` it before git operations.
- **pty/interactive tests**: `make pty-test`. It discovers every `tests/*.py` automatically. Run a subset with `tests/pty_suite.sh -- <pattern>`. Each `.py` takes the shell path as its only `argv[1]`; keep it that way.
  - Many `make *-test` targets run one pty test; see `make help`.
- **Whole scripts**: `tests/run_scripts.sh`. **Hard corpus**: `tests/hard/run.sh`.
- **Allocator parity**: `cd tests && ./verify_alloc.sh` builds and diffs both `SAFE=1` and `SAFE=0` against bash. Run `make test-release` after touching memory code (ASan and `-O3` can disagree).
- `make arena-stress`: parse arena under ASan with tiny chunks. `make help-test`: every builtin must have a `help` entry.
- `make norm` runs 42 norminette (it only reports, and always exits 0).
- Pre-PR gates: `make re`, `make re OPT=1`, `make test`, `tests/run_scripts.sh`, `tests/hard/run.sh`, `make pty-test`, `cd tests && ./verify_alloc.sh`, `make norm`.

Leak checking: ASan/LSan only work on `SAFE=1`. On `SAFE=0` use `HELLISH_ALLOC_STATS=1`.

## Architecture

One command's path through the shell (`src/core/shell.c`: `main` → `on()` → `repl_shell` → `off()`):

1. **Input** (`src/infrastructure/input*.c`, readline in `rl_*`/`history*`): one unified line buffer (`t_rl`) for tty, file, `-c` and pipe input, plus multi-line joining and heredoc collection (`src/heredoc/`).
2. **Alias expansion** (`src/alias/`) produces `state->alias_exp`, which is what gets lexed.
3. **Lexer** (`src/lexer/`) and **parser** (`src/parsing/`) build the AST (`incs/ast.h`, `src/infrastructure/ast*.c`).
4. **Reparser** (`src/word_splitting/`) runs between parsing and expansion. It splits coarse `AST_WORD` tokens into quote- and variable-aware sub-tokens and re-tags assignment words.
5. **Expander** (`src/expander/`) handles parameter/arith/command/process substitution, IFS splitting, brace expansion, globbing (`src/glob/`) and redirect construction. `expand_zsh_*` is the zsh dialect.
6. **Execution** (`src/execution/`, fork/job parts in `src/platform/posix/`, `src/job_control/`) runs builtins (`src/builtins/`, dispatch table plus `help_data*.c`), functions, pipelines and compound commands.
7. After each REPL turn, the per-command AST, redirects and input are freed, so memory stays flat over long scripts.

**`t_shell`** (`incs/shell.h`) holds all the shell's state and is passed everywhere. New state goes there, or in a struct you pass down. **Never add a global variable.** The few legacy globals (word slab, parse arena, env index, readline completion iterators, dir stack) are scheduled for cleanup and aren't precedent.

**Memory model** (`src/helpers/README.md`):
- All allocation goes through `xmalloc`/`xfree`. They route to either libc or `ft_malloc` at compile time, and a pointer must never cross heaps.
- The **parse arena** (`parena_*`) is a bump allocator, open only around the main cycle's parse. Nested parses (`eval`, `source`, cmdsub bodies) fall through to the heap. `parena_free` is a no-op for arena pointers. Heap memory attached to an arena tree must call `parena_note_attach()`. Before sharing a struct between nodes, check `parena_owns()`; issue #94 was a double free caused by skipping that check.
- The **word slab** (`word_slab_push`) holds argv strings. Values that can escape into the environment (assignments, `for` values) must live on the general heap.
- Shutdown goes through `free_all_state`. Its ordering is deliberate.
- Bugs in libft/ft_malloc get fixed in the submodule's own repo, then the pointer gets bumped here. Don't work around them from the shell.

## Code style (42 Norm, enforced)

- At most 5 functions per file and 80 columns per line. No `for` loops and no ternaries. Every file starts with the standard 42 header. Because files hit the 5-function limit, modules have many `foo2.c`, `foo3.c` files; continue that pattern.
- Comments are block comments placed **before** a function or at file scope, never inside a function body. They explain the *why* (root cause, invariant, issue number). Complex functions get a Doxygen block (`@brief`/`@param`/`@return`, plus the hard part).

## Workflow

- Every fix ships with a test that covers the bug and nearby cases.
- Branch from `develop` (named `type/short-description`). PRs target `develop`; `main` and `develop` are protected.
- Commits use Conventional Commits (`type(scope): imperative subject`, types `feat|fix|refactor|perf|test|docs|chore|style`), enforced by the hook from `./vendor/scripts/install-hooks.sh`. The body explains the root cause and what the test proves.
- **Don't add `Co-authored-by:` trailers for AI tools.**
- After changing builtins, regenerate the wiki page with `make docs-builtins`.
