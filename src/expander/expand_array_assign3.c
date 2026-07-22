/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_array_assign3.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "arith.h"

/* Indexed compound-init that mixes explicit and implicit subscripts:
   a=([2]=x foo [7]=y bar). An explicit [expr]= sets that arithmetic
   index and the running counter jumps to expr+1; a bare value fills the
   current counter and bumps it — exactly bash's rule. Built incrementally
   through arr_with_set so ordering and replacement stay correct. */

/* Arithmetic value of a subscript slice (already word-expanded, so only
   arithmetic remains: [1+1] -> 2, [$k] came through as its value). */
static long	sub_index(t_shell *state, const char *sub, int subl)
{
	char	*tmp;
	char	*res;
	long	idx;

	tmp = ft_strndup(sub, subl);
	res = arith_expand(state, tmp, subl);
	idx = 0;
	if (res)
		idx = ft_atoi(res);
	return (xfree(tmp), xfree(res), idx);
}

/* arr_with_set on `cur`, freeing the old buffer; returns the new one. */
static char	*idx_set_free(char *cur, long idx, const char *val)
{
	char	*nv;

	nv = arr_with_set(cur, idx, val);
	xfree(cur);
	return (nv);
}

char	*build_indexed_sub(t_shell *state, t_vec *args,
			const char *base, int append)
{
	char	*cur;
	char	*sub;
	char	*val;
	long	run;
	int		subl;
	size_t	i;

	cur = ft_strdup((char [2]){ARR_MAGIC});
	run = 0;
	if (append && arr_is(base))
	{
		xfree(cur);
		cur = ft_strdup(base);
		run = arr_max_idx(base) + 1;
	}
	i = 0;
	while (i < args->len)
	{
		val = ((char **)args->ctx)[i++];
		if (parse_sub_elem(val, &sub, &subl, &val))
			run = sub_index(state, sub, subl);
		cur = idx_set_free(cur, run++, val);
	}
	return (cur);
}
