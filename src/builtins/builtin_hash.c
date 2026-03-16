/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_hash.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "cmd_hash.h"
#include "env.h"

void	cmd_hash_init(t_hash *ht)
{
	hash_init(ht, 64);
}

static void	free_cmd_entry(void *ptr)
{
	t_cmd_hash_entry	*e;

	e = (t_cmd_hash_entry *)ptr;
	if (e)
	{
		free(e->name);
		free(e->path);
		free(e);
	}
}

void	cmd_hash_free(t_hash *ht)
{
	hash_destroy(ht, free_cmd_entry);
}

char	*cmd_hash_lookup(t_hash *ht, const char *name)
{
	t_cmd_hash_entry	*e;

	e = (t_cmd_hash_entry *)hash_get(ht, name);
	if (e)
	{
		e->hits++;
		return (e->path);
	}
	return (NULL);
}

void	cmd_hash_insert(t_hash *ht, const char *name, const char *path)
{
	t_cmd_hash_entry	*entry;
	t_cmd_hash_entry	*old;

	old = (t_cmd_hash_entry *)hash_get(ht, name);
	if (old)
	{
		free(old->path);
		old->path = ft_strdup(path);
		old->hits = 0;
		return ;
	}
	entry = malloc(sizeof(t_cmd_hash_entry));
	if (!entry)
		return ;
	entry->name = ft_strdup(name);
	entry->path = ft_strdup(path);
	entry->hits = 0;
	hash_set(ht, entry->name, entry);
}

void	cmd_hash_remove(t_hash *ht, const char *name)
{
	t_cmd_hash_entry	*e;
	int					idx;

	e = (t_cmd_hash_entry *)hash_get(ht, name);
	if (!e)
		return ;
	idx = hash_find_idx(ht, name);
	if (idx < 0)
		return ;
	((t_hash_entry *)ht->ctx)[idx].key = NULL;
	((t_hash_entry *)ht->ctx)[idx].value = NULL;
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

static int	hash_add_from_path(t_shell *state, const char *name)
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

static int	handle_hash_flags(t_shell *state, char **av, int ac)
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

int	builtin_hash(t_shell *state, t_vec argv)
{
	char	**av;
	int		ac;
	int		ret;
	int		i;

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
