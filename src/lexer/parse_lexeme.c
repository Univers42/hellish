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
char	*tokenize_subshell(t_deque_tok *tokens, char **str);
char	*parse_quote(t_deque_tok *tokens, char **str, char q);

/* Check if current position should end word parsing */
bool	is_word_boundary(const char *s);

/* generic advancement for backslash / non-special / dollar */
static int	parse_generic(char **str)
{
	if (**str == '\\')
		advance_bs(str);
	else if (!is_word_boundary(*str) || **str == '$')
		(*str)++;
	else
		return (1);
	return (0);
}

/* unify single/double-quote advance and prompt handling */
/* push the final word token */
static void	push_word_token(t_deque_tok *tokens, char *start, char *end)
{
	t_token	tmp;

	tmp = create_token(start, (int)(end - start), TT_WORD);
	deque_push_end(&tokens->deqtok, &tmp);
}

static int	handle_special(t_deque_tok *tokens, char **str, char **out)
{
	char	*res;

	if (**str == '$' && (*str)[1] == '(')
	{
		res = tokenize_subshell(tokens, str);
		if (res)
			return (*out = res, -1);
		return (1);
	}
	if (**str == '`')
		return (advance_backtick(str), 1);
	if (**str == '$' && (*str)[1] == '{')
		return (advance_brace_param(str), 1);
	return (0);
}

static int	handle_next_chunk(t_deque_tok *tokens,
							char **str,
							char **out_prompt)
{
	int		ret;
	char	*res;

	ret = handle_special(tokens, str, out_prompt);
	if (ret != 0)
		return (ret);
	if (parse_generic(str) == 0)
		return (1);
	if (**str == '\'' || **str == '"')
	{
		res = parse_quote(tokens, str, **str);
		if (res)
			return (*out_prompt = res, -1);
		return (1);
	}
	return (0);
}

/* parse a word lexeme, delegating to small helpers (<=25 lines) */
char	*parse_lexeme(t_deque_tok *tokens, char **str)
{
	char	*start;
	char	*res;
	int		ret;

	start = *str;
	while (**str)
	{
		ret = handle_next_chunk(tokens, str, &res);
		if (ret == -1)
			return (res);
		if (ret == 0)
			break ;
	}
	push_word_token(tokens, start, *str);
	return (0);
}
