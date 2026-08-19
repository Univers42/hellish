# hellish backlog

Living tracker for the "fastest and most complete shell" program. Every item
lands only with: `make test` green (2878 golden cases), `tests/hard` 16/16,
`tests/scripts` diffs clean vs bash --posix, `verify_alloc.sh` green on both
heaps, and the `make conformance` baseline gate not regressing (Oils / mksh
pass counts in `bench/baseline/`). Performance claims come from
`make perf` / hyperfine on the OPT build only.

## Done (this program)

- [x] **`"$VAR"` reaches the expander fast path: −19.5% loop instructions**
      (callgrind, loop_arith: 1,266.9M → 1,019.2M Ir). `try_simple_envvar`
      (`src/expander/expand_word2.c`) gated on `t->tt != TT_ENVVAR`, so the
      DOUBLE-QUOTED form fell through every fast path and paid `clone_ast`
      plus the whole `expand_word_glob_ctl` pipeline — ~2.5k instructions to
      read one variable, on what is the most common construct in real POSIX
      scripts. Accepting TT_DQENVVAR alongside TT_ENVVAR is safe because the
      reparser emits exactly one subtoken for `"$v"` and both types carry the
      bare name in start/len.
      The existing guards are what make it sound, and they must not be
      relaxed casually: `!*v` routes unset/empty to the slow path (keeping
      `"$empty"` as ONE empty field rather than vanishing), `name_is_plain`
      rejects `$@`/`$*`/`${a[@]}` so aggregates still split, and
      `needs_split_or_glob` catches the rest. What is left is exactly the set
      where quoted and unquoted expansion are byte-identical.
      Found by a profile-driven fan-out; the claim was adversarially
      re-verified before landing. Gates: 3016/3016 golden, plus 12 targeted
      quoting cases (empty/unset → one field, `"$v"` with spaces → one field,
      `$v` with spaces → three, custom IFS both ways, quoted glob char,
      `"$@"` and its iteration form).
- [x] **Hot-path predicate ordering: −33% parse instructions** (callgrind,
      parse50k: 573.5M → 383.5M Ir, 1.50x less work). Two instances of the
      same bug — an expensive test placed before the cheap discriminating
      one — found by profiling, not reading:
      1. `hazard_at` (`src/infrastructure/rl_multi_line2.c`) ran
         `ft_strncmp(...,"alias",5)` and `...,"source",6` on **every byte**
         of input: ~4M libc strncmp calls on a 2MB script, 17.4% of all
         instructions retired. Gating each on `s[i]=='a'` / `'s'` is exact,
         not heuristic (strncmp can only match where the first byte does),
         and cut strncmp 99.8M → 10.0M Ir and `buff_readline` 92.7M → 35.1M.
      2. `reparse_dquote` evaluated `getenv("HELLISH_DBG_DQ")` **before** a
         one-byte test, once per double-quoted word — 34.6k linear `environ`
         walks per parse, 5% of parse time to prove a debug flag was unset.
         Pure operand swap; both operands are side-effect-free.
      Gates: 3016/3016 golden, 213/213 regress_hellish incl. 6 new hazard
      cases (words merely CONTAINING alias/source, dot-source, heredoc
      bodies, backslash-newline, `$LINENO`), `40_batch_semantics.sh`
      byte-identical to `bash --posix`.
      Lesson worth keeping: `perf(1)` is unusable here
      (`perf_event_paranoid=4`), but callgrind counts *instructions*, which
      are deterministic — so on a noisy `powersave` box it is strictly
      better than wall-clock for finding where the work is.
- [x] **Batched non-interactive input delivery** — parse50k 705ms → ~200ms.
      Hazard fallback (alias/source/heredoc/backslash-newline), newline as
      top-level list operator, per-range heredoc gathering, failure replay
      (rewind + line-exact rerun so pre-error commands still run), exact
      `$LINENO` from token offsets. Regression net:
      `tests/scripts/40_batch_semantics.sh`. Commit `74ac593`.
- [x] **ft_malloc morecore batching** (nested submodule): small-bin refills
      went from one 4KB mmap per block to 64KB batches; 23.7k → 1.5k mmaps
      on parse50k. ft_malloc `045e360`, libft bump `b5550ed0`.
- [x] **Parse arena** (`src/helpers/parse_arena*.c`, `incs/parena.h`):
      chunked bump allocator for cycle-lifetime parse objects (AST children
      buffers via `ast_push_child`, full_word copies), gated ON only around
      the main-cycle parse_tokens so eval/source/cmdsub keep heap
      discipline; `parena_free` routing makes every teardown path safe for
      both arena and heap trees. Also fixed latent raw-xfree bombs in
      expander_split / assignment_word_to_word / split_envvar2 /
      process_word_token2. RSS for parse50k capped (~160MB), allocation
      churn removed.
- [x] **Plain-word reparse fast path**: words with no quote/expansion/glob
      chars skip the reparse rebuild entirely (~2/3 of real-script words).
      Gotcha learned: plain subtokens keep split_eligible FALSE (only
      cmdsub output splits) and full_word NULL.

## Done (continued)

- [x] Arena + fast path committed (`5bb80e2`), gate green (Oils 997 /
      mksh 184 unchanged).
- [x] **`./configure` COMPLETES under hellish** (`ebeb50e`) — exit 0,
      config.status + Makefile generated. Four bugs fixed, each with a
      repro in tests/regress_hellish: heredoc-body text corrupting the
      cycle-completeness quote state (tokenize the stripped text),
      backticks inside dquotes not scanned atomically by the lexer,
      quote-blind \$() paren scans in reparser AND expander (shared
      sh_skip_quoted), for-without-in iterating live positionals instead
      of a POSIX entry snapshot (SEGV under config.status's shift).

## Done (continued 2)

- [x] Configure-fixes gate GREEN and IMPROVED: Oils 997→998, mksh
      184→186.
- [x] **word-split/IFS cluster fixed**: unquoted $@/$* now defer to the
      splitter (one field per positional, each IFS-split — never joined,
      so IFS='' keeps positionals separate); trailing IFS separators
      close the field (IFS=:; ${w}:b splits at the boundary); deferred
      positionals are recognised by pos_mark() POINTER identity, fixing
      the v=@; echo "$v" collision; ${@-M}/${@+P} operator forms share
      the same sentinel path. split_envvar refactored onto one
      split_value core with uniform lead/trail rules for whitespace and
      non-whitespace IFS. 8 new regress_hellish cases.

## Done (continued 3)

- [x] IFS gate GREEN: **Oils 991 → 1007, mksh 184 → 187**, baseline
      accepted (`bc4d0c9`). Session total: +16 Oils, +3 mksh, configure
      complete, parse 3.4× faster.
- [x] bench/KNOWN_ISSUES.md rewritten — configure saga documented as
      resolved, 64B ASan-configure leak tracked as the open remainder.

## Done (continued 5)

- [x] **Job control LIVE** (`2a7c5c1`): background launches register in
      the job table; jobs/fg/bg/kill %1 work for the first time. Report
      lifecycle matches bash EMPIRICALLY (7 probes, all MATCH):
      script-silent notify, jobs report-once-keep-status, wait purges +
      forgets (re-wait = 127), ring-miss falls back to the table (jobs'
      own WNOHANG poll used to eat statuses). job_print byte-identical
      to bash (27-pad + trailing " &" on running bg jobs). 6 regress
      cases. hard/12 was broken mid-flight by script-mode notices —
      the INP_RL gate in job_notify is what fixed it.
- [x] **Prompt v3 — prettier AND faster** (committed after `2a7c5c1`):
      truecolor palette + ember gradient fill (COLORTERM-gated, 256
      fallback preserved), dim-parents/bold-last cwd, @host over SSH,
      and the dirty `*` RESURRECTED from dead code (real git status
      fork, 3s TTL cache, one subprocess max). repo_locate cwd-keyed
      root cache: deep-dir render 44us -> 6.5us; repo root 4.4us
      steady-state. HELLISH_PROMPT_BENCH=1 is the measuring stick.
      Battery: 2901 golden, 16/16 hard, 40/40 scripts, both heaps.

## In flight

- [ ] The special-builtin abort rule is implemented for the two error
      classes that could be pinned down against bash 5.3.9: a redirection
      error on a special builtin, and a non-zero return from export /
      readonly / unset. bash's real rule is per-error-site -- `shift 99`
      and `trap "" BADSIG` both return 1 without aborting, so status alone
      cannot decide it. Other special builtins with usage errors (`. `
      with a missing file already aborts via its own path; `times x`,
      `trap` with a bad action) were not audited one by one.

- [ ] A locale change made INSIDE a running script does not reach the glob
      sort. Every startup form is right now (LC_ALL=C hellish -c 'echo *',
      LC_COLLATE=C, and the prefix assignment LC_ALL=C echo *), but:
        $ hellish -c 'export LC_ALL=C; echo *'  -> still locale order
        $ bash    -c 'export LC_ALL=C; echo *'  -> byte order
      bash re-runs setlocale when LC_ALL / LC_COLLATE / LANG is assigned.
      Doing the same needs the assignment path to push the value into the
      C library's environ (t_vec_env is the truth here and envp is only
      materialised for execve), so it is not a one-liner.

- [ ] Backslash before a brace inside a double-quoted expansion keeps the
      slash where bash drops it:
        $ hellish --posix -c 'echo "${u-\}}"'   ->  \}
        $ bash    --posix -c 'echo "${u-\}}"'   ->  }
      The unquoted form `echo ${u-\}}` agrees (both print `}`), so it is
      the dq path's escape handling, not the brace scan. Pre-existing:
      reproduced identically before and after the lexer/reparser nested
      quote fix.

- [ ] Redirections are RESOLVED (all of them) and only then APPLIED, where
      bash interleaves resolve+apply left to right. Parking every opened
      scratch fd at >= 10 fixes the common collisions (fixed, see
      redir_fd.c), but two shapes still need the interleave and are the
      only cases dropped from `tests/redir_multi_fd`:
        exec 3>a 4>&3   -- `4>&3` is resolved before `3>a` is installed,
        exec 4>a 3>&4      so the dup sees a closed fd and errors
        exec 4>a 5>&4
        exec 11>a 2>b   -- a target fd >= 10 can collide with the parking
                           range itself
      The lazy AST path (apply_redirs_from_ast) ALREADY interleaves and is
      correct; only the pre-resolved vec path needs it.
- [ ] Glob results are sorted in ASCII order; bash sorts by locale
      collation. In any mixed-case directory the two disagree:
        $ hellish --posix -c 'echo *'   ->  CLAUDE.md CODE_OF_CONDUCT.md ...
        $ bash    --posix -c 'echo *'   ->  backlog.md bench build CLAUDE.md ...
      True with LANG=en_US.UTF-8 and with LC_ALL=en_US.UTF-8 set
      explicitly, so it is not an unset-locale artefact. src/glob has an
      ft_strcoll.c already; either it is not wired into glob_sort or it
      does not consult the locale. Found because tests/hard/03_rpn_calc.sh
      globs `**` against the cwd and picks a different first file in each
      shell -- that hard test is cwd-sensitive by construction and will
      keep failing from the repo root until this is fixed.

- [ ] `history` does not list itself. bash records an entry BEFORE running
      it, so `history` shows the `history` command as its own last line;
      hellish records in manage_history AFTER execution, so the listing
      always stops one short. Found measuring issue #6 against bash 5.3.9
      in a pty; separate contract from the multi-line joining fixed there.
- [ ] Prompt render window: the fix for #5/#10/#19 makes the prefix write
      atomic, so an echoed keystroke can no longer split an escape or a
      multibyte glyph -- but it is still ECHOED at whatever point of the
      prompt it arrives, and readline does not know it is on screen. The
      screen can therefore still look off by one keystroke after fast
      type-ahead, even though nothing is corrupted. Closing the window
      entirely means turning echo off around the prefix write, which is
      delicate: readline saves the termios it finds and restores it on
      return, so modifying it before readline preps would leave the
      terminal without echo after the line is read.

- [ ] Conformance gate for `2a7c5c1`+prompt running (expect possible
      Oils `background` cluster gains). Norminette NOT INSTALLED on
      this host — norm gate unverified since the audit commits; run
      `make norm` wherever norminette exists before the next PR.
- [ ] `make perf BENCH_LAX=1` rerun — first run where the configure
      dimension can TIME hellish (completion gate now passes). Commit
      bench/results.md when it lands.
- [ ] Job polish: bash prints "Terminated"/"Killed" per signal where we
      print "Done" (interactive-only path, no golden coverage); wait
      %jobspec; wait -n.
- [ ] `set -o` roster: the full bash option list is now ACCEPTED, listed
      and reported in `$-` (issue #8), and `braceexpand` is genuinely
      wired.  Still tracked-only, i.e. the flag is remembered but nothing
      reads it yet: `hashall`, `physical`, `monitor`, `notify`,
      `keyword`, `onecmd`, `errtrace`, `functrace`, `histexpand`,
      `history`, `ignoreeof`, `interactive-comments`, `nolog`,
      `privileged`.  Wire them one at a time, each with golden cases;
      `hashall` (gate src/builtins/builtin_hash*.c) and `physical`
      (gate cd's default -L/-P) are the two cheapest.
- [ ] `set -r` (restricted) stays a DELIBERATE divergence: bash accepts
      it, hellish returns 2.  Recording a security flag we do not
      enforce would be worse than failing loudly.  Implement restricted
      mode or leave it rejected -- do not "fix" it by accepting it.
- [ ] Job label for a COMPOUND background job drops its opening
      delimiter: `( sleep 1 ) &` and `{ sleep 1; } &` show up in `jobs`
      as `sleep 1 ) &` / `sleep 1; } &`.  bg_job_label
      (src/platform/posix/execute_range_bg.c) descends past the
      compound node because that node carries no token.start, and
      starts copying at the first child's token instead.  Needs the
      parser to record the compound's own span; found while fixing #18,
      deliberately left out of that branch.  Simple commands and
      pipelines are correct.
- [ ] `job_notify` (the interactive Done announcer) still walks table
      SLOTS, so with a reused slot it can announce out of job-number
      order.  `jobs` was fixed for #18 (job_next_after); do the same
      there.  Interactive-only, so no golden coverage -- needs a pty
      test.
- [ ] `! cmd &` exit status: bash does NOT negate an async child's
      status -- `! false & wait $!` is 1 there and 0 here, `! true &`
      is 0 there and 1 here.  execute_pipeline applies node->negate
      inside the background child; bash applies it only to the
      synchronous return value (and an async list returns 0 anyway).
      Found while fixing #13, deliberately left out of that branch --
      `!` is why bg_lone_command refuses the exec-in-place path, so the
      two are independent.
- [ ] A background job's real command has SIGINT/SIGQUIT at SIG_DFL when
      it goes through the FORKED path (compounds, pipelines): the
      grandchild's default_signal_handlers() undoes the wrapper's
      SIG_IGN.  The exec-in-place path was fixed for #13
      (async_child_signals); the forked path still needs an
      "I am inside an async child" signal so it can re-apply them.
- [ ] `exec` with MORE THAN ONE redirection is broken and silently does
      the wrong thing.  `exec 3>&2 2>file` applies neither; `exec 4>a 2>b`
      pointed fd 2 at *a*, i.e. it paired the last fd with the first
      target.  Single-redirection `exec` is fine, which is why this hid
      for so long -- and it is why 17_bg_signal_report.sh spells its
      stderr capture as two `exec` lines.  Found while writing that test.
- [ ] Script line numbers in diagnostics are the END OF THE INPUT BATCH,
      not the running command: a 4-line script reports every error as
      "line 5".  update_ctx (src/infrastructure/rl_helpers.c) formats
      state->rl.line, which points past the batch; the correct value is
      tok_lineno(), which $LINENO already uses and which does match bash.
      Not a one-liner: tok_lineno lives behind execution_private.h, and
      update_ctx runs once per input CYCLE, so it would also have to move
      to per-command.  Affects every message, including the new #17 one.
- [ ] A finished job is not reaped until something ASKS: `jobs` inside a
      pipeline (`jobs | cat`) or a command substitution (`$(jobs)`) runs
      in a forked child whose waitpid cannot reap the parent's children,
      so it reports "Running" where bash reports "Done"/"Killed".  bash
      reaps asynchronously on SIGCHLD; hellish only polls in `jobs` and
      `wait`.  Architectural, not a display bug.
- [ ] `kill -STOP` on a background job: hellish's `jobs` says "Stopped",
      bash says "Running" when job control is off (`-c`, scripts).  bash
      only tracks stops when it has job control.
- [ ] The '-' marker on a FINISHED job follows a rule I could not pin
      down, and hellish gets one of the two shapes wrong.  Measured:
        `sleep 0.2 & sleep 0.5; sleep 0.2 & sleep 0.5; jobs`
            bash "[1]  Done" (bare) -- hellish "[1]-  Done"
        `sleep 0.3 & sleep 0.05 & sleep 0.5 & wait %1; jobs`
            bash "[2]-  Done" -- hellish agrees
      So "a dead job never wears '-'" is NOT the rule; it was tried and
      the second case disproves it.  The difference may be whether the
      CURRENT job is itself dead.  '+' is unaffected: a dead job keeps
      it in both shells.  Needs bash's set_current_job() semantics read
      properly rather than another guess.
- [ ] tests/level01.sh diffs INTERMITTENTLY in verify_alloc.sh -- seen
      three times across a session, never reproducible on demand.  Not
      for want of trying: 70 direct back-to-back invocations (including
      verify_alloc's exact two-run crash-probe-then-output sequence) and
      six full verify_alloc suites with the mismatch dumped to a file
      all came back clean, so the failing pair was never captured.  The
      script is nothing but `echo`s and one `pwd`, which makes `pwd` the
      only candidate: logical PWD vs physical getcwd under the
      automounted /sgoinfre path.  It costs an investigation every time
      it appears.  Next step is probably to have verify_alloc keep the
      mismatching output permanently rather than to keep guessing.

## Done (continued 4)

- [x] Prompt v2 committed (`af2aade`) + first timed-configure scoreboard
      (hellish 19.8s vs bash 7.7s / dash 6.8s — completes, 2.6× gap to
      close, fork-path dominated).
- [x] Audit quick wins committed (`daf5a99`): $RANDOM (pid^time-seeded
      PRNG), $SECONDS/$EPOCHSECONDS/$UID/$HOSTNAME/$OSTYPE, times
      format fix, `. file args` positional binding. Gate pending.

## Done (continued 7)

- [x] **PS1/PS2 user theming** — the prompt is configurable from
      ~/.hellishrc (already sourced at interactive startup): PS1/PS2
      with the bash escape set (\u \h \H \w \W \$ \n \t \d \e \a \\
      \j \s \v \[ \]) plus live $NAME/${NAME} expansion; the rich
      built-in prompt is now just the default theme when PS1 is unset.
      prompt_ps1.c/prompt_ps1b.c. Pty-test gotcha recorded: script(1)
      uses $SHELL and bash-family shells strip PS1 from the exported
      env — test with SHELL=/bin/sh.
- [ ] PROMPT_COMMAND, \D{fmt}, \! \# history escapes, promptvars
      cmdsub — natural PS1 follow-ups, small.

## Done (continued 8) — conformance batch 1

- [x] **$'…' ANSI-C quoting** (full bash escape set incl \uHHHH/\U/\cX,
      atomic lexing with live \' escapes, TT_SQWORD decode in the
      reparser, "$'…'" stays literal). 10/10 bash probes.
- [x] **wait %jobspec + multi-arg + bash error semantics** — wait %1
      used to HANG (atoi("%1")=0 → waitpid on the whole group).
- [x] **[[ a < b ]]** string comparison (was silently a REDIRECT).
- [x] **set -e AND-OR suppression reaches subtrees** — f(){ false; };
      f || true no longer exits from inside the body. 7/7 probes.
- 12 regression lines added (tests/regress_hellish, 2913 golden total).

## Done (continued 9) — ARRAYS (the big wall)

- [x] **Indexed arrays**: arr=(a b c), arr+=(d), arr[i]=v (arith
      subscripts, sparse, scalar promotion), $arr, ${arr[i]},
      ${arr[@]}/${arr[*]} quoted+unquoted with exact field semantics,
      ${#arr[@]}, ${#arr[i]}, bash-style set-listing, never exported.
      16/17 bash probes. Storage = encoded ordinary env value (magic +
      idx/US/val records) → ZERO env-lifecycle changes; [@] defers via
      a name-carrying marker registry mirroring pos_mark; splitter has
      exact analogues of the positional emitters.
- [ ] Array follow-ups: element-level split/glob in arr=(...) (the one
      probe divergence), slices ${a[@]:1:2}, ${!a[@]}, unset a[i],
      read -a, mapfile, declare -a, PIPESTATUS, associative arrays.

## Done (continued 10) — bash-replacement batch 2

- [x] **<<< herestrings** (7/7 probes, incl 0<<< fd form; pipe backing
      + unlinked-tmpfile fallback; TT_HERESTRING keeps the heredoc
      machinery clean; hang root-caused to get_default_src_fd).
- [x] **PIPESTATUS** array after every pipeline (all stages now waited
      synchronously like bash) + single-command form.
- [x] **unset a[i]** (arith subscript; variable survives as empty array).
- [x] **read -a NAME / read -p PROMPT** (value-taking option parser;
      -rp combos work).
- [x] Array gate ACCEPTED: **Oils 1029 → 1047 (+18, campaign record),
      mksh 191 → 193**.
- [ ] FOUND pre-existing: `exit` in the LAST pipeline stage runs
      in-parent and kills the shell (`exit 3 | exit 5; echo x` never
      echoes; bash contains it). Pipeline builtin containment fix.

## Done (continued 11) — regex + pipeline containment

- [x] **[[ =~ ]] with BASH_REMATCH** (POSIX ERE; the lexer grew a
      regex-word mode — the word after =~ is ONE token so parens
      survive; parked-pointer cell threads state to the evaluator).
      7/7 probes incl multi-group captures.
- [x] **Pipeline containment fixed**: stages never run in-parent
      (was zsh-lastpipe style — `true | exit 7` killed the shell,
      `echo x | read v` leaked v). Inside a forked compound the flag
      flips back so `| while read` loops work exactly like bash.
- [x] Batch-2 gate accepted: **Oils 1047 → 1057, mksh 193 → 195**.
- Session running total: Oils 1029 → 1057+, mksh 191 → 195+.

## Done (continued 12) — the "replace bash" campaign (autonomous session)

Landed toward flipping hellish as a login shell (all probe-verified vs
bash, full battery green each step, both heaps leak-free):
- type -t/-p/-P, wait -n, set rejects unknown flags fatally.
- Array completion: ${a[@]:off:len} slices, ${!a[@]} keys, ${a[-1]}
  negative index, ${a[@]/p/r} & ${a[@]#p} element-wise ops, mapfile/
  readarray -t, declare/typeset -p/-a, local/declare a=(...) arrays.
- shopt -s/-u/-q (nullglob+dotglob wired into the glob path).
- ${v^^}/${v,,}/${v~} case conversion, ${v@Q}/@U/@L/@u/@A transforms.
- printf -v NAME (with bash name validation), [[ -v NAME ]], [[ =~ ]]
  with BASH_REMATCH.
- Pipeline containment fixed (true|exit no longer kills the shell;
  echo x|read v no longer leaks) — the one shell-KILLING bug.
- $'...', <<< herestrings, PIPESTATUS, unset a[i], read -a/-p, wait
  %jobspec, [[ a<b ]], errexit and-or suppression, var-sub-quote.
- BASH_VERSION/BASH_VERSINFO set so scripts take their bash path.
- CONFORMANCE this session: Oils 1029 -> 1060 (+31), mksh 191 -> 201
  (+10). PARSE PERF: 53.8ms vs bash 54.3ms (nominally faster, ~13%
  less CPU), 20MB RSS.
- Known scope-outs (v1, documented, low-frequency): ${a[@]:-word}
  (array + default op), mixed-quote op-words a "b c" d, element-level
  split/glob in arr=(...), slice per-element field structure,
  ${a[@]^^}, ${!prefix*}, associative arrays, declare -i arithmetic
  attribute, ${@:0} including $0.
- REMAINING for full parity (queued, none architectural): associative
  arrays (declare -A), ${a[@]:-w}, the pull-lexer to decisively beat
  bash on parse, PROMPT_COMMAND, trap DEBUG/RETURN/ERR, coproc.

## Done (continued 13) — associative arrays + the long tail

- [x] **Associative arrays (declare -A)**: string keys, h[k]=v, h[$var]=v,
      ${h[k]}, ${h[$k]}, ${!h[@]} keys, ${h[@]} values, ${#h[@]}, unset
      h[k], declare -p h, for k in "${!h[@]}". Attribute in the value
      encoding (distinct magic byte) — no attribute table. Keys deferred
      as fields via a "!"-marker.
- [x] **${a[i]OP}/${h[k]OP}** single-element operators (:- := # % / …) via
      scratch-var reuse — the ${cnt[$w]:-0} counting idiom works.
- [x] **NAME[$var]=v / NAME[$((e))]=v** assignments (pre-pass classifies
      the raw word before reparse splits the subscript) — fixed for
      indexed AND assoc (was pre-existing).
- [x] **a=() empty array literal** (is_function_def no longer eats a word
      ending in "=").
- [x] **${array[x]} inside $((...))** (routed through full expand_token).
- [x] **${!var} indirection**, **PROMPT_COMMAND**, **scalar += append**
      (n+=3, s+=world).
- Scope-outs remaining (v1, low-freq): declare -i integer attribute
      (use n=$((...))), compound assoc init declare -A c=([k]=v), bare
      c[x] in arith (use ${c[x]}), a[i]+= element append, ${!prefix*},
      printf %(fmt)T, coproc, trap DEBUG/RETURN/ERR, nameref (declare -n).

## Done (continued 14, 2026-07-22) — declare -i/-n, compound init, traps, coproc

- [x] **declare -i / declare -n** (per-var attribute table, env_attr.c;
      state->var_attrs, empty ⇒ hot paths short-circuit). -i arith-evaluates
      every assignment (n="3*4"→12, n+=5→old+5); -n retargets read
      (env_expand_n) and write (apply_var_attrs). Commit `31f2f65`.
- [x] **Compound array init [k]=v** (`4756c1e`, expand_array_assign2/3.c):
      declare -A c=([k]=v [k2]=v2) assoc; a=([2]=x foo [7]=y) indexed with
      arithmetic subscripts + running counter; plain a=(x y z) unchanged.
      Assoc declare -p prints bash's trailing space before ')'.
- [x] **trap DEBUG/RETURN/ERR** (`60f4583`, exec_traps{,2}.c): slots above
      the signals, trap_depth re-entrancy guard. DEBUG before each simple
      command + for-iter bind + case; ERR in errexit_check (decoupled from
      -e); RETURN on function return. Not inherited by functions/subshells
      (save/blank across calls) — matches bash w/o functrace/errtrace. Deep
      nested-leak firing NOT matched (rare).
- [x] **coproc** (`10b9cc5`, TT_COPROC/AST_COPROC, parse_coproc.c,
      execute_coproc.c): all 3 forms; NAME[0]/[1] fds (>fd9, CLOEXEC via
      save_fd) + NAME_PID, backgrounded own-pgid child. Reclassify keeps the
      token after `coproc NAME` in cmd position so `{` opens a compound.
- Suite 3012 green, both-heap parity, ASan clean, regress_hellish covers all.

## Done (continued 15, 2026-07-22) — parse profiled; hellish now BEATS bash

Profiled with strace -c + hyperfine User/System split (perf/ptrace locked).
Finding: hellish's parse USER-CPU already beats bash (47 vs 53ms); the whole
gap was SYSTEM time (page faults from 4x RSS). Two fixes, both green (3016
tests, both heaps, ASan, Oils 1069 / mksh 210):

- [x] **64KB file-read buffer** (libft `b7666008`, bump `b477496`): script
      slurp was 1766 read() calls (1KB buffer in vec_append_fd) vs bash's
      225; now ~34. Flipped hellish ahead of bash on wall-clock.
- [x] **Lean 16B deque token** (`2989d00`): the raw token deque stored full
      32B t_tokens, but full_word/arith_cache are NULL there (attached later
      on AST nodes / at exec). New t_ltoken (16B prefix) + ltok2tok(); 3
      deque_init sites use sizeof(t_ltoken), all 87 deque casts routed
      through t_ltoken (compiler-verified). parse50k RSS 19.9->15.1MB (-24%),
      system 5.8->4.5ms.
- pull-lexer verdict: it's a MEMORY lever (user-CPU already wins), not worth a
      from-scratch rewrite; ft_lexer/ft_yacc rejected (LALR slower on CPU than
      the tuned RD; shell grammar hostile to yacc). Remaining RSS over bash:
      two whole-file copies (input + alias_exp, ~3.4MB) and the full-file token
      deque still materialized (~4.75MB) — only a pull-lexer eliminates that.

## VERDICT (self-assessment, 2026-07-21)
hellish is a drop-in bash replacement for interactive use and the vast
majority of real scripts: arrays (indexed + associative), all common
parameter expansions, [[ =~ ]], herestrings, PIPESTATUS, job control,
process subs, config-themed animated prompt — at bash-parity parse
speed (56ms vs 54ms) with 20MB RSS. Oils 1029 -> 1065+, mksh 191 -> 201+
this session. The remaining gaps are the documented low-frequency tail.

## Feature-gap audit results (agent, 2026-07-21) — ranked queue

1. **Job control inert**: `job_add` (src/job_control/job_table.c:38) has
   ZERO callers — jobs/fg/bg/kill %1/wait %1 all dead despite complete
   plumbing (jobspec parser, table, builtins registered). Wire it in the
   background-launch path (execute_range_background / exe_bg). Also the
   Oils `background` cluster (3+ cases) depends on this.
2. Quick potent fixes: `times` prints its literal format string;
   `$RANDOM` prng exists in t_shell but never wired to expansion;
   `$SECONDS`/`$EPOCHSECONDS` missing; `$UID`/`$HOSTNAME`/`$OSTYPE`
   unset at init; `. file args` doesn't bind $1..$N (bash/ksh/zsh do —
   Oils builtin-eval-source cluster); `$0`/`$-` empty under -c;
   `read -t` timeout rc=1 (bash >128); `read -p/-n/-d` silently no-op
   (worse than erroring); `type -a/-t/-P` missing; README claims a
   `dirs` builtin that does not exist (stale doc).
3. `$'…'` ANSI-C quoting — silent literal passthrough today; everywhere
   in real scripts. Lexer+expander feature.
4. `<<<` herestring (+ `|&`, `;&`, `;;&`, `function` keyword) — parser
   features, herestring can reuse heredoc backing.
5. `[[ ]]` is plain test: no `=~`, and `<`/`>` become REDIRECTIONS
   (silently wrong). At minimum make `<`/`>` compare and error on `=~`.
6. Arrays (indexed first) — the biggest script-compat wall; also blocks
   PIPESTATUS/BASH_REMATCH/read -a/mapfile. Large project.
7. `declare`/`typeset` + `shopt` (globstar/extglob/nullglob/autocd
   gates) — large.
8. PS1/PROMPT_COMMAND/HISTCONTROL/HISTIGNORE — personalization; the
   rich prompt should fall back to PS1 when set.
9. `set -e; f(){ false; }; f || true` still exits — errexit suppression
   around function calls (Oils errexit cluster).
10. Unknown `set` flags silently accepted rc=0 (`-m -b -H -k -E -T`…) —
    should at least not lie.
- [ ] NEXT CLUSTER picked: var-sub-quote (10 cases) — diagnosis in task
      list: ${x:-"c d"} flattens op-word quoting; needs segment-aware
      default emission (quoted segments split_eligible=false). After it:
      builtin-vars export semantics (8), assign/local scoping (7),
      builtin-meta command -v/-V/-p (6), fatal-errors (5).
- [ ] KNOWN ISSUE: one 64-byte realloc_to leak, only visible in a fully
      ASan-instrumented configure run (suites all leak-clean). Repro:
      debug build, CONFIG_SHELL=build/bin/hellish on the vendored hello
      configure, grep conf.log for LeakSanitizer. Chase with
      ASAN_OPTIONS=fast_unwind_on_malloc=0:malloc_context_size=20.
- [ ] Update bench/KNOWN_ISSUES.md (configure section is now history) +
      let make perf time the configure dimension (completion gate should
      now include hellish).

## Next — profiled perf queue (2026-07-22, adversarially verified)

Both survived a skeptical reviewer whose brief was to refute them. Neither is
landed: each is medium-risk and needs coverage the current suite cannot give,
because every category in `tests/tester` runs through `hellish -c`.

- [~] **dlopen readline instead of linking it — ~30% of startup.**
      **HARNESS DONE (`make readline-test`), payoff re-measured, surface mapped.**
      The coverage gap that blocked this is CLOSED: tests/readline_paths_test.py
      drives a real pty over every readline entry point (line read+run, up-arrow
      recall, command/filename/ambiguous completion, vi<->emacs, emacs keys after
      the round trip, clean exit). It passes on the unmodified shell, so it is a
      valid before/after gate. Two traps it encodes: a pty ECHOES input (a literal
      marker "passes" against a dead shell — assert on an EXPANSION result), and
      completion output interleaves with prompt redraws (assert by EXECUTION).
      Re-measured payoff (callgrind, deterministic): a trivial C program costs
      137,115 Ir; the same program linked -lreadline costs 430,920 Ir, so loading
      libreadline+libtinfo is **293,805 Ir** — confirming the old 291k figure.
      hellish `-c true` startup is 814,492 Ir (bash 1,208,806 — we already beat
      bash 33%; dash 268,563), so this is **36% of our startup** and would land
      us at ~521k, roughly 1.9x dash instead of 3.0x.
      EXACT surface (from `nm -D` + `readelf -r`, not guesswork) — 4 imported
      functions: readline, add_history, rl_completion_matches, rl_variable_bind;
      7 R_X86_64_COPY data relocs: rl_outstream, rl_completion_append_character,
      rl_attempted_completion_function, rl_instream, rl_editing_mode, rl_point,
      rl_event_hook; plus rl_line_buffer, rl_readline_state and
      rl_filename_completion_function referenced in source. 10 files touch them:
      completion/{completion,complete_files,complete_commands2,complete_variables}.c,
      editing/{vi_mode,emacs_mode}.c,
      infrastructure/{rl,history,mascot_anim,mascot_redraw}.c.
      DESIGN THAT AVOIDS TOUCHING ALL 10 (do it this way): a `t_rl_dyn` table of
      function+data pointers filled once by a lazy dlopen/dlsym, exposed as ONE
      accessor `rl_dyn()`, then a header that redirects the names —
      `#define rl_editing_mode (*rl_dyn()->editing_mode)`,
      `#define readline(p) rl_dyn_readline(p)` etc. Swap
      `#include <readline/readline.h>` for that header in the 10 files and NO call
      site changes. Keep the readline headers included for types/RL_STATE_*; an
      unreferenced `extern` emits no relocation. Link becomes -ldl, not
      -lreadline. Norm: one accessor + two loader statics, not 14 accessors.
      The pointer cache is a file-static — allowed under the documented
      "readline is the painful exception" clause in CLAUDE.md; document it.
      REMAINING: write the shim, swap the includes, flip the link, add the
      HELLISH_STATIC_READLINE fallback, then gate on `make readline-test` +
      `make hist-test` + `make anim-test` + `make docker-test` (4 distros) and
      re-measure startup Ir.
      Original notes below.
- [ ] (original) **dlopen readline instead of linking it — ~30% of startup.**
      hellish needs 3 shared objects at load time (libreadline, libtinfo,
      libc); bash needs 2 (its readline is static), dash needs 1. Relocating
      libreadline + libtinfo costs 291,493 Ir of the 829,882 Ir `-c true`
      startup, and a non-interactive run calls exactly zero readline entry
      points. Surface is small — 4 function symbols (`readline`,
      `add_history`, `rl_completion_matches`, `rl_variable_bind`); the real
      work is the ~10 R_X86_64_COPY relocations for readline globals
      (`rl_attempted_completion_function`, `rl_editing_mode`, …), each of
      which becomes a dlsym'd pointer deref, confined to `src/editing/` and
      `src/completion/`. Measured dead end: `-z lazy` is NOT the fix (427,753
      vs 427,782 Ir — the cost is data relocations, not PLT stubs).
      Keep a `HELLISH_STATIC_READLINE` compile-time fallback for musl/hardened
      loaders. **Coverage gap**: needs `make hist-test`, `make docker-test`
      across all four distros, and a NEW pty test for tab-completion and
      vi/emacs switching. Startup compounds at `./configure`'s fork rate, so
      this is really a configure fix wearing a startup hat.
- [ ] **Stop forking twice per command substitution — ~12% of fork workloads.**
      `x=$(/bin/true)` spawns TWO processes where bash and dash spawn one
      (measured: clone=2000 vs 1000 vs 1000 over 1000 iterations): the
      subshell child forks again instead of exec'ing in place. Fix is to
      thread a "this process is a subshell body and this is its final
      command" bool through `t_shell` (no new global) from
      `fork_and_run_inproc` / `execute_subshell` down to `execute_cmd_bg`,
      and reuse `cmdsub_fast2.c`'s eligibility scan for the conservative
      single-simple-command precondition.
      **Hazard**: exec-in-place is irreversible, so `$(a; b)`, `$(a && b)`,
      `$(a &)` and non-final pipeline stages must never take it or they
      silently lose the trailing work; DEBUG/ERR/EXIT traps must still fire.
      Whole-script diffs in `tests/scripts/` and `tests/hard/` are what would
      catch a lost trailing statement.

Not pursued: the `configure` track proposed a quadratic-parser root cause
that the verifier refuted (0 of 5 findings survived) — do not re-open it
without a fresh profile.

## Next (ordered)

1. **word-split/IFS conformance cluster** (~30 certified bugs, biggest
   cluster). Isolated repros:
   - `set -- a "b c"; IFS=""; printf "[%s]" $@` → must give `[a][b c]`
     (empty IFS must NOT join positionals; hellish joins).
   - `IFS=:; word=a:; printf "[%s]" ${word}:b` → must give `[a][:b]`
     (split at expansion/literal boundary; hellish gives `[a:b]`).
   Fix lives in `src/expander/ifs_split*.c` + `expander_split*.c`.
   Related mksh families: IFS-*, single-quotes-in-braces (quoted patterns
   inside `${v#'pat'}`).
3. **Full lexer/reparser fusion** (the remaining parse-speed lever, target
   bash parity ≤70ms on parse50k): emit typed subtokens during the main
   lex instead of the 3-pass lex → reparse_words → reparse_assign_words
   pipeline. Profile first with gprof
   (`make OPT=1 CFLAGS='... -O2 -pg -g' LDFLAGS='-pg' OBJ_DIR=<scratch>`;
   perf/ptrace are locked on this host). Current profile: reparse ~37%,
   ft_memcpy (push growth) ~17%, parser proper ~25%.
4. **Teardown-walk elimination** (blocked on a clean design): skip free_ast
   for fully-arena cycle trees. Blockers found: exec-time heap attachments
   to arena nodes (arith caches, heredoc bodies, expander token rewrites in
   assignment_word_to_word/process_word_token2) would leak — needs either
   defer-registries at every attach site (fragile) or arena-ing those
   attachments via parena_owns(node) checks.
5. **Cmdsub `$(/bin/true)` fork path** — 0.86×/0.57× vs bash/dash; profile
   the fork+exec+pipe path (posix_spawn? vfork? close-range?).
6. **3-stage pipeline** — 0.92× vs dash; same fork-path work.
7. **Remaining Oils consensus divergences** (168 at last count) by cluster:
   assign/local scoping (~10), builtin-vars export semantics (~7),
   builtin-meta `command -v/-V/-p` (~6), fatal-errors expansion aborts (5),
   var-op-strip char-class (~5), sh-usage `$0` handling (3), xtrace PS4 (5).
8. **mksh check.t divergences** (55): alias-3..6, bksl-nl-ign family,
   exit-err family (errexit interactions), heredoc-quoting-subst.
9. **`make agnostic-bench`** — rerun the 8-shell docker matrix once parse
   lands; publish the scoreboard in bench/results.md.
10. **Feature-gap audit** — agent review: what does hellish lack vs
    bash/zsh/ksh for daily-driver use (HISTIGNORE? PROMPT_COMMAND?
    arrays? `[[ =~ ]]` regex? printf %(fmt)T? job specs %+/%-? RANDOM/
    SECONDS? set -o pipefail — check present). File findings here.

## Pull-lexer — FULL PLAN for a future session (deferred 2026-07-22)

Branch `perf/pull-lexer` (off develop; carries the 8B deque token). Goal: stop
materialising the whole file's token deque up front. Even after the 8-byte
token, a 50k-line parse holds ~310k slots × 8B ≈ **2.5MB** resident that a
pull-lexer would cut to one range's worth (a few KB). parse50k would go
~11.9 → ~9.5MB.

**THE DECISIVE CONSTRAINT (why this was deferred, read before starting):** the
deque's whole-file materialisation is COUPLED to the whole-file *parse*. To lex
lazily AND stay CPU-neutral you must ALSO make the parser incremental. A simpler
"re-lex + re-parse each range as it accumulates line-by-line" driver works but
re-does O(n²) parse work inside every multi-line construct — and parse50k is
*all* multi-line function bodies, so that is a ~7–14× parse-CPU regression,
which violates the project's hard "faster than bash" requirement. So a correct
pull-lexer is effectively a pull-*parser* too: a multi-week, load-bearing
rewrite. Do NOT ship the simple CPU-regressing version.

**Foundation already built on the branch:** `src/lexer/tokenizer2.c` `lex_line`
— a resumable single-logical-line lexer (stops after a top-level newline,
appends to the deque WITHOUT clearing or pushing TT_END, offsets stay relative
to the whole `base`). `skip_noise`/`tokenize_step` were un-static'd in
tokenizer.c and declared in lexer.h. It compiles clean, suite green, but is
NOT wired (dead until the driver exists). Whole-file `tokenizer()` unchanged.

**The exact seam to convert** (verified via architecture map):
- PRIMARY: `update_prompt` — `src/infrastructure/input.c:92`/`:97` calls
  `tokenizer(alias_exp.ctx, tt)` over the ENTIRE buffer on the completeness pass;
  the final NULL-returning call leaves the whole-file deque that
  `try_parse_tokens` → `stream_try` then consumes. This is the materialisation.
- The streaming driver `stream_try` — `src/infrastructure/input_stream.c:69`
  already parses one range at a time (`parse_tokens_range`) but from a
  PRE-FILLED whole-file deque; it must instead REFILL `tt` per range via
  `lex_line` before each `parse_tokens_range`.
- Range boundary is a PARSER decision, not a lexer one: top-level `TT_NEWLINE`
  consumed as a list operator with `parser->stream` set —
  `src/parsing/simple_list.c:52-53` (`stream_more=true`). A whole `if…fi` /
  `while…done` / `{…}` spanning many newlines is ONE range (compound_list has no
  stream boundary check), so the lexer alone cannot find a range end without
  tracking keyword nesting (fragile: command-position `echo done` vs `done`).

**Design that avoids the O(n²) trap (parser-driven continuation):**
1. Drop the whole-buffer completeness tokenize in `update_prompt` for the
   stream-eligible case (preloaded + ring drained — exactly `stream_eligible`,
   input_stream.c:32). Completeness then falls out of per-range parsing hitting
   EOF, like bash: no up-front whole-file scan.
2. `stream_try` loop: `lex_line` the next logical line(s) into `tt` (append,
   keep `base = alias_exp.ctx` constant so offsets stay coherent), run
   `parse_tokens_range`. On RES_OK for a top-level range → execute, CLEAR the
   deque, advance. On RES_GETMOREINPUT with input remaining → the construct
   spans more lines: **append the next line's tokens and RESUME the parse WITHOUT
   re-lexing/re-parsing from scratch** — this is the hard part and needs the
   parser to (a) distinguish soft-EOF (more tokens coming) from hard-EOF (input
   exhausted), and (b) not destructively consume until the range is complete, or
   be re-entrant on the accumulated deque. On hard-EOF mid-construct → syntax
   error (`stream_finish` already prints bash's "unexpected end of file").

**Correctness mechanisms that MUST keep working (each verified):**
- **Completeness/continuation**: `tokenizer` returns a continuation prompt on
  unterminated lexeme; `looking_for` (lexer.h:33). Per-range must still detect
  an unterminated quote/construct whose range boundary sits inside it.
- **`base`+offset coherence** (8B token): every slot's `off` is relative to
  `base`; `ltok2tok(l, base)` is called at PARSE time, so `base` (==
  `alias_exp.ctx`, kept whole and alive) must still point at the buffer when the
  range parses. Per-range materialisation is fine because base never changes.
- **Heredocs**: KEEP EXCLUDED initially. `stream_eligible` returns false when
  `state->cycle_has_hd` (input_stream.c:34); `split_heredocs` (extract2.c:104)
  and `gather_range_heredocs` (execute_simple_list.c:92) assume the whole
  buffer. Incremental `<<` body extraction is a separate follow-up.
- **Failure replay** (`try_replay_exact` rl_helpers2.c:26): already BYPASSED on
  the stream path (ranges pre-execute). Keep it disabled while streaming; it
  rewinds `rl.buff` cursor by `state->input.len` and assumes one-batch-per-cycle.
- **$LINENO** (`tok_lineno`/`note_cmd_lineno` exec_lineno.c:47/90): maps a
  token pointer to a line via `nl_count(base, tok-base)` with `base =
  alias_exp.ctx` and `cycle_line0`. Keep `alias_exp` WHOLE (don't window it) so
  the offset→line count stays correct across ranges.

**Also note (bonus targets the map surfaced):** the whole file sits in `rl.buff`
(the ring, ~1.8MB) AND is mirrored into `state->input` (~1.8MB) per batch, on
top of the deque — so there are ~3 whole-file copies. For the stream path
(replay bypassed, ring drained) `rl.buff` could potentially be freed/shrunk
after the batch is consumed for another ~1.8MB — but the cursor/replay coupling
needs care. This is a smaller, orthogonal lever to the deque one.

**Staged steps for the future session (each gated on 3016 tests + verify_alloc
both heaps + conformance Oils/mksh unchanged):**
1. [done] `lex_line` resumable primitive + un-static helpers.
2. Make `parse_tokens_range`/top-level `parse_simple_list` re-entrant: accept an
   already-partially-consumed deque and a "soft-EOF vs hard-EOF" signal, so
   appending tokens and re-invoking continues rather than restarts. Prove with a
   unit harness that feeding tokens in chunks yields the SAME AST as one-shot.
3. Wire `stream_try` to refill via `lex_line` per range (non-heredoc only);
   drop the whole-buffer completeness tokenize in `update_prompt` for
   stream-eligible cycles. Measure RSS (target ~9.5MB) AND parse50k user-CPU
   (must NOT regress vs the 8B-token baseline ~53ms — if it does, the driver is
   re-doing work; fix before landing).
4. Only then consider heredoc-cycle streaming and the `rl.buff` drain.

Rollback: `perf/pull-lexer` is isolated; if step 3 regresses CPU, revert to the
8B-token baseline (already merged to develop) — that state beats bash and is the
safe stopping point.

## Done (continued 6) — the parse-speed session (2026-07-21)

- [x] **parse50k 221ms → 72ms** (bash 60ms same load — ratio 3.2× → 1.21×),
      RSS 158MB → 22MB, in four committed rounds:
      1. `35e4f56` first-char operator dispatch (parse_op rebuilt a
         21-entry table per call, ~60 libft calls each — libft.a has NO
         LTO objects, nothing ever inlines), 256-byte char-class table
         for is_special_char/is_word_boundary (1.4M ft_strchr calls),
         dbracket_toggle call-site guard (527k → ~0 calls), arena-pure
         teardown skip via parena attach-tracking (2.17M parena_free
         routes gone; ASan net caught the one impure site — procsub's
         parse-time strndup, now arena).
      2. `0002690` reparse helpers operate in place (was: copy 104-byte
         node in + out per subtoken), table word_is_plain, shared arena
         full_word copy, deque presize for >4KB batches.
      3. **`(streaming)` — the big one**: fully-buffered batches parse ONE
         newline-range at a time, execute, parena_reset, repeat. Peak
         memory = one range. Gated: ring drained + preloaded/EOF source,
         never interactive, no heredoc cycles (conservative). Replay
         skipped mid-stream (prefix already ran = what replay emulated).
         Bonus bash-parity: healthy prefix executes before an
         unexpected-EOF error.
- [x] **Second wave — parse50k TIES bash** (64.2 vs 63.2ms overlapping
      sigma; user CPU 55.6 vs 60.7 = hellish computes LESS): libft
      primitives → compiler builtins (glibc AVX; constant sizes inline),
      fat-LTO archive for the SAFE=0 tree ONLY (linker plugin disturbs
      LeakSanitizer on plain links — bisected via alias_posix),
      ft_strnstr memchr-skipping, t_token 40→32B (type in a byte),
      t_token_old 24→16B, MADV_HUGEPAGE on 2MB+ ft_malloc zones (inert
      on this fragmented host, free elsewhere). libft ea7a804a,
      ft_malloc dbc08f5 — both submodules need pushing.
- [ ] REMAINING to clearly BEAT bash (~4-5ms, all page-fault sys time):
      pull-based lexing (feed the parser tokens on demand instead of
      materialising the whole ~450k-token deque — bash's yylex model;
      kills most of the 20MB touched). Smaller: move full_word +
      arith_cache out of t_token into t_ast_node (deque slots 32→16B,
      wide but mechanical ripple). ft_printf checked: 1 write per echo,
      same as bash — not a bottleneck (we win output-heavy dims).
      Measure ratios under identical load only.
- [ ] Loosen streaming gates later: heredoc cycles (cursor logic is
      order-compatible, unproven), INP_NOTTY before EOF.

## Perf scoreboard memory (OPT, quiet machine, hyperfine)

| dimension | hellish | bash | dash | standing |
|---|---|---|---|---|
| parse50k | 72ms | ~60ms | ~21ms | 1.21× bash, closing (was 705→221) |
| startup -c true | tied | tied | tied | tied |
| arith loop | wins 2× vs bash | | tied dash | winning |
| string concat | wins ~7× vs bash | | tied dash | winning |
| read loop | fastest | 1.5× slower | 3.5-5.8× slower | **winning** |
| cmdsub $(true) | fastest | 2.9× slower | 2× slower | **winning** |
| cmdsub $(/bin/true) | 0.83× bash | | 0.67× dash | losing |
| pipeline 3-stage | 1.09× bash | | 0.91× dash | close |
| configure | 14.1s (was 19.8) | 7.1s | 6.4s | 2× gap, closing |

## Invariants (do not break)

- Batching semantics pinned by `tests/scripts/40_batch_semantics.sh`.
- Arena gate: ON only around main-cycle parse_tokens; anything that frees
  AST/token/full_word/children memory MUST route through parena_free.
- Plain-word fast path: split_eligible stays false; full_word stays NULL.
- Function bodies / eval trees are always heap (clone_ast/deep_clone_ast
  keep vec_push; the gate is closed during exec_string parses).
- ASan (SAFE=1) is the free-routing oracle; verify_alloc.sh proves both
  heaps.
