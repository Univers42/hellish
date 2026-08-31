/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_array.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "shell.h"

/* Read-side helpers for the encoded array values (see env.h for the
   format). Everything walks the record string directly — no parsed
   representation exists anywhere, which is what keeps arrays out of the
   env lifecycle entirely. */

/* Is this env value an encoded array? */
bool	arr_is(const char *val)
{
	return (val != NULL && val[0] == ARR_MAGIC);
}

/* Record iterator: *cur starts at val+1 and advances one record per call.
   Yields the index, a pointer to the value bytes and their length.
   Returns false at end of string. */
bool	arr_next(const char **cur, long *idx, const char **v, int *vl)
{
	const char	*p;

	p = *cur;
	if (!p || !*p)
		return (false);
	*idx = 0;
	while (*p >= '0' && *p <= '9')
		*idx = *idx * 10 + (*p++ - '0');
	if (*p == ARR_US)
		p++;
	*v = p;
	*vl = 0;
	while (p[*vl] && p[*vl] != ARR_RS)
		(*vl)++;
	p += *vl;
	if (*p == ARR_RS)
		p++;
	*cur = p;
	return (true);
}

/* Number of set elements (sparse-aware: ${#arr[@]} counts records). */
int	arr_count(const char *val)
{
	const char	*cur;
	const char	*v;
	long		idx;
	int			vl;
	int			n;

	if (!arr_is(val))
		return (0);
	cur = val + 1;
	n = 0;
	while (arr_next(&cur, &idx, &v, &vl))
		n++;
	return (n);
}

/* Heap copy of element `want`, or NULL when unset. */
char	*arr_get_idx(const char *val, long want)
{
	const char	*cur;
	const char	*v;
	long		idx;
	int			vl;

	if (!arr_is(val))
		return (NULL);
	cur = val + 1;
	while (arr_next(&cur, &idx, &v, &vl))
	{
		if (idx == want)
			return (ft_strndup(v, vl));
	}
	return (NULL);
}

/* Heap string of all elements joined with `sep` (0 = no separator):
   the "${arr[*]}" and no-split "${arr[@]}" expansions.  The whole array is
   just the widest possible range, so this defers to arr_join_range and
   there is ONE join loop rather than two that can drift apart. */
char	*arr_join(const char *val, char sep)
{
	t_slice	all;

	all.lo = 0;
	all.hi = LONG_MAX;
	return (arr_join_range(val, sep, all));
}
