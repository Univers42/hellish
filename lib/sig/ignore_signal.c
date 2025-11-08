/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ignore_signal.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:23 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 09:01:18 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Make this signal be ignored. */
void	ignore_signal(int sig)
{
	if (spec_trap(sig) && ((g_sig.sigmodes[sig] & SIG_IGNORED) == 0))
	{
		change_signal (sig, (char *)IGNORE_SIG);
		return ;
	}
	get_original_signal(sig);
	if (g_sig.sigmodes[sig] & SIG_HARD_IGNORE)
		return ;
	if (g_sig.sigmodes[sig] & SIG_IGNORED)
	{
		g_sig.sigmodes[sig] |= SIG_TRAPPED;
		return ;
	}
	if ((g_sig.sigmodes[sig] & SIG_NO_TRAP) == 0)
		set_signal_handler (sig, SIG_IGN);
	change_signal (sig, (char *)IGNORE_SIG);
}
