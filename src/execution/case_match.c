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
   (issue #120). Returns where the walk stopped -- the closing ']' or the
   NUL of an expression bracket_close should have rejected. */
static const char	*cm_members(const char *c, size_t n, const char *p,
					bool *hit)
{
	size_t	m;

	while (*p && *p != ']')
	{
		m = mb_len0(p);
		if (p[0] == '[' && p[1] == ':')
			*hit = cm_class_match(c, n, &p) || *hit;
		else if (p[m] == '-' && p[m + 1] && p[m + 1] != ']')
		{
			*hit = *hit || cm_in_range(c, n, p, p + m + 1);
			p += m + 1 + mb_len0(p + m + 1);
		}
		else
		{
			*hit = *hit || (m == n && ft_memcmp(p, c, n) == 0);
			p += m;
		}
	}
	return (p);
}

/* Match the single character c (n bytes) against a [...] bracket expression
   starting at *pp (which points at '['). Advances *pp past the closing ']'.
   The leading-']' rule is bracket_close's, applied again here: in `[]]`
   the first ']' is a MEMBER, so it has to be consumed as one rather than
   ending the scan before anything is collected. */
static bool	match_bracket(const char *c, size_t n, const char **pp)
{
	const char	*p;
	bool		neg;
	bool		hit;

	p = *pp + 1;
	neg = (*p == '!' || *p == '^');
	p += neg;
	hit = (*p == ']' && n == 1 && *c == ']');
	p += (*p == ']');
	p = cm_members(c, n, p, &hit);
	*pp = p + (*p == ']');
	return (hit != neg);
}

/* The single-byte arms of advance_one: `\\x` for a quoted metacharacter
   (append_pat_tok puts it there; it must match a literal x) and a plain
   literal byte.  A multibyte literal in the pattern comes through here
   once per byte, which lands on the same answer. */
static bool	advance_byte(const char **s, const char **p)
{
	if (**p == '\\' && (*p)[1] == **s && **s)
		(*p) += 2;
	else if (**p != '?' && **p != '\\' && **p == **s && **s)
		(*p)++;
	else
		return (false);
	(*s)++;
	return (true);
}

/* Try to match exactly one CHARACTER of *s -- all of its bytes under a
   multibyte locale, issue #120 -- against the non-'*' pattern at *p (?,
   [...], '\\x', or a literal char).  Returns true and advances both
   pointers if it matched, false (leaving both unchanged) if not.  A '['
   that bracket_close does not accept falls through to the literal arm,
   which is what makes an unterminated one an ordinary character. */
static bool	advance_one(const char **s, const char **p)
{
	size_t	n;

	n = mb_len0(*s);
	if (**p == '?' && **s)
	{
		(*s) += n;
		(*p)++;
		return (true);
	}
	if (**p == '[' && bracket_close(*p))
	{
		if (!**s || !match_bracket(*s, n, p))
			return (false);
		(*s) += n;
		return (true);
	}
	return (advance_byte(s, p));
}

/* fnmatch-style glob match used by `case` patterns: '*' '?' '[..]' '\',
   plus the extglob groups when `shopt -s extglob` is on.
     The extglob test comes FIRST in the loop because advance_one would
   otherwise take the `?` of `?(a|b)` as an ordinary one-character wildcard
   and leave the group's paren behind. xg_match answers for the whole
   remaining pattern, so its result is the answer. */
bool	case_match(const char *s, const char *p)
{
	while (*p)
	{
		if (xg_start(p))
			return (xg_match(s, p));
		if (*p == '*')
		{
			while (*p == '*')
				p++;
			if (!*p)
				return (true);
			while (*s)
				if (case_match(s++, p))
					return (true);
			return (case_match(s, p));
		}
		if (!advance_one(&s, &p))
			return (false);
	}
	return (*s == '\0');
}
