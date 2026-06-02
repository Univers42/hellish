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

#include "env_private.h"

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

/* Return the vector position of `key` (len bytes, or strlen if len<0).
   Verifies against the vector; returns -1 if not found. */
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
