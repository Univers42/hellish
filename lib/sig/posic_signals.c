/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   posic_signals.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 23:46:43 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 00:18:52 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"



unsigned int ft_sigmask(int x)
{
	return (1U << (x - 1));
}

// clear all bits -- meaning no signals are part of the set
void    ft_sigemptyset(t_sigset *set)
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
    if (!set || sig <= 0 || sig >= NSIG) {
        errno = EINVAL;
        return -1;
    }
    return ((*set & (1U << (sig - 1))) != 0);
}

// fills all bits up to NSIG - 1, meaning every signal is included
void    ft_sigfillset(t_sigset *set)
{
	*set = ft_sigmask(NSIG) - 1;
}

// Turns ON the bit for a specific signal
void    ft_sigaddset(t_sigset *set, int sig)
{
	*set |= sigmask(sig);
}

// Turns OFF the bit for a specific signal
void	ft_sigdelset(t_sigset *set, int sig)
{
	*set &= ~sigmask(sig);
}

/*
 * BSD-style sigblock()
 * Adds the signals in `mask` to the process’s current signal mask.
 * Returns the previous mask (as our integer-based sigset_t).
 */
int sigblock(int mask)
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

int	ft_sigprocmask(int operation, int *newset, int *oldset)
{
	int	old;
	int	new;

	if (newset)
		new = *newset;
	else
		new = 0;
	if (operation == SIG_BLOCK)
		old = ft_sigblock(new);
	else if (operation == SIG_SET_MASK)
		old = ft_sigsetmask(new);
	else
		return (SIG_ERR);
	if (oldset)
		*oldset = old;
}

int	ft_sigsuspend(const t_sigset *set)
{
	t_sigset	oldmask;
	t_sigset	newmask;
	t_sigset	realset;
	int			sig;

	ft_sigemptyset(&realset);
	sig = 1;
	while (sig < NSIG)
	{
		if (*set & (1U << (sig - 1)))
			ft_sigaddset(&realset, sig);
	}
}

t_sig_handler *set_signal_handler(int sig, t_sig_handler *handler)
{
	return ((t_sig_handler *)signal(sig, handler));
}

int		ft_sigpause(int mask)
{
	t_sigset	newset;
	int			sig;

	ft_sigemptyset(&newset);
	sig = 1;
	while (sig < NSIG)
	{
		if (mask & (1U << (sig - 1)))
			ft_sigaddset(&newset, sig);
		sig++;
	}
	return (ft_sigsuspend(&newset));
}

t_sig_handler *set_signal_handler(int sig, t_sig_handler *handler)
{
	struct sigaction	act;
	struct sigaction	old;

	act.sa_handler = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(sig, &act, &old) < 0)
		return (SIG_ERR);
	return old.sa_handler;
}