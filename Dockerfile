# hellish — a fast, POSIX-compliant shell, with horns.
#
# Binary-only image: the prebuilt `hellish` binary on a glibc-matching base
# (Ubuntu 24.04 == the release build host). Build the binary first:
#     make OPT=1 all
# then:
#     docker build -t <login>/hellish-shell:latest .
#     docker run --rm -it <login>/hellish-shell

FROM ubuntu:24.04

LABEL org.opencontainers.image.title="hellish" \
      org.opencontainers.image.description="A fast, POSIX-compliant shell, with horns." \
      org.opencontainers.image.source="https://github.com/Univers42/42sh" \
      org.opencontainers.image.licenses="MIT"

# libreadline8 + libtinfo6 (line editing), ca-certificates + curl (so the
# in-shell `update` command can reach GitHub from inside the container).
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
        libreadline8 ca-certificates curl \
 && rm -rf /var/lib/apt/lists/*

COPY build/bin/hellish /usr/local/bin/hellish

RUN useradd --create-home --shell /usr/local/bin/hellish hellish
USER hellish
WORKDIR /home/hellish

ENTRYPOINT ["/usr/local/bin/hellish"]
