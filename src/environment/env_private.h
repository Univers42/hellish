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

#ifndef ENV_PRIVATE_H
# define ENV_PRIVATE_H

# include "env.h"
# include "libft.h"

typedef struct s_eix
{
	unsigned long	h;
	int				idx;
}	t_eix;

extern t_eix	*g_tab;
extern size_t	g_cap;
extern size_t	g_count;
extern int		g_dirty;

unsigned long	eix_hash(const char *s, int len);
void			eix_put(unsigned long h, int idx);
void			env_index_reset(t_vec_env *env);
void			env_index_mark_dirty(void);
void			env_index_free(void);
void			env_index_add(t_vec_env *env, int idx);
int				env_index_find(t_vec_env *env,
					const char *key, int len);

#endif
