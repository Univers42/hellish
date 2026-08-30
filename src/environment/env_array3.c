/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_array3.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "shell.h"

/* One [idx]="value" chunk of the display form. */
static void	fmt_rec(t_string *out, long idx, const char *v, int vl)
{
	char	*n;

	if (out->len > 1)
		vec_push_char(out, ' ');
	vec_push_char(out, '[');
	n = ft_itoa((int)idx);
	if (n)
		vec_push_str(out, n);
	xfree(n);
	vec_push_str(out, "]=\"");
	if (v && vl > 0)
		vec_push_dquoted(out, v, vl);
	vec_push_char(out, '"');
}

/* Human form for `set`/export listings: ([0]="a" [3]="b") — the bash
   display, so the private encoding never reaches the user's eyes. */
char	*arr_format(const char *val)
{
	t_string	out;
	const char	*cur;
	const char	*v;
	long		idx;
	int			vl;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_char(&out, '(');
	cur = val + 1;
	while (arr_next(&cur, &idx, &v, &vl))
		fmt_rec(&out, idx, v, vl);
	vec_push_char(&out, ')');
	vec_push_char(&out, '\0');
	return ((char *)out.ctx);
}

/* Elements `r.lo`..`r.hi` (0-based, inclusive) joined with `sep`, 0 for no
** separator.  Backs both "${arr[*]}" (the whole array, via arr_join) and
** zsh's "${arr[lo,hi]}".
**
** Iteration is by POSITION, not by stored index: the store is sparse, so
** `a=(x); a[9]=y` holds records 0 and 9, and zsh's `a[1,2]` means the first
** two elements that EXIST, not indices 1 and 2.  Counting as we walk is the
** only reading that agrees with ${#a} and with the negative subscripts that
** are resolved against it.
**
** An empty range (hi < lo) yields "" rather than the whole array, which is
** what makes `${a[3,2]}` empty instead of everything.
*/
static void	join_walk(t_string *out, const char *val, char sep, t_slice r)
{
	const char	*cur;
	const char	*v;
	long		idx;
	long		pos;
	int			vl;

	cur = "";
	if (arr_is(val))
		cur = val + 1;
	pos = 0;
	while (arr_next(&cur, &idx, &v, &vl))
	{
		if (pos >= r.lo && pos <= r.hi)
		{
			if (out->len && sep)
				vec_push_char(out, sep);
			vec_push_nstr(out, (char *)v, vl);
		}
		pos++;
	}
}

char	*arr_join_range(const char *val, char sep, t_slice r)
{
	t_string	out;

	vec_init(&out);
	out.elem_size = 1;
	join_walk(&out, val, sep, r);
	vec_push_char(&out, '\0');
	return ((char *)out.ctx);
}

/* Encoded value minus element `idx` (unset arr[i]). An emptied array
   keeps its magic byte, so the variable remains an (empty) array —
   matching bash, where unset a[0] on a one-element array leaves `a`
   set-but-empty. */
char	*arr_without(const char *old, long idx)
{
	t_string	out;
	const char	*cur;
	const char	*v;
	long		ci;
	int			vl;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_char(&out, ARR_MAGIC);
	cur = "";
	if (arr_is(old))
		cur = old + 1;
	while (arr_next(&cur, &ci, &v, &vl))
	{
		if (ci != idx)
			rec_append(&out, ci, v, vl);
	}
	vec_push_char(&out, '\0');
	return ((char *)out.ctx);
}
