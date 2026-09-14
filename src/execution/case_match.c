/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   case_match.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"
#include "case_match.h"
#include "mbchar.h"

/* Walk the members between the (optional) leading ']' and the closing one,
   accumulating whether the character c (n bytes) matched any: [:class:]es,
   X-Y ranges, then single characters -- each member stepped as a whole
   character, so `[é]` is one member and `[à-ü]` a range of code points
   (issue #120). Stops at the closing ']' or at the end of the pattern
   slice, which bracket_close should already have rejected. */
static void	cm_members(t_cmp *m, const char *c, size_t n, bool *hit)
{
	size_t	k;

	while (m->p < m->pe && *m->p != ']')
	{
		k = mb_len0(m->p);
		if (k > (size_t)(m->pe - m->p))
			k = (size_t)(m->pe - m->p);
		if (m->p[0] == '[' && m->p + 1 < m->pe && m->p[1] == ':')
			*hit = cm_class_match(c, n, &m->p, m->pe) || *hit;
		else if (m->p + k + 1 < m->pe && m->p[k] == '-' && m->p[k + 1] != ']')
		{
			*hit = *hit || cm_in_range(c, n, m->p, m->p + k + 1);
			m->p += k + 1 + mb_len0(m->p + k + 1);
		}
		else
		{
			*hit = *hit || (k == n && ft_memcmp(m->p, c, n) == 0);
			m->p += k;
		}
	}
}

/* Match the subject character at m->s (n bytes) against the [...] bracket
   expression at m->p, advancing m->p past the closing ']'.
   The leading-']' rule is bracket_close's, applied again here: in `[]]`
   the first ']' is a MEMBER, so it has to be consumed as one rather than
   ending the scan before anything is collected. */
static bool	cm_bracket(t_cmp *m, size_t n)
{
	const char	*c;
	bool		neg;
	bool		hit;

	c = m->s;
	m->p++;
	neg = (m->p < m->pe && (*m->p == '!' || *m->p == '^'));
	m->p += neg;
	hit = (m->p < m->pe && *m->p == ']' && n == 1 && *c == ']');
	if (m->p < m->pe && *m->p == ']')
		m->p++;
	cm_members(m, c, n, &hit);
	if (m->p < m->pe && *m->p == ']')
		m->p++;
	return (hit != neg);
}

/* The single-byte arms of cm_advance_one: `\\x` for a quoted metacharacter
   (append_pat_tok puts it there; it must match a literal x) and a plain
   literal byte.  A multibyte literal in the pattern comes through here
   once per byte, which lands on the same answer. */
static bool	cm_advance_byte(t_cmp *m)
{
	if (m->s >= m->se)
		return (false);
	if (*m->p == '\\' && m->p + 1 < m->pe && m->p[1] == *m->s)
		m->p += 2;
	else if (*m->p != '?' && *m->p != '\\' && *m->p == *m->s)
		m->p++;
	else
		return (false);
	m->s++;
	return (true);
}

/* Try to match exactly one CHARACTER of the subject -- all of its bytes
   under a multibyte locale, issue #120 -- against the non-'*' pattern at
   m->p (?, [...], '\\x', or a literal char).  Returns true and advances
   both cursors if it matched, false (leaving both where they were) if not.
   A '[' that bracket_close does not accept falls through to the literal
   arm, which is what makes an unterminated one an ordinary character. */
static bool	cm_advance_one(t_cmp *m)
{
	size_t	n;

	n = 0;
	if (m->s < m->se)
		n = mb_len0(m->s);
	if (n > (size_t)(m->se - m->s))
		n = (size_t)(m->se - m->s);
	if (*m->p == '?' && n)
		return (m->s += n, m->p++, true);
	if (*m->p == '[' && bracket_close(m->p, m->pe))
	{
		if (!n || !cm_bracket(m, n))
			return (false);
		return (m->s += n, true);
	}
	return (cm_advance_byte(m));
}

/* fnmatch-style glob match used by `case` patterns: '*' '?' '[..]' '\',
   plus the extglob groups when `shopt -s extglob` is on.
     The extglob test comes FIRST in the loop because cm_advance_one would
   otherwise take the `?` of `?(a|b)` as an ordinary one-character wildcard
   and leave the group's paren behind. xg_match answers for the whole
   remaining pattern, so its result is the answer. */
bool	cm_run(t_cmp m)
{
	while (m.p < m.pe)
	{
		if (xg_start(m.p, m.pe))
			return (xg_match(m));
		if (*m.p == '*')
		{
			while (m.p < m.pe && *m.p == '*')
				m.p++;
			if (m.p == m.pe)
				return (true);
			while (m.s < m.se)
			{
				if (cm_run(m))
					return (true);
				m.s++;
			}
			return (cm_run(m));
		}
		if (!cm_advance_one(&m))
			return (false);
	}
	return (m.s == m.se);
}
