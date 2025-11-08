/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_signal.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:50 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 09:03:30 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Set SIG to call STRING as a command. */
void	set_signal(int sig, const char *string)
{
	sigset_t	set;
	sigset_t	oset;

	if (SPECIAL_TRAP (sig))
	{
		change_signal (sig, savestring (string));
		if (sig == EXIT_TRAP && g_sig.interactive == 0)
			initialize_terminating_signals ();
		return ;
	}
	if (g_sig.sigmodes[sig] & SIG_HARD_IGNORE)
		return ;
	if ((g_sig.sigmodes[sig] & SIG_TRAPPED) == 0)
	{
		if (g_sig.original_signals[sig] == (void *)IMPOSSIBLE_TRAP_HANDLER)
			GETORIGSIG (sig);
		if (g_sig.original_signals[sig] == SIG_IGN
			&& (g_sig.sigmodes[sig] & SIG_ASYNCSIG) == 0)
			return ;
	}
	if ((g_sig.sigmodes[sig] & SIG_NO_TRAP) == 0)
	{
		block_signal(sig, &set, &oset);
		change_signal(sig, savestring(string));
		set_signal_handler (sig, trap_handler);
		unblock_signal(&oset);
	}
	else
		change_signal (sig, savestring (string));
}
