/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 18:40:02 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 18:36:23 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

int valid_number(const char *s, intmax_t *out)
{
	char *end;
	long long v;

	if (!s || !*s)
		return (0);
	errno = 0;
	v = strtoll(s, &end, 10);
	if (errno || *end != '\0')
		return (0);
	*out = (intmax_t)v;
	return (1);
}

void restore_bash_trapsig(char *oldval)
{
	if (oldval == 0)
		unbind_variable_noref("SH_TRAPSIG");
	else
	{
		bind_variable("SH_TRAPSIG", oldval, 0);
		free(oldval);
	}
}

void block_signal(int sig, t_sigset *set, t_sigset *oset)
{
	if (!set || !oset)
		return;
	ft_sigemptyset(set);
	ft_sigaddset(set, sig);
	ft_sigprocmask(SIG_BLOCK, set, oset);
}

void unblock_signal(const t_sigset *oset)
{
	if (!oset)
		return;
	ft_sigprocmask(SIG_SETMASK, (t_sigset *)oset, NULL);
}
