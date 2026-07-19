#!/bin/bash
# Fetch anything bench/ needs that isn't present (suites and workloads are
# gitignored).  Idempotent: only downloads what's missing.  Total footprint
# ~100MB, dominated by the shallow oils clone.
set -eu

BENCH_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$BENCH_DIR"
mkdir -p suites .bin workloads

if [ ! -d suites/oils ]; then
    echo "fetch: oils (shallow)" >&2
    git clone --depth 1 --quiet https://github.com/oils-for-unix/oils suites/oils
    # sh_spec.py + spec/bin helpers are python2; port in place (see README).
    "$BENCH_DIR/lib/port_oils_py3.sh"
fi

if [ ! -d suites/mksh ]; then
    echo "fetch: mksh" >&2
    git clone --depth 1 --quiet https://github.com/MirBSD/mksh suites/mksh
fi

if [ ! -x .bin/hyperfine ] && ! command -v hyperfine >/dev/null; then
    echo "fetch: hyperfine" >&2
    curl -sL -o .bin/hf.tgz https://github.com/sharkdp/hyperfine/releases/download/v1.19.0/hyperfine-v1.19.0-x86_64-unknown-linux-musl.tar.gz
    tar xzf .bin/hf.tgz -C .bin --strip-components=1 --wildcards '*/hyperfine'
    rm .bin/hf.tgz
fi

if [ ! -f workloads/hello-2.12.1/configure ]; then
    echo "fetch: GNU hello (configure workload)" >&2
    curl -sL -o workloads/hello.tgz https://ftp.gnu.org/gnu/hello/hello-2.12.1.tar.gz
    tar xzf workloads/hello.tgz -C workloads
    rm workloads/hello.tgz
fi
