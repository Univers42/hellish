/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   span_skip.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "helpers.h"
#include "casescan.h"
#include "libft.h"

/* Bounded span skipping for quoted and nested word material.
**
** Everything here reads a SLICE: `s` need not be NUL-terminated at `len`,
** and nothing reads s[len]. A span that does not close inside the slice
** answers -1 -- the caller decides what an open span means, and none of
** them may read on past the end to find out. That rule is what the
** ${x//"/"/_} segfault broke (tests/expansion_quote_sweep_test.py). */

/* Past a '...', "..." or `...` opening at s[i]. A backslash escapes the
   next byte except inside single quotes, and double quotes hold nested
   $( ), ${ } and `...` spans whose own quotes are independent. */
static int	sp_quote(const char *s, int i, int len)
{
	char	q;

	q = s[i++];
	while (i >= 0 && i < len && s[i] != q)
	{
		if (q != '\'' && s[i] == '\\')
			i += 2;
		else if (q == '"' && (s[i] == '`' || (s[i] == '$' && i + 1 < len
					&& (s[i + 1] == '(' || s[i + 1] == '{'))))
			i = span_skip(s, i, len);
		else
			i++;
	}
	if (i < 0 || i >= len)
		return (-1);
	return (i + 1);
}

/* One unit inside a $( ): escapes, quotes and ${ } are skipped whole --
   they are word material, so they end command position -- and the rest
   goes to casescan, which owns the paren depth and case patterns. */
static int	sp_paren_step(t_casescan *cs, const char *s, int i, int *depth)
{
	if (s[i] == '\\')
		return (cs->cmdpos = false, i + 2);
	if (s[i] == '\'' || s[i] == '"' || s[i] == '`'
		|| (s[i] == '$' && i + 1 < cs->len && s[i + 1] == '{'))
		return (cs->cmdpos = false, span_skip(s, i, cs->len));
	*depth += casescan_step(cs, s, &i, *depth);
	return (i);
}

/* Past a $( ... ) or $(( ... )) opening at s[i]. */
static int	sp_paren(const char *s, int i, int len)
{
	t_casescan	cs;
	int			depth;

	casescan_init(&cs, len);
	depth = 1;
	i += 2;
	while (i >= 0 && i < len && depth > 0)
		i = sp_paren_step(&cs, s, i, &depth);
	if (i < 0 || depth > 0)
		return (-1);
	return (i);
}

/* Past a ${ ... } opening at s[i]. Only `${` opens a level, as in the
   lexer's advance_brace_param: a bare `{` is an ordinary character. */
static int	sp_brace(const char *s, int i, int len)
{
	int	depth;

	depth = 1;
	i += 2;
	while (i >= 0 && i < len && depth > 0)
	{
		if (s[i] == '\\')
			i += 2;
		else if (s[i] == '$' && i + 1 < len && s[i + 1] == '{')
		{
			depth++;
			i += 2;
		}
		else if (s[i] == '\'' || s[i] == '"' || s[i] == '`'
			|| (s[i] == '$' && i + 1 < len && s[i + 1] == '('))
			i = span_skip(s, i, len);
		else
			depth -= (s[i++] == '}');
	}
	if (i < 0 || depth > 0)
		return (-1);
	return (i);
}

/* Index just past the span that opens at s[i] -- a quote, a backtick,
   `$(` or `${` -- or -1 when it does not close before len. Any other byte
   is a span of one. */
int	span_skip(const char *s, int i, int len)
{
	if (i < 0 || i >= len)
		return (-1);
	if (s[i] == '$' && i + 1 < len && s[i + 1] == '(')
		return (sp_paren(s, i, len));
	if (s[i] == '$' && i + 1 < len && s[i + 1] == '{')
		return (sp_brace(s, i, len));
	if (s[i] == '\'' || s[i] == '"' || s[i] == '`')
		return (sp_quote(s, i, len));
	return (i + 1);
}
