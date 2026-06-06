/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_ulimit2.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <sys/resource.h>

/* Show every resource in the table with its label (the -a output). `hard`
   follows the same -1/0/1 convention: -1 means show the soft limit. */
static void	ulimit_show_all(int hard)
{
	const t_ulim	*t;
	int				i;

	t = ulim_table();
	i = 0;
	while (t[i].label)
	{
		ulimit_show(&t[i], hard == 1, 1);
		i++;
	}
}

/* Parse the option words: -H/-S set the hard/soft selector, -a returns the
   special sentinel -2 to trigger show_all, and any other letter selects
   the resource. Returns the index of the first non-flag argument (i.e. the
   limit value, if present) — or -2 for -a. */
static int	ulimit_parse_flags(char **av, int len, char *opt, int *hard)
{
	int	i;

	i = 0;
	while (++i < len && av[i][0] == '-' && av[i][1])
	{
		if (av[i][1] == 'H' || av[i][1] == 'S')
			*hard = (av[i][1] == 'H');
		else if (av[i][1] == 'a')
			return (-2);
		else
			*opt = av[i][1];
	}
	return (i);
}

/* ulimit [-HS] [-acdfnstuv] [limit] : query or set resource limits. */
int	builtin_ulimit(t_shell *state, t_vec argv)
{
	char			**av;
	const t_ulim	*t;
	char			opt;
	int				hard;
	int				i;

	av = (char **)argv.ctx;
	opt = 'f';
	hard = -1;
	i = ulimit_parse_flags(av, (int)argv.len, &opt, &hard);
	if (i == -2)
		return (ulimit_show_all(hard), 0);
	t = ulim_table();
	while (t->label && t->opt != opt)
		t++;
	if (!t->label)
		return (ft_eprintf("%s: ulimit: invalid option\n", state->ctx), 1);
	if (i < (int)argv.len)
		return (ulimit_set(state, t, av[i], hard));
	return (ulimit_show(t, hard == 1, 0), 0);
}
