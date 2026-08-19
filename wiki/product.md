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
entire Linux From Scratch distro under itself**, and it's held to a strict automated bar (2481+
tests, conformance vs `bash --posix`, ASan/leak-clean, norm). See
**[Performance & Robustness](performance.md)**.

---

## Install

**One-liner (Linux x86-64):**
```sh
curl -fsSL https://raw.githubusercontent.com/Univers42/hellish/main/install.sh | sh
```

**npm / pnpm / yarn:**
```sh
npm install -g hellish-shell      # or: pnpm add -g hellish-shell
```

**Docker:**
```sh
docker run --rm -it dlesieur/hellish-shell
```

**From source:**
```sh
git clone --recursive https://github.com/Univers42/hellish && cd hellish
make OPT=1 all && ./build/bin/hellish
```

After install, hellish checks for newer releases in the background and can self-update with
`update --now`. Opt out via `HELLISH_NO_UPDATE_CHECK=1`.

---

## Roadmap

**This cycle (4 weeks, UX-first):**
1. **Week 1 — Interactive UX:** syntax highlighting + autosuggestions + completion polish. → [details](interactive.md)
2. **Weeks 2–3 — Scripting:** indexed arrays (`declare -a`, `${a[@]}`, …) + crash-hardening + correct `set -e` in pipelines. → [details](scripting.md)
3. **Week 4 — Performance + release:** script-mode startup (narrow the dash gap), rigorous benchmarks, a tagged release. → [details](performance.md)

**Beyond:**
- Associative arrays (`declare -A`).
- A native line editor (replacing readline) for fully native highlighting/autosuggest/completion menus.
- `extglob`, `mapfile`/`readarray`, namerefs, `coproc`, `${v/pat/repl}`.

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
