# Helpers Module

The **helpers** module groups small, focused utilities used across the
shell, and -- less obviously from the name -- it is where hellish's memory
model lives. If you need to know why a pointer can be freed twice safely,
why `eval` in a loop does not grow memory, or why `HELLISH_ALLOC_STATS`
prints a number, this is the directory.

Public prototypes: `incs/helpers.h`; the arena: `incs/parena.h`; the case
scanner: `incs/casescan.h`.

---

## 1. Role in the architecture

The executor, expander, environment and builtins are about *what a shell
does*. Whenever they need to parse a number safely, free a nested structure,
record `$?`, validate a variable name, or allocate thousands of tiny
parse-lifetime objects, they call in here. The point is one audited
implementation of each tricky detail rather than five approximate ones.

---

## 2. The memory model

Three allocators sit above the heap, each for a different lifetime. All of
them route through `xmalloc`/`xfree` so the compile-time heap choice (libc
vs `ft_malloc`, `SAFE=0`) is preserved and no pointer ever crosses heaps.

### 2.1 The parse arena (`parse_arena.c`, `parse_arena2.c`, `parse_arena3.c`)

A chunked bump allocator for cycle-lifetime parse objects: AST children
buffers, `full_word` back-refs, escape-processed copies. One input cycle
allocates thousands of tiny blocks that all die together, so bump
allocation plus one reset beats per-block `malloc`/`free`.

- **Gated.** `parena_on(true)` is called only around the main cycle's
  `parse_tokens` / `parse_tokens_range` (`src/infrastructure/input_utils4.c`
  and `input_stream.c`). Nested parses -- `eval`, `source`, command
  substitution bodies -- run with the gate closed and fall through to
  `xmalloc`, keeping their own exact free discipline; an `eval` inside
  `while true` must not grow the arena forever.
- **Free routing.** `parena_free(p)` is a no-op for arena pointers
  (`parena_owns`, a binary search over the address-sorted chunk registry)
  and forwards anything else to `xfree`. One teardown walk therefore frees
  both arena-backed cycle trees and heap-backed clones (function bodies,
  `eval` ASTs).
- **`parena_note_attach()`** records that *heap* memory was attached to the
  cycle tree (an arith cache, a heredoc body, an expander rewrite, or an
  arena-full fallback). While the flag is clear, the teardown walk would be
  a pure no-op, so the cycle finaliser skips it and reclaims the tree with
  the O(chunks) `parena_reset()` instead of an O(nodes) traversal. Missing a
  site can only leak, never double-free.
- **`parena_reset()`** runs once per cycle: the first chunk stays warm,
  the rest go back to the heap so a one-off giant script does not pin its
  high-water mark. `parena_try_extend()` grows the *last* allocation in
  place, which is what the depth-first parser nearly always asks for.
  `parena_destroy()` (from `free_all_state`) returns everything.
- **The registry is finite** (`PARENA_MAX_CHUNKS`, chunk sizes doubling
  from `PARENA_FIRST_CHUNK` to `PARENA_MAX_CHUNK`). When it fills,
  `parena_alloc` falls back to `xmalloc` and sets `attached`. That fallback
  is where the **issue #94 double free** lived: a struct shared between
  children on the assumption "the gate is open, so `parena_free` is a
  no-op" was in fact heap-owned and freed once per child. The share is now
  gated on `parena_owns()` (see `src/word_splitting/reparse_escape.c`), and
  `tests/alloc_stress.sh` (`make arena-stress`) rebuilds with 512-byte
  chunks under ASan so the rare states become the common case.

The arena is a process-wide singleton (`parena()`), like the word slab,
because the reparse pass allocates deep in call chains that carry no
`t_shell`. Known cleanup target.

### 2.2 The word slab (`word_slab.c`)

A size-class slab (16/32/64/128/256-byte caches, built on
`vendor/libft`'s `slab.h`) for short-lived simple-command argv strings,
which are tiny and die together at command end.

- `word_slab_push(1)` is set around argv expansion
  (`src/expander/expand_cmd_simple_word.c`) and `word_slab_push(0)` around
  assignment and `for` values (`expand_word5.c`, `execute_for.c`) -- values
  that may escape into the environment must live on the general heap.
  The call returns the previous state so callers restore it.
- `word_strndup` picks the slab when the flag is on and the string fits,
  `xmalloc` otherwise; `word_free` routes any pointer to the right place.
  Because of that routing, **a wrong flag can only cost speed, never
  correctness** -- the argv of a command is routinely a mix of slab
  literals and heap expansions.
- `word_slab_teardown` at shutdown keeps the live-bytes oracle at zero.

### 2.3 The argv pool (`free_utils2.c`)

`argv_pool_acquire` / `argv_pool_release` lend a simple command its
`argv` backing array from `state->argv_pool[ARGV_POOL_DEPTH]`, so a plain
command does no per-command `malloc` for the array; strictly LIFO, with a
fresh vector past the depth limit. `free_argv_pool` releases the slots once.

### 2.4 Teardown (`free_utils.c`, `free_utils2.c`)

`free_all_state(state)` is the single canonical shutdown path, and the
order is deliberate:

1. `free_functions` -- function table (`name`, `src`, `text`, body AST),
   `func_index`, the retired bodies in `dead_funcs`, `call_frames`.
2. `free_session_strings` -- `input`, `alias_exp`, `pid`, `last_bg_pid`,
   `ctx`/`dft_ctx`, `rl.buff`, the split-`$PATH` cache.
3. `free_redirects`, `cleanup_proc_subs`, `free_ast(&state->tree)` -- the
   per-command scratch.
4. `arr_marks_clear`, `attr_clear`, then `free_session_data` -- history,
   the alias table, the command-hash cache, the ZLE tables, `cwd`,
   positional args, the argv pool, the trap strings, dirstack, compspecs,
   local saves, the `for` snapshot.
5. `env_index_free()` after all env lookups, `parena_destroy()`, then
   `word_slab_teardown()` last so a stray `word_free` still works, and
   finally `alloc_live_report()`.

**`state->env` is not freed here.** `off()` and `exit_clean()` call
`free_env(&state->env)` first (the EXIT trap may still read variables), and
that omission is exactly what once leaked ~9.6 KB per `exit` (#78).

The comments in that file are worth reading as a case study: `alias_table_free`
and `cmd_hash_free` were both correct destructors that nothing called, and
LeakSanitizer never said so because the tables stayed reachable from
`state`. The `ft_malloc` oracle counts live bytes, not reachability, which is
how sourcing oh-my-zsh's git plugin (201 aliases, 18 KB) found them.

Per-command releases: `free_redirects` (unlinks heredoc tmpfiles, closes fds
we own, frees names, resets the vec), `free_executable_cmd` (pre-assign
key/value pairs, argv via `word_free`, the pooled array back to the pool),
`free_executable_node` (the redirect list embedded in an executable node).

### 2.5 The live-bytes oracle (`alloc_stats.c`)

ASan and valgrind are blind to `ft_malloc`. With `HELLISH_ALLOC_STATS` set,
`alloc_live_report()` prints the bytes still live on the `ft_malloc` heap
after `free_all_state()`; a non-zero number is a leak the sanitizers cannot
see. It is a no-op on the libc backend, selected by `HAVE_ALLOC_ORACLE` from
the Makefile (the comment explains why the old weak-symbol link trick broke
on macOS).

---

## 3. The rest of the toolbox

- **`utils.c`** -- `set_cmd_status` records `$?` in both forms: the
  `t_execution_state` for logic and a decimal string formatted into
  `state->statbuf` (no allocation; `last_cmd_st` points into the struct and
  is never freed). `forward_exit_status` exits the process, restoring the
  default SIGINT handler and using `128+SIGINT` when the last command was
  interrupted. `write_to_file` retries short writes.
- **`checked_atoi.c`** -- `ft_checked_atoi(str, &out, flags)`: a `long`
  accumulator with `INT_MIN`/`INT_MAX` checks, returning 0 or -1. The flag
  bits allow leading whitespace (1), a `-` sign (2), leading non-digits (4),
  trailing garbage (8), trailing spaces (16); the magic value `42` is the
  "POSIX arithmetic context" preset (bits 1+2+8). One audited parse for
  `exit`, `ulimit`, `kill`, `SHLVL`, ...
- **`var_name.c`** -- `is_var_name_p1` / `is_var_name_p2` (`[A-Za-z_]`
  then `[A-Za-z0-9_]`, with the `unsigned char` cast the C standard
  requires), and `sh_skip_quoted`, which steps over a quoted span or
  backslash escape so paren-matching scans never count parens inside quotes.
- **`casescan.c`** -- the case-aware `$(...)` span automaton (issue #95),
  shared by the lexer's `tokenize_subshell`, the word reparser and the
  expander so all three agree where a substitution ends.
- **`sq_quote.c`** -- wrap a string in single quotes using the `'\''`
  idiom, as `printf %q` does; used by `${x@Q}` and by the completion
  dispatcher to hand a half-typed word to a shell function unmangled.
- **`x_getcwd.c`** -- `getcwd(NULL, 0)` returns a libc-malloc'd buffer;
  `x_getcwd` copies it onto the active heap and libc-frees the original so
  every cwd pointer the shell stores can be `xfree`d.
- **`verbose.c`** -- `verbose(flag, fmt, ...)` forwards to libft's
  `claptrap` in a `VERBOSE` build (flags match the `OPT_FLAG_DEBUG_*` bits)
  and compiles to nothing otherwise.

---

## 4. Why these live together

Cleanup, allocation and checked parsing are the places where a shell leaks
or corrupts memory quietly. Keeping them in one directory with one set of
conventions -- everything through `xmalloc`/`xfree`, every free router
tolerant of the "wrong" pointer, one teardown order -- is what lets the
`SAFE=0` parity run and the ASan suite mean the same thing.
