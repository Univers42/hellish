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

/* Nuke the whole alias table by freeing it and re-initialising. Cheaper than
   iterating and removing one by one, and `-a` semantics require exactly this:
   all aliases gone, none spared. */
static void	unalias_all(t_shell *state)
{
	alias_table_free(&state->aliases);
	alias_table_init(&state->aliases);
}

/* Remove named aliases one by one, accumulating an error if any name was not
   in the table. The standard says exit status 1 for "no such alias". */
static int	unalias_names(t_shell *state, t_vec argv)
{
	size_t	i;
	int		ret;
	char	**av;

	av = (char **)argv.ctx;
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

/* unalias [-a] name ...: remove aliases. Without -a, at least one name must
   be given — we return 2 (usage error) rather than silently succeeding, which
   makes it easier to catch `unalias` with no args in scripts. */
int	builtin_unalias(t_shell *state, t_vec argv)
{
	char	**av;

	av = (char **)argv.ctx;
	if (argv.len < 2)
	{
		ft_eprintf("%s: unalias: usage: unalias [-a] name ...\n",
			state->ctx);
		return (2);
	}
	if (ft_strcmp(av[1], "-a") == 0)
	{
		unalias_all(state);
		return (0);
	}
	return (unalias_names(state, argv));
}
