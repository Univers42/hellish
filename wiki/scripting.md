# Bash Compatibility & Scripting

> **hellish runs real scripts** — not a toy REPL.
> Status legend: ✅ shipped · 🚧 in progress · 📋 planned

## The proof: it built a Linux distribution

The strongest compatibility claim a shell can make isn't a feature list — it's *running real-world
scripts*. hellish was used as the **build-script interpreter for a complete Linux From Scratch
build**: it drove `./configure`, autotools, gcc/glibc/binutils, and ~80 packages, producing a disk
image that **boots** (`uname -r` → `6.6.32-dlesieur`, network up, SysVinit, clean shutdown).

Most "write your own shell" projects die on the first `./configure`. hellish built an OS. That's the
bar this page documents against.

---

## Feature matrix vs. bash

| Area | Feature | Status |
|---|---|---|
| **Control flow** | `if`/`for`/`while`/`until`/`case`, functions, `&&`/`\|\|`/`;`/`!` | ✅ |
| **Pipelines/redir** | pipes, `>`/`>>`/`<`, `2>&1`, fd dup, subshells `( )`, groups `{ }` | ✅ |
| **Conditionals** | `[[ … ]]` (string/int/file/unary, `&&`/`\|\|`/`!`/`( )`, `==`), `[ ]`/`test` | ✅ |
| **Substitution** | `$(…)` / backticks, arithmetic `$(( … ))` (`++ -- += ?: ,` , bases) | ✅ |
| **Heredocs** | `<<`, `<<-`, quoted & unquoted, `${VAR}` bodies, consecutive, in-function | ✅ |
| **Process subst.** | `<(…)`, `>(…)`, `exec > >(tee …)` | ✅ |
| **Parameter exp.** | `${v:-w}` `${v:+w}` `${#v}` `${v#p}` `${v%p}`, substring, positional | ✅ |
| **Brace expansion** | `{a,b,c}`, `{1..n}`, with `$var`/quoted prefixes | ✅ |
| **Globbing** | `*` `?` `[…]`, sorted matches, no-match passthrough | ✅ |
| **Options/traps** | `set -o pipefail`, `set -e`/`-u`/`-o`, `trap` (EXIT/INT/TERM) | ✅ |
| **Job control** | `bg`/`fg`/`jobs`/`kill`, `&`, dirstack `pushd`/`popd`/`dirs` | ✅ |
| **Builtins** | `getopts`, `read`, `printf`, `alias`/`unalias`, `hash`, `type`, `local`, … | ✅ |
| **Arrays** | indexed `a=(…)`, `a[i]=v`, `${a[i]}`, `${a[@]}`, `declare -a` | 🚧 *(Wk2–3)* |
| **`set -e`** | abort on failure in a **multi-stage pipeline** (simple commands ✅) | 🚧 *(Wk3)* |
| **Assoc. arrays** | `declare -A`, string keys | 📋 |
| **Pattern/misc** | `${v/pat/repl}`, `var+=value` (string append), `extglob`, `[[ str == glob ]]` RHS, `mapfile`/namerefs/`coproc` | 📋 |

> Honesty matters more than a green wall: the 🚧/📋 rows are real gaps, tracked openly. The ✅ rows
> are each locked behind a regression test that's diffed against `bash --posix`.

---

## Arrays — the headline of this cycle 🚧

Arrays are the single biggest compatibility unlock (a huge fraction of real bash scripts use them),
and they're the current focus.

```sh
# Target syntax (Weeks 2–3)
declare -a fruits=(apple banana cherry)
fruits[3]=date
echo "${fruits[@]}"      # apple banana cherry date
echo "${#fruits[@]}"     # 4
echo "${!fruits[@]}"     # 0 1 2 3
fruits+=(elderberry)     # append
for f in "${fruits[@]}"; do echo "$f"; done   # one field per element, even quoted
```

**Why it's a real change, not a bolt-on:** hellish's variable store is currently one string per name.
Arrays add a value-vector + type tag to that store, subscript parsing on the assignment side, and
`@`/`*` handling (with the subtle "`"${a[@]}"` splits to one field per element even when quoted"
rule) on the expansion side — plus a `declare`/`typeset` builtin. The good news: bash arrays are
shell-local (never exported to the environment), so the `execve` path is untouched.

---

## How compatibility is guaranteed

Every fixed construct gets a permanent regression case, and a **conformance gate** diffs each
construct under hellish vs. `bash --posix`:

```sh
make -C vendor/42sh test          # the full suite (2481+ cases, growing)
HELLISH=… bash scripts/conformance.sh   # construct-by-construct vs bash → 0 divergences
```

If a behavior diverges from bash, it shows up here *before* it ships. That's the contract behind
every ✅ above.

---

See also: **[Interactive Experience](interactive.md)** · **[Performance & Robustness](performance.md)** · **[What hellish is + Install](product.md)**
