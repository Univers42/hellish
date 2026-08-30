# Prompt themes

Themes are data, not code. Each is one `.hsh` file that sets `PS1` (or
`PROMPT`, the zsh spelling), and `prompt <name>` sources it.

    prompt              list themes, marking the active one
    prompt <name>       switch now
    prompt preview      render every theme, one line each
    prompt save <name>  make it the default for new sessions
    prompt edit <name>  open it in $EDITOR

Installed to `$XDG_CONFIG_HOME/hellish/themes/` by `tools/seed_hellishrc.sh`.
Yours once they are there: the seeder never overwrites a file it did not
just create, so edits survive an upgrade.

## Why these themes

They exist to **exercise the renderer**, so they are chosen by edge case
rather than by looks. Each group targets something that has broken, or that
could break silently:

| theme | what it stresses |
|---|---|
| `plain` `classic` `minimal` | no colour, no escapes -- the control |
| `powerline` `ember` `pure` | 24-bit colour, reverse video, `\[ \]` width guards |
| `tworow` `rightaligned` | multi-line prompts and column arithmetic |
| `wide` `emoji` `cjk` | `wcwidth`, combining marks, double-width cells |
| `githeavy` `jobs` `duration` `update` | the `\g \S \p \J \U` badges and their self-spacing |
| `titled` | OSC `\e]0;…\a` -- **known-broken width accounting** (#72) |
| `zshpure` `zshpowerline` | the `%` frontend, proving both syntaxes agree |
| `cmdsub` `arith` `paramop` | `$(…)`, `$((…))`, `${x:-y}` inside a prompt |
| `long256` `empty` `quoteheavy` | boundary cases: overflow, nothing, nested quotes |
| `octal` `escapes` | `\nnn` and the literal-passthrough rule |

`tests/prompt_themes_test.py` asserts every one of them: it renders without
error, its visible width matches a computed expectation, the cursor lands in
the right column, no ANSI escape leaks past the prompt, and switching between
any two leaves no residue.

`titled` is expected to get its width **wrong** today. That is the OSC bug in
issue #72, and the test pins it as a known divergence rather than hiding it --
so the day it is fixed, the test fails and says so.

## Writing your own

Single quotes matter. In double quotes `$?` and friends expand ONCE, when the
file is sourced, and are frozen for the rest of the session.

Wrap every escape sequence in `\[ \]`. Those mark zero-width regions; without
them the line editor counts the colour codes as visible columns and wraps in
the wrong place as soon as you type past the width of the terminal.

    PS1='\[\e[32m\]\u\[\e[0m\] \w \$ '
    #    ^^^^      ^^^^^^^^^^ guards, not decoration
