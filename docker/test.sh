#!/usr/bin/env bash
# ============================================================================
# docker/test.sh -- build hellish from source in every distro container and run
# a smoke test, printing a pass/fail table. This is the reproducible "does it
# build and run on a clean machine?" check, independent of your host.
#
#   make docker-test            # all distros
#   docker/test.sh alpine arch  # just these
#
# Requires docker + the compose plugin (docker compose).
# ============================================================================
set -u
cd "$(dirname "$0")/.."

if ! docker info >/dev/null 2>&1; then
	echo "error: docker daemon not reachable (is it running? do you have perms?)" >&2
	exit 2
fi

distros=("$@")
[ ${#distros[@]} -eq 0 ] && distros=(alpine debian ubuntu ubuntu2204 arch)

# A small but representative workout: externals, arithmetic, a pipeline, a
# heredoc nested in a compound, and command hashing -- if these match, the
# build and the core shell work in that environment.
smoke='echo "uname: $(uname -sm)"; printf "%s\n" "arith=$((6*7))"; echo a b c | tr a-z A-Z; if true; then cat <<EOF
nested-heredoc ok
EOF
fi; ls >/dev/null; type ls'

pass=0; fail=0; summary=""
for d in "${distros[@]}"; do
	printf '\n\033[1;36m═══ %s : build from source ═══\033[0m\n' "$d"
	if docker compose build "$d" \
		&& docker compose run --rm "$d" ./build/bin/hellish -c "$smoke"; then
		printf '\033[1;32m✓ %s OK\033[0m\n' "$d"; pass=$((pass + 1)); summary="$summary  $d:OK"
	else
		printf '\033[1;31m✗ %s FAILED\033[0m\n' "$d"; fail=$((fail + 1)); summary="$summary  $d:FAIL"
	fi
done

printf '\n\033[1m═══ docker distro matrix: %d ok / %d fail ═══\033[0m\n%s\n' \
	"$pass" "$fail" "$summary"
[ "$fail" -eq 0 ]
