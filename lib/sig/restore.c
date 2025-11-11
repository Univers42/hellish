/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   restore_traps.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:56 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 18:35:40 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

static void	restore_spec_traps(void)
{
	char	*trapstr;

	trapstr = get_g_sig()->trap_list[EXIT_TRAP];
	if (trapstr)
		reinit_trap(EXIT_TRAP);
	trapstr = get_g_sig()->trap_list[DEBUG_TRAP];
	if (trapstr && get_g_sig()->function_trace_mode == 0)
		reinit_trap(DEBUG_TRAP);
	trapstr = get_g_sig()->trap_list[RETURN_TRAP];
	if (trapstr && get_g_sig()->function_trace_mode == 0)
		reinit_trap(RETURN_TRAP);
	trapstr = get_g_sig()->trap_list[ERROR_TRAP];
	if (trapstr && get_g_sig()->error_trace_mode == 0)
		reinit_trap(ERROR_TRAP);
}

static void	restore_signal_trap(int i)
{
	char	*trapstr;

	trapstr = get_g_sig()->trap_list[i];
	if (get_g_sig()->sigmodes[i] & SIG_SPECIAL)
	{
		if (trapstr && trapstr != (char *)DEFAULT_SIG)
			reinit_trap(i);
		if (trapstr == (char *)IGNORE_SIG
			&& (get_g_sig()->sigmodes[i] & SIG_NO_TRAP) == 0)
			set_signal_handler(i, SIG_IGN);
	}
	else if (trapstr == (char *)IGNORE_SIG)
	{
		reinit_trap(i);
		if ((get_g_sig()->sigmodes[i] & SIG_NO_TRAP) == 0)
			set_signal_handler(i, SIG_IGN);
	}
	else if (trapstr != (char *)DEFAULT_SIG)
		set_signal(i, trapstr);
	get_g_sig()->pending_traps[i] = 0;
}

void	restore_traps(void)
{
	int	i;

	restore_spec_traps();
	i = 0;
	while (++i < NSIG)
		restore_signal_trap(i);
}

/* Reset all trapped signals to their original values.  Signals set to be
	 ignored with trap '' SIGNAL should be ignored, so we make sure that they
	 are.  Called by child processes after they are forked. */
void	restore_original_signals(void)
{
	reset_or_restore_signal_handlers(restore_signal);
}

void	restore_signal(int sig)
{
	if ((get_g_sig()->sigmodes[sig] & SIG_ASYNCSIG)
		&& signal_is_trapped(sig)
		&& get_g_sig()->original_signals[sig] == SIG_IGN)
		get_g_sig()->original_signals[sig] = SIG_DFL;
	set_signal_handler(sig,
		(void (*)(int))get_g_sig()->original_signals[sig]);
	change_signal(sig, (char *)DEFAULT_SIG);
	get_g_sig()->sigmodes[sig] &= ~SIG_TRAPPED;
}
