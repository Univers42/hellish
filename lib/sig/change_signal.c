/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   change_signal.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:51:56 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 08:45:03 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* If SIG has a string assigned to it, get rid of it.  Then give it
	 VALUE. */
void	change_signal(int sig, char *value)
{
	if ((g_sig.sigmodes[sig] & SIG_INPROGRESS) == 0)
		free_trap_command (sig);
	g_sig.trap_list[sig] = value;
	g_sig.sigmodes[sig] |= SIG_TRAPPED;
	if (value == (char *)IGNORE_SIG)
		g_sig.sigmodes[sig] |= SIG_IGNORED;
	else
		g_sig.sigmodes[sig] &= ~SIG_IGNORED;
	if (g_sig.sigmodes[sig] & SIG_INPROGRESS)
		g_sig.sigmodes[sig] |= SIG_CHANGED;
}
