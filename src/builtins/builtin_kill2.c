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

/* Parse the optional signal specifier from the argument list. Accepts:
     -TERM  or  -15   (embedded in the first word)
     -s TERM          (next word is the signal name)
     -n 15            (next word is the signal number)
   Default when none of these match is SIGTERM. Advances *i past the
   specifier words so the caller knows where the target list starts. */
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

/* kill -l | kill [-s SIG | -SIG | -n NUM] target ...: send a signal to
   processes or job specs. Targets beginning with '%' are job specs resolved
   via job_by_spec (signal goes to the whole process group, hence
   kill(-pgid, sig)). Plain integers are treated as PIDs. Returns the
   bitwise-or of all individual target results so a single failure propagates
   even when other targets succeed. */
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
