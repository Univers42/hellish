/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   change_signal.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:51:56 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 14:53:03 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	change_signal(int sig, char *value)
{
	if ((get_g_sig()->sigmodes[sig] & SIG_INPROGRESS) == 0)
		free_trap_command(sig);
	get_g_sig()->trap_list[sig] = value;
	get_g_sig()->sigmodes[sig] |= SIG_TRAPPED;
	if (value == (char *)IGNORE_SIG)
		get_g_sig()->sigmodes[sig] |= SIG_IGNORED;
	else
		get_g_sig()->sigmodes[sig] &= ~SIG_IGNORED;
	if (get_g_sig()->sigmodes[sig] & SIG_INPROGRESS)
		get_g_sig()->sigmodes[sig] |= SIG_CHANGED;
}
