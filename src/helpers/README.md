# Helpers Module – Conceptual Documentation

The **helpers** module groups together small, focused utilities that are used
across many parts of the shell. These are not features on their own, but
building blocks that keep the rest of the code simple and readable.

You can think of this directory as the shell’s **toolbox**:

> Small, well‑defined functions that solve common, low‑level problems
> (parsing integers, freeing complex structures, updating status, etc.) so that
> higher‑level modules (parser, expander, executor, builtins) can stay focused
> on shell semantics.

---

## 1. Role in the Overall Architecture

Other modules like the **executor**, **environment**, **expander**, and
**builtins** are mostly concerned with “what a shell does”: run commands,
expand variables, manage environment, handle redirections, etc.

Whenever they need to:

- parse a number safely from user input,
- free a nested data structure without leaking memory,
- update the global exit status in a consistent way,
- check whether a string is a valid variable name,
- print debug information conditionally,

…they call into the helpers module.

The design goal is to:

- **avoid code duplication** (e.g. multiple different ways to parse an int),
- **centralize tricky details** (like overflow checks and partial frees),
- give the rest of the codebase a set of **small, reusable primitives**.

---

## 2. Groups of Helpers

### 2.1 Safe integer parsing – `checked_atoi.c`

User input often needs to be interpreted as an integer: think of `exit 42`,
`SHLVL`, or environment values. A naive `atoi()` has several issues:

- It silently overflows,
- It does not let you control whether extra garbage after the number is
  allowed,
- It does not clearly signal errors.

`ft_checked_atoi` wraps the logic into a single helper that:

- Accepts a string and an output `int *`,
- Uses an internal `long` accumulator with **overflow checks** against
  `INT_MIN` / `INT_MAX`,
- Supports configurable rules (via flags) for:
  - skipping leading spaces,
  - accepting or rejecting a sign (`+` / `-`),
  - requiring at least one digit,
  - allowing or forbidding trailing garbage.

The function returns `0` on success and `-1` on any invalid input. This lets
callers cleanly distinguish “user gave a bad number” from “user gave 0”.

Higher‑level code can then decide **how to react** (e.g. print an error,
clamp the value, reset to default) without re‑implementing parsing rules.

### 2.2 Memory and resource cleanup – `free_utils.c`

As soon as you have nested vectors, environment entries, redirection structures,
and ASTs, cleanup becomes non‑trivial. `free_utils.c` centralizes the logic for
freeing common composite structures:

- `free_redirects`:
  - walks through the vector of `t_redir` objects,
  - unlinks temporary files when needed,
  - closes file descriptors,
  - frees allocated filenames,
  - and resets the vector.

- `free_all_state`:
  - is used when shutting down the shell,
  - frees input buffers, status strings, PID strings,
  - clears redirections and process substitutions,
  - frees the AST and history,
  - and releases the current working directory buffer.

- `free_executable_cmd`:
  - frees a single `t_executable_cmd` structure returned by the expander
    (pre‑assignments and argv entries),
  - ensuring every `key`/`value` or string in those vectors is released.

- `free_executable_node`:
  - cleans up redirection vectors associated with an executable node.

Having these centralized helpers means executors and higher‑level orchestration
code do not need to remember all ownership details for every nested field.
They just call the appropriate helper and know that the structure is safely
cleaned up.

### 2.3 Shell status and exit forwarding – `utils.c`

The shell must keep track of **the exit status of the last command**, both in
numeric form and as a string (for `$?` expansion). It also must sometimes
**forward** that status to the OS when exiting.

Helpers in `utils.c` cover this:

- `set_cmd_status`:
  - updates the shell’s notion of the last command result,
  - stores both the raw `t_exe_res` (status + extra flags) and a freshly
    allocated decimal string for expansion.

- `forward_exit_status`:
  - inspects the execution result and decides how the shell should exit,
  - respects cases where Ctrl‑C or signals changed behavior (e.g. `128+SIGINT`),
  - ensures signal handlers are reset appropriately before exiting.

- `write_to_file`:
  - provides a small wrapper that writes a full string to a file descriptor,
  - handles partial writes in a loop until everything is written or an error
    occurs.

These helpers keep signal, status, and I/O concerns out of business logic like
builtins and executors.

### 2.4 Variable name validation – `var_name.c`

Shell variable names have slightly stricter rules than generic strings:

- The **first character** must be a letter or underscore (`[A-Za-z_]`).
- Subsequent characters can also be digits (`[A-Za-z0-9_]`).

`is_var_name_p1` and `is_var_name_p2` encode this logic in two simple helpers.

This is used when:

- parsing assignments (`VAR=value`),
- validating names for `export`, `unset`, and other builtins,
- or when expanding variables selectively (e.g. in export‑value expansion).

Centralizing this rule avoids discrepancies where different parts of the shell
would accept or reject different names.

### 2.5 Conditional verbose output – `verbose.c`

Debugging a shell can be tricky. The helper `verbose` provides a **single
entry point** for conditional, formatted logging.

- When the `VERBOSE` macro is defined at compile time:
  - `verbose(flag, format, ...)` forwards everything to a lower‑level function
    (`claptrap`) that does the actual printing or formatting.

- When `VERBOSE` is not defined:
  - `verbose` is compiled as a **no‑op**; the calls remain in the code but
    with zero runtime overhead.

This design makes it easy to sprinkle debugging information in critical paths
without cluttering the code with `#ifdef` blocks everywhere.

---

## 3. Why These Helpers Are Kept Together

Although each helper is small, keeping them in a dedicated module brings a few
benefits:

1. **Clarity of purpose**
   - If you need to understand memory cleanup, checked integer parsing, or
     status forwarding, you know where to look.

2. **Consistency across the codebase**
   - Everyone uses the same checked‑atoi logic, the same exit handling, the
     same redirection cleanup, so behavior is uniform.

3. **Ease of extension**
   - When new helper needs arise (e.g. string utilities that don’t belong in
     `libft`, or shell‑specific parsing helpers), they can be added here
     without touching core modules.

4. **API‑like feeling**
   - Other modules treat helpers as a small standard library tailored to this
     shell: “if you need X, there’s a helper for it”. This keeps high‑level
     code concise and focused on shell semantics rather than plumbing.

In short, the helpers module provides the **low‑level glue** that lets the rest
of the shell stay modular and readable, by factoring out repetitive or
error‑prone pieces of logic into well‑named, reusable functions.