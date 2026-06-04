/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_hash3.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "cmd_hash.h"
#include "env.h"

static int	hash_add_names(t_shell *state, char **av, int ac)
{
	int	ret;
	int	i;

	ret = 0;
	i = 1;
	while (i < ac)
	{
		if (hash_add_from_path(state, av[i]))
			ret = 1;
		i++;
	}
	return (ret);
}

int	builtin_hash(t_shell *state, t_vec argv)
{
	char	**av;
	int		ac;
	int		ret;

	av = (char **)argv.ctx;
	ac = (int)argv.len;
	if (ac == 1)
	{
		cmd_hash_print_all(&state->cmd_cache);
		return (0);
	}
	if (av[1][0] == '-')
	{
		ret = handle_hash_flags(state, av, ac);
		if (ret >= 0)
			return (ret);
	}
	return (hash_add_names(state, av, ac));
}
