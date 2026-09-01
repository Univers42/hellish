# Builtins — the reference

> Every name built into the shell, grouped the way `help` groups
> them, with the same synopses `help NAME` prints — because this
> page IS `help` output: regenerate it with `make docs-builtins`
> (tools/gen_builtins_md.py), never edit it by hand. The help
> table itself is test-enforced against the dispatch table, so
> neither this page nor `help` can drift from what actually
> runs. 69 builtins; anything else on `$PATH` works as usual —
> `type NAME` says which is which.

## navigation

**[`cd`](cd.md)** — `cd [-L|-P] [dir] | cd - | cd old new`
<br>change directory (honours CDPATH, - goes back)

**`dirs`** — `dirs [-c]`
<br>display the directory stack

**`popd`** — `popd [+N | -N] [-n]`
<br>pop the directory stack and cd there

**`pushd`** — `pushd [dir | +N | -N] [-n]`
<br>push a directory on the stack and cd there

**`pwd`** — `pwd [-L|-P]`
<br>print the working directory

## output

**`echo`** — `echo [-neE] [arg ...]`
<br>write arguments (-n no newline, -e escapes)

**`mapfile`** — `mapfile [-t] [-n n] [-O i] [-s n] [-u fd] [array]`
<br>read lines of input into an array

**`printf`** — `printf [-v var] format [arg ...]`
<br>format and print, like printf(3)

**`read`** — `read [-r] [-p prompt] [-n n] [-t sec] [name ...]`
<br>read one line into variables

**`readarray`** — `readarray [-t] [-n n] [-O i] [-s n] [array]`
<br>same as mapfile

**`umask`** — `umask [-S] [-p] [mode]`
<br>show or set the file-creation mask

## variables

**`declare`** — `declare [-aAfFgiIlnrtux] [-p] [name[=value] ...]`
<br>declare variables and give them attributes

**`export`** — `export [-p] [-n] [name[=value] ...]`
<br>put variables into the environment of commands

**`getopts`** — `getopts optstring name [arg ...]`
<br>parse option arguments in a loop

**`let`** — `let arg [arg ...]`
<br>evaluate arithmetic; status 1 if the last is 0

**`local`** — `local [name[=value] ...]`
<br>declare variables local to a function

**`readonly`** — `readonly [-p] [name[=value] ...]`
<br>make variables unassignable and unremovable

**`typeset`** — `typeset [-aAfFgiIlnrtux] [-p] [name[=value]...]`
<br>same as declare

**`unset`** — `unset [-f|-v] [name ...]`
<br>remove variables or functions

## jobs

**`bg`** — `bg [jobspec ...]`
<br>resume a stopped job in the background

**`fg`** — `fg [jobspec]`
<br>bring a job to the foreground

**`jobs`** — `jobs [-l] [-p] [jobspec ...]`
<br>list background jobs

**`kill`** — `kill [-s sig|-n num|-sig] pid|job ... | kill -l`
<br>send a signal to a process or job

**`wait`** — `wait [-n] [id ...]`
<br>wait for background jobs to finish

## control

**`break`** — `break [n]`
<br>leave a for/while/until loop

**`continue`** — `continue [n]`
<br>start the next turn of a loop

**`return`** — `return [n]`
<br>return from a function or a sourced file

**`shift`** — `shift [n]`
<br>drop the first n positional parameters

## commands

**`alias`** — `alias [-p] [name[=value] ...]`
<br>define or list command aliases

**`command`** — `command [-pVv] name [arg ...]`
<br>run a command, ignoring functions and aliases

**`hash`** — `hash [-lr] [-p path] [-dt] [name ...]`
<br>show or change the remembered command paths

**`type`** — `type [-afptP] name [name ...]`
<br>say how a name would be resolved

**`unalias`** — `unalias [-a] name [name ...]`
<br>remove aliases

## tests

**`:`** — `:`
<br>do nothing; the classic no-op

**`[`** — `[ expr ]`
<br>same as test; the closing ] is required

**`[[`** — `[[ expr ]]`
<br>conditional with pattern and regex matching

**`false`** — `false`
<br>do nothing, unsuccessfully

**`test`** — `test expr | [ expr ]`
<br>evaluate a conditional expression

**`true`** — `true`
<br>do nothing, successfully

## history

**`fc`** — `fc [-e ed] [-lnr] [first] [last] | fc -s [pat=rep]`
<br>re-edit and re-run past commands

**`history`** — `history [-c] [-d n] [n] | history -anrw [file]`
<br>show or edit the command history

## shell

**`.`** — `. filename [arguments]`
<br>read and run a file in the current shell

**`compgen`** — `compgen [-abcdfkv] [-A action] [-W wordlist] [word]`
<br>print the completions the shell would offer for word

**`complete`** — `complete [-abcdfkv] [-A action] [-W list] [-F fn] [-pr] name ...`
<br>register what to offer when completing an argument of name

**`eval`** — `eval [arg ...]`
<br>join the arguments and run them as a command

**`exec`** — `exec [command [arg ...]] [redirection ...]`
<br>replace the shell, or apply redirections to it

**`exit`** — `exit [n]`
<br>leave the shell with status n

**`help`** — `help [-s] [topic ...]`
<br>this. `help NAME` explains one topic

**`pretty`** — `pretty [-p] | list | on NAME... | off NAME... | mode NAME`
<br>named presets for the shell's behaviour knobs

**`set`** — `set [-abefhkmnptuvxCH] [-o opt] [--] [arg ...]`
<br>set shell options and positional parameters

**`shopt`** — `shopt [-pqsu] [-o] [optname ...]`
<br>toggle the extra (non-POSIX) shell options

**`source`** — `source filename [arguments]`
<br>same as .

**`times`** — `times`
<br>print user and system time for the shell

**`trap`** — `trap [-lp] [[action] signal ...]`
<br>run an action on a signal or on EXIT

**`ulimit`** — `ulimit [-SHabcdefiklmnpqrstuvxPT] [limit]`
<br>show or set process resource limits

**`update`** — `update [--now|--check|--version]`
<br>check for a newer hellish and install it

## syntax

Not builtins — the grammar `help` also explains, kept here for the same one-stop reason.

**`for`** — `for NAME [in WORD ...]; do LIST; done`
<br>loop over words

**`for((`** — `for ((exp1; exp2; exp3)); do LIST; done`
<br>C-style counting loop

**`while`** — `while LIST; do LIST; done`
<br>loop while a list succeeds

**`until`** — `until LIST; do LIST; done`
<br>loop until a list succeeds

**`if`** — `if LIST; then LIST; [elif ...] [else LIST] fi`
<br>branch on a command's exit status

**`case`** — `case WORD in PATTERN) LIST ;; ... esac`
<br>branch on a pattern match

**`function`** — `name () { LIST; } | function name { LIST; }`
<br>define a function

**`((`** — `(( expression ))`
<br>arithmetic; status 0 when the value is non-zero

**`$((`** — `$(( expression ))`
<br>substitute the value of an arithmetic expression

**`$(`** — `$( LIST )  or  `LIST``
<br>substitute a command's output

**`redirection`** — `> < >> 2>&1 <<EOF <<<word /dev/tcp/host/port`
<br>send a command's input and output elsewhere

**`pipeline`** — `cmd1 | cmd2   [!] cmd1 |& cmd2`
<br>feed one command's output into the next

## zsh

Only reachable when the dialect is armed (`set -o zsh`, `emulate zsh`, or sourcing a `.zsh` file) — see the [zsh dialect](../architecture.md#the-zsh-dialect).

**`add-zsh-hook`** — `add-zsh-hook hook function`
<br>run a function on an event; only chpwd fires here

**`autoload`** — `autoload [-Uz] name ...`
<br>define a function from a file on $fpath

**`bindkey`** — `bindkey [-M keymap] seq widget`
<br>record a key binding for a zle widget

**`colors`** — `colors`
<br>define $fg[..] $bg[..] $fg_bold[..] $reset_color like zsh's colors

**`compdef`** — `compdef [args ...]`
<br>not supported: hellish has no zsh completion system

**`emulate`** — `emulate [-L] {zsh|sh|ksh|bash}`
<br>switch dialect; in a function it reverts on return

**`print`** — `print [-nrlP] [--] [arg ...]`
<br>zsh's echo: escapes on by default, -r turns them off

**`setopt`** — `setopt [name ...]`
<br>turn on zsh-named shell options

**`unsetopt`** — `unsetopt [name ...]`
<br>turn off zsh-named shell options

**`vcs_info`** — `vcs_info`
<br>fill $vcs_info_msg_0_ with the git branch, zstyle formats honoured

**`zle`** — `zle -N widget [fn] | zle widget`
<br>register a line-editor widget; key dispatch is not wired yet

**`zmodload`** — `zmodload [args ...]`
<br>not supported: hellish has no loadable modules

**`zstyle`** — `zstyle [pattern style value ...]`
<br>vcs_info formats/actionformats are honoured; the rest is stubbed

