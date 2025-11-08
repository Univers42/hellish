/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   posic_signals.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 23:46:43 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 05:48:52 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

unsigned int	ft_sigmask(int x)
{
	return (1U << (x - 1));
}

// clear all bits -- meaning no signals are part of the set
void	ft_sigemptyset(t_sigset *set)
{
	*set = 0;
}

/*
 * Check whether `sig` is a member of the bitmask-style signal set `set`.
 * Returns:
 *   1 if present
 *   0 if not present
 *  -1 if sig is invalid
 */
int	ft_sigismember(const t_sigset *set, int sig)
{
	if (!set || sig <= 0 || sig >= NSIG)
	{
		errno = EINVAL;
		return (-1);
	}
	return ((*set & (1U << (sig - 1))) != 0);
}

/*
 * BSD-style sigblock()
 * Adds the signals in `mask` to the process’s current signal mask.
 * Returns the previous mask (as our integer-based sigset_t).
 */
int	sigblock(int mask)
{
	t_sigset	blockset;
	t_sigset	oldset;
	int			sig;
	int			oldmask;

	sigemptyset(&blockset);
	sig = 1;
	while (sig < NSIG)
	{
		if (mask & (1U << (sig - 1)))
			ft_sigaddset(&blockset, sig);
		sig++;
	}
	if (sigprocmask(SIG_BLOCK, &blockset, &oldset) < 0)
		return (-1);
	oldmask = 0;
	sig = 1;
	while (sig < NSIG)
	{
		if (sigismember(&oldset, sig))
			oldmask |= (1U << (sig - 1));
	}
	return (oldmask);
}

// retrn 1 if the bit is set  otherwise 0.
int	ft_sigismember(const t_sigset *set, int sig)
{
	return ((*set & sigmask(sig)) != 0);
}
