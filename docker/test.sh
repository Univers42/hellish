#!/usr/bin/env bash
# ============================================================================
# docker/test.sh -- build hellish from source in every distro container and run
# a smoke test, printing a pass/fail table. This is the reproducible "does it
# build and run on a clean machine?" check, independent of your host.
#
#   make docker-test            # all distros
#   docker/test.sh alpine arch  # just these
#
# Services live in docker-compose.yml; the list below is every one of them.
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
[ ${#distros[@]} -eq 0 ] && distros=(alpine debian ubuntu ubuntu2204 arch
	fedora rocky opensuse void alpine-clang debian-clang alpine-ftmalloc)

pass=0; fail=0; summary=""
for d in "${distros[@]}"; do
	printf '\n\033[1;36m═══ %s : build from source ═══\033[0m\n' "$d"
	# docker/smoke.sh is the same 40-check portability workout every CI
	# platform job runs, so a distro failure here and a CI failure there
	# mean the same thing and are read the same way.
	if docker compose build "$d" \
		&& docker compose run --rm "$d" bash docker/smoke.sh; then
		printf '\033[1;32m✓ %s OK\033[0m\n' "$d"; pass=$((pass + 1)); summary="$summary  $d:OK"
	else
		printf '\033[1;31m✗ %s FAILED\033[0m\n' "$d"; fail=$((fail + 1)); summary="$summary  $d:FAIL"
	fi
done

printf '\n\033[1m═══ docker distro matrix: %d ok / %d fail ═══\033[0m\n%s\n' \
	"$pass" "$fail" "$summary"
[ "$fail" -eq 0 ]
