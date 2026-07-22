# hellish — Release Notes

> 🔥 **hellish** is a fast, almost‑POSIX shell written from scratch in C —
> hackable, observable, and pleasant to live in.

This file is what the welcome banner points you at. It tracks what's new and
shows you how to drive the shell.

---

## v2.2.0 — *the friendly release*

**New**

- **A welcome panel.** A full‑width, two‑column box: a greeting
  and the **42 logo in salmon** on the left; getting‑started tips and a
  "What's new" block on the right. Shown once per session.
  (`HELLISH_NO_BANNER=1` to silence.)
- **A one‑time entrance animation.** On the very first run the logo draws in,
  row by row, like the GitHub Copilot CLI banner; every later startup is
  instant. (`HELLISH_ANIM=1` to replay it, `HELLISH_NO_ANIM=1` to skip.)
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
[hellishrc.example](hellishrc.example).

**Knobs:**

| variable | effect |
|---|---|
| `HELLISH_NO_BANNER=1` | hide the welcome panel entirely |
| `HELLISH_ANIM=1` | replay the entrance animation on this startup |
| `HELLISH_NO_ANIM=1` | never play the entrance animation |
| `HELLISH_NO_UPDATE_CHECK=1` | never check for updates in the background |

---
