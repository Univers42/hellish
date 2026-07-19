/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alias_scan4.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/19 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/19 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "sh_alias.h"

/* Span of a quoted string starting at ' or ".  Inside double quotes a
   backslash escapes the next character; single quotes are literal to the
   closing quote.  An unterminated quote spans to the end (the tokenizer
   will be the one to ask for more input). */
size_t	asc_span_quote(const char *p)
{
	size_t	i;
	char	q;

	q = p[0];
	i = 1;
	while (p[i] && p[i] != q)
	{
		if (q == '"' && p[i] == '\\' && p[i + 1])
			i += 2;
		else
			i++;
	}
	if (p[i] == q)
		i++;
	return (i);
}

/* Span of $ at p: $(...) and $((...)) are copied through opaquely with
   balanced parens (their bodies get alias treatment when the subshell
   parses them); a plain $var is just the dollar, the name chars follow
   as ordinary word characters. */
size_t	asc_span_dollar(const char *p)
{
	size_t	i;
	int		depth;

	if (p[1] != '(')
		return (1);
	i = 2;
	depth = 1;
	while (p[i] && depth > 0)
	{
		if (p[i] == '\'' || p[i] == '"')
			i += asc_span_quote(p + i);
		else if (p[i] == '\\' && p[i + 1])
			i += 2;
		else
		{
			if (p[i] == '(')
				depth++;
			if (p[i] == ')')
				depth--;
			i++;
		}
	}
	return (i);
}

/* Span of a backtick command substitution: to the closing unescaped
   backtick (or end of input if unterminated). */
size_t	asc_span_btick(const char *p)
{
	size_t	i;

	i = 1;
	while (p[i] && p[i] != '`')
	{
		if (p[i] == '\\' && p[i + 1])
			i += 2;
		else
			i++;
	}
	if (p[i] == '`')
		i++;
	return (i);
}

/* An fd number glued to a redirection (2>err, 10<in) is part of the
   operator, not a command word: all digits and the very next character
   is < or >. */
bool	asc_digits_redir(const char *w, size_t len)
{
	size_t	i;

	if (len == 0)
		return (false);
	i = 0;
	while (i < len)
	{
		if (!ft_isdigit(w[i]))
			return (false);
		i++;
	}
	return (w[len] == '<' || w[len] == '>');
}

/* Assignment prefix (NAME=...): keeps the scanner looking for the command
   word, and is itself never an alias candidate. */
bool	asc_is_assign(const char *w, size_t len)
{
	size_t	i;

	if (len == 0 || (!ft_isalpha(w[0]) && w[0] != '_'))
		return (false);
	i = 1;
	while (i < len && (ft_isalnum(w[i]) || w[i] == '_'))
		i++;
	return (i < len && w[i] == '=');
}
