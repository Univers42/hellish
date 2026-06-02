/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_index.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* O(1) name->position index over the env vector. The vector stays the source
   of truth (order preserved for export/env listing); this is only a fast path.
   Every probe verifies against the vector, so a stale/colliding slot can never
   return a wrong entry — at worst it falls through to a rebuild. Rebuilt lazily
   when dirty (set by try_unset, the only site that reorders entries). */

#include "env.h"
#include "libft.h"

typedef struct s_eix
{
	unsigned long	h;
	int				idx;
}	t_eix;

static t_eix	*g_tab;
static size_t	g_cap;
static size_t	g_count;
static int		g_dirty = 1;

static unsigned long	eix_hash(const char *s, int len)
{
	unsigned long	h;
	int				i;

	h = 5381;
	i = 0;
	while (s[i] && (len < 0 || i < len))
		h = ((h << 5) + h) + (unsigned char)s[i++];
	return (h);
}

void	env_index_mark_dirty(void)
{
	g_dirty = 1;
}

void	env_index_free(void)
{
	free(g_tab);
	g_tab = NULL;
	g_cap = 0;
	g_count = 0;
	g_dirty = 1;
}

/* Insert (hash, idx) by linear probing. Table never full (load kept < 0.7). */
static void	eix_put(unsigned long h, int idx)
{
	size_t	m;

	if (!g_tab)
		return ;
	m = h & (g_cap - 1);
	while (g_tab[m].idx >= 0)
		m = (m + 1) & (g_cap - 1);
	g_tab[m].h = h;
	g_tab[m].idx = idx;
	g_count++;
}

/* Rebuild the whole index from the vector (sized to keep load < 0.5). */
static void	env_index_reset(t_vec_env *env)
{
	size_t	cap;
	size_t	i;

	cap = 16;
	while (cap < (env->len + 1) * 2)
		cap *= 2;
	free(g_tab);
	g_tab = malloc(cap * sizeof(t_eix));
	if (!g_tab)
	{
		g_cap = 0;
		g_dirty = 1;
		return ;
	}
	g_cap = cap;
	g_count = 0;
	i = 0;
	while (i < cap)
		g_tab[i++].idx = -1;
	i = 0;
	while (i < env->len)
	{
		eix_put(eix_hash(((t_env *)env->ctx)[i].key, -1), (int)i);
		i++;
	}
	g_dirty = 0;
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

/* Return the vector position of `key` (first `len` bytes, NUL-terminated at
   key[len]), or -1. `len < 0` means strlen(key). Verifies against the vector. */
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
