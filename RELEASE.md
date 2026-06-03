# hellish — Release Notes

> 🔥 **hellish** is a fast, almost‑POSIX shell written from scratch in C —
> hackable, observable, and pleasant to live in.

This file is what the welcome banner points you at. It tracks what's new and
shows you how to drive the shell.

---

## v2.2.0 — *the friendly release*

**New**

- **A living welcome header.** Full terminal width, with the hellish mascot,
  the version, a one‑line description, and quick links. Shown once per
  interactive session. (`HELLISH_NO_BANNER=1` to silence.)
- **A blinking mascot in the prompt.** A tiny devil keeps blinking even while
  you type — calm, not noisy. It cheers on success and frowns on a failed
  command. (`HELLISH_NO_MASCOT=1` to silence.)
- **Origin‑aware `update`.** The shell now knows *how it was installed* and
  upgrades the right way:
  | installed via | `update --now` runs |
  |---|---|
  | npm | `npm install -g hellish-shell@latest` |
  | pnpm | `pnpm add -g hellish-shell@latest` |
  | Docker | tells you to `docker pull dlesieur/hellish-shell:latest` |
  | source checkout | `git pull && make OPT=1 all` in your clone |
  | standalone binary | re‑runs the install script |

## v2.1.0

- Welcome banner, `HELLISH_VERSION`, and the `update` builtin.
- Once‑a‑day background check for new releases (never blocks the prompt).
- Packaging: install script, Docker image, and the `hellish-shell` npm package.

## v2.0.0

- The 42sh milestone: lexer → parser → expander → executor, jobs, heredocs,
  arithmetic, globbing, history, and a large POSIX‑conformance pass.

---

## How to use it

**Run it:** `hellish` (or set it as your login shell — see Install in the
[README](README.md)).

**Everyday builtins:** `cd`, `pwd`, `export`, `unset`, `alias`, `jobs`, `fg`,
`bg`, `history`, `type`, `command`, `test`/`[`, `read`, `printf`, `umask`,
`ulimit`, `trap`, `getopts`, `set`, `local`, `return`, `exit`. Run `type <name>`
to see how any name resolves.

**Update yourself:**

```sh
update            # check GitHub for a newer release
update --now      # upgrade the way this copy was installed
update --version  # print the running version
```

**Config:** drop a `~/.hellishrc` (aliases, exports, functions, `set` options) —
it's sourced on interactive startup, the `.bashrc` analogue. A starter lives in
[assets/hellishrc.example](assets/hellishrc.example).

**Knobs:**

| variable | effect |
|---|---|
| `HELLISH_NO_BANNER=1` | hide the welcome header |
| `HELLISH_NO_MASCOT=1` | freeze the prompt mascot (no animation) |
| `HELLISH_NO_UPDATE_CHECK=1` | never check for updates in the background |

---

Source, issues, and full history: <https://github.com/Univers42/42sh>
