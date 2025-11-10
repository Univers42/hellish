/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_sigint_handler.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:31 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 19:24:16 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void trap_handler(int sig)
{
	/* Record trap occurrence so pending queue APIs can be demonstrated */
	set_trap_state(sig);
}

void sigint_sighandler(int sig)
{
	(void)sig;
}

void termsig_sighandler(int sig)
{
	(void)sig;
}

void *set_sigint_handler(void)
{
	if (get_g_sig()->sigmodes[SIGINT] & SIG_HARD_IGNORE)
		return ((void *)SIG_IGN);
	else if (get_g_sig()->sigmodes[SIGINT] & SIG_IGNORED)
		return (set_signal_handler(SIGINT, SIG_IGN));
	else if (get_g_sig()->sigmodes[SIGINT] & SIG_TRAPPED)
		return (set_signal_handler(SIGINT, trap_handler));
	else if (get_g_sig()->interactive)
		return (set_signal_handler(SIGINT, sigint_sighandler));
	else
		return (set_signal_handler(SIGINT, termsig_sighandler));
}
