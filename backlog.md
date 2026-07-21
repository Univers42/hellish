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
- [ ] REMAINING to BEAT bash (~13ms): reparse dquote/envvar families
      still do the t_reparser copy dance; t_token is 40B (750k in the
      deque ≈ 30MB touched — shrink to 24B?); sys 8.7ms vs bash 2.1.
      Re-measure on a QUIET machine before claiming anything.
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
