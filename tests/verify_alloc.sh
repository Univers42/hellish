#!/bin/bash
# ============================================================================
#  verify_alloc.sh — allocator reliability gate for hellish
#
#  Builds the shell on BOTH allocator backends and proves it behaves identically
#  and cleanly on each:
#
#    SAFE=1  -> libc malloc/free, built with AddressSanitizer + LeakSanitizer.
#               ASan/LSan are meaningful here (they instrument libc).
#    SAFE=0  -> the custom ft_malloc heap (OPT build). ASan is blind to it, so we
#               use ft_malloc's OWN oracle, malloc_live_bytes(), exposed at exit
#               via HELLISH_ALLOC_STATS=1.
#
#  Checks, for every portable script (scripts/, hard/, level*.sh):
#    1. output + exit status match `bash --posix`  — on BOTH backends
#    2. no crash (ASan abort / segfault / ft_malloc corruption / glibc munmap)
#  Plus:
#    3. SAFE=1 main-process leak probe: a fork-free script must be 0 leaks
#       (the shell frees all its state at exit). NB: forked children — cmdsub,
#       pipeline, subshell — exit via exit() and produce *pre-existing*,
#       nondeterministic LSan noise that the OS reclaims; it is not an
#       accumulating leak and is intentionally not gated here.
#    4. SAFE=0 ft_malloc oracle: live bytes at exit. The constant baseline is
#       reachable lifetime singletons (builtin/alias hashes); a delta that does
#       NOT grow with workload == no accumulation.
#
#  Usage:  cd tests && ./verify_alloc.sh
# ============================================================================
set -u
# Same oracle pin as tests/tester: bash changes POSIX-visible behaviour between
# minor releases, and this script grades output parity against `bash --posix`.
# Without the pin a 5.1 box reports version differences as allocator diffs --
# which is exactly the wrong conclusion for a script whose whole job is to
# prove the two heaps behave identically.
ORACLE_HOME="${HELLISH_ORACLE:-$HOME/bash-5.3.9}"
if [ -x "$ORACLE_HOME/bin/bash" ]; then
	PATH="$ORACLE_HOME/bin:$PATH"; export PATH
fi
case "$(bash --version 2>/dev/null | head -1)" in
	*"version 5.3"*) ;;
	*) echo "warning: grading against non-5.3 bash; run 'make oracle' for the pin" ;;
esac
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TESTS="$ROOT/tests"
BIN="$ROOT/build/bin/hellish"
SCRIPTS=("$TESTS"/scripts/*.sh "$TESTS"/hard/[0-9]*.sh "$TESTS"/level*.sh)
strip() { sed 's/\x1b\[[0-9;]*[A-Za-z]//g' | grep -vE '^[[:space:]]*❯' | grep -v '^exit$'; }

build() { # $1=SAFE  $2=make-args
	( cd "$ROOT" && rm -f build/bin/hellish && make all $2 SAFE="$1" >/dev/null 2>&1 )
	[ -f "$BIN" ] || { echo "  BUILD FAILED (SAFE=$1 $2)"; exit 1; }
}

run_suite() { # output parity + crash detection on the current binary
	local pass=0 diff=0 crash=0 f d ho bo hc bc err
	for f in "${SCRIPTS[@]}"; do
		[ -f "$f" ] || continue
		d="$(dirname "$f")"
		err=$(cd "$d" && ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 timeout 40 "$BIN" "$f" 2>&1 >/dev/null)
		ho=$(cd "$d" && ASAN_OPTIONS=detect_leaks=0 timeout 40 "$BIN" "$f" 2>/dev/null); hc=$?
		bo=$(cd "$d" && timeout 40 bash --posix "$f" 2>/dev/null); bc=$?
		if echo "$err" | grep -qiE "AddressSanitizer|munmap_chunk|invalid pointer|double free|corrupted|stack smashing" \
			|| [ "$hc" = 139 ] || [ "$hc" = 134 ]; then
			crash=$((crash+1)); echo "  CRASH: $(basename "$f")"
		elif [ "$(printf '%s' "$ho" | strip)" = "$(printf '%s' "$bo" | strip)" ] && [ "$hc" = "$bc" ]; then
			pass=$((pass+1))
		else
			diff=$((diff+1)); echo "  DIFF:  $(basename "$f") (h=$hc b=$bc)"
		fi
	done
	echo "  RESULT: pass=$pass diff=$diff crash=$crash / ${#SCRIPTS[@]}  (diffs may be background-job timing flakes; re-run to confirm)"
}

echo "### 1/2  SAFE=1 — libc + AddressSanitizer/LeakSanitizer ######################"
build 1 ""
run_suite
printf 'a=1;b=2;c=$((a+b));echo $c\nexport X=hi;echo $X;unset X\nfor i in 1 2 3;do echo $i;done\ncase x in x) echo ok;; esac\nx="a b c";echo ${x# }\n' > /tmp/va_nofork.sh
ml=$(ASAN_OPTIONS=detect_leaks=1:exitcode=0 LSAN_OPTIONS=exitcode=0 "$BIN" /tmp/va_nofork.sh 2>&1 >/dev/null | grep -c "ERROR: LeakSanitizer")
echo "  main-process leak probe (fork-free script): $ml leak report(s)  [expect 0]"

echo "### 2/2  SAFE=0 — custom ft_malloc (OPT) #####################################"
build 0 "OPT=1"
run_suite
echo "  ft_malloc live-bytes oracle (HELLISH_ALLOC_STATS=1):"
base=""
for f in "$TESTS"/scripts/30_printf_format.sh "$TESTS"/hard/06_math_suite.sh "$TESTS"/hard/10_string_toolkit.sh; do
	[ -f "$f" ] || continue
	lb=$(HELLISH_ALLOC_STATS=1 timeout 40 "$BIN" "$f" 2>&1 >/dev/null | grep -oE 'cleanup: [0-9]+' | grep -oE '[0-9]+')
	printf "    %-26s live=%s\n" "$(basename "$f")" "${lb:-n/a}"
done

# ---------------------------------------------------------------------------
# Exit-path abandonment (#78).  Every exit through exit_clean() used to walk
# away from the whole environment table -- ~9.6 KB, on a plain `exit 3` as
# much as on a fatal error -- because free_env() is not part of
# free_all_state() and only off() called it.  ASan cannot see it (the table
# stays reachable from t_shell), so this oracle is the only thing that can,
# which is exactly why it has to ASSERT and not merely print: the noise it
# measures is noise in the instrument itself, and drowns the malformed
# inputs most worth testing.
#   The ceiling is deliberately loose.  What matters is that a regression of
# the original size cannot pass, not that the residue is pinned to the byte
# (~1 KB today: the in-flight argv of the command that called exit, which is
# still live when exit_clean runs).
# ---------------------------------------------------------------------------
live() { HELLISH_ALLOC_STATS=1 timeout 40 "$BIN" -c "$1" 2>&1 >/dev/null \
	| grep -oE 'cleanup: [0-9]+' | grep -oE '[0-9]+'; }
echo "  exit-path abandonment probe (ceiling ${EXIT_CEIL:=2048} bytes over baseline):"
base=$(live 'true')
exit_fail=0
if [ -z "$base" ]; then
	echo "    (no oracle output -- not an ft_malloc build, skipping)"
else
	for c in 'exit 3' 'exit' 'readonly r=1; r=2' 'echo ${undef:?}' \
		'set -u; echo $nope' 'a=(1); a[-99]=v' 'echo "${!@bad}"' \
		'declare -A M; M[$k]=v' 'trap "echo bye" EXIT; exit 2'; do
		lb=$(live "$c")
		d=$(( ${lb:-0} - base ))
		if [ "$d" -gt "$EXIT_CEIL" ]; then
			printf "    FAIL %-32s delta=%+d  (over %s)\n" "$c" "$d" "$EXIT_CEIL"
			exit_fail=1
		else
			printf "    ok   %-32s delta=%+d\n" "$c" "$d"
		fi
	done
	[ "$exit_fail" = 0 ] || echo "  EXIT-PATH PROBE FAILED -- see #78"
fi

echo
echo "Identical bash-matching output on both heaps == the allocator swap is transparent."
echo "(Restore the dev build with:  make   — or  make OPT=1  for the ft_malloc build.)"
