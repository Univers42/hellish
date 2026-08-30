/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_complete2.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Declared here rather than in the private header: the 42 norm aligns
   every declaration in a file to one column, and `t_compspec` is too wide
   to reach the one that header already uses. */
t_compspec	*comp_find(t_shell *st, const char *name);

/* Storage, printing and removal for `complete`. */

void	comp_free_spec(t_compspec *c)
{
	xfree(c->name);
	xfree(c->words);
	xfree(c->func);
	xfree(c->opts);
	c->name = NULL;
	c->words = NULL;
	c->func = NULL;
	c->opts = NULL;
}

void	comp_vec_push(t_shell *st, t_compspec *c)
{
	if (st->compspecs.elem_size == 0)
	{
		vec_init(&st->compspecs);
		st->compspecs.elem_size = sizeof(t_compspec);
	}
	vec_push(&st->compspecs, c);
}

/* Drop one spec by moving the LAST entry into its slot. The order of the
   two steps matters and ASan proved it: shrinking first and then indexing
   the new length walks off the end whenever the removed spec was the last
   one -- which is every `complete -r` on a shell holding a single spec,
   i.e. the common case. Read the tail, then shrink. */
static void	comp_drop(t_shell *st, t_compspec *c)
{
	t_compspec	*last;

	comp_free_spec(c);
	last = (t_compspec *)vec_idx(&st->compspecs, st->compspecs.len - 1);
	if (last != c)
		*c = *last;
	st->compspecs.len--;
}

/* `complete -r [NAME…]`: drop one spec or, with no name, all of them.
   Removing an absent name is silent and succeeds, matching bash -- an rc
   that clears a spec it may never have set must not fail. */
int	comp_remove(t_shell *st, t_vec argv, size_t i)
{
	t_compspec	*c;
	size_t		n;

	if (i >= argv.len)
	{
		n = 0;
		while (n < st->compspecs.len)
			comp_free_spec((t_compspec *)vec_idx(&st->compspecs, n++));
		st->compspecs.len = 0;
		return (0);
	}
	while (i < argv.len)
	{
		c = comp_find(st, ((char **)argv.ctx)[i++]);
		if (c)
			comp_drop(st, c);
	}
	return (0);
}
