# Infrastructure Module – Design, Algorithms, and Data Structures

The **infrastructure** layer provides all the low‑level services that make the
shell feel like a modern, interactive program rather than a pile of `read()`
loops. It is where we handle:

- Input acquisition (readline / non‑TTY / file / argv),
- Token streaming into the lexer and parser,
- Prompt rendering (including Git/venv/time metadata),
- History loading/saving and filtering,
- Buffered readline logic, signals, and Ctrl‑C/Eof semantics,
- AST visualization and debugging helpers.

Higher‑level modules (lexer, parser, expander, executor) treat this as a mostly
opaque **input and debug service**.

---

## 1. Big Picture: From Terminal to AST and Back

At the top level, `parse_and_execute_input()` is the orchestrator:

1. **Prepare** a `t_parser`, token deque (`t_deque_tok`), and an initial prompt
   string.
2. Enter a general **input loop** (`get_more_input_parser`):
   - request more tokens from the lexer (`get_more_tokens`),
   - handle EOF / interrupts / debug flags,
   - drive the parser until it has a full AST or needs more input.
3. Once the parser returns `RES_OK`, **execute** the top‑level AST and clean up.

Underlying that are several subsystems:

- `rl_*` and `input_*` handle reading **lines** from various sources and
  buffering them.
- `prompt_*` constructs context‑aware prompts and tracks visible width with
  ANSI and multibyte awareness.
- `history_*` manages user command history across sessions.
- `ast_*` offers visualization and cleanup utilities for the AST.
- `error.c` prints consistent, context‑aware error messages.

All of these aim to make the rest of the shell see a **simple stream of
tokens** and a clean AST, even though the underlying control flow is complex.

---

## 2. Input Model and Data Structures

### 2.1 `t_rl` – line buffer and cursor

The core data structure for line input is `t_rl` (see `sh_input.h`):

- `t_string buff` – backing byte buffer holding **all unread input**.
- `size_t cursor` – index of the next unread byte within `buff`.
- `bool has_line` – whether there is at least one full line to return.
- `bool has_finished` – whether the input source is exhausted (EOF).
- `int line` – current logical line number (for error messages and context).
- `bool should_update_context` – toggle for whether `state->context` should be
  updated with the new line number.

The invariant is:

- Bytes `[0 .. cursor)` are already consumed.
- Bytes `[cursor .. buff.len)` form unconsumed data.
- When a newline exists in `[cursor .. buff.len)`, `has_line` is true.

`buff_readline_update()` and `buff_readline_reset()` maintain this invariant:

- **update**:
  - adjust `cursor` to stay within bounds,
  - set `has_line` by testing whether `cursor != buff.len`.

- **reset**:
  - shift the remaining unconsumed bytes to the start,
  - shrink `buff.len` accordingly,
  - reset `cursor` to 0 and recompute `has_line`.

This design allows us to:

- Append arbitrary chunks of input (from `read()` or `readline()`),
- Return lines incrementally without re‑allocating a new buffer each time,
- Support multi‑line inputs (e.g. `\` continuation, `RES_MoreInput` in parser)
  by simply appending more data and re‑running the tokenizer.

### 2.2 Line extraction: `buff_readline`

`buff_readline(t_shell *state, t_string *ret, char *prompt)` is the abstract
“get next line” function:

1. If input is already exhausted (`has_finished`), return 0.
2. If there is no ready line:
   - decide **how** to get more input based on `state->input_method`:
     - `INP_RL`: interactive terminal via `readline()` in a child,
     - `INP_NOTTY`: raw `read()` loop for non‑TTY stdin,
     - `INP_FILE`/`INP_ARG`: read once then EOF.
   - append new data into `readline_buff.buff`.
   - for interactive mode, append a `\n` artificially so that each `Enter`
     counts as a full line.
3. Once a full line is present, `return_new_line()`:
   - finds the next `\n` starting at `cursor`,
   - copies that slice into `ret`,
   - advances `cursor`,
   - updates `has_line` and increments `line` count,
   - updates `state->context` to the new `base_context: line N`.

If the trailing newline is never found (e.g. final partial line before EOF),
`return_last_line()` returns the remaining bytes once, then empties the buffer.

This model decouples **line boundaries** from how data is actually read
(chunks, signals, etc.), which is crucial for correctness.

---

## 3. Readline Strategy, Multibyte Characters, and ANSI Handling

### 3.1 Why we fork for `readline`

Interactive input is handled via GNU Readline, but we want the parent shell
process to remain in control of signals and buffering. The strategy is:

- `get_more_input_readline`:
  - creates a pipe (`pp[2]`),
  - forks a child,
  - child calls `bg_readline(pp[1], prompt)` and exits.

- In `bg_readline`:
  - set `rl_instream` and `rl_outstream` appropriately,
  - optionally dump debug info (byte values and visible width) if
    `MINISHELL_DEBUG_PROMPT` is set,
  - call `readline(prompt)`,
  - write the result into the pipe and exit.

- The parent (`attach_input_readline`):
  - reads the child’s data into `t_rl.buff` via `vec_append_fd`,
  - updates `has_line`/`cursor`,
  - waits for the child and inspects exit status or signals.

This split gives us:

- Clean separation of Readline’s internal state from the main process.
- Proper Ctrl‑C handling: if Readline is interrupted, the child dies by signal
  and the parent sees that, converting it into `status == 2` and setting
  `should_unwind`.

### 3.2 ANSI and multibyte width tracking for prompts

Terminals care about **visual width**, not byte count. We want prompts that:

- Use color and Unicode icons (`⏳`, branch markers, etc.),
- Align nicely regardless of wide characters,
- Don’t confuse Readline about where the cursor really is.

Two key ideas are used:

1. **Readline markers**: sequences `\001`/`\002` wrap invisible ANSI escapes.
   - `vec_push_ansi` inserts `\001` + actual escape + `\002`.
   - Readline recognizes text between these markers as zero‑width.

2. **Visible width computation**: `visible_width_cstr` in
   `visible_with_cstr.c`:

   - Walks the prompt string byte‑by‑byte.
   - Skips over internal markers `\001...\002`.
   - Skips over raw escape sequences beginning with `0x1b '['` until their
     final letter.
   - Uses `mbrtowc` and `wcwidth` to compute display width of multibyte
     characters.

Similarly, generic width measurement for prompt layout uses `measure_width`,
which runs a similar multibyte loop.

With these tools, prompt generation can compute how many **columns** the left
part (user, cwd, git, venv) takes, so it can pad correctly before printing the
right‑aligned time segment.

---

## 4. Prompt Construction and Customization

### 4.1 Data structure: `t_prompt`

`prompt_private.h` defines a small state struct used while building the prompt:

- `char *user` – current user name.
- `char *short_cwd` – shortened current directory path.
- `char *branch` – git branch name (if any).
- `char *venv` – Python venv or Conda env name.
- `int cols` – terminal width in columns.
- Several cached width and padding values.
- `char time_buf[32]` – formatted current time.

### 4.2 Algorithm: `prompt_normal`

`prompt_normal()` returns a `t_string` that is a full multi‑line PS1‑like
prompt:

1. Call `ensure_locale()` to ensure wide‑char functions behave.
2. Initialize an empty `t_string`.
3. Fill `t_prompt p` and append segments:
   - `prompt_user_and_cwd`:
     - fetch `$USER` or default to `user`,
     - get `getcwd`, shorten path if needed with `shorten_path`,
     - print a Unicode/ASCII prefix line:
       - `╭─ user ─ cwd` with colors.

   - `prompt_branch`:
     - call `get_git_info` to gather branch name and dirty flag using
       `popen("git rev-parse ...")` and `popen("git status ...")`.
     - append `─ branch` in gray/white or magenta when dirty.

   - `prompt_venv`:
     - read `CONDA_DEFAULT_ENV` or `VIRTUAL_ENV` via `get_venv_name`,
     - append `(<venv>)` in blue.

   - `prompt_branch_marker`:
     - append a `*` marker if the branch is dirty.

   - `prompt_time_and_pad`:
     - compute left side width using `measure_width` and raw `strlen` for
       strings stored in `t_prompt`;
     - compute how much horizontal padding remains (`cols - left_width - time`),
       ensuring at least 1 space;
     - append that many spaces;
     - append `⏳HH:MM:SS` in dim color;
     - finally print `\n$ `.

4. Free allocated fields (`short_cwd`, `branch`, `venv`) and return the string.

If you want to change the prompt, you can:

- Edit or replace any of the small helpers (`prompt_branch`, `prompt_venv`,
  `prompt_time_and_pad`),
- Or write your own version of `prompt_normal` while still relying on
  `vec_push_ansi` and `measure_width` for alignment.

### 4.3 More‑input prompt (`prompt_more_input`)

When the parser needs more tokens (`RES_MoreInput`), the prompt changes to
remind the user *why* more input is being asked.

`prompt_more_input` inspects `parser->parse_stack`, which stores a sequence of
operators/constructs that require closure (e.g. `{`, `|`, `&&`, `||`). It then
builds a compact prompt like:

- `subsh>` if expecting a closing brace,
- `pipe>` when continuing a pipeline,
- `cmdand>` or `cmdor>` for `&&`/`||`.

This prompt is returned as a simple `t_string` – no ANSI complexities here –
which makes it easy to adjust or extend.

---

## 5. History Management: Parsing, Encoding, and Filtering

### 5.1 Persistent history storage

History is stored in a single file under `$HOME`:

- `get_hist_file_path`:
  - uses `env_get(&state->env, "HOME")` to compute `$HOME/<HIST_FILE>`.

- `parse_history_file`:
  - opens the file (creating it if missing),
  - reads its entire content into a `t_string hist`,
  - passes it to `parse_hist_file`, which splits it into commands.

### 5.2 Escaping newlines and backslashes

To support multi‑line commands in history, lines are encoded with a simple
escaping scheme:

- `encode_cmd_hist`:
  - For each char `c` in a command:
    - If `c` is `\\` or `\n`, prefix it with an extra `\\`.
  - Append a real `\n` at the end of the encoded command.

- `parse_single_cmd` and `parse_hist_file` reverse this process:
  - `parse_single_cmd` reads until `\n`, but treats `\\` specially:
    - a backslash sets a “escaped” flag; the next char is taken literally,
    - if the newline is **not escaped**, it marks the end of the stored
      command.
  - `parse_hist_file` repeatedly calls `parse_single_cmd` and adds each result
    to the in‑memory history and readline’s internal history (via
    `add_history`).

This encoding means the history file can distinguish **embedded newlines** in
commands from the newline that terminates a command record.

### 5.3 Recording new history entries

`manage_history` is called at the end of each command cycle:

- Checks `worthy_of_being_remembered`:
  - line length > 1,
  - history is active,
  - command is not identical to the last recorded command (no duplicates).

- If worthy:
  - ensures the readline buffer is null‑terminated at `cursor - 1`,
  - duplicates the line into `hist_entry`,
  - calls `add_history(hist_entry)` so Readline sees it,
  - appends `hist_entry` to `state->hist.hist_cmds`,
  - if the history file is open for append, encodes and writes the entry.

The history subsystem therefore maintains a **vector of strings in memory** and
an **append‑only file** on disk.

### 5.4 Initialization and teardown

- `init_history` sets default flags and reads existing history from disk.
- `free_hist` frees all in‑memory command strings and vector storage.

If you want to filter or transform history behavior (e.g. ignore leading
spaces, custom filters), you can adjust `worthy_of_being_remembered` and
`encode_cmd_hist` in one place.

---

## 6. Parser and Lexer Driver Loops

The infrastructure code implements several **driver loops** that manage tokens
and parsers under different debug modes:

- `default_parser_loop` (normal operation):
  - repeatedly calls `get_more_tokens`,
  - passes them to `try_parse_tokens`, which:
    - if tokens list is effectively empty → resets buffer and continues,
    - otherwise calls `parse_tokens`, stores AST in `state->tree`, and
      analyzes `parser->res`:
      - `RES_OK`: return to top‑level to execute `state->tree`.
      - `RES_MoreInput`: generate new `prompt_more_input` and continue
        reading.
      - `RES_FatalError`: set `last_cmd_status` to syntax error.

- `debug_lexer_loop` / `debug_parser_loop`:
  - similar structure, but either print tokens (`print_tokens`) or AST
    (`print_ast_dot`, `print_ast_tree`) after each read or parse step.
  - They also call appropriate cleanup functions:
    - clear token deque,
    - reset readline buffer and prompt,
    - keep shell status coherent.

Control of which loop runs is done via `state->option_flags`:

- `OPT_FLAG_DEBUG_PARSER` → `debug_parser_loop`.
- `OPT_FLAG_DEBUG_LEXER` → `debug_lexer_loop`.
- default → `default_parser_loop`.

Each branch uses the same underlying primitives: `get_more_tokens`,
`buff_readline`, prompt helpers, history, and signal handling, but with
slightly different behaviors.

---

## 7. AST Utilities and Error Reporting

### 7.1 AST printing and DOT export

`ast_utils*.c` define a small API for **AST introspection**:

- `node_name` maps enum values to human‑readable names.
- `print_node`, `print_node_line` print compact text representations.
- `print_ast_tree` prints a colorful tree using Unicode box drawing chars,
  showing node types and token payloads.
- `print_ast_dot` writes a `out.dot` file in Graphviz DOT format:
  - nodes are colored and shaped based on type (pipeline vs token vs redirect),
  - edges are labeled with child indices,
  - IDs are pseudo‑random to make the graph visually spread out.

These tools help debug parser behavior and operator precedence without
inspecting raw arrays.

### 7.2 Freeing the AST – postorder traversal

- `ast_postorder_traversal` recursively visits children then the node itself.
- `free_node`:
  - frees token strings if owned (`allocated` flag),
  - frees children vector storage,
  - zeroes the struct.
- `free_ast` is a thin wrapper around the postorder traversal.

Postorder ensures children are always freed before their parent, making it safe
when pointers between nodes reference children.

### 7.3 Error messages

`error.c` provides a consistent **front‑end error language**:

- `err_cmd_not_found`, `err_no_path_var`, `err_1_errno`, `err_2` are small
  wrappers that prefix messages with `state->context` and print via
  `ft_eprintf`.

- `unexpected` is used by the parser when an unexpected token is seen:
  - it peeks at the upcoming token,
  - prints `syntax error near unexpected token 'X'` or `'newline'`,
  - sets `parser->res = RES_FatalError`.

This centralization of error messages keeps output consistent and makes it
simple to update wording later.

---

## 8. Signals, EOF, and Control Flow

The infrastructure code tries to give a **uniform behavior** for Ctrl‑C,
Ctrl‑D, and non‑TTY input:

- `handle_eof`:
  - if EOF is seen and it’s the primary shell (not a subshell), sets
    `should_exit = true`.

- `handle_interrupt` / `handle_ctrl_c`:
  - clear token deque,
  - reset readline buffer,
  - reset prompt and status to `130` (
    `CANCELED` with `c_c = true`),
  - allow the main loop to continue for a new command.

- `get_more_input_notty`:
  - loops `read()` calls, pushing into `readline_buff.buff` until a `\n` or
    EOF, and returns different status codes for EOF vs interrupt.

- All main loops check `get_g_sig()->should_unwind` and `state->should_exit`
  frequently to bail out as soon as possible.

---

## 9. Why This Infrastructure Is Structured Like an API

Although this directory may look like a collection of many small files, they
form a coherent **input and debugging service**. The goals are:

1. **Separation of concerns**
   - Lexing/parsing do not have to know about Readline, terminal width,
     history files, or signal details.
   - They just receive token streams and return ASTs.

2. **Reusability and testability**
   - Each helper (prompt building, width measurement, history encode/parse,
     readline buffering) can be reasoned about, tested, or replaced
     independently.

3. **Configurability**
   - Changing the prompt style, history behavior, or debug printing can be
     done by editing localized functions, without touching core parsing or
     execution logic.

4. **Robustness**
   - Centralizing the tricky edge cases (multibyte chars, ANSI escapes,
     partial lines, interrupted reads, EOF in the middle of constructs) keeps
     the rest of the shell code cleaner and reduces subtle bugs.

In short, the infrastructure layer turns raw I/O, terminal quirks, and shell
state into **clean, high‑level services**: prompt, history, input lines,
ASTs, and error reporting. Understanding these pieces makes it clear how the
shell can remain responsive, visually consistent, and debuggable even under
complex input and signal patterns.