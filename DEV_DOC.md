# hellish — Developer Documentation

How to set the environment up from scratch, build every configuration, run
every test layer, and understand where Docker and the data live. End-user
material is in [USER_DOC.md](USER_DOC.md); per-module design notes are in
`src/<module>/README.md`; the full manual is the
[docs site](https://univers42.github.io/hellish/).

## 1. From scratch

**Prerequisites**: gcc or clang, GNU make, `libreadline-dev`, git, python3
(for the pty test suites), and Docker for the container suites. No other
configuration files or secrets are needed — the project has none.

```sh
git clone --recursive https://github.com/Univers42/hellish && cd hellish
# forgot --recursive? →  git submodule update --init --recursive
make            # prints the self-documenting help page (every target described)
make all        # debug build (ASan) → build/bin/hellish
```

Two submodules are required: `vendor/libft` (stdlib + the `ft_malloc`
allocator) and `vendor/scripts` (dev tooling). A build failing on a missing
libft means the submodules are not initialised.

Install the git hooks once: `./vendor/scripts/install-hooks.sh`
(Conventional Commits are enforced at commit time).

## 2. Build — the MODE × SAFE matrix

| Command | Optimization | Allocator | Sanitizers |
|---|---|---|---|
| `make all` | `-O0 -g3` (`MODE=debug`, default) | libc (`SAFE=1`) | ASan + LSan |
| `make MODE=release` / `make OPT=1` | `-O3 -flto`, `-DNDEBUG` | `ft_malloc` (`SAFE=0`) | none |
| `make MODE=relwithdebinfo` | `-O2 -g`, `-DNDEBUG` | `ft_malloc` | none |

- An explicit `SAFE=…` always wins over the per-mode default.
- Objects live in `build/obj-$(MODE)-$(SAFE_TAG)`; modes never share objects.
  The **binary path is shared**, so mode-switching targets delete and relink
  it first.
- Flags are `-Wall -Wextra -Werror` — a warning is a build failure. Add flags
  with `EXTRA_CFLAGS=…`, never with `CFLAGS=…` (that would *replace* the set).

Runtime knobs while developing: `HELLISH_ALLOC_STATS=1` (live-bytes at exit —
the leak oracle on `SAFE=0`), `HELLISH_BANNER=0`, `HELLISH_NO_UPDATE_CHECK=1`,
`--debug=lexer --debug=parser --debug=ast`.

## 3. Test layers (a green `make test` is not enough)

| layer | run | covers |
|---|---|---|
| golden suite | `make test` (4248 cases) | `hellish -c` diffed vs **pinned bash 5.3.9** — run `make oracle` once to build it into `~/bash-5.3.9`; `make zsh-oracle` pins zsh 5.9 for the dialect tests |
| whole scripts | `tests/run_scripts.sh` | real programs + ASan scrape |
| hard corpus | `tests/hard/run.sh` | whole programs with timings |
| pty / interactive | `make pty-test` | every `tests/*.py`, discovered by glob — a new test file is covered the moment it exists |
| allocator parity | `cd tests && ./verify_alloc.sh` | both heaps byte-identical, zero leaks |
| plugin corpus | `make plugin-corpus` | 13 real third-party plugins vs release AND ASan builds |
| release build | `make test-release` | ASan and `-O3` disagree; run after touching memory |
| login shell (docker) | `make my-shell-test` | a real `make my_shell` + the updater |
| ssh login shell (docker) | `make ssh-shell-test` | sshd + chsh: ssh-cmd, scp, sftp, rsync, git vs bash |
| installer (docker) | `make installer-test` | `install.sh` end-to-end: sudo and no-sudo users, plugin selection, uninstall |

Gotchas that bite: the golden tester `chmod 000`s
`tests/test_files/invalid_permission` (restore with `chmod 755` before git
operations); new golden category files must be added to `test_lists` in
`tests/tester` or they silently never run; leak checking is ASan on `SAFE=1`
and `HELLISH_ALLOC_STATS=1` on `SAFE=0`.

## 4. Docker — what each container proves

| file / target | proves |
|---|---|
| `docker/Dockerfile.static` → `make static` | the **published release binary**: static musl build, no shared deps — runs on Debian 11, RHEL 8, stock ubuntu:24.04 alike |
| `docker-compose.yml` → `make docker-test` | source builds on 11 distro rungs (glibc + musl × gcc + clang) |
| `docker/Dockerfile.my-shell` → `make my-shell-test` | a real `make my_shell` as a non-root sudoer, then the update button — needs root-owned `/usr/bin`, `chsh`, `/etc/shells`: machine state a test must never touch on a developer's box |
| `docker/Dockerfile.sshd` → `make ssh-shell-test` | hellish as sshd's login shell: ssh-cmd, scp, sftp, rsync, git-over-ssh diffed against bash |
| `docker/Dockerfile.installer` → `make installer-test` | `curl \| sh` install for both privilege worlds + plugin selection, against a **local fake release server** |
| `docker/Dockerfile.agnostic` → `make agnostic-bench` | the 9-shell race: all competitors built into one image so every shell runs on identical ground |

### The four comparisons, as this project actually lives them

**Virtual machines vs Docker.** Every need above is process- and
filesystem-isolation with a throwaway root: a container delivers that in
seconds from a cached layer, where a VM buys a whole kernel we do not need at
the price of minutes and gigabytes. The one place a VM would be honest and a
container cannot be — macOS and WSL — is exactly where we use the `Platforms`
CI runners instead of Docker.

**Secrets vs environment variables.** The shell itself manages no
credentials, so nothing secret exists at runtime; `HELLISH_*` environment
variables are behaviour knobs, never secrets. The project's real secrets —
the npm publish token, GHCR credentials — live in GitHub Actions **secrets**,
injected only into the release workflow, never in the tree, an image layer,
or a Dockerfile `ENV` (which would bake them into image history).

**Docker network vs host network.** Test containers stay on the default
isolated bridge and talk to **fake servers inside the container** (the update
and installer suites run a local release server), so they prove the mechanism
offline instead of testing GitHub's uptime — and nothing can accidentally
reach the developer's host services. Host networking is used nowhere; the
sshd suite maps one port explicitly, which documents its surface.

**Volumes vs bind mounts.** Test containers use **neither**: their state must
die with them — an installer test that persisted `/etc/shells` edits between
runs would be testing its own residue. Bind mounts appear only as developer
conveniences (sharing a build cache or the source tree into a distro rung),
where seeing the host's files *is* the point. Named volumes would add
persistence we explicitly do not want anywhere.

## 5. Where data lives and how it persists

| location | contents | lifetime |
|---|---|---|
| `build/obj-<mode>-<alloc>/`, `build/bin/` | objects, the binary | until `make fclean`; modes coexist |
| `vendor/libft/build-{libc,ft}/` | per-allocator libft trees | rebuilt on submodule bump |
| `bench/suites/` (gitignored, ~100 MB) | fetched conformance suites | `make conformance` re-fetches |
| `bench/baseline/`, `bench/charts/` | committed pass-counts and SVGs | updated deliberately (`UPDATE_BASELINE=1`, `make charts`) |
| `~/bash-5.3.9`, `~/zsh-5.9` | the pinned oracles | built once by `make oracle` / `make zsh-oracle` |
| `~/.cache/hellish-plugin-corpus` | downloaded corpus plugins | cache; corpus skips cleanly offline |
| user side | see the table in [USER_DOC.md](USER_DOC.md) | survives reinstalls by design (never-clobber) |

## 6. Workflow

Branch from `develop`; PRs target `develop` (`main`/`develop` are protected).
Conventional Commits (`type(scope): imperative subject`), enforced by the
hook. Every fix ships with a test — non-negotiable. Pre-PR gates, all green
from a clean tree: `make re` + `make re OPT=1` build clean, `make test`,
`tests/run_scripts.sh`, `tests/hard/run.sh`, `make pty-test`,
`cd tests && ./verify_alloc.sh`, `make norm`. CI runs 11 required jobs;
releases ship via tags (`release.yml`: static binary + npm + images).

See [CONTRIBUTING.md](CONTRIBUTING.md) and [CLAUDE.md](CLAUDE.md) for the
house rules (42 Norm, allocator discipline, the test model).
