/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free_trap_strings.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:17 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 08:48:31 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Free all the allocated strings in the list of traps and reset the trap
	 values to the default.  Intended to be called from subshells that want
	 to complete work done by reset_signal_handlers upon execution of a
	 subsequent `trap' command that changes a signal's disposition.  We need
	 to make sure that we duplicate the behavior of
	 reset_or_restore_signal_handlers and not change the disposition of signals
	 that are set to be ignored. */
void	free_trap_strings(void)
{
	register int	i;

	i = -1;
	while (++i < NSIG)
		if (g_sig.trap_list[i] != (char *)IGNORE_SIG)
			free_trap_string(i);
	i = NSIG - 1;
	while (++i < BASH_NSIG_TOTAL)
	{
		if ((g_sig.sigmodes[i] & SIG_TRAPPED) == 0)
		{
			free_trap_string(i);
			g_sig.trap_list[i] = (char *) NULL;
		}
	}
}
