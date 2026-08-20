# hellish — Release Notes

> 🔥 **hellish** is a fast, almost‑POSIX shell written from scratch in C —
> hackable, observable, and pleasant to live in.

This file is what the welcome banner points you at. It tracks what's new and
shows you how to drive the shell.

---

## v2.4.1

**Fixed**

- The session that had just installed an update immediately re-announced
  it. True — replacing the file cannot change the process already
  executing, so the running shell is still the old build — but it read as
  if the install had failed. Installing now marks the version as
  announced, and the shell just waits to be restarted, as it already
  said it would.

---

## v2.4.0 — *the update release*

**New**

- **`update` actually works.** The updater had been pointed at
  `Univers42/42sh` — a repository that does not exist — for its whole
  life, so every check 404'd and reported itself as "could not reach
  GitHub (offline?)". It now names the real repository, and so do
  `install.sh`, the npm installer and the Dockerfile.
- **An update button.** A background check (detached, 24h TTL, never on
  the startup path) discovers new releases. The next prompt carries
  `[Update] / [Later]`; `update --now` installs. The notice is printed
  between commands, never into a line you are typing.
- **Verified, atomic installs.** Download → sha256 against the checksum
  published beside the asset → run the binary and require it to report the
  version it was advertised as → `rename(2)` into place. Any failure
  leaves the installed binary untouched. Releases now ship a `.sha256`.
- **No-sudo by default.** A user-local install (`~/.local/bin`) updates
  with no elevation at all; a system-wide one asks first and says exactly
  which command will run as root. Package-managed installs (npm, pnpm,
  docker) still delegate to their package manager.
- **The banner is lazy.** It appears once a day, or when it has something
  new to say (new version, new header revision, an unannounced update) —
  not on every single shell. It also stopped wiping your screen and
  scrollback on startup.
- **`/dev/tcp` and `/dev/udp`** redirections, bash-style.

**Fixed** — a long list this cycle, all diffed against bash 5.3.9:

- `"${u:-"a b"}"` used to **crash** the shell; the whole `${...}` operator
  family mishandled nested quotes, escapes and single quotes.
- `exec 4>a 2>b` pointed fd 2 at the wrong file and closed fd 4.
- Globs sorted in ASCII order instead of locale collation, so `echo *`
  disagreed with bash in any mixed-case directory.
- A failing POSIX special builtin now aborts a non-interactive shell.
- `readonly` reported success on every error; `unset` removed read-only
  variables.
- A fatal error inside a subshell exited 127 instead of 1.
- The prompt is written in one syscall, so type-ahead can no longer be
  echoed into the middle of a colour escape (the `38;2;112` garbage).
- `history` shows multi-line commands the way bash does.
- Background job labels keep their opening `(`, `{`, `for`, `if`.

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
