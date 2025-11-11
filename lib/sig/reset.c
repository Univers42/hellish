/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reset.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 18:27:18 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 18:28:02 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	reinit_trap(int sig)
{
	get_g_sig()->sigmodes[sig] |= SIG_TRAPPED;
	if (get_g_sig()->trap_list[sig] == (char *)IGNORE_SIG)
		get_g_sig()->sigmodes[sig] |= SIG_IGNORED;
	else
		get_g_sig()->sigmodes[sig] &= ~SIG_IGNORED;
	if (get_g_sig()->sigmodes[sig] & SIG_INPROGRESS)
		get_g_sig()->sigmodes[sig] |= SIG_CHANGED;
}

void	reset_signal(int sig)
{
	if ((get_g_sig()->sigmodes[sig] & SIG_ASYNCSIG)
		&& signal_is_trapped(sig)
		&& get_g_sig()->original_signals[sig] == SIG_IGN)
		get_g_sig()->original_signals[sig] = SIG_DFL;
	set_signal_handler(sig,
		(void (*)(int))get_g_sig()->original_signals[sig]);
	get_g_sig()->sigmodes[sig] &= ~SIG_TRAPPED;
}

/* Reset trapped signals to their original values, but don't free the
	 trap strings.  Called by the command substitution code and other places
	 that create a "subshell environment". */
void reset_signal_handlers(void)
{
	reset_or_restore_signal_handlers(reset_signal);
}
