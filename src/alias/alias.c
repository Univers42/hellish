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

/* Alias table: a thin wrapper around libft's t_hash, keyed on alias name,
   value is a heap-allocated t_alias_entry.  Lookup is O(1) average; the
   table starts at 32 slots and libft rehashes automatically.
   Note: alias names and values are BOTH duplicated on entry so the table
   fully owns its memory -- callers can safely free what they passed in. */

#include "sh_alias.h"
#include "libft.h"
#include <stdlib.h>

/* Initialise the alias hash table with 32 initial slots.  32 is enough
   for the typical "alias ll='ls -la'; alias la='ls -A'" workload with
   room to grow without an early rehash. */
void	alias_table_init(t_hash *aliases)
{
	hash_init(aliases, 32);
}

/* Destructor passed to hash_destroy.  Each t_alias_entry was fully
   heap-allocated so we free name, value, and the struct itself. */
static void	free_alias_entry(void *ptr)
{
	t_alias_entry	*e;

	e = (t_alias_entry *)ptr;
	if (e)
	{
		xfree(e->name);
		xfree(e->value);
		xfree(e);
	}
}

void	alias_table_free(t_hash *aliases)
{
	hash_destroy(aliases, free_alias_entry);
}

/* Upsert an alias.  If a same-named alias exists, only the value string
   is replaced (no re-allocation of the entry or the key).  If new, a
   fresh t_alias_entry is allocated and inserted.  Returns 0 on success,
   1 on OOM. */
int	alias_set(t_hash *aliases, const char *name, const char *value)
{
	t_alias_entry	*entry;
	t_alias_entry	*old;

	old = (t_alias_entry *)hash_get(aliases, name);
	if (old)
	{
		xfree(old->value);
		old->value = ft_strdup(value);
		return (0);
	}
	entry = xmalloc(sizeof(t_alias_entry));
	if (!entry)
		return (1);
	entry->name = ft_strdup(name);
	entry->value = ft_strdup(value);
	hash_set(aliases, entry->name, entry);
	return (0);
}

/* Look up an alias by name.  Returns a borrowed pointer into the entry
   (do NOT free it), or NULL if not found. */
char	*alias_get(t_hash *aliases, const char *name)
{
	t_alias_entry	*e;

	e = (t_alias_entry *)hash_get(aliases, name);
	if (e)
		return (e->value);
	return (NULL);
}
