# hellish — User Documentation

How to install it, use it, configure it, and check it is healthy. For
building and hacking on it, see [DEV_DOC.md](DEV_DOC.md); for the full
manual, see the [docs site](https://univers42.github.io/hellish/).

## What you get

| service | what it is |
|---|---|
| the shell | an almost-POSIX shell that runs your bash scripts and muscle memory — `hellish`, `hellish script.sh`, `hellish -c 'cmd'` |
| prompt themes | 29 named themes; `prompt` lists them, `prompt <name>` switches, `prompt save <name>` persists |
| plugin framework | `~/.hellish/` with a `conf` manager — `conf list`, `conf on\|off <name>`, `hxp list`; plus oh-my-zsh plugin support through the zsh dialect |
| completion | tab completion for commands/files/variables; `complete`/`compgen` specs behind `shopt -s progcomp` |
| update channel | a daily background check; `update` on demand, `update --now` to self-update |

## Install

**One command** (detects sudo rights, asks about plugins):

```sh
curl -fsSL https://raw.githubusercontent.com/Univers42/hellish/main/install.sh | sh
```

- **With sudo** → installs to `/usr/bin/hellish`, adds it to `/etc/shells`,
  and offers to make it your login shell (`chsh`). Same as `make my_shell`.
- **Without sudo** (42 school machines, shared boxes) → installs to
  `~/.local/bin/hellish`, seeds `~/.hellishrc`, and adds a small hook to your
  login shell's rc that `exec`s hellish for interactive sessions — a real
  login shell, not an alias. Same as `make user-install`.
- Non-interactive: `sh install.sh --yes --plugins=all` (or `none`, or a list:
  `--plugins="git jump z"`); force a mode with `--user` / `--system`.

**From a source checkout:** `make user-install` (no sudo) or `make my_shell`
(sudo). Or just run it without installing: `./build/bin/hellish` after
`make OPT=1 all`.

## Start and stop

- **Start**: open a new terminal (if installed as a login shell), or run
  `hellish`.
- **Stop**: `exit` or Ctrl-D. If stopped jobs exist, the first exit warns
  and the second obeys — like bash.
- **Skip the hook once** (user-install route): `HELLISH_NO_EXEC=1 bash`.
  Disable it entirely: `touch ~/.hellish-disable`.

## Uninstall

| how you installed | how to undo |
|---|---|
| `install.sh` / `make my_shell` | `make my-shell-uninstall` (restores your login shell **before** removing the binary); `make my-shell-purge` also removes config + caches |
| `install.sh` / `make user-install` | `make user-uninstall` — removes the rc hook and PATH block, leaves your `~/.hellishrc` |
| npm | `npm uninstall -g hellish-shell` |

## Configuration — where everything lives

| path | what it holds |
|---|---|
| `~/.hellishrc` | your startup file (aliases, exports, `shopt`, PS1) — sourced by **interactive** shells only, never by scripts. Seeded from `hellishrc.example`, never overwritten |
| `$XDG_CONFIG_HOME/hellish/rc.d/` | drop-in config fragments, sourced in order |
| `$XDG_CONFIG_HOME/hellish/themes/` | the 29 prompt themes (yours to edit; edited ones are never clobbered by reinstall) |
| `$XDG_CONFIG_HOME/hellish/plugins/` | plugins as `<name>/plugin.hsh` |
| `~/.hellish/` | the plugin framework (`conf`, `hxp`, its plugins and state) |
| history file | in `$HOME`, multi-line-safe; `shopt -s lithist` keeps newlines on recall |
| `~/.cache/hellish` | update-check state (which release was already announced) |

No passwords or credentials are stored anywhere — hellish has none to manage.
The files above are the complete list of what an install touches.

## Check that it works

```sh
hellish --version            # version + which repo the updater points at
echo 'echo ok $((6*7))' | hellish    # prints: ok 42
make doctor                  # from a checkout: which hellish PATH reaches,
                             # and whether `update` will need sudo
update                       # ask the release channel by hand
conf list                    # plugin framework: what is on and off
```

If a terminal ever misbehaves after an experiment: `reset` — and the
escape hatches above get you back to your previous shell at any time.

## Knobs

| variable | effect |
|---|---|
| `HELLISH_BANNER=0\|1` | welcome panel off / on |
| `HELLISH_NO_UPDATE_CHECK=1` | never check for updates, no badge |
| `HELLISH_NO_ANIM=1` | no startup animation |
| `HELLISH_NO_EXEC=1` | user-install hook: skip for this one session |
