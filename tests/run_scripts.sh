#!/usr/bin/env bash
# ============================================================================
# run_scripts.sh -- run every self-contained test script through hellish AND
# `bash --posix`, from a throwaway working directory, comparing stdout + exit
# status and scanning hellish's stderr for AddressSanitizer / LeakSanitizer
# reports. Exits non-zero if ANY script mismatches bash or leaks.
#
# This is what CI runs and what you should run locally before opening a PR:
#     make            # debug build (SAFE=1, ASan -- leak detection is valid)
#     tests/run_scripts.sh
#
# By default it covers tests/scripts, tests/hard and tests/stress (all
# deterministic, all expected to match bash byte-for-byte). Pass extra script
# paths as arguments to check them too.
# ============================================================================
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
bash "$ROOT/tests/gen_fixtures.sh"
H="${HELLISH_BIN:-$ROOT/build/bin/hellish}"
if [ ! -x "$H" ] && [ -x "$H.exe" ]; then
	H="$H.exe"
fi
if [ ! -x "$H" ]; then
	echo "error: hellish not built at $H (run 'make' first)" >&2
	exit 2
fi

# Grade against the SAME pinned oracle tests/tester uses. Without this the
# corpus was diffed against whatever bash happened to be in PATH, and bash
# changes POSIX-visible behaviour between minor releases: 5.2 pads the `jobs`
# status column to 24 columns where 5.3 pads to 27, and the two disagree on
# when a SIGTERM'd background job is announced. That made
# tests/hard/17_bg_signal_report.sh fail here while passing against the
# pinned 5.3.9 -- a bash-version difference reported as a hellish bug, which
# is exactly the trap `make oracle` exists to close.
ORACLE_HOME="${HELLISH_ORACLE:-$HOME/bash-5.3.9}"
if [ -x "$ORACLE_HOME/bin/bash" ]; then
	PATH="$ORACLE_HOME/bin:$PATH"
	export PATH
fi
ORACLE_VERSION="$(bash --version 2>/dev/null | head -1 \
	| sed 's/.*version \([0-9.]*\).*/\1/')"
case "$ORACLE_VERSION" in
	5.3*) ;;
	*)
		echo "⚠  oracle drift: grading against bash ${ORACLE_VERSION:-unknown}," \
			"not the pinned 5.3.x." >&2
		echo "   Expect phantom failures. Build it once:  make oracle" >&2
		;;
esac

OUT="$(mktemp -d)"
WORK="$(mktemp -d)"
trap 'rm -rf "$OUT" "$WORK"' EXIT
export ASAN_OPTIONS="detect_leaks=1:abort_on_error=0:exitcode=0"
export LSAN_OPTIONS="exitcode=0"
export HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1

scripts=()
for g in "$ROOT"/tests/scripts/*.sh "$ROOT"/tests/hard/[0-9]*.sh \
	"$ROOT"/tests/stress/*.sh "$@"; do
	[ -f "$g" ] && scripts+=("$g")
done

ok=0; bad=0
for s in "${scripts[@]}"; do
	name="$(basename "$s")"
	rm -rf "$WORK"; mkdir -p "$WORK"
	( cd "$WORK" && timeout 90 "$H" "$s" >"$OUT/ho" 2>"$OUT/he" ); hc=$?
	rm -rf "$WORK"; mkdir -p "$WORK"
	( cd "$WORK" && timeout 90 bash --posix "$s" >"$OUT/bo" 2>/dev/null ); bc=$?
	asan=$(grep -cE "AddressSanitizer|LeakSanitizer|runtime error:|SEGV|heap-buffer|use-after" "$OUT/he")
	if diff -q "$OUT/ho" "$OUT/bo" >/dev/null 2>&1 && [ "$asan" -eq 0 ] && [ "$hc" = "$bc" ]; then
		ok=$((ok + 1))
	else
		bad=$((bad + 1))
		printf 'FAIL  %-30s h_rc=%s b_rc=%s asan=%s\n' "$name" "$hc" "$bc" "$asan"
		diff "$OUT/ho" "$OUT/bo" 2>/dev/null | head -10 | sed 's/^/      /'
		[ "$asan" -gt 0 ] && grep -E "Sanitizer|leak of" "$OUT/he" | head -3 | sed 's/^/      /'
	fi
done

printf -- '---- scripts vs bash --posix: %d ok / %d FAIL (of %d) ----\n' \
	"$ok" "$bad" "$((ok + bad))"
[ "$bad" -eq 0 ]
