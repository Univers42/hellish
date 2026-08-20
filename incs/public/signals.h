/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signals.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:11:08 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:11:08 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Signal handler utilities.  The shell's signal strategy:
   - While waiting for a foreground child: SIGINT -> SIG_DFL (child gets
     it and dies; our waitpid sees the exit, we set $?=130).
   - While in readline: SIGINT -> set_unwind() (marks the global sig struct
     so readline can be interrupted cleanly without leaking memory).
   - SIGQUIT is always ignored in the shell process itself (SIG_IGN).
   Inlined to avoid a function-call overhead on the hot signal path. */

#ifndef SIGNALS_H
# define SIGNALS_H

# include <signal.h>
# include <stdint.h>

/* Restore the job-control and interrupt signals to their default kernel
   action.  Called in the child just before execve so the new program starts
   with clean dispositions (its own handlers, not ours).

   SIGTSTP/SIGTTIN/SIGTTOU matter here because the interactive shell now
   ignores them (see interactive_job_signals): SIG_IGN survives execve, so
   without this reset every command the shell launched would inherit "^Z
   does nothing" and could never be suspended.  A background child re-ignores
   them right after calling us -- see bg_child_body. */
static inline void	default_signal_handlers(void)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
	signal(SIGTTIN, SIG_DFL);
	signal(SIGTTOU, SIG_DFL);
}

/* Job-control signals for the SHELL PROCESS, interactive only.  A shell must
   not stop when the user hits ^Z: the terminal sends SIGTSTP to the whole
   foreground process group, which contains the shell as well as the command
   it is waiting on, so a shell at SIG_DFL suspends itself along with the job.
   That is what hellish did -- ^Z froze the shell instead of the command, and
   with no outer shell to return to the terminal simply went dead while the
   kernel kept echoing whatever was typed (issue #25).

   SIGTTIN/SIGTTOU likewise: the shell reads and writes the terminal from a
   process group that is not always the foreground one, and stopping there
   would wedge it in the same way.  Scripts and -c keep the defaults, so a
   non-interactive shell stays killable exactly as before. */
static inline void	interactive_job_signals(int interactive)
{
	if (!interactive)
		return ;
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
}

/* Signal setup for the readline read loop: SIGINT propagates to children
   but SIGQUIT is ignored so Ctrl-\ doesn't dump core from the shell. */
static inline void	readline_bg_signals(void)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_IGN);
}

/* SIGINT handler used while the shell is inside readline.  Sets the
   global unwind flag so the shell can break out of readline cleanly
   without longjmp-style stack unwinding. */
static inline void	set_unwind(int sig)
{
	(void)sig;
	get_g_sig()->should_unwind = SIGINT;
}

void	set_unwind_sig(void);
void	set_unwind_sig_norestart(void);

#endif