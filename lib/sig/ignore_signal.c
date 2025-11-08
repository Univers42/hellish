/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ignore_signal.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:23 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 06:54:13 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Make this signal be ignored. */
void	ignore_signal(int sig)
{
	if (spec_trap(sig) && ((sigmodes[sig] & SIG_IGNORED) == 0))
	{
		change_signal (sig, (char *)IGNORE_SIG);
		return ;
	}
	get_original_signal(sig);
	if (sigmodes[sig] & SIG_HARD_IGNORE)
		return ;
	if (sigmodes[sig] & SIG_IGNORED)
	{
		sigmodes[sig] |= SIG_TRAPPED;
		return ;
	}
	if ((sigmodes[sig] & SIG_NO_TRAP) == 0)
		set_signal_handler (sig, SIG_IGN);
	change_signal (sig, (char *)IGNORE_SIG);
}
