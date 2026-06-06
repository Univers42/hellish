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
   of truth; this is only a fast-path lookup. Every probe verifies against
   the vector, so a stale slot can never return a wrong entry. */

#include "env_private.h"

t_eix	*g_tab;
size_t	g_cap;
size_t	g_count;
int		g_dirty = 1;

unsigned long	eix_hash(const char *s, int len)
{
	unsigned long	h;
	int				i;

	h = 5381;
	i = 0;
	while (s[i] && (len < 0 || i < len))
		h = ((h << 5) + h) + (unsigned char)s[i++];
	return (h);
}

void	env_index_free(void)
{
	xfree(g_tab);
	g_tab = NULL;
	g_cap = 0;
	g_count = 0;
	g_dirty = 1;
}

/* Insert (hash, idx) by linear probing. Table never full (load < 0.7). */
void	eix_put(unsigned long h, int idx)
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

static void	eix_reset_fill(t_vec_env *env, size_t cap)
{
	size_t	i;

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

/* Rebuild the whole index from the vector (sized to keep load < 0.5). */
void	env_index_reset(t_vec_env *env)
{
	size_t	cap;

	cap = 16;
	while (cap < (env->len + 1) * 2)
		cap *= 2;
	xfree(g_tab);
	g_tab = xmalloc(cap * sizeof(t_eix));
	if (!g_tab)
	{
		g_cap = 0;
		g_dirty = 1;
		return ;
	}
	eix_reset_fill(env, cap);
}
