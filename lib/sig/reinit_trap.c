/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reinit_trap.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:52:35 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 02:59:38 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Change the flags associated with signal SIG without changing the trap
	 string. The string is TRAP_LIST[SIG] if we need it. */
void	reinit_trap(int sig)
{
	g_sig.sigmodes[sig] |= SIG_TRAPPED;
	if (g_sig.trap_list[sig] == (char *)IGNORE_SIG)
		g_sig.sigmodes[sig] |= SIG_IGNORED;
	else
		g_sig.sigmodes[sig] &= ~SIG_IGNORED;
	if (g_sig.sigmodes[sig] & SIG_INPROGRESS)
		g_sig.sigmodes[sig] |= SIG_CHANGED;
}
