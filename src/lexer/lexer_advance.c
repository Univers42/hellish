/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:55 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:55 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

static void	advance_cmdsub(char **str);

/* Skip a $(...) command substitution, honouring nested parens and quoted
   segments (whose quotes are independent of any enclosing quote). Used so a
   '"' inside $(...) does not prematurely close an outer double quote. */
static void	advance_cmdsub(char **str)
{
	int	depth;

	*str += 2;
	depth = 1;
	while (**str && depth > 0)
	{
		if (**str == '\\')
			advance_bs(str);
		else if (**str == '\'')
			advance_squoted(str);
		else if (**str == '\"')
			advance_dquoted(str);
		else
		{
			depth += (**str == '(');
			depth -= (**str == ')');
			(*str)++;
		}
	}
}

int	advance_dquoted(char **str)
{
	bool	prev_bs;

	ft_assert(**str == '\"');
	(*str)++;
	prev_bs = false;
	while (**str && (**str != '\"' || prev_bs))
	{
		if (!prev_bs && **str == '$' && (*str)[1] == '(')
		{
			advance_cmdsub(str);
			continue ;
		}
		prev_bs = **str == '\\' && !prev_bs;
		(*str)++;
	}
	if (**str != '\"')
		return (1);
	(*str)++;
	return (0);
}

/* Scan a ${...} parameter expansion as one span so spaces inside (e.g.
   ${x:-a b c}) do not break the surrounding word. Honours nested braces and
   quoted segments. */
int	advance_brace_param(char **str)
{
	int	depth;

	*str += 2;
	depth = 1;
	while (**str && depth > 0)
	{
		if (**str == '\'')
			advance_squoted(str);
		else if (**str == '"')
			advance_dquoted(str);
		else
		{
			depth += (**str == '{');
			depth -= (**str == '}');
			(*str)++;
		}
	}
	return (depth != 0);
}

/* Scan a `...` backtick command substitution as one span so spaces inside do
   not break the surrounding word. Honours \` escapes. */
int	advance_backtick(char **str)
{
	ft_assert(**str == '`');
	(*str)++;
	while (**str && **str != '`')
	{
		if (**str == '\\' && (*str)[1])
			(*str)++;
		(*str)++;
	}
	if (**str != '`')
		return (1);
	(*str)++;
	return (0);
}

int	advance_squoted(char **str)
{
	ft_assert(**str == '\'');
	(*str)++;
	while (**str && **str != '\'')
	{
		(*str)++;
	}
	if (**str != '\'')
		return (1);
	(*str)++;
	return (0);
}
