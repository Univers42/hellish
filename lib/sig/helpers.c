/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 05:49:56 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 08:51:30 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"
#include "quit.h"

int	is_term_signal(void)
{
	return (g_term.terminating_signal != 0);
}

void	check_termsig(void)
{
	if (g_term.terminating_signal)
		termsig_handler(g_term.terminating_signal);
}

int	last_signal(void)
{
	if (g_term.terminating_signal)
		return (g_term.terminating_signal);
	if (g_term.interrupt_state)
		return (SIGINT);
	return (0);
}

void	reset_sigterm(void)
{
	g_term.sigterm_received = 0;
}
