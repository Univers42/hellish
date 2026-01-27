# Core Module and `t_shell` Structure Documentation

## 1. Overview

The `core/` directory implements the high-level lifecycle of the shell:

- Program entry (`shell.c` / `main`)
- Global shell state initialization (`on.c` + `init.c` + `helpers.c`)
- Command‑line option parsing (`opt.c`)
- Integration with environment, history, prompt, lexer, parser and executor

At the center of this design is the `t_shell` structure defined in `incs/shell.h`. It is the *single source of truth* for the running shell instance: all subsystems read or modify this state.

```c
// ...existing code...
typedef struct s_shell
{
		/* fields described in detail below */
}   t_shell;
// ...existing code...
```

Everything in `core/` is built around creating, initializing, using and finally cleaning up a single `t_shell` instance.

---

## 2. `t_shell` – Global Shell State

```c
typedef struct s_shell
{
	 t_string        input;
	 t_vec_env       env;
	 t_string        cwd;
	 t_ast_node      tree;
	 int             input_method;
	 char            *base_context;
	 char            *context;
	 char            *pid;
	 char            *last_cmd_status_s;
	 t_execution_state       last_cmd_status_res;
	 t_history       hist;
	 bool            should_exit;
	 t_vec_redir     redirects;
	 int             heredoc_idx;
	 t_rl readline_buff;
	 t_prng    prng;
	 uint32_t        option_flags;
	 int             bg_job_count;
	 t_vec_procsub   proc_subs;
}   t_shell;
```

### 2.1 `input` – Raw user/program input buffer

- **Type:** `t_string` (a `t_vec`-like growable byte buffer)
- **Owner:** `repl_shell()` in `shell.c`
- **Usage:**
  - Stores the current command line or script chunk about to be parsed
  - Filled by the input subsystem (readline or file/buffer)
  - Consumed by `parse_and_execute_input(state)`
- **Lifecycle:**
  - Reinitialized each loop iteration in `repl_shell()`
  - Freed after every command round to avoid leaks

Expectation: after input collection, `input` contains exactly the bytes which the lexer/parser will consume. No other subsystem should modify it concurrently.

---

### 2.2 `env` – Environment vector

- **Type:** `t_vec_env` (vector of `t_env`)
- **Owner:** `on()` (core init) and builtin env management (`export`, `unset`, etc.)
- **Usage:**
  - Represents the shell’s view of the environment (`KEY`, `VALUE`, `exported` flag)
  - Initialized from `envp` via `env_to_vec_env()` in `on.c`
  - Mutated by builtins like `export`, `unset`, and `cd` (PWD/OLDPWD)
- **Guarantees:**
  - Centralized, single representation – executors and child processes derive their env from here
  - No direct dependency on global `environ`, easier to test and reason about

---

### 2.3 `cwd` – Current working directory cache

- **Type:** `t_string`
- **Role:** Cache of the logical current directory
- **Usage:**
  - Filled by `init_cwd(state)` at startup
  - Updated by `cd` builtin (`update_pwd_vars`, `cd_refresh_cwd`)
  - Used by `pwd` builtin and possibly the prompt system
- **Why cache?**
  - Avoid repeated `getcwd()` calls (expensive on deep trees or slow FS)
  - Maintain a consistent logical path even if underlying directories are deleted (matches shell behavior when `getcwd` fails)

---

### 2.4 `tree` – Parsed AST of the current input

- **Type:** `t_ast_node`
- **Owner:** Parser and executor
- **Usage:**
  - `parse_and_execute_input()` lexes `input`, builds an AST into `tree`, then executes it
  - Freed on each REPL iteration with `free_ast(&state->tree)`

Expectation: at any given time, `tree` either represents the AST for the *current* input or is empty/cleared.

---

### 2.5 `input_method` – Input source mode

- **Type:** `int` (values from `sh_input.h`, e.g. `INP_RL`, `INP_FILE`, `INP_ARG`, `INP_NOTTY`)
- **Set by:** `mode_input()` in `on.c`, via helpers in `init.c`
- **Modes:**
  - `INP_RL` – Interactive terminal (history, prompts)
  - `INP_FILE` – Script file execution
  - `INP_ARG` – `-c "command string"` mode
  - `INP_NOTTY` – Non‑TTY stdin (pipes, redirections)
- **Why it matters:**
  - Controls prompt behavior, history integration, and some builtin behaviors (e.g. `exit` prints "exit" only when using readline)

---

### 2.6 `base_context` and `context` – Error/reporting context strings

- **Type:** `char *`
- **Ownership:** Allocated with `ft_strdup`, freed in `free_all_state()`
- **Roles:**
  - `base_context`: original identifier of the shell (argv[0] at startup)
  - `context`: mutable display name used in error messages
    - Updated for scripts: `update_context_from_file()` sets it to the filename
    - Used in error reporting throughout the code (`ft_eprintf("%s: ...", state->context, ...)`)

Expectation: all user‑visible diagnostics use `context` so that errors correctly refer to the current script or shell name.

---

### 2.7 `pid` – Stringified PID of the main shell process

- **Type:** `char *`
- **Set by:** `on()` using `xgetpid()`
- **Usage:**
  - Used in `exit_clean()` to decide whether we are in the *original* shell process
  - When a child process calls builtin `exit`, we **do not** want to free global state or history
- **Behavior:**
  - If current PID string matches `state->pid`, `exit_clean()` will:
    - `manage_history(state)`
    - `free_all_state(state)`
  - Otherwise, the child process exits without trampling parent state

---

### 2.8 `last_cmd_status_s` and `last_cmd_status_res`

- **Types:**
  - `last_cmd_status_s`: textual representation (e.g. `$?` string form)
  - `last_cmd_status_res`: structured execution result `t_execution_state`
- **Set by:** `set_cmd_status(state, res_status(...))` and executor code
- **Used by:**
  - `exit` builtin when no explicit code is given
  - Prompt, scripting constructs and error reporting

Expectation: after each command or pipeline, `last_cmd_status_res` accurately reflects the final status, and `last_cmd_status_s` (if used) maintains a printable form.

---

### 2.9 `hist` – Command history

- **Type:** `t_history`
- **Initialized by:** `init_history(state)` (from `on.c`) in interactive mode
- **Usage:**
  - Stores previous commands when using readline
  - Saved/restored during `exit_clean()` for the main shell only

---

### 2.10 `should_exit` – REPL termination flag

- **Type:** `bool`
- **Owned by:** Builtins and signal/unwind logic
- **Usage:**
  - `repl_shell()` loops while `!state->should_exit`
  - Builtins can set `should_exit = true` to break out of the loop (e.g. `exit` builtin)

This design allows a clean, controlled shutdown without forcing immediate `exit()` from deep inside subsystems, except where explicitly desired (e.g. non‑interactive fatal errors).

---

### 2.11 `redirects` – Active redirections for current command

- **Type:** `t_vec_redir`
- **Usage:**
  - Collected during parsing
  - Consumed by executor to set up file descriptor redirections
  - Freed after each command with `free_redirects(&state->redirects)` in `repl_shell()`

Expectation: `redirects` only contains entries for *current* input; it is cleared each loop to avoid leaking old redirections into new commands.

---

### 2.12 `heredoc_idx` – Index for heredoc processing

- **Type:** `int`
- **Role:**
  - Tracks which heredoc is being processed when the parser/executor handle multiple heredocs
  - Used to map heredoc placeholders in the AST to real temporary files or pipes

---

### 2.13 `readline_buff` – Buffered input for readline/FILE/STDIN

- **Type:** `t_rl`
- **Filled by:**
  - `buff_readline_init()` and `vec_init()` in `on()`
  - `read_file_to_buffer()` in `init.c` for file mode
  - stdin/tty reading in the input subsystem
- **Usage:**
  - Acts as unified buffer for various input methods
  - `should_update_context` flag indicates when the prompt/context should be recalculated

Design goal: have one abstraction for "a stream of lines" regardless of whether they come from a TTY, a file, a string (`-c`) or a pipe.

---

### 2.14 `prng` – Pseudo‑random number generator state

- **Type:** `t_prng`
- **Initialized by:** `prng_initialize_state(&state->prng, 19650218UL)` in `on()`
- **Usage:**
  - Provides deterministic PRNG state for shell features (e.g., `$RANDOM`‑like behavior, job IDs, or other future features)
- **Benefits:**
  - No reliance on global RNG state
  - Easy to reproduce sessions for debugging by keeping the seed known

---

### 2.15 `option_flags` – Global runtime options

- **Type:** `uint32_t` bitmask
- **Filled by:** `select_mode_from_argv()` in `opt.c`
- **Bits:**
  - `OPT_FLAG_HELP`
  - `OPT_FLAG_VERBOSE`
  - `OPT_FLAG_DEBUG_LEXER`
  - `OPT_FLAG_DEBUG_PARSER`
  - `OPT_FLAG_DEBUG_AST`
- **Short‑circuit option design:**
  - Command‑line parsing is simple and linear; for each argument we update the bitmask:
    - `--help` / `-h` → `OPT_FLAG_HELP`
    - `--verbose` or `-c` → `OPT_FLAG_VERBOSE` (note: currently `-c` also enables verbose)
    - `--debug=lexer` → `OPT_FLAG_DEBUG_LEXER`
    - `--debug=parser` → `OPT_FLAG_DEBUG_PARSER`
    - `--debug=ast` → `OPT_FLAG_DEBUG_AST`
  - Options are **composable**: `--debug=parser --debug=lexer` simply sets multiple bits.

This short‑circuit approach avoids complex parsing: each option is independent, and combining them is trivial bitwise OR.

---

### 2.16 `bg_job_count` – Background jobs tracking

- **Type:** `int`
- **Usage:**
  - Counts how many background jobs are currently known to the shell
  - Updated by executor when spawning jobs with `&` or process‑substitutions
- **Future extensions:**
  - Can be used for job control display, wait logic, or clean shutdown checks

---

### 2.17 `proc_subs` – Process substitution tracking

- **Type:** `t_vec_procsub` (vector of `s_procsub_entry`)
- **Each entry:**
  - `pid` – child process ID performing the substitution
  - `fd` – file descriptor connected to that process
  - `path` – associated path, if any (e.g. `/dev/fd/*` or named pipes)
- **Usage:**
  - Allows the executor to remember which process substitutions are active
  - Ensures proper closing and waiting for substituted processes

---

## 3. Lifecycle: From `main` to REPL and back

1. **`main()` in `shell.c`**
   - Normalizes login shell name (removes leading `-`)
   - Declares a single `t_shell state;`
   - Calls `on(&state, argv, envp);`
   - Enters `repl_shell(&state);`
   - On exit, calls `off(&state);`

2. **`on()` in `on.c`**
   - Sets signal handling via `set_unwind_sig()`
   - Initializes `state` to zero with `shell_init()` (inline in `shell.h`)
   - Parses command‑line options with `select_mode_from_argv()`
   - Initializes readline buffer, PID, context, cwd, environment and vectors
   - Decides input method (`-c`, file, stdin, TTY) and prepares corresponding buffers
   - Seeds PRNG and sets `bg_job_count = 0`

3. **`repl_shell()`**
   - While `!should_exit`:
     - Initializes `state->input`
     - Calls `parse_and_execute_input(state)`
     - Frees `redirects`, AST, and `input`

4. **`off()`**
   - Frees environment and all remaining state
   - Forwards the last command status as process exit code

This lifecycle ensures one well‑defined `t_shell` that lives for the duration of the program.

---

## 4. Thread Safety and POSIX Compliance

### 4.1 Single‑threaded design for safety

- The shell is designed to run a **single `t_shell` instance on a single thread**.
- All global process‑wide interactions (signals, PIDs, file descriptors) are inherently non‑thread‑safe; POSIX shells are typically single‑threaded.
- By *not* sharing `t_shell` across threads and by avoiding concurrent mutation, we get thread safety by design:
  - No locks or atomics are needed inside `t_shell`.
  - All accesses are serialized by the REPL loop.

In other words, the shell is "thread‑safe" in the sense that there is no unsynchronized shared mutable state *between* threads, because we intentionally avoid multi‑threaded access to `t_shell`.

### 4.2 POSIX‑style behavior

The core adheres to POSIX shell expectations:

- **Environment:** `env_to_vec_env()` initializes `env` from `envp`, and builtins modify this view as POSIX specifies.
- **Exit status:** `last_cmd_status_res` follows standard 0–255 exit codes and is used for `$?` and `exit` default.
- **Input modes:**
  - Interactive vs non‑interactive behavior is distinguished via `isatty(0)` and `input_method`.
  - `exit` printing behavior (`print_exit_if_readline`) mimics POSIX shells printing `exit` only in interactive sessions.
- **Signals:** `set_unwind_sig()` and `get_g_sig()` integrate with POSIX signal handling; unwinding behavior is centralized and respects typical shell expectations (e.g., interrupting the current command but not killing the entire shell inappropriately).
- **Redirections and heredocs:** handled via `redirects`, `heredoc_idx` and the AST, providing POSIX redirection semantics.

### 4.3 Globals vs state

- The design minimizes use of raw global variables: instead, `t_shell` carries nearly all logical state.
- Where C or POSIX APIs force some global behavior (signals, process IDs, file descriptors), the shell encapsulates it via helper functions but keeps logical decisions in `t_shell`.

---

## 5. Option / Debug Flag Design (`select_mode_from_argv`)

The option system in `opt.c` is intentionally **simple and composable**:

- Each argument is scanned once.
- `process_opt_flag()` updates a `uint32_t flags` bitmask.
- Flags are independent; there is no complex state machine.

Example:

```sh
sh42 --debug=parser --debug=lexer --verbose script.sh
```

This will set:

- `OPT_FLAG_DEBUG_PARSER`
- `OPT_FLAG_DEBUG_LEXER`
- `OPT_FLAG_VERBOSE`

and then execution continues normally. There is no requirement that these options appear in any particular order; the logic short‑circuits per token only (i.e., once an argument is recognized and processed, we move on), but the *combination* is simply a bitwise OR of all requested flags.

Benefits:

1. **Simplicity:** Easy to extend with new debug targets (`--debug=ast`, `--debug=exec`, etc.) without rewriting the parser.
2. **Composability:** Multiple `--debug=*` flags can be chained; each sets its own bit.
3. **Low overhead:** Only string comparisons and a few bitwise operations at startup.
4. **Clarity:** Runtime consumers check `if (state->option_flags & OPT_FLAG_DEBUG_LEXER)` instead of parsing strings repeatedly.

---

## 6. Summary

- `t_shell` is the central structure that models the entire state of the running shell.
- Each field has a clearly defined responsibility (input, env, cwd, AST, history, redirects, PRNG, options, etc.).
- The `core/` module wires this structure to the outside world: command‑line, environment, signals, and REPL.
- The design favors:
  - **Single‑threaded safety**,
  - **POSIX‑like behavior**, and
  - **Composability** (especially in options/debug flags and subsystems).

Understanding `t_shell` and the core lifecycle is key to extending the shell safely—for example, adding new debug flags, new input modes, or new executor features without breaking global invariants.

---

## 7. Practical Code Examples

### 7.1 Using `last_cmd_status_res` for `$?` and `exit`

The executor is responsible for updating the last command status after each top‑level evaluation:

```c
void some_executor_entry(t_shell *state, t_ast_node *root)
{
    t_execution_state res;

    res = execute_ast(root, state);
    set_cmd_status(state, res_status(res.status));
    state->last_cmd_status_res = res;
}
```

A builtin that needs the default exit status (like `exit` with no argument) simply reads from `state->last_cmd_status_res`:

```c
int builtin_exit(t_shell *state, t_vec argv)
{
    int ret;

    // No explicit code: use last command status
    if (argv.len == 1)
        exit_clean(state, state->last_cmd_status_res.status);
    // ...existing numeric parsing and error logic...
}
```

Similarly, an expansion implementation for `$?` can be written as:

```c
char *expand_last_status(t_shell *state)
{
    // Convert last status to string if needed
    if (!state->last_cmd_status_s)
        state->last_cmd_status_s = ft_itoa(state->last_cmd_status_res.status);
    return (ft_strdup(state->last_cmd_status_s));
}
```

This shows the intended pattern: the *numeric* truth lives in `last_cmd_status_res.status`, while the string form is derived on demand.

### 7.2 How `pid` controls cleanup in `exit`

The `exit` builtin uses `exit_clean()` (see `src/builtins/exit.c`). That helper compares the current PID against `state->pid`:

```c
void exit_clean(t_shell *state, int code)
{
    char *pid_s;

    pid_s = xgetpid();
    if (pid_s && state->pid && ft_strcmp(state->pid, pid_s) == 0)
    {
        // We are in the original shell process: full cleanup
        manage_history(state);
        free_all_state(state);
    }
    free(pid_s);
    exit(code);
}
```

Implications:

- In the **main shell process**, calling `exit` will:
  - Save history,
  - Free global state,
  - Then terminate.
- In a **child process** that inherited `t_shell` but has a different PID:
  - `ft_strcmp(state->pid, pid_s)` fails,
  - No global cleanup is performed,
  - Only `exit(code)` is executed.

This prevents child processes from corrupting or double‑freeing parent state.

---

## 8. How to Extend `t_shell` Safely

When adding new fields to `t_shell`, keep these invariants and guidelines in mind:

1. **Initialization must be explicit**
   - `shell_init()` returns a zero‑initialized struct:
     ```c
     static inline t_shell shell_init(void)
     {
         return ((t_shell){0});
     }
     ```
   - Any non‑zero / non‑NULL default must be set explicitly in `on()` (or a dedicated initializer).

2. **Define clear ownership and lifetime**
   - If your field points to heap memory (`char *`, `t_vec`, custom struct), decide:
     - Who allocates it,
     - Who frees it,
     - At what stage in the lifecycle (per‑command, per‑session, on exit only).
   - Add the corresponding `free_*` calls to `free_all_state()` (or the appropriate per‑loop cleanup) so there are no leaks.

3. **Avoid hidden global state**
   - Prefer storing logical state inside `t_shell` instead of in global variables.
   - If you must interact with global APIs (signals, FDs), keep the *policy* in `t_shell` and make helper functions thin wrappers.

4. **Respect the REPL loop boundaries**
   - Fields that are **per command** (e.g., AST, redirects, temporary vectors) must be reset or freed in `repl_shell()` on each iteration.
   - Fields that are **per session** (history, env, options, PRNG, pid) should only be initialized in `on()` and freed in `off()` / `exit_clean()`.

5. **Consider input modes and POSIX semantics**
   - If your new field affects user‑visible behavior (e.g. prompts, logging, debug info), think about:
     - Interactive vs non‑interactive (`input_method`),
     - Script file vs `-c` vs stdin.
   - Try to mirror how POSIX shells differ between these modes.

6. **Integrate with `option_flags` when appropriate**
   - If you add a new debug or verbose feature, add a new bit in `enum e_opt_flag` and set it in `process_opt_flag()`.
   - Use bit checks (`state->option_flags & NEW_FLAG`) instead of parsing strings at runtime.

7. **Document the field in `core/README.md`**
   - For every new member, update this README with:
     - Type,
     - Who owns it,
     - When it is initialized and freed,
     - How it interacts with existing parts of the shell.

By following these rules, you keep `t_shell` coherent, prevent subtle lifetime bugs, and make it easier for future contributors (or your future self) to reason about the shell’s behavior.