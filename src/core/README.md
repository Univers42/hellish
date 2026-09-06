# Core Module and `t_shell`

## 1. Overview

`src/core/` owns the lifecycle of one shell process:

- entry and the REPL (`shell.c`: `main`, `repl_shell`, `off`);
- bootstrap (`on.c`), with the input-source initialisers in `init.c`,
  `init2.c` and `helpers.c`;
- command-line options (`opt.c`, `opt2.c`, the `t_cli` scan);
- configuration loading (`profile.c`, `rc_load.c`, `rc_load_scan.c`,
  `rc_load_utils.c`) and the rc hook points (`hooks.c`, `hooks2.c`);
- the zsh dialect gate (`zsh_mode.c`, `zsh_mode2.c`);
- `shell2.c`, a stub pair (`setup_output_buffer` returns -1, output
  buffering was removed) kept so the REPL call sites need not change.

`core.h` is the module-private header; nothing outside `core/` includes it.

At the centre is `t_shell`, defined in `incs/shell.h` (roughly lines
306-456, about a hundred fields). It is the single source of truth for the
running instance: every subsystem reads or mutates it, and nothing outlives
it. `shell_init()`, a `static inline` at the bottom of the same header,
returns `(t_shell){0}`; every non-zero default is set explicitly in `on()`.

---

## 2. `t_shell` by group

The struct is commented in groups; this follows them. Field names are the
real ones -- grep `incs/shell.h` for the rest.

**I/O and input.** `input` is the raw line buffer for the current cycle;
`alias_exp` is the alias-expanded copy fed to the lexer (`alias_exp_owned`
says whether it is its own allocation or a borrow of `input.ctx`). `tree` is
the parsed AST. `metinp` is the input mode (`INP_RL`, `INP_FILE`, `INP_ARG`,
`INP_NOTTY` from `incs/sh_input.h`). `rl` (`t_rl`, `incs/prompt.h`) is the
unified line buffer for tty, file, `-c` string and pipe; `rl.should_update_ctx`
tells the prompt to recompute, `rl.no_compact` disables multi-line joining
for sources that are already complete. `rcfile` borrows `--rcfile=FILE`
from argv.

**Diagnostics.** `dft_ctx` and `ctx` are the name in error messages: the
basename of `argv[0]` at startup (`shell_basename`), replaced by the script
path in file mode (`update_ctx_from_file`).

**Special variables.** `pid` is `$$` as a string, set once in `on()` from
`ft_itoa(getpid())`; `last_bg_pid` is `$!`; `last_cmd_st` is `$?` and
points into the `statbuf` scratch (never freed); `last_cmd_st_exe` is the
structured `t_execution_state` (`status`, `pid`, `ctrl_c`;
`incs/public/executor_types.h`). `flagbuf`, `linebuf`, `statbuf` are small
in-struct scratch buffers so `$-`, `$LINENO`/`$RANDOM` and `$?` never
allocate. `start_sec` backs `$SECONDS`.

**Session.** `hist` (`t_history`), `cmd_no` (PS1's `\#`), `should_exit`,
`builtin_fatal` (a special builtin got a malformed request; read after
teardown by `strict_builtin_failed`), the `exit_warned`/`exit_attempt`
pair (stopped-job exit warning, issue #58), and `edit_mode`.

**Control flow.** `loop_break`, `loop_continue`, `loop_depth`,
`func_return`, `func_depth`, `source_depth`; `call_frames` is the
`t_call_frame` stack behind `FUNCNAME`/`BASH_SOURCE` (`frames_dirty` defers
the rebuild to the next read); `functions` + `func_index` (name -> slot,
O(1)) + `dead_funcs` (bodies unset during their own call).

**Positional parameters and scopes.** `pos` (`t_pos`: `$1..$N`, `$#`),
`local_saves` (the `t_scope_save` stack behind `local`), `for_snapshot`
(a live `"$@"` copy while a `for` runs), `getopts_pos`/`getopts_ref`.

**Options.** One `bool` per `set` letter (`opt_errexit`, `opt_nounset`,
`opt_xtrace`, `opt_noglob`, `opt_noclobber`, `opt_allexport`, `opt_noexec`,
`opt_verbose`, `opt_pipefail`, `opt_posix`, `opt_interactive`), the
`setopt` bitset (`enum e_setopt`: braceexpand, the zsh dialect bit
`SETOPT_ZSH`, ...), the `shopt` bitset (`SHOPT_*`), `errexit_off`, and
`option_flags` (`enum e_opt_flag`, section 4). The roster and
letter<->name mapping live in `src/builtins/set_opts4.c`.

**Environment.** `env` (`t_vec_env`, see `src/environment/README.md`),
`cwd` (cached logical directory, so `pwd` works in a deleted directory),
`path_dirs`/`path_dirs_src` (split-`$PATH` cache validated against the
exact PATH string), `readonly_vars`, `var_attrs` (`declare -i/-n`),
`arr_marks`, `dirstack`, `compspecs`, `aliases`, `cmd_cache`.

**Traps.** `traps[SH_NTRAP]` (handler strings, freed in
`free_all_state`), `trap_depth`, `traps_quiet` (pseudo traps inherited by a
subshell are listable but must not fire), `prompt_depth` (an error while
rendering PS1 must never end the session).

**Heredocs.** `redirects` (`t_vec_redir`, per command), `heredoc_idx`,
`hd_defer`, `cycle_has_hd`, `cycle_streamed`, `hd_src`/`hd_pos`/
`hd_stripped`, `gather_in_func`, `gathering_compound`.

**Expansion state.** `input_expanded`, `last_cmdsub_status` (`$?` inside
`$(...)`), `cmdsub_in_place` and `bg_exec_node` (the one command a
disposable `$( )` body or `cmd &` child may `execve` without forking again,
or the lone `( ... )` whose body a `&` child runs in place so the traps it
sets live in `$!`; the latter is the AST node's address so a nested fork
cannot mistake itself for the authorised command -- issue #13).

**Jobs and processes.** `bg_job_count`, the `bg_done` ring, `proc_subs`
(`t_procsub_entry`: `pid`, `fd`, and the `/proc/self/fd/<fd>` `path`
handed to the command), `job_table`, `shell_pid`/`shell_pgid`/`fg_pgid`,
`jobctl`, `pal_procs`. `prng` is the `$RANDOM` generator, seeded in `on()`
from `getpid() * 2654435761u ^ time(NULL)` so two shells started in the
same second diverge.

**Allocation.** `argv_pool[ARGV_POOL_DEPTH]` + `argv_pool_depth`: the
zero-malloc argv backing for simple commands (`src/helpers/free_utils2.c`).

---

## 3. Lifecycle: `main` -> `on` -> `repl_shell` -> `off`

**`main()` (`shell.c`).** A login shell arrives with `argv[0]` starting
with `-`; the dash is stripped but remembered, and `OPT_FLAG_LOGIN` is OR-ed
in *after* `on()` so option parsing cannot clear it. Then, in order:
`on()`, `tty_snapshot_save()`, `set_default_ps1()`, `source_profile()`,
`source_hellishrc()`, `maybe_spawn_update_check()`, `show_welcome()`,
`repl_shell()`, `off()`. There is no `return`: `off()` exits with the last
status.

**`on()` (`on.c`).** Order matters and the comment above it says why:

1. `set_unwind_sig()` -- Ctrl-C must be safe before anything reads input.
2. `*state = shell_init()`, then the non-zero defaults:
   `shopt = SHOPT_CHECKWINSIZE`.
3. `cli_parse()` fills a `t_cli`; `cli_early_exit()` handles `--version`
   and `--help` (exit 0) and an unrecognised option (`invalid option`,
   exit 2 like bash and dash), freeing state first so sanitizers stay quiet.
4. `init_rl_buffer()`: `buff_readline_init`, byte-granular `rl.buff`,
   `rl.edit_mode = 1` (emacs).
5. `pid`, `ctx`/`dft_ctx`, `set_cmd_status(state, res_status(0))`.
6. `init_cwd()`, `env = env_to_vec_env(state, envp)`,
   `ensure_essential_env_vars()`, and `$0 = argv[0]` verbatim (seeded here
   because only two of the four input modes assign it -- issue #14).
7. `init_tables()`: `jc_init`, `redirects`, `proc_subs`, `functions` +
   `func_index_init`, `call_frames`, `dead_funcs`, `job_table_init`,
   `alias_table_init`, `cmd_hash_init`, `edit_mode = 1`.
8. `cli_dispatch()` picks the input source (section 5).
9. `interactive_job_signals()`, `update_winsize_vars()`, the PRNG seed,
   `start_sec`.

**`repl_shell()` (`shell.c`).** While `!should_exit`: `open_cycle()`
(fresh `input`, `cmd_no++`, age the two exit flags, clear the unwind flag,
and -- interactively only -- run `$PROMPT_COMMAND` with `$?` restored
afterwards, then `HELLISH_PRECMD_FUNCS` and zsh `precmd` hooks);
`job_notify()`; `parse_and_execute_input()`
(`src/infrastructure/input_utils2.c`: one full read+lex+parse+execute
cycle); `run_pending_traps()`; then the per-cycle frees -- `free_redirects`,
`free_ast(&state->tree)`, `input`, `alias_exp`, `hd_src`, `hd_stripped`.
That pile of frees is the whole trick to staying leak-flat over an
hour-long script.

**`off()` (`shell.c`).** Snapshot `last_cmd_st_exe` *before* the EXIT trap
(POSIX: the shell exits with the status it had on reaching the trap, not
the trap body's), `run_exit_trap()`, `jobs_hangup_on_exit()`,
`tty_snapshot_restore()`, `free_env(&state->env)`, `free_all_state()`,
`forward_exit_status(final)`. The env is freed *after* the trap because the
trap may still read variables; `exit_clean()` in `src/builtins/exit.c`
follows the same order.

**Executor entry.** `parse_and_execute_input` hands the tree to
`execute_top_level()` (`src/execution/execute_top_level.c`), which builds
the root node with `create_exe_node(0, 1, &state->tree, true)`, pre-gathers
heredocs, calls `execute_tree_node()`, cleans up process substitutions and
stores `last_cmd_st_exe`/`last_cmd_ms`. `set_cmd_status()`
(`src/helpers/utils.c`) is what keeps `last_cmd_st` in step with it.

---

## 4. Command-line options (`opt.c`, `opt2.c`)

`option_flags` is a `uint32_t` of `enum e_opt_flag` bits (`incs/shell.h`):

```
OPT_FLAG_HELP  OPT_FLAG_VERBOSE  OPT_FLAG_POSIX  OPT_FLAG_LOGIN  OPT_FLAG_VERSION
OPT_FLAG_DEBUG_LEXER  OPT_FLAG_DEBUG_PARSER  OPT_FLAG_DEBUG_AST  OPT_FLAG_NORC
```

`cli_parse()` -> `cli_scan()` walks argv once and stops at the first
operand, leaving `cli->i` on it:

- `--` ends options; a lone `-` goes to `cli_lone_dash`.
- `--word` -> `cli_long_word`: `--posix`, `--verbose`, `--help`,
  `--debug=lexer|parser|ast`, `--login`, `--version`, `--norc`,
  `--rcfile=FILE` (stored in `state->rcfile`). Anything else sets
  `cli->err = 2`.
- `-xyz` / `+xyz` -> `cli_opt_word`: each letter is validated by
  `cli_known_short` against the `set` builtin's roster (`setopt_find`) plus
  the invocation-only `c`, `i`, `l`, and applied by `cli_apply_short` through
  the same `apply_flag_letters` the builtin uses. An `o` consumes the next
  argv word (`cli_take_o` -> `set_long_option`), so `-oo a b` sets two.

So `hellish -e -c '...'`, `hellish -o errexit script.sh` and
`hellish --debug=parser --debug=lexer --verbose script.sh` all parse like
`bash --posix ...`; flags are independent and simply OR together. `--posix`
sets `OPT_FLAG_POSIX` and `state->opt_posix` (also toggleable at runtime
with `set -o posix`), which disables extensions such as `cd old new`.

`cli_dispatch()` then routes: `-c` -> `init_arg`; an operand ->
`init_file`; `!isatty(0)` -> `init_stdin_notty`; otherwise `init_history`
(interactive). The argv pointer is biased so `init_arg`/`init_file` see the
string/script where they expect it regardless of how many option words came
first.

---

## 5. Input modes (`init.c`, `init2.c`, `helpers.c`)

- `INP_ARG` -- `init_arg`: `argv[2]` is pushed into `rl.buff`, an optional
  `argv[3]` becomes `$0` and `argv[4..]` become `$1..$N`
  (`set_argv_params`). Missing string -> usage error.
- `INP_FILE` -- `init_file`: open, `read_file_to_buffer` (appends a
  trailing `\n` so a last line without one does not stall the parser),
  `update_ctx_from_file` (ctx = script path; a `.zsh` path arms the zsh
  dialect, the same rule `source` and the rc loader use), then `frame_push`
  with the script path as typed, so `${BASH_SOURCE[0]}` equals `$0` the way
  bash's does and a function defined by the script records it as its origin
  (issue #118; the frame is the bottom of the stack for the life of the
  process). `-c` and piped input push nothing, as in bash. Open failure ->
  `handle_file_open_error`: `EISDIR` exits 127, otherwise
  `EXE_BASE_ERROR + errno`, like bash.
- `INP_NOTTY` -- `init_stdin_notty`: piped or redirected stdin, no prompts.
- `INP_RL` -- interactive readline with history.

`metinp` gates everything that differs between them: prompts, history, rc
loading, `exit` printing "exit", `COLUMNS`/`LINES`, hooks.

---

## 6. Configuration and hooks

`source_profile()` (`profile.c`) runs the login profile (e.g. `~/.profile`)
through `read_file` + `exec_string`; a missing file is not an error.
`source_hellishrc()` (`shell.c`) runs only under `INP_RL` -- `-c`, scripts
and piped input must never inherit your dotfile -- and loads, in order
(issue #70):

```
/etc/hellish/rc.d/*.hsh *.zsh                system-wide, lexical order
$XDG_CONFIG_HOME/hellish/rc.d/*.hsh *.zsh    yours, lexical order
$XDG_CONFIG_HOME/hellish/plugins/*/plugin.hsh
~/.hellishrc                                 LAST, so it always wins
```

`rc_load_all` (`rc_load.c`) owns the first three; `collect`/
`collect_plugins` (`rc_load_scan.c`) sort directory entries because
`readdir` order differs per machine. A `.zsh` module is read in the zsh
dialect and restored when it ends. `--norc` or `--rcfile=FILE` skips
`~/.hellishrc`. Every file is sourced with `frame_push`, so
`${BASH_SOURCE[0]}` names the file itself.

Hooks (`hooks.c`, `hooks2.c`): `HELLISH_PRECMD_FUNCS` and
`HELLISH_PREEXEC_FUNCS` are arrays of function *names*, not code strings,
so two plugins can both attach without one overwriting the other (#72).
zsh's own `precmd`/`preexec` functions and `precmd_functions`/
`preexec_functions` arrays are honoured too (#91), except when bash-preexec
is loaded and owns the convention. Interactive only.

---

## 7. The zsh dialect gate (`zsh_mode.c`, `zsh_mode2.c`)

zsh syntax (`${(f)x}`, `print -P`, `} always {`, unbraced `$arr[i]`) means
something else, or nothing, in the language the golden suite pins, so none
of it is reachable unless something arms `SETOPT_ZSH`. Exactly three things
can: `set -o zsh` / `set +o zsh`, `emulate zsh`, and sourcing (or running)
a `.zsh` path -- `zsh_path()` matches `.zsh`, `.zshrc`, `.zshenv`; `.sh`,
`.bash` and no extension are bash. Nothing is a heuristic on file content.

- `zsh_mode(state)` -- the predicate.
- `zsh_mode_swap(state, on)` -- set and return the previous value; also
  mirrors the bit into the glob layer's cell, so the mirror cannot be
  forgotten. The automatic arming from a `.zsh` file uses this and is
  restored when the file ends.
- `zsh_mode_pin(state, on)` -- an explicit request (`set -o zsh`, `emulate
  zsh` without `-L`) also rewrites the saved bit in every open frame, so no
  pending `frame_pop` can undo what was asked for by name.
- `zsh_mode_req(state, on, local)` -- what `emulate` calls.
- `zsh_arrays()` / `sub_to_index()` -- 1-based subscripts, unless
  `SETOPT_KSHARRAYS`.

---

## 8. Extending `t_shell` safely

1. **Initialisation is explicit.** `shell_init()` zeroes everything; a
   non-zero default belongs in `on()` (see `init_rl_buffer`, `init_tables`).
2. **Decide ownership and lifetime.** Per-command fields are freed at the
   bottom of `repl_shell`; per-session fields are freed in
   `free_all_state()` (`src/helpers/free_utils.c`) -- add your `free_*`
   there, in the documented order. Note that `env` is *not* part of it:
   `off()` and `exit_clean()` call `free_env` first.
3. **Remember the fork.** A child inherits a copy of `t_shell`.
   `exit_clean()` compares `getpid()` with `state->pid` and only the
   original process runs the teardown; anything you add that must not run
   twice needs the same guard.
4. **Avoid hidden globals.** A few legacy process-wide statics remain (the
   env index, the parse arena, the word slab, the ZLE tables) and are known
   cleanup targets; do not add more.
5. **Respect the input modes.** If a field changes user-visible behaviour,
   check `metinp` and mirror how bash differs between interactive, script,
   `-c` and piped input.
6. **New startup flags** get a bit in `enum e_opt_flag` and a branch in
   `cli_long_word`; runtime `set -o` options go through the roster in
   `src/builtins/set_opts4.c` instead, so the command line inherits them.
7. **Document it here.**
