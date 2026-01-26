# 🐚 sh42 – A Modern Educational Shell

> A from‑scratch, almost‑POSIX shell written in C, designed to be **hackable**,
> **observable**, and **pleasant** to use.

---

## ✨ What Is This Project?

`sh42` is a full interactive shell implementation done as a long‑running
learning project. It aims to be:

- **Very close to POSIX semantics** (and moving closer over time),
- **Fully modular inside** (lexer, parser, expander, glob, heredoc, etc.),
- **Comfortable to use every day**, not just as a school exercise,
- **Easy to extend and debug** if you like playing with interpreters.

You can think of it as a cross between a traditional Bourne‑style shell and a
teaching lab:

- Internally, everything is structured like a set of APIs
  (input → tokens → AST → expansion → execution).
- Externally, it behaves like a regular shell: you can run commands, use
  pipelines, redirects, subshells, process substitution, here‑documents,
  environment variables, history, and more.

---

## 🚀 Quick Start

### 1. Build

The project comes with a `Makefile` at the root. The usual targets are:

```sh
make          # Build the default (debug/normal) shell binary
make debug    # Build with extra debug flags (if provided in the Makefile)
make clean    # Remove object files
make fclean   # Remove objects + binaries
make re       # Full rebuild
```

The resulting binary is typically named something like `sh42` (or as defined
in the `Makefile`). After `make`, you can run:

```sh
./sh42
```

and you’ll be dropped into the shell prompt.

### 2. Try It as an Interactive Shell

You don’t have to replace your login shell to experiment. A safe way to test:

```sh
# From your usual shell
$ ./sh42
sh42$ echo "Hello from sh42"
```

You can exit back to your normal shell with:

```sh
sh42$ exit
```

### 3. (Optional) Use as Your Login Shell

> **Warning**: Only do this if you understand what you’re doing and trust the
> binary. A broken shell as `$SHELL` can make your life painful.

1. Find the full path to the binary, e.g.:

   ```sh
   $ pwd
   /home/you/sh42
   $ ls sh42
   # ...
   $ which ./sh42
   /home/you/sh42/sh42
   ```

2. (System‑wide) You would normally add this path to `/etc/shells`.
3. Change your shell for your user:

   ```sh
   chsh -s /home/you/sh42/sh42
   ```

4. Log out / log in again, or open a new terminal.

Because this is a personal/educational shell, it is usually better to keep it
as an **alternative shell** you launch explicitly (`./sh42`) rather than
replacing `/bin/bash` or `/bin/zsh` everywhere.

---

## 🎯 Why This Shell Exists

### 1. To learn shells *properly*

Writing a shell is one of the deepest ways to understand Unix:

- How lexers and parsers work together.
- How quoting and expansion rules interact.
- How pipelines, redirections, and process substitutions really work.
- How job control, signals, and heredocs are implemented in practice.

`sh42` was created to explore all of that with **realistic complexity** but
without the decades of historical baggage of a production shell.

### 2. To design a clean, modular architecture

Many shells grew organically. This one was designed from day one to have
**clear internal boundaries**:

- **Infrastructure** – input, readline buffering, prompts, history.
- **Lexer** – Crast‑style cursor‑based tokenizer with debug tables.
- **Parser** – hand‑written, explicit grammar building an AST.
- **Word reparser / splitting** – refines words into quote/envvar subtokens.
- **Expander** – environment, substitutions, tilde, glob, process subs.
- **Globbing** – separate pattern engine with its own API.
- **Heredoc** – pre‑processing step that resolves here‑documents.
- **Executor** – runs pipelines, builtins, external programs.

The result is a codebase that’s surprisingly readable for a shell, and that
invites experimentation.

### 3. To be *usable*, not just theoretical

This is not a toy REPL: the goal is to be something you **could** use for real
work:

- Proper prompts with user, cwd, git branch, venv, and time.
- Command history across sessions.
- Syntax close enough to your usual shell that you don’t have to relearn
  everything.

---

## 📐 POSIX Compliance (and Where We Are)

The project does **not** claim to be a complete POSIX shell, but it aims to be
“POSIX‑shaped”:

- Grammar and constructs largely follow the POSIX sh grammar
  (pipelines, lists, subshells, redirects, here‑documents).
- Expansion order and rules (tilde, parameter expansion, command and
  arithmetic substitution, word splitting, globbing) are implemented in the
  classic pipeline, with some corner cases still being refined.
- Builtins and environment semantics are implemented with POSIX in mind.

It is **in good shape** to become more and more compliant:

- The parser is explicit and easy to align with the standard grammar.
- The expander is modular, making it possible to adjust behavior without
  touching parsing or execution.
- Tests and usage keep pushing it toward the behavior users expect from a
  POSIX‑like shell.

There are also intentional modern/experimental bits (process substitution,
rich prompts) that go beyond strict POSIX, but these are layered *on top* of a
POSIX‑inspired core.

---

## 🧱 Internal Design Highlights

While each submodule has its own README, here is the bird’s‑eye view:

- **Input / Prompt / History**
  - Buffered readline wrapper that handles multi‑line input, Ctrl‑C, and
    non‑TTY stdin.
  - ANSI‑ and multibyte‑aware prompt rendering, so colorful prompts don’t
    confuse the cursor.
  - History file in `$HOME`, with escaping to safely store multi‑line commands.

- **Lexer**
  - Cursor‑based, inspired by the Crast interpreter style.
  - Longest‑match operator table (for `||`, `&&`, `<<-`, `<(`, `>(`, etc.).
  - Debug printing: pretty token tables with colors and aligned columns.

- **Parser**
  - Hand‑written descent-ish parser building an AST of `t_ast_node`.
  - Distinguishes simple lists, pipelines, commands, subshells, redirects,
    process substitutions.
  - Uses a small `parse_stack` plus `RES_MoreInput`/`RES_FatalError` protocol
    to drive multi‑line prompts and error messages.

- **Word Reparser / Splitting**
  - Takes coarse `AST_WORD` nodes and refines them into quote/envvar/segment
    subtokens (`TT_SQWORD`, `TT_DQWORD`, `TT_ENVVAR`, `TT_DQENVVAR`, etc.).
  - Detects `NAME=value` assignment words and re‑tags them as
    `AST_ASSIGNMENT_WORD` with key/value structure.

- **Expansion & Globbing**
  - Expander walks the AST and applies shell semantics:
    env vars, command and arithmetic substitution, tilde, word splitting,
    globbing, and construction of `t_executable_cmd` structs.
  - Globbing is delegated to a dedicated module that tokenizes patterns and
    walks directories, returning sorted matches.

- **Heredoc**
  - Pre‑scan the AST for `<<` / `<<-` here‑documents.
  - Read user input with special prompts (`heredoc>` / `pipe heredoc>`),
    optionally expanding `$`, and writing to temporary files wired into
    redirects.

All of this together makes the shell feel cohesive and “engineered”, not just
stitched together.

---

## 🛠 Using the Makefile and Build Profiles

The root `Makefile` is the main way to work with the project. Typical
scenarios:

- **Development** – keep symbols and assertions:

  ```sh
  make
  ./sh42
  ```

- **Debugging internals** – enable lexer/parser debug modes via options or
  environment flags (see `src/infrastructure` and `src/lexer` docs), then
  recompile.

- **Optimized builds** – the file you’re reading used to list a huge set of
  aggressive optimization flags (LTO, vectorization, etc.). Those can be
  re‑enabled in the Makefile when you want to experiment with performance.
  For normal development, a saner subset is usually chosen.

The key idea: **all interaction goes through `make`**. You don’t have to learn
custom scripts; just inspect the `Makefile` and pick the targets you need.

---

## 💡 Extensibility

Because everything is modular, adding features is usually localized:

- New syntax → adjust lexer/operator tables + parser grammar.
- New expansion behavior → patch expander or reparser.
- New builtins → plug into the executor’s builtin table.
- New prompt elements → extend the prompt helpers.

The internal docs under `src/*/README.md` are written to explain *concepts*,
not just functions, so you can understand how a change propagates through the
pipeline.

---

## 📜 A Short History of the Project

This shell has been built incrementally over many sessions of experimentation
and refactoring:

1. **First steps** – a minimal REPL that could read a line, split it on
   spaces, and `execve` a binary.
2. **Lexer + parser** – introduction of a real token stream and an AST so that
   pipelines, subshells, and redirects could be represented cleanly.
3. **Expansion engine** – careful implementation of the classic shell
   transformation pipeline (tilde, variables, substitutions, word splitting,
   globbing) on top of the AST.
4. **Heredocs, process substitution, and history** – adding more advanced
   features while keeping the architecture clean.
5. **Reparser and infrastructure polish** – introducing the word‑splitting
   reparser and the modular infrastructure for prompts, multibyte handling,
   and debug loops.

Throughout this evolution, the guiding principles were:

- **Understandability over shortcuts** – prefer more code and clearer
  boundaries to clever but opaque hacks.
- **API‑like modules** – each subsystem should feel like a mini library with a
  small contract.
- **Room to grow** – it should be possible to tighten POSIX compliance or add
  features without tearing everything apart.


## 🛠 Use Of AI

> to fill in ...

---

## ❤️ Why We Like Working on It

- It’s a genuine, real‑world interpreter you can reason about.
- It rewards careful thought about **APIs, ownership, and semantics**.
- It’s fun to see it get closer and closer to a real login shell you could use
  daily.

If you enjoy interpreters, Unix internals, or just want to see how a modern
from‑scratch shell can be structured, this project is meant to be
approachable, hackable, and instructive.

Pull it, `make`, run `./sh42`, and poke at it.

Welcome to `sh42`. 🐚