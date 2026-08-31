# Architecture

> The whole shell in one breath, then where to read more. Each module under
> [`src/`](https://github.com/Univers42/hellish/tree/main/src) carries its own
> `README.md` with the real design notes — this page is the map, those are the
> territory.

## The pipeline

`main → on() → repl_shell() → parse_and_execute_input()`, conceptually:

```
input → lexer → parser (AST) → word reparser → heredoc → expander → executor
```

- The **lexer** slices tokens straight out of the input buffer — no copies.
- The **parser** builds a `t_ast_node` tree that *deliberately keeps raw
  tokens*, so loops and functions re-expand each iteration without re-parsing.
- The **expander** runs *lazily, per simple command*, during the executor's
  tree walk — the pipeline is conceptual, not literal.
- **Heredocs** are gathered in a pre-exec pass; bodies land in temp files and
  are rewritten as plain redirects.
- **Command substitution** has a forkless fast path (`cmdsub_fast`) for
  provably side-effect-free bodies; anything else forks.

One struct, `t_shell` (`incs/shell.h`), is the single source of truth; every
subsystem takes `t_shell *`. Subshells fork and the child inherits a copy.

| stage | entry point | design notes |
|---|---|---|
| lifecycle / REPL | `src/core/shell.c` | [`src/core/README.md`](https://github.com/Univers42/hellish/blob/main/src/core/README.md) |
| lexer | `tokenizer()` | [`src/lexer/README.md`](https://github.com/Univers42/hellish/blob/main/src/lexer/README.md) |
| parser | `parse_tokens()` | [`src/parsing/README.md`](https://github.com/Univers42/hellish/blob/main/src/parsing/README.md) |
| word reparser | `reparse_words()` | [`src/word_splitting/README.md`](https://github.com/Univers42/hellish/blob/main/src/word_splitting/README.md) |
| heredoc | `gather_heredocs()` | [`src/heredoc/README.md`](https://github.com/Univers42/hellish/blob/main/src/heredoc/README.md) |
| expander | `expand_simple_command()` | [`src/expander/README.md`](https://github.com/Univers42/hellish/blob/main/src/expander/README.md) |
| executor | `execute_top_level()` | [`src/execution/README.md`](https://github.com/Univers42/hellish/blob/main/src/execution/README.md) |

Other modules: `builtins` (65 names, O(1) hash dispatch), `environment`,
`arith` (a self-contained lexer/parser/eval for `$(( ))`), `glob`, `alias`,
`job_control`, `completion` + `editing` (readline, vi/emacs, and the ZLE
widget layer), `infrastructure` (input driver, prompt, history, error
reporting).

## The zsh dialect

zsh syntax is not bash and not POSIX, so **none of it is reachable unless
something arms the mode**: `set -o zsh`, `emulate zsh`, or sourcing a `.zsh`
file (restored when the file finishes). There is no heuristic on file content —
the golden suite pins the bash meaning of the same text by construction. See
[Interactive Experience](interactive.md) and the plugin corpus in
[`tests/plugin_corpus_test.py`](https://github.com/Univers42/hellish/blob/main/tests/plugin_corpus_test.py).

## The two allocators (SAFE)

Every allocation goes through one `xmalloc`/`xfree` macro family that resolves
**at compile time** to libc `malloc` or our own `ft_malloc` — no pointer ever
crosses heaps. So you can A/B the exact same shell on two completely different
allocators and prove they produce byte-identical output:

```sh
cd tests && ./verify_alloc.sh   # builds BOTH heaps, proves output parity + zero leaks
```

`ft_malloc` (in the `vendor/libft` submodule) is a slab + big-alloc allocator
with an O(1) free path; on `SAFE=0` its own live-bytes oracle
(`HELLISH_ALLOC_STATS=1`) replaces ASan as the leak gate.

## Build matrix

| Command | Optimization | Allocator | Sanitizers | Use it for |
|---|---|---|---|---|
| `make all` | `-O0 -g3` | libc (`SAFE=1`) | ASan + LeakSanitizer | dev, debugging, leak hunts |
| `make OPT=1` | `-O3 -flto` | **`ft_malloc`** (`SAFE=0`) | none | speed, benchmarks, daily driving |
| `make MODE=relwithdebinfo` | `-O2 -g` | `ft_malloc` | none | bugs that only appear optimized |
| `make OPT=1 SAFE=1` | `-O3 -flto` | libc | none | optimized build on the battle-tested heap |

An explicit `SAFE=…` on the command line always wins. Objects live in
per-mode, per-allocator trees, so modes never share objects. `make` with no
target prints the self-documenting help page.

## Testing & quality gates

Nothing lands without all of these green from a clean tree:

- **`make test`** — 4000+ golden cases, each diffed (stdout + exit status +
  files written) against a **pinned bash 5.3.9** (`make oracle`).
- **`tests/run_scripts.sh`** — whole real programs vs `bash --posix`.
- **`make pty-test`** — every `tests/*.py`, discovered by glob: real-pty
  coverage of the interactive paths `-c` can't reach.
- **`cd tests && ./verify_alloc.sh`** — output parity + zero leaks on both heaps.
- **`make plugin-corpus`** — 13 real third-party plugins sourced against the
  release *and* the ASan build, each with a declared expectation.
- **`make conformance`** — Oils spec + mksh `check.t`, gated so a pass-count
  drop fails.
- **`make norm`** — 42 norminette clean.

See also: **[Benchmarks](benchmarks.md)** · **[Performance](performance.md)** ·
**[Platforms](platforms.md)** · the developer guide in
[`DEV_DOC.md`](https://github.com/Univers42/hellish/blob/main/DEV_DOC.md)
