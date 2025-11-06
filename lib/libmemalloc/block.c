/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   block.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 17:48:01 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/06 20:07:55 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arena.h"
#include <string.h>

static char	*expand_top_block(size_t newlen, t_garena *g)
{
	t_stack_block	*sp;
	t_stack_block	*prev;
	size_t			grosslen;
	char			*p;

	sp = g->stackp;
	prev = sp->prev;
	grosslen = newlen + sizeof(t_stack_block) - ARENA_MINSIZE;
	sp = (t_stack_block *)arena_realloc(sp, grosslen);
	sp->prev = prev;
	g->stackp = sp;
	p = sp->space;
	g->stack_next = p;
	g->stack_nleft = newlen;
	g->sstrend = p + newlen;
	return (p);
}

static char	*allocate_and_move(size_t newlen, t_garena *g)
{
	char	*oldspace;
	size_t	oldlen;
	char	*p;

	oldspace = g->stack_next;
	oldlen = g->stack_nleft;
	p = (char *)st_alloc(newlen);
	g->stack_next = (char *)memcpy(p, oldspace, oldlen);
	g->stack_nleft += newlen;
	return (p);
}

char	*grow_stack_block(size_t min)
{
	size_t		newlen;
	t_garena	*g;

	g = arena_ctx();
	newlen = compute_new_len(min, g);
	if (g->stack_next == g->stackp->space && g->stackp != &g->stack_base)
		return (expand_top_block(newlen, g));
	return (allocate_and_move(newlen, g));
}

void	*grow_stack_str(void)
{
	size_t	len;

	len = stack_block_size();
	return ((void *)(grow_stack_block(0) + len));
}

char	*grow_stack_to(size_t len)
{
	if (stack_block_size() < len)
		return (grow_stack_block(len));
	return (stack_block());
}
