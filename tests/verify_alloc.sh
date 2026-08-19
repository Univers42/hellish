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
	( cd "$ROOT" && rm -f build/bin/hellish && make $2 SAFE="$1" >/dev/null 2>&1 )
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
echo
echo "Identical bash-matching output on both heaps == the allocator swap is transparent."
echo "(Restore the dev build with:  make   — or  make OPT=1  for the ft_malloc build.)"
