/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sigaction_utils.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 05:47:37 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 09:03:37 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void	(*set_signal_handler(int sig, void (*handler)(int)))(int)
{
	struct sigaction act;
	struct sigaction old;

	act.sa_handler = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(sig, &act, &old) < 0)
		return (SIG_ERR);
	return (old.sa_handler);
}

static void build_sigset_from_mask(int mask, sigset_t *dst)
{
	sigemptyset(dst);
	for (int sig = 1; sig < NSIG; ++sig)
		if (mask & (1U << (sig - 1)))
			sigaddset(dst, sig);
}

int ft_sigprocmask(int operation, int *newset, int *oldset)
{
	sigset_t nset;
	sigset_t oset;
	int mask = newset ? *newset : 0;

	if (newset)
		build_sigset_from_mask(mask, &nset);
	if (sigprocmask(operation, newset ? &nset : NULL, &oset) < 0)
		return (-1);
	if (oldset)
	{
		int omask = 0;
		for (int sig = 1; sig < NSIG; ++sig)
			if (sigismember(&oset, sig))
				omask |= (1U << (sig - 1));
		*oldset = omask;
	}
	return (0);
}

int ft_sigsuspend(const t_sigset *set)
{
	sigset_t rset;
	int mask = set ? *set : 0;
	build_sigset_from_mask(mask, &rset);
	return (sigsuspend(&rset));
}
