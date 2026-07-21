/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_pos_slice.c                                 :+:      :+:    :+:   */
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

/* ${@:off:len} / ${*:off:len} / ${@:off}: positional-parameter slices.
   off is 1-based over $1..$N but the slice conventionally COUNTS $0 as
   index 0, so ${@:0} includes $0 and ${@:1} starts at $1 — bash
   semantics. A negative off counts from the end (of $1..$N plus one).
   Materialised space-joined (echo/for parity); per-element field
   structure is the same v1 scope-out as array slices. */

/* Space-join positional params [from, to) (1-based, from>=0 with 0=$0). */
static char	*pos_join(t_shell *state, int from, int to)
{
	t_string	out;
	char		*k;
	char		*v;
	int			i;

	vec_init(&out);
	out.elem_size = 1;
	i = from;
	while (i < to)
	{
		k = ft_itoa(i);
		v = env_expand(state, k);
		xfree(k);
		if (v)
		{
			if (out.len)
				vec_push_char(&out, ' ');
			vec_push_str(&out, v);
		}
		i++;
	}
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}

/* Recognise "@:..." / "*:..." and emit the slice. tt->start[0] is @ or *,
   [1] is ':'. Returns false otherwise. */
bool	expand_pos_slice(t_shell *state, t_token *tt)
{
	char	*cnt;
	int		n;
	int		off;
	int		to;

	if (tt->len < 3 || (tt->start[0] != '@' && tt->start[0] != '*')
		|| tt->start[1] != ':')
		return (false);
	if (tt->start[2] == '-' || tt->start[2] == '+'
		|| tt->start[2] == '=' || tt->start[2] == '?')
		return (false);
	cnt = env_expand(state, "#");
	n = 0;
	if (cnt)
		n = ft_atoi(cnt);
	off = arith_num(state, tt->start + 2, slice_off_len(tt->start, tt->len, 1)
			- 2);
	if (off < 0)
		off += n + 1;
	to = n + 1;
	if (slice_off_len(tt->start, tt->len, 1) < tt->len)
		to = off + arith_num(state,
				tt->start + slice_off_len(tt->start, tt->len, 1) + 1,
				tt->len - slice_off_len(tt->start, tt->len, 1) - 1);
	tt->start = pos_join(state, off, to);
	tt->len = (int)ft_strlen(tt->start);
	tt->allocated = true;
	return (parena_note_attach(), true);
}
