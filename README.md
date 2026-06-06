# hellish 🐚🔥

> A from-scratch, almost-POSIX shell written in C — fast, hackable, and pleasant
> to use every day. Built as a 42 project by **dlesieur** and **alcacere**, but
> grown well past the school subject.

`hellish` reads like a real shell (pipelines, redirects, here-documents,
subshells, process substitution, job control, functions, arithmetic, globbing,
parameter expansion) and is engineered like a teaching lab: input → lexer →
parser → word reparser → heredoc → expander → executor, each a small, readable
module. It ships with **two allocators you can swap at compile time** (libc
`malloc` or our own `ft_malloc`) so you can A/B their behaviour and speed.

- **Latest:** v2.3.0 — compile-time `SAFE` allocator, `${var/pat/rep}`
  substitution, a leak-clean run across the whole test corpus

---

## Table of contents

- [Quick start](#quick-start)
- [Install](#install)
- [Build & the SAFE / OPT matrix](#build--the-safe--opt-matrix)
- [What it can do](#what-it-can-do)
- [The two allocators (SAFE)](#the-two-allocators-safe)
- [Run it your way](#run-it-your-way)
- [Make it your login shell](#make-it-your-login-shell)
- [Architecture in one breath](#architecture-in-one-breath)
- [Testing & quality gates](#testing--quality-gates)
- [Contributing](#contributing)
- [License](#license)

---

## Quick start

```sh
git clone --recursive https://github.com/Univers42/hellish && cd hellish
make OPT=1            # optimized build (the one you'll want day to day)
./build/bin/hellish   # drop into the shell
```

`--recursive` matters: `hellish` pulls in two git submodules,
[`vendor/libft`](vendor/libft) (the standard-lib + the `ft_malloc` allocator)
and `vendor/scripts` (dev tooling). If you forgot it:

```sh
git submodule update --init --recursive
```

---

## Install

All three prebuilt paths download the same `hellish-linux-x86_64` artifact that
the release CI builds and attaches to each GitHub Release, so they only work
**once a release is published** (Linux x86-64). The from-source path always
works.

**From source (recommended, always works):**

```sh
git clone --recursive https://github.com/Univers42/hellish && cd hellish
make OPT=1 all && ./build/bin/hellish
```

**Prebuilt binary (curl one-liner):** fetches the latest release binary into
`$PREFIX` (default `/usr/local/bin`, falling back to `~/.local/bin`).

```sh
curl -fsSL https://raw.githubusercontent.com/Univers42/hellish/main/install.sh | sh
```

**npm / pnpm / yarn:** the package is `hellish-shell`; its `postinstall`
downloads the matching release binary. This works once the package is published
to the npm registry (the release workflow publishes it when the maintainer's
`NPM_TOKEN` secret is set).

```sh
npm install -g hellish-shell      # or: pnpm add -g hellish-shell
```

**Docker:** build the image locally from the repo's [`Dockerfile`](Dockerfile):

```sh
docker build -t hellish . && docker run --rm -it hellish
```

Once installed, `hellish` checks for newer releases in the background (once a
day, never blocking the prompt) and flags one in the welcome banner. Run
`update` to check on demand, or `update --now` to self-update the binary. Opt
out with `HELLISH_NO_UPDATE_CHECK=1` (and `HELLISH_NO_BANNER=1`).

---

## Build & the SAFE / OPT matrix

Everything goes through the root `Makefile`. Two independent knobs shape the
build: **`OPT`** (optimization) and **`SAFE`** (which allocator). They combine
freely, and the build prints which allocator it picked so it's never a surprise.

| Command | Optimization | Allocator | Sanitizers | Use it for |
|---|---|---|---|---|
| `make` | `-O0 -g3` | libc (`SAFE=1`) | ASan + LeakSanitizer | day-to-day dev, debugging, leak hunts |
| `make OPT=1` | `-O3 -flto` | **`ft_malloc`** (`SAFE=0`) | none | speed, benchmarks, daily driving |
| `make SAFE=0` | `-O0 -g3` | `ft_malloc` | ASan | exercising the custom heap under a debugger |
| `make OPT=1 SAFE=1` | `-O3 -flto` | libc | none | optimized build on the battle-tested heap |

So the **default per mode** is: debug → `SAFE=1` (libc, so ASan stays
meaningful), optimized → `SAFE=0` (our `ft_malloc`). An explicit `SAFE=…` on the
command line always wins.

Common targets (all repeatable, all idempotent):

```sh
make            # debug build  -> build/bin/hellish
make OPT=1      # optimized build
make re         # fclean + rebuild
make clean      # remove object files
make fclean     # remove objects, binary, and libft build trees
make test       # run the full test suite (diffs hellish vs bash --posix)
make bench      # benchmark hellish vs bash --posix (geomean + per-task)
make norm       # run norminette over src/ incs/ tests/
make my_shell   # install as your login shell (rebuilds OPT=1 SAFE=1 first)
```

libft is compiled into a **per-`SAFE` tree** (`vendor/libft/build-libc` vs
`build-ft`) so the two allocators never share object files — flip `SAFE` and you
get the right archive, not a stale one.

---

## What it can do

**Interactive**
- Line editing with **vi** and **emacs** modes (readline-backed).
- Persistent, de-duplicated command history in `$HOME`, with safe escaping for
  multi-line commands; `history`, `fc`, and `!`-style history expansion.
- Tab completion for commands, files, and `$variables`.
- Rich, multibyte- and ANSI-aware prompt (user, cwd, git branch, venv, time)
  that never drifts the cursor.
- A `~/.hellishrc` startup file (the `.bashrc` analogue) sourced only in
  interactive sessions.
- Job control: `&`, `jobs`, `fg`, `bg`, `wait`, `kill`, `$!`.

**Scripting / POSIX**
- Pipelines, lists (`;`, `&&`, `||`, `&`), subshells `( … )`, brace groups.
- Control flow: `if/elif/else`, `for`, `while`, `until`, `case/esac`, and
  shell **functions** (with `local`, `return`, recursion).
- Redirections: `>`, `>>`, `<`, `>|`, `<>`, `n>&m`, here-documents `<<` / `<<-`,
  and **process substitution** `<( … )` / `>( … )`.
- The full expansion pipeline in classic order: brace expansion, tilde `~`,
  parameter expansion (`${v:-d}`, `${v:=d}`, `${v:?}`, `${v:+a}`, `${#v}`,
  `${v#p}`/`${v##p}`/`${v%p}`/`${v%%p}`, and **`${v/p/r}` / `${v//p/r}`**
  substitution), command substitution `$( … )` and `` `…` ``, arithmetic
  `$(( … ))`, word splitting on `IFS`, and pathname globbing (`*`, `?`,
  `[…]`, POSIX classes).
- Positional parameters `$1 … $@ $* $#`, `shift`, `getopts`, `set` / `set -o`
  options (`-e`, `-u`, `-x`, `-f`, `-C`, `-a`, `-n`, …), `$?`, `$$`, `$-`.
- `trap` (including `EXIT` and signal traps), `[[ … ]]`, arithmetic `let`.

**Builtins** (47): `echo export cd pushd popd dirs [[ exit pwd env unset type set
shift : break continue eval . source true false umask command return getopts
exec wait times trap readonly read test [ alias unalias hash jobs fg bg fc
history let local kill printf ulimit update`.

---

## The two allocators (SAFE)

Every allocation in the shell goes through one macro family —
`xmalloc` / `xcalloc` / `xfree` — that resolves **at compile time** to either
libc or our own allocator:

- **`SAFE=1`** → libc `malloc`/`free`. AddressSanitizer and LeakSanitizer
  instrument it, so this is where leak/heap checking is *meaningful*.
- **`SAFE=0`** → **`ft_malloc`**, the custom slab/arena allocator living in
  `vendor/libft`. Faster, and a fun thing to study — but ASan is blind to it, so
  for leak checking on this side use its own oracle:

```sh
make OPT=1                                   # SAFE=0 build
HELLISH_ALLOC_STATS=1 ./build/bin/hellish script.sh   # prints live bytes at exit
```

The two heaps do **not** share memory, so the shell is careful never to free a
pointer on the wrong one. Both backends pass the entire test suite identically —
the swap is transparent to behaviour, only the performance and the debugging
tools differ.

---

## Run it your way

```sh
./build/bin/hellish                 # interactive
./build/bin/hellish script.sh       # run a script file
./build/bin/hellish -c 'echo hi'    # run a command string
echo 'echo piped' | ./build/bin/hellish   # read from a pipe (non-TTY)
```

Debug views (compose them freely):

```sh
./build/bin/hellish --debug=lexer --debug=parser --debug=ast script.sh
```

---

## Make it your login shell

> ⚠️ Only do this if you understand the risk — a broken `$SHELL` makes life
> painful. Keeping it as an *alternative* shell you launch explicitly is safer.

```sh
make my_shell                       # rebuilds OPT=1 SAFE=1, installs, registers
make my_shell BAPTIZE_SHELL=myname  # install under a custom name
```

`my_shell` deliberately rebuilds **`OPT=1 SAFE=1`** (optimized, on the
battle-tested libc heap) before installing — the shell you live in should be the
safe, fast one. Pass `SAFE=0` if you really want the custom heap; then stability
is on you.

---

## Architecture in one breath

```
input → lexer → parser (AST) → word reparser → heredoc → expander → executor
```

Each stage is its own module under `src/` with its own README, all orbiting one
struct — `t_shell` in [`incs/shell.h`](incs/shell.h), the single source of truth
for a running shell. The codebase is heavily and *humanly* commented: read any
`.c` top-to-bottom and the comments explain the *why*, the trick, and the gotcha,
not just the *what*. See [`CLAUDE.md`](CLAUDE.md) for an architectural map.

---

## Testing & quality gates

```sh
make test                       # the whole suite, hellish vs bash --posix
cd tests && ./tester redir pipe # run specific category files
cd tests && ./verify_alloc.sh   # build BOTH allocators, prove parity + no leaks
make bench                      # speed vs bash --posix (geomean verdict)
make norm                       # 42 norminette
```

The test model is a **golden diff against `bash --posix`**: ~2500 cases compare
stdout, exit status, and any files written, on both allocator backends. The
debug build runs under AddressSanitizer + LeakSanitizer; `verify_alloc.sh`
additionally gates output-parity and leak-cleanliness across `SAFE=1` and
`SAFE=0`.

---

## Contributing

Pull requests welcome — please read [`CONTRIBUTING.md`](CONTRIBUTING.md) first.
In short: **fork, branch, keep the commit format, add a test for every bug you
fix, and make sure `make norm` / the suite / ASan are all green before you open
the PR.** Bugs that live in `vendor/libft` or `ft_malloc` are fixed in *those*
submodule repositories, not here.

Security issues: see [`SECURITY.md`](SECURITY.md). Be excellent to each other:
[`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md).

---

## License

[MIT](LICENSE) © dlesieur, alcacere. An educational project built on the POSIX
shell grammar, *Crafting Interpreters*-style lexing/parsing, and a lot of
late-night debugging. Welcome to `hellish`. 🐚
