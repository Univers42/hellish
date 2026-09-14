/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   case_match_ext2.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"
#include "case_match.h"

/* The five extglob operators, as three shapes.  Every one of them asks the
** same question -- "can some prefix of the subject be consumed by the
** alternatives, leaving a remainder the rest of the pattern accepts" -- and
** they differ only in how many times, so they share one prefix loop.
**
** The subject is split at every position rather than parsed greedily: an
** alternative is a full pattern and may itself contain `*`, so there is no
** single "longest match" to commit to.  That is the same reason cm_run
** backtracks over `*`.
**
** `op` points at the group's '(' and `tail` just past its ')', so the
** alternatives are the slice [op + 1, tail - 1) and the rest of the pattern
** is [tail, m.pe) -- three cursors into one string, no copies.
*/

/* Zero or more repeats, then the tail. Trying the tail FIRST is what makes
   zero repeats legal, and the cut > 0 floor is what stops an alternative
   that matches the empty string from recursing forever. */
static bool	xg_rep(t_cmp m, const char *op, const char *tail)
{
	t_cmp	t;
	size_t	cut;

	t = m;
	t.p = tail;
	if (cm_run(t))
		return (true);
	cut = 0;
	while (++cut <= (size_t)(m.se - m.s))
	{
		t = m;
		t.p = op + 1;
		t.pe = tail - 1;
		if (!xg_any_alt(t, cut))
			continue ;
		t = m;
		t.s = m.s + cut;
		if (xg_rep(t, op, tail))
			return (true);
	}
	return (false);
}

/* `+(p)`: one repeat, then as many more as `*` would take. */
static bool	xg_plus(t_cmp m, const char *op, const char *tail)
{
	t_cmp	t;
	size_t	cut;

	cut = 0;
	while (++cut <= (size_t)(m.se - m.s))
	{
		t = m;
		t.p = op + 1;
		t.pe = tail - 1;
		if (!xg_any_alt(t, cut))
			continue ;
		t = m;
		t.s = m.s + cut;
		if (xg_rep(t, op, tail))
			return (true);
	}
	return (false);
}

/* The single-shot operators: `@` exactly one, `?` zero or one, `!` a prefix
   that matches NONE of the alternatives.  `!` is not "the opposite of @":
   it still has to leave a remainder the tail accepts, which is why it runs
   the same loop with the alternative test inverted rather than negating the
   whole answer. */
static bool	xg_once(t_cmp m, const char *op, const char *tail)
{
	t_cmp	t;
	size_t	cut;
	bool	hit;

	t = m;
	t.p = tail;
	if (*m.p == '?' && cm_run(t))
		return (true);
	cut = -1;
	while (++cut <= (size_t)(m.se - m.s))
	{
		t = m;
		t.p = op + 1;
		t.pe = tail - 1;
		hit = xg_any_alt(t, cut);
		t = m;
		t.s = m.s + cut;
		t.p = tail;
		if (hit == (*m.p != '!') && cm_run(t))
			return (true);
	}
	return (false);
}

/* Match the subject against a pattern that BEGINS with a group at m.p.
   The whole remaining pattern is handled here -- the group and everything
   after it -- so cm_run can hand off and return the answer directly.
     zsh's bare `(a|b)` carries no operator, so it falls through to xg_once
   and is read as `@` -- exactly one alternative, which is what zsh means by
   it. Nothing else in this file had to learn the second spelling: xg_open
   is the only place that knows where the paren is. */
bool	xg_match(t_cmp m)
{
	const char	*op;
	const char	*tail;

	op = xg_open(m.p);
	tail = xg_group_end(op, m.pe);
	if (!tail)
		return (false);
	if (*m.p == '*')
		return (xg_rep(m, op, tail));
	if (*m.p == '+')
		return (xg_plus(m, op, tail));
	return (xg_once(m, op, tail));
}
