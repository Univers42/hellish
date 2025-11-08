/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_sigint_handler.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:31 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 09:01:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

// Stub handlers
void	trap_handler(int sig)
{
	(void)sig;
}

void	sigint_sighandler(int sig)
{
	(void)sig;
}

void	termsig_sighandler(int sig)
{
	(void)sig;
}

/* Reset the SIGINT handler so that subshells that are doing `shellsy'
	 things, like waiting for command substitution or executing commands
	 in explicit subshells ( ( cmd ) ), can catch interrupts properly. */
void	*set_sigint_handler(void)
{
	if (g_sig.sigmodes[SIGINT] & SIG_HARD_IGNORE)
		return ((void *)SIG_IGN);
	else if (g_sig.sigmodes[SIGINT] & SIG_IGNORED)
		return (set_signal_handler (SIGINT, SIG_IGN));
	else if (g_sig.sigmodes[SIGINT] & SIG_TRAPPED)
		return (set_signal_handler (SIGINT, trap_handler));
	else if (g_sig.interactive)
		return (set_signal_handler (SIGINT, sigint_sighandler));
	else
		return (set_signal_handler (SIGINT, termsig_sighandler));
}
