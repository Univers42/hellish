/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   job_pgrp.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 10:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/22 10:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include <termios.h>
#include <unistd.h>

/* Per-job process groups.  Without them ^Z does not merely aim badly, it
   does nothing at all, and the reason is POSIX XSH 2.4.3: SIGTSTP, SIGTTIN
   and SIGTTOU delivered to a member of an ORPHANED process group are
   DISCARDED.  A process group is orphaned when no member has a parent in a
   different group of the same session -- which is exactly the shell's own
   group once the shell is a session leader (every terminal emulator makes
   it one) and its children stay inside that group.  So the terminal sent
   SIGTSTP, the kernel dropped it, and the foreground command kept running
   and kept eating the keystrokes meant for the shell (issue #27).

   Giving each job its own group breaks the orphan condition -- the job's
   members have the shell as a parent, and the shell is in a different group
   of the same session -- so the kernel delivers the stop for real.

   Recorded once at startup: the shell's pid and group, and whether it owns
   the terminal at all.  A shell that does not own the terminal on entry
   leaves shell_pgid at 0 and never touches job control, so nothing here can
   steal a terminal from a shell that launched us. */
void	jc_init(t_shell *st)
{
	st->shell_pid = getpid();
	st->shell_pgid = 0;
	if (isatty(STDIN_FILENO) && tcgetpgrp(STDIN_FILENO) == getpgrp())
		st->shell_pgid = getpgrp();
}

/* Start of one foreground job (one pipeline).  Decide HERE, once, whether
   this process may drive job control, so the answer is already in the
   inherited t_shell copy by the time a child needs it -- a child cannot
   work it out for itself, having a pid of its own by then.

   The getpid() test is what makes this safe without threading a flag
   through every fork site: a background body, a subshell and a $( ) capture
   all re-enter this function on their own way down, all fail the test, and
   so none of them can move a group or grab the terminal for the real shell.
   INP_RL is checked first and short-circuits, so a script or -c never even
   pays for the getpid(). */
void	jc_begin(t_shell *st)
{
	st->fg_pgid = 0;
	st->jobctl = (st->metinp == INP_RL && st->shell_pgid > 0
			&& st->shell_pid == getpid());
}

/* Child side of the fork, before execve.  fg_pgid is 0 for the first (or
   only) stage, so setpgid puts us in a group of our own; later stages read
   the leader's pgid out of the inherited t_shell and join it, which is what
   makes a whole pipeline one job the way bash reports it.

   Both sides of the fork call setpgid because either may be scheduled
   first, and both call tcsetpgrp because a child that writes to the tty
   before the parent's handover lands would take a SIGTTOU it has already
   reset to SIG_DFL.  Whichever call loses the race simply fails, so the
   return values are deliberately unchecked -- the winner did the work. */
void	jc_child(t_shell *st)
{
	pid_t	pgid;

	if (!st->jobctl)
		return ;
	pgid = st->fg_pgid;
	if (pgid == 0)
		pgid = getpid();
	setpgid(0, pgid);
	tcsetpgrp(STDIN_FILENO, pgid);
}

/* Parent side of the same fork: adopt the first stage's pid as the job's
   group, put the child there, and hand the terminal to it. */
void	jc_parent(t_shell *st, pid_t pid)
{
	if (!st->jobctl || pid <= 0)
		return ;
	if (st->fg_pgid == 0)
		st->fg_pgid = pid;
	setpgid(pid, st->fg_pgid);
	tcsetpgrp(STDIN_FILENO, st->fg_pgid);
}

/* End of the job: take the terminal back.  Runs after the wait on BOTH
   outcomes -- a job that exited and a job that just stopped under ^Z hand
   the tty back identically, and that is what lets the REPL draw the next
   prompt instead of wedging on a read from a terminal it no longer owns.

   Safe to call when no job ever forked (fg_pgid still 0): plain builtins
   take the early return and issue no syscall at all. */
void	jc_end(t_shell *st)
{
	if (!st->jobctl || st->fg_pgid == 0)
		return ;
	tcsetpgrp(STDIN_FILENO, st->shell_pgid);
	st->fg_pgid = 0;
}
