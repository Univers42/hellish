/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_subscript.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/14 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Where does a subscript end?

   At its own ']' -- which is not the first one, whenever anything nested
   brings a ']' of its own:

     ${a[${#b[@]}]}          the [@] closes first
     ${a[$((${#a[@]} - 1))]} same, inside arithmetic
     ${m[${k[0]}]}           a subscript inside a subscript

   Scanning forward for the first ']' cut those off mid-expression, and
   what came back was the tail of someone else's syntax: "$((${#a[@"
   reported as an arithmetic error, or "]}" taken for a parameter
   operator and answered with "bad substitution". bash reads all three.

   So this counts what it is inside. Brackets nest; ${...} and $(...) and
   $((...)) are skipped as one unit each, because a ']' inside them is
   theirs; a quoted ']' is text. A backslash hides the next character. */

/* Skip a quoted run starting at s[i] (i is the quote). A single quote
   takes everything to its partner; inside double quotes a backslash
   still escapes. Returns the index of the closing quote, or len. */
static int	skip_quoted(const char *s, int len, int i)
{
	char	q;

	q = s[i++];
	while (i < len && s[i] != q)
	{
		if (q == '"' && s[i] == '\\' && i + 1 < len)
			i++;
		i++;
	}
	return (i);
}

/* Skip ${...}, $(...) or $((...)) starting at the '$' at s[i]; returns
   the index of the construct's last character, or i when it is a plain
   '$'. The closer is counted by depth, so nesting inside is covered. */
static int	skip_dollar(const char *s, int len, int i)
{
	char	open;
	char	close;
	int		depth;

	if (i + 1 >= len || (s[i + 1] != '{' && s[i + 1] != '('))
		return (i);
	open = s[i + 1];
	close = '}';
	if (open == '(')
		close = ')';
	depth = 0;
	while (++i < len)
	{
		if (s[i] == '\'' || s[i] == '"')
			i = skip_quoted(s, len, i);
		else if (s[i] == open)
			depth++;
		else if (s[i] == close && --depth == 0)
			return (i);
	}
	return (len);
}

/* Index of the ']' closing the '[' at `open`, or -1 when there is none.
   `open` must be the '['. */
int	subscript_close(const char *s, int len, int open)
{
	int	i;
	int	depth;

	if (open < 0 || open >= len || s[open] != '[')
		return (-1);
	depth = 1;
	i = open;
	while (++i < len)
	{
		if (s[i] == '\\' && i + 1 < len)
			i++;
		else if (s[i] == '\'' || s[i] == '"')
			i = skip_quoted(s, len, i);
		else if (s[i] == '$')
			i = skip_dollar(s, len, i);
		else if (s[i] == '[')
			depth++;
		else if (s[i] == ']' && --depth == 0)
			return (i);
	}
	return (-1);
}
