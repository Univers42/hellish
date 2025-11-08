/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sigaction_utils.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 05:47:37 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 05:48:05 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

t_sig_handler	*set_signal_handler(int sig, t_sig_handler *handler)
{
	return ((t_sig_handler *)signal(sig, handler));
}

int	ft_sigpause(int mask)
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

t_sig_handler	*set_signal_handler(int sig, t_sig_handler *handler)
{
	t_sigaction	act;
	t_sigaction	old;

	act.sa_handler = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(sig, &act, &old) < 0)
		return (SIG_ERR);
	return (old.sa_handler);
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
