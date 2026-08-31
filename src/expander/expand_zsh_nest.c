/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_nest.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 18:50:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 18:50:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* Nested flags: `${(j:-:)${(f)x}}`.
**
** Flags compose in zsh because the inner expansion hands the outer one an
** ARRAY, not a string.  Run through the ordinary scalar path the inner
** `${(f)x}` would first join its fields with a space and the outer (j) would
** then have a single element to join -- giving `a b` where zsh gives `a-b`.
** Wrong, and quietly so: the answer still looks like a list.
**
** So a `${(...)...}` operand is evaluated in LIST mode and handed back
** encoded as an array value.  zl_from already unpacks an array operand into
** its elements, which is the same protocol a plain `${(j:,:)$arr}` uses, so
** nesting needs no special case anywhere downstream -- only a way to produce
** the encoded value, which is this file.
*/

/* Is the brace opened at index 1 of `s` closed exactly at slen-1?  Without
   this, `${(f)a}${(f)b}` -- one operand, two expansions -- would be taken
   for a single nested one and mis-parsed from the middle. */
static bool	spans_whole(const char *s, int slen)
{
	int	depth;
	int	i;

	depth = 0;
	i = 1;
	while (i < slen)
	{
		if (s[i] == '{')
			depth++;
		else if (s[i] == '}' && --depth == 0)
			return (i == slen - 1);
		i++;
	}
	return (false);
}

/* True for an operand that is exactly one flagged expansion, optionally with
   a trailing [@] or [*].  That subscript is how zsh code forces array-ness
   back on inside double quotes -- `"${(o)${(f)x}[@]}"` sorts where
   `"${(o)${(f)x}}"` does not -- so treating it as literal text would leave a
   sorted list unsorted and print the `[@]` as part of the value, which is
   both wrong and silent. */
bool	zf_is_nested(const char *s, int slen)
{
	slen -= zn_sub_len(s, slen);
	return (slen > 4 && s[0] == '$' && s[1] == '{' && s[2] == '('
		&& spans_whole(s, slen));
}

/* 3 when the operand ends in [@] or [*], else 0. */
int	zn_at_len(const char *s, int slen)
{
	if (slen > 3 && s[slen - 1] == ']' && s[slen - 3] == '['
		&& (s[slen - 2] == '@' || s[slen - 2] == '*'))
		return (3);
	return (0);
}

/* Hand the inner result up: an ARRAY as an encoded value the outer flag set
   unpacks into elements, a SCALAR as a plain space-joined string.
**
** An inner expansion stays an array if the enclosing quoting allows one, OR
** if its own flags MAKE one -- (f), (s) and (z) split, so their result is a
** list whatever the context.  Anything else in a quoted context joins, and
** the level above it sees a single field.  Watch it decide three nested
** flags in `"${(j:,:)${(ou)${(f)x}}}"` on four lines c a c b:
**
**   (f)     splits              -> array, because f splits
**   (ou)    quoted, no split    -> joins with a space, ONE field
**   (j:,:)  one field to join   -> "c a c b"
**
** So the sort, the de-duplication and the comma all do nothing, and zsh 5.9
** agrees.  Every plausible simplification of this rule -- always pack an
** array, never pack one, follow only the quoting -- gets at least one of
** those three levels wrong, and gets it wrong by producing a tidier answer
** than the truth.
*/
static char	*zn_pack(t_zflags *f, t_vec *l)
{
	if (f->array || zf_has(f, 'f') || zf_has(f, 's') || zf_has(f, 'z'))
		return (arr_from_elems((char **)l->ctx, (int)l->len, NULL));
	return (zl_join(l, " "));
}

/* Evaluate one nested `${(flags)body}` to an encoded array value, or NULL if
   its flag list is malformed -- in which case the caller falls back to the
   scalar path and the error surfaces there, once, rather than twice. */
char	*zf_nested(t_shell *state, t_token *tt, const char *s, int slen)
{
	t_zflags	f;
	int			end;
	char		*enc;
	t_vec		l;

	f = (t_zflags){0};
	f.array = (zn_at_len(s, slen) != 0) || tt->tt != TT_DQENVVAR;
	slen -= zn_sub_len(s, slen) + 3;
	s += 2;
	end = zf_parse(s, slen, &f);
	if (end < 0 || !zf_check(state, &f, tt))
		return (zf_free(&f), NULL);
	f.array = f.array || zf_arrayness(&f, tt, s + end, slen - end);
	zf_unesc(&f);
	enc = zf_inner(state, tt, s + end, slen - end);
	l = zl_from(state, &f, enc);
	xfree(enc);
	zl_order(&f, &l);
	zl_map(state, &f, &l);
	zf_free(&f);
	enc = zn_pack(&f, &l);
	return (zl_free(&l), enc);
}
