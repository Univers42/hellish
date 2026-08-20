# hellish — Release Notes

> 🔥 **hellish** is a fast, almost‑POSIX shell written from scratch in C —
> hackable, observable, and pleasant to live in.

This file is what the welcome banner points you at. It tracks what's new and
shows you how to drive the shell.

---

## v2.7.0 — *the it-tells-you release*

**New**

- **A pending update stays visible in the prompt.** The "update available"
  notice is said once and then never again — deliberately, because a banner
  on every prompt is how people learn to stop reading banners. But miss it
  once and nothing brought it back. Now a quiet badge persists for as long
  as the update is actually waiting:

  ```
  ╭─ you in ~/project ⬆2.7.1 ──────────────────── 15:04 ─╮
  ╰─❯
  ```

  Self-spacing (invisible when there is nothing to say), dropped on a narrow
  row like the other badges, gone by itself once you update, and silenced by
  `HELLISH_NO_UPDATE_CHECK`. Custom prompts get `\U`.

**Fixed**

- **Releases stop reporting themselves as failed.** Every tag showed
  `release -> failure` while shipping perfectly good artifacts: the Docker
  Hub push (a secondary channel — GHCR publishes the same image) was failing
  on an expired token and taking the whole run's status with it. A
  permanently red release run makes the next genuine failure invisible.

---

## v2.6.0 — *the ask-it-what-it-is release*

**New**

- **`hellish --version`.** It used to answer "invalid option". Now it prints
  the version, the asset the updater will fetch, and the repo it will fetch
  it from — because a build pointing somewhere unexpected is worth seeing
  *before* it downloads anything. Exits 0 without sourcing a startup file or
  reading stdin, so package managers and CI can probe it safely.

**Fixed**

- **`printf` accepts length modifiers.** `printf "%ld\n" 9999999999` failed
  with `` `%l': invalid format character ``. Every C-habit format string hit
  this: `%ld %lld %zu %hd %jd %td %Lf`. bash accepts and ignores them; so do
  we now, measured against it rather than guessed.
- **No more `git <defunct>`.** The prompt's async git check was reaped only
  during a prompt render, so any scan finishing while a foreground command
  ran left a zombie for that command's whole life — one per nested shell.
  It is double-forked onto init now; there is no child to reap.
- **`top &` works, and `^Z` no longer wedges the terminal.** Background jobs
  keep the tty in an interactive shell (POSIX only redirects stdin to
  `/dev/null` when job control is *off*), and the foreground wait passes
  `WUNTRACED` — without it a stopped child never satisfied the wait, so the
  shell blocked in `waitpid` forever while the kernel echoed keystrokes that
  nothing ran.
- **Whole prompt frames.** The prompt writers used a single unchecked
  `write()`. `write(2)` may transfer fewer bytes than asked and report that
  as success, so the tail was silently dropped.

**Changed**

- **The prompt animation ships off.** It was the only thing that wrote to
  your terminal while you were not typing — 6.4KB of escape traffic every
  2.5 seconds at an idle prompt. Set `HELLISH_ANIM=spinner|pulse|ember` to
  opt back in. Nothing else about prompt customisation changes.
- `HELLISH_ANIM` now means what it says. It only ever governed `\A` in a
  custom `PS1`; the built-in prompt animated regardless, with no way to stop
  it short of writing a whole custom prompt.

**Under the hood**

- CI runs the suites that previously only ran when somebody remembered:
  the pty gates, startup argv parsing, login file order, the help table,
  the update path, the whole-program corpus, allocator parity across both
  heaps, and benchmarks (published, never a gate).
- `vendor/libft` gained `%ld`-family support and the 64-bit correctness
  fixes that exposed, and its own CI is green across ubuntu 22.04/24.04 ×
  gcc/clang for the first time.

---

## v2.5.0 — *the help release*

**New**

- **`help`.** `help` lists what the shell can do, grouped by what you want
  to do with it rather than alphabetically; `help NAME` explains one thing;
  `help -s NAME` prints just the form.
- **Syntax topics.** `help for`, `help case`, `help function`,
  `help redirection`, `help pipeline`, `help $((` — because knowing that
  `for` is a keyword does not tell you how to write one.

Its exit status matches bash exactly (0 when at least one topic matched),
and a test derives the expected topic list from the builtin dispatch table,
so a builtin added without a help entry fails the build rather than
quietly going undocumented.

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
