# hellish — Release Notes

> 🔥 **hellish** is a fast, almost‑POSIX shell written from scratch in C —
> hackable, observable, and pleasant to live in.

This file is what the welcome banner points you at. It tracks what's new and
shows you how to drive the shell.

---

## v2.8.9 — *the export release*

The first 42 account to load its real `~/.zshrc` inside hellish saw
`manpath: warning: $PATH not set` at every prompt. nvm was the messenger,
not the bug: `nvm_use` reassigns `PATH="$(nvm_change_path …)"`, runs
`$(manpath)`, and only then says `export PATH` — and in hellish **a plain
assignment to an exported variable dropped its export attribute**, so the
child ran with no PATH. `PATH="$PATH:/x"` has un-exported PATH in every
release so far; nothing in 4362 golden cases assigned to an exported
variable and then looked at a child.

- **The export attribute is sticky**, as in every POSIX shell: only
  `unset`, `export -n` and `declare +x` take it away.
- **`export -n`, `declare +x`, `typeset +x` actually un-export** — the
  option was accepted and ignored, and nothing could ever un-export a
  variable.
- `tests/export_attr`: 29 cases diffed against bash 5.3.9 — reassignment
  in functions, through `$(...)`, `+=`, `for`, subshells, groups; every
  un-export spelling; `unset` then reassign; and the exact nvm shape.
- `tests/rc_realworld_test.py`: a 42-style `~/.zshrc` (Homebrew, the nvm
  block, `~/.local/bin`, aliases) loaded through the installer's import
  module, on a pty, with a vendored stand-in for the nvm pattern so it
  runs everywhere — no warning, the alias works, children see PATH, first
  prompt under two seconds.

---

## v2.8.8 — *the re-run release*

One line in one install log — `generated ~/.hellishrc would not parse —
left untouched` — and the install stopped there: binary copied, nothing
hooked, no plugin question. The seeder validated the rc with `sh -n`, and
`/bin/sh` is dash on Debian/Ubuntu, which rejects `HX_LOADED=()` — the
plugin framework's own loader, bash syntax hellish reads happily. So every
**re-run** of the installer over a framework-managed `~/.hellishrc` died
under the callers' `set -e`.

- The seeder now asks **the hellish it just installed** (then bash) whether
  the rc parses, refuses its PATH block only when the block itself broke a
  file that parsed before, and a refusal is a warning — the install carries
  on to the smoke test, the hook and the framework.
- The seeder ships in the install bundle, which is why this was cut as a
  release — and why the same failure reproduced once more *after* the fix
  was on `main`: `curl | sh` took `tools/seed_hellishrc.sh` from the
  latest release's bundle. **`install.sh` now refreshes its driver scripts
  from `main` on top of whatever bundle it downloaded** (themes still come
  from the bundle; an unreachable `main` falls back to the bundle's copies
  and says so). A fix to the install path is live the moment it is pushed,
  for every release, old ones included.

Detectors: the installer suite re-runs `install.sh` over the framework rc
after scenario A and requires it to finish, write the block, and leave the
hook and framework in place; scenario I serves a release whose bundle
carries a marked, stale seeder and requires `main`'s drivers to run
instead — and the bundle's only when `main` is unreachable.

---

## v2.8.7 — *the school release*

Two field reports from 42 Madrid, the same afternoon, one install each:

- **`curl | sh` no longer stops at `Username for 'https://github.com':`
  (#111).** The school image's git 2.34 takes a 401 from GitHub over
  HTTP/2 and, with a terminal attached, *asks* instead of failing. The
  clone now speaks HTTP/1.1 and runs with `GIT_TERMINAL_PROMPT=0` and no
  credential helper — a public repo never needs a password, so any request
  for one is a failure, and failing is what lets the tarball fallback
  install the plugin framework anyway.
- **A zsh config works as a hellish config (#112).** `~/.hellishrc` is a
  bash-dialect file, and the first zsh idiom in every prompt tutorial —
  `precmd() { vcs_info }` — closes its group with a bare `}` that bash
  reads as an argument; the rest of the file was swallowed and the only
  word was "unexpected end of file". Now:
  - **`emulate zsh` / `set -o zsh` on line 1 governs the rest of the
    file** — sourced or run as a script. The dialect switch joins
    `shopt`, `alias` and `source` as a lexing hazard, so the lines after
    it are tokenised *after* it ran (the #105 family, one member short).
  - **`~/.config/hellish/rc.d/NN-name.zsh` loads in the zsh dialect**,
    sorted in with the `.hsh` modules — the marker-free home for a zsh
    file. The installer offers to write one that loads your whole
    `~/.zshrc` when your login shell is zsh.
  - **The unmarked paste names the cure**: the bash dialect stays bash
    (`echo }` prints a brace, as the golden suite pins), but the error
    now says which line's `}` is zsh-only and points at `emulate zsh`.
  - **vcs_info matches zsh 5.9** byte for byte: `%c`/`%u` from
    `stagedstr`/`unstagedstr` under `check-for-changes` (index vs work
    tree, untracked never counts), `%r`/`%R`, `%%`, and prompt escapes
    inside a format (`%F{242}`) pass *through* to the prompt instead of
    being eaten. `zstyle ':vcs_info:*' enable git` and every other
    vcs_info key are accepted silently — no more "not supported" at
    every shell start.
  - **PROMPT expands in zsh's order**: parameters first, then `%F{..}`
    over the result — so `${vcs_info_msg_0_}` carrying colours renders
    them instead of printing `%F{magenta}`.
- **The plugin framework's `forge lint` works on a user-mode install**
  (the 42 machine): it linted with a hardcoded `/usr/bin/hellish` and
  failed all seven builtin plugins; it now uses the hellish running it,
  and the framework's own suite is 94/94 on a `~/.local/bin` install.
  The framework's `rc.d` loads `*.zsh` modules too.
- **CI: the Tumbleweed rung survives an image/mirror skew** (the image
  shipped glibc 2.44 while the mirrors' gcc16 wanted 2.43): a refused
  install now falls back to `zypper dup`, the documented way to bring a
  rolling container in line with its repositories.

**Installer follow-up, on `main` (every version fetches `install.sh` from
there):** piped through `sh`, the script has no file of its own, and
`dirname "$0"` was the *current directory* — so `update --now` typed inside
any hellish checkout treated that checkout as the source tree and
installed its `build/bin/hellish`, however old, without downloading
anything. A 2.3.2 came back that way onto a machine whose channel said
2.8.7. Now only a real script path counts as a checkout, a checkout build
is used only when it matches `incs/version.h` (a stale one is refused by
name and the release fetched), and an older `hellish` sitting ahead on
`PATH` is called out at the end. Scenario H of the installer suite pins
all three.

Detectors, permanent: `tests/zsh_dialect_switch_test.py` (the verbatim
#112 rc under every route, the hint, `.zsh` under rc.d, scope of `-L`);
`tests/vcs_info_zstyle_test.py` (state by state against a real zsh);
`tests/installer_suite.sh` grew scenario F (a fake GitHub that answers
git with 401, on a real pty, bounded by a timeout), scenario G (a zsh
login shell with the #112 `~/.zshrc`, imported and running with no
parse error) and runs the framework's own suite after every user
install; `tests/helpers/fake_release_server.py` is the 401-capable channel.

---

## v2.8.6 — *the TAB release*

v2.8.5 made bash-completion LOAD; the first real TAB on a Debian 13 box
then ran its completion functions and hit the next stratum. All fixed,
with a detector that presses TAB for real:

- **`[[ ]]` continues across newlines** where bash's grammar allows it
  (after `[[`, after `&&`/`||`, before `]]`) — bash-completion's
  compat-dir loop spells its test `[[ a != @(...) &&` ⏎ `-f b ]]`, and
  the split conditional printed `[[: missing ]]` once per drop-in file
  at every login.
- **Indirection composes**: `${!ref-}`, `${!ref:-x}` — and the target
  may be an array element (`ref="words[0]"`), which is how
  _comp_get_words assembles every completion word. `printf -v "a[0]"`
  writes the element, not a scalar named `a[0]`.
- **Substring bounds expand**: `${cur:0:${#words[i]}}` — the exact
  spelling the cursor walk uses.
- **bash's upvar rule**: `unset -v x` from a deeper frame pops the
  caller's local cell so a following assignment writes the revealed
  scope — the documented trick `_comp_upvars` returns values with. A
  bare `local x` re-declaration keeps the value, like bash.
- **Quoted slices keep fields**: `"${@:3:N}"` is one field per
  positional (so `words=("${@:3:N}")` counts right), and the
  set-guarded pass-through `${w+"${w[@]}"}` / `${w[@]+"${w[@]}"}` keeps
  per-element fields, empties included.
- **`compgen` grew `-X` filter / `-P` prefix / `-S` suffix**, the
  `-e/-g/-j/-s/-u` action letters, and **expands its `-W` list** — the
  deferred `-W '"${toks[@]}"'` spelling every generator uses now yields
  elements instead of pasting `"${toks[@]}"` into the command line.
  **`declare -F -- name`** answers 0 like bash.

And the third wave — "source ~/.hellishrc got slow again":

- **Re-sourcing an rc is as cheap as the first load.** Two multipliers
  stacked: the forkless `$(...)` fast path turned itself off the moment
  ANY alias existed (now it only bails when the command word itself is
  aliased), and the DEBUG trap fired before every statement *inside* a
  sourced file where bash fires once for the whole `.` run (eval keeps
  its per-statement fires — both measured on 5.3.9). A theme rc that
  installs `trap hx_preexec DEBUG` plus a dozen aliases forked ~370
  extra times per re-source; it now re-sources *faster than bash*
  (0.35s vs 0.43s for nine loads of the 1225-line theme file).
- **Sourcing a construct with interior hazard words is linear**: growth
  spans stop re-clipping at alias/source/shopt words once the parser has
  demanded more — a hazard line inside an open construct cannot take
  effect before the construct executes in bash either. The 2244-line
  monolithic rc: 0.21 s → 0.005 s on the reporting machine.

Detectors, permanent: `tests/tab_completion_chain_test.py` sources the
real bash-completion 2.16 with a POPULATED compat dir on a pty and
presses TAB, asserting no error of any class and that a filename
actually completes; `tests/issue105_expansions` (39 golden lines),
`tests/scripts/45_dbracket_newline.sh` and
`tests/scripts/46_debug_trap_source.sh` pin every construct against
bash 5.3.9; `tests/parse_scaling_test.py` now also caps the re-source
ratio with aliases + a DEBUG trap installed.

---

## v2.8.5 — *the login-chain release*

One field report — `ssh` into a Debian 13 box printed
`hellish: syntax error near unexpected token `('` at every login (#105) —
unravelled into a family of "lexing ahead of execution" bugs, all fixed:

- **`. bash_completion` works.** hellish sets `BASH_VERSION`, so the stock
  Debian `~/.profile` sources `~/.bashrc`, which sources bash-completion —
  a file that runs `shopt -s extglob` on line 47 and relies on it ~1800
  lines down. `exec_string` (eval and the dot builtin) tokenized the whole
  file before executing any of it, so the option never reached the lexer.
  Sourced text now flows through hazard-clipped chunks exactly like the
  main input driver: `shopt`, alias definitions and nested sources take
  effect for the rest of the same file, on bash's own line-granular terms.
  Both bash-completion 2.11 (Ubuntu 22.04) and 2.14 (24.04) now load
  silently, and the plugin corpus row moved from `unsupported` to `loads`.
- **`shopt` joined the batch hazards**, so the script/file path lexes
  later lines after a top-level `shopt` executed, matching bash.
- **Extglob inside `[[ ]]` without `shopt -s extglob`** — bash enables it
  while parsing and matching the right side of == / != (4.1 semantics);
  hellish's lexer and matcher now do too, so bash-completion's
  `[[ $(bind -v) == *+([[:space:]])on* ]]` idiom matches.
- **`eval -- "text"`** no longer runs `--` as a command;
  **`complete`** learned the `-e/-g/-j/-s/-u` action letters and installs
  `complete -D` default specs silently instead of dumping every
  registration to stdout; **`shopt`** answers real-but-unimplemented bash
  option names honestly (query/print say off, `-u` succeeds, `-s` refuses
  loudly) instead of "invalid shell option name".

Detectors, permanent: `tests/login_bashrc_chain_test.py` drives the whole
profile→bashrc→completion chain in a pty; `tests/scripts/42–44` diff the
extglob/alias/[[ shapes against bash 5.3.9 byte-for-byte; the corpus now
*requires* bash-completion to load.

---

## v2.8.4 — *the fast-scripts release*

Two field reports, one performance audit — and permanent detectors for
all of it:

- **Script files parse in linear time (#101, #102):** a script that is
  one large compound command — a monolithic rc whose whole body is a
  single `if`/`else`, like hellishrc_plugins' theme file — parsed in
  O(n²): the batch reader only batched a cycle's *first* delivery, so one
  hazard byte (the word "alias" in a comment sufficed) pushed the whole
  remaining body onto the line-at-a-time path, and every appended line
  re-lexed and re-parsed everything before it. 2242 real rc lines took
  **5 seconds** where bash takes 7 ms — while *sourcing the same bytes*
  took 14 ms, which is why nobody's startup felt slow. Continuation
  rounds now batch too (heredoc cycles keep the exact line path, and a
  failed batch still rewinds to the line-exact replay), taking that file
  to **~15 ms**. This also resolves the audit's mystery of why repeating
  a fragment was fast but the real file was slow: repetition makes many
  *small* compounds, the real rc is one *large* one.
  `tests/parse_scaling_test.py` now pins the shape — t(4N) must stay
  near-linear over t(N) across the file, pipe, and monolith paths.
- **`exit` with no `$HOME` no longer segfaults (#98):** with a
  near-empty environment (`env -i`, some display managers, containers)
  the history vector was never initialised, and its zero stride made
  every append grow a phantom entry — the dedup check and the exit
  teardown then read wild pointers. The vector is now valid before the
  history file is even looked for. `tests/empty_env_test.py` sweeps the
  whole class: bare exit, dedup, `history` builtins, `cd`/`~` with no
  HOME, a HOME that doesn't exist, piped and `-c` runs, and PATH-less
  absolute execution.
- **The performance audit (#102)** found the rest healthy: startup is
  the fastest of dash/bash/hellish (0.33 ms/spawn), execution beats bash
  on most microbenchmarks, and hellish is the only one of the three that
  doesn't fork for an all-builtin `$( )`. The one loss (`case` at scale)
  is the static-musl build flavor, not the algorithm — the glibc build
  beats bash on the same test.

---

## v2.8.3 — *the four-bug-reports release*

Four field reports, four root causes, and a detector for each so none of
them can quietly come back:

- **The big one (#94):** a double free in AST teardown could SIGSEGV the
  shell on large rc-shaped scripts. The parse arena's rare heap-fallback
  handed the reparser a shared token it then freed once per word — a
  state so deep it needed hundreds of MB of parse allocations to reach,
  which is why no normal-size test ever saw it. Fixed at the root, and
  `make arena-stress` now shrinks the arena chunks to 512 bytes so those
  rare states happen every few nodes, under ASan, on every CI run.
- **`$(case ...)` parses (#95):** a case pattern's unbalanced `)` no
  longer ends the command substitution — so the idiomatic
  `n=$(case "$x" in ''|*[!0-9]*) echo no ;; *) echo yes ;; esac)` works,
  and one such line no longer silently disables the rest of your rc.
- **Preexec hooks from functions (#96):** `trap 'my_preexec' DEBUG`
  inside a function now persists after the function returns, exactly as
  bash does — the canonical installable-hook shape works. Subshells got
  bash's other half too: inherited DEBUG/RETURN/ERR traps stay listable
  in `$( )` but never fire there. And because bash-preexec's hook could
  now actually run, hellish gained the **`builtin`** builtin it calls
  (`builtin history 1`) — 69 builtins now.
- **`$COLUMNS` and `$LINES` (#97):** set at interactive startup and kept
  current across resizes (honouring `shopt checkwinsize`), unexported,
  like bash — a right prompt no longer needs to fork `tput cols`.

Verified green across every layer: 4323 golden cases on both the ASan
and release builds, whole-script + hard corpora, the pty suite (plus a
new resize test), allocator parity on both heaps, the 13-plugin corpus,
and the new arena stress.

---

## v2.8.2 — *the customize-your-prompt release*

Paste any zsh prompt tutorial into `~/.hellishrc` and it now just works
(issue #91):

- **`colors`** defines `$fg[..]`, `$bg[..]`, `$fg_bold[..]`,
  `$reset_color` — the arrays zsh's own colors function provides.
- **`vcs_info`** fills `$vcs_info_msg_0_` from the prompt's fork-free
  cached git reader; **`zstyle`** silently honours the
  `formats`/`actionformats` it reads (everything else keeps the loud stub).
- **`precmd` / `preexec`** functions (and the `precmd_functions` /
  `preexec_functions` arrays) fire like zsh's — unless bash-preexec is
  loaded, which owns the convention and would otherwise double-fire.
- **`RPROMPT`** renders at the right margin, invisible to readline's
  width model, skipped on dumb terminals.
- And the bug that started it: the alias scanner expanded aliases inside
  `case` **patterns**, so re-sourcing a config whose loader contains
  `case "$1" in list|ls)` died on
  `syntax error near unexpected token --color=auto` with the stock
  `ls` alias active. Patterns are never commands now — 12 golden cases
  diff every shape (`|`, `(`, `;;`, `;&`, `;;&`, newline-led, nested)
  against bash 5.3.9, and `tests/prompt_zshrc_test.py` drives the whole
  tutorial rc on a pty inside a real repository, ten checks.

---

## v2.8.1 — *the first-prompt release*

A fresh `curl install.sh | sh` with the plugin framework, bash-preexec and z
enabled greeted its very first prompt with
`_hx_precmd_run__bp_install: command not found`, a `[1] <pid>` job notice,
and two whole function bodies dumped over the banner (issue #88). Three
separate shell bugs stacked into that first impression, each fixed and
pinned:

- **POSIX character classes in `case` and `${var#…}` patterns** —
  `[[:space:]]`, `[![:space:]]` and the rest matched *nothing* in the
  case/trim matcher (filename globs had them all along — a second matcher
  had drifted). bash-preexec's own PROMPT_COMMAND surgery depends on them.
  31 new golden cases diff the classes against bash 5.3.9 on every push.
- **`declare -ft fn` is silent** — `-f` plus an attribute letter applies
  the attribute in bash; it printed the function bodies here, which is
  where the dump came from. Measured semantics: silent, 0 when every name
  is a function, 1 otherwise. Plain `declare -f fn` still prints.
- **`( cmd & )` no longer announces a job** — bash keeps job control off
  in subshells, and z backgrounds its bookkeeping exactly that way to stay
  silent. A top-level `cmd &` still prints its `[n] pid` line.

New regression rig: `tests/fresh_install_test.py` rebuilds the reporting
machine — the framework's PROMPT_COMMAND convention plus the real upstream
`bash-preexec.sh` and `z.sh` on a live pty. It fails 5 of its 8 checks
against v2.8.0 and passes 8/8 here.

---

## v2.8.0 — *the runs-your-plugins release*

The biggest release since 2.0. The headline: **real third-party plugins now
load and run** — oh-my-zsh plugins, git's own completion and prompt scripts,
bash-preexec, z — and the shell now proves it on every CI run instead of
claiming it. Under that headline sit an opt-in zsh dialect, programmable
completion wired into TAB, a whole zsh prompt language, a line-editor widget
layer, an installer that can prove what it installed, and a long list of
silent divergences from bash found *by running other people's code* and fixed
with a test each.

### The zsh dialect — opt-in, and off means off

zsh syntax is not bash and not POSIX, so none of it is reachable until
something arms the mode: `set -o zsh`, `emulate zsh`, or sourcing a `.zsh`
file (restored when the file finishes). There is deliberately **no heuristic
on file content** — the 4248-case golden suite pins the bash meaning of the
same text, so the dialect cannot leak into your scripts by guesswork.

Inside the mode, the things real plugins actually use:

- **Parameter-expansion flags** — `${(f)x}` `${(s:/:)x}` `${(j:-:)x}`
  `${(k)x}` `${(%)x}` `${(q)x}` `${(U)x}` `${(L)x}` and friends, plus the
  modifiers `${x:a}` `:h` `:t` `:r`, nested expansions, and the `:#` filter.
  Anything unimplemented fails loudly instead of returning the unflagged
  value.
- **Builtins**: `setopt`/`unsetopt` (mirrored into the glob layer, so
  `setopt dotglob` in a plugin actually changes what `*` matches), `print`
  (`-r -n -l -P`), `emulate`, `autoload`, `add-zsh-hook`, `shift NAME`, and
  loud stubs for `zmodload`/`zstyle`/`compdef` — they say what is missing
  rather than pretending.
- **Grammar**: `function name { }` (also plain bash, #71), anonymous
  `() { … }` functions, empty compound bodies, `} always { }`,
  `for a b (…)`, `}` without a separator, multi-name function definitions,
  `[[ -o opt ]]`, `$+commands[x]`, unbraced `$arr[i]`.
- **Arrays, the zsh way**: 1-based indexing, `$#name`, `a[lo,hi]` slices
  (read *and* write), `a[i]=(…)` splices, `shift NAME` — measured against
  zsh 5.9, which `make zsh-oracle` pins the same way `make oracle` pins
  bash 5.3.9.
- **Glob qualifiers** and `=(cmd)` process substitution.

### The plugin corpus — the acceptance test for all of it

`make plugin-corpus` sources **13 real third-party plugins** — eight from
oh-my-zsh, git's `git-prompt.sh` and `git-completion.bash`, bash-preexec,
`z`, and bash-completion — against both the release build and the ASan
build. Twelve load; every row declares an expectation (`loads`,
`loads-noisy`, `unsupported`), and **a plugin that starts working is a
failure until its expectation is updated**, so the matrix cannot rot.

It earned its keep immediately: a heap bug that existed only in release
builds while the golden suite passed everything in debug, and an 18 KB alias
leak invisible to ASan, were both found by sourcing code nobody here wrote.

### The line editor speaks zsh — `zle`, `bindkey`, widgets

- `zle -N name [fn]` registers widgets; `bindkey` binds them; `BUFFER`,
  `LBUFFER`, `RBUFFER` and `CURSOR` are readable *and writable* from widget
  functions. oh-my-zsh's **sudo** plugin (ESC-ESC prepends `sudo`) works.
- **A widget's `cd` now reaches your shell.** Readline runs in a forked
  child, so a directory change used to die with the fork. The child now
  reports its final directory on a dedicated pipe — which is what makes
  oh-my-zsh's **dirhistory** (Alt-Left/Alt-Right to walk your cd history)
  actually navigate.
- `zle -M msg` prints a message under the line being edited and repaints;
  `zle -R` refreshes. Every *other* zle option now says once, honestly, that
  it is not supported instead of claiming success — a message you never see
  is indistinguishable from a working one.
- A built-in widget's edit is no longer undone by the write-back, and the
  dispatcher no longer leaks a widget name per keypress.
- `region_highlight` stays out, deliberately: readline has no styled-region
  model, and a registration that silently never fires would leave a plugin
  believing it is installed. `incs/zle.h` says so; the corpus records it.

### Programmable completion — TAB consults your specs

`complete` and `compgen` exist, and — the half that was missing — **TAB
actually asks them**: `-W` word lists, `-F` functions (with `COMP_WORDS`,
`COMP_CWORD`, `COMP_LINE`, `COMP_POINT`, `COMPREPLY` and `$1 $2 $3` as bash
defines them), and the `-A` actions. Sourcing git's own
`git-completion.bash` and typing `git che<TAB>` offers
`checkout cherry-pick cherry` — byte-identical to bash 5.3.9.

It sits behind `shopt -s progcomp`, **off by default where bash has it on**,
and the reason is measured, not cautious: `shopt -q progcomp` is the exact
gate `/etc/profile.d/bash_completion.sh` probes, and answering yes makes a
Debian/Ubuntu login source bash-completion's 3800-line framework, which
hellish cannot yet parse (`shopt -s extglob` on line 47 has not *run* when
the extglob pattern on line 1810 is tokenised — hellish lexes a sourced file
whole, bash reads it incrementally). Put `shopt -s progcomp` in your rc and
every spec works; the corpus carries the row that will say when the default
can flip.

### The prompt — two languages, 29 themes, and a quieter default

- **The default prompt is now zsh's own**: `hostname% ` — plus exactly one
  thing hellish still volunteers, the self-spacing `⬆` update badge. A
  shell's first prompt should look like the shell you already know, not like
  something you have to figure out how to turn off. The rich two-row theme
  did not go anywhere: `prompt` lists **29 themes** (`prompt <name>` to
  switch, `prompt save` to persist), and `PS1='\B'` is the old default by
  name.
- **The entire zsh prompt language** — `%n %m %~ %#`, `%F{color}`,
  `%(?.ok.no)` conditionals, truncation, the time family — measured against
  zsh 5.9. `PROMPT` and `print -P` keep exact zsh semantics; under
  `set -o zsh`, PS1 *is* PROMPT, as in zsh.
- **PS1 is bilingual.** Paste `%n@%m %~ %#` *or* `\u@\h \w \$` into plain
  PS1 and both render — while `100% `, a csh-style `%> `, and the strftime
  percents inside `$(date +%H:%M)` or `\D{%M}` all survive literally. Unknown
  `%` sequences stay on screen in plain PS1; strict zsh contexts consume
  them, exactly as zsh does.
- **Bash's escape set is complete**: `\D{fmt} \T \@ \! \# \l \r \V` joined
  `\u \h \w \t` and the rest — they used to print their own spelling into
  the prompt. `\A` is the one deliberate divergence (hellish's animation
  frame, shipped first; `\D{%H:%M}` is bash's clock). Bare `$?`, `$(cmd)`
  and `$((expr))` in PS1 expand like bash's promptvars.
- **Hook arrays**: `HELLISH_PRECMD_FUNCS` and `HELLISH_PREEXEC_FUNCS` run
  around every interactive command — *arrays* of function names, so two
  plugins can both attach without silently removing each other (the
  `trap DEBUG` failure mode). `$?` is preserved across them. bash-preexec
  loads.
- OSC title sequences no longer confuse the width model, `$((…))` works in
  prompts, and 24-bit color, wide characters and the venv badge all keep
  the cursor where it belongs.

### Install, update, and the login shell — issue #76, end to end

- **Releases ship a static musl binary.** The old glibc build did not run
  on Debian 11, Ubuntu 20.04, RHEL 8 — or a stock ubuntu:24.04, the distro
  it was *built* on (no libreadline8 by default). Measured on the published
  asset, fixed by publishing `make static`'s output, verified on all three.
- **The updater survives its own install.** Judging an elevated install by
  its exit status (not by a version re-read that raced), staging the
  download somewhere writable when elevation is needed, and surviving
  `/proc/self/exe` pointing at a replaced binary mid-run.
- **`make doctor`** tells you which `hellish` your PATH actually reaches and
  whether `update` will need elevation — the two things behind most "the
  update did nothing" reports (a stale `~/.local/bin` copy shadowing
  `/usr/bin` being the classic).
- **`make my_shell VERSION=2.7.2`** installs a *published release* instead
  of the working tree — the only way to reproduce a bug that lives in an
  installed updater. `make my-shell-uninstall` reverses it (login shell
  restored *before* the binary is deleted); `my-shell-purge` also drops the
  caches that remember which release was announced.
- **`command_not_found_handle`** — bash's hook, so Ubuntu's
  "Command 'vim' not found, but can be installed with: sudo apt install vim"
  helper works. Runs where bash runs it (a child; a handler cannot `cd` your
  shell), and its status becomes the command's status.
- A real **ssh login-shell suite**: sshd + `chsh` in docker, diffing
  ssh-command, scp, sftp, rsync and git-over-ssh against bash as the login
  shell — because those protocols fail on one stray byte of stdout, and no
  other layer can see it.

### Silent divergences from bash, found by running real code

Each of these was reached by sourcing third-party scripts, not by a test we
wrote for it — and every one was silent:

- `[[ x =~ "a b" ]]` came apart at the space: the line is in Ubuntu's own
  `/etc/profile.d/vte-2.91.sh`, so **every GNOME login printed a syntax
  error**. All of `/etc/profile.d` parses clean now.
- A bare `{` inside `${…}` counted as a nesting brace → `unexpected EOF` on
  valid bash.
- `${#arr[@]}` was unknown to the `(( ))` *command*, so
  `for ((i=0; i<${#W[@]}; i++))` ran zero times and reported success.
- An empty unquoted `${a:+…}` contributed an empty argv slot instead of no
  field.
- Prefix/suffix trims and `${v//…/…}` used a private pattern matcher that
  ignored `[ranges]`; there is now **one** matcher shared by `case`,
  `[[ == ]]`, trims and globs — which surfaced three more bugs, including an
  unterminated `[` that matched things it did not name.
- `compgen -W "$o" -- "$cur"` — the idiom every completion script uses —
  died on `--`.
- Array literals `arr=($V)` and `arr=(*.md)` now split and glob like bash.
- `VAR=x cmd` corrupted the heap via a second, incomplete `t_scope_save`
  constructor — release-only, invisible to the entire debug suite.
- `a[i]+=` appended nothing; `declare NAME[sub]=` did nothing; `export`
  value fidelity, `shift` operand validation, empty assoc keys and
  `declare -p` quoting all diffed against bash and fixed.
- A process substitution can be an assignment's value (#83); `exit_clean`
  abandoned the environment table (#78); `printf` wide field widths and the
  full unsigned range (#73); `local` no longer leaves bookkeeping on the
  heap; killing a foreground job gives the terminal back (#85).

### Options that stopped lying

`nullglob`, `dotglob`, `globstar`, `extglob`, `nocaseglob`: every one used
to be storable, reportable — and read by nothing. All five now change what
actually matches, through **one** mirror function with three callers, so
`pretty`, `shopt` and zsh's `setopt` can no longer disagree about what `**`
means.

### The long tail

`read -n N`, `&>file`, `${!prefix*}`, `${arr[@]:-default}`, `printf %q`,
`printf '\e'`, `case … ;;&`, `test -v`, `hash -l`, `\D{…}` in PS1 — and
`FUNCNAME`/`BASH_SOURCE` are rebuilt on read rather than on every call.

### rc files and plugins have a real load path

`/etc/hellish/rc.d` → `$XDG_CONFIG_HOME/hellish/rc.d` → `plugins/*/plugin.hsh`
→ `~/.hellishrc` last; `--norc` and `--rcfile=FILE` for scripts and tests;
`tools/seed_hellishrc.sh` seeds rc.d, plugins, the 29 themes and the
`prompt` switcher — never clobbering anything you wrote.

### Testing and CI, because none of the above counts unclaimed

- The golden suite grew to **4248 cases**, still diffed live against a
  pinned bash 5.3.9 (`make oracle`); the zsh dialect diffs against a pinned
  zsh 5.9 (`make zsh-oracle`).
- `make test-release` exists because ASan and `-O3` disagree: one heap bug
  passed 3790/3790 in debug while the release binary segfaulted.
- CI gained required jobs for the ft_malloc allocator (release + `SAFE=0`),
  the prompt width model, the plugin corpus, and the update path.
- The pty suite runs `--norc`, so it stopped inheriting the developer's
  own config, and proves its isolation instead of just claiming it.



The 2.7.5 fix taken through the full CI gate, plus a proper split between the
build you develop against and the build you ship.

**Changed**

- **Three named build configurations.** `make` now takes
  `MODE=debug|release|relwithdebinfo`, and each one is a deliberate answer
  rather than a side effect of which flags happened to be set:

  | MODE | flags | for |
  |---|---|---|
  | `debug` *(default)* | `-O0 -g3 -ggdb`, ASan + LSan, libc malloc | developing |
  | `release` | `-O3 -DNDEBUG`, LTO, `--gc-sections`, no `-g`, no sanitizer | shipping |
  | `relwithdebinfo` | `-O2 -g -DNDEBUG`, no sanitizer, no LTO | bugs that only appear optimized |

  Sizes on this machine: **462 KB** release, 3.0 MB relwithdebinfo, 5.6 MB
  debug.

  This did not change what gets shipped. `OPT=1` — which the release
  workflow, the platform matrix, both install targets, the Docker build and
  `make bench` all pass — still means exactly `MODE=release`, byte for byte,
  and there is a test pinning that. What was missing was the middle
  configuration and any way to name the other two.

  Nothing is stripped after the fact. Release carries no debug information
  because release never compiles `-g` in, which is also why `strip` recovers
  only about a kilobyte from it: that last 807 bytes is libgcc's
  `crtfastmath.c`, pulled in by `-ffast-math` and shipped by the distro
  already compiled with `-g`. None of it is ours.

  If you have been reading a 5.6 MB `build/bin/hellish` as the shipped
  binary, it never was — that is the debug build, and it is large on purpose.

**Fixed**

- **`make user-install` now leaves `hellish` on your PATH**, not just on your
  disk. The installer put the binary in `$PREFIX/bin` (default
  `~/.local/bin`) and added an `exec` hook to your login rc — so the shell
  came up, and the gap was invisible from the outside. What was missing was
  the *name*:

      $ hellish update
      hellish: command not found

  on a machine that had just installed it. Nothing on either route ever put
  that directory on PATH, and the two reasons it looked covered both fail
  here: a user-install hellish is exec'd from an *interactive* rc, so it is
  not a login shell and reads neither `/etc/profile` nor `~/.profile`; and
  Debian/Ubuntu's `~/.profile` adds `~/.local/bin` only `if [ -d ]` — on a
  first install, this install is what creates that directory.

  The seeder now maintains one marker-delimited, case-guarded PATH block in
  `~/.hellishrc`, and the login-rc hook prepends the same directory before
  the `exec` so bash finds it too. Re-sourcing cannot stack duplicates,
  `--uninstall` removes only that block, and a hand-written `~/.hellishrc`
  is still never clobbered. `make my_shell`, which installs to `/usr/bin`,
  is unchanged.

- **Build modes no longer share an object tree.** The object directory keyed
  on `ifdef OPT`, which covered the `OPT=1` benchmark build and nothing else,
  so a tree filled by a debug build and then reused by an optimized one
  handed the linker ASan-instrumented objects under a link line carrying no
  `-fsanitize`:

      func_retire.o: undefined reference to `__asan_report_load4'

  `make re` hid this; a plain `make MODE=release` after a debug build did
  not. Objects now live in `build/obj-<mode>-<allocator>`, so the three modes
  and the two allocator backends coexist and none of them can poison another.

- Release notes and code comments now point at the issue the update fix
  addressed (#64). 2.7.5 referenced a number that had been handed to the pull
  request instead, so the trail led back to the fix rather than the report.

If you installed with `make user-install` and have been typing the full path
to reach the shell, this is the release that fixes it. Otherwise nothing in
the shell's own behaviour differs from 2.7.5.

---

## v2.7.5

**Fixed**

- **The shell notices a release published since its last check** (#64). The
  report was a screenshot:

      hellish 2.7.3 ...
      ✓ 2.7.3 up to date · via user binary · 50m ago

  2.7.4 was out. The shell had checked 50 minutes earlier, when 2.7.3 really
  was the newest thing there was — and the check interval was a flat 24
  hours, so it would not look again until the next day. Every session in
  between reported "up to date" with complete confidence, and the only way
  to find out was typing `update` by hand: the exact chore a background
  check exists to remove.

  The interval is now adaptive, because the two states are not the same
  question. When an update is **already known pending** there is nothing
  left to learn — the badge is on your prompt and the banner has said so —
  and it keeps the long interval. When the shell **believes it is current**,
  that is the only state in which a release can exist without it knowing, so
  it looks again every quarter of an hour instead.

  The asymmetry is what keeps it cheap: the frequent interval applies only
  while there is genuinely something to find, and stops the moment it is
  found. If you have an update pending, this changes nothing — still one
  request a day.

  Two guards came with it, because "check more often" must not become
  "check on every shell". A failed check now backs off like a successful
  one, instead of re-firing on every startup forever on a machine that
  cannot reach the release server. And the attempt is claimed before the
  fork, so twenty terminals opened at once make one request rather than
  twenty.

  Unchanged: the check is a detached child and the prompt never waits on the
  network. A dead release server still costs the shell nothing.

**Tests**

`update_freshness_test.py` drives real ptys against a counting local release
server — no network, so it is deterministic anywhere. It pins the report
itself, the discovering session announcing the release without a restart,
the badge on the next one, the absence of a re-check when an update is
already known, six concurrent shells making at most two requests, a failed
check recording the attempt but not claiming a success, and startup timing
against a black-holed endpoint.

---

## v2.7.4

Two bug reports from real sessions. One of them could leave your terminal
unusable, so if you are on 2.7.x, take this one.

**Fixed**

- **Leaving a shell that still holds a stopped job no longer wrecks the
  terminal** (#58). `top &` and then Ctrl-D, and the terminal you came back
  to had no echo and spliced your next command into garbage — sometimes.
  Four defects, stacked, each hiding the next:

  1. **Ctrl-D never asked.** The stopped-jobs guard existed and worked, but
     only the `exit` *builtin* called it. Both end-of-input paths set "time
     to leave" themselves, and every report came in through Ctrl-D.

  2. **The guard read a stale answer.** A job the kernel stops the moment it
     touches the terminal is only recorded when the shell next reaps — on a
     *later* prompt. So `top &` followed immediately by an exit saw a
     running job and let you out, while the same pair with any command in
     between saw a stopped one and warned. That is the entire "sometimes it
     works, sometimes it doesn't": a race, not a flake.

  3. **The warning never came back.** It was forgotten only when a builtin
     ran, so an external command in between left it standing: warn once, run
     `ps`, press Ctrl-D, and the shell walked out over a job it had already
     been told about. The warning now stands only while the previous thing
     you did was itself an attempt to leave — which is bash's rule.

  4. **And the damage itself, which survived all three.** On the exit you
     actually meant, the shell handed the terminal back and left *while the
     jobs it had just hung up were still dying*. A full-screen program does
     not die quietly: it repaints, restores its own idea of the terminal and
     prints a farewell. With a screenful of stopped `top`s that is dozens of
     processes writing to the terminal after the shell let go of it. The
     shell now hangs them up, **waits for them**, and only then puts the
     terminal back — and it restores the settings it started with, because a
     background job stopped halfway through raw mode never can.

  hellish keeps bash's semantics — the first exit is refused, the second is
  obeyed — and is deliberately stricter in one place: bash's `jobs` marks
  what it lists as already mentioned and then leaves without a word, so
  running `jobs` and pressing Ctrl-D silently abandons your stopped jobs.
  hellish warns anyway. bash can afford that leniency because it is usually
  the session leader and the kernel cleans up after it; a nested hellish is
  not, and a job left stopped there holds a raw terminal forever.

- **The shell tells you about a new release by itself** (#56). It did not.
  A new version stayed invisible until you typed `update`, and only then did
  the prompt badge appear. The banner's "X available — run `update --now`"
  line renders perfectly well and was simply unreachable: it asked a flag
  owned by the prompt's one-shot notice, and whichever spoke first silenced
  the other. The prompt always won. The banner now keeps its own record,
  announces each new release once, and leaves the standing reminder to the
  prompt badge. The check was also started *after* the banner that reports
  it, so the session that discovered a release was guaranteed to draw a
  stale panel; that order is swapped.

  Unchanged and now guarded by a test: the check is a detached child and the
  prompt never waits on the network. A dead release server costs the shell
  nothing.

- **`HELLISH_BANNER=0|1`** (#56). There was no such knob — only
  `HELLISH_NO_BANNER` and `HELLISH_ALWAYS_BANNER`, two names for the two
  ends of one tri-state, neither of them the one anybody guesses. Both old
  names still work.

**Tests**

Two new pty regression files, both discovered automatically by the suite:
`exit_stopped_jobs_test.py` (27 checks, bash as the oracle for the damaging
cases — the race, both exit paths, the warning re-arming, many stopped jobs,
raw-mode jobs, the terminal coming back usable, nothing left behind) and
`banner_update_test.py` (16 checks — the knob, the banner announcing a
release unprompted and only once per version, the badge persisting, and
startup timing against a black-holed endpoint).

---

## v2.7.3

A bug-fix release: two issues reported from real machines, both on the path
between installing hellish and getting a usable prompt. Nothing else changes.

**Fixed**

- **The prompt's process tracker is back** (#50). The built-in prompt shows
  a background-jobs badge — ` ⚙N` — on its info row, next to `took N.Ns`
  after a slow command. Both silently disappeared in 2.7.0 and neither was
  coming back on its own.

  The prompt keeps mirrors of the process state, because it is also
  repainted from a timer between commands with no shell state in hand. The
  refresh sat below an early return that only became reachable-from-nowhere
  when 2.7.0 gave interactive shells a default `PS1` of `\B` (so a Python
  virtualenv could restore it, #39). From that release every shell took the
  early return, the mirrors stayed frozen at zero, and both badges rendered
  as nothing at all. If you never set your own `PS1`, this is the release
  that gives them back.

- **`make my_shell` now creates `~/.hellishrc`** (#51). It never did. The
  seeding lived inside `user-install.sh`, so only the no-sudo route ever ran
  it, and anyone who installed with `make my_shell` met a shell with no
  config at all — no `EDITOR`, no aliases, no prompt theme — and no hint one
  was meant to exist. Both routes now call `tools/seed_hellishrc.sh`, which
  still never touches an rc you already have.

- **`shopt -o` stopped ignoring its arguments** (#51). `-o` selects the
  `set -o` roster; it was being parsed as an *action*, so the names, the
  `-q` and the `-p` were all discarded. Ubuntu's stock `~/.bashrc` asks

      if ! shopt -oq posix; then ... fi

  which dumped all 27 option lines onto the screen at every login **and**
  returned success regardless of the setting — so the branch was decided
  wrongly, not merely loudly. `shopt -o NAME`, `-op`, `-oq`, `-so` and
  `-uo` now match bash byte for byte, including its distinct
  "invalid option name" rejection.

- **`shopt -q progcomp` no longer errors on every login** (#51).
  `/etc/profile.d/bash_completion.sh` probes it, and hellish did not know
  the name, so each login opened with
  `hellish: shopt: progcomp: invalid shell option name`. It is now a known
  option that is **off** — hellish has no `complete` builtin, so there is no
  programmable completion to enable, and reporting it honestly is also what
  makes bash-completion correctly skip itself instead of feeding hellish a
  2500-line bash script.

**Changed**

- **`hellishrc.example` no longer edits `PATH`.** It shipped an active
  `case ":$PATH:" ... esac` prepend of `~/.local/bin` that the login chain
  had already done moments earlier — a login hellish sources `/etc/profile`
  and then `~/.profile`, and on Debian/Ubuntu `~/.profile` is exactly what
  adds `~/.local/bin` and `~/bin`. Redundant, and it read as the pattern you
  were meant to copy. It is now a commented example with the reasoning
  attached. Your own `~/.hellishrc` is never rewritten, so nothing changes
  under you; this only affects a freshly seeded one.

**Tests**

Four new regression files, all picked up automatically by the pty suite:
`prompt_jobs_badge_test.py` (the badge on all three prompt routes),
`shopt_setopt_test.py` (every `-o` form against bash),
`login_chain_test.py` (a real login against Ubuntu's stock dotfiles, in a
sandbox so CI runners without those files agree), and
`hellishrc_seed_test.py` (seeds when absent, never clobbers, and both
install routes still call the seeder).

---

## v2.7.2

Everything since v2.7.1, in one release. The bulk of it is portability:
hellish now builds and runs on macOS (Apple Silicon) and inside WSL, both
of which are covered by CI. On Linux nothing about the shell's behaviour
changes.

It also carries the `pretty` builtin and `shopt -s lithist`, which were
finished earlier and never shipped on their own — see below.

**Fixed**

- **macOS (Apple Silicon) builds again.** Four separate defects, each
  hidden behind the last: `st_mtim` is `st_mtimespec` on Darwin, `SIGPWR`
  and the realtime signals do not exist there, `bcopy` is a fortified
  macro, and `MB_CUR_MAX` is an `int` rather than a `size_t` — which made a
  correct-on-Linux comparison a `-Werror=sign-compare` failure.

- **A library function that was declared, called, and never defined.**
  `get_original_tty_job_signals()` had been missing a body for months.
  GNU ld lets a shared library keep undefined symbols and hope; Apple's
  does not, which is the only reason anyone found out. It is a real bug
  everywhere: a crash waiting for the first caller.

- **The allocator-backend probe no longer relies on ELF semantics.** An
  undefined *weak* symbol resolves to NULL on Linux and is a link error on
  macOS. Which heap is being built is now a compile-time fact, which is
  what it always was.

- **Process substitution works off Linux.** `<(cmd)` and `>(cmd)` re-exec
  the shell, and did it through the literal `/proc/self/exe` — a path that
  exists on Linux and nowhere else, so on macOS both produced nothing at
  all. Now resolved at runtime (`_NSGetExecutablePath` on Darwin). The
  same assumption was in three more places: the ENOEXEC script-interpreter
  fallback, and the update machinery's idea of where it lives — which had
  been classifying every non-Linux install as a downloaded binary, so
  `update` would have offered to overwrite a source checkout.

- **Windows checkouts no longer break the scripts.** `.gitattributes`
  pins everything with a shebang to LF, so `core.autocrlf` cannot turn
  `set -u` into `set -u\r`.

**Testing**

- Three new gates reproduce the macOS and Windows failures **on Linux**, in
  about a second each. `link_closure_test.py` asks GNU ld the question
  Apple's linker asks by default. `crlf_hygiene_test.py` checks both that
  no executed file is stored with CRLF and that every one of them is
  covered by a rule — the second being the half that catches the next file
  someone adds. `linux_only_apis_test.py` asserts that `/proc/self/exe`
  is named in exactly one file, which is the property that actually broke:
  the knowledge had been copied into four, so porting meant finding all
  four.

- The WSL rung builds on the distro's own filesystem rather than through
  the Windows bridge, where the same build could not finish inside an hour.

### Also here, from the issue-fixing work before it

**Added**

- **`pretty` — named presets for how the shell feels.** The multi-line
  history recall people kept asking for was one `shopt` bit, `lithist`.
  That is fine if you already know the name; `shopt` lists eleven options
  with no hint which are cosmetic and which one you actually wanted.

  ```
  pretty                 what is on right now
  pretty -p              the same, as ~/.hellishrc lines
  pretty list            every feature and mode, with descriptions
  pretty on|off NAME...  toggle features
  pretty mode NAME       plain | friendly | full
  ```

  Every feature *is* a `SHOPT_*` bit rather than a copy of one, so `pretty`
  and `shopt` can never disagree. `pretty -p` prints lines that reproduce
  your configuration when pasted into `~/.hellishrc` — a setup you can copy
  between machines instead of remember. Defaults are unchanged
  (bash-identical); `pretty mode friendly` is the line that turns on
  multi-line recall.

- **`shopt -s lithist`.** Recall keeps a compound's newlines and
  indentation instead of joining it onto one line.

**Fixed**

- **`history` parsed its options instead of printing everything.**
  `ft_atoi("-a")` is `0`, a count of `0` meant "no count given", and no
  count meant *print the whole list* — so every option word dumped your
  history. With `PROMPT_COMMAND='history -a'` inherited from the
  environment (a stock bashrc line), that fired before every prompt with
  nothing typed. All of bash's options work now; an unknown option and a
  non-numeric count are both status 2.

- **A recalled function definition stopped defining a function.** `f()`
  followed by a newline joined to `f(); { … }`, which is a syntax error.
  The joiner knew a `;` is illegal after `then`/`do`/`in` and after a case
  pattern, but not after a function header.

- **`lithist` skipped the history scanner instead of changing it**, so it
  also skipped the top-level `\`-newline splice that bash performs in both
  modes.

- **TAB in command position offers commands, not documents.** The PATH scan
  matched on the directory entry name alone, so every `readdir()` result
  was offered — data files, subdirectories, and `.`/`..` from every PATH
  element. POSIX (XCU 2.9.1.1) uses a PATH prefix only when it names an
  *executable file*. Also: the word after `;` `&&` `||` `|` `&`, inside
  `$( )`, or after leading blanks is a command word; and the builtin list
  offered before the scan held 18 of 52 names.

- **The git dirty star no longer outlives the tree it describes.** A
  `git status` that took a second armed a 30-second cache, and the prompt
  then asserted "dirty" for half a minute after a `git checkout` in the
  same shell had made it clean. A command running in the tree now retires
  the cached answer whatever the throttle says; idle prompts still hit the
  cache, so git is not re-run for nothing.

- **An empty job count is no longer a fork bomb.** `nproc` succeeding with
  empty output — which a sandboxed or cgroup-restricted one can do — left
  `-j` with no argument, meaning *unlimited* jobs: one compiler per source
  file. Guarded in pure make, since a tool that might be missing cannot be
  what guards against a missing tool.

- **The build works on distros that ship no `find`.** openSUSE Tumbleweed
  is one. Sources are discovered with `find`, so the source list came back
  empty and the link died on `cc: fatal error: no input files` — an error
  nowhere near its cause. It now refuses to run with an empty source list
  and says why.

- **Non-x86 builds compile again.** `libft.h` included `<immintrin.h>`
  unconditionally — an x86-only header — so every arm64 and Apple Silicon
  build died at the first source file.

**Testing & CI**

- **A `Platforms` matrix.** 14 distro/compiler rungs — Ubuntu 22.04/24.04,
  Debian, Alpine (musl), Arch, Fedora, Rocky, openSUSE, Void; gcc and
  clang; plus a musl + `ft_malloc` rung — each running a 40-check
  portability workout, plus a native arm64 runner and informational macOS
  and WSL jobs. Two of the bugs above were found by it within the hour.

- **Regression tests run by discovery.** Every `tests/*.py` was wired in by
  hand, twice, and the list had drifted: one test sat in `tests/` with no
  target and no CI job, guarding nothing. `make pty-test` globs the
  directory instead, so a new test is covered the moment it is written.

---

## v2.7.1

**Fixed**

- **`printf` renders the full unsigned range.** `%u`, `%o`, `%x` and `%X`
  went through the signed parser, so every value above `LLONG_MAX`
  saturated before the conversion ever ran:

  ```
  printf "%u\n" 18446744073709551615
    bash     18446744073709551615
    hellish   9223372036854775807     (before)
  ```

  The printed spec was never at fault — the value handed to it had already
  been clamped. `printf %u -1` gives `18446744073709551615` like bash, and
  genuine overflow still exits 1 while emitting the clamped prefix.

---

## v2.7.0 — *the it-tells-you release*

**New**

- **A pending update stays visible in the prompt.** The "update available"
  notice is said once and then never again — deliberately, because a banner
  on every prompt is how people learn to stop reading banners. But miss it
  once and nothing brought it back. Now a quiet badge persists for as long
  as the update is actually waiting:

  ```
  ╭─ you in ~/project ⬆2.7.1 ──────────────────── 15:04 ─╮
  ╰─❯
  ```

  Self-spacing (invisible when there is nothing to say), dropped on a narrow
  row like the other badges, gone by itself once you update, and silenced by
  `HELLISH_NO_UPDATE_CHECK`. Custom prompts get `\U`.

**Fixed**

- **Releases stop reporting themselves as failed.** Every tag showed
  `release -> failure` while shipping perfectly good artifacts: the Docker
  Hub push (a secondary channel — GHCR publishes the same image) was failing
  on an expired token and taking the whole run's status with it. A
  permanently red release run makes the next genuine failure invisible.

---

## v2.6.0 — *the ask-it-what-it-is release*

**New**

- **`hellish --version`.** It used to answer "invalid option". Now it prints
  the version, the asset the updater will fetch, and the repo it will fetch
  it from — because a build pointing somewhere unexpected is worth seeing
  *before* it downloads anything. Exits 0 without sourcing a startup file or
  reading stdin, so package managers and CI can probe it safely.

**Fixed**

- **`printf` accepts length modifiers.** `printf "%ld\n" 9999999999` failed
  with `` `%l': invalid format character ``. Every C-habit format string hit
  this: `%ld %lld %zu %hd %jd %td %Lf`. bash accepts and ignores them; so do
  we now, measured against it rather than guessed.
- **No more `git <defunct>`.** The prompt's async git check was reaped only
  during a prompt render, so any scan finishing while a foreground command
  ran left a zombie for that command's whole life — one per nested shell.
  It is double-forked onto init now; there is no child to reap.
- **`top &` works, and `^Z` no longer wedges the terminal.** Background jobs
  keep the tty in an interactive shell (POSIX only redirects stdin to
  `/dev/null` when job control is *off*), and the foreground wait passes
  `WUNTRACED` — without it a stopped child never satisfied the wait, so the
  shell blocked in `waitpid` forever while the kernel echoed keystrokes that
  nothing ran.
- **Whole prompt frames.** The prompt writers used a single unchecked
  `write()`. `write(2)` may transfer fewer bytes than asked and report that
  as success, so the tail was silently dropped.

**Changed**

- **The prompt animation ships off.** It was the only thing that wrote to
  your terminal while you were not typing — 6.4KB of escape traffic every
  2.5 seconds at an idle prompt. Set `HELLISH_ANIM=spinner|pulse|ember` to
  opt back in. Nothing else about prompt customisation changes.
- `HELLISH_ANIM` now means what it says. It only ever governed `\A` in a
  custom `PS1`; the built-in prompt animated regardless, with no way to stop
  it short of writing a whole custom prompt.

**Under the hood**

- CI runs the suites that previously only ran when somebody remembered:
  the pty gates, startup argv parsing, login file order, the help table,
  the update path, the whole-program corpus, allocator parity across both
  heaps, and benchmarks (published, never a gate).
- `vendor/libft` gained `%ld`-family support and the 64-bit correctness
  fixes that exposed, and its own CI is green across ubuntu 22.04/24.04 ×
  gcc/clang for the first time.

---

## v2.5.0 — *the help release*

**New**

- **`help`.** `help` lists what the shell can do, grouped by what you want
  to do with it rather than alphabetically; `help NAME` explains one thing;
  `help -s NAME` prints just the form.
- **Syntax topics.** `help for`, `help case`, `help function`,
  `help redirection`, `help pipeline`, `help $((` — because knowing that
  `for` is a keyword does not tell you how to write one.

Its exit status matches bash exactly (0 when at least one topic matched),
and a test derives the expected topic list from the builtin dispatch table,
so a builtin added without a help entry fails the build rather than
quietly going undocumented.

---

## v2.4.1

**Fixed**

- The session that had just installed an update immediately re-announced
  it. True — replacing the file cannot change the process already
  executing, so the running shell is still the old build — but it read as
  if the install had failed. Installing now marks the version as
  announced, and the shell just waits to be restarted, as it already
  said it would.

---

## v2.4.0 — *the update release*

**New**

- **`update` actually works.** The updater had been pointed at
  `Univers42/42sh` — a repository that does not exist — for its whole
  life, so every check 404'd and reported itself as "could not reach
  GitHub (offline?)". It now names the real repository, and so do
  `install.sh`, the npm installer and the Dockerfile.
- **An update button.** A background check (detached, 24h TTL, never on
  the startup path) discovers new releases. The next prompt carries
  `[Update] / [Later]`; `update --now` installs. The notice is printed
  between commands, never into a line you are typing.
- **Verified, atomic installs.** Download → sha256 against the checksum
  published beside the asset → run the binary and require it to report the
  version it was advertised as → `rename(2)` into place. Any failure
  leaves the installed binary untouched. Releases now ship a `.sha256`.
- **No-sudo by default.** A user-local install (`~/.local/bin`) updates
  with no elevation at all; a system-wide one asks first and says exactly
  which command will run as root. Package-managed installs (npm, pnpm,
  docker) still delegate to their package manager.
- **The banner is lazy.** It appears once a day, or when it has something
  new to say (new version, new header revision, an unannounced update) —
  not on every single shell. It also stopped wiping your screen and
  scrollback on startup.
- **`/dev/tcp` and `/dev/udp`** redirections, bash-style.

**Fixed** — a long list this cycle, all diffed against bash 5.3.9:

- `"${u:-"a b"}"` used to **crash** the shell; the whole `${...}` operator
  family mishandled nested quotes, escapes and single quotes.
- `exec 4>a 2>b` pointed fd 2 at the wrong file and closed fd 4.
- Globs sorted in ASCII order instead of locale collation, so `echo *`
  disagreed with bash in any mixed-case directory.
- A failing POSIX special builtin now aborts a non-interactive shell.
- `readonly` reported success on every error; `unset` removed read-only
  variables.
- A fatal error inside a subshell exited 127 instead of 1.
- The prompt is written in one syscall, so type-ahead can no longer be
  echoed into the middle of a colour escape (the `38;2;112` garbage).
- `history` shows multi-line commands the way bash does.
- Background job labels keep their opening `(`, `{`, `for`, `if`.

---

## v2.2.0 — *the friendly release*

**New**

- **A welcome panel.** A full‑width, two‑column box: a greeting
  and the **42 logo in salmon** on the left; getting‑started tips and a
  "What's new" block on the right. Shown when it has something new to say —
  a new day, a new release waiting, or a hellish you have just upgraded.
  (`HELLISH_BANNER=0` to silence, `HELLISH_BANNER=1` to force.)
- **A one‑time entrance animation.** On the very first run the logo draws in,
  row by row, like the GitHub Copilot CLI banner; every later startup is
  instant. (`HELLISH_ANIM=1` to replay it, `HELLISH_NO_ANIM=1` to skip.)
- **Origin‑aware `update`.** The shell now knows *how it was installed* and
  upgrades the right way:
  | installed via | `update --now` runs |
  |---|---|
  | npm | `npm install -g hellish-shell@latest` |
  | pnpm | `pnpm add -g hellish-shell@latest` |
  | Docker | tells you to `docker pull dlesieur/hellish-shell:latest` |
  | source checkout | `git pull && make OPT=1 all` in your clone |
  | standalone binary | re‑runs the install script |

## v2.1.0

- Welcome banner, `HELLISH_VERSION`, and the `update` builtin.
- Once‑a‑day background check for new releases (never blocks the prompt).
- Packaging: install script, Docker image, and the `hellish-shell` npm package.

## v2.0.0

- The 42sh milestone: lexer → parser → expander → executor, jobs, heredocs,
  arithmetic, globbing, history, and a large POSIX‑conformance pass.

---

## How to use it

**Run it:** `hellish` (or set it as your login shell — see Install in the
[README](README.md)).

**Everyday builtins:** `cd`, `pwd`, `export`, `unset`, `alias`, `jobs`, `fg`,
`bg`, `history`, `type`, `command`, `test`/`[`, `read`, `printf`, `umask`,
`ulimit`, `trap`, `getopts`, `set`, `local`, `return`, `exit`. Run `type <name>`
to see how any name resolves.

**Update yourself:**

```sh
update            # check GitHub for a newer release
update --now      # upgrade the way this copy was installed
update --version  # print the running version
```

**Config:** drop a `~/.hellishrc` (aliases, exports, functions, `set` options) —
it's sourced on interactive startup, the `.bashrc` analogue. A starter lives in
[hellishrc.example](hellishrc.example).

**Knobs:**

| variable | effect |
|---|---|
| `HELLISH_BANNER=0` / `=1` | force the welcome panel off / on |
| `HELLISH_NO_BANNER=1` | hide the welcome panel entirely (older name for `=0`) |
| `HELLISH_ALWAYS_BANNER=1` | draw it every startup (older name for `=1`) |
| `HELLISH_ANIM=1` | replay the entrance animation on this startup |
| `HELLISH_NO_ANIM=1` | never play the entrance animation |
| `HELLISH_NO_UPDATE_CHECK=1` | never check for updates in the background |

---
