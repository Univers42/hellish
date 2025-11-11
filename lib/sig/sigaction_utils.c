/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sigaction_utils.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 05:47:37 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 18:33:27 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

t_sig_handler set_signal_handler(int sig, t_sig_handler handler)
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

/* Build a POSIX sigset_t from our bitmask (t_sigset) */
static void build_sigset_from_mask(t_sigset mask, sigset_t *dst)
{
	int sig;

	sigemptyset(dst);
	sig = 1;
	while (sig < NSIG)
	{
		if (mask & (1U << (sig - 1)))
			sigaddset(dst, sig);
		sig++;
	}
}

/* Convert a POSIX sigset_t to our bitmask (t_sigset) */
static void compute_oldmask(const sigset_t *oset, t_sigset *oldset)
{
	int sig;
	t_sigset omask;

	omask = 0;
	sig = 1;
	while (sig < NSIG)
	{
		if (sigismember(oset, sig))
			omask |= (1U << (sig - 1));
		sig++;
	}
	*oldset = omask;
}

int ft_sigprocmask(int operation, t_sigset *newset, t_sigset *oldset)
{
	sigset_t nset;
	sigset_t oset;
	sigset_t *nptr;

	nptr = NULL;
	if (newset)
	{
		build_sigset_from_mask(*newset, &nset);
		nptr = &nset;
	}
	if (sigprocmask(operation, nptr, &oset) < 0)
		return (-1);
	if (oldset)
		compute_oldmask(&oset, oldset);
	return (0);
}

int ft_sigsuspend(const t_sigset *set)
{
	sigset_t rset;
	t_sigset mask;

	mask = 0;
	if (set)
		mask = *set;
	build_sigset_from_mask(mask, &rset);
	return (sigsuspend(&rset));
}
