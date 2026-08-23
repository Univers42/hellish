# Interactive Experience

> **hellish at the prompt** — the part you actually live in.
> Status legend: ✅ shipped · 🚧 in progress · 📋 planned

hellish is built to be *comfortable*, not just correct. The lane we're chasing is
**"your bash, but it suggests, highlights, and is fast"** — fish-grade comfort without
giving up bash compatibility or speed. This page covers what the prompt does today and
what's landing next.

---

## Line editing ✅

Full GNU-readline-backed editing, so every muscle-memory binding you already have works:

- **Emacs and vi modes** — toggle the editing mode; standard keymaps (`set -o vi` / `-o emacs`).
- **Multi-line input** — unfinished constructs (open quote, pipe, `if`/`for`/heredoc) re-prompt
  with a continuation prompt instead of erroring; the buffer is reassembled transparently.
- **ANSI- and multibyte-aware prompt** rendering, so colorful prompts and wide characters don't
  desync the cursor.
- **Ctrl-C / Ctrl-D / signal handling** behaves like a real login shell, including inside heredocs.

## History ✅

- Persistent history in `$HOME`, with escaping so multi-line commands round-trip safely.
- `history`, `fc`, and history expansion.
- Reverse search via the readline binding you expect.

A compound command (`if`, `for`, `while`, `case`, `{ }`) is stored as **one**
history entry. By default it is joined onto a single line the way bash's
`cmdhist` does — a boundary newline becomes `; `, or a bare space where a `;`
would not parse:

```sh
if true; then    # typed over three lines
echo hi
fi
# recalled as:   if true; then echo hi; fi
```

`shopt -s lithist` keeps the newlines instead, so recall gives back the
multi-line buffer you actually typed:

```sh
shopt -s lithist
# recalled as:   if true; then
#                echo hi
#                fi
```

Both forms re-execute identically, and either survives a restart — the history
file escapes embedded newlines. Put `shopt -s lithist` in `~/.hellishrc` to
make it the default.

## Completion ✅

Readline-driven completion with context-aware generators:

| Context | Completes |
|---|---|
| First word | commands on `$PATH` + builtins |
| `$…` / `${…}` | environment & shell variable names |
| Argument | files and directories |

It's functional and fast today. The fish/zsh-grade layer (menus, descriptions, fuzzy matching)
is on the roadmap below.

## Prompt ✅

Rich, configurable prompt elements: user, cwd, git branch, virtualenv, and time — rendered with
correct width accounting so segments and colors line up.

## Update channel ✅

hellish checks for newer releases in the background (once a day, never blocking the prompt — the
check runs in a detached child, so a slow or dead release server costs the shell nothing). You are
told twice, and without asking: the welcome banner announces each new release once, and the prompt
carries a quiet `⬆<version>` badge for as long as the update is actually pending. `update` checks on
demand; `update --now` self-updates the binary.

Opt out with `HELLISH_NO_UPDATE_CHECK=1`. The banner has one knob, `HELLISH_BANNER=0|1`, to force it
off or on; `HELLISH_NO_BANNER=1` and `HELLISH_ALWAYS_BANNER=1` are the older names and still work.

---

## Roadmap — the differentiators

These are the features that turn "a solid shell" into "a shell you choose." They build on
infrastructure that already exists: hellish's **tokenizer emits typed tokens and tolerates
incomplete input**, which is exactly the "what to color" half of live highlighting.

### Syntax highlighting 🚧 *(Week 1)*

Color the command line **as you type** — commands vs. unknown commands, keywords, strings,
quotes, operators, `$variables`, and comments — by tokenizing the live buffer and mapping each
token type to a color. Implemented via readline's redisplay hook, so it layers onto the existing
editor without a rewrite.

```text
$ git commit -m "fix: close [[ ]] on metachar-adjacent ]]"
  └cmd └sub    └flag └─────────────── string ──────────────┘
```
*(commands green, flags cyan, strings yellow — illustrative)*

### Autosuggestions 🚧 *(Week 1)*

fish-style "ghost text": as you type, the longest matching command from history is shown dimmed
after the cursor; **→ / End** accepts it. Reuses the loaded history and the same redisplay path
as highlighting.

```text
$ make OPT=1 all          ← typed
$ make OPT=1 all && ./build/bin/hellish   ← suggested (dim), press → to accept
```

### Completion polish 🚧 *(Week 1, time-boxed)*

Case-insensitive matching and a short description column for builtins, keeping the existing
generators.

### Beyond this cycle 📋

A **custom line editor** (replacing readline) is the long-term path to fully native highlighting,
autosuggestions, and completion menus — the way fish and zsh's ZLE do it. It's a larger project
tracked separately; the readline-hook approach above delivers the experience first.

---

## Try it

```sh
make OPT=1 all && ./build/bin/hellish
```

See also: **[Bash Compatibility & Scripting](scripting.md)** · **[Performance & Robustness](performance.md)** · **[What hellish is + Install](product.md)**
