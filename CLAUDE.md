# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

`hellish` is a from-scratch, almost-POSIX shell written in C (a 42 project grown
well past the subject). It implements pipelines, redirects, here-documents,
subshells, process substitution, job control, functions, arithmetic, globbing,
and the full parameter-expansion pipeline.

## Build

Everything goes through the root `Makefile`. Two independent knobs shape the build:

- **`OPT`** — optimization. Unset → `-O0 -g3` + ASan/LeakSanitizer. `OPT=1` → `-O3 -flto`, no sanitizers.
- **`SAFE`** — allocator backend. `SAFE=1` → libc `malloc`/`free`. `SAFE=0` → the custom `ft_malloc` heap in `vendor/libft`.

`SAFE` defaults track the mode: **debug → `SAFE=1`** (so ASan stays meaningful), **`OPT=1` → `SAFE=0`** (exercise `ft_malloc`). An explicit `SAFE=...` on the command line always wins.

```sh
make              # debug: -O0 -g3, ASan+LSan, libc allocator -> build/bin/hellish
make OPT=1        # optimized: -O3 -flto, ft_malloc (the daily-driver / benchmark build)
make re           # fclean + rebuild (uses two sub-makes to avoid a -j race)
make clean        # remove object files
make fclean       # remove objects, binary, and BOTH libft build trees
```

The binary is always `build/bin/hellish`. Object trees are split per mode
(`build/obj` vs `build/obj-opt`) and libft is built into a per-`SAFE` tree
(`vendor/libft/build-libc` vs `build-ft`) so flipping a knob never reuses stale
objects. The build prints which allocator it picked (`safe_banner`).

**Submodules are required.** `vendor/libft` (stdlib + `ft_malloc`) and
`vendor/scripts` (dev tooling). After a non-recursive clone:
`git submodule update --init --recursive`.

## The two allocators

Every allocation goes through one macro family — **`xmalloc` / `xcalloc` / `xfree`**
— that resolves *at compile time* to libc (`SAFE=1`) or `ft_malloc` (`SAFE=0`).
Never call raw `malloc`/`free` in shell code; use the `x*` macros so both
backends stay swappable. The two heaps do **not** share memory, so a pointer
must always be freed on the heap it came from. Both backends must pass the
entire suite identically — behaviour is invariant, only speed and debug tooling differ.

For leak checking: `SAFE=1` is where ASan/LSan are meaningful. On `SAFE=0`,
ASan is blind, so use `ft_malloc`'s own oracle:
`HELLISH_ALLOC_STATS=1 ./build/bin/hellish script.sh` prints live bytes at exit.

## Test & quality gates

The test model is a **golden diff against `bash --posix`**: each test category
is a plain file of one-command-per-line cases under `tests/`; the harness runs
each through `hellish -c` *and* `bash --posix`, then compares stdout, exit
status, and any files written.

```sh
make test                          # full suite (relinks debug build first)
cd tests && ./tester redir pipe    # run specific category files only
cd tests && ./tester -v redir      # verbose (show diffs for failures)
cd tests && ./verify_alloc.sh      # build BOTH allocators; prove output parity + leak-cleanliness
make bench                         # speed vs bash --posix (always builds OPT=1); ROUNDS=, BENCH=micro
make norm                          # 42 norminette over src/ incs/ tests/ (notices are not failures)
```

`tests/tester` is a parallel harness; with no args it runs a default category
list (see the top of the script). `verify_alloc.sh` is the allocator-parity
gate: it builds `SAFE=1` and `SAFE=0` and checks both match bash and leak
cleanly (forked children — cmdsub/pipeline/subshell — produce expected LSan
noise that is *not* gated).

## Running it

```sh
./build/bin/hellish                       # interactive
./build/bin/hellish script.sh             # run a script file
./build/bin/hellish -c 'echo hi'          # run a command string
echo 'echo piped' | ./build/bin/hellish   # non-TTY / piped input
./build/bin/hellish --debug=lexer --debug=parser --debug=ast script.sh   # composable debug views
```

`~/.hellishrc` (the `.bashrc` analogue) is sourced **only** in interactive
sessions — never for `-c`, scripts, or piped input. This guard matters: without
it, test runs would silently inherit the dotfile.

## Architecture

The pipeline, one stage per directory under `src/`:

```
input → lexer → parser (AST) → word reparser → heredoc → expander → executor
```

Everything orbits one struct — **`t_shell` in `incs/shell.h`**, the single
source of truth for a running shell (env, AST, options, jobs, aliases, cmd
cache, heredoc state, positional params, traps…). There is exactly one
`t_shell` alive; subshells `fork` and the child gets a copy. Any subsystem takes
a `t_shell *` and reads/writes fields here.

- **`src/core`** — `main()` (in `shell.c`, despite the banner), the REPL, startup/teardown, option parsing (`opt.c`). The REPL frees the AST/redirects/input/heredoc scratch every turn — that per-command teardown is what keeps memory flat over long sessions.
- **`src/lexer`** — byte string → `t_token` stream (slices into the input buffer, not copies). Returns a "more input" prompt for incomplete constructs (open quotes/subshells) so multiline input works. Crafting-Interpreters-style hand-written scanner.
- **`src/parsing`** — token stream → `t_ast_node` tree via mutually-recursive grammar functions. Communicates readiness via a status protocol: `RES_OK` / `RES_GETMOREINPUT` / `RES_ERR`. Runs `reparse_words` / `reparse_assignment_words` post-passes.
- **`src/heredoc`** — gathers here-doc bodies before execution.
- **`src/expander`** (+ `src/word_splitting`, `src/glob`, `src/arith`) — applies shell semantics so the executor never has to: parameter/command/arithmetic substitution, tilde, word splitting on `IFS`, pathname globbing, brace expansion, assignments. Turns the syntactic tree into concrete strings/FDs.
- **`src/execution`** — the orchestrator: decides what runs in the current process (builtins that mutate the shell: `cd`, `export`, `exit`) vs a child (pipelines, externals, subshells), wires redirections/pipes/process-substitutions, and propagates statuses into `$?`, `&&`/`||`, `set -e`, `pipefail`.
- **`src/builtins`** — the 47 builtins.
- **`src/job_control`** — `&`, `jobs`, `fg`, `bg`, `wait`, `$!`; background reaping and the finished-job status ring (`bg_done`).
- **`src/infrastructure`** — input/readline integration, history (+ `!`-expansion), the prompt, the banner/intro, and the background update checker.
- **`src/environment`**, **`src/alias`**, **`src/completion`**, **`src/editing`**, **`src/helpers`** — the variable store, alias table, tab completion, vi/emacs editing modes, and shared utilities (e.g. `x_getcwd`).

Each `src/<module>/` has its own `README.md` explaining the *why*; read those
before changing a module. Public headers live in `incs/` (and `incs/public/`);
each module also keeps a private `*_private.h`.

Performance notes baked into `t_shell`: an **argv slab pool** (`argv_pool`,
depth-indexed) gives simple commands a zero-malloc fast path; positional
parameters live outside the env (`t_pos`) so function calls swap them in O(1).

## Conventions

- **Commits follow Conventional Commits** — a `commit-msg` hook enforces it: `type(scope): imperative subject` where type ∈ `feat|fix|refactor|perf|test|docs|chore|style` and scope is the module (`lexer`, `parser`, `expander`, `executor`, `builtins`, `alloc`, `glob`, `heredoc`, `job-control`, …). Install hooks with `./vendor/scripts/install-hooks.sh`.
- **Branches** use a `type/short-description` slug (`fix/heredoc-eof`, `feat/process-substitution`).
- **PRs target `develop`, not `main`.** Both `main` and `develop` are protected — never push to them directly.
- **Every fix ships with a test** covering the bug (and ideally its neighbours), added to the relevant `tests/` category file. Make `make norm`, the suite, and ASan all green before opening a PR.
- Code is heavily, *humanly* commented (the *why* / the trick / the gotcha). Match that density and the 42 norm when editing.
- Bugs in `vendor/libft` or `ft_malloc` are fixed in **those** submodule repos, not here.
