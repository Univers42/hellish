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
