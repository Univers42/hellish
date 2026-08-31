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

/* Where does the bracket expression starting at p (p points at '[') close?
** NULL when it never does -- and that is not an error, it is a character:
** POSIX says a '[' introducing no valid bracket expression is an ordinary
** '['. Both directions were wrong without this:
**
**     case "["  in [)     bash: matches.    here: did not
**     case "a"  in [abc)  bash: no match.   here: matched, on the 'a'
**
** The second is the dangerous one: a pattern that matches things it does
** not name. `${line#[}`, stripping a literal bracket off an INI section
** header, is the shape that found it.
**
** A ']' as the FIRST member is a literal member and not the close, so
** `[]abc]` is a four-character class. */
static const char	*bracket_close(const char *p)
{
	const char	*q;

	q = p + 1;
	if (*q == '!' || *q == '^')
		q++;
	if (*q == ']')
		q++;
	while (*q && *q != ']')
		q++;
	if (*q == ']')
		return (q);
	return (NULL);
}

/* Match the single char `c` against a [...] bracket expression starting at
   *pp (which points at '['). Advances *pp past the closing ']'.
   The leading-']' rule is bracket_close's, applied again here: in `[]]`
   the first ']' is a MEMBER, so it has to be consumed as one rather than
   ending the scan before anything is collected. */
static bool	match_bracket(char c, const char **pp)
{
	const char	*p;
	bool		neg;
	bool		hit;

	p = *pp + 1;
	neg = (*p == '!' || *p == '^');
	p += neg;
	hit = (*p == ']' && c == ']');
	p += (*p == ']');
	while (*p && *p != ']')
	{
		if (p[1] == '-' && p[2] && p[2] != ']')
		{
			hit = hit || ((unsigned char)c >= (unsigned char)p[0]
					&& (unsigned char)c <= (unsigned char)p[2]);
			p += 3;
		}
		else
		{
			hit = hit || (c == *p);
			p++;
		}
	}
	*pp = p + (*p == ']');
	return (hit != neg);
}

/* Try to match exactly one character of *s against the non-'*' pattern
   at *p (?, [...], '\x', or a literal char).  Returns true and advances
   both pointers if it matched, false (leaving both unchanged) if not.
   The '\' case handles quoted metacharacters: `\*` in the pattern (placed
   there by append_pat_tok) must match a literal asterisk in the string.
   A '[' that bracket_close does not accept falls through to the literal
   arm, which is what makes an unterminated one an ordinary character. */
static bool	advance_one(const char **s, const char **p)
{
	if (**p == '?' && **s)
	{
		(*s)++;
		(*p)++;
	}
	else if (**p == '[' && bracket_close(*p))
	{
		if (!**s || !match_bracket(**s, p))
			return (false);
		(*s)++;
	}
	else if (**p == '\\' && (*p)[1] == **s && **s)
	{
		(*s)++;
		(*p) += 2;
	}
	else if (**p != '?' && **p != '\\' && **p == **s && **s)
	{
		(*s)++;
		(*p)++;
	}
	else
		return (false);
	return (true);
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
