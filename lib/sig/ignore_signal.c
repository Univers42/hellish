/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ignore_signal.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:23 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 15:12:35 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	ignore_signal(int sig)
{
	if (spec_trap(sig) && ((get_g_sig()->sigmodes[sig] & SIG_IGNORED) == 0))
	{
		change_signal(sig, (char *)IGNORE_SIG);
		return ;
	}
	get_original_signal(sig);
	if (get_g_sig()->sigmodes[sig] & SIG_HARD_IGNORE)
		return ;
	if (get_g_sig()->sigmodes[sig] & SIG_IGNORED)
	{
		get_g_sig()->sigmodes[sig] |= SIG_TRAPPED;
		return ;
	}
	if ((get_g_sig()->sigmodes[sig] & SIG_NO_TRAP) == 0)
		set_signal_handler(sig, SIG_IGN);
	change_signal(sig, (char *)IGNORE_SIG);
}
