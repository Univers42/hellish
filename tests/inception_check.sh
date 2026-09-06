#!/usr/bin/env bash
# ============================================================================
# tests/inception_check.sh -- hellish runs Inception's scripts the way sh does.
#
# Inception (tests/inception, a submodule) is the project born2root's VM
# exists to run: a docker-compose stack whose Makefile, entrypoints and test
# suite are POSIX sh. Inside the VM, `make` from hellish now runs all of it
# under hellish (the Makefile picks the shell it was launched from). This
# checks that dialect on the host, against dash -- the /bin/sh those scripts
# were written for -- and bash:
#
#   1. every *.sh parses under `hellish -n` exactly when it parses under
#      `dash -n` and `bash -n`;
#   2. tests/compliance.sh, Inception's own 1400-line audit, prints the same
#      report and exits with the same status under dash and under hellish.
#      With the stack down its runtime checks are skipped, so what runs is
#      the static audit of the repository (--no-clone: no network);
#   3. `make -n up` and `make -n test` launched FROM hellish pick hellish as
#      the Makefile's SHELL and print the same plan as when launched from
#      bash.
#
# This is the corpus that found printf truncating a long %s at 4096 bytes:
# the compliance suite counted 7 services where dash counted 8.
#
# Skips cleanly (exit 0, with a notice) when the submodule is not checked out:
#     git submodule update --init tests/inception
# ============================================================================
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
H="${HELLISH_BIN:-$ROOT/build/bin/hellish}"
INC="${INCEPTION_DIR:-$ROOT/tests/inception}"

if [ ! -f "$INC/Makefile" ]; then
	echo "inception: submodule not checked out (git submodule update --init tests/inception) -- skipping"
	exit 0
fi
if [ ! -x "$H" ]; then
	echo "error: hellish not built at $H (run 'make' first)" >&2
	exit 2
fi
DASH="$(command -v dash || true)"
BASH_BIN="$(command -v bash)"
H="$(cd "$(dirname "$H")" && pwd)/$(basename "$H")"

OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
export ASAN_OPTIONS="detect_leaks=1:abort_on_error=0:exitcode=0"
export LSAN_OPTIONS="exitcode=0"
fail=0

# ---- 1. parse sweep --------------------------------------------------------
total=0; bad=0
while IFS= read -r f; do
	total=$((total + 1))
	timeout 20 "$BASH_BIN" -n "$f" >/dev/null 2>&1; brc=$?
	drc=$brc
	[ -n "$DASH" ] && { timeout 20 "$DASH" -n "$f" >/dev/null 2>&1; drc=$?; }
	timeout 20 "$H" -n "$f" >/dev/null 2>"$OUT/perr"; hrc=$?
	if [ "$brc" != "$hrc" ] || [ "$drc" != "$hrc" ]; then
		bad=$((bad + 1))
		echo "PARSE MISMATCH  dash=$drc bash=$brc hellish=$hrc  ${f#$INC/}"
		head -3 "$OUT/perr" | sed 's/^/    /'
	fi
done < <(find "$INC" -name '*.sh' -type f -not -path '*/.git/*' | sort)
echo "---- parse: $((total - bad)) ok / $bad mismatch (of $total scripts) ----"
[ "$bad" -eq 0 ] || fail=1

# ---- 2. the compliance suite, dash vs hellish -----------------------------
# Same environment for both; hellish is first on PATH for its side so the
# Makefile's shell probe (step 3) and anything the suite re-launches see it.
run_in() { # run_in <shell> <path-prefix> <cmd...>
	local sh="$1" prefix="$2"; shift 2
	( cd "$INC" && env -i PATH="$prefix$PATH" HOME="$OUT/home" TERM=dumb \
		NO_COLOR=1 COLUMNS=80 LC_ALL=C \
		HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_ANIM=1 \
		timeout 600 "$sh" "$@" </dev/null )
}
mkdir -p "$OUT/home"
ok=0; ko=0
REF="$BASH_BIN"; refname=bash
[ -n "$DASH" ] && { REF="$DASH"; refname=dash; }
run_in "$REF" "" tests/compliance.sh --no-clone >"$OUT/c.ref" 2>/dev/null; rrc=$?
run_in "$H" "$(dirname "$H"):" tests/compliance.sh --no-clone >"$OUT/c.h" 2>"$OUT/c.herr"; hrc=$?
leak=""
grep -qE 'AddressSanitizer|LeakSanitizer' "$OUT/c.herr" && leak="  (sanitizer report in hellish stderr)"
if [ "$rrc" = "$hrc" ] && cmp -s "$OUT/c.ref" "$OUT/c.h" && [ -z "$leak" ]; then
	ok=$((ok + 1)); printf 'ok    %-44s rc=%s (%s lines)\n' "tests/compliance.sh --no-clone" "$hrc" "$(wc -l <"$OUT/c.h")"
else
	ko=$((ko + 1)); printf 'FAIL  %-44s %s rc=%s hellish rc=%s%s\n' "tests/compliance.sh --no-clone" "$refname" "$rrc" "$hrc" "$leak"
	diff "$OUT/c.ref" "$OUT/c.h" | head -12 | sed 's/^/    /'
	grep -E 'AddressSanitizer|LeakSanitizer|hellish:' "$OUT/c.herr" | head -4 | sed 's/^/    stderr: /'
fi

# ---- 3. the Makefile's shell probe, launched from hellish ------------------
# The test recipe spells out the shell it picked, so that word is normalised
# before the plans are compared; which shell hellish's launch picked is
# asserted separately below.
for tgt in up test; do
	run_in "$BASH_BIN" "" -c "make -n $tgt" 2>&1 \
		| sed -E 's#^/[^ ]+ tests/(compliance|bench)\.sh#$SH tests/\1.sh#' >"$OUT/mk.$tgt.b"
	run_in "$H" "$(dirname "$H"):" -c "make -n $tgt" 2>&1 \
		| sed -E 's#^/[^ ]+ tests/(compliance|bench)\.sh#$SH tests/\1.sh#' >"$OUT/mk.$tgt.h"
	if cmp -s "$OUT/mk.$tgt.b" "$OUT/mk.$tgt.h"; then
		ok=$((ok + 1)); printf 'ok    %-44s (%s lines)\n' "make -n $tgt, same plan from both shells" "$(wc -l <"$OUT/mk.$tgt.h")"
	else
		ko=$((ko + 1)); printf 'FAIL  %-44s\n' "make -n $tgt differs by launcher"
		diff "$OUT/mk.$tgt.b" "$OUT/mk.$tgt.h" | head -8 | sed 's/^/    /'
	fi
done
picked="$(run_in "$H" "$(dirname "$H"):" -c 'make -pn up 2>/dev/null' | sed -n 's/^SHELL := //p' | head -1)"
if [ "$picked" = "$H" ]; then
	ok=$((ok + 1)); printf 'ok    %-44s\n' "make picks hellish as SHELL"
else
	ko=$((ko + 1)); printf 'FAIL  %-44s picked %s\n' "make picks hellish as SHELL" "${picked:-nothing}"
fi
echo "---- run: $ok ok / $ko FAIL (of $((ok + ko)) cases vs $refname) ----"
[ "$ko" -eq 0 ] || fail=1
exit "$fail"
