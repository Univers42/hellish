# syntax=docker/dockerfile:1
# ============================================================================
# Build + run hellish FROM SOURCE in a clean, dependency-complete container.
# The base image is parameterised so one recipe covers Alpine (musl), Debian,
# Ubuntu and Arch -- this is how you prove the shell builds and runs without
# depending on whatever happens to be installed on your host.
#
#   docker compose build                 # all distros
#   docker compose run --rm alpine       # interactive hellish on Alpine
#   make docker-test                     # build + smoke-test every distro
# ============================================================================
ARG BASE=debian:stable-slim
FROM ${BASE}

# Minimal build + runtime deps for whichever distro this is: a C toolchain
# (gcc + make), GNU readline and its ncurses dependency (line editing), git,
# and bash (the test scripts use it). This is the whole point of the image --
# nobody has to chase "fatal error: readline/readline.h: No such file".
RUN set -eux; \
	if command -v apk >/dev/null 2>&1; then \
		apk add --no-cache build-base readline-dev ncurses-dev git bash coreutils; \
	elif command -v apt-get >/dev/null 2>&1; then \
		apt-get update; \
		apt-get install -y --no-install-recommends \
			build-essential libreadline-dev git ca-certificates bash; \
		rm -rf /var/lib/apt/lists/*; \
	elif command -v pacman >/dev/null 2>&1; then \
		pacman -Sy --noconfirm --needed base-devel readline git bash; \
	else \
		echo "unsupported base image: need apk, apt-get or pacman" >&2; exit 1; \
	fi

WORKDIR /hellish
COPY . .

# Build the optimised, libc-allocator binary -- the most portable combination
# (no AddressSanitizer, no custom heap) so it compiles cleanly on glibc AND
# musl. Override to exercise the custom allocator:
#   docker compose build --build-arg BUILD_FLAGS="OPT=1 SAFE=0"
ARG BUILD_FLAGS="OPT=1 SAFE=1"
RUN make fclean >/dev/null 2>&1 || true; \
	make ${BUILD_FLAGS}; \
	./build/bin/hellish -c 'echo "hellish built OK on $(uname -s) $(uname -m)"'

ENV HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_BANNER=1
# Default: an interactive hellish session. Override with e.g.
#   docker compose run --rm alpine ./build/bin/hellish -c 'echo hi'
CMD ["./build/bin/hellish"]
