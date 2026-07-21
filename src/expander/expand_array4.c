/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_array4.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "arith.h"

bool	is_valid_ident(char *s, int len);

/* Small statics-and-arith helpers for the array slice/keys code. */

/* Decimal of a small non-negative index into a rotating static buffer
   (only ever used to build a transient joined string, one call at a time
   inside a push loop). */
char	*idx_str(long idx)
{
	static char	buf[24];

	snprintf(buf, sizeof(buf), "%ld", idx);
	return (buf);
}

/* Evaluate a slice offset/length arithmetic sub-expression to an int. */
int	arith_num(t_shell *state, const char *s, int len)
{
	char	*res;
	int		v;

	if (len <= 0)
		return (0);
	res = arith_expand(state, s, len);
	v = 0;
	if (res)
		v = ft_atoi(res);
	xfree(res);
	return (v);
}

/* Offset of the SECOND colon in "name[@]:off:len" (the one before len), or
   tt->len when there is no length field. `first` is the first colon (the
   one after the ]). */
int	slice_off_len(const char *s, int len, int first)
{
	int	i;

	i = first + 1;
	while (i < len && s[i] != ':')
		i++;
	return (i);
}

/* If `s` is NAME[@]:... or NAME[*]:..., return the name length and set
   *colon to the offset of the ':' after the ']'; else 0. */
int	slice_name_len(const char *s, int len, int *colon)
{
	int	i;

	i = 0;
	while (i < len && s[i] != '[')
		i++;
	if (i < 1 || i + 3 >= len || !is_valid_ident((char *)s, i))
		return (0);
	if ((s[i + 1] != '@' && s[i + 1] != '*') || s[i + 2] != ']'
		|| s[i + 3] != ':')
		return (0);
	*colon = i + 3;
	return (i);
}

/* Build the space-joined element slice from the resolved context. */
char	*arr_slice_build(t_slice_ctx *c)
{
	t_string	out;
	const char	*cur;
	const char	*v;
	long		idx;
	int			vl;
	int			k;

	vec_init(&out);
	out.elem_size = 1;
	k = 0;
	cur = "";
	if (arr_is(c->val))
		cur = c->val + 1;
	while (arr_next(&cur, &idx, &v, &vl))
	{
		if (k >= c->off && k < c->lim)
		{
			if (out.len)
				vec_push_char(&out, ' ');
			vec_push_nstr(&out, (char *)v, vl);
		}
		k++;
	}
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}
