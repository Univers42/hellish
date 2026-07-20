# Known issues surfaced by the bench harness

## RESOLVED — autoconf `configure` completes (2026-07-21)

GNU hello 2.12.1's `./configure` with `CONFIG_SHELL=hellish` now runs
end-to-end: exit 0, `config.status` and `Makefile` generated. The saga,
for archaeology (each stage is pinned by a case in `tests/regress_hellish`):

1. **fd saves collided with script fds 5/6/7** — fixed earlier via
   `save_fd()` (`fcntl(F_DUPFD_CLOEXEC, 10)`).
2. **Quote-state desync at scale** — the cycle-completeness check
   tokenized the raw accumulated input where heredoc BODIES still sat
   inline; apostrophes in embedded C code inverted the tokenizer's quote
   parity and a giant `gl_mda_defines='…'` assignment was cut mid-string.
   The check now tokenizes the body-stripped text (`split_heredocs`).
3. **`config.guess` SEGV** — the `$(…)` boundary scans in the reparser
   and the expander counted parens inside quotes; sed scripts like
   `'s/[-(].*//'` derailed the depth. Both now skip quoted spans
   (`sh_skip_quoted`).
4. **`config.status` mis-lex** — a backtick substitution inside double
   quotes was not scanned atomically, so a `""` within it closed the
   outer quote. `advance_dquoted` now recurses into `advance_backtick`.
5. **`for` over positionals crashed** — the loop read `$N` lazily while
   the body's `shift` shrank `$#`. It now iterates a snapshot of `"$@"`
   taken at loop entry (also the POSIX-required semantics).

## OPEN — 64-byte `realloc_to` leak in full-ASan configure runs

One 64-byte vector leaks somewhere in a child during an ASan-instrumented
configure run (`CONFIG_SHELL=<debug build> …`; grep the log for
LeakSanitizer). All test suites and `verify_alloc.sh` are leak-clean, so
the escape path is configure-specific. Chase with
`ASAN_OPTIONS=fast_unwind_on_malloc=0:malloc_context_size=20` for full
frames. Cosmetic (OPT builds carry no ASan), but it makes ASan configure
runs exit 1.
