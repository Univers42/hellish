/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_array3.c                                    :+:      :+:    :+:   */
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
#include "parena.h"

bool	is_valid_ident(char *s, int len);

/* Array slice ${a[@]:off:len} / ${a[@]:off} and index list ${!a[@]}.
   These are the two [@] forms the strict NAME[...] parser rejects (its
   `]` must be the last byte). Both materialise their result EAGERLY as a
   space-joined string: for `echo`, `for`, and the vast majority of real
   uses this is byte-identical to bash. (Per-element field structure of a
   slice is a documented v1 scope-out.) */

/* Store an owned result string into the token. */
static bool	arr_emit(t_token *tt, char *owned)
{
	tt->start = owned;
	tt->len = (int)ft_strlen(owned);
	tt->allocated = true;
	return (parena_note_attach(), true);
}

/* ${!name[@]} / ${!name[*]}: space-joined index list of the array (or
   "0" for a set scalar, nothing for unset — bash semantics). */
bool	arr_keys(t_shell *state, t_token *tt, bool split_ctx)
{
	t_string	out;
	char		*val;
	const char	*cur;
	const char	*v;
	long		idx;
	int			vl;

	if (split_ctx)
		return (arr_keys_defer(state, tt));
	val = env_expand_n(state, tt->start + 1, tt->len - 4);
	vec_init(&out);
	out.elem_size = 1;
	if (assoc_is(val))
	{
		vec_push_char(&out, '\0');
		xfree(out.ctx);
		return (arr_emit(tt, assoc_keys(val)));
	}
	if (val && !arr_is(val))
		vec_push_str(&out, "0");
	else if (val)
	{
		cur = val + 1;
		while (arr_next(&cur, &idx, &v, &vl))
		{
			if (out.len)
				vec_push_char(&out, ' ');
			vec_push_str(&out, (char *)(idx_str(idx)));
		}
	}
	vec_push_char(&out, '\0');
	return (arr_emit(tt, (char *)out.ctx));
}

/* ${name[@]:off:len}: collect elements [off, off+len) (off<0 counts from
   the end, len<0 or absent means to the end) into a space-joined string. */
bool	arr_slice(t_shell *state, t_token *tt, int nl, int colon)
{
	t_slice_ctx	c;

	c.val = env_expand_n(state, tt->start, nl);
	c.n = arr_count(c.val);
	c.off = (int)arith_num(state, tt->start + colon + 1,
			slice_off_len(tt->start, tt->len, colon) - colon - 1);
	if (c.off < 0)
		c.off += c.n;
	c.lim = c.n;
	if (slice_off_len(tt->start, tt->len, colon) < tt->len)
		c.lim = c.off + (int)arith_num(state,
				tt->start + slice_off_len(tt->start, tt->len, colon) + 1,
				tt->len - slice_off_len(tt->start, tt->len, colon) - 1);
	return (arr_emit(tt, arr_slice_build(&c)));
}

/* Recognise the two extended [@] forms and dispatch; return false to let
   the normal NAME[...] path handle everything else. */
bool	expand_array_ext(t_shell *state, t_token *tt, bool split_ctx)
{
	int	nl;
	int	colon;

	if (tt->tt != TT_ENVVAR && tt->tt != TT_DQENVVAR)
		return (false);
	if (tt->len >= 5 && tt->start[0] == '!'
		&& tt->start[tt->len - 1] == ']'
		&& (tt->start[tt->len - 2] == '@' || tt->start[tt->len - 2] == '*')
		&& tt->start[tt->len - 3] == '[')
		return (arr_keys(state, tt, split_ctx));
	nl = slice_name_len(tt->start, tt->len, &colon);
	if (nl > 0)
		return (arr_slice(state, tt, nl, colon));
	return (false);
}
