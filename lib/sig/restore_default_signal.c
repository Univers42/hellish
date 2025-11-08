/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   restore_default_signal.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:47 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 06:10:50 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Restore the default action for SIG; i.e., the action the shell
	 would have taken before you used the trap command.  This is called
	 from trap_builtin (), which takes care to restore the handlers for
	 the signals the shell treats specially. */
static void	restore_special_signal(int sig)
{
	if ((sig != DEBUG_TRAP && sig != ERROR_TRAP && sig != RETURN_TRAP)
		|| (g_sig.sigmodes[sig] & SIG_INPROGRESS) == 0)
		free_trap_command(sig);
	g_sig.trap_list[sig] = NULL;
	g_sig.sigmodes[sig] &= ~SIG_TRAPPED;
	if (g_sig.sigmodes[sig] & SIG_INPROGRESS)
		g_sig.sigmodes[sig] |= SIG_CHANGED;
}

static int	handle_async_or_untrapped(int sig)
{
	if ((g_sig.sigmodes[sig] & SIG_HARD_IGNORE)
		|| ((g_sig.sigmodes[sig] & (SIG_TRAPPED | SIG_ASYNCSIG | SIG_NO_TRAP))
			== SIG_ASYNCSIG))
	{
		g_sig.original_signals[sig] = SIG_DFL;
		set_signal_handler(sig, SIG_DFL);
		change_signal(sig, (char *)DEFAULT_SIG);
		return (1);
	}
	if (((g_sig.sigmodes[sig] & SIG_TRAPPED) == 0)
		&& (sig != SIGCHLD
			|| (g_sig.sigmodes[sig] & SIG_INPROGRESS) == 0
			|| g_sig.trap_list[sig] != (char *)IMPOSSIBLE_TRAP_HANDLER))
		return (1);
	return (0);
}

void	restore_default_signal(int sig)
{
	if (spec_trap(sig))
	{
		restore_special_signal(sig);
		return ;
	}
	get_original_signal(sig);
	if (handle_async_or_untrapped(sig))
		return ;
	if ((g_sig.sigmodes[sig] & SIG_NO_TRAP) == 0)
		set_signal_handler(sig, g_sig.original_signals[sig]);
	change_signal(sig, (char *)DEFAULT_SIG);
	g_sig.sigmodes[sig] &= ~SIG_TRAPPED;
}
