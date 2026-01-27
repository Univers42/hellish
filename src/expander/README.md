# Expander Module – Conceptual Documentation

This document describes **what the expander does and how it behaves**, in human terms.
It is meant to help you _reason_ about the shell, not to mirror the code line‑by‑line.

Think of the expander as a **semantic bridge**:

> **Parser AST  →  “Almost‑ready” commands for the executor**

It takes a purely syntactic tree (tokens, words, redirects, etc.) and turns it into
something that very closely resembles what `execve(2)` and the rest of the shell
will actually use.

---

## 1. Position in the Overall Architecture

The shell pipeline is roughly:

1. **Lexer** – breaks the input line into tokens (`WORD`, `|`, `>`, `$(...)`, etc.).
2. **Parser** – builds an **AST** (abstract syntax tree) from those tokens.
3. **Expander** (this module) – interprets shell semantics on top of that tree:
   - variables, command / arithmetic substitutions,
   - word splitting, globbing, tilde expansion,
   - assignments, redirects, process substitution.
4. **Executor** – takes the expanded description of a command pipeline and:
   - sets up redirections and process substitutions,
   - applies temporary environment assignments,
   - calls builtins or `execve()`.

The key idea is that **the executor never has to understand quoting rules,
`$VAR`, `$(...)`, or `$((...))`**. By the time it runs, all of that has been
turned into concrete strings, FDs and data structures by the expander.

---

## 2. What the Expander Produces

When the parser sees a simple command like:

```sh
FOO=bar echo "~/$USER" *.c 2>err
```

it creates an AST node for a **simple command** with children like:

- an *assignment word* (`FOO=bar`),
- one or more *word nodes* (the command name and its arguments),
- *redirect nodes* (`2> err`).

The expander walks this AST and produces higher‑level objects for the executor:

- A **`t_executable_cmd`** describing one command:
  - `pre_assigns`: environment assignments to apply **only for this command**.
  - `argv`: the final argument vector: command name and arguments.
- A **vector of redirections** that says:
  - which FD is affected (e.g. `2` or `1`),
  - which file or `/dev/fd/N` it maps to,
  - whether that FD should be closed or removed later.

The executor then:

- applies `pre_assigns` (temporarily or permanently, depending on ctx),
- applies the redirections,
- chooses **builtin vs external** command,
- and finally executes.

The expander’s **contract** is: “given an AST, return a clean, ready‑to‑run
`executable_cmd` + redirects, or report semantic errors like _ambiguous
redirect_.”

---

## 3. Core Responsibilities (Conceptually)

### 3.1 Environment‑driven transformations

The expander is the **only place** where the parser’s plain tokens are interpreted
using the current shell environment:

- `$VAR`, `${VAR}`, `$?`, `$$`, etc. are mapped to their values via the
  **environment module** (`env_expand_n`, `env_expand`).
- Assignment words (`NAME=value`) are turned into **`t_env` entries** that can
  be applied to the shell environment (either temporarily for a single command
  or permanently when they appear alone).
- `IFS` (*Internal Field Separator*) controls **word splitting** behavior.

By centralizing this logic, we avoid duplicating environment semantics in
multiple places; everything that cares about `$FOO` simply asks the expander.

### 3.2 Tilde handling

The expander knows when a leading `~` should be treated specially:

- `~` → `$HOME`
- `~+` → `$PWD`
- `~-` → `$OLDPWD`
- `~/path` → `$HOME/path`

It only applies these rules in the correct syntactic ctxs (e.g., at the
start of a word, and not inside quotes). This keeps the parser simple while
still matching traditional shell behavior.

### 3.3 Command and arithmetic substitution

The expander handles:

- **Command substitution** – `$(cmd)`
  - Finds the matching closing `)`, handling nested parentheses.
  - Builds the inner command string `cmd`.
  - Forks a child process, executes `cmd` via the shell again, captures its
    standard output, strips trailing newlines, and inserts the result back into
    the token text.

- **Arithmetic substitution** – `$((expression))`
  - Recognizes the `$((` … `))` pattern and balances parentheses.
  - Sends `expression` to the **arithmetic engine**.
  - Converts the numeric result to text and replaces the original segment.

These substitutions are done **inside word tokens**, before word splitting and
before globbing. If parsing of `$(...)` or `$((...))` fails (e.g. unmatched
parentheses), the expander **fails gracefully** and leaves the text as‑is
instead of crashing.

### 3.4 Word splitting and globbing

After variables and substitutions are expanded, a single syntactic “word” may
turn into multiple **logical words**. Typical example:

```sh
FOO="a b"; echo $FOO
```

Here `$FOO` must be split into `"a"` and `"b"` if unquoted, using
`IFS` (usually space, tab, newline). The expander performs this splitting
**only when the shell rules say it should** (e.g. not in single quotes).

Then, for each resulting word, it can perform **pathname expansion** (globbing):

- `*.c` → all matching `.c` files in the current directory.

Globbing expands one logical word into **zero, one, or many** filesystem paths.
In most argument positions, “zero matches” means “keep the literal pattern”; in
redirection positions, “zero or many matches” generally means an
**ambiguous redirect** error.

### 3.5 Assignments and `export`

The expander makes a firm distinction between:

- A **pure assignment list**:

  ```sh
  FOO=1 BAR=2   # no command name, just assignments
  ```

  → should update the **shell’s own environment** immediately and not run any
  command.

- Assignments that are **attached to a command**:

  ```sh
  FOO=1 BAR=2 somecmd arg
  ```

  → `FOO` and `BAR` apply only while running `somecmd`, not globally.

For this it collects assignment words into a `pre_assigns` vector; if
no actual command name was ever found, the executor will simply commit those
assignments to the main environment.

`export VAR=val` is treated as a normal command whose arguments may contain
assignments; the expander still handles the variable side, but semantics are
ultimately enforced by the `export` builtin.

### 3.6 Redirections and process substitution

When the parser sees redirection syntax like:

```sh
2>err
> out
<(producer)
>(consumer)
```

it builds **AST redirect nodes**. The expander then:

1. Expands the target (filename or process substitution) using the same rules as
   for normal words.
2. Ensures the result is a **single, unambiguous word**; otherwise it reports an
   “ambiguous redirect” error.
3. Constructs a **`t_redir` description** that encodes:
   - original FD (`2`, `1`, etc.),
   - the destination FD or path,
   - whether the FD must be closed or file deleted later.
4. Appends that `t_redir` to the shell’s redirect list.

Process substitution (`<(...)`, `>(...)`) is handled by small helpers that:

- create a pipe and fork a helper process running the inner command,
- expose one end of the pipe as a `/dev/fd/N` path,
- track that process and FD in `state->proc_subs` so they can be cleaned up
  after use.

The executor then treats `/dev/fd/N` like any other filename in a redirect.

---

## 4. How Data Flows Through the Expander

A simplified view for a simple command AST node:

1. **Initialization**
   - Create an empty `t_executable_cmd`:
     - `argv` (vector of `char *`),
     - `pre_assigns` (vector of `t_env`).
   - Prepare a vector for redirect indices.

2. **Iterate over children** of the AST simple command:
   - If child is a **word node**:
     - Possibly mark that we have seen the command name.
     - Run the **word expansion pipeline** (tilde, substitutions, env, split,
       glob) to get `0..N` strings.
     - Append those strings to `argv`.

   - If child is an **assignment word**:
     - Before any command name is seen:
       - Convert it to a `t_env` entry and push into `pre_assigns`.
     - After a command name is seen:
       - Treat it as a normal word that happens to contains `=`.

   - If child is a **redirect AST node**:
     - Run the redirect expansion logic described above.

   - If child is a **process substitution node**:
     - Turn it into a `/dev/fd/N` path and append to `argv`.

3. **Post‑processing**
   - If `argv` is empty but `pre_assigns` is not:
     - This was an “assignment‑only” command → apply `pre_assigns` to the
       shell’s environment and run no further command.

The executor finally receives:

- `argv` (may be empty in the assignment‑only case),
- `pre_assigns` (possibly already applied),
- redirect indices pointing into the shell’s redirect list.

This separation lets different parts of the shell (pipelines, subshells, etc.)
share the same **expansion engine** while managing lifetime and ownership
locally.

---

## 5. Security and Robustness Considerations

Although this is a learning project, the expander was designed with some
practical robustness goals:

- **Clear ownership of memory**
  - When the expander allocates new strings, it either:
    - passes them into an owning structure (`argv`, `pre_assigns`, redirect
      lists), or
    - frees them before returning.
  - AST nodes are “consumed” or cleared once expanded to avoid double expansion
    and double frees.

- **Defensive behavior on errors**
  - Unknown or malformed constructs in `$(...)` / `$((...))` cause the
    substitution logic to back out, leaving the original text intact rather
    than crashing.
  - Ambiguous redirects are detected and reported with clear error messages.

- **Isolation of subprocesses**
  - Command and process substitutions run in **separate child processes**.
  - The environment passed to those children is constructed from the shell’s
    own environment vector; children cannot corrupt the parent’s structures.

- **Signal awareness**
  - Expansion routines consult global signal state (`should_unwind`) to stop
    ongoing work when the user interrupts the shell (e.g. Ctrl‑C), helping to
    keep the shell responsive.

All of this makes the module behave like a mini API: as long as callers obey
simple contracts (e.g., free what they own, don’t re‑expand AST nodes), the
system remains stable.

---

## 6. Why It Is Structured Like an API

The expander code may look “over‑modularized” at first glance. This is
intentional. The goals were:

1. **Local reasoning**
   - Each small function answers a very specific question, such as:
     - “Turn this AST assignment into a `t_env` entry.”
     - “Expand this word node into a list of strings.”
     - “Build a redirect description from this AST redirection node.”
   - This makes it easier to unit‑test or mentally simulate.

2. **Reusability**
   - The same core operations (word expansion, env lookup, globbing) are used in
     several ctxs:
     - simple commands,
     - here‑documents,
     - process substitutions,
     - arithmetic / command substitutions nested inside others.
   - By isolating these as helpers, new features can reuse them without
     duplicating tricky logic.

3. **Clear contracts between modules**
   - The rest of the shell treats the expander as **“given an AST, give me
     something executable”**. It does not need to know **how** `$HOME` is
     looked up, how many times `getcwd()` is called, or how nested
     substitutions are parsed.

4. **Future extension**
   - Because the expander is essentially an internal API, adding features like
     new forms of substitution, different globbing strategies, or extended
     parameter expansion is mostly a matter of plugging in new transformers,
     not rewriting the executor.

In short, the expander is **the semantic engine** of the shell. It sits between
“text plus syntax” and “actual processes and file descriptors”, applying all
shell rules in a controlled, testable way. Understanding it conceptually makes
it much easier to reason about why the shell behaves the way it does when you
mix variables, quotes, globs, assignments, and redirections in complex ways.
