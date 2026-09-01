# hellish 🐚🔥

**A from-scratch, almost-POSIX shell in C — runs like a minimalist shell,
reads like bash, and loads real oh-my-zsh plugins.**

hellish is engineered like a teaching lab and benchmarked like a production
shell: `input → lexer → parser → word reparser → heredoc → expander →
executor`, each a small readable module, one `t_shell` struct as the single
source of truth. It diffs **byte-for-byte against a pinned bash 5.3.9** on
4248 golden cases, runs clean under AddressSanitizer, ships **two allocators
you swap at compile time**, and races **#3 of 9** against the shells you
actually use — faster than bash, zsh, mksh, yash and fish.

## Install in one command

```sh
curl -fsSL https://raw.githubusercontent.com/Univers42/hellish/main/install.sh | sh
```

The installer detects whether you have sudo rights and routes itself — system
install + login shell with sudo, `~/.local/bin` + rc hook without (the 42
school machine path) — then offers the plugin framework and a pick-list of
plugins. Details and every other route (source, npm, Docker):
**[What hellish is + Install](product.md)**.

## Find your way

| you want to… | read |
|---|---|
| know what it is and install it | [Product & Install](product.md) |
| `man hellish` — invocation, startup files, grammar, everything | [The manual](manual.md) |
| look up one builtin | [Builtins reference](builtins/index.md) — all 69, from the shell's own `help` |
| install and write plugins (with screenshots) | [Plugins](plugins.md) |
| live at the prompt — themes, completion, zsh dialect | [Interactive Experience](interactive.md) |
| run your bash scripts on it | [Bash Compatibility & Scripting](scripting.md) |
| see the numbers | [Benchmarks](benchmarks.md) · [Performance](performance.md) |
| understand how it's built | [Architecture](architecture.md) |
| run it on your OS | [Platforms](platforms.md) |
| fix something | [Troubleshooting](troubleshoot/README.md) · [Fix write-ups](fixes/README.md) |

Source, issues and releases live at
[github.com/Univers42/hellish](https://github.com/Univers42/hellish).
