/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_sigint_handler.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:53:31 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 07:00:15 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

/* Reset the SIGINT handler so that subshells that are doing `shellsy'
	 things, like waiting for command substitution or executing commands
	 in explicit subshells ( ( cmd ) ), can catch interrupts properly. */
void	*set_sigint_handler(void)
{
	if (sigmodes[SIGINT] & SIG_HARD_IGNORE)
		return ((SigHandler *)SIG_IGN);
	else if (sigmodes[SIGINT] & SIG_IGNORED)
		return ((SigHandler *)set_signal_handler (SIGINT, SIG_IGN));
	else if (sigmodes[SIGINT] & SIG_TRAPPED)
		return ((SigHandler *)set_signal_handler (SIGINT, trap_handler));
	else if (interactive)
		return (set_signal_handler (SIGINT, sigint_sighandler));
	else
		return (set_signal_handler (SIGINT, termsig_sighandler));
}
