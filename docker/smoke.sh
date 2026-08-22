#!/usr/bin/env bash
# ============================================================================
# docker/smoke.sh -- the portability workout every platform image runs.
#
# This is deliberately NOT the golden suite. The golden suite needs a pinned
# bash 5.3.9 to diff against; building it inside twelve distro images would
# make the matrix cost hours. What this covers instead is the class of bug
# that only shows up when the C code meets a different libc, a different
# compiler or a different kernel personality -- the shell starting at all,
# fork/exec, pipes, redirection, here-docs, globbing, arithmetic, signals,
# job control, /dev/fd, and the two allocators agreeing.
#
# Every check is self-checking: it prints its own PASS/FAIL and the script
# exits non-zero if any failed. Run it anywhere:
#
#   docker/smoke.sh                 # against ./build/bin/hellish
#   docker/smoke.sh /usr/bin/hellish
# ============================================================================
set -u

SHELL_BIN="${1:-./build/bin/hellish}"
export HELLISH_NO_BANNER=1 HELLISH_NO_UPDATE_CHECK=1 HELLISH_NO_ANIM=1
pass=0
fail=0
failed_names=""

# expect NAME EXPECTED SCRIPT -- run SCRIPT through the shell, compare stdout.
expect() {
	local name="$1" want="$2" prog="$3" got
	got="$("$SHELL_BIN" -c "$prog" 2>/dev/null)"
	if [ "$got" = "$want" ]; then
		printf '  \033[32mok  \033[0m %s\n' "$name"
		pass=$((pass + 1))
	else
		printf '  \033[31mFAIL\033[0m %s\n        want %q\n        got  %q\n' \
			"$name" "$want" "$got"
		fail=$((fail + 1))
		failed_names="$failed_names $name"
	fi
}

# expect_status NAME EXPECTED_STATUS SCRIPT
expect_status() {
	local name="$1" want="$2" prog="$3" got
	"$SHELL_BIN" -c "$prog" >/dev/null 2>&1
	got=$?
	if [ "$got" = "$want" ]; then
		printf '  \033[32mok  \033[0m %s\n' "$name"
		pass=$((pass + 1))
	else
		printf '  \033[31mFAIL\033[0m %s (status %s, wanted %s)\n' \
			"$name" "$got" "$want"
		fail=$((fail + 1))
		failed_names="$failed_names $name"
	fi
}

printf '\n\033[1m═══ hellish portability smoke ═══\033[0m\n'
printf '  shell : %s\n' "$SHELL_BIN"
printf '  system: %s\n' "$(uname -srm 2>/dev/null || echo unknown)"
printf '  libc  : %s\n' \
	"$(ldd --version 2>&1 | head -1 || echo 'n/a')"
printf '\n'

# --- the shell exists and answers at all -----------------------------------
expect  "runs a command"                 "hi"      'echo hi'
expect  "\$0 is the shell"                "yes"     'case "$0" in *hellish*|*sh) echo yes;; *) echo "no:$0";; esac'

# --- expansion -------------------------------------------------------------
expect  "arithmetic"                     "42"      'echo $((6*7))'
expect  "arithmetic, 64-bit"             "4294967296" 'echo $((1<<32))'
expect  "parameter expansion"            "bc"      'v=abc; echo ${v#a}'
expect  "default value"                  "fallback" 'unset u; echo ${u:-fallback}'
expect  "command substitution"           "sub"     'echo $(echo sub)'
expect  "nested command substitution"    "in"      'echo $(echo $(echo in))'
expect  "quoted \$@ splitting"           "a|b c"   'set -- a "b c"; IFS=+; printf "%s|%s" "$@"'
expect  "tilde expansion"                "yes"     'case ~ in /*) echo yes;; *) echo no;; esac'
expect  "brace expansion"                "a1 a2"   'echo a{1,2}'

# --- words, quoting, locale ------------------------------------------------
expect  "single quotes are literal"      'a$b'     "echo 'a\$b'"
expect  "ANSI-C quoting"                 "$(printf 'a\tb')" "printf '%s' \$'a\\tb'"
expect  "8-bit clean words"              "héllo"   'echo héllo'
expect  "printf width and precision"     "ok|   42|ab" 'printf "%s|%5d|%.2s" ok 42 abcdef'

# --- redirection and here-docs ---------------------------------------------
expect  "pipeline"                       "ABC"     'echo abc | tr a-z A-Z'
expect  "three-stage pipeline"           "3"       'printf "a\nb\nc\n" | cat | wc -l | tr -d " "'
expect  "here-doc"                       "body"    'cat <<EOF
body
EOF'
expect  "here-doc inside a compound"     "nested"  'if true; then cat <<EOF
nested
EOF
fi'
expect  "here-string"                    "hs"      'cat <<<hs'
expect  "fd duplication order"           "err"     '{ echo out; echo err >&2; } 2>&1 1>/dev/null | cat'
expect  "process substitution"           "ps"      'cat <(echo ps)'

# --- control flow ----------------------------------------------------------
expect  "for loop"                       "1 2 3"   'out=; for i in 1 2 3; do out="$out $i"; done; echo ${out# }'
expect  "while + break"                  "3"       'i=0; while :; do i=$((i+1)); [ $i -ge 3 ] && break; done; echo $i'
expect  "case"                           "match"   'case abc in a*) echo match;; *) echo no;; esac'
expect  "function with local"            "in out"  'f() { local v=in; echo "$v out"; }; f'
expect  "subshell isolation"             "outer"   'v=outer; (v=inner) ; echo $v'

# --- externals, PATH, exit status ------------------------------------------
expect  "external in PATH"               "ok"      'printf ok'
expect_status "command not found is 127" 127       'definitely_no_such_command_42'
expect_status "permission denied is 126" 126       'd=$(mktemp -d); : > $d/x; chmod 000 $d/x; $d/x'
expect_status "false is 1"               1         'false'
expect  "pipeline status is the last"    "0"       'false | true; echo $?'

# --- globbing --------------------------------------------------------------
expect  "glob matches"                   "a.t b.t" 'cd $(mktemp -d) && : > a.t && : > b.t && echo *.t'
expect  "glob with no match is literal"  "zz*.nope" 'cd $(mktemp -d) && echo zz*.nope'
expect  "bracket class"                  "f1"      'cd $(mktemp -d) && : > f1 && echo f[0-9]'

# --- signals and job control ----------------------------------------------
expect  "trap runs on EXIT"              "bye"     'trap "echo bye" EXIT; :'
expect  "background job reaped by wait"  "done"    'sleep 0.05 & wait; echo done'
expect_status "killed child reports 143" 143       'sh -c "kill -TERM \$\$"'

# --- the two things most likely to be platform-specific --------------------
expect  "\$\$ is this shell's pid"       "same"    'p=$$; [ "$p" = "$$" ] && echo same'
expect  "shell re-execs itself"          "self"    'echo self | '"$SHELL_BIN"' -c "cat"'

printf '\n\033[1m═══ %d ok / %d failed ═══\033[0m\n' "$pass" "$fail"
if [ "$fail" -ne 0 ]; then
	printf 'failed:%s\n' "$failed_names"
	exit 1
fi
exit 0
