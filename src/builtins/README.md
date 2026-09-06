# Builtins Module

## Overview

`src/builtins/` implements the commands hellish runs without an `execve`.
The module has grown far past the original seven: the dispatch table now
registers seventy names, covering the POSIX special builtins, most of bash's,
a set of zsh's, and two that are hellish's own (`pretty`, `update`). This file
explains how a name reaches its C function, when that function runs in the
parent versus a fork, how to add one, and the design of the core builtins
whose helpers are spread across many files.

## File organisation

There are ~150 files here, so a grouped map beats a tree:

- `hash_builtins_dispatch.c`, `hash_builtins_zsh.c` -- the dispatch table.
- `core_builtins.c` (`export`, `exit`, `:`, `echo`) and `core_builtins2.c`
  (`unset`, `pwd`, `cd`) -- the original core, thin over their helpers:
  `echo_flags.c`, `echo_help.c`; `exit.c`, `exit_helpers.c`, `exit_jobs.c`;
  `export_helpers.c`..`export_helpers3.c`, `collect_and_print_exported.c`;
  `cd_helpers1.c`..`cd_helpers6.c`; `try_unset.c`; `utils2.c`.
- `builtin_<name>*.c` -- one family per builtin (`builtin_read*.c`,
  `builtin_declare*.c`, `builtin_test*.c`, `builtin_zsh_*.c`, ...). Numeric
  suffixes exist only for the 42-norm five-functions-per-file cap.
- `set_opts*.c` -- the `set -o` roster, which the command-line parser in
  `src/core/opt2.c` reuses so the two can never disagree.
- `help.h`, `help_data.c`, `help_data2.c`, `help_list.c` -- the `help` index.
- `builtins_private.h` -- module-internal prototypes and the per-builtin
  option structs (`t_cdopt`, `t_rdopt`, `t_getopts`, ...). Public prototypes
  live in `incs/ft_builtins.h`.

## Dispatch

```c
typedef int	(*t_builtin_fn)(t_shell *state, t_vec argv);      /* incs/ft_builtins.h */
int			(*builtin_func(char *name))(t_shell *state, t_vec argv);
```

`builtin_func(name)` in `hash_builtins_dispatch.c` owns a `static t_hash`
that is filled exactly once, on the first call (the `!h.ctx` guard), by
`init_builtin_hash` -> `fill_builtin_hash1..3`. `fill_builtin_hash3` chains
into `fill_builtin_hash4` in `hash_builtins_zsh.c`, which chains into
`fill_builtin_hash5`. The split is purely the 25-line norm ceiling: all five
fill the same table, and new names go wherever there is room. Function
pointers are stored as `void *` and cast back on retrieval -- ugly but
correct on the targets we build for. `NULL` means "not a builtin", which is
what the executor branches on to decide whether to fork.

Registered names (one `hash_set` line each):

- `fill_builtin_hash1`: `echo export cd pushd popd [[ exit pwd unset type
  set shift : break continue eval . source true false umask command builtin`
- `fill_builtin_hash2`: `return getopts exec wait times trap readonly read
  test [ alias unalias hash jobs fg bg fc history let local kill printf
  ulimit update help`
- `fill_builtin_hash3`: `mapfile readarray declare typeset shopt pretty dirs`
- `fill_builtin_hash4` (zsh dialect): `setopt unsetopt emulate print autoload
  zmodload zstyle compdef colors vcs_info zle bindkey add-zsh-hook`
  (`zmodload` and `compdef` map to `builtin_zunsupported`)
- `fill_builtin_hash5`: `compgen complete` (programmable completion, #72)

Two things worth knowing about that list.

**`env` is deliberately not a builtin.** Real `env` execs its argument
(`env cmd`, `env -i cmd`) and prints the environment with no arguments;
`/usr/bin/env` does both, and registering a builtin only broke `env cmd`.
A `builtin_env` prototype lingers in `ft_builtins.h`; nothing defines or
registers it.

**The zsh names are registered unconditionally**, not behind `zsh_mode()`.
The reasoning in `hash_builtins_zsh.c`: a new NAME is additive -- a bash
script that never says `setopt` cannot tell it exists -- whereas a changed
MEANING for syntax that already parses is what has to be gated, which is why
the expander's zsh flags are gated and these are not.

### Adding a builtin

1. Implement `int builtin_x(t_shell *state, t_vec argv)` returning the exit
   status. `argv.ctx` is a `char **`; `argv[0]` is the command name.
2. Prototype it in `incs/ft_builtins.h`.
3. Add `hash_set(h, "x", (void *)builtin_x);` to whichever
   `fill_builtin_hash*` still has room under 25 lines.
4. Add a `t_help` entry (name, group, synopsis, summary) to `help_data.c`
   or `help_data2.c`. Not optional: `make help-test` (`tests/help_test.sh`)
   derives the expected set from the `hash_set` lines in
   `hash_builtins_dispatch.c` and fails on any name without an entry.
   `make docs-builtins` regenerates the wiki page from `help` output.

## In-process or forked?

Whether a builtin (or a shell function) runs in the parent is decided by one
flag on the executable node: `modify_parent_ctx`, a field of
`t_executable_node` (`incs/executor.h`), set at construction by
`create_exe_node(infd, outfd, node, modify_parent_ctx)` in
`src/execution/execution_private.h`. The top-level tree, loop bodies, `if`
branches and function bodies are all created with it `true`.

The pipeline executor is where it gets cleared. `prepare_child_exec` in
`src/execution/execute_pipeline.c` stamps every stage of a multi-stage
pipeline with `modify_parent_ctx = false` (its comment says "kept for the
last stage", but the assignment is unconditional), so `x | cd /tmp` forks
its `cd` exactly like bash without `lastpipe`. The single-stage fast path
`execute_pipeline_one` (`execute_pipeline2.c`) copies the template
unchanged, which is how a bare `cd` keeps running in the parent.

`dispatch_cmd` in `src/execution/execute_simple_command.c` then applies the
lookup order:

1. no command word -> `handle_assign_only` (the assignments persist only
   when `modify_parent_ctx` is set);
2. `argv[0] == ""` -> error;
3. a shell function (`func_lookup`) -> `handle_func_call` in the parent,
   only if `modify_parent_ctx`;
4. a builtin (`builtin_func`) -> `execute_builtin_cmd_fg`
   (`execute_builtin.c`) in the parent, only if `modify_parent_ctx`;
5. otherwise `execute_cmd_bg`, which forks and execs. A function or builtin
   whose flag was cleared falls through here and runs in the child like any
   external command.

`execute_builtin_cmd_fg` saves fds 0/1/2, applies the node's redirections,
applies `NAME=val` prefixes temporarily (`apply_temp_assigns` /
`restore_temp_assigns`), calls the function, then restores -- except for a
bare `exec`, whose whole point is that the redirections persist. A special
builtin that received a malformed request sets `state->builtin_fatal`;
`strict_builtin_failed` reads it after teardown and `exit_clean`s a
non-interactive shell, as POSIX requires.

## Echo

`builtin_echo` (`core_builtins.c`) is two files.

`echo_flags.c` -- `parse_flags(argv, &n, &e)` returns the index of the first
non-flag word. It walks each leading `-xyz` word through
`process_flag_token`, which validates every character with `is_flag_char`
(only `n`, `e`, `E`) before `apply_flag_char` commits any of them, so a word
with one bad letter (`-nq`) is printed as text, as in bash. `print_args`
writes the rest.

`echo_help.c` -- `e_parser(out, s, drop_unknown)` decodes `-e` escapes into a
buffer: `\n \t \r \b \f \v \a \\ \e`; `\0NNN` (at most three octal digits,
values above 0xFF wrap); `\xHH` (one or two hex digits; with none the `\x`
stays literal); and `\c`, which returns 1 so the caller stops output and
drops the newline. `parse_numeric_escape` owns those digit caps.
`drop_unknown` is the one bash/zsh difference (`echo -e '\d'` prints `\d`,
zsh's `print` prints `d`): one decoder serves both `echo` and
`builtin_print` rather than two escape tables kept in step.

## CD

`builtin_cd` (`core_builtins2.c`) is a thin orchestrator over six helper
files. It is a near-complete `cd`: options, the `--` terminator, logical vs
physical resolution, CDPATH, plus a zsh-style two-argument extension.
Behaviour is diffed against `bash --posix` (`tests/cd_posix`) and, for the
extension, against real zsh in Docker (`tests/cd_zsh_compare.sh`,
`make cd-zsh-test`).

### Pipeline

```
cd_parse_opts -> cd_collect_ops -> { cd_one | cd_two_arg } -> cd_apply
```

1. **Options (`cd_helpers1.c`)** -- `cd_parse_opts` fills a `t_cdopt` from
   leading `-X` words (bundled, e.g. `-LP`; last of `-L`/`-P` wins), accepts
   `-e`/`-@`, stops at `--` or the first operand, and on an unknown letter
   (`cd_invalid_opt`) prints `invalid option` plus a usage line and returns
   exit status **2** (bash parity).
2. **Operands (`cd_helpers2.c`)** -- `cd_collect_ops` counts operand words
   (defensively skipping any stray redirection tokens) and records the first
   two. 0 -> `$HOME` (`cd_target_home`, errors if unset), `-` -> `$OLDPWD`
   (`cd_target_dash`, echoes the destination), `""` -> POSIX no-op success.
3. **Resolution** -- `cd_one` (static in `core_builtins2.c`) looks a plain
   name up through **CDPATH** (`cd_cdpath`, `cd_helpers5.c`); a hit via a
   non-empty component echoes the destination.
4. **Move (`cd_helpers4.c`)** -- `cd_apply` performs the chdir:
   - **logical (`-L`, default)**: `cd_logical_path` / `cd_canonicalize`
     (`cd_helpers3.c`) anchor the operand to `$PWD` and collapse `.`/`..`
     *textually*, so `cd link; cd ..` returns to where you started rather
     than to the symlink's physical parent; `$PWD` keeps the path as typed.
   - **physical (`-P`)**: chdir straight to the operand, then `getcwd()`.
   Then `$PWD`/`$OLDPWD` are rotated via `update_pwd_vars`
   (`src/environment/utils2.c`), and `run_chpwd_hooks` fires zsh's
   `chpwd_functions`.

`pwd` prints the cached `state->cwd` rather than calling `getcwd()`, so it
still answers after the directory has been deleted; `cd`, `pushd` and `popd`
keep the cache current.

### Two-argument extension (`cd_helpers6.c`)

`cd old new` (a zsh feature, **not** POSIX/bash) replaces the first
occurrence of `old` with `new` in `$PWD` and cds there; `old` absent from
`$PWD` is an error (`string not in pwd`). Three or more operands remain the
bash `too many arguments` error (exit 1). Verified against zsh, not bash.

In **POSIX mode** (`hellish --posix`, or `set -o posix` at runtime) the
extension is disabled: two operands produce the bash `too many arguments`
error (exit 1) like every other shell. The gate is `state->opt_posix`,
checked in `builtin_cd`; POSIX-mode behaviour is diffed against
`bash --posix` via `tests/cd_posix` and `make cd-posix-test`.

## Export

`builtin_export` (`core_builtins.c`): `export_skip_opts` consumes `-p`/`-n`/
`-f` and `--` (`bad_opt_word` rejects anything else with status 2). No
operands -> `collect_and_print_exported`. With `-n` (`export_wants_unexport`,
`export_helpers3.c`) each name goes through `export_unexport_arg` ->
`env_unexport`. Otherwise every word goes through `process_arg`, one at a
time -- operands are never read pairwise, so `export A B C` marks three
variables and rewrites none. Errors accumulate but do not stop the loop.

- `export_helpers.c`: `parse_export_arg` splits at the first `=`;
  `ft_is_valid_ident` enforces `[A-Za-z_][A-Za-z0-9_]*`;
  `strip_surrounding_quotes` removes a matching outer quote pair and returns
  the quote character so the caller knows whether to expand.
- `export_helpers2.c`: `process_arg` -> `handle_identifier`.
  `export_apply_append` handles `NAME+=value` (the parser hands over
  `id="NAME+"`; the `+` is stripped and the old value spliced in front).
  Values are expanded by `expand_export_value` (`src/expander/`) unless they
  were single-quoted. A bare `NAME` just flips the flag on an existing entry.
- `collect_and_print_exported.c`: `collect_exported_list` formats each
  exported entry as `export KEY="value"` with `dquote_str` escaping so the
  output reads back; `sort_export_list` runs `ft_quicksort` (bash sorts
  `export -p`, and order-insensitive diffs are worth it);
  `print_and_free_list` prints and releases.

The export attribute is sticky: `env_set` keeps `exported` across a plain
reassignment, and only `unset`, `export -n` and `declare +x` remove it. See
`src/environment/README.md` for why that mattered (nvm's `PATH=` dance).

## Exit

`builtin_exit` (`core_builtins.c`), helpers in `exit_helpers.c`:

1. `exit_stopped_guard` (`exit_jobs.c`): an interactive shell with a stopped
   job refuses the first attempt and says so (issue #41); the
   `exit_warned`/`exit_attempt` pair in `t_shell` is what makes "twice"
   mean twice *in a row* (issue #58).
2. `print_exit_if_readline` prints `exit` to stderr only under `INP_RL`.
3. `handle_no_args` -> leave with `$?`; `handle_double_dash` skips `--`;
   `handle_non_numeric` / `exit_parse_ll` parse a `long long` with overflow
   detection.
4. The status is masked to 8 bits (`code & 0xFF`).

Error handling is mode-dependent, matching bash: under `-c` (`INP_ARG`) a
bad operand exits the shell (too many arguments -> 1, non-numeric -> 2);
from a script, pipe or tty it prints the error and returns 2 so the shell
keeps running.

`exit_clean(state, code)` in `exit.c` is the single leave-the-process path
(fatal errors use it too). It runs the EXIT trap (`run_exit_trap`, once),
then compares `ft_itoa(getpid())` against `state->pid` and only in the
original process persists history (`manage_history`), calls `free_env`, then
`free_all_state`. A subshell that inherited the `t_shell` copy has a
different pid and skips the teardown, so it can never double-free or rewrite
the history file. `free_env` before `free_all_state` mirrors `off()`: the env
is not part of `free_all_state`, and forgetting it leaked ~9.6 KB per `exit`
that only the `HELLISH_ALLOC_STATS` oracle (#78) could see.
`shell_fatal_status` uses the same pid test to return bash's 127 (top-level
`-c` shell) or 1.

## Unset

`builtin_unset` (`core_builtins2.c`) accepts standalone `-v`/`-f` words and
`--`, then hands the names to `unset_operands` (`utils2.c`), which ORs the
statuses: a read-only refusal must survive the rest of the list because
`unset` is a special builtin that aborts a non-interactive shell afterwards.
`-f` routes to `unset_function`.

`try_unset` (`try_unset.c`) is the variable path, and it is no longer a
plain linear scan:

- read-only names are refused with status 1;
- `scope_pop_upvar` implements bash's upvar rule: unsetting a name whose
  newest `local` cell belongs to a *caller* pops that cell, which is how
  `_comp_upvars` returns values (issue #105);
- `unset_array_elem` handles `unset arr[i]`, rebuilding the encoded value;
- `env_drop_entry` shifts the vector down and calls `env_index_mark_dirty`
  so the lookup index rebuilds on the next probe.

`unset_raw` skips the upvar rule; scope unwinding uses it to avoid a
`restore_one`/`try_unset` ping-pong.

## Redirection-aware argv helpers (`utils2.c`)

Redirection words can leak into a builtin's argv. `parse_redir_len` measures
the operator prefix (`[n]<`, `[n]>`, `[n]>>`, `[n]<<`), `redir_needs_next`
says whether the target is the following word, and `is_redir_operator` is the
cheap "starts like one" test. `cd_collect_ops` uses them to skip such tokens
when counting operands.

## Conventions

- A builtin returns its exit status; the executor stores it via
  `set_cmd_status`. Diagnostics go to stderr prefixed with `state->ctx`.
- Anything that must be visible after the call (variables, cwd, fds, traps)
  is only reachable because the builtin ran in the parent; see the
  `modify_parent_ctx` rule above before assuming that.
- The `t_shell` state, the environment API (`incs/env.h`) and the `t_vec` /
  `t_hash` containers are the only shared structures; there is no per-builtin
  global state except the dispatch table itself.
