/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_unalias.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "sh_alias.h"

static void	unalias_all(t_shell *state)
{
	alias_table_free(&state->aliases);
	alias_table_init(&state->aliases);
}

int	builtin_unalias(t_shell *state, t_vec argv)
{
	size_t	i;
	int		ret;
	char	**av;

	av = (char **)argv.ctx;
	if (argv.len < 2)
	{
		ft_eprintf("%s: unalias: usage: unalias [-a] name ...\n", state->ctx);
		return (2);
	}
	if (ft_strcmp(av[1], "-a") == 0)
	{
		unalias_all(state);
		return (0);
	}
	ret = 0;
	i = 1;
	while (i < argv.len)
	{
		if (alias_remove(&state->aliases, av[i]))
		{
			ft_eprintf("%s: unalias: %s: not found\n", state->ctx, av[i]);
			ret = 1;
		}
		i++;
	}
	return (ret);
}
