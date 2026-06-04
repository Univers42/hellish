/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_getopts3.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

static int	gopt_done(t_shell *state, t_getopts *g, char *cur)
{
	gopt_set_char(state, g->name, '?');
	state->getopts_pos = 0;
	if (cur && cur[1] == '-')
		gopt_commit_optind(state, g->optind + 1);
	else
		gopt_commit_optind(state, g->optind);
	return (1);
}

int	builtin_getopts(t_shell *state, t_vec argv)
{
	t_getopts	g;
	char		*cur;

	if (argv.len < 3)
		return (ft_eprintf(
				"getopts: usage: getopts optstring name [arg]\n"), 2);
	gopt_init(state, argv, &g);
	if (state->getopts_pos < 1)
		state->getopts_pos = 1;
	if (g.optind <= g.count)
		cur = gopt_arg(state, argv, g.optind);
	else
		cur = NULL;
	if (!cur || cur[0] != '-' || !cur[1]
		|| (cur[1] == '-' && !cur[2]))
		return (gopt_done(state, &g, cur));
	if (!cur[state->getopts_pos])
	{
		state->getopts_pos = 0;
		gopt_commit_optind(state, g.optind + 1);
		return (builtin_getopts(state, argv));
	}
	return (one_option(state, argv, &g, cur));
}
