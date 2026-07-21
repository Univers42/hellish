/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_array2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "shell.h"

/* Write-side helpers: every mutation builds a fresh encoded value string
   (records stay sorted by index); the old value is left to the normal
   env_set replace-and-free path. */

/* Append one "idx<US>value" record to the buffer under construction. */
static void	rec_append(t_string *out, long idx, const char *v, int vl)
{
	char	*n;

	if (out->len > 1)
		vec_push_char(out, ARR_RS);
	n = ft_itoa((int)idx);
	if (n)
		vec_push_str(out, n);
	xfree(n);
	vec_push_char(out, ARR_US);
	if (v && vl > 0)
		vec_push_nstr(out, (char *)v, vl);
}

/* Copy `old`'s records into `out` with element `idx` set to `nv`,
   preserving index order: the new record is inserted before the first
   record with a greater index, and an existing record for `idx` is
   replaced rather than duplicated. */
static void	set_records(t_string *out, const char *old, long idx,
				const char *nv)
{
	const char	*cur;
	const char	*v;
	long		ci;
	int			vl;
	bool		done;

	cur = "";
	if (arr_is(old))
		cur = old + 1;
	done = false;
	while (arr_next(&cur, &ci, &v, &vl))
	{
		if (!done && ci >= idx)
		{
			rec_append(out, idx, nv, (int)ft_strlen(nv));
			done = true;
		}
		if (ci != idx)
			rec_append(out, ci, v, vl);
	}
	if (!done)
		rec_append(out, idx, nv, (int)ft_strlen(nv));
}

/* New encoded value = `old` with element `idx` set to `nv`. A scalar
   `old` is first promoted to a one-element array at index 0 — bash's
   x=hi; x[1]=b behaviour. */
char	*arr_with_set(const char *old, long idx, const char *nv)
{
	t_string	out;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_char(&out, ARR_MAGIC);
	if (old && !arr_is(old) && *old && idx != 0)
		rec_append(&out, 0, old, (int)ft_strlen(old));
	set_records(&out, old, idx, nv);
	vec_push_char(&out, '\0');
	return ((char *)out.ctx);
}

/* Highest index present, -1 for empty/scalar. */
static long	arr_max_idx(const char *val)
{
	const char	*cur;
	const char	*v;
	long		idx;
	long		max;
	int			vl;

	max = -1;
	if (!arr_is(val))
		return (max);
	cur = val + 1;
	while (arr_next(&cur, &idx, &v, &vl))
	{
		if (idx > max)
			max = idx;
	}
	return (max);
}

/* Build an encoded value from `n` element strings. With `base` NULL this
   is arr=(...): indices 0..n-1. With `base` an existing array this is
   arr+=(...): base's records are kept and the new elements continue
   after its highest index. */
char	*arr_from_elems(char **elems, int n, const char *base)
{
	t_string	out;
	long		next;
	int			i;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_char(&out, ARR_MAGIC);
	next = 0;
	if (base && arr_is(base))
	{
		vec_push_str(&out, (char *)base + 1);
		next = arr_max_idx(base) + 1;
	}
	i = 0;
	while (i < n)
	{
		rec_append(&out, next + i, elems[i], (int)ft_strlen(elems[i]));
		i++;
	}
	vec_push_char(&out, '\0');
	return ((char *)out.ctx);
}
