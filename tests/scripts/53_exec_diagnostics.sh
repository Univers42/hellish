# Issue #133: `./life` answered "No such file or directory" -- for a file
# that was there. Running a path that cannot be run said the same thing
# whatever the reason:
#
#   * the direct-path check forced errno to ENOENT before printing, so a
#     file without +x, a directory, `file/x` (ENOTDIR) and a symlink loop
#     (ELOOP) all read "No such file or directory", and the last two came
#     back 127 where bash says 126;
#   * once execve itself had failed, only EACCES was reported: a script
#     whose `#!` interpreter is missing (or a program whose loader is --
#     execve's ENOENT on a file that exists) printed NOTHING and exited 127;
#   * a binary execve would not load was handed to /bin/sh, which printed
#     its own confusion about "line 1" -- and ran the rest of the file when
#     the NUL came late enough.
#
# Each case prints its diagnostic and status on stdout, so the corpus diff
# against bash grades both. What precedes the message is the shell's name
# and `line N:`; that is stripped, so the text after it is what is compared.
# The fixtures live in a directory of their own: verify_alloc.sh runs the
# corpus from tests/scripts itself.

T=$(mktemp -d) || exit 1
cd "$T" || exit 1

diag() {
	"$@" 2>err.txt
	rc=$?
	sed -e 's|^.*: line [0-9]*: ||' -e "s|^$0: ||" -e "s|$PWD|PWD|g" err.txt
	echo "$1 rc=$rc"
}

mkdir d pbin
printf 'int main(void){return 0;}\n' > notexec
printf '#!/nonexistent/interp\necho ran\n' > nointerp
printf '#!/bin/sh\r\necho ran\r\n' > crlf
printf '#!/\necho ran\n' > dirinterp
printf '\177ELFnot-a-real-header\n' > elfjunk
printf 'ab\000cd\necho ran-after-nul\n' > nulfirst
printf '#!/bin/sh\n\000\necho ran\n' > nulsecond
printf 'echo text-script-ran\n' > noshebang
: > empty
ln -s loop2 loop1
ln -s loop1 loop2
chmod +x nointerp crlf dirinterp elfjunk nulfirst nulsecond noshebang empty
cp nointerp pbin/onpath

diag ./notexec
diag ./d
diag ./nope
diag ./notexec/x
diag ./loop1
diag ./nointerp
diag ./crlf
diag ./dirinterp
diag ./elfjunk
diag ./nulfirst
diag ./nulsecond
diag ./noshebang
diag ./empty
PATH="$PWD/pbin:$PATH" diag onpath

# The same failures one level down, where the status is all that is seen.
./notexec/x 2>/dev/null; echo "enotdir $?"
./loop1 2>/dev/null; echo "eloop $?"
(./elfjunk) 2>/dev/null; echo "subshell binary $?"
x=$(./nointerp 2>&1); echo "cmdsub nointerp $? [${x##*: }]"

cd / && rm -rf "$T"
