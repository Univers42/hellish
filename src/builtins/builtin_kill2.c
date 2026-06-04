/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_kill2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "job_control.h"
#include <signal.h>
#include <errno.h>
#include <string.h>

static int	kill_parse_sig(char **av, int *i)
{
	int	sig;

	sig = SIGTERM;
	if (av[1][0] == '-' && av[1][1])
	{
		if ((!ft_strcmp(av[1], "-s") || !ft_strcmp(av[1], "-n")) && av[2])
			sig = kill_sig_from_name(av[++(*i)]);
		else
			sig = kill_sig_from_name(av[1] + 1);
		(*i)++;
	}
	return (sig);
}

/* kill -l | kill [-s SIG | -SIG | -n NUM] target ... */
int	builtin_kill(t_shell *state, t_vec argv)
{
	char	**av;
	int		sig;
	int		i;
	int		ret;

	av = (char **)argv.ctx;
	if (argv.len >= 2 && !ft_strcmp(av[1], "-l"))
		return (kill_list_sigs());
	i = 1;
	sig = kill_parse_sig(av, &i);
	if (sig < 0)
		return (ft_eprintf("%s: kill: %s: invalid signal\n",
				state->ctx, av[1]), 1);
	if (i >= (int)argv.len)
		return (ft_eprintf("%s: kill: usage: kill [-s sig|-n num|-sig]"
				" pid|%%job ...\n", state->ctx), 1);
	ret = 0;
	while (i < (int)argv.len)
		ret |= kill_one_target(state, av[i++], sig);
	return (ret);
}
