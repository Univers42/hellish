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

/* Advance past a double-quoted span. Double quotes still expand $(...),
   ${...} and `...`, so we must recurse into each of them — otherwise a `"`
   that belongs to the nested construct is read as the outer quote's end
   (autoconf: x="`... "" ...`"). Missing the ${...} case split
   "${u:-"a b"}" at the space, because the quote after :- looked like the
   closing one; the word came apart into two tokens and the reparser then
   tripped its own closing-quote assertion. advance_brace_param recurses
   back into here for a quote inside the braces, so the two stay in step.
   The prev_bs flag handles `\"` escapes while avoiding a double-count
   on `\\`. */
int	advance_dquoted(char **str)
{
	bool	prev_bs;

	ft_assert(**str == '\"');
	(*str)++;
	prev_bs = false;
	while (**str && (**str != '\"' || prev_bs))
	{
		if (!prev_bs && **str == '$' && (*str)[1] == '(')
			advance_cmdsub(str);
		else if (!prev_bs && **str == '$' && (*str)[1] == '{')
			advance_brace_param(str, 1);
		else if (!prev_bs && **str == '`')
			advance_backtick(str);
		else
		{
			prev_bs = **str == '\\' && !prev_bs;
			(*str)++;
		}
	}
	if (**str != '\"')
		return (1);
	(*str)++;
	return (0);
}

/* Scan a ${...} parameter expansion as one span so spaces inside (e.g.
   ${x:-a b c}) do not break the surrounding word. Honours nested braces and
   quoted segments. in_dq says the expansion sits inside a double-quoted
   word, where a ' is an ordinary character: bash prints a'b for
   "${u:-a'b}" but reports an unterminated quote for ${u:-a'b}.
   A backslash escapes the next byte, so ${u-\"} does
   not open a quoted section and ${u-\}} does not close the brace
   early — the same rule the reparser's scan_brace_depth applies. It
   has to be here too now that advance_dquoted recurses in: without it
   the \" of "${u-\"}" read as the start of a nested quote and
   swallowed the rest of the line. */
int	advance_brace_param(char **str, int in_dq)
{
	int	depth;

	*str += 2;
	depth = 1;
	while (**str && depth > 0)
	{
		if (**str == '\\' && (*str)[1])
			(*str) += 2;
		else if (**str == '\'' && !in_dq)
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

/* Advance past a single-quoted span. Single quotes are the simplest case:
   nothing is special inside them -- not even backslash -- so we just scan
   forward to the matching `'`. Returns 1 if the quote was never closed
   (caller will prompt for more input). */
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
