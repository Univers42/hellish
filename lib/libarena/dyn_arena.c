/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dyn_arena.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 16:45:17 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/06 20:25:46 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arena.h"
#include <string.h>

static void	init_ctx(t_garena *g)
{
	g->stackp = &g->stack_base;
	g->stack_next = g->stack_base.space;
	g->stack_nleft = min_size(ARENA_MINSIZE);
	g->sstrend = g->stack_base.space + g->stack_nleft;
}

t_garena	*arena_ctx(void)
{
	static t_garena	g;
	static int		init = 0;

	if (!init)
	{
		init_ctx(&g);
		init = 1;
	}
	return (&g);
}

void	arena_cleanup(void)
{
	t_garena		*g;
	t_stack_block	*sp;
	t_stack_block	*prev;

	g = arena_ctx();
	sp = g->stackp;
	while (sp != &g->stack_base)
	{
		prev = sp->prev;
		free(sp);
		sp = prev;
	}
	init_ctx(g);
}

void	*st_alloc(size_t nbytes)
{
	t_garena		*g;
	t_dynall		b;
	t_stack_block	*sp;
	char			*p;
	const size_t	minsize = min_size(ARENA_MINSIZE);

	g = arena_ctx();
	b.aligned = arena_align(nbytes);
	if (b.aligned > g->stack_nleft)
	{
		b.block_size = b.aligned;
		if (b.block_size < minsize)
			b.block_size = minsize;
		b.len = sizeof(t_stack_block) - minsize + b.block_size;
		sp = (t_stack_block *)arena_malloc(b.len);
		sp->prev = g->stackp;
		g->stackp = sp;
		g->stack_next = sp->space;
		g->stack_nleft = b.block_size;
		g->sstrend = g->stack_next + b.block_size;
	}
	p = g->stack_next;
	g->stack_next += b.aligned;
	g->stack_nleft -= b.aligned;
	return (p);
}

void	st_unalloc(void *p)
{
	t_garena	*g;

	if (!p)
		return ;
	g = arena_ctx();
	if ((char *)p > g->stack_next || (char *)p < g->stackp->space)
		return ;
	g->stack_nleft += (size_t)(g->stack_next - (char *)p);
	g->stack_next = (char *)p;
}
