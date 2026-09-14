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
** It hangs off cm_run rather than beside it because an alternative is an
** ordinary pattern -- `@(a*|b?)` and nested `@(a|@(b|c))` both have to
** work -- so the two are mutually recursive by nature.  One matcher, both
** spellings: `case`, `[[ == ]]` and filename globbing cannot end up
** disagreeing about what a pattern means.
*/

/* Past the ')' that closes the group `p` opens, or NULL if unbalanced.
   `p` points at the '('. Nested groups are counted, so `@(a|@(b|c))` ends
   at the right paren rather than the first one.
     A backslash-escaped byte is stepped over whole. That is what a quoted
   `")"` inside a group is by the time it gets here (append_pat_tok escapes
   it), and counting it as the closing paren would end the group in the
   middle of itself. */
const char	*xg_group_end(const char *p, const char *pe)
{
	int	depth;

	depth = 0;
	p++;
	while (p < pe)
	{
		if (*p == '\\' && p + 1 < pe)
			p++;
		else if (*p == '(')
			depth++;
		else if (*p == ')' && depth-- == 0)
			return (p + 1);
		p++;
	}
	return (NULL);
}

/* Does a group start here?
     bash's spelling needs the operator AND the option, so with extglob off
   `@(` is a literal at-sign followed by whatever the shell made of the paren
   -- exactly the reading every existing pattern has today. zsh's spelling is
   the bare paren and needs neither; xg_alt_group_n carries that rule, and
   the `|` it insists on is what keeps `f()` a function definition. */
bool	xg_start(const char *p, const char *pe)
{
	if (p >= pe)
		return (false);
	if (*p == '(')
		return (xg_alt_group_n(p, pe) != 0);
	return (ft_strchr("?*+@!", *p) != NULL && p + 1 < pe && p[1] == '('
		&& glob_extglob());
}

/* The end of the alternative starting at `p`: the next top-level `|`, the
   group's closing `)`, or the end of the slice. Nested groups are skipped
   whole, and so is a backslash-escaped byte -- a quoted `"a|b"` written
   inside a group is ONE alternative, not two. */
const char	*xg_alt_end(const char *p, const char *pe)
{
	int	depth;

	depth = 0;
	while (p < pe)
	{
		if (*p == '\\' && p + 1 < pe)
			p++;
		else if (*p == '(')
			depth++;
		else if (*p == ')' && depth-- == 0)
			return (p);
		else if (*p == '|' && depth == 0)
			return (p);
		p++;
	}
	return (p);
}

/* Does ANY alternative match the first `cut` bytes of the subject?
     m carries the subject in m.s and the alternatives as the pattern slice
   [m.p, m.pe) -- m.pe is the group's ')' -- so each alternative is taken by
   moving two pointers rather than by copying it out, and so is the prefix
   it is asked about. That is the whole reason this matcher stopped being
   quadratic in memory: an alternative is any pattern, and cm_run is
   whole-slice by contract, which is exactly what makes nesting work. */
bool	xg_any_alt(t_cmp m, size_t cut)
{
	t_cmp	a;

	a.s = m.s;
	a.se = m.s + cut;
	while (1)
	{
		a.p = m.p;
		a.pe = xg_alt_end(m.p, m.pe);
		if (cm_run(a))
			return (true);
		if (a.pe >= m.pe || *a.pe != '|')
			return (false);
		m.p = a.pe + 1;
	}
}
