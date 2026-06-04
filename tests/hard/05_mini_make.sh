#!/bin/sh
# A tiny "make": targets with prerequisites and a simulated mtime map; computes
# build order (recursive DFS), decides what is stale, "builds" it, updates the
# clock. Exercises: recursion, functions, case, command substitution, arithmetic,
# global state via vars, set -e behaviour with explicit checks.
set -u

# dependency graph (target:dep1 dep2 ...)
deps_of() {
	case $1 in
		app)    echo "main.o util.o net.o" ;;
		main.o) echo "main.c common.h" ;;
		util.o) echo "util.c common.h" ;;
		net.o)  echo "net.c common.h net.h" ;;
		*)      echo "" ;;
	esac
}

is_source() {
	case $1 in
		*.c|*.h) return 0 ;;
		*) return 1 ;;
	esac
}

# simulated mtimes (logical clock). sources start older than clock.
clock=100
mtime_main_c=10
mtime_util_c=20
mtime_net_c=15
mtime_common_h=5
mtime_net_h=8
mtime_main_o=0
mtime_util_o=0
mtime_net_o=0
mtime_app=0

mtime_get() { eval "echo \${mtime_$(echo "$1" | tr '.' '_')}"; }
mtime_set() { eval "mtime_$(echo "$1" | tr '.' '_')=$2"; }

built_count=0

# DFS post-order to compute build order, avoiding duplicates
visited=""
seen() { case " $visited " in *" $1 "*) return 0 ;; *) return 1 ;; esac; }

order=""
dfs() {
	node=$1
	seen "$node" && return 0
	visited="$visited $node"
	for d in $(deps_of "$node"); do
		dfs "$d"
	done
	order="$order $node"
}

needs_rebuild() {
	t=$1
	is_source "$t" && return 1
	tt=$(mtime_get "$t")
	for d in $(deps_of "$t"); do
		dt=$(mtime_get "$d")
		if [ "$dt" -gt "$tt" ]; then return 0; fi
	done
	[ "$tt" -eq 0 ] && return 0
	return 1
}

build() {
	t=$1
	clock=$(( clock + 1 ))
	mtime_set "$t" "$clock"
	built_count=$(( built_count + 1 ))
	printf 'BUILD %-8s (clock=%d) from:%s\n' "$t" "$clock" "$(deps_of "$t")"
}

do_make() {
	target=$1
	order=""
	visited=""
	dfs "$target"
	printf 'order:%s\n' "$order"
	for t in $order; do
		if is_source "$t"; then
			printf 'skip  %-8s (source, mtime=%s)\n' "$t" "$(mtime_get "$t")"
		elif needs_rebuild "$t"; then
			build "$t"
		else
			printf 'up2date %-6s (mtime=%s)\n' "$t" "$(mtime_get "$t")"
		fi
	done
}

echo "=== first build (everything stale) ==="
do_make app
echo "built=$built_count"

echo "=== second build (nothing changed) ==="
built_count=0
do_make app
echo "built=$built_count"

echo "=== touch util.c, rebuild ==="
built_count=0
clock=$(( clock + 1 ))
mtime_set util.c "$clock"
echo "touched util.c -> $(mtime_get util.c)"
do_make app
echo "built=$built_count"

echo "=== touch common.h (affects all .o), rebuild ==="
built_count=0
clock=$(( clock + 1 ))
mtime_set common.h "$clock"
echo "touched common.h -> $(mtime_get common.h)"
do_make app
echo "built=$built_count"

echo "=== final mtimes ==="
for t in main.o util.o net.o app; do
	printf '%-8s %s\n' "$t" "$(mtime_get "$t")"
done
echo "done clock=$clock"
