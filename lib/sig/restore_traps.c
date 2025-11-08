/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   restore_traps.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:56 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 08:57:35 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Undo the effects of reset_signal_handlers(), which unsets the traps but
	 leaves the trap strings in place. This understands how reset_signal_handlers
	 works. */
static void	restore_special_traps(void)
{
	char	*trapstr;

	trapstr = g_sig.trap_list[EXIT_TRAP];
	if (trapstr)
		reinit_trap(EXIT_TRAP);
	trapstr = g_sig.trap_list[DEBUG_TRAP];
	if (trapstr && g_sig.function_trace_mode == 0)
		reinit_trap(DEBUG_TRAP);
	trapstr = g_sig.trap_list[RETURN_TRAP];
	if (trapstr && g_sig.function_trace_mode == 0)
		reinit_trap(RETURN_TRAP);
	trapstr = g_sig.trap_list[ERROR_TRAP];
	if (trapstr && g_sig.error_trace_mode == 0)
		reinit_trap(ERROR_TRAP);
}

static void	restore_signal_trap(int i)
{
	char	*trapstr;

	trapstr = g_sig.trap_list[i];
	if (g_sig.sigmodes[i] & SIG_SPECIAL)
	{
		if (trapstr && trapstr != (char *)DEFAULT_SIG)
			reinit_trap(i);
		if (trapstr == (char *)IGNORE_SIG
			&& (g_sig.sigmodes[i] & SIG_NO_TRAP) == 0)
			set_signal_handler(i, SIG_IGN);
	}
	else if (trapstr == (char *)IGNORE_SIG)
	{
		reinit_trap(i);
		if ((g_sig.sigmodes[i] & SIG_NO_TRAP) == 0)
			set_signal_handler(i, SIG_IGN);
	}
	else if (trapstr != (char *)DEFAULT_SIG)
		set_signal(i, trapstr);
	g_sig.pending_traps[i] = 0;
}

void	restore_traps(void)
{
	int	i;

	restore_special_traps();
	i = 0;
	while (++i < NSIG)
		restore_signal_trap(i);
}
