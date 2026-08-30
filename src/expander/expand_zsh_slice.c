/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_slice.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 15:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "arith.h"

/* zsh's `a[lo,hi]` -- a RANGE of elements, not one.
**
** This is the shape that fails silently without it.  `lo,hi` is a perfectly
** good arithmetic expression: C's comma operator evaluates both sides and
** yields the right one, so `a[2,3]` reads element 3, one element comes back,
** and nothing anywhere reports a problem.  A plugin slicing an array gets a
** single element that looks exactly like data.
**
** bash has no slice and DOES mean the comma operator there -- bash's
** `a[2,3]` is `a[3]` -- so every entry point is gated on zsh_arrays(), the
** same bit that decides the counting base.  `setopt ksharrays` turns the
** whole zsh array dialect off, slices included; that is the option's job.
*/

/* Index of the comma that separates the two bounds, or -1 for none.
**
** Depth-aware, because a comma only separates at the top level: in
** `a[$((1,2))]` it is the arithmetic comma operator and zsh reads the single
** index 2.  Parentheses and brackets nest; a comma inside quotes is text. */
static int	slice_comma(const char *s, int len)
{
	int		i;
	int		depth;
	char	q;

	i = -1;
	depth = 0;
	q = 0;
	while (++i < len)
	{
		if (q && s[i] == q)
			q = 0;
		else if (!q && (s[i] == '\'' || s[i] == '"'))
			q = s[i];
		else if (!q && (s[i] == '(' || s[i] == '['))
			depth++;
		else if (!q && (s[i] == ')' || s[i] == ']'))
			depth--;
		else if (!q && depth == 0 && s[i] == ',')
			return (i);
	}
	return (-1);
}

/* One bound, as a 0-based store position.
**
** Two rules that look alike and are not, both measured against zsh 5.9:
**   a NEGATIVE subscript counts back from the end (-1 is the last), and one
**   that reaches past the START voids the whole range -- `a[-6,2]` on five
**   elements is EMPTY, not clamped to the first;
**   a subscript of 0 is clamped UP to the first element -- `a[0,2]` is the
**   first two.
** -6 and 0 both name position 0 here and answer differently, so a shared
** clamp gets one of them wrong. */
static long	slice_bound(t_shell *state, const char *t, int l, long count)
{
	char	*res;
	long	v;

	res = arith_expand(state, t, l);
	v = 0;
	if (res)
		v = ft_atol(res);
	xfree(res);
	if (v < 0)
		return (count + v);
	if (v > 0)
		return (v - 1);
	return (0);
}

/* Resolve `body` as a slice of `val`.  .lo == SLICE_NONE means it is not a
** slice at all and the caller should treat the subscript as an ordinary
** index.
**
** `hi` is deliberately NOT clamped to the container.  Reading skips
** positions that do not exist anyway, and the write path NEEDS the raw
** value: `a[9,10]=(P)` on five elements must pad out to nine, and a clamped
** hi would turn that into an insert followed by a stray empty. */
t_slice	zsh_slice_bounds(t_shell *state, const char *body, int blen,
			const char *val)
{
	t_slice	r;
	long	count;
	int		c;

	r.lo = SLICE_NONE;
	r.hi = 0;
	if (!zsh_arrays(state))
		return (r);
	c = slice_comma(body, blen);
	if (c < 0)
		return (r);
	count = zsh_slice_universe(val);
	r.lo = slice_bound(state, body, c, count);
	r.hi = slice_bound(state, body + c + 1, blen - c - 1, count);
	if (r.lo < 0 || r.hi < 0)
		r.hi = r.lo - 1;
	return (r);
}
