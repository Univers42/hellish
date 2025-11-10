/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   posix_signals.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 23:46:43 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 18:26:53 by dlesieur         ###   ########.fr       */
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
 * ft_sigblock: build a sigset_t from integer mask and block those signals
 * Returns the previous mask (as our integer-based sigset_t).
 */
int	ft_sigblock(int mask)
{
	t_sigset	blockset;
	t_sigset	oldset;
	int			sig;
	int			oldmask;

	ft_sigemptyset(&blockset);
	sig = 1;
	while (sig < NSIG)
	{
		if (mask & (1U << (sig - 1)))
			ft_sigaddset(&blockset, sig);
		sig++;
	}
	if (ft_sigprocmask(SIG_BLOCK, &blockset, &oldset) < 0)
		return (-1);
	oldmask = 0;
	sig = 1;
	while (sig < NSIG)
	{
		if (ft_sigismember(&oldset, sig))
			oldmask |= (1U << (sig - 1));
		sig++;
	}
	return (oldmask);
}
