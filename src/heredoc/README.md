# Heredoc Module – Conceptual Documentation

This document explains, in human terms, **what the heredoc module does and how
it fits into the rest of the shell**. It focuses on ideas and data flow rather
than implementation details.

In a POSIX‑style shell, *here‑documents* are constructs like:

```sh
cat <<EOF
hello $USER
EOF
```

The heredoc syntax tells the shell:

> “Before running this command, read lines from the user until a given
>  delimiter, optionally expand variables inside them, and feed the result to
>  the command as if it was coming from a file or pipe.”

The heredoc module is responsible for all of that behavior.

---

## 1. Where Heredocs Fit in the Shell

The overall pipeline is:

1. **Lexer / parser** – recognize `<<` / `<<-` operators and build AST
   redirection nodes that represent heredocs.
2. **Heredoc module** – walks the AST *before* execution to:
   - find every heredoc,
   - prompt the user for its contents,
   - store those contents in temporary files,
   - attach the proper redirect information to the AST.
3. **Expander + executor** – later, when running the command, they just see
   normal redirections from files (or pipes) and do not need to handle the
   interactive “read until delimiter” logic.

This means the heredoc module acts as a **pre‑processing step**: it resolves all
here‑documents into concrete file descriptors so that the rest of the shell can
stay simpler.

---

## 2. What the Heredoc Module Produces

For each heredoc redirection in the AST, the module ensures that:

- There is a **temporary file** (or shared file) containing exactly the text
  of the heredoc.
- There is a corresponding **`t_redir` entry** in the shell’s redirect list:
  - pointing from the command’s input FD (typically stdin) to that file,
  - with flags indicating if the file should be deleted after use.
- The AST redirection node is annotated with:
  - `has_redirect = true`,
  - an index into the redirect list (`redir_idx`).

As a result, when the executor later walks the AST, it does not need to know
whether an input file came from a heredoc or from explicit `< file`. It simply
applies the redirections it is given.

---

## 3. Core Responsibilities (Conceptually)

### 3.1 Discovering and grouping heredocs in the AST

The parser builds a tree of nodes: commands, pipelines, redirect nodes, etc.
The heredoc module walks this tree with one main goal:

> “Locate every heredoc redirection node and materialize it into a temporary
>  file, before any command starts executing.”

Key ideas:

- **AST traversal**:
  - The module walks the tree recursively and skips nodes that cannot contain
    redirections (plain tokens, words, process substitutions).
  - For nodes that can contain redirections (simple commands, pipelines,
    compound commands), it inspects their children.

- **Redirect grouping**:
  - When a command has several heredocs in a row, they are treated as a
    **group** so their temporary storage can be shared or appended to in a
    consistent way, especially inside pipelines.
  - The logic distinguishes:
    - heredocs that should each get their own file,
    - heredocs that should append to an existing file (e.g. same logical
      stdin across a pipeline segment).

This pre‑processing pass is exposed as a single conceptual operation:

> “Given the root AST and the shell state, prepare all heredocs.”

### 3.2 Creating and sharing temporary files

When the module encounters a heredoc redirection, it needs a **file to read
from later**. It does this by:

- Building a unique temporary path, typically using:
  - a dedicated heredoc directory (`TMP_HC_DIR`),
  - the shell’s PID,
  - a small index that increments for each heredoc.

- Opening the file in **write mode** to fill it, and in **read mode** to use as
  the redirection source.

For grouped heredocs (e.g. a series of heredocs that conceptually feed the same
stdin within a pipeline segment), the module can:

- Create the first file and record its corresponding `t_redir` entry.
- For subsequent heredocs in the group, **reopen the same file in append mode**
  and write additional content, without creating new redirection entries.

This ensures that the executor sees a **single redirect** representing all
combined heredoc content as the input to that part of the pipeline.

### 3.3 Reading heredoc content from the user

For each heredoc, the module must read lines from the user until the delimiter
is seen, similar to how shells behave interactively.

The logic is:

1. **Choose a prompt**:
   - Normal heredoc → `heredoc> `.
   - Heredoc inside a pipeline → `pipe heredoc> ` (to reflect the context).

2. **Read lines** using the shell’s readline buffer:
   - The module integrates with the shell’s line‑reading infrastructure so that
     history, editing, and signal handling are consistent.

3. **Stop conditions**:
   - If the user types the exact delimiter line (with or without trailing
     newline), the heredoc is finished.
   - If the user hits EOF (Ctrl‑D) before the delimiter, the module prints a
     warning like:

     > here-document at line N delimited by end-of-file (wanted `SEP`)

     and marks the heredoc as finished.
   - If the user interrupts (Ctrl‑C), the shell’s global signal state is
     updated and the heredoc logic stops accordingly.

All collected lines are appended into an internal buffer (`full_file`) before
being written out to the temporary file.

### 3.4 Expansion rules inside heredocs

Shells support different behaviors depending on the heredoc operator:

- `<<` – **expand** variables and some backslash escapes.
- `<<-` – same as `<<`, but also **strip leading tab characters** from the
  input lines.
- Quoted delimiters – if the delimiter itself is quoted, then the heredoc
  **must not expand** variables or backslashes.

The module implements these rules as follows:

- It inspects the AST node that holds the delimiter to decide:
  - whether the delimiter contains quotes (→ no expansion),
  - whether the operator was `<<-` (→ enable tab stripping).

- For each input line:
  - If tab stripping is enabled, leading tabs are removed before processing.
  - If expansion is enabled:
    - `$VAR`‑like patterns are replaced via the environment module
      (`env_expand_n`), similar to how command‑line variables are expanded.
    - Certain backslash escapes are interpreted so that you can control when
      characters like `$` or `\` are taken literally.
  - If expansion is disabled, lines are copied as‑is.

The result is appended to the heredoc’s `full_file` buffer, preserving
newlines between lines.

### 3.5 Writing the final content to disk

Once all the lines for a heredoc are collected and optionally expanded:

- The module ensures the buffer is null‑terminated.
- It writes the entire content to the heredoc’s write file descriptor.
- It closes that descriptor and keeps only the read descriptor in the
  redirection entry.

The temporary file is later cleaned up by the redirection cleanup and state
cleanup helpers, so user code never has to know where it lived on disk.

---

## 4. Interaction with Other Modules

The heredoc module is intentionally **narrow in scope** and interacts with
other parts of the shell in limited, well‑defined ways:

- **Parser / AST**:
  - It reads the structure of the AST (node types, children, tokens) but does
    not change the syntax; it only annotates redirect nodes with
    `has_redirect` and an index.

- **Environment module**:
  - When expansion is enabled, heredoc variable expansion uses the same
    `env_expand_n` logic that regular command expansion uses, ensuring
    consistent semantics.

- **I/O & redirection layer**:
  - It relies on `t_redir` structures and the shell’s redirect vector
    (`state->redirects`), delegating the actual redirection setup to the
    executor.

- **Helpers / readline / signals**:
  - It uses the shared readline buffer and helper functions to read lines,
    handle EOF, and respect global interrupt flags.

Because the heredoc module runs **before** normal expansion/execution, it helps
keep the expander and executor simpler: they only see regular redirections.

---

## 5. Robustness Considerations

Heredocs are interactive and can be error‑prone. The module addresses this with
several design choices:

- **Graceful EOF handling**:
  - If the user ends input early, a clear warning is printed with line number
    and expected delimiter, but the shell does not crash.

- **Signal awareness**:
  - Input loops check the shell’s signal state (`should_unwind`) so that
    Ctrl‑C cancels heredoc reading promptly.

- **Strict delimiter checks**:
  - The `is_sep` helper carefully compares lines (with or without trailing
    newline) against the separator string so that only the correct line ends
    the heredoc.

- **Centralized temp file creation**:
  - A dedicated function handles all details of constructing and opening
    temporary files, which reduces the chance of inconsistent behavior or
    leaks.

- **Memory ownership clarity**:
  - Strings allocated for separators and heredoc content are either handed off
    to well‑defined owners (e.g. `t_heredoc_req`, redirect entries) or freed
    immediately after use.

---

## 6. Why the Heredoc Module Is Structured Like an API

As with the expander and globbing modules, heredoc handling is kept as a
self‑contained “service”:

- From the outside, other parts of the shell only need to know:
  - “Call the heredoc preparer on the AST before executing anything.”
  - Then handle the resulting redirections as usual.

- Internally, the heredoc module breaks the problem into small, focused
  helpers:
  - AST traversal and grouping of redirects,
  - temp file creation and sharing,
  - interactive line reading and delimiter detection,
  - optional variable/escape expansion,
  - final write‑out and cleanup.

This makes it easier to:

- **Reason about behavior** (e.g. why a certain heredoc expands `$VAR` while
  another does not),
- **Extend** the logic later (support more escape rules, different prompts,
  non‑interactive input sources),
- **Test** individual parts in isolation.

In short, the heredoc module turns a high‑level syntax feature (`<<` / `<<-`)
into concrete files and descriptors in a way that is **modular, predictable,
and reusable**, so the rest of the shell can treat heredocs as just another
kind of input redirection.