#!/usr/bin/env python3
"""Deterministic rc-shaped script generator for the arena stress suite.

Emits the kind of file a real ~/.hellishrc is: function definitions built
from prompt-theme idioms (escape-heavy strings, $'..' quoting, parameter
expansion, case dispatch, command substitution) and -- in `dense` mode --
usage heredocs inside function bodies. Heredocs matter twice over: they
break the input streamer (so the rest of the file parses as one large
batch) and their bodies are the heap attachment that arms the AST
teardown walk. This is the exact shape that exposed issue #94: a shared
full_word struct handed out by the parse arena's xmalloc fallback, freed
once per child by the walk.

Usage: gen_rc_corpus.py <plain|dense> <seed> <nfunc>
Output is deterministic for a given (mode, seed, nfunc).
"""
import random
import sys

SNIPPETS = [
    '    local c; case "$1" in a*) c=A ;; *) c=B ;; esac\n',
    '    case "${1:-x}" in\n        -h|--help) printf "%s\\n" "usage" ;;\n'
    '        [0-9]*) n=$(($1 + 1)) ;;\n        *) : ;;\n    esac\n',
    '    local d="${PWD##*/}"\n',
    '    printf "\\033[38;5;%sm%s\\033[0m" "$((n % 255))" "${2:-}"\n',
    '    if [ -n "${GIT_BRANCH:-}" ]; then\n        b="(${GIT_BRANCH})"\n'
    '    elif [ -d .git ]; then\n'
    '        b=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)\n    fi\n',
    '    local t=$(date +%H:%M:%S)\n',
    '    x="${x%%:*}"; y="${y##*=}"\n',
    '    n=$((n * 2 + ${#1}))\n',
    '    local arr="a b c"\n    set -- $arr\n',
    '    s=$(printf "%*s" 8 "");\n    echo "${s// /-}" > /dev/null\n',
    '    v=${VAR:-"fall back"}; w=${OTHER:+set}\n',
    '    [ "$#" -gt 0 ] && shift\n',
    '    case $- in *i*) inter=1 ;; *) inter=0 ;; esac\n',
    '    ret=$?\n    [ $ret -ne 0 ] && printf "exit:%d " "$ret"\n',
    '    s=$\'\\e[1;32m\'"ok"$\'\\e[0m\'\n',
]

DENSE_EXTRA = [
    '    cat <<EOF > /dev/null\nusage: cmd [-h] [-v] ARG\n'
    '  more usage text $HOME and ${PWD}\nEOF\n',
    '    read -r a b <<< "one two"\n',
    '    while read -r line; do :; done <<-\tEOT\n'
    '\tindented body $USER\n\tEOT\n',
]

PROMPT = [
    'PS1_SEG_%d="\\[\\033[1;3%dm\\]\\u@\\h\\[\\033[0m\\]:\\w\\$ "\n',
    'THEME_%d_%d="%%n@%%m %%1~ %%# "\n',
    'HELLISH_COLOR_%d=$((30 + %d %% 8))\n',
]


def main():
    mode, seed, nfunc = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
    r = random.Random(seed)
    pool = SNIPPETS + (DENSE_EXTRA if mode == "dense" else [])
    out = ["# generated rc corpus: mode=%s seed=%d nfunc=%d\n"
           % (mode, seed, nfunc)]
    if mode == "dense":
        out.append('cat <<TOP > /dev/null\nbanner ${USER}\nTOP\n')
    for i in range(nfunc):
        if r.random() < 0.1:
            out.append((PROMPT[i % len(PROMPT)]) % (i, i))
            continue
        body = "".join(r.sample(pool, r.randint(2, 6)))
        out.append("rc_fn_%d() {\n%s}\n" % (i, body))
    out.append("true\n")
    sys.stdout.write("\n".join(out))


main()
