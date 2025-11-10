/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 05:49:56 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 18:37:29 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"
#include "quit.h"

int is_term_signal(void)
{
	return (get_g_term()->terminating_signal != 0);
}

void check_termsig(void)
{
	if (get_g_term()->terminating_signal)
		termsig_handler(get_g_term()->terminating_signal);
}

int last_signal(void)
{
	if (get_g_term()->terminating_signal)
		return (get_g_term()->terminating_signal);
	if (get_g_term()->interrupt_state)
		return (SIGINT);
	return (0);
}

void reset_sigterm(void)
{
	get_g_term()->sigterm_received = 0;
}
