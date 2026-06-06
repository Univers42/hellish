/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_private.h                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Private implementation details for the O(1) env-name index.
   Included only by env_index.c and env_index2.c -- nothing else should
   touch the globals directly.  The index is a lazy open-addressing hash
   table: rebuild is deferred until the first lookup after a structural
   change (marked by g_dirty=1). */

#ifndef ENV_PRIVATE_H
# define ENV_PRIVATE_H

# include "env.h"
# include "libft.h"

/* One slot in the hash table: the full hash (for quick comparison before
   the key strcmp) and the index into the env vector. idx==-1 means empty. */
typedef struct s_eix
{
	unsigned long	h; /* cached hash of the key */
	int				idx; /* position in the t_vec_env; -1 = free slot */
}	t_eix;

/* Globals owned by env_index.c.  All mutable state for the index lives
   here; env_index_reset allocates/resizes g_tab as needed. */
extern t_eix	*g_tab; /* heap-allocated hash table of t_eix slots */
extern size_t	g_cap; /* current capacity (always a power of two) */
extern size_t	g_count; /* number of occupied slots */
extern int		g_dirty; /* 1 = index is stale, rebuild before next use */

unsigned long	eix_hash(const char *s, int len);
void			eix_put(unsigned long h, int idx);
void			env_index_reset(t_vec_env *env);
void			env_index_mark_dirty(void);
void			env_index_free(void);
void			env_index_add(t_vec_env *env, int idx);
int				env_index_find(t_vec_env *env,
					const char *key, int len);

#endif
