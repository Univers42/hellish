# What hellish Is (and Isn't)

> **Your bash, but it suggests, highlights, and doesn't make you choose between speed and comfort.**

## The pitch

Most people pick a shell and make a trade:

- **bash** — universal and compatible, but the interactive experience is dated.
- **fish** — gorgeous out-of-the-box UX (highlighting, autosuggestions), but *not* bash-compatible:
  you can't paste a bash snippet or `source` your `.bashrc`.
- **zsh** — powerful, but you need oh-my-zsh + a prompt framework + plugins to get the good parts.
- **dash** — tiny and blazing, but feature-barren (no `[[ ]]`, arrays, completion, editing).

**hellish aims for the open seat:** bash-compatible *and* fast *and* friendly out of the box. Run
your existing scripts, keep your muscle memory, and get fish-style comfort on top — without a config
marathon.

## Honest positioning

We don't claim to dethrone anything — almost nothing dethrones bash, and that's fine. Here's the
straight read:

| vs. | Where hellish stands |
|---|---|
| **bash** | **Speed parity** (geomean ~1.01×, wall 1.19× faster). Behind on 35 years of feature breadth/edge-cases. |
| **fish/zsh** | Behind on interactive polish *today* (highlighting/autosuggest are landing); ahead on **bash compatibility**. |
| **dash** | Slower and larger (dash is the minimalist floor) — but vastly more capable. |

What makes it credible rather than a toy: it's **fast**, it's **correct enough to build and boot an
entire Linux From Scratch distro under itself**, and it's held to a strict automated bar (4248
golden cases diffed against a pinned bash 5.3.9, conformance suites, ASan/leak-clean on two
allocators, norm). See **[Benchmarks](benchmarks.md)** and
**[Performance & Robustness](performance.md)**.

---

## Install

**One-liner (Linux x86-64)** — detects whether you have sudo rights and routes
itself: with sudo it installs to `/usr/bin` and registers hellish as your login
shell (the `make my_shell` path); without sudo — a 42 school machine, a shared
box — it installs to `~/.local/bin` with an rc hook (the `make user-install`
path). It also offers to set up the plugin framework and lets you pick plugins:

```sh
curl -fsSL https://raw.githubusercontent.com/Univers42/hellish/main/install.sh | sh
```

Non-interactive (scripts, CI): `sh install.sh --yes --plugins=all` (or
`--plugins=none`, `--plugins="git jump z"`, `--user`, `--system`).

**From a source checkout:**

```sh
git clone --recursive https://github.com/Univers42/hellish && cd hellish
make OPT=1 all && ./build/bin/hellish    # just run it
make user-install                        # no sudo: ~/.local/bin + rc hook
make my_shell                            # sudo: /usr/bin + login shell
```

`--recursive` matters — hellish pulls in two submodules (`vendor/libft`,
`vendor/scripts`). Forgot it? `git submodule update --init --recursive`.

**npm / pnpm / yarn:**
```sh
npm install -g hellish-shell      # or: pnpm add -g hellish-shell
```

**Docker (easiest way to try it):**
```sh
docker run --rm -it dlesieur/hellish-shell
```

Prefer to build from source in a clean container? Every supported platform has
a rung — glibc and musl, gcc and clang, five package managers:

| | rungs |
|---|---|
| glibc | `ubuntu` (24.04), `ubuntu2204`, `debian`, `arch`, `fedora`, `rocky`, `opensuse`, `void` |
| musl | `alpine`, `alpine-clang`, `alpine-ftmalloc` |
| compilers | gcc (11 → current) and clang, on both libcs, `-Werror` throughout |
| architectures | x86_64 and arm64 (native runner in CI) |

```sh
docker compose run --rm alpine     # interactive hellish on Alpine/musl
make docker-test                   # build + run the portability smoke on ALL of them
```

macOS and WSL are covered by the `Platforms` workflow rather than Docker; both
are **informational** today — see [platforms.md](platforms.md).

**Updates.** Once installed, hellish checks for newer releases in the
background (once a day, in a detached child — a dead release server costs your
prompt nothing). A pending release is announced once in the welcome panel and
sits as a quiet `⬆x.y.z` badge in the prompt until you update. `update` checks
on demand, `update --now` self-updates. Knobs: `HELLISH_BANNER=0|1`,
`HELLISH_NO_UPDATE_CHECK=1`.

---

## Roadmap

**Shipped since this page was first written** (the previous roadmap, in full):
indexed *and* associative arrays, `${v/pat/repl}`, `extglob`,
`mapfile`/`readarray`, namerefs, `coproc`, the zsh dialect + plugin corpus,
programmable completion at TAB, the zsh prompt language, and the ZLE widget
layer. See [RELEASE.md](https://github.com/Univers42/hellish/blob/main/RELEASE.md).

**Still ahead:**
- Syntax highlighting + autosuggestions at the prompt. → [details](interactive.md)
- bash-completion framework support (incremental lexing vs `shopt -s extglob`) —
  the thing that flips `progcomp` on by default.
- A native line editor (replacing readline) for fully native
  highlighting/autosuggest/completion menus.

---

## Why it's maintainable (the contributor story)

hellish is built as a set of small, API-like modules — `input → tokens → AST → expansion →
execution` — each with its own `README.md` under `src/*/`. The real maintainability asset is
**process**: every fix ships with a regression test, a conformance check vs bash, and must pass
norm + the benchmark gate before merge. That discipline is what lets the shell move fast without
rotting — and what makes it approachable to hack on.

If you like interpreters, Unix internals, or shells: clone it, `make OPT=1 all`, and poke at it.

---

See also: **[Interactive Experience](interactive.md)** · **[Bash Compatibility & Scripting](scripting.md)** · **[Performance & Robustness](performance.md)**
