# <div style="background-color: rgba(231, 140, 153, 1); color: rgba(11, 86, 65, 1); padding: 20px;">Hellish – A Modern Educational Shell

_This project was created as part of the 42 curriculum by dlesieur, alcacere_.

> A from‑scratch, almost‑POSIX shell written in C, designed to be **hackable**,
> **observable**, and **pleasant** to use.

---

## Install

**One-liner (Linux x86-64):**

```sh
curl -fsSL https://raw.githubusercontent.com/Univers42/42sh/main/install.sh | sh
```

**npm / pnpm / yarn:**

```sh
npm install -g hellish-shell      # or: pnpm add -g hellish-shell
```

**Docker:**

```sh
docker run --rm -it <login>/hellish-shell
```

**From source:**

```sh
git clone --recursive https://github.com/Univers42/42sh && cd 42sh
make OPT=1 all && ./build/bin/hellish
```

Once installed, the shell checks for newer releases in the background (once a
day, never blocking the prompt) and flags one in the welcome banner. Run
`update` to check on demand, or `update --now` to self-update the binary.
Opt out with `HELLISH_NO_UPDATE_CHECK=1` (and `HELLISH_NO_BANNER=1`).

---

## <div style="color:rgba(250,75,100,90)" >Documentation

The four pillars of hellish, each its own deep-dive (status: ✅ shipped · 🚧 in progress · 📋 planned):

| Pillar | What's inside |
|---|---|
| **[Interactive Experience](docs/interactive.md)** | Editing, history, completion, prompt — plus syntax highlighting & autosuggestions (roadmap) |
| **[Bash Compatibility & Scripting](docs/scripting.md)** | Feature matrix vs bash, the "it built an LFS distro" proof, arrays roadmap |
| **[Performance & Robustness](docs/performance.md)** | Benchmarks vs bash & dash, the test/conformance/leak gates |
| **[What hellish Is (and Isn't) + Install](docs/product.md)** | The pitch, honest positioning, install methods, roadmap |

---

## <div style="color:rgba(250,75,100,90)" >What Is This Project?

`hellish` is a full interactive shell implementation done as a long‑running
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

## <div style="color:rgba(250,75,100,90)" >Quick Start

### <div style="color:rgba(195, 67, 103, 1)">1. Build

The project comes with a `Makefile` at the root. The usual targets are:

```sh
make          # default debug build (AddressSanitizer + LeakSanitizer), SAFE=1
make OPT=1    # optimized build (-O3 -flto), SAFE=0 — the custom ft_malloc heap
make clean    # Remove object files
make fclean   # Remove objects + binaries
make re       # Full rebuild
```

**Allocator toggle (`SAFE`).** Every allocation in the shell goes through a
single macro family (`xmalloc` / `xcalloc` / `xfree`) that is switched **at
compile time** between libc `malloc`/`free` and our own `ft_malloc`. `SAFE=1`
(the debug default) uses libc so AddressSanitizer stays meaningful; `SAFE=0`
(the `OPT=1` default) uses `ft_malloc` — faster, less battle-tested. Override
either way (e.g. `make OPT=1 SAFE=1` for an optimized libc build); the build
banner tells you which heap is active. `make my_shell` installs an
`OPT=1 SAFE=1` binary. Both backends pass the full test suite.

After `make`, the binary is `./build/bin/hellish`:

```sh
./build/bin/hellish
```

and you’ll be dropped into the shell prompt.

### <div style="color:rgba(195, 67, 103, 1)">2. Try It as an Interactive Shell

You don’t have to replace your login shell to experiment. A safe way to test:

```sh
# From your usual shell
$ ./build/bin/minishell
minishell$ echo "Hello from minishell"
```

You can exit back to your normal shell with:

```sh
minishell$ exit
```

### <div style="color:rgba(195, 67, 103, 1)">3. (Optional) Use as Your Login Shell

> **Warning**: Only do this if you understand what you’re doing and trust the
> binary. A broken shell as `$SHELL` can make your life painful.

#### <div style="color:rgba(189, 82, 95, 1)">From Now on You Get Two Solutions

##### <div style="color:rgba(159, 93, 101, 1)"> Either makefile auto tools (recommended)

```bash
$ make my_shell

# or rebaptize hellish to name to your convenience
$ make  my_shell BAPTIZE_SHELL=<name>

```

Using those command will use the sudo rights and set this shell as your main shell into your computer. I approve ! and you  will be surpises how fast it is, especially if you compile the binary 

```bash
make OPT=1 # faster compilation; faster running time..   --- > hooo yeah
```


##### <div style="color:rgba(159, 93, 101, 1)"> or manually
1. Find the full path to the binary, e.g.:

   ```sh
   $ pwd
   /home/you/sh42
   $ ls sh42
   # ...
   $ which ./minishell
   /home/you/sh42/build/bin/minishell
   ```

2. (System‑wide) You would normally add this path to `/etc/shells`.
3. Change your shell for your user:

   ```sh
   chsh -s /home/you/sh42/build/bin/minishell
   ```

4. Log out / log in again, or open a new terminal.

Because this is a personal/educational shell, it is usually better to keep it
as an **alternative shell** you launch explicitly (`./minishell`) rather than
replacing `/bin/bash` or `/bin/zsh` everywhere.

---

## <div style="color:rgba(250,75,100,90)" >Why This Shell Exists

### <div style="color:rgba(195, 67, 103, 1)">1. To learn shells *properly*

Writing a shell is one of the deepest ways to understand Unix:

- How lexers and parsers work together.
- How quoting and expansion rules interact.
- How pipelines, redirections, and process substitutions really work.
- How job control, signals, and heredocs are implemented in practice.

`hellish` was created to explore all of that with **realistic complexity** but
without the decades of historical baggage of a production shell.

> This shell called `hellish` was baptized like that due to 3 months of battling to create this giant motor of shell linux. That  we think you would like..

### <div style="color:rgba(195, 67, 103, 1)">2. To design a clean, modular architecture

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

### <div style="color:rgba(195, 67, 103, 1)">3. To be *usable*, not just theoretical

This is not a toy REPL: the goal is to be something you **could** use for real
work:

- Proper prompts with user, cwd, git branch, venv, and time.
- Command history across sessions.
- Syntax close enough to your usual shell that you don’t have to relearn
  everything.

---

## <div style="color:rgba(250,75,100,90)" >POSIX Compliance (and Where We Are)

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

## <div style="color:rgba(250,75,100,90)" >Internal Design Highlights

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
  - Uses a small `parse_stack` plus `RES_GETMOREINPUT`/`RES_ERR` protocol
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

## <div style="color:rgba(250,75,100,90)" >Using the Makefile and Build Profiles

The root `Makefile` is the main way to work with the project. Typical
scenarios:

- **Development** – keep symbols and assertions:

  ```sh
  make
  ./build/bin/minishell
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

## <div style="color:rgba(250,75,100,90)" >Extensibility </div>

Because everything is modular, adding features is usually localized:

- New syntax → adjust lexer/operator tables + parser grammar.
- New expansion behavior → patch expander or reparser.
- New builtins → plug into the executor’s builtin table.
- New prompt elements → extend the prompt helpers.

The internal docs under `src/*/README.md` are written to explain *concepts*,
not just functions, so you can understand how a change propagates through the
pipeline.

---

## <div style="color:rgba(250,75,100,90)" >A Short History of the Project

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

## <div style="color:rgba(250,75,100,90)" >References & Documentation

This shell implementation is built upon the foundational logic of the POSIX standard, supplemented by classic systems programming literature and modern implementation guides.

### <div style="color:rgba(195, 67, 103, 1)">Standards & Manuals
* **[POSIX Standard (Shell & Utilities)](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html)** – The primary authority on shell grammar, environment variables, and command execution.
* **[GNU Bash Reference Manual](https://www.gnu.org/software/bash/manual/bash.html)** – Essential for comparing standard POSIX behavior against modern extensions.
* **[Shell Man Page](assets/manual/man.1)** – Local documentation for this specific shell's usage.

### <div style="color:rgba(195, 67, 103, 1)">Architecture & Logic
* **[Crafting Interpreters](https://craftinginterpreters.com/)** – Guidance on the lexing and parsing patterns used to transform input into executable commands.
* **[Architecture of the Shell Motor](assets/pdfs/builtins.pdf)** – Detailed breakdown of the internal execution engine and process lifecycle.
* **[Grokking Algorithms](https://www.manning.com/books/grokking-algorithms)** – Reference for implementing the data structures required for environment variables and command history.

### <div style="color:rgba(195, 67, 103, 1)">Implementation Guides
* **[Stephen Brennan's Guide: Write a Shell in C](https://brennan.io/2015/01/16/write-a-shell-in-c/)** – A foundational resource for handling the `fork-exec` loop and basic process management.
* **[Writing Your Own Shell](assets/pdfs/Chapter5-WritingYourOwnShell.pdf)** – Advanced insights into signal handling and terminal control.
* **[Code Crafters: Build Your Own Shell](https://app.codecrafters.io/courses/shell/overview)** – Test-driven development challenges that helped validate our implementation.

### <div style="color:rgba(195, 67, 103, 1)">Advanced Topics & Strict Mode
* **[Unofficial Bash Strict Mode](http://redsymbol.net/articles/unofficial-bash-strict-mode/)** – Best practices for error handling and preventing unexpected behavior.
* **[Restricted Bash (rbash)](assets/pdfs/rbash.pdf)** – Documentation used to understand shell security and execution constraints.
* **[Subject sh42 / minishell](assets/pdfs/minishell.pdf)** – Technical requirements and constraints for the project's core features.

### <div style="color:rgba(195, 67, 103, 1)">Tester
- [minishell tester](https://github.com/LucasKuhn/minishell_tester),  powered by our mind and based on his.

## <div style="color:rgba(250,75,100,90)" >🛠 Use Of AI

**Documentation & Research** - Used to clarify ambiguities in edge cases where conventions differ and to research complex topics not detailed in standard textbooks, ensuring realistic solutions within the project's scope.

**Concept Explanation** - Served as a study aid to solidify the understanding of abstract functionalities and architectural requirements necessary for building a shell.

**Code Generation** Not used for logic implementation; limited to clarification and study.

## <div style="color:rgba(250,75,100,90)" >❤️ Why We Like Working on It

- It’s a genuine, real‑world interpreter you can reason about.
- It rewards careful thought about **APIs, ownership, and semantics**.
- It’s fun to see it get closer and closer to a real login shell you could use
  daily.

If you enjoy interpreters, Unix internals, or just want to see how a modern
from‑scratch shell can be structured, this project is meant to be
approachable, hackable, and instructive.

Pull it, `make`, run `./build/bin/minishell`, and poke at it.

Welcome to `hellish`. 🐚
