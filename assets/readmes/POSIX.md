# POSIX-friendly Shell Rules — a friendly, practical guide

*A tidy, developer-focused rewrite of “Bash and POSIX” — designed to inspire rules you can follow when building your own shell.*


<!-- Paste this at the top of a rendered Markdown viewer that allows HTML/CSS -->
<style>
:root{
  --radius:12px;
  --mono: Menlo, Monaco, "Courier New", monospace;
  /* Light theme */
  --bg-light: #f7fafc; --card-light: #ffffff; --muted-light:#475569; --accent-light:#0ea5a4; --text-light:#0f1724;
  /* Dark theme */
  --bg-dark:#071026; --card-dark:#071827; --muted-dark:#9aa4b2; --accent-dark:#6ee7b7; --text-dark:#e6eef6;
}
/* Base layout */
body{background:var(--bg-dark); color:var(--text-dark); font-family:Inter,system-ui,Segoe UI,Roboto,Helvetica,Arial,sans-serif; padding:28px;}
.container{max-width:1000px;margin:0 auto;}
.header{display:flex;gap:16px;align-items:center;}
.badge{background:var(--accent-dark);color:#07211b;padding:8px 12px;border-radius:999px;font-weight:700;}
.card{background:linear-gradient(180deg, rgba(255,255,255,0.02), rgba(255,255,255,0.01)); border-radius:var(--radius); padding:18px; box-shadow:0 6px 24px rgba(2,6,23,0.6); margin:18px 0;}
h1{margin:0;font-size:32px}
h2{color:var(--accent-dark); margin-top:22px;}
h3{color:#bfeadf;margin-top:14px}
p,li{color:var(--muted-dark);line-height:1.55}
pre{background:#021826;padding:14px;border-radius:10px;color:#cfeef0;overflow:auto;font-family:var(--mono);font-size:13px}
.kv{display:flex;gap:8px;align-items:center}
.rule{background:linear-gradient(180deg, rgba(255,255,255,0.01), rgba(0,0,0,0.02));border-radius:10px;padding:12px;margin:12px 0;border-left:6px solid var(--accent-dark)}
.chips{display:flex;gap:8px;flex-wrap:wrap}
.chip{background:#072935;padding:6px 10px;border-radius:999px;color:var(--accent-dark);font-weight:600}
.small{font-size:13px;color:#94a3b8}
.cheat{background:linear-gradient(90deg,#04131b,#07212a);padding:12px;border-radius:12px;border:1px solid rgba(255,255,255,0.02)}
a{color:var(--accent-dark)}

/* Light theme override */
@media (prefers-color-scheme: light) {
  body{background:var(--bg-light); color:var(--text-light)}
  .card{background:var(--card-light); box-shadow:0 6px 18px rgba(15,23,36,0.06)}
  pre{background:#f3f7f9;color:#0f1724}
  .rule{border-left-color:var(--accent-light)}
  a{color:var(--accent-light)}
  .small{color:var(--muted-light)}
}
</style>


## Table of contents

1. [Purpose & philosophy](#toc-1)
2. [Quick summary (what POSIX expects)](#toc-2)
3. [Startup & environment rules](#toc-3)
4. [Parsing, quoting, and expansion rules](#toc-4)
5. [Redirections, globbing, and word splitting](#toc-5)
6. [Builtins, special builtins, and functions](#toc-6)
7. [Job control, signals, and interactive behavior](#toc-7)
8. [Error handling and exit rules for non-interactive shells](#toc-8)
9. [History, editing, and prompt behavior](#toc-9)
10. [Command lookup and hashing rules](#toc-10)
11. [Compatibility gotchas (multibyte, editor defaults, echo, etc.)](#toc-11)
12. [Cheat sheet — compact rule list](#toc-12)
13. [Examples & small snippets](#toc-13)

---

# 1 — Purpose & philosophy

Keep behavior predictable and strictly defined. POSIX is about well-specified, minimal, composable behaviour: if you follow small, unambiguous rules for parsing, expansion, redirection, exit statuses, and builtins, other programs and scripts will behave reliably with your shell.

Design goal for your shell: **implement the POSIX shell semantics by default (or provide a strict POSIX mode)**, and document exceptions clearly.

---

# 2 — Quick summary (what POSIX expects)

* A standard command language for scripts and interactive use: variables, expansion, quoting, control structures, redirections, pipelines, and utilities.
* Clear rules for when the shell exits on errors in non-interactive mode.
* Defined behavior for special builtins (commands that can affect the shell state).
* Defined interactive behaviors: job control, command-line editing (POSIX mandates vi-style), history behavior, prompts, and signals.

---

# 3 — Startup and environment rules

.rule

<div class="kv"><strong>Rule 3.1 — POSIX mode switch</strong><span class="small">: support a strict POSIX mode toggle</span></div>
<div class="small">Provide `--posix` on the command line and a `set -o posix` runtime switch. When enabled, set `POSIXLY_CORRECT` and adjust behavior as below.</div>

.rule

<div class="kv"><strong>Rule 3.2 — Startup files</strong><span class="small">: read `$ENV` when POSIX mode or `sh` invocation</span></div>
<div class="small">When in POSIX mode (or invoked as `sh`), read POSIX startup files as specified. For non-POSIX default shells, keep your usual startup files but document precedence.</div>

.rule

<div class="kv"><strong>Rule 3.3 — Alias expansion</strong><span class="small">: enable aliases even in non-interactive shells (POSIX mode)</span></div>
<div class="small">In POSIX mode, expand aliases during parsing where POSIX requires it.</div>

---

# 4 — Parsing, quoting, and expansion rules

.rule

<div class="kv"><strong>Rule 4.1 — Quoting precedence</strong><span class="small">: follow POSIX ordering for quotes and expansions</span></div>
<div class="small">
- Double quotes allow parameter and command expansion but prevent word splitting and pathname expansion.  
- Single quotes prevent all expansions inside (except where POSIX specifies pattern removal inside `${...}` for certain operators).  
- Inside `$(...)` or backquotes used for command substitution, treat `"` specially per POSIX (backslashes before `"` may be removed in here-doc ctxs).
</div>

.rule

<div class="kv"><strong>Rule 4.2 — Parameter expansion semantics</strong><span class="small">: honor POSIX parameter expansion rules and error semantics</span></div>
<div class="small">
- Implement `${var}`, `${var:-word}`, `${var:+word}`, `${var%pattern}`, `${var#pattern}`, etc.  
- If expansion encounters an invalid arithmetic or parameter expression in a non-interactive shell, treat it as a fatal error (shell exits).
</div>

.rule

<div class="kv"><strong>Rule 4.3 — Command substitution</strong><span class="small">: do not set `?` from command substitutions</span></div>
<div class="small">
- Command substitutions do not modify `$?`. The exit status of a simple command without a command word is the last command substitution's status only after assignments and redirections are evaluated.
</div>

.rule

<div class="kv"><strong>Rule 4.4 — Reserved words & aliasing</strong><span class="small">: reserved words never undergo alias expansion when recognized as reserved</span></div>
<div class="small">If a token is a reserved word in parsing position, do not expand aliases on it.</div>

---

# 5 — Redirections, globbing, and word splitting

.rule

<div class="kv"><strong>Rule 5.1 — Redirection words: no globbing or splitting (non-interactive)</strong><span class="small">: filenames in redirections are literal unless interactive</span></div>
<div class="small">Except for interactive shells, words in redirection position must not be subject to filename expansion or word splitting. (Interactive shells may expand.)</div>

.rule

<div class="kv"><strong>Rule 5.2 — Word splitting & IFS</strong><span class="small">: be careful with multibyte IFS</span></div>
<div class="small">POSIX expects word splitting to operate on bytes in `IFS`. Many modern shells treat multibyte characters as single delimiters; document your choice. If you deviate, clearly state it; consider providing a compatibility mode for byte-oriented splitting.</div>

.rule

<div class="kv"><strong>Rule 5.3 — Tilde expansion</strong><span class="small">: only expand tilde for assignments before command name</span></div>
<div class="small">Perform `~` expansion on assignments preceding a command name (POSIX behavior), not on arbitrary assignment statements later on the line.</div>

.rule

<div class="kv"><strong>Rule 5.4 — Globbing/filename expansion</strong><span class="small">: standard glob semantics</span></div>
<div class="small">Implement `* ? [..]` with usual semantics; when no match is found, decide whether to leave literal or return non-zero — document behavior (POSIX defines ctx-sensitive expectations).</div>

---

# 6 — Builtins, special builtins, and functions

.rule

<div class="kv"><strong>Rule 6.1 — Special builtins order & effect</strong><span class="small">: choose builtin precedence carefully</span></div>
<div class="small">POSIX special builtins (like `exit`, `readonly`, `export`, `set`, `unset`, `.`) may affect the shell environment or cause an exit. When looking up a command, prefer POSIX special builtins before functions. Also, errors from special builtins in non-interactive mode can cause the shell to exit.</div>

.rule

<div class="kv"><strong>Rule 6.2 — Function names & slashes</strong><span class="small">: disallow function names with slashes in POSIX mode</span></div>
<div class="small">When POSIX mode is enabled, do not execute a function whose name contains a `/` even if defined earlier.</div>

.rule

<div class="kv"><strong>Rule 6.3 — `export` / `readonly` formatting</strong><span class="small">: follow POSIX output formats</span></div>
<div class="small">Make `export` and `readonly` display precisely as POSIX expects (use `-p` for portable output where required).</div>

.rule

<div class="kv"><strong>Rule 6.4 — `read` and `test` builtins</strong><span class="small">: implement as builtins where performance/semantics require</span></div>
<div class="small">`read` may be interrupted by signals with traps set; `test` should use locale for `<` and `>` string comparisons.</div>

---

# 7 — Job control, signals, and interactive behaviour

.rule

<div class="kv"><strong>Rule 7.1 — Job notifications</strong><span class="small">: interactive vs non-interactive differences</span></div>
<div class="small">If interactive: postpone job-change notifications until the next prompt (do not print between commands separated by `;` or newline); non-interactive shells may print after a foreground job completes.</div>

.rule

<div class="kv"><strong>Rule 7.2 — Job table lifetime</strong><span class="small">: remove jobs after notification, but keep status for `wait` as needed</span></div>
<div class="small">Remove jobs from the displayed jobs list after notifying the user; keep status retrievable by `wait PID` so scripts can learn an exited job's status.</div>

.rule

<div class="kv"><strong>Rule 7.3 — SIGCHLD/trap interaction</strong><span class="small">: do not let SIGCHLD interrupt `wait` unexpectedly</span></div>
<div class="small">If a trap is set on `SIGCHLD`, arrival of the signal should not interrupt a pending `wait` builtin and cause it to return prematurely; instead, run trap handlers for each child when appropriate.</div>

.rule

<div class="kv"><strong>Rule 7.4 — Job exit/stopped messages</strong><span class="small">: consistent, POSIX-specified textual format</span></div>
<div class="small">Use the specified formats such as `Done(status)` for exit status and `Stopped(SIGNAME)` for stops, where POSIX requires them.</div>

---

# 8 — Error handling & exit rules (non-interactive)

.rule

<div class="kv"><strong>Rule 8.1 — Fatal errors cause exit</strong><span class="small">: follow POSIX for certain errors</span></div>
<div class="small">In non-interactive shells, treat specific fatal conditions as exiting the shell: invalid options to special builtins, redirection errors, assignment errors preceding a command, syntax errors in sourced files, invalid arithmetic or parameter expansion, etc. Maintain a clear list of fatal vs. non-fatal errors in documentation.</div>

.rule

<div class="kv"><strong>Rule 8.2 — Assignment errors</strong><span class="small">: assignment errors before a command can cause exit</span></div>
<div class="small">If an assignment preceding a command is invalid (e.g., assigning to readonly), treat as a fatal error in non-interactive mode; otherwise, abort the command but continue at top level, per POSIX details.</div>

.rule

<div class="kv"><strong>Rule 8.3 — `inherit_errexit` semantics</strong><span class="small">: subshell `-e` propagation in POSIX mode</span></div>
<div class="small">POSIX mode should make subshells inherit the `-e` (errexit) behavior so that failures in command substitutions respect the parent `-e` setting.</div>

---

# 9 — History, editing, and prompts

.rule

<div class="kv"><strong>Rule 9.1 — Line editing</strong><span class="small">: implement vi-style editing by default for POSIX compliance</span></div>
<div class="small">POSIX requires vi-style command-line editing commands; emacs-style was not standardized — you may offer both, but vi must be present in POSIX mode.</div>

.rule

<div class="kv"><strong>Rule 9.2 — History file and expansion</strong><span class="small">: default POSIX history file and `!` handling</span></div>
<div class="small">Set default history file to `~/.sh_history` in POSIX mode. The `!` history expansion character should not introduce expansion inside double-quoted strings when `histexpand` is on.</div>

.rule

<div class="kv"><strong>Rule 9.3 — PS1/PS2 prompt expansions</strong><span class="small">: expand `!` to history numbers & always perform parameter expansion</span></div>
<div class="small">Support POSIX prompt expansions such as history number `!` and `!!`, and always perform parameter expansion on `PS1` and `PS2` values regardless of promptvars option when POSIX mode is on.</div>

---

# 10 — Command lookup, hashing, and PATH behavior

.rule

<div class="kv"><strong>Rule 10.1 — Lookup precedence</strong><span class="small">: special builtins → builtins → functions → PATH → re-search on missing</span></div>
<div class="small">POSIX expects special builtins to be found before functions. If a hashed command no longer exists, re-search `$PATH` to locate the (possibly moved) executable.</div>

.rule

<div class="kv"><strong>Rule 10.2 — Hash table & executable bit</strong><span class="small">: do not hash non-executable files into table</span></div>
<div class="small">Do not insert files that lack the execute bit into the command hash table, even if `PATH` search finds them as a last resort; if such a file is the only match, you may still try to execute it but do not record it in the hash table.</div>

---

# 11 — Compatibility gotchas & special cases

.rule

<div class="kv"><strong>Rule 11.1 — Multibyte `IFS` behavior</strong><span class="small">: document and expose a compatibility toggle</span></div>
<div class="small">POSIX expects IFS splitting to be byte-based. Modern environments use UTF-8. If you treat a multibyte IFS character as a single delimiter (practical for UTF-8), provide clear docs and (optionally) a compatibility mode for strict byte-splitting.</div>

.rule

<div class="kv"><strong>Rule 11.2 — `echo` behavior</strong><span class="small">: provide `xpg_echo` or `echo -e` semantics toggle</span></div>
<div class="small">POSIX leaves some `echo` behaviors unspecified. Offer an option (e.g., `xpg_echo`) to control whether `echo` interprets backslash escapes; document default precisely.</div>

.rule

<div class="kv"><strong>Rule 11.3 — `fc` editor fallback</strong><span class="small">: POSIX expects `ed` fallback, but shells often use `$EDITOR`</span></div>
<div class="small">For `fc`: if `FCEDIT` is unset, POSIX expects to default to `ed`. Many shells check `$EDITOR` first — choose behaviour and document; consider following POSIX but respecting `$EDITOR` when compatibility mode is off.</div>

---

# 12 — Cheat sheet — compact rule list

<div class="cheat">
<ul>
<li><strong>POSIX mode toggle</strong> → `--posix` / `set -o posix` (set `POSIXLY_CORRECT`).</li>
<li><strong>Startup files</strong> → read `$ENV` in POSIX mode/`sh` invocation.</li>
<li><strong>Aliases</strong> → expand in POSIX mode even non-interactive.</li>
<li><strong>Reserved words</strong> → do not do alias expansion when recognized.</li>
<li><strong>Redirections</strong> → no globbing/word-splitting on redir targets in non-interactive shells.</li>
<li><strong>IFS splitting</strong> → POSIX byte-splitting vs UTF-8 — choose & document.</li>
<li><strong>Command substitution</strong> → does not set `$?` itself; careful with `-e` inheritance.</li>
<li><strong>Special builtins</strong> → may cause shell exit on errors (non-interactive).</li>
<li><strong>Functions</strong> → function names with `/` disallowed in POSIX mode.</li>
<li><strong>Job control</strong> → different notification semantics interactive vs non-interactive.</li>
<li><strong>History</strong> → `~/.sh_history` default in POSIX mode; `!` not expanded inside double-quotes.</li>
</ul>
</div>

---

# 13 — Examples & snippets

### Example: enable POSIX mode

```sh
# command-line
yoursh --posix

# runtime
set -o posix
```

### Example: redirection should be treated literally (non-interactive)

```sh
# In non-interactive POSIX mode, this should not perform pathname expansion on the filename:
cmd > "$SOME_VAR"
# ensure you do not perform globbing on the redirection word.
```

### Example: function lookup precedence

```sh
# If `exit` is a POSIX special builtin it must be found before a shell function named exit.
```

---

## Implementation notes & suggestions (for your shell project)

* **Configuration mode**: implement a clear `posix` mode flag and an explanatory `--compat` or `--strict` flag. Documentation must list every semantic change when `posix` is enabled.
* **Tests**: create a POSIX conformance test suite with small scripts demonstrating fatal errors (assignment error before command, invalid arithmetic, syntax errors in sourced files).
* **Compatibility knobs**: expose explicit toggles for multibyte-IFS handling and `echo` escape interpretation so users can match scripts written for other shells.
* **Documentation**: for every behaviour where you deviate from POSIX for pragmatic reasons (UTF-8 handling, editor defaults), add a short "why we deviated" note.
* **Fallbacks**: where POSIX mandates `ed` fallback (e.g., `fc`), provide sane modern defaults but honor POSIX behavior when in POSIX mode.

---

## Final friendly note

This document condenses the essential POSIX expectations into actionable rules you can implement and test while building your shell. Use the cheat sheet for quick decisions, and the rules sections when you need detail. If you want, I can now:

* produce a one-page printable PDF layout of this guide,
* generate a checklist of tests (100+ tiny posix test cases), or
* convert the rules into a machine-readable YAML specification you can use in CI.

Which of those would you like next? (No waiting — I can generate it right away.)
# What is POSIX

POSIX is the name for a family of standars based on UNIX.  A number of Unix services, tools, and function are part of the standard.