/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   restore_default_signal.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:47 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 15:10:39 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

static void	restore_special_signal(int sig)
{
	if ((sig != DEBUG_TRAP && sig != ERROR_TRAP && sig != RETURN_TRAP)
		|| (get_g_sig()->sigmodes[sig] & SIG_INPROGRESS) == 0)
		free_trap_command(sig);
	get_g_sig()->trap_list[sig] = NULL;
	get_g_sig()->sigmodes[sig] &= ~SIG_TRAPPED;
	if (get_g_sig()->sigmodes[sig] & SIG_INPROGRESS)
		get_g_sig()->sigmodes[sig] |= SIG_CHANGED;
}

static int	handle_async_or_untrapped(int sig)
{
	if ((get_g_sig()->sigmodes[sig] & SIG_HARD_IGNORE)
		|| ((get_g_sig()->sigmodes[sig]
				& (SIG_TRAPPED | SIG_ASYNCSIG | SIG_NO_TRAP))
			== SIG_ASYNCSIG))
	{
		get_g_sig()->original_signals[sig] = (void *)SIG_DFL;
		set_signal_handler(sig, SIG_DFL);
		change_signal(sig, (char *)DEFAULT_SIG);
		return (1);
	}
	if (((get_g_sig()->sigmodes[sig] & SIG_TRAPPED) == 0)
		&& (sig != SIGCHLD || (get_g_sig()->sigmodes[sig] & SIG_INPROGRESS) == 0
			|| get_g_sig()->trap_list[sig] != (char *)IMPOSSIBLE_TRAP_HANDLER))
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
	if ((get_g_sig()->sigmodes[sig] & SIG_NO_TRAP) == 0)
		set_signal_handler(sig,
			(void (*)(int))get_g_sig()->original_signals[sig]);
	change_signal(sig, (char *)DEFAULT_SIG);
	get_g_sig()->sigmodes[sig] &= ~SIG_TRAPPED;
}
