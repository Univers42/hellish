/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reset_or_restore_signal_handlers.c                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:37 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 03:04:05 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/**
 * 
 * !!!!!!!!SPECIFIC shell function
 */

// void	reset_or_restore_signal_handlers (sh_resetsig_func_t *reset)
// {
// 	register int	i;

// 	if (g_sig.sigmodes[EXIT_TRAP] & SIG_TRAPPED)
// 	{
// 		g_sig.sigmodes[EXIT_TRAP] &= ~SIG_TRAPPED;
// 			if (reset != g_sig.reset_signal)
// 			{
// 				free_trap_command (EXIT_TRAP);
// 				g_sig.trap_list[EXIT_TRAP] = (char *)NULL;
// 			}
// 	}
// 	i = -1;
// 	while (++i < NSIG)
// 	{
// 		if (g_sig.sigmodes[i] & SIG_TRAPPED)
// 		{
// 			if (g_sig.trap_list[i] == (char *)IGNORE_SIG)
// 				set_signal_handler (i, SIG_IGN);
// 			else
// 				(*reset) (i);
// 		}
// 		else if (g_sig.sigmodes[i] & SIG_SPECIAL)
// 			(*reset) (i);
// 		g_sig.pending_traps[i] = 0;
// 	}
// 	if (g_sig.function_trace_mode == 0)
// 	{
// 		g_sig.sigmodes[DEBUG_TRAP] &= ~SIG_TRAPPED;
// 		g_sig.sigmodes[RETURN_TRAP] &= ~SIG_TRAPPED;
// 	}
// 	if (g_sig.error_trace_mode == 0)
// 		g_sig.sigmodes[ERROR_TRAP] &= ~SIG_TRAPPED;
// }