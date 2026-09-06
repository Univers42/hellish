# Environment Module

## 1. Concept

In a POSIX shell the **environment** is the `KEY=VALUE` mapping that is
inherited from the parent (`envp`), mutated by `export`/`unset`/`cd`/plain
assignment, handed to children on `execve`, and read by the shell itself
(`HOME`, `PATH`, `PWD`, `IFS`, `SHLVL`).

`src/environment/` provides:

- the structured store (`t_env`, `t_vec_env`) and its O(1) name index;
- conversion between `char **envp` and that store, in both directions;
- lookup and mutation primitives (`env_get`, `env_nget`, `env_set`,
  `env_unexport`, `env_extend`);
- expansion of `$NAME` and the special variables (`$?`, `$$`, `$!`, `$-`,
  `$#`, `$0`, `$1..$N`, `$LINENO`, `$RANDOM`, ...);
- the startup defaults (`PATH`, `SHLVL`, `PWD`, `_`, `OPTIND`, `PS1`, ...);
- the encodings for indexed arrays, associative arrays and `declare`
  attributes, which the rest of the env lifecycle never has to know about.

The store lives in `t_shell` as `state->env`, one view for the whole shell.

Public API: `incs/env.h` (a second, older `incs/public/env.h` carries the
init-setter prototypes).

---

## 2. Data structures

`t_env` (`incs/env.h`): `bool exported`, `char *key`, `char *value`. The
rule that everything else follows from: **a `t_env` owns its key and value
strings; never alias them.** `exported` decides whether the entry reaches
`execve`; a shell-local variable is the same struct with the flag off.

`t_vec_env` is a `t_vec` with `elem_size = sizeof(t_env)`. Order carries no
meaning, and lookups do not scan it (section 4). `free_env` is an inline in
`env.h`; it is *not* part of `free_all_state`, so callers free the env first
(`off()`, `exit_clean()`).

Three encodings hide inside an ordinary `value` string:

- an **indexed array** starts with `ARR_MAGIC` and holds `index<US>value`
  records joined by `RS`, kept sorted by index (sparse indices allowed);
- an **associative array** starts with `ARR_ASSOC_MAGIC`, same records but
  the subscript is a literal key and order is insertion order;
- `declare -i` / `-n` live in a separate tiny table, `t_var_attr` in
  `state->var_attrs`, empty unless a script uses them.

Nothing in set/copy/free/fork knows about these; only the array helpers,
the expander and the two listing sites (`declare -p`, `set`) do. Arrays are
never exported to `execve`, as in bash.

---

## 3. Bootstrap (`conv.c`)

`t_env str_to_env(char *str)` splits `KEY=VALUE` at the first `=`,
duplicates both halves and marks the entry exported -- everything in `envp`
is exported by definition. A missing `=` trips an `ft_assert`: a malformed
`envp` is a host bug, and crashing early beats misreading it.

`t_vec_env env_to_vec_env(t_shell *state, char **envp)` imports every entry,
marks the index dirty, then, when `state->cwd` is known, overrides `PWD`
with the real cwd (a parent that chdir'd after setting it would hand us a
stale value) and force-sets `IFS` to `" \t\n"`, non-exported, so word
splitting has a sane default even if the parent cleared it.

---

## 4. Lookup and mutation (`utils.c`, `helpers.c`, `env_export.c`, `env_index*.c`)

- `t_env env_create(key, value, exported)` -- wraps three fields; the caller
  passes heap strings that `env_set` will eventually own.
- `t_env *env_get(env, key)` and `t_env *env_nget(env, key, len)` -- both go
  through the index; `env_nget` takes an explicit length so expansion can
  look up a name inside a larger string without copying. The returned pointer
  is valid until the next `env_set`/unset that could realloc the vector.
- `int env_set(env, el)` -- upsert. Existing key: free the old value, free
  the *new* key (the old string is kept), install the value. New key: push
  and tell the index. Returns 0, or 1 when the push fails. **The export
  attribute is sticky**: `old->exported = old->exported || el.exported`.
  Every plain assignment arrives with `exported=false`, and the previous
  overwrite meant `PATH="$PATH:/x"` silently un-exported `PATH` -- nvm does
  exactly that before its own `export PATH`, and every login that loaded it
  printed `manpath: warning: $PATH not set`.
- `int env_unexport(env, key)` (`env_export.c`) -- the only way the
  attribute comes off short of `unset`: `export -n`, `declare +x`,
  `typeset +x`. The value stays for the shell.
- `env_extend(dest, src, export)` (`expand.c`) -- drain `src` into `dest`,
  overriding each entry's flag; used for assignment-only commands.

### 4.1 The index (`env_index.c`, `env_index2.c`, `env_private.h`)

A lazy open-addressing hash table from name to vector position. The vector
stays the source of truth: every probe re-checks the key bytes in the
vector, so a collision or a stale slot can never return the wrong variable
-- worst case it falls through to -1. `env_index_add` follows an append;
`env_index_mark_dirty` is the escape hatch for anything that shuffles
entries (`unset`, bulk import), and the next `env_index_find` rebuilds
(`env_index_reset`). Staying dirty is always correct, just slower.

The table is four file-scope globals (`g_tab`, `g_cap`, `g_count`,
`g_dirty`) rather than a member of `t_shell` -- a legacy that the `TODO` at
the top of `env_index.c` names as a cleanup target. `env_index_free` runs
from `free_all_state`, after the last lookup.

---

## 5. Expansion (`expand.c`, `expand2.c`, `expand_zsh0.c`)

```c
char	*env_expand_n(t_shell *state, char *key, int len);
char	*env_expand(t_shell *state, char *key);   /* len = strlen(key) */
```

`env_expand_n` is the hot path, called thousands of times per script, and
its contract is the important part: it returns a **borrowed** pointer into
`state` or the vector -- never free it -- and it distinguishes `NULL`
(unset) from `""` (set but empty), which `${v:?}` and `set -u` depend on.
Resolution order:

1. `expand_special`: the `FUNCNAME`/`BASH_SOURCE` rebuild hook
   (`frames_sync`, deferred until the first read after a call); `$?`
   (`state->last_cmd_st`); `$$` (`state->pid`); `$!` (`last_bg_pid` or
   `""`); the empty name -> `""`; one-character names via
   `expand_special_1` (`$-` from `build_flagstr`, `$0` inside a zsh
   function via `zsh_arg_zero`, `$#` from `pos.cnt_str`); then
   `expand_special_dyn` (`expand2.c`): `$LINENO`, `$RANDOM` (session PRNG
   masked to 15 bits), `$SECONDS`, `$EPOCHSECONDS`, all formatted into
   `state->linebuf`. A user assignment to these names does not shadow them
   (accepted divergence).
2. a nameref (`attr_target`) -> recurse on the target;
3. a positional index (`pos_index`; `$0` is not positional, it is a plain
   variable);
4. `env_nget` on the store.

`env_apply_cmd_assigns(state, cmd, export)` (`expand2.c`) applies the
`NAME=val` words that prefixed a simple command, moving each `t_env` into the
store and NULLing the source's `key`/`value` so the caller's free path knows
ownership moved. A read-only target prints the error and, outside an
interactive shell, exits with `shell_fatal_status` (127 for the top-level
`-c` shell, 1 otherwise, like bash).

---

## 6. Startup defaults and maintenance

`helpers.c`, `init_dft_env.c`, `init_ps1.c`, `winsize.c`, `utils2.c`. The
pattern is uniform: patch a gap the parent left, never override a value it
gave us.

- `init_cwd` seeds `state->cwd` via `x_getcwd()` (or warns with
  `MSG_GETCWD_SHINIT` and leaves it empty); `set_cwd` is the same plus a
  second push, kept decoupled for subshell init paths.
- `HOME` is **not** synthesised. Like bash it comes only from the parent, so
  `cd` with `HOME` unset errors and a bare `~` falls back to the passwd home
  (`tilde_home_dir` in the expander).
- `set_path`: an absent or empty `PATH` becomes `DFT_PATH` (`incs/sys.h`).
- `set_shlvl`: parse with `ft_checked_atoi(..., 42)` and increment; missing
  or non-numeric -> `1`.
- `set_shell_var`: `$SHELL` from the passwd login shell only when entirely
  absent, and un-exported, as bash does.
- `set_underscore`: seeds `$_` when absent. Gotcha: it sets it from
  `state->ctx` and then -- the second `env_set` is not in an `else` --
  overwrites it with the `MINISHELL` macro (`"./minishell"` in
  `incs/sys.h`, a leftover from the project's old name). Parents almost
  always export `_`, so the fallback is rarely observed; both the macro and
  the missing `else` are cleanup targets.
- `set_optind` resets `OPTIND` to `1` (keeping the parent's export flag);
  `set_id_vars` adds non-exported `UID`, `EUID`, `HOSTNAME`, `OSTYPE`,
  `BASH_VERSION`, `BASH_VERSINFO`.
- `ensure_essential_env_vars` (called from `on()` after `env_to_vec_env`)
  runs all of the above, adds `PPID`, and creates `PWD` from `x_getcwd()`
  (or `TMP_DIR`) if still absent.
- `set_default_ps1` (interactive only): `HELLISH_PS1_DEFAULT`
  (`incs/prompt.h`), un-exported, filled in only if `PS1` is unset. Shipping
  a value at all is what lets a Python virtualenv's `deactivate` restore the
  prompt (issue #39).
- `update_winsize_vars`: `COLUMNS`/`LINES` for interactive shells, refreshed
  before each top-level execution so a resize is visible to the next command
  (issue #97); `set_winsize_var` preserves a user export.
- `update_pwd_vars` (`utils2.c`, called by `cd`, `pushd`, `popd` after a
  successful chdir): rotate `PWD` into `OLDPWD` -- or unset `OLDPWD` if
  there was no `PWD` (POSIX 2.5.3) -- then set `PWD` from `state->cwd`.

---

## 7. `envp` for `execve` (`utils.c`, `utils2.c`)

`char *env_to_str(t_env *e)` serialises one entry to `KEY=VALUE` with a
single sized allocation (it runs once per exported variable per external
command; the old vector-growth path paid three reallocs each).

`char **get_envp(t_shell *state, char *exe_path)` builds the NULL-terminated
array from entries with `exported == true` and a non-array value; the caller
owns the array and every element. `exe_path` is unused. It is what
`src/execution/run.c` and `exec` pass to `execve`.

Process substitutions no longer need a variant that includes non-exported
variables: `<(cmd)` / `>(cmd)` run their body in the forked child in process,
the same way `$(cmd)` does, so the child simply inherits the whole shell
state -- functions and arrays included (`src/platform/posix/procsub_input.c`).

---

## 8. Arrays, associative arrays, attributes, quoting

- `env_array.c`..`env_array4.c` -- the `arr_*` family: `arr_is`, `arr_next`
  (record iterator), `arr_count`, `arr_get_idx`, `arr_join`/`arr_join_range`
  (`t_slice`, zsh's `a[lo,hi]` resolved to 0-based inclusive bounds),
  `arr_with_set`, `arr_without`, `arr_splice`, `arr_from_elems`,
  `arr_max_idx`, `arr_format` (the `[i]="v"` display form). Every mutation
  builds a fresh encoded string and lets `env_set` free the old one.
- `env_assoc.c`..`env_assoc3.c` -- the `assoc_*` family with the
  `t_assoc_it` streaming iterator (`assoc_it_init`, `assoc_next`), plus
  `assoc_get`, `assoc_with_set`, `assoc_without`, `assoc_keys`,
  `assoc_values`, `assoc_format`.
- `env_attr.c` -- `attr_kind`, `attr_target`, `attr_set`, `attr_clear`:
  linear scans over a table that is empty unless `declare -i/-n` was used,
  so the assignment and read hot paths pay nothing.
- `env_quote.c` -- `vec_push_dquoted` / `dquote_str` escape the four
  characters that stay special inside double quotes (`"`, `$`, `` ` ``,
  `\`) so `declare -p`, `export -p` and `set` output reads back as what it
  describes; `assoc_key_quoted` decides when an assoc key needs quoting.

---

## 9. Why this design

- The environment is a first-class structure, and `char **` exists only at
  the two boundaries: startup import and `execve`.
- Everything funnels through `env_set`/`env_get`, so ownership and the
  index invariant are enforced in one place; the sticky export attribute and
  the verify-on-probe index are both consequences of that.
- Special variables are computed at read time from `t_shell`, in one
  ordered chain, so adding one is one branch in `expand_special*` and no new
  state.
