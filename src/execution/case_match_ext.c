/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   case_match_ext.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"
#include "ft_glob.h"
#include "case_match.h"

/* `shopt -s extglob` -- the five grouping operators:
**
**     ?(p)  zero or one      *(p)  zero or more      +(p)  one or more
**     @(p)  exactly one      !(p)  anything BUT p
**
** each taking `|`-separated alternatives that are themselves patterns.
**
** It reported `on` and then died in the PARSER, not the matcher: `@(a|b)`
** never became a word, so `case x in @(a|b))` was a syntax error and the
** option was pure fiction.  The lexer half is in helper2.c; this is the
** matching half.
**
** It hangs off case_match rather than beside it because an alternative is
** an ordinary pattern -- `@(a*|b?)` and nested `@(a|@(b|c))` both have to
** work -- so the two are mutually recursive by nature.  One matcher, both
** spellings: `case`, `[[ == ]]` and filename globbing cannot end up
** disagreeing about what a pattern means.
*/

/* Past the ')' that closes the group `p` opens, or NULL if unbalanced.
   `p` points at the '('. Nested groups are counted, so `@(a|@(b|c))` ends
   at the right paren rather than the first one. */
const char	*xg_group_end(const char *p)
{
	int	depth;

	depth = 0;
	p++;
	while (*p)
	{
		if (*p == '(')
			depth++;
		else if (*p == ')' && depth-- == 0)
			return (p + 1);
		p++;
	}
	return (NULL);
}

/* Does an extglob group start here? Gated on the option, so with extglob
   off `@(` is a literal at-sign followed by whatever the shell made of the
   paren -- exactly the reading every existing pattern has today. */
bool	xg_start(const char *p)
{
	return (*p && ft_strchr("?*+@!", *p) != NULL && p[1] == '('
		&& glob_extglob());
}

/* The end of the alternative starting at `p`: the next top-level `|`, or
   the group's closing `)`. Nested groups are skipped whole. */
static const char	*xg_alt_end(const char *p)
{
	int	depth;

	depth = 0;
	while (*p)
	{
		if (*p == '(')
			depth++;
		else if (*p == ')' && depth-- == 0)
			return (p);
		else if (*p == '|' && depth == 0)
			return (p);
		p++;
	}
	return (p);
}

/* Does the alternative slice [alt, alt+alen) match the prefix s[0..cut)?
   Both are copied out because case_match is whole-string by contract --
   and that contract is exactly what lets an alternative be any pattern at
   all, nested groups included. */
static bool	xg_try_alt(const char *s, size_t cut, const char *alt, int alen)
{
	char	*head;
	char	*pat;
	bool	ok;

	head = ft_strndup((char *)s, cut);
	pat = ft_strndup((char *)alt, (size_t)alen);
	ok = case_match(head, pat);
	xfree(head);
	xfree(pat);
	return (ok);
}

/* Does ANY alternative in the `|`-separated list match the prefix
   s[0..cut)? The list runs to the group's ')'. */
bool	xg_any_alt(const char *s, size_t cut, const char *alts)
{
	const char	*e;

	while (1)
	{
		e = xg_alt_end(alts);
		if (xg_try_alt(s, cut, alts, (int)(e - alts)))
			return (true);
		if (*e != '|')
			return (false);
		alts = e + 1;
	}
}
