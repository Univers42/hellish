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
		vec_push_nstr(out, (char *)v, vl);
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
