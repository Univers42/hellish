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

/* Split one expanded element into "[sub]=value": true with sub/subl/val
   filled when it starts '[' and has a "]=", false (bare value) otherwise.
   Quoting that would suppress the subscript is already lost by word
   expansion — a documented element-level v1 divergence. Shared with the
   assoc builder in expand_array_assign2.c; it lives here so both files
   stay at the norm's five functions. */
int	parse_sub_elem(char *elem, char **sub, int *subl, char **val)
{
	char	*rb;

	if (elem[0] != '[')
		return (0);
	rb = ft_strchr(elem, ']');
	if (!rb || rb[1] != '=')
		return (0);
	*sub = elem + 1;
	*subl = (int)(rb - (elem + 1));
	*val = rb + 2;
	return (1);
}

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

/* One compound element onto `cur`: an explicit [expr]= resets the
   running counter first, then the value lands there and it advances. */
static char	*idx_elem(t_shell *state, char *cur, long *run, char *val)
{
	char	*sub;
	int		subl;

	if (parse_sub_elem(val, &sub, &subl, &val))
		*run = sub_index(state, sub, subl);
	return (idx_set_free(cur, (*run)++, val));
}

char	*build_indexed_sub(t_shell *state, t_vec *args,
			const char *base, int append)
{
	char	*cur;
	long	run;
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
		cur = idx_elem(state, cur, &run, ((char **)args->ctx)[i]);
		i++;
	}
	return (cur);
}
