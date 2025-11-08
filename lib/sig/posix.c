/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   posix.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 05:48:35 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 05:49:35 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

// fills all bits up to NSIG - 1, meaning every signal is included
void	ft_sigfillset(t_sigset *set)
{
	*set = ft_sigmask(NSIG) - 1;
}

// Turns ON the bit for a specific signal
void	ft_sigaddset(t_sigset *set, int sig)
{
	*set |= sigmask(sig);
}

// Turns OFF the bit for a specific signal
void	ft_sigdelset(t_sigset *set, int sig)
{
	*set &= ~sigmask(sig);
}

void	check_sigterm(void)
{
	if (g_term.sigterm_received)
		termsig_handler(SIGTERM);
}

void	zreset_if_needed(void)
{
	if (g_term.interrupt_state)
		zreset();
}
