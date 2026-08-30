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

/* `hash -l`: the same table as `hash`, printed as commands that would
   rebuild it -- `builtin hash -p PATH NAME` per entry, which is what makes
   the output pipeable back into a shell. An empty table prints nothing at
   all (bash does the same) rather than a header with no rows. */
void	cmd_hash_print_reusable(t_hash *ht)
{
	size_t				i;
	t_hash_entry		*entries;
	t_cmd_hash_entry	*ce;

	entries = (t_hash_entry *)ht->ctx;
	i = 0;
	while (ht->len && i < ht->cap)
	{
		if (entries[i].key && entries[i].value)
		{
			ce = (t_cmd_hash_entry *)entries[i].value;
			ft_printf("builtin hash -p %s %s\n", ce->path, entries[i].key);
		}
		i++;
	}
}

/* Add each operand to the cache. Accumulates errors so a missing name does
   not abort the rest, but the overall exit status is 1 if any failed. */
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

/* hash [-r] [-d name] [name ...]: manage the command-path cache. With no
   arguments, print the full table. A leading '-' word goes to handle_hash_flags
   (which returns -1 when the flag is not one of the two it knows). Anything
   else is treated as a list of names to pre-cache. */
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
