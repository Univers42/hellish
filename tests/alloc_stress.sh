#!/bin/sh
# Parse-arena chunk-boundary stress (issue #94's detector).
#
# The arena's rare states -- a new chunk opening mid-parse, the chunk
# registry filling up and parena_alloc falling back to the heap -- need
# hundreds of megabytes of parse allocations to reach at the default
# 256KB..8MB chunk sizes, which is why the #94 double free shipped: no
# normal-size test could ever visit those states. This script makes the
# rare states the common case instead: it rebuilds the debug/ASan binary
# with 512-byte chunks (PARENA_* are override-able in incs/parena.h for
# exactly this) so every few nodes cross a chunk boundary and the
# registry exhausts within one screenful of script. Any misrouted free
# then dies loudly under ASan on a corpus of rc-shaped scripts.
#
# Cost: two full rebuilds (the stress build here, and your next `make`
# after the fclean below -- the object tree is keyed on MODE, not on
# EXTRA_CFLAGS, so contaminated objects must not survive this script).
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT" || exit 1
tmp=$(mktemp -d)
cleanup()
{
	rm -rf "$tmp"
	make -s fclean >/dev/null 2>&1
	echo "note: object tree cleaned (stress flags) — next make rebuilds"
}
trap cleanup EXIT INT TERM

echo "== arena stress: building with 512-byte chunks + ASan =="
if ! make re EXTRA_CFLAGS='-DPARENA_FIRST_CHUNK=512 -DPARENA_MAX_CHUNK=2048' \
		-j"$(nproc 2>/dev/null || echo 4)" >"$tmp/build.log" 2>&1; then
	echo "FAIL: stress build failed"; tail -20 "$tmp/build.log"; exit 1
fi
H=$ROOT/build/bin/hellish
export HELLISH_BANNER=0 HELLISH_NO_ANIM=1 HELLISH_NO_UPDATE_CHECK=1

fail=0
run_one()
{
	# $1=label $2=script; -n first (parse+teardown only), then executing
	for flag in -n ""; do
		# shellcheck disable=SC2086
		if ! timeout 180 "$H" $flag "$2" >"$tmp/out" 2>&1; then
			echo "FAIL: $1 flags='$flag' exit=$?"
			grep -m1 "ERROR: AddressSanitizer" "$tmp/out" || tail -3 "$tmp/out"
			fail=1
		fi
	done
}

echo "== generated rc corpus (deterministic) =="
for mode in plain dense; do
	for seed in 1 3 7; do
		python3 tools/gen_rc_corpus.py "$mode" "$seed" 300 >"$tmp/rc.sh"
		run_one "gen $mode seed=$seed" "$tmp/rc.sh"
	done
done

echo "== shipped rc-shaped files =="
for f in hellishrc.example share/rc.d/*.hsh share/themes/*.hsh; do
	[ -f "$f" ] || continue
	if ! timeout 60 "$H" -n "$f" >"$tmp/out" 2>&1; then
		echo "FAIL: -n $f exit=$?"
		grep -m1 "ERROR: AddressSanitizer" "$tmp/out" || tail -3 "$tmp/out"
		fail=1
	fi
done

if [ "$fail" -eq 0 ]; then
	echo "arena stress: all clean"
else
	echo "arena stress: FAILURES (see above)"
fi
exit "$fail"
