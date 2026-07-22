# hellish backlog

Living tracker for the "fastest and most complete shell" program. Every item
lands only with: `make test` green (2878 golden cases), `tests/hard` 16/16,
`tests/scripts` diffs clean vs bash --posix, `verify_alloc.sh` green on both
heaps, and the `make conformance` baseline gate not regressing (Oils / mksh
pass counts in `bench/baseline/`). Performance claims come from
`make perf` / hyperfine on the OPT build only.

## Done (this program)

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
