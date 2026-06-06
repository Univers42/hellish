/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_index2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Incremental update and lookup for the env hash index.
   The dirty flag is the escape hatch: any operation that shuffles vector
   entries (unset, bulk import) marks the index dirty so the next lookup
   rebuilds from scratch.  Staying dirty is always correct, just slower. */

#include "env_private.h"

/* Called whenever the env vector is structurally changed in a way that
   could invalidate cached positions (e.g. unset shifts later entries).
   The next env_index_find will trigger a full rebuild automatically. */
void	env_index_mark_dirty(void)
{
	g_dirty = 1;
}

/* Add one freshly-appended entry at position idx (key NUL-terminated). */
void	env_index_add(t_vec_env *env, int idx)
{
	if (g_dirty)
		return ;
	if (!g_tab || (g_count + 1) * 10 >= g_cap * 7)
	{
		env_index_reset(env);
		return ;
	}
	eix_put(eix_hash(((t_env *)env->ctx)[idx].key, -1), idx);
}

/* O(1) lookup by name.  Two-level check: hash table says "try slot m",
   then we verify the actual key bytes in the vector.  This means a hash
   collision or a stale slot (after an unset+re-add) can never silently
   return the wrong variable -- worst case we fall through to -1.
   len < 0 means "use strlen", handy when callers already know the key
   is NUL-terminated and don't want to re-measure. */
int	env_index_find(t_vec_env *env, const char *key, int len)
{
	unsigned long	h;
	size_t			m;
	t_env			*e;

	if (g_dirty || !g_tab)
		env_index_reset(env);
	if (!g_tab)
		return (-1);
	if (len < 0)
		len = (int)ft_strlen(key);
	h = eix_hash(key, len);
	m = h & (g_cap - 1);
	while (g_tab[m].idx >= 0)
	{
		if (g_tab[m].h == h && (size_t)g_tab[m].idx < env->len)
		{
			e = &((t_env *)env->ctx)[g_tab[m].idx];
			if (ft_strncmp(e->key, key, len) == 0 && e->key[len] == 0)
				return (g_tab[m].idx);
		}
		m = (m + 1) & (g_cap - 1);
	}
	return (-1);
}
