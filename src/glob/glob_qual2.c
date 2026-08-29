/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_qual2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 11:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 11:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"
#include "ft_glob.h"

size_t	gq_run_start(t_vec_glob *g, const char **end);
void	gq_trim(t_vec_glob *g, size_t from, const char *open);

/* Parsing the `(...)` at the end of a glob pattern.
**
** The hard part is not the letters, it is deciding whether a trailing
** `(...)` is a QUALIFIER at all. `echo *(N)` is one in the zsh dialect, and
** `ls *(foo)` is an ordinary filename with parentheses in it -- both are
** valid, and the difference is whether every character inside is a
** qualifier letter. So the parse is all-or-nothing: one character that is
** not a qualifier makes the whole thing literal text, exactly as it is
** today, and nothing changes for a pattern that merely happens to end in a
** parenthesis.
*/

/* Consume a Y<digits> limit.  Returns the index after the digits, or -1 when
   no digits follow -- a bare `Y` is not a qualifier. */
static int	gq_limit(const char *s, int i, int len, t_gqual *q)
{
	int	n;

	n = 0;
	if (i >= len || !ft_isdigit((unsigned char)s[i]))
		return (-1);
	while (i < len && ft_isdigit((unsigned char)s[i]))
		n = n * 10 + (s[i++] - '0');
	q->limit = n;
	return (i);
}

/* One qualifier character.  Returns the next index, or -1 to reject the
   whole group -- which is how an unknown letter turns the parenthesis back
   into literal text instead of being silently ignored. */
static int	gq_one(const char *s, int i, int len, t_gqual *q)
{
	if (s[i] == 'D')
		return (q->dots = true, i + 1);
	if (s[i] == 'N')
		return (q->null = true, i + 1);
	if (s[i] == '.')
		return (q->plain = true, i + 1);
	if (s[i] == '/')
		return (q->dir = true, i + 1);
	if (s[i] == '@')
		return (q->link = true, i + 1);
	if (s[i] == 'Y')
		return (gq_limit(s, i + 1, len, q));
	return (-1);
}

/* Parse a `(...)` group out of one literal string.  On success `*plen` is
   cut back to exclude the group.
     The backward scan stops AT index 0 rather than below it. Letting it run
   to -1 read one byte before the buffer, which is the shape that segfaulted
   once a caller handed it a length spanning two allocations. */
static bool	gq_from_text(const char *pat, int *plen, t_gqual *q)
{
	int	open;
	int	i;

	if (*plen < 3 || pat[*plen - 1] != ')')
		return (false);
	open = *plen - 2;
	while (open > 0 && pat[open] != '(')
		open--;
	if (pat[open] != '(')
		return (false);
	i = open + 1;
	while (i < *plen - 1)
	{
		i = gq_one(pat, i, *plen - 1, q);
		if (i < 0)
			return (false);
	}
	q->on = true;
	*plen = open;
	return (true);
}

/* Find and parse a trailing qualifier group on a tokenised pattern, and trim
** it off so the walk never sees it.
**
** The group is always one TRAILING G_LITERAL: `(`, the letters and `)` are
** all ordinary characters to the tokeniser, so `*(DNY2)` is an asterisk
** followed by the literal "(DNY2)". Reading it there rather than from the
** word's text means the quoting has already been resolved -- `*"(N)"` is a
** filename and stays one, because a quoted token is a different token.
**
** Only in the zsh dialect: in bash `*(N)` is an extglob pattern meaning
** "zero or one N" -- the same bytes in a different language.
*/
bool	glob_qual_parse(t_vec_glob *g, t_gqual *q)
{
	const char	*end;
	size_t		from;
	int			len;

	*q = (t_gqual){0};
	if (!glob_zsh() || g->len == 0)
		return (false);
	if (((t_glob *)g->ctx)[g->len - 1].ty != G_LITERAL)
		return (false);
	from = gq_run_start(g, &end);
	len = (int)(end - ((t_glob *)g->ctx)[from].start);
	if (!gq_from_text(((t_glob *)g->ctx)[from].start, &len, q))
		return (false);
	gq_trim(g, from, ((t_glob *)g->ctx)[from].start + len);
	return (true);
}
