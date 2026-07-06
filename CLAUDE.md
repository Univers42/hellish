# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

`hellish` is a from-scratch, almost-POSIX shell in C — a 42 project grown past
the subject. It reads like a real shell (pipelines, redirects, here-docs,
subshells, process substitution, job control, functions, arithmetic, globbing,
parameter expansion) and is built as a teaching lab: a clean staged pipeline,
heavy human commenting, and **two swappable allocators**. The README is the
user-facing tour; this file is the contributor's map.

## Build, run, test

Everything goes through the root `Makefile`. **Submodules are mandatory** —
`vendor/libft` (the libc-shim + `ft_malloc`) and `vendor/scripts` (dev tooling).
Clone with `--recursive` or run `git submodule update --init --recursive`, or the
build fails on a missing `libft.a` / `register_shell.sh`.

```sh
make                  # debug build  -> build/bin/hellish (SAFE=1 libc + ASan/LSan)
make OPT=1            # optimized     (SAFE=0 ft_malloc, -O3 -flto, no sanitizers)
make re               # fclean + rebuild (sequential sub-makes; NOT -j prerequisites)
make test             # full golden-diff suite vs `bash --posix` (relinks debug first)
make bench            # geomean speed vs `bash --posix` (builds OPT=1; raw
                      #   per-task timings land in tests/bench_results.txt)
make norm             # 42 norminette over src/ incs/ tests/
make hist-test        # interactive multi-line history regression (real pty)
make my_shell         # install as login shell (forces OPT=1 SAFE=1 first)

./build/bin/hellish                 # interactive
./build/bin/hellish script.sh       # run a script
./build/bin/hellish -c 'echo hi'    # command string
echo 'echo x' | ./build/bin/hellish # piped (non-tty)
./build/bin/hellish --debug=lexer --debug=parser --debug=ast script.sh
```

### The SAFE × OPT build matrix

Two orthogonal knobs. The build prints the active allocator (`safe_banner`) so it
is never a surprise.

| Command | Opt | Allocator | Sanitizers |
|---|---|---|---|
| `make` | `-O0 -g3` | libc (`SAFE=1`) | ASan + LSan |
| `make OPT=1` | `-O3 -flto` | **`ft_malloc`** (`SAFE=0`) | none |
| `make SAFE=0` | `-O0 -g3` | `ft_malloc` | ASan (blind to ft_malloc) |
| `make OPT=1 SAFE=1` | `-O3 -flto` | libc | none |

- **Default `SAFE` tracks mode**: debug ⇒ `SAFE=1` (so ASan stays meaningful),
  OPT ⇒ `SAFE=0`. An explicit `SAFE=` on the command line always wins.
- Object trees are split per mode (`build/obj` vs `build/obj-opt`); the binary
  path is shared, so `test`/`bench` `rm -f` the binary to force a relink.
- libft is rebuilt into a **per-SAFE tree** (`vendor/libft/build-libc` vs
  `build-ft`) so the two allocators never share objects or a stale archive.

### Running specific tests

The suite is a **golden diff against `bash --posix`** (see Testing below). To run
a subset instead of the whole thing:

```sh
cd tests && ./tester globbing            # one category file
cd tests && ./tester arith builtins redir # several
cd tests && ./tester -v globbing         # verbose (also shows stderr, still not gated)
cd tests && ./verify_alloc.sh            # build BOTH allocators, prove parity + no leaks
```

`tester` args are **category/test-list filenames** under `tests/` (line-delimited
`-c` cases). With no args it runs the default set (globbing, arith, builtins, redir,
var, pipe, compound, torture, … — 2616 cases; `torture` is the adversarial
category from the break-it sweep). Cases run 16-way parallel; a failing run leaves
its temp dir (`/tmp/sh42_tester_*`) for inspection.

Docker / cross-shell targets: `make docker-test` (build+smoke on Alpine/Debian/
Ubuntu/Arch), `make agnostic-bench` (race hellish vs bash/dash/zsh/ksh/… in one
image; `ROUNDS=` / `TIMEOUT_S=` overrides), `make cd-zsh-test` / `make cd-posix-test`
(the zsh-style two-arg `cd` extension, which the bash suite can't cover).

Interactive (real-pty) regressions the golden-diff suite can't reach:
`make hist-test` (multi-line history recall) and `make ai-test` (AI provider
shape + tuned completion bodies via a fake LLM server, REPL latency on a
blackholed backend, prompt-arrow duplication + live header reflow on resize,
ghost safety against multi-line history entries, and the empty-prompt
next-command prediction incl. the common-idiom fallback). `ai-test` needs only
the stdlib; the display/prediction checks additionally use `pyte` and are
skipped with a notice if it is absent.

## Architecture

One staged pipeline, one god struct. Each stage is a module under `src/` (most
with their own `README.md`).

```
input → lexer → parser (AST) → word reparser → heredoc gather → expander → executor
```

**`t_shell` (`incs/shell.h`, `struct s_shell`) is the single source of truth** —
~50 fields, passed by pointer into every stage. Exactly one is alive at a time;
subshells `fork()` and the child gets a copy. Zero-init via inline `shell_init()`
`(t_shell){0}`; any non-zero default must be set explicitly in `on()`.
(`incs/sh_state.h` holds a **stale duplicate** of the struct — `shell.h` is
canonical; ignore the copy. Likewise `src/core/README.md`'s struct listing lags
the code.)

### Control flow

- `main()` (`src/core/shell.c`) → `on()` bootstrap → `source_hellishrc()`
  (interactive only) → `repl_shell()` loop → `off()` teardown.
- `on()` (`src/core/on.c`) order is deliberate: install signals → `shell_init()`
  → parse options (`--posix` lifts `OPT_FLAG_POSIX` → `state->opt_posix`) → pid/
  ctx/cwd → `env_to_vec_env` → init tables → `mode_input()` → seed PRNG.
- Each REPL turn calls **`parse_and_execute_input()`** (`src/infrastructure/
  input_utils2.c`): drives the lexer↔parser feedback loop until a complete AST
  exists, runs the two word-reparse passes, then `execute_top_level()`. Then it
  **frees the per-command state**.
- Exit status flows executor → `state->last_cmd_st_exe`; `off()` calls
  `forward_exit_status()` so the process exit code is the last command's status
  (`main` has no explicit `return`).

### Input dispatch (`mode_input` in `on.c`, sets `metinp`)

Priority: `-c` string (`INP_ARG`) > script file (`INP_FILE`) > non-tty stdin
(`INP_NOTTY`) > interactive readline (`INP_RL`). `metinp` gates behavior
throughout — only `INP_RL` builds a prompt, sources `~/.hellishrc`, and treats
Ctrl-C as "newline and continue". Leading global flags are counted/skipped
before operand detection, so `hellish --posix -c '…'` works.

### Module map

| Module | Role |
|---|---|
| `core/` | Lifecycle (`main`/`on`/`repl_shell`/`off`); defines `t_shell`. |
| `infrastructure/` | REPL glue: input acquisition (all 4 modes), lex↔parse loop, prompts, history, AST free/clone, central error wording. |
| `lexer/` | `tokenizer()` → deque of non-owning `(start,len)` slices ending `TT_END`. Context-free; **caller must then `reclassify_keywords()`**. |
| `parsing/` | Recursive-descent parser → `t_ast_node` tree. Sole entry `parse_tokens()`. No expansion here. |
| `word_splitting/` | The "reparser": decomposes each raw `AST_WORD` into typed subtokens and promotes `KEY=val` to `AST_ASSIGNMENT_WORD`. |
| `heredoc/` | `<<`/`<<-` resolution. Two passes: a string pre-scan (strip bodies before tokenizing) and an AST pre-scan (`gather_heredocs`, materialize fds before any fork). |
| `expander/` | Turns an AST simple-command into `argv` + pre-assigns + redirects. Owns the expansion order (below). |
| `arith/` | Self-contained `$(( ))` / `(( ))` / `let` evaluator (64-bit signed); runs inside the expander. |
| `glob/` | Pathname expansion (`* ? [...]`); last step of word expansion. No-match ⇒ literal word. |
| `execution/` | The executor: walks the AST → fork/exec, builtins, pipelines, redirs, control flow, lists. |
| `builtins/` | ~45 builtins behind an O(1) hash dispatch (`builtin_func`). |
| `environment/` | Canonical variable store (`t_vec_env` + name→index hash), `$`-expansion, `envp[]` for execve. Replaces raw `environ`. |
| `alias/`, `completion/`, `editing/`, `job_control/`, `helpers/` | Cross-cutting: alias table, readline TAB-completion, vi/emacs mode shim, background-job table, low-level toolbox (teardown paths, word-slab allocator). |
| `ai/` | Optional LLM assist (interactive-only, `opt_ai`). `ai_request` pairs a task-tuned system prompt + decoding params (`ai_provider.c`/`ai_body.c`: completions get max_tokens 64, temp 0, stop `\n`, lite context) with shell context incl. `$HELLISH_LAST_STATUS` (`ai_context.c`), dispatching by provider (`ai_provider()`): OpenAI-compatible (Bearer) or Anthropic Messages (`x-api-key`, top-level `system`, `"text"` reply). Switch with `ai setup local\|openai\|anthropic\|groq\|openrouter`. **All network stays off the REPL parent** — the pro-tip worker double-forks and never probes; ghost suggestions fork too. **Prediction is local-first**: the empty prompt ghosts the most frequent historical successor of the last command (`rl_predict.c`, zero latency); the LLM ghost stays opt-in (`HELLISH_AI_SUGGEST`). |

### Expander order (load-bearing, `src/expander/`)

`brace → tilde → command-sub → parameter/$ → arith → IFS field-split → glob`.
**Quoting is not tracked by quote characters here** — it is encoded in the token
*type* by upstream passes (`TT_SQWORD`/`TT_DQWORD`/`TT_DQENVVAR` vs `TT_WORD`/
`TT_ENVVAR`), so quoted pieces never reach split/glob.

## The two allocators & memory model

Every allocation goes through one macro family that resolves **at compile time**:

- `xmalloc`/`xcalloc`/`xfree` (`vendor/libft/include/ft_memory.h`) → `xfn_*`
  wrappers → `fn_*` (`xalloc.h`) → either `ft_malloc/…` or libc `malloc/…`,
  switched by `HAVE_FT_MALLOC`, which libft's Makefile **generates** per-SAFE
  build (`xalloc_config.h`). No `-D` at call sites; flipping `SAFE` swaps the
  whole shell. (Note `xfn_realloc` is 3-arg — it takes the old size.)
- **ASan is meaningful only at `SAFE=1`.** At `SAFE=0`, use
  `HELLISH_ALLOC_STATS=1 ./build/bin/hellish script.sh` for ft_malloc live-byte
  accounting instead.
- The two heaps never share memory, so freeing a pointer on the wrong one
  corrupts the heap. **The one trap that bites everyone:** readline returns a
  **libc-malloc'd** string (it runs in a forked child in `src/infrastructure/
  rl.c`) — free it with libc `free()`, NEVER `xfree()`.

Lifetime classification is the core invariant — every new `t_shell` field is one
of:

- **Per-command** (freed every `repl_shell()` turn / in `finalize_parser_and_cleanup`):
  `input`, `tree` (`free_ast`), `redirects`, heredoc scratch. New per-cycle
  allocations must be freed here or they leak across a long script.
- **Per-session** (init in `on()`, freed in `off()`/`free_all_state`): `env`,
  `cwd`, `ctx`, `pid`, history, options, and all tables.
- **PID-guarded cleanup**: `exit_clean()` compares the live pid against
  `state->pid`; only the original process frees global state / saves history, so
  a forked child can't double-free the parent's `t_shell`.
- AST tokens are **non-owning slices** into the input buffer (which must outlive
  the tree); only tokens with the `allocated` bit own their text. `clone_ast` is
  shallow (borrows); paths that outlive the source buffer (command-sub, fork)
  need `deep_clone_ast`. The expander's `argv` comes from a process-wide
  **word slab** — free it with `word_free`, not `xfree`; functions store a
  deep-cloned AST so the source tree can be freed independently.

## POSIX-mode gating

`--posix` (startup) and `set -o posix` (runtime) both drive the boolean
`state->opt_posix`. Extensions check that flag **at the decision point** rather
than re-parsing flags. The canonical example: the zsh-style two-arg `cd old new`
works in normal mode but errors under posix mode (`src/builtins/core_builtins2.c`;
verify with `make cd-posix-test` / `make cd-zsh-test`).

## Testing model

Each case runs through both `hellish -c '…'` and `bash --posix -c '…'` from an
isolated dir, comparing **stdout** (ANSI-stripped), **exit status**, and **files
written**. All three must match. **stderr / error-message wording is NOT gated** —
do not waste effort matching bash's exact error text. The debug build runs under
ASan+LSan; `verify_alloc.sh` additionally proves output parity + leak-cleanliness
across `SAFE=1` and `SAFE=0` (the shell must report 0 LSan leaks at exit). Corpus
lives under `tests/`: `levelNN.sh`, category files, `scripts/`, `glob-zoo/`,
`stress/`, `hard/`, `test_files/`.

Setup gotcha: `tests/test_files/invalid_permission` is intentionally `chmod 000`
(the runner re-applies it). **Restore it to `chmod 755` before any git op**, or
git/cp on the fixture misbehaves.

## Conventions

- **42 norminette** clean: `make norm` over `src/ incs/ tests/` must pass.
- **Conventional Commits** enforced by a `commit-msg` hook
  (`type(scope): description`). Install hooks: `./vendor/scripts/install-hooks.sh`.
- PRs target **`develop`** (per `CONTRIBUTING.md`), not `main`. Every bug fix
  ships with a test that proves it; `make norm`, the suite, and ASan must be
  green before opening a PR.
- Bugs in `vendor/libft` or `ft_malloc` are fixed in **those submodule repos**,
  not here.

## Watch-outs (verified during analysis; READMEs sometimes lag the code)

- `src/alias/alias_expand.c` (`alias_expand_input`) is **dead code**. Real alias
  substitution is single-level only, in the executor (`apply_alias`,
  `src/execution/execute_simple_command2.c`); `ALIAS_MAX_DEPTH`/recursion don't
  exist. (`incs/alias.h` is unrelated typedefs — the real header is
  `incs/sh_alias.h`.)
- **`modify_parent_ctx`** is the executor's central invariant: builtins/functions
  mutate parent state (`cd`/`export`/`read`/`set`) only when it's true — i.e. the
  last/only stage of a pipeline. Inner pipeline stages fork, so `cd x | y` has no
  lasting effect, by design.
- Adding a builtin = **three edits in lockstep**: register in
  `fill_builtin_hash1/2` (`hash_builtins_dispatch.c`), prototype in
  `incs/ft_builtins.h`, add the `.c`. Miss the hash and it's silently "command
  not found". `test`/`[`/`[[` return `0=true,1=false,2=error` (POSIX, inverse of C).
- The lexer emits essentially only `TT_WORD` in practice (quotes/`$VAR` stay
  embedded); the richer `TT_*` types are resolved later by the reparser/expander.
- `arith` errors are **destructive in non-interactive mode**: `arith_fail` calls
  `exit_clean(127)` unless `metinp == INP_RL`. `glob/` has two match engines but
  only the directory-walker (`matches_pattern`) is live.
- **AI must never block the REPL**: `SO_*TIMEO` does *not* bound `connect()`
  (`ai_net.c` uses a non-blocking connect + `poll`), and `ai_tip_spawn` never
  probes the network in the parent — it just double-forks a throttled worker.
  Reintroducing a synchronous `ai_reachable()` on the prompt path brings back the
  intermittent multi-second `clear` stall. Guarded by `make ai-test`.
- **NEVER install a custom `rl_redisplay_function`**: it silently degrades
  readline's multi-row rendering — recalled multi-line history displays as `^J`
  soup and resize redraws stack the arrow line. The ghost text instead uses a
  `rl_getc_function` wrapper (erase before every key) + the idle event hook
  (paint on settle), keeping the DEFAULT redisplay (`rl_ai.c`/`rl_ghost.c`).
- **AI mode owns SIGWINCH** (`rl_resize.c` + `rl_header.c`): the prompt header's
  dash-fill is an `\x1e` marker resolved by the readline child at the live
  width, and re-rendered on zoom from the idle hook (with the event hook active
  readline waits in `select()`, so `rl_signal_event_hook` alone never fires).
  Background AI work must never fight the user: the tip worker defers when
  loadavg is high (`ai_guard.c`), caps its request at 8s, and the llama compose
  services run at low `cpu_shares`. Re-check `make ai-test` after touching any
  of this.
