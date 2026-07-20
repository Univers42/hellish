# Known issues surfaced by the bench harness

## autoconf `configure` does not complete under hellish

Running GNU hello 2.12.1's `./configure` with `CONFIG_SHELL=hellish`
aborts with status 77 (dash/bash complete in ~7s). This is the classic
real-world shell torture test and it exposes **two independent bugs**, one
fixed and one open.

### 1. FIXED — internal fd saves collided with script fds 5/6/7

hellish's redirect save/restore backed up std fds with plain `dup()`,
which returns the lowest free descriptor (5, 6, 7…). autoconf keeps
`config.log` on fd 5 and the original stdout on fd 6 (`exec 5>>config.log`,
`exec 6>&1`), so every internal save clobbered them and `>&5`/`>&6` failed
with EBADF. Fixed by routing saves through `save_fd()`
(`fcntl(F_DUPFD_CLOEXEC, 10)`, `src/execution/utils.c`) — the standard
shell technique. This moved configure from rc 77 in 0.6s to running the
full compiler-probe sequence.

### 2. OPEN — parser state drifts on autoconf-scale scripts

`hellish --posix -n configure` (parse only, no execution) reports
`syntax error near unexpected token '('` at several lines (first around
4756 / 7673 depending on run). Confirmed characteristics:

- **Not reproducible in isolation.** Every suspected construct extracted
  on its own parses correctly: `cat file - <<EOF >out` (heredoc followed
  by an output redirect), `case "(($x" in *\"*|*\`*|*\\*)` (autoconf's
  quote-detection case), `case x in (x)` (leading-paren pattern), escaped
  backticks inside double quotes, multi-line double-quoted strings with
  `\`config.log'`. All match bash byte-for-byte.
- **Only manifests in aggregate**, which — together with the drift between
  the reported line number and the actual construct — points to a
  **heredoc line-counting / body-consumption bug**: one heredoc variant
  (configure uses many: `cat <<X`, `cat >f <<X`, `cat f - <<X >g`) is
  consumed by the wrong number of lines, so every command after it is
  parsed against the wrong input offset.
- Prefix bisection (`head -N | hellish -n`) is unreliable here because the
  cuts land mid-heredoc / mid-string and inject their own false errors.

### Recommended next step (for a focused session)

Instrument the parser, not the script: dump the token stream / heredoc
gather decisions (`--debug=lexer --debug=parser`) while running the real
configure, and diff hellish's heredoc-body line spans against the file.
The first heredoc whose consumed span differs from its true
`<<DELIM…DELIM` extent is the culprit. Fix is expected in
`src/heredoc/` (body extraction / line accounting), after which the
config.log flow and the downstream `(` errors should both clear, and the
perf `configure` dimension can time hellish alongside bash/dash.

Until then the perf harness excludes hellish from the configure timing and
flags it N/A (see `bench/run.sh` completion gate and `results.md`).
