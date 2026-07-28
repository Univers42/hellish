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

/* Join all positional parameters into a single string separated by the first
   character of IFS (space by default).  This is the $* / unquoted $@
   expansion: POSIX says "$@" in a split context should produce one field per
   positional — that case is handled separately in emit_positional_at.
   env_get_ifs never returns NULL (unset falls back to " \t\n"), so the
   separator test must look at the first CHARACTER: with IFS='' there is no
   separator at all and "$*" concatenates ("ab", not "a<NUL>b").
   Note: getting $# as a string to do ft_atoi on it is slightly clunky but
   avoids adding a separate counter field to t_shell. */
char	*join_positionals(t_shell *state)
{
	t_string	out;
	char		*cnt;
	char		*k;
	char		*v;
	int			i;

	cnt = env_expand(state, "#");
	i = 0;
	vec_init(&out);
	out.elem_size = 1;
	while (cnt && ++i <= ft_atoi(cnt))
	{
		k = ft_itoa(i);
		v = env_expand(state, k);
		xfree(k);
		if (i > 1 && env_get_ifs(&state->env)[0])
			vec_push(&out, &env_get_ifs(&state->env)[0]);
		if (v)
			vec_push_str(&out, v);
	}
	return (vec_push(&out, &(char){0}), (char *)out.ctx);
}

/* Compute the [*off, *to) window of an "@:off[:len]" body.  slice_off_len
   (pure — it just finds the second ':', or tt->len when there is no length
   field) is cached in `ol`; a negative offset counts back from $# plus one
   so the $0 slot stays addressable, exactly as the inline code did. */
static void	pos_slice_range(t_shell *state, t_token *tt, int *off, int *to)
{
	char	*cnt;
	int		n;
	int		ol;

	cnt = env_expand(state, "#");
	n = 0;
	if (cnt)
		n = ft_atoi(cnt);
	ol = slice_off_len(tt->start, tt->len, 1);
	*off = arith_num(state, tt->start + 2, ol - 2);
	if (*off < 0)
		*off += n + 1;
	*to = n + 1;
	if (ol < tt->len)
		*to = *off + arith_num(state, tt->start + ol + 1, tt->len - ol - 1);
}

/* Recognise "@:..." / "*:..." and emit the slice. tt->start[0] is @ or *,
   [1] is ':'. Returns false otherwise. */
bool	expand_pos_slice(t_shell *state, t_token *tt)
{
	int	off;
	int	to;

	if (tt->len < 3 || (tt->start[0] != '@' && tt->start[0] != '*')
		|| tt->start[1] != ':')
		return (false);
	if (tt->start[2] == '-' || tt->start[2] == '+'
		|| tt->start[2] == '=' || tt->start[2] == '?')
		return (false);
	pos_slice_range(state, tt, &off, &to);
	tt->start = pos_join(state, off, to);
	tt->len = (int)ft_strlen(tt->start);
	tt->allocated = true;
	return (parena_note_attach(), true);
}
