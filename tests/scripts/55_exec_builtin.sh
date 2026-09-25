# `exec NAME` for a NAME that is not there used find_cmd_path, the lookup
# a forked child makes just before it exits: on a miss that prints
# "command not found" and frees the whole shell. exec runs in the shell
# itself, so it went on to print its own message through the freed state
# (a heap-use-after-free under ASan, garbage or a crash without it), and
# a subshell's EXIT trap ran on freed memory too.
#
# bash's rules, which each case below checks against it:
#   * a NAME found nowhere: "exec: NAME: not found", status 127;
#   * a file execve refuses: "PATH: <why>", 126 -- 127 when it is not
#     there -- a directory reads "Is a directory"; PATH is the file's full
#     name, a relative one joined to the working directory less one
#     leading "./" (bash's full_pathname);
#   * a file on PATH without execute permission is still the one picked,
#     and fails "Permission denied"; a directory on PATH is skipped;
#   * `--` ends exec's options;
#   * a shell that is not interactive (this script, any subshell) exits
#     with that status after running its EXIT trap; nothing after it runs.
#
# Each case prints its diagnostic and status on stdout, so the corpus diff
# grades both. The shell's name and `line N:` before a message are
# stripped. Paths are absolute and the fixture directory is printed as T.
# The fixtures live in a directory of their own: verify_alloc.sh runs the
# corpus from tests/scripts itself.

T=$(mktemp -d) || exit 1
cd "$T" || exit 1

run() {
	( eval "$1" ) 2>err.txt
	rc=$?
	sed -e 's|^.*: line [0-9]*: ||' -e "s|^$0: ||" -e "s|$T|T|g" err.txt
	echo "[$1] rc=$rc"
}

mkdir d pdir pdir/dircmd
printf 'echo ran\n' > notexec
printf '#!/bin/sh\necho "noxcmd ran"\n' > pdir/noxcmd
printf '#!/bin/sh\necho "args:$*"\n' > pdir/showargs
chmod +x pdir/showargs

run 'exec nosuch_cmd_q'
run 'exec -- nosuch_cmd_q'
run 'exec nosuch_cmd_q </dev/null 3>out.txt'
run 'exec nosuch_cmd_q; echo not-reached'
run 'exec "$T/notexec"'
run 'exec "$T/d"'
run 'exec "$T/nope"'
run 'exec "$T/notexec/x"'
run 'exec ./notexec'
run 'exec ./d'
run 'exec nope/../notexec'
run 'exec .//notexec'
run 'cd d && exec ../notexec'
run 'PATH=pdir; exec noxcmd'
run 'PATH=.; cd pdir && exec noxcmd'
run 'PATH="$T/pdir:$PATH"; exec noxcmd'
run 'PATH="$T/pdir:$PATH"; exec dircmd'
run 'PATH="$T/pdir:$PATH"; exec showargs a "b c"'
run 'PATH="$T/pdir:$PATH"; exec -- showargs -- x'
run 'exec echo via-exec'
run 'exec -- echo dashdash'
run 'exec --'
run 'exec 3>fd3.txt; echo kept >&3'
cat fd3.txt
run 'trap "echo exit-trap-ran" EXIT; exec nosuch_cmd_q; echo not-reached'
run 'f() { exec nosuch_cmd_q; echo after-f; }; f; echo after-call'
run 'exec nosuch_cmd_q | cat; echo after-pipeline'

x=$(exec nosuch_cmd_q 2>/dev/null; echo after)
echo "cmdsub [$x] rc=$?"
( exec nosuch_cmd_q ) 2>/dev/null | cat
echo "pipeline rc=$?"

cd / && rm -rf "$T"
trap 'echo final-exit-trap' EXIT
exec nosuch_cmd_q 2>/dev/null
echo "not reached: a script ends at a failed exec"
