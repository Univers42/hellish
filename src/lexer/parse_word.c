/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:59 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:59 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

/* forward declaration to avoid implicit declaration warning */
static char	*parse_quote(t_deque_tt *tokens, char **str, char q);

/* handle subshell $(...) with nested parentheses and quoted segments */
static char	*parse_subshell(t_deque_tt *tokens, char **str)
{
	int		depth;
	char	*res;

	(*str) += 2;
	depth = 1;
	while (**str && depth > 0)
	{
		if (**str == '\\')
			advance_bs(str);
		else if (**str == '\'' || **str == '"')
		{
			res = parse_quote(tokens, str, **str);
			if (res)
				return (res);
		}
		else
		{
			if (**str == '(')
				depth++;
			else if (**str == ')')
				depth--;
			(*str)++;
		}
	}
	return (0);
}

/* generic advancement for backslash / non-special / dollar */
static int	parse_generic(char **str)
{
	if (**str == '\\')
		advance_bs(str);
	else if (!is_special_char(**str) || **str == '$')
		(*str)++;
	else
		return (1);
	return (0);
}

/* unify single/double-quote advance and prompt handling */
static char	*parse_quote(t_deque_tt *tokens, char **str, char q)
{
	if (q == '\'')
	{
		if (advance_squoted(str))
			return (tokens->looking_for = '\'', LEXER_SQUOTE_PROMPT);
	}
	else if (q == '"')
	{
		if (advance_dquoted(str))
			return (tokens->looking_for = '"', LEXER_DQUOTE_PROMPT);
	}
	return (0);
}

/* push the final word token */
static void	push_word_token(t_deque_tt *tokens, char *start, char *end)
{
	t_token	tmp;

	tmp = create_token(start, (int)(end - start), TT_WORD);
	deque_push_end(&tokens->deqtok, &tmp);
}

/* parse a word lexeme, delegating to small helpers (<=25 lines) */
char	*parse_lexeme(t_deque_tt *tokens, char **str)
{
	char	*start;
	char	*res;

	start = *str;
	while (**str)
	{
		if (**str == '$' && (*str)[1] == '(')
		{
			res = parse_subshell(tokens, str);
			if (res)
				return (res);
			continue;
		}
		if (parse_generic(str) == 0)
			continue ;
		if (**str == '\'' || **str == '"')
		{
			res = parse_quote(tokens, str, **str);
			if ((res = parse_quote(tokens, str, **str)))
				return (res);
		}
		else
			break ;
	}
	push_word_token(tokens, start, *str);
	return (0);
}
