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
** single "longest match" to commit to.  That is the same reason case_match
** backtracks over `*`.
*/

/* Zero or more repeats, then the tail. Trying the tail FIRST is what makes
   zero repeats legal, and the cut > 0 floor is what stops an alternative
   that matches the empty string from recursing forever. */
static bool	xg_rep(const char *s, const char *p, const char *tail)
{
	size_t	cut;
	size_t	n;

	if (case_match(s, tail))
		return (true);
	n = ft_strlen(s);
	cut = 1;
	while (cut <= n)
	{
		if (xg_any_alt(s, cut, p + 2) && xg_rep(s + cut, p, tail))
			return (true);
		cut++;
	}
	return (false);
}

/* `+(p)`: one repeat, then as many more as `*` would take. */
static bool	xg_plus(const char *s, const char *p, const char *tail)
{
	size_t	cut;
	size_t	n;

	n = ft_strlen(s);
	cut = 1;
	while (cut <= n)
	{
		if (xg_any_alt(s, cut, p + 2) && xg_rep(s + cut, p, tail))
			return (true);
		cut++;
	}
	return (false);
}

/* The single-shot operators: `@` exactly one, `?` zero or one, `!` a prefix
   that matches NONE of the alternatives.  `!` is not "the opposite of @":
   it still has to leave a remainder the tail accepts, which is why it runs
   the same loop with the alternative test inverted rather than negating the
   whole answer. */
static bool	xg_once(const char *s, const char *p, const char *tail)
{
	size_t	cut;
	size_t	n;
	bool	hit;

	if (*p == '?' && case_match(s, tail))
		return (true);
	n = ft_strlen(s);
	cut = -1;
	while (++cut <= n)
	{
		hit = xg_any_alt(s, cut, p + 2);
		if (*p == '!' && !hit && case_match(s + cut, tail))
			return (true);
		if (*p != '!' && hit && case_match(s + cut, tail))
			return (true);
	}
	return (false);
}

/* Match `s` against a pattern that BEGINS with an extglob group at `p`.
   The whole remaining pattern is handled here -- the group and everything
   after it -- so case_match can hand off and return the answer directly. */
bool	xg_match(const char *s, const char *p)
{
	const char	*tail;

	tail = xg_group_end(p + 1);
	if (!tail)
		return (false);
	if (*p == '*')
		return (xg_rep(s, p, tail));
	if (*p == '+')
		return (xg_plus(s, p, tail));
	return (xg_once(s, p, tail));
}
