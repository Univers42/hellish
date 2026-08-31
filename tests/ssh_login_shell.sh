#!/bin/bash
# ============================================================================
# tests/ssh_login_shell.sh -- hellish as a real LOGIN SHELL, behind a real
# sshd, doing the things people actually do over ssh.
#
# WHY THIS EXISTS
# ---------------
# `make my_shell` runs chsh. From that moment hellish is not just the thing
# you type into: it is what sshd execs for EVERY remote operation, because
# `ssh host cmd`, scp, sftp, rsync and git-over-ssh all run `$SHELL -c ...`.
# A shell that prints one stray byte on stdout there breaks the scp/rsync
# protocols outright, and nothing in the golden suite can see it -- that
# suite runs `hellish -c` directly, with no sshd, no chsh and no pipe.
#
# It was found the way these things usually are: a machine in the wild had a
# hand-written bash wrapper installed over /usr/bin/hellish, forcing bash for
# every non-tty invocation, to work around exactly this class of breakage.
# Whatever prompted it no longer reproduces -- this file is what proves that,
# and keeps proving it.
#
# HOW IT JUDGES. Not against a hardcoded expectation: every case runs twice,
# once with bash as the login shell and once with hellish, and the two must
# agree on output AND exit status. bash is the specification here exactly as
# it is in the golden suite, so a case cannot silently encode a hellish bug
# as "expected". The two known-good differences are asserted rather than
# ignored: $0 and $SHELL SHOULD name the running shell.
#
# Runs as root inside docker/Dockerfile.sshd (it needs sshd and chsh).
#   make ssh-shell-test
# ============================================================================
set -u

OPTS="-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR"
U=tester
FAILS=0

ok()   { printf '  \033[32mok\033[0m   %s\n' "$1"; }
bad()  { printf '  \033[31mFAIL\033[0m %s\n' "$1"; FAILS=$((FAILS + 1)); }
head_() { printf '\n\033[1;36m▸\033[0m \033[1m%s\033[0m\n' "$1"; }

use_shell() { chsh -s "$1" "$U" >/dev/null || exit 2; }
as_user()   { su "$U" -c "$1" 2>&1; }

/usr/sbin/sshd 2>/dev/null || { echo "sshd would not start"; exit 2; }
echo payload > "/home/$U/src.txt"; chown "$U" "/home/$U/src.txt"

# ── differential: same command, both shells, must agree ───────────────────
diff_case() {
	_desc="$1"; _cmd="$2"
	use_shell /bin/bash
	_bo="$(as_user "ssh $OPTS localhost \"$_cmd\"")"; _bs=$?
	use_shell /usr/bin/hellish
	_ho="$(as_user "ssh $OPTS localhost \"$_cmd\"")"; _hs=$?
	if [ "$_bo" = "$_ho" ] && [ "$_bs" = "$_hs" ]; then
		ok "$_desc"
	else
		bad "$_desc"
		printf '        bash   (%s): %s\n' "$_bs" "$(printf '%s' "$_bo" | head -2)"
		printf '        hellish(%s): %s\n' "$_hs" "$(printf '%s' "$_ho" | head -2)"
	fi
}

head_ "the shell sshd runs for a remote command"
diff_case "a plain command's output and status" 'echo CMD-OK'
diff_case "a non-zero exit status propagates"   'exit 7'
diff_case "command substitution, nested"        'echo \$(echo a \$(echo b))'
diff_case "a heredoc"                           'cat <<E
line
E'
diff_case "\$HOME and \$USER are set"           'echo \$HOME \$USER'
diff_case "PATH still finds things"             'command -v env || echo none'
diff_case "a loop"                              'for i in 1 2; do printf x; done; echo'
diff_case "an assignment then use"              'V=1; echo \$V'
diff_case "tilde expansion"                     'echo ~'
diff_case "&& / || chaining"                    'false || echo fallback'

# ── the byte-exactness cases: protocols that die on stray output ──────────
# scp/rsync/sftp speak a binary protocol over the same channel the shell
# writes to. One banner line and the transfer fails, which is the specific
# breakage the wrapper in the wild was built to dodge.
transfer_case() {
	_desc="$1"; _cmd="$2"; _proof="$3"
	rm -rf "$_proof"
	_err="$(as_user "$_cmd")"
	if [ -e "$_proof" ]; then
		ok "$_desc"
	else
		bad "$_desc"
		printf '        %s\n' "$(printf '%s' "$_err" | head -3)"
	fi
	rm -rf "$_proof"
}

head_ "protocols that break on a single stray byte (login shell = hellish)"
use_shell /usr/bin/hellish
transfer_case "scp copies a file" \
	"scp $OPTS /home/$U/src.txt localhost:/tmp/p-scp" /tmp/p-scp
transfer_case "rsync copies a file" \
	"rsync -q -e 'ssh $OPTS' /home/$U/src.txt localhost:/tmp/p-rs" /tmp/p-rs
# Asserted on the CONTENT, not on the file existing: `> /tmp/p-sftp` creates
# the file whether sftp worked or not, so an existence check here passed even
# against a deliberately poisoned shell that broke scp, rsync and git.
rm -f /tmp/p-sftp
as_user "echo ls | sftp $OPTS localhost > /tmp/p-sftp 2>&1"
if grep -q 'src\.txt' /tmp/p-sftp 2>/dev/null; then
	ok "sftp lists a directory"
else
	bad "sftp lists a directory"
	printf '        %s\n' "$(head -3 /tmp/p-sftp 2>/dev/null)"
fi
rm -f /tmp/p-sftp

# git-over-ssh runs `$SHELL -c 'git-upload-pack ...'` and is just as fragile.
su "$U" -c "git init -q --bare /home/$U/r.git \
	&& git -c user.email=t@t -c user.name=t -c init.defaultBranch=main \
	   clone -q /home/$U/r.git /tmp/seed \
	&& cd /tmp/seed && echo x > f && git add f \
	&& git -c user.email=t@t -c user.name=t commit -qm c \
	&& git push -q origin main" >/dev/null 2>&1
# GIT_SSH_COMMAND, or git spawns a bare `ssh` that has never seen this host
# key and dies at verification before hellish is reached at all -- a green
# test for a step that never ran, and a red one for a shell that was fine.
transfer_case "git clones over ssh" \
	"GIT_SSH_COMMAND='ssh $OPTS' git clone -q ssh://$U@localhost/home/$U/r.git /tmp/p-git" \
	/tmp/p-git

# ── an interactive session still works, and says nothing extra ────────────
head_ "an interactive ssh session"
use_shell /usr/bin/hellish
out="$(as_user "ssh -tt $OPTS localhost 'echo PTY-OK; exit'" | tr -d '\r')"
case "$out" in
*PTY-OK*) ok "a PTY login session runs a command and exits" ;;
*) bad "a PTY login session runs a command and exits"; printf '        %s\n' "$out" ;;
esac

# ── the two differences that SHOULD exist ─────────────────────────────────
# Asserted, not ignored: if these ever start matching bash, hellish has begun
# lying about its own identity.
head_ "what is allowed to differ"
use_shell /usr/bin/hellish
v="$(as_user "ssh $OPTS localhost 'echo \$0'")"
case "$v" in
*hellish*) ok "\$0 names hellish, not bash" ;;
*) bad "\$0 names hellish, not bash (got: $v)" ;;
esac
v="$(as_user "ssh $OPTS localhost 'echo \$SHELL'")"
case "$v" in
*hellish*) ok "\$SHELL names the registered login shell" ;;
*) bad "\$SHELL names the registered login shell (got: $v)" ;;
esac

use_shell /bin/bash
printf '\n  %s failure(s)\n' "$FAILS"
[ "$FAILS" -eq 0 ]
