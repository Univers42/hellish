/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alias.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "sh_alias.h"
#include "libft.h"
#include <stdlib.h>

void	alias_table_init(t_hash *aliases)
{
	hash_init(aliases, 32);
}

static void	free_alias_entry(void *ptr)
{
	t_alias_entry	*e;

	e = (t_alias_entry *)ptr;
	if (e)
	{
		free(e->name);
		free(e->value);
		free(e);
	}
}

void	alias_table_free(t_hash *aliases)
{
	hash_destroy(aliases, free_alias_entry);
}

int	alias_set(t_hash *aliases, const char *name, const char *value)
{
	t_alias_entry	*entry;
	t_alias_entry	*old;

	old = (t_alias_entry *)hash_get(aliases, name);
	if (old)
	{
		free(old->value);
		old->value = ft_strdup(value);
		return (0);
	}
	entry = malloc(sizeof(t_alias_entry));
	if (!entry)
		return (1);
	entry->name = ft_strdup(name);
	entry->value = ft_strdup(value);
	hash_set(aliases, entry->name, entry);
	return (0);
}

char	*alias_get(t_hash *aliases, const char *name)
{
	t_alias_entry	*e;

	e = (t_alias_entry *)hash_get(aliases, name);
	if (e)
		return (e->value);
	return (NULL);
}
