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

/* Restore both SIGINT and SIGQUIT to default kernel action.  Called in
   the child just before execve so the new program starts with clean
   signal dispositions (its own handlers, not ours). */
static inline void	default_signal_handlers(void)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
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