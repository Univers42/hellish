/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_make_rank2.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "complete_make.h"
#include <string.h>
#include <stdlib.h>

/* Fill st->make_cache.order with the cached targets in the order
   `make <TAB>` offers them (complete_make_rank.c): used first, by
   frecency, then the rest alphabetically. */

/* One record per target, sorted by name so that words can be looked up
   by binary search. Returns NULL for no targets or no memory. */
static t_mk_use	*mk_records(t_vec *targets)
{
	t_mk_use	*u;
	size_t		i;

	if (!targets->len)
		return (NULL);
	u = xmalloc(targets->len * sizeof(*u));
	i = 0;
	while (i < targets->len)
	{
		u[i] = (t_mk_use){((char **)targets->ctx)[i], 0, 0, 0};
		i++;
	}
	qsort(u, targets->len, sizeof(*u), mk_cmp_name);
	return (u);
}

/* Walk the window of the newest HELLISH_SUGGEST_MAX history entries, the
   same one the inline suggestion looks at, crediting make invocations.
   Cost: O(window * words * log targets) per TAB. */
static void	mk_credit(t_shell *st, t_mk_rank *r)
{
	char	**a;
	size_t	i;

	a = (char **)st->hist.hist_cmds.ctx;
	i = hist_window_floor(r->len, st->rl.sg_max);
	while (i < r->len)
	{
		if (a[i] && mk_is_make_entry(a[i]))
			mk_count_entry(r, a[i], i);
		i++;
	}
}

/* The ranked order, as borrowed pointers into the target list. It is
   rebuilt on every TAB: the history moves on, the targets rarely do. */
void	mk_order(t_shell *st)
{
	t_mk_rank	r;
	size_t		i;

	st->make_cache.order.len = 0;
	st->make_cache.order.elem_size = sizeof(const char *);
	r.cnt = st->make_cache.targets.len;
	r.u = mk_records(&st->make_cache.targets);
	r.len = st->hist.hist_cmds.len;
	r.recent = st->rl.sg_recent;
	if (!r.u)
		return ;
	mk_credit(st, &r);
	qsort(r.u, r.cnt, sizeof(*r.u), mk_cmp_use);
	i = 0;
	while (i < r.cnt)
		vec_push(&st->make_cache.order, &r.u[i++].name);
	xfree(r.u);
}
