/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   casescan2.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/24 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "casescan.h"
#include "libft.h"

/* Comments and heredoc operators inside a $( ) span (issue #139). The
   bodies those operators owe are skipped by casescan3.c. */

bool	casescan_in(const t_casescan *cs, const char *s, int j)
{
	return ((cs->len < 0 || j < cs->len) && s[j] != '\0');
}

/* Does s[i] begin a word? Only there does `#` open a comment: after an
   unescaped blank, a newline, `;` `&` `|`, or the `(` of the `$(` itself.
   Not after any other `(` (zsh's `(#i)` glob flag, `${(#)x}`), nor after
   `)` (`$(a)#b` is one word). The backslash count keeps `\ #x` a word. */
static bool	cs_word_start(const char *s, int i)
{
	char	p;
	int		n;

	p = s[i - 1];
	if (p == '\n' || p == ';' || p == '&' || p == '|')
		return (true);
	if (p == '(')
		return (s[i - 2] == '$');
	if (p != ' ' && p != '\t')
		return (false);
	n = 0;
	while (s[i - 2 - n] == '\\')
		n++;
	return (n % 2 == 0);
}

/* A comment runs to the newline, which is left for the caller: it still
   ends the command line (and may owe heredoc bodies). */
bool	casescan_comment(t_casescan *cs, const char *s, int *i)
{
	if (s[*i] != '#' || !cs_word_start(s, *i))
		return (false);
	while (casescan_in(cs, s, *i) && s[*i] != '\n')
		(*i)++;
	return (true);
}

/* End of the delimiter word at s[j]: a quoted span belongs to it
   (`<<'E O'`), a backslash takes the next byte, and an unquoted blank
   or operator byte ends it. */
static int	cs_hd_word_end(const t_casescan *cs, const char *s, int j)
{
	char	q;

	while (casescan_in(cs, s, j) && !ft_strchr(" \t\n;&|<>()", s[j]))
	{
		if (s[j] == '\'' || s[j] == '"')
		{
			q = s[j++];
			while (casescan_in(cs, s, j) && s[j] != q)
				j++;
		}
		else if (s[j] == '\\')
			j++;
		if (casescan_in(cs, s, j))
			j++;
	}
	return (j);
}

/* `<<word` or `<<-word`: consume the operator and its word, and owe the
   body at the next newline. `<<<` is a here-string and `<<=` a shift
   assignment; neither owes one. */
bool	casescan_heredoc_op(t_casescan *cs, const char *s, int *i)
{
	int		j;
	bool	dash;

	if (s[*i] != '<' || !casescan_in(cs, s, *i + 1) || s[*i + 1] != '<')
		return (false);
	j = *i + 2;
	cs->cmdpos = false;
	if (casescan_in(cs, s, j) && (s[j] == '<' || s[j] == '='))
		return (*i = j + 1, true);
	dash = (casescan_in(cs, s, j) && s[j] == '-');
	j += dash;
	while (casescan_in(cs, s, j) && (s[j] == ' ' || s[j] == '\t'))
		j++;
	*i = cs_hd_word_end(cs, s, j);
	if (*i > j && cs->nhd < CASESCAN_HD_MAX)
		cs->hd[cs->nhd++] = (t_cshd){.word = s + j, .wlen = *i - j,
			.dash = dash};
	return (true);
}
