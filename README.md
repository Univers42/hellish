*This project has been created as part of the 42 curriculum by dlesieur, alcacere.*

# hellish 🐚🔥

![shell](https://img.shields.io/badge/shell-POSIX%20%2B%20bashisms-e2543a)
![golden suite](https://img.shields.io/badge/golden%20suite-4248%20vs%20bash%20--posix-6f9e78)
![license](https://img.shields.io/badge/license-MIT-7f8fa0)

## Description

`hellish` is a from-scratch, almost-POSIX shell written in C. It started as
42's shell project and grew well past the subject: it now diffs
byte-for-byte against a pinned `bash --posix` on 4248 golden cases, races
**#3 of 9** against real shells (faster than bash, zsh and fish), loads real
oh-my-zsh plugins through an opt-in zsh dialect, and ships with programmable
completion, 29 prompt themes, a plugin framework and a self-updater.

The goal: prove that one codebase can pair **minimalist-shell speed with
bash-class features** — and stay readable, 42-norm-clean, and leak-free on
two different allocators while doing it.

**Full documentation: <https://univers42.github.io/hellish/>**

## Instructions

**Install in one command** (Linux x86-64 — detects your rights: with sudo it
installs system-wide and registers your login shell; without sudo — a 42
machine — it installs to `~/.local/bin` with an rc hook; then it lets you
pick plugins):

```sh
curl -fsSL https://raw.githubusercontent.com/Univers42/hellish/main/install.sh | sh
```

**From a source checkout:**

```sh
git clone --recursive https://github.com/Univers42/hellish && cd hellish
make OPT=1 all          # optimized build → build/bin/hellish
./build/bin/hellish     # run it
make user-install       # no sudo: ~/.local/bin + rc hook (42 machines)
make my_shell           # sudo: /usr/bin + login shell registration
```

`--recursive` matters (two submodules); if forgotten:
`git submodule update --init --recursive`. `make` alone prints a
self-documenting help page; `make test` runs the golden suite;
`make doctor` diagnoses an installation. To undo an install:
`make user-uninstall` or `make my-shell-uninstall`.

More: **[USER_DOC.md](USER_DOC.md)** (use it) ·
**[DEV_DOC.md](DEV_DOC.md)** (build, test and hack on it).

## Features

- Everything POSIX plus the bashisms you use: arrays, `[[ =~ ]]`,
  `${v/pat/repl}`, process substitution, heredocs, job control, `set -o`
  options, traps, functions — diffed against bash, not guessed.
- Interactive comfort: readline editing (vi/emacs), persistent multi-line
  history, tab completion with `complete`/`compgen` specs, 29 prompt themes
  and both prompt languages (bash `\u\w\$` *and* zsh `%n %~ %#`).
- An opt-in **zsh dialect** that loads real third-party plugins — proven by
  a 13-plugin corpus (oh-my-zsh, git's own scripts, z, bash-preexec) run in CI.
- Two compile-time-swappable allocators, ASan-clean, `-Wall -Wextra -Werror`,
  42-norm-clean.
- Background update channel: `update --now` self-updates from GitHub releases.

## Technical choices

Docker is load-bearing here, not decoration: the **published release binary
is built statically (musl) inside a container** so it runs on any distro; a
**14-rung distro matrix** (glibc/musl × gcc/clang) builds from source in CI;
the **installer, login-shell and ssh suites** run in containers because they
must edit `/etc/shells`, `chsh`, and a root-owned `/usr/bin` no developer
machine should lend them; and the **9-shell benchmark** builds all
competitors into one image so every shell races on identical ground.

| choice | what we use | why |
|---|---|---|
| VMs vs **Docker** | Docker | test needs are per-process isolation + a throwaway filesystem, not a kernel; a container gives both in seconds where a VM costs minutes and GBs |
| Secrets vs **env variables** | repo secrets in CI, env knobs at runtime | the shell itself holds no credentials; CI tokens (npm, GHCR) live in GitHub secrets, never in the tree — env vars configure behaviour (`HELLISH_*`), they never carry secrets |
| Docker vs **host network** | isolated bridge + local fake servers | update/installer tests talk to a fake release server inside the container, so they prove the mechanism offline instead of testing GitHub's uptime |
| Volumes vs **bind mounts** | neither in tests, bind mounts for dev caches | test containers are hermetic by design (state must die with them); only developer conveniences (build caches) bind-mount |

Full discussion: **[DEV_DOC.md](DEV_DOC.md)**.

## Resources

- [POSIX.1-2018 Shell & Utilities (XCU)](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html) — the specification
- [Bash Reference Manual](https://www.gnu.org/software/bash/manual/) — the behavioural oracle (pinned at 5.3.9 by `make oracle`)
- [zsh documentation](https://zsh.sourceforge.io/Doc/) — the dialect's oracle (pinned at 5.9)
- [GNU Readline](https://tiswww.case.edu/php/chet/readline/rltop.html) — the line editor underneath
- [Oils spec tests](https://github.com/oils-for-unix/oils) and [mksh check.t](https://github.com/MirBSD/mksh) — third-party conformance suites we run
- [oh-my-zsh](https://github.com/ohmyzsh/ohmyzsh) — source of the real-world plugin corpus

**AI use.** AI assistance (Claude Code) was used in a deliberately
fine-grained, controlled way — and learning to *drive* an AI on a real
system-programming project was itself one of this project's goals. The
authors designed the control system first and made the assistant work
inside it: a golden suite that diffs every behaviour against a **pinned
bash 5.3.9 oracle** (and zsh 5.9 for the dialect), TDD with a
test-per-fix rule enforced at review, a plugin corpus where third-party
code declares expectations that go red when reality moves, allocator-parity
and sanitizer gates, and a CI wall of 11 required jobs that no change —
human or AI — merges without passing. Those guardrails are what shrink the
assistant's scope for mistakes to near zero and make its output
*ownable*: every AI-assisted change was specified, measured against the
oracles, reviewed and accepted by the authors, who own the architecture,
the design decisions and the 42-norm C throughout. The strategy — reduce
the blast radius, measure everything, never trust a claim without a
command and its output — is documented across [DEV_DOC.md](DEV_DOC.md)
and [CONTRIBUTING.md](CONTRIBUTING.md), and is, we think, the honest way
to build with an AI and still sign your own work.

## Documentation

**[Docs site](https://univers42.github.io/hellish/)** ·
[USER_DOC.md](USER_DOC.md) · [DEV_DOC.md](DEV_DOC.md) ·
[RELEASE.md](RELEASE.md) · [CONTRIBUTING.md](CONTRIBUTING.md) ·
[LICENSE](LICENSE) (MIT)

