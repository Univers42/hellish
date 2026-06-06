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

/* The command hash table caches PATH lookups so `ls` does not need a full
   PATH scan on every invocation. Initial capacity 64 buckets; grow on
   demand inside hash_set. Distinct from the builtin-dispatch table —
   that one maps name -> function pointer, this one maps name -> full path. */

/* Initialise the command cache with 64 buckets. */
void	cmd_hash_init(t_hash *ht)
{
	hash_init(ht, 64);
}

/* Free a single cache entry (name + path + the struct itself). This is the
   destructor callback passed to hash_destroy — we cast to the real type to
   access the named fields rather than relying on layout offsets. */
static void	free_cmd_entry(void *ptr)
{
	t_cmd_hash_entry	*e;

	e = (t_cmd_hash_entry *)ptr;
	if (e)
	{
		xfree(e->name);
		xfree(e->path);
		xfree(e);
	}
}

void	cmd_hash_free(t_hash *ht)
{
	hash_destroy(ht, free_cmd_entry);
}

/* Look up `name` in the cache. If found, increment its hit counter and
   return the cached path. Returns NULL on a miss — caller must do a PATH
   search and then insert the result with cmd_hash_insert. */
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

/* Insert or update `name` -> `path`. Updating an existing entry resets its
   hit count to 0 so the display stays accurate after a PATH change. The
   entry struct owns copies of both strings so the caller's buffers can be
   freed without corrupting the cache. */
void	cmd_hash_insert(t_hash *ht, const char *name, const char *path)
{
	t_cmd_hash_entry	*entry;
	t_cmd_hash_entry	*old;

	old = (t_cmd_hash_entry *)hash_get(ht, name);
	if (old)
	{
		xfree(old->path);
		old->path = ft_strdup(path);
		old->hits = 0;
		return ;
	}
	entry = xmalloc(sizeof(t_cmd_hash_entry));
	if (!entry)
		return ;
	entry->name = ft_strdup(name);
	entry->path = ft_strdup(path);
	entry->hits = 0;
	hash_set(ht, entry->name, entry);
}
