/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   stack.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 19:34:28 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/06 20:38:58 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arena.h"

void	pop_stack_mark(t_stack_mark *mark)
{
	t_garena		*g;
	t_stack_block	*sp;

	if (!mark)
		return ;
	g = arena_ctx();
	while (g->stackp != mark->stackp)
	{
		sp = g->stackp;
		g->stackp = sp->prev;
		arena_free(sp);
	}
	g->stack_next = mark->stacknxt;
	g->stack_nleft = mark->stacknleft;
	g->sstrend = mark->stacknxt + mark->stacknleft;
}

void	push_stack_mark(t_stack_mark *mark, size_t len)
{
	t_garena	*g;

	if (!mark)
		return ;
	g = arena_ctx();
	mark->stackp = g->stackp;
	mark->stacknxt = g->stack_next;
	mark->stacknleft = g->stack_nleft;
	grab_stack_block(len);
}

void	set_stack_mark(t_stack_mark *mark)
{
	t_garena	*g;
	size_t		len;

	if (!mark)
		return ;
	g = arena_ctx();
	len = (g->stack_next == g->stackp->space && g->stackp != &g->stack_base);
	push_stack_mark(mark, len);
}
