/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   posix.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 05:48:35 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 18:47:47 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"
#include "quit.h"

/* Singleton accessor for global termination state */
struct s_termg *get_g_term(void)
{
	static struct s_termg g_term = {0};

	return (&g_term);
}

void ft_sigfillset(t_sigset *set)
{
	unsigned int n;

	/* Fill all bits safely even if NSIG > 32 */
	if (!set)
		return;
	n = (NSIG >= 32) ? 32U : (unsigned int)NSIG;
	if (n == 0)
		*set = 0U;
	else
		*set = (n == 32) ? ~0U : ((1U << n) - 1U);
}

void ft_sigaddset(t_sigset *set, int sig)
{
	*set |= ft_sigmask(sig);
}

void ft_sigdelset(t_sigset *set, int sig)
{
	*set &= ~ft_sigmask(sig);
}

void check_sigterm(void)
{
	if (get_g_term()->sigterm_received)
		termsig_handler(SIGTERM);
}

void zreset_if_needed(void)
{
	if (get_g_term()->interrupt_state)
		zreset();
}
