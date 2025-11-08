/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   maybe_call_trap_handler.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:26 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 05:43:01 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/**
 * if a trap handler exists for signal SIG, then we acll it otherwise
 * we jsut return failure. Returns 1 if it called the trap handler
 */
int	maybe_call_trap_handler(int sig)
{
	if ((g_sig.sigmodes[sig] & SIG_TRAPPED)
		&& ((g_sig.sigmodes[sig] & SIG_IGNORED) == 0))
	{
		if (sig == SIGINT)
			return (run_interrupt_trap(0), 1);
		else if (sig == EXIT_TRAP)
			return (run_exit_trap(), 1);
		else if (sig == DEBUG_TRAP)
			return (run_debug_trap(), 1);
		else if (sig == ERROR_TRAP)
			return (run_error_trap(), 1);
		else
			return (trap_handler(sig), 1);
	}
	return (0);
}
