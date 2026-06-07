# syntax=docker/dockerfile:1
# ============================================================================
# Build hellish from source AND install zsh, so the zsh-style two-argument
# `cd old new` extension can be diffed against the shell it imitates. The host
# has no zsh; this image is its oracle.
#
#   docker build -f docker/Dockerfile.zsh -t hellish:zsh .
#   docker run --rm hellish:zsh            # runs tests/cd_zsh_compare.sh
# or simply:  make cd-zsh-test
# ============================================================================
FROM debian:stable-slim

RUN set -eux; \
	apt-get update; \
	apt-get install -y --no-install-recommends \
		build-essential libreadline-dev git ca-certificates bash zsh; \
	rm -rf /var/lib/apt/lists/*

WORKDIR /hellish
COPY . .

# Optimised, libc-allocator binary: the most portable combination.
RUN make fclean >/dev/null 2>&1 || true; \
	make OPT=1 SAFE=1; \
	./build/bin/hellish -c 'echo "hellish built OK for zsh-compare"'

ENV HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_BANNER=1
CMD ["bash", "tests/cd_zsh_compare.sh"]
