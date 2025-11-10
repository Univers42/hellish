/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   trap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 21:16:02 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 18:22:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

t_glob_sig	*get_g_sig(void)
{
	static t_glob_sig	g_sig = {0};

	return (&g_sig);
}

void	*trap_to_sighandler(int sig)
{
	if (get_g_sig()->sigmodes[sig] & (SIG_IGNORED | SIG_HARD_IGNORE))
		return (SIG_IGN);
	else if (get_g_sig()->sigmodes[sig] & SIG_TRAPPED)
		return (trap_handler);
	else
		return (SIG_DFL);
}

void	trap_if_untrapped(int sig, const char *command)
{
	if ((get_g_sig()->sigmodes[sig] & SIG_TRAPPED) == 0)
		set_signal(sig, command);
}

int	signal_is_trapped(int sig)
{
	return (get_g_sig()->sigmodes[sig] & SIG_TRAPPED);
}
