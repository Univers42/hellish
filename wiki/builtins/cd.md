# `cd` — change the working directory

> **Move around without thinking about it.**
> hellish's `cd` is POSIX/bash-compatible *and* adds zsh's handy two-argument
> shortcut. This page is the practical guide: every form, every option, and the
> tips that make it pleasant day to day.

```
cd [-L|-P] [-e] [-@] [--] [dir]
cd -                      # jump back to the previous directory
cd old new                # substitution shortcut (hellish/zsh extension)
```

`cd` changes the shell's current directory and keeps `$PWD` / `$OLDPWD` in sync.
It is a **builtin** (it must be — a child process can't change *your* shell's
directory), so it always affects the shell you typed it in.

---

## The everyday forms

| You type | What happens |
|---|---|
| `cd` | Go **home** (`$HOME`). Errors `cd: HOME not set` if `HOME` is unset. |
| `cd /etc` | Go to an absolute path. |
| `cd src` | Go to `src` relative to where you are. |
| `cd ..` | Up one level. `cd ../..` up two, etc. |
| `cd ~` | Home again (`~` expands to `$HOME`, or your passwd home if `HOME` is unset). |
| `cd ~/Music` | A path under home. |
| `cd -` | **Toggle** back to the previous directory (`$OLDPWD`) — and print it. |
| `cd ""` | Empty operand: a no-op that succeeds (stays put), like bash. |

### `cd -` is the back button

The single most useful trick. `cd -` flips between the last two directories and
prints where you landed:

```sh
❯ cd /var/log
❯ cd /etc/nginx
❯ cd -
/var/log              # printed by cd; you're back in /var/log
❯ cd -
/etc/nginx            # and back again
```

Under the hood every successful `cd` saves your old location in `$OLDPWD`, and
`~-` is shorthand for it (`cat ~-/error.log`).

---

## `cd <arg1> <arg2>` — the substitution shortcut ⭐

This is the form you asked about. With **exactly two arguments**, `cd old new`
takes your **current directory string** (`$PWD`), replaces the **first
occurrence** of `old` with `new`, and changes to the result.

Think of it as *"swap one piece of my current path for another and take me
there."* It's perfect for hopping between parallel directory trees.

```sh
❯ cd /home/me/project/v1/src
❯ cd v1 v2
❯ pwd
/home/me/project/v2/src     # "v1" became "v2", everything else kept
```

More examples:

```sh
# Jump from one service's logs to another's, same layout:
❯ cd /srv/app/staging/logs
❯ cd staging production
❯ pwd
/srv/app/production/logs

# Bounce between a build and source tree:
❯ cd ~/code/mylib/build
❯ cd build src
❯ pwd
/home/me/code/mylib/src
```

**Rules of the substitution form**

- It edits `$PWD` *as a string* — `old` must literally appear in your current
  path. If it doesn't, you get an error and stay put:
  ```sh
  ❯ cd /home/me/project/v1/src
  ❯ cd zzz yyy
  hellish: cd: string not in pwd: zzz      # exit status 1, directory unchanged
  ```
- Only the **first** match of `old` is replaced.
- It does **not** print the new directory (in scripts / `-c`), matching zsh.
- Quote arguments that contain spaces or special characters as usual:
  `cd "Old Name" "New Name"`.

> **Heads-up — this is an extension, not bash.** bash (and `bash --posix`)
> reject two arguments with `cd: too many arguments`. hellish borrows this
> behaviour from **zsh**. There is no three-argument form anywhere: `cd a b c`
> is always an error (`too many arguments`, exit 1).

---

## Options

| Option | Meaning |
|---|---|
| `-L` | **Logical** (the default): treat the path the way you typed it, following symlinks "by name". |
| `-P` | **Physical**: resolve symlinks to their real targets. |
| `-e` | With `-P`, fail if the real directory can't be determined (accepted for bash parity). |
| `-@` | Present a file's extended attributes as a directory (accepted; platform-dependent). |
| `--` | End of options — everything after is treated as the directory, even if it starts with `-`. |

Options can be **bundled** and the last of `-L`/`-P` wins: `cd -LP …` is
physical, `cd -PL …` is logical. An unknown option is an error:

```sh
❯ cd -x
hellish: cd: -x: invalid option
hellish: cd: usage: cd [-L|[-P [-e]] [-@]] [dir]    # exit status 2
```

### Need to `cd` into a directory named like an option or `-`?

Use `--`:

```sh
cd -- -weird-dir        # a directory literally named "-weird-dir"
cd -- -                 # a directory literally named "-" (not the OLDPWD toggle)
```

---

## Logical vs physical (symlinks) — `-L` vs `-P`

This is the subtle one. Say `link` is a symlink to `project/v1`:

```sh
❯ cd /tmp/demo/link
❯ pwd
/tmp/demo/link              # -L (default): keeps the name you used
❯ cd ..
❯ pwd
/tmp/demo                   # ".." undoes "link" textually — back where you came from
```

With `-P` you walk the *real* filesystem instead:

```sh
❯ cd -P /tmp/demo/link
❯ pwd
/tmp/demo/project/v1        # resolved to the symlink's real target
```

- **Default (`-L`)** is what most people want: paths read back the way you typed
  them, and `cd somelink; cd ..` returns you to where you started.
- **`-P`** is for when you care about the canonical on-disk location (e.g.
  scripting around real paths).

---

## `CDPATH` — a search path for `cd`

Set `CDPATH` to a colon-separated list of "base" directories. When you `cd` to a
**plain name** (not absolute, not starting with `.` / `..`), hellish looks for it
under each `CDPATH` entry. On a hit via a non-empty entry, it **prints** the
directory it chose (so you're never surprised where you landed):

```sh
❯ export CDPATH=~/projects:/srv
❯ cd webapp           # finds ~/projects/webapp without typing the full path
/home/me/projects/webapp
```

Tips:
- Put `.` first (or an empty entry, e.g. `:~/projects`) if you still want the
  current directory searched before the shortcuts: `CDPATH=.:~/projects`.
- `CDPATH` only applies to bare names; `cd ./webapp` or `cd /abs/path` ignore it.

---

## What `cd` keeps in sync

- **`$PWD`** — your current directory (logical, unless you used `-P`).
- **`$OLDPWD`** — the directory you were in before the last `cd` (this is what
  `cd -` and `~-` use).
- `cd` reads **`$HOME`** for the no-argument and `~` forms. If `HOME` is unset,
  `cd` errors (`HOME not set`) while a bare `~` falls back to your password-file
  home — exactly like bash.

---

## Exit status

| Status | When |
|---|---|
| `0` | Directory changed (or empty operand no-op). |
| `1` | Directory doesn't exist / not a directory / not permitted; `HOME`/`OLDPWD` unset; `old` not found in `$PWD` for the two-argument form; three or more arguments. |
| `2` | Invalid option (a usage line is printed). |

Use it in scripts to guard work:

```sh
cd "$build_dir" || exit 1
make
```

---

## Quick reference

```sh
cd                      # $HOME
cd -                    # previous dir (prints it)
cd ~user                # another user's home (via the password database)
cd -- "$name"           # cd to $name even if it starts with '-'
cd -P "$link"           # resolve symlinks to the real path
cd old new              # swap "old" -> "new" in $PWD and go there  (extension)
CDPATH=~/proj cd app    # find ~/proj/app from anywhere
```

---

## See also

- `pwd` — print the current directory (the logical `$PWD`; use `cd -P` first if
  you need the physical path printed back).
- `pushd` / `popd` / `dirs` — a directory **stack** for deeper back-and-forth
  than `cd -`.
- The implementation and its test coverage: `src/builtins/README.md`
  (CD Implementation), `tests/cd_posix` (bash-parity cases), and
  `tests/cd_zsh_compare.sh` / `make cd-zsh-test` (the two-argument form verified
  against real zsh).
