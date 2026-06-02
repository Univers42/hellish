/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_hash2.c                                    :+:      :+:    :+:   */
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

void	cmd_hash_remove(t_hash *ht, const char *name)
{
	t_cmd_hash_entry	*e;

	e = (t_cmd_hash_entry *)hash_del(ht, name);
	if (!e)
		return ;
	free(e->name);
	free(e->path);
	free(e);
}

void	cmd_hash_clear(t_hash *ht)
{
	cmd_hash_free(ht);
	cmd_hash_init(ht);
}

void	cmd_hash_print_all(t_hash *ht)
{
	size_t				i;
	t_hash_entry		*entries;
	t_cmd_hash_entry	*ce;

	entries = (t_hash_entry *)ht->ctx;
	ft_printf("hits\tcommand\n");
	i = 0;
	while (i < ht->cap)
	{
		if (entries[i].key && entries[i].value)
		{
			ce = (t_cmd_hash_entry *)entries[i].value;
			ft_printf("  %2d\t%s\n", ce->hits, ce->path);
		}
		i++;
	}
}

int	hash_add_from_path(t_shell *state, const char *name)
{
	char	*path;
	char	**dirs;
	int		perm;

	path = env_expand(state, "PATH");
	if (!path)
		return (1);
	dirs = ft_split(path, ':');
	if (!dirs)
		return (1);
	perm = 0;
	path = exe_path(dirs, (char *)name, &perm);
	free_tab(dirs);
	if (!path)
	{
		ft_eprintf("%s: hash: %s: not found\n", state->ctx, name);
		return (1);
	}
	cmd_hash_insert(&state->cmd_cache, name, path);
	free(path);
	return (0);
}

int	handle_hash_flags(t_shell *state, char **av, int ac)
{
	if (ft_strcmp(av[1], "-r") == 0)
	{
		cmd_hash_clear(&state->cmd_cache);
		return (0);
	}
	if (ft_strcmp(av[1], "-d") == 0 && ac >= 3)
	{
		cmd_hash_remove(&state->cmd_cache, av[2]);
		return (0);
	}
	return (-1);
}
