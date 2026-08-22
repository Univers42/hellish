# ============================================================================
# Build + run hellish FROM SOURCE in a clean, dependency-complete container.
# The base image and the compiler are parameterised so one recipe covers
# every distro we claim to support -- glibc and musl, apt and apk and pacman
# and dnf and zypper and xbps, gcc and clang. This is how you prove the shell
# builds and runs without depending on whatever happens to be on your host.
#
#   docker compose build                 # all distros
#   docker compose run --rm alpine       # interactive hellish on Alpine
#   make docker-test                     # build + smoke-test every distro
#   docker build -f docker/Dockerfile --build-arg BASE=fedora:41 \
#                --build-arg CC=clang .  # one distro, one compiler
# ============================================================================
ARG BASE=debian:stable-slim
FROM ${BASE}

# Minimal build + runtime deps for whichever distro this is: a C toolchain
# (gcc + make), GNU readline and its ncurses dependency (line editing), git,
# and bash (the test scripts use it). This is the whole point of the image --
# nobody has to chase "fatal error: readline/readline.h: No such file".
#
# findutils is not decoration: the Makefile discovers its sources with
# `find`, and openSUSE Tumbleweed's base image does not ship one. Without it
# the build produced no source list at all and stopped at "cc: fatal error:
# no input files", which points nowhere near the cause.
# The compiler is a build arg so the same image recipe covers gcc AND clang
# on every distro -- a warning is a build failure here (-Werror), and the two
# front ends do not warn about the same things.
ARG CC=cc

RUN set -eux; \
	if command -v apk >/dev/null 2>&1; then \
		apk add --no-cache build-base readline-dev ncurses-dev git bash \
			coreutils python3 clang; \
	elif command -v apt-get >/dev/null 2>&1; then \
		apt-get update; \
		DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
			build-essential libreadline-dev git ca-certificates bash \
			python3 clang; \
		rm -rf /var/lib/apt/lists/*; \
	elif command -v pacman >/dev/null 2>&1; then \
		pacman -Sy --noconfirm --needed base-devel readline git bash \
			python clang; \
	elif command -v dnf >/dev/null 2>&1; then \
		dnf -y install gcc clang make readline-devel ncurses-devel git \
			bash python3 diffutils findutils which; \
		dnf clean all; \
	elif command -v zypper >/dev/null 2>&1; then \
		zypper --non-interactive --gpg-auto-import-keys refresh; \
		zypper --non-interactive install gcc clang make readline-devel \
			ncurses-devel git-core bash python3 tar gzip which \
			findutils diffutils; \
		zypper clean --all; \
	elif command -v xbps-install >/dev/null 2>&1; then \
		xbps-install -Suy xbps; \
		xbps-install -y gcc clang make readline-devel ncurses-devel git \
			bash python3 which findutils diffutils; \
	else \
		echo "unsupported base image: need apk, apt-get, pacman, dnf," \
			"zypper or xbps-install" >&2; exit 1; \
	fi

WORKDIR /hellish
COPY . .

# Build the optimised, libc-allocator binary -- the most portable combination
# (no AddressSanitizer, no custom heap) so it compiles cleanly on glibc AND
# musl. Override to exercise the custom allocator:
#   docker compose build --build-arg BUILD_FLAGS="OPT=1 SAFE=0"
ARG BUILD_FLAGS="OPT=1 SAFE=1"
RUN make fclean >/dev/null 2>&1 || true; \
	printf '\n=== building with %s on %s ===\n' "$CC" "$(uname -sm)"; \
	$CC --version | head -1; \
	make all ${BUILD_FLAGS} CC=$CC; \
	./build/bin/hellish -c 'echo "hellish built OK on $(uname -s) $(uname -m)"'

ENV HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_BANNER=1
# Default: an interactive hellish session. Override with e.g.
#   docker compose run --rm alpine ./build/bin/hellish -c 'echo hi'
CMD ["./build/bin/hellish"]
