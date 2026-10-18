# What's new in 3.2

hellish 3.2 is about the prompt. The up arrow finds what you started typing.
A dim hint from your history can complete the line, and you take it with one
key. `make <TAB>` knows your targets, and `bind` puts a shell command on a
key. This page shows how to use each one. Every example on it was run on a
3.2 build.

!!! tip "Get it"
    Already running hellish? Type `update --now`. Otherwise install it:

    ```sh
    curl -fsSL https://raw.githubusercontent.com/Univers42/hellish/main/install.sh | sh
    ```

## At a glance

This is the table from the welcome banner, explained.

| Keys | What it does | Needs |
|---|---|---|
| `↑ ↓` | [history by prefix](#history-that-matches-what-you-typed) | on by default |
| `make <TAB>` | [Makefile targets](#completing-make-targets) | |
| `bind -x` | [key runs a command](#key-bindings-with-bind) | |
| `bindkey` | [zsh binds now work](#bindkey-in-zsh-configs) | zsh mode |
| `${var@P}` | [render as a prompt](#rendering-text-like-the-prompt) | |
| `$SRANDOM` | [and `$EPOCHREALTIME`](#two-new-variables) | |
| `→ ^F End` | [take the dim hint](#inline-hints) | `HELLISH_AUTOSUGGEST=1` |
| `Alt-F` | [take one word](#inline-hints) | `HELLISH_AUTOSUGGEST=1` |
| `Alt-N/P` | [older/newer hint](#inline-hints) | `HELLISH_AUTOSUGGEST=1` |

## History that matches what you typed

Type the start of a command, then press <kbd>↑</kbd>. Up now steps only
through history lines that begin with the text before the cursor, and
<kbd>↓</kbd> steps back toward the newest one:

```text
$ git<Up>          ->  git push      the newest line starting with "git"
$ git<Up><Up>      ->  git status    the one before it
```

On an empty line, Up and Down walk the whole history as before.

- It is on by default. `HELLISH_HISTORY_PREFIX=0` (or `off`) brings back
  plain Up/Down. The variable is read at every prompt, so you can change it
  in the middle of a session.
- Configs that already asked for this now get it. That covers bash's
  `bind '"\e[A": history-search-backward'`, zsh's
  `bindkey '^[[A' history-beginning-search-backward`, and oh-my-zsh's
  `up-line-or-beginning-search`. Before 3.2, each of them left the arrow
  dead.
- A key you bound yourself keeps your binding.

## Inline hints

Hints are off until you ask for them. Add this line to `~/.hellishrc`, or
type it at the prompt to try it:

```sh
export HELLISH_AUTOSUGGEST=1
```

From then on, when you type the start of a line you ran before, the rest of
it appears dimmed after the cursor. The hint is the newest history line that
starts with what you typed. It is not part of your line until you take it:

```text
$ echo hello          ran earlier
$ ech|o hello         you typed "ech", the cursor is at |, "o hello" is dim
```

| Key | What it does |
|---|---|
| <kbd>→</kbd>, <kbd>End</kbd> or <kbd>Ctrl-F</kbd> | take the whole hint |
| <kbd>Alt-F</kbd> or <kbd>Ctrl-→</kbd> | take its next word |
| <kbd>Alt-N</kbd> / <kbd>Alt-P</kbd> | show an older / a newer match instead |
| <kbd>Enter</kbd> | run only what you typed |

- With no hint on screen, these keys keep their usual job: Right moves the
  cursor, Alt-F moves a word, and Alt-N / Alt-P are readline's history
  search.
- A *word* is the hint's leading blanks plus the text up to the next space
  or tab.
- Cycling skips duplicates. It stops at either end with a bell instead of
  wrapping around. Any edit starts again from the newest match.
- Hints also work in vi insert mode.
- A history line that holds a newline or a control character is never
  offered. A terminal that cannot move the cursor (`TERM` unset or `dumb`)
  never shows a hint.
- `HELLISH_SUGGEST_MAX=N` sets how many of the newest history lines are
  searched (default 1000).
- In zsh mode, widgets see the hint as `POSTDISPLAY`, and can set their own.

## Completing make targets

<kbd>Tab</kbd> after `make` offers the targets of the Makefile in the
current directory. With this Makefile:

```make
.PHONY: all clean fclean re
NAME = demo
all: $(NAME)
$(NAME): main.o
	cc -o $@ $^
%.o: %.c
	cc -c $<
clean:
	rm -f *.o
fclean: clean
	rm -f $(NAME)
re: fclean all
```

```text
$ make <TAB><TAB>
all     clean   fclean  re
$ make fc<TAB>
$ make fclean
```

The variable, the pattern rule, `$(NAME)` and `.PHONY` are not targets you
would type, so they are not offered.

- The shell reads the file itself; `make` is never run. It reads the first
  of `GNUmakefile`, `makefile` and `Makefile` (the order GNU make uses).
- It also works for `gmake` and `/usr/bin/make`, and after `sudo`,
  `env X=1`, `&&` or `(`.
- `include` lines are followed one level deep. If you change only an
  included file, the list is not refreshed: touch the Makefile.
- With no Makefile, Tab completes file names as usual.

The full rules are in the manual's
[make targets](manual.md#make-targets) section.

## Key bindings with bind

hellish now has bash's `bind` builtin, so the lines you would put in
`~/.inputrc` work in `~/.hellishrc`:

```sh
bind '"\e[A": history-search-backward'     # a key -> a readline command
bind 'set completion-ignore-case on'        # a readline setting
bind -x '"\C-t": date'                       # a key -> a shell command
```

`bind -x` runs the command the way bash does. It can read and change the
line you are editing through `READLINE_LINE` and `READLINE_POINT` (the
cursor). This key puts `sudo` in front of the line and leaves the cursor at
the end:

```sh
bind -x '"\C-xs": READLINE_LINE="sudo $READLINE_LINE"; READLINE_POINT=${#READLINE_LINE}'
```

```text
$ apt update<Ctrl-X s>
$ sudo apt update
```

To see what is bound: `bind -p` lists key bindings, `bind -v` the settings,
`bind -X` the `-x` commands and `bind -l` every command name. `help bind`
shows every option.

## bindkey in zsh configs

In zsh mode (`set -o zsh`, `emulate zsh`, or a sourced `.zsh` file),
`bindkey` now understands zsh's key notation (`^[[A`, `^R`, `^[b`, `\E`)
and reaches the line editor's own widgets. These two lines from a zsh
config now do what they say:

```zsh
bindkey '^[[A' history-beginning-search-backward
bindkey '^[[B' history-beginning-search-forward
```

## Rendering text like the prompt

`${var@P}` expands a value the way the prompt does. It uses the same
renderer as `PS1`, so the result always matches your prompt:

```text
$ USER=ada; PS1='\u in \W \$ '; cd /usr/share
$ echo "${PS1@P}"
ada in share $
```

Both escape languages work, bash's `\u \w` and zsh's `%n %~`. `prompt
preview` uses it to draw the themes.

## Two new variables

```text
$ echo $SRANDOM
2414656789
$ echo $EPOCHREALTIME
1792288722.298309
```

`$SRANDOM` is a 32-bit random number, a new one each time you read it.
`$EPOCHREALTIME` is the time in seconds since the epoch, with microseconds.
Both come from bash: `$EPOCHREALTIME` from 5.0, `$SRANDOM` from 5.1.

## Fixed

At the prompt:

- On some builds, the hint keys did nothing and Up did not search by prefix:
  the keys kept their stock behaviour. They now work on every build.
- Two small memory leaks in line editing are fixed. One came from walking
  history with Up or <kbd>Ctrl-P</kbd> and then pressing Enter or
  <kbd>Ctrl-C</kbd>; the other from cancelling an <kbd>Alt-P</kbd> search.
- In zsh mode, pressing <kbd>Esc</kbd> twice (oh-my-zsh's `sudo` plugin)
  printed `builtin: which: not a shell builtin`. `which [-ap] name` now
  exists in zsh mode.
- `PS2` is the prompt of every continuation line.
- A <kbd>Ctrl-C</kbd> at the wrong moment no longer eats the next key.

In scripts:

- `exec nosuch` no longer crashes the shell.
- A runtime error names the line it happened on, and a command that cannot
  run says why, in bash's words and with bash's exit status.
- `read` assigns its variables at end of input, and never a readonly one.
- A quoted `"$@"` element is never expanded as a glob pattern.
- `export X` before `X` is set keeps the export.
- Heredocs work inside function bodies, `eval` and `source`.

Installing and building:

- The `curl … | sh` installer prints the complete `PATH` instruction
  ([#164](https://github.com/Univers42/hellish/issues/164)).
- `make re` no longer stops on NFS when a hellish from `build/` is still
  running.

Every change since 3.1.3:
[v3.1.3...v3.2.0](https://github.com/Univers42/hellish/compare/v3.1.3...v3.2.0).
