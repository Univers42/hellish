/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helper1.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 23:04:21 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 23:04:21 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "helpers.h"

bool	is_word_boundary(const char *s);

static void	skip_shell_comment(char **str)
{
	while (**str && **str != '\n')
		(*str)++;
}

static char	*try_parse_lexeme(char **str, t_deque_tt *ret)
{
	if (**str == '\'' || **str == '"' || **str == '$'
		|| !(is_word_boundary(*str)))
		return (parse_lexeme(ret, str));
	return (0);
}

static void	emit_newline(char **str, t_deque_tt *ret)
{
	t_token	tmp;

	tmp = create_token(*str, 1, TT_NEWLINE);
	deque_push_end(&ret->deqtok, &tmp);
	(*str)++;
}

char	*tokenizer(char *str, t_deque_tt *ret)
{
	char	*prompt;
	t_token	tmp;

	prompt = 0;
	deque_clear(&ret->deqtok, NULL);
	while (str && *str)
	{
		if (*str == '#')
		{
			skip_shell_comment(&str);
			continue ;
		}
		prompt = try_parse_lexeme(&str, ret);
		if (prompt)
			break ;
		if (*str == '\n')
			emit_newline(&str, ret);
		else if (is_space(*str))
			str++;
		else if (*str)
			parse_op(ret, &str);
	}
	tmp = create_token(0, 0, TT_END);
	deque_push_end(&ret->deqtok, &tmp);
	return (prompt);
}
