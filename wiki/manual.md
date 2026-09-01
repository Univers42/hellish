# The manual

> `man bash`, but `man hellish`: one page that answers "how do I invoke it,
> what does it read at startup, what is the grammar, where does everything
> live". The per-name detail lives in the generated
> [builtins reference](builtins/index.md), which is the `help` builtin in
> page form.

## NAME

**hellish** — a from-scratch, almost-POSIX shell with bash compatibility,
an opt-in zsh dialect, and a line editor built on GNU readline.

## SYNOPSIS

```
hellish [options] [script [arguments...]]
hellish -c 'command'
some-producer | hellish
```

## DESCRIPTION

hellish reads commands from a terminal, a script file, a `-c` string, or a
pipe, and executes them with bash's semantics — the test suite diffs it
byte-for-byte against a pinned `bash --posix` on 4248 golden cases, so where
the two disagree it is a bug here, not an opinion. Interactive sessions get
readline editing (vi and emacs), persistent history, programmable completion,
prompt themes, and job control.

## INVOCATION OPTIONS

| option | effect |
|---|---|
| `-c <string>` | execute the string, then exit |
| `--login` | act as a login shell: source `/etc/profile`, then `~/.profile` |
| `--norc` | interactive, but skip every startup file |
| `--rcfile=FILE` | source FILE instead of the usual rc chain |
| `--posix` | disable the non-POSIX extensions |
| `--version` | print the version, the release asset name and the repo the updater points at, then exit |
| `--help` | usage, then exit |
| `--verbose` | verbose mode |
| `--debug=lexer` `--debug=parser` `--debug=ast` | print a stage's view of each input line (composable) |

With a `script` operand the file is run non-interactively;
`arguments` become `$1 $2 …`.

## STARTUP FILES

Only **interactive** shells read startup files — never scripts, `-c`, or
piped input, so tests and cron jobs stay clean. The chain, in order:

1. `/etc/hellish/rc.d/*.hsh` — machine-wide drop-ins
2. `$XDG_CONFIG_HOME/hellish/rc.d/*.hsh` — your drop-ins, lexical order
3. `$XDG_CONFIG_HOME/hellish/plugins/*/plugin.hsh` — plugins ([how to install](plugins.md))
4. `~/.hellishrc` — last, so it can override everything

A login shell (`--login`, or installed via `make my_shell`) first sources
`/etc/profile` and `~/.profile` the way bash does — hellish advertises
`BASH_VERSION`, so a distribution's stock dotfile chain works unmodified.
Sourcing a file ending in `.zsh` arms the [zsh dialect](#the-zsh-dialect)
for that file.

## GRAMMAR

Everything bash accepts in the common core: pipelines (`|`, `|&`), lists
(`;`, `&&`, `||`, `&`), subshells `( )`, brace groups `{ }`,
`if/elif/else/fi`, `for`, `for ((;;))`, `while`, `until`, `case` (with `;;`,
`;;&`, `;&`), functions (`name()` and `function name`), `[[ … ]]` with
pattern and `=~` regex matching, `(( ))`, coprocesses, and `select`-free
honesty about what is not there. `help <keyword>` explains each form from
inside the shell; the [builtins reference](builtins/index.md#syntax) lists
them all.

## EXPANSION

Performed in bash's order: brace expansion; tilde; parameter and variable
expansion (`${v:-d}`, `${v#p}`/`${v%p}`, `${v/pat/rep}`, `${v@Q}`, case
mods, substrings, `${!prefix*}`, arrays `${a[@]}` and friends); command
substitution `$( )` / backticks (with a forkless fast path for provably
side-effect-free bodies); arithmetic `$(( ))`; word splitting on `IFS`;
pathname expansion (`*`, `?`, `[…]`, POSIX classes, `**` with `globstar`,
extended patterns with `extglob`, case-free with `nocaseglob`, plus
`nullglob`/`dotglob`); quote removal. Process substitution `<( )` `>( )`
works wherever bash allows it.

## REDIRECTION

`<`, `>`, `>>`, `<<` and `<<-` heredocs, `<<<` herestrings, `2>&1`-style fd
duplication, `&>file`, fd-numbered forms, and bash's `/dev/tcp/host/port`
and `/dev/udp/host/port` network files.

## THE PROMPT

Unconfigured, the prompt is zsh's own default — `hostname% ` — plus a
self-spacing `⬆` badge when a release is pending. `prompt` lists 29 themes;
`PS1` accepts **both** escape languages at once (bash `\u \w \$` and zsh
`%n %~ %#`, literal percents preserved), and `PROMPT` keeps exact zsh
semantics. Two hook arrays, `HELLISH_PRECMD_FUNCS` and
`HELLISH_PREEXEC_FUNCS`, run around every interactive command. Details:
[Interactive Experience](interactive.md).

## HISTORY

Persistent, de-duplicating, multi-line-safe. A compound command is one
entry, joined bash-`cmdhist`-style on recall (`shopt -s lithist` keeps the
newlines). `history`, `fc`, and `!`-expansion work; reverse-search is the
readline binding you expect.

## COMPLETION

TAB completes commands on `$PATH` + builtins in command position, variable
names after `$`, and filenames elsewhere. With `shopt -s progcomp`,
`complete`/`compgen` specs are consulted exactly as bash defines them
(`COMP_WORDS`, `COMP_CWORD`, `COMP_LINE`, `COMP_POINT`, `COMPREPLY`) —
git's own `git-completion.bash` works. The default stays off where bash has
it on, for a measured reason recorded in the
[interactive page](interactive.md#completion-).

## JOB CONTROL

`&`, `jobs`, `fg`, `bg`, `wait`, `kill` with job specs, `$!`, Ctrl-Z, and
bash's exit protocol for stopped jobs (first exit warns, second obeys — and
the terminal is restored even when a foreground job is killed).

## THE ZSH DIALECT

None of zsh's grammar is reachable until something arms the mode —
`set -o zsh`, `emulate zsh`, or sourcing a `.zsh` file (restored when the
file ends). Armed, real oh-my-zsh plugins load: parameter-expansion flags
(`${(f)x}` …), modifiers (`:h :t :r`), `setopt`, `print`, `autoload`,
1-based arrays with slices and splices, glob qualifiers, anonymous
functions, `always` blocks, and the `zle`/`bindkey` widget layer. The
[plugin corpus](https://github.com/Univers42/hellish/blob/main/tests/plugin_corpus_test.py)
is the acceptance test. See [Architecture](architecture.md#the-zsh-dialect).

## SHELL BUILTINS

69 names, documented one by one in the
**[builtins reference](builtins/index.md)** — generated from the shell's own
`help` system, which is test-enforced against the dispatch table. From
inside the shell: `help` for the grouped list, `help NAME` for one,
`type NAME` to see how any name resolves.

## ENVIRONMENT

| variable | effect |
|---|---|
| `HELLISH_BANNER=0\|1` | force the welcome panel off / on |
| `HELLISH_NO_UPDATE_CHECK=1` | never check for releases; no badge |
| `HELLISH_NO_ANIM=1` | skip the startup animation |
| `HELLISH_ANIM=spinner\|pulse\|ember` | opt into a prompt animation |
| `HELLISH_ALLOC_STATS=1` | print live heap bytes at exit (`SAFE=0` builds) |
| `HELLISH_NO_EXEC=1` | user-install hook: stay in the login shell this once |
| `HELLISH_PRECMD_FUNCS` / `HELLISH_PREEXEC_FUNCS` | hook arrays around every interactive command |

Plus the classics it honours: `PATH`, `HOME`, `PS1`/`PS2`/`PROMPT`, `IFS`,
`CDPATH`, `EDITOR`, `HISTSIZE`, `SHLVL`, `OLDPWD`.

## FILES

| path | role |
|---|---|
| `~/.hellishrc` | interactive startup, sourced last, never overwritten by installers |
| `$XDG_CONFIG_HOME/hellish/{rc.d,themes,plugins}/` | drop-ins, the 29 themes, plugins |
| `~/.hellish/` | the [plugin framework](plugins.md), if installed |
| `~/.hellish_history` | history, multi-line-safe |
| `~/.cache/hellish/` | update-check state |
| `~/.hellish-disable` | its existence disables the user-install exec hook |

## EXIT STATUS

The last command's, as bash: `127` command not found (after
`command_not_found_handle`, if defined — its status wins), `126` found but
not executable, `128+n` fatal signal *n*, `2` builtin usage errors.

## SEE ALSO

[Builtins reference](builtins/index.md) ·
[Plugins](plugins.md) ·
[Interactive Experience](interactive.md) ·
[Scripting & Compatibility](scripting.md) ·
[Architecture](architecture.md) ·
`bash(1)`, `zsh(1)`, `readline(3)` — the shells this one is measured
against, byte for byte where it counts.
