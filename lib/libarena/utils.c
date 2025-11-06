/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 19:30:23 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/06 20:44:16 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arena.h"

size_t	arena_align(size_t nbytes)
{
	size_t	mask;

	mask = sizeof(void *) - 1;
	return ((nbytes + mask) & ~mask);
}

size_t	min_size(size_t nbytes)
{
	return (arena_align(nbytes));
}

char	*make_str_space(size_t newlen, char *p)
{
	size_t		used;
	t_garena	*g;

	g = arena_ctx();
	used = (size_t)(p - g->stack_next);
	return (grow_stack_to(used + newlen) + used);
}

size_t	compute_new_len(size_t min, t_garena *g)
{
	size_t	newlen;

	newlen = g->stack_nleft * 2;
	if (newlen < g->stack_nleft)
		out_of_space();
	min = arena_align(min | 128U);
	if (newlen < min)
		newlen += min;
	return (newlen);
}

int	st_topc(char *p)
{
	return ((unsigned char)p[-1]);
}
