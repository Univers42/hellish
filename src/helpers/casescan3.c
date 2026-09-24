/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   casescan3.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/24 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "casescan.h"
#include "libft.h"

/* Heredoc bodies inside a $( ) span, and the parens (issue #139). */

/* Is [line, line + n) the delimiter line of h? The word is compared with
   its quotes dropped and one backslash level undone -- the rule hd_delim
   applies when the body is read, so both agree on where a body ends. */
static bool	cs_delim_is(const t_cshd *h, const char *line, int n)
{
	int		a;
	int		b;
	char	c;

	a = 0;
	b = 0;
	while (h->dash && b < n && line[b] == '\t')
		b++;
	while (a < h->wlen)
	{
		c = h->word[a++];
		if (c == '\\' && a < h->wlen)
			c = h->word[a++];
		else if (c == '\'' || c == '"')
			c = '\0';
		if (c && (b >= n || line[b++] != c))
			return (false);
	}
	return (b == n);
}

/* Past one body: every line up to and including the delimiter line, or
   to the end of the span when that line never comes. */
static void	cs_skip_body(t_casescan *cs, const char *s, int *i,
				const t_cshd *h)
{
	int	ls;

	while (casescan_in(cs, s, *i))
	{
		ls = *i;
		while (casescan_in(cs, s, *i) && s[*i] != '\n')
			(*i)++;
		if (cs_delim_is(h, s + ls, *i - ls))
		{
			*i += casescan_in(cs, s, *i);
			return ;
		}
		*i += casescan_in(cs, s, *i);
	}
}

/* The newline that ends a command line: every body owed on it follows,
   in operator order, and none of their bytes is shell text. */
bool	casescan_bodies(t_casescan *cs, const char *s, int *i)
{
	int	k;

	if (s[*i] != '\n' || cs->nhd == 0)
		return (false);
	(*i)++;
	k = 0;
	while (k < cs->nhd)
		cs_skip_body(cs, s, i, &cs->hd[k++]);
	cs->nhd = 0;
	cs->cmdpos = true;
	return (true);
}

/* `(` and `)`: the depth delta, and the case-pattern exception -- a `)`
   at the depth where the innermost open `case` started ends a pattern,
   not the span. A `(` straight after a `(` opens (( )) or $(( )), whose
   `<<` is a shift: the heredoc scan stays off until the depth drops back
   below where that span began. */
int	casescan_paren(t_casescan *cs, const char *s, int *i, int depth)
{
	if (s[*i] == '(')
	{
		if (cs->arith < 0 && s[*i - 1] == '(')
			cs->arith = depth - 1;
		return (cs->cmdpos = true, (*i)++, 1);
	}
	(*i)++;
	if (cs->ncase > 0 && cs->at[cs->ncase - 1] == depth)
		return (cs->cmdpos = true, 0);
	return (cs->cmdpos = false, -1);
}
