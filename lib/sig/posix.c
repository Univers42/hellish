/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   posix.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 05:48:35 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 09:14:20 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"
#include "quit.h"

// fills all bits up to NSIG - 1, meaning every signal is included
void	ft_sigfillset(t_sigset *set)
{
	*set = ft_sigmask(NSIG) - 1;
}

// Turns ON the bit for a specific signal
void	ft_sigaddset(t_sigset *set, int sig)
{
	*set |= ft_sigmask(sig);
}

// Turns OFF the bit for a specific signal
void	ft_sigdelset(t_sigset *set, int sig)
{
	*set &= ~ft_sigmask(sig);
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

void	termsig_handler(int sig)
{
	(void)sig; // stub
}

void	throw_to_top_level(void)
{
	// stub
}

void	check_read_timeout(void)
{
	// stub
}

void	zreset(void)
{
	// stub
}

void	initialize_terminating_signals(void)
{
	// stub
}

void	block_signal(int sig, sigset_t *set, sigset_t *oset)
{
	if (!set || !oset)
		return;
	sigemptyset(set);
	sigaddset(set, sig);
	sigprocmask(SIG_BLOCK, set, oset);
}

void	unblock_signal(const sigset_t *oset)
{
	if (!oset)
		return;
	sigprocmask(SIG_SETMASK, oset, NULL);
}
