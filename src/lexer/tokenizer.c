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

static char	*try_parse_lexeme(char **str, t_deque_tok *ret)
{
	if (**str == '\'' || **str == '"' || **str == '$'
		|| !(is_word_boundary(*str)))
		return (parse_lexeme(ret, str));
	return (0);
}

static void	emit_newline(char **str, t_deque_tok *ret)
{
	t_token	tmp;

	tmp = create_token(*str, 1, TT_NEWLINE);
	deque_push_end(&ret->deqtok, &tmp);
	(*str)++;
}

/* Skip inter-token noise: shell comments and backslash-newline line
   continuations. The continuation must be removed here, not tokenised, else a
   `\<newline>` becomes a spurious empty/newline argument (notably in sourced
   function bodies, which the REPL's line-joining never reaches). */
static bool	skip_noise(char **str)
{
	if (**str == '#')
	{
		while (**str && **str != '\n')
			(*str)++;
		return (true);
	}
	if ((*str)[0] == '\\' && (*str)[1] == '\n')
		return (*str += 2, true);
	return (false);
}

/* One tokenizing step: toggle [[ ]] mode, parse a lexeme, or emit an operator.
   Inside [[ ]], &&/||/(/) are emitted as WORD tokens (not operators). Returns a
   non-null continuation prompt if a lexeme is unterminated, else NULL. */
static char	*tokenize_step(char **str, t_deque_tok *ret, int *in_db)
{
	char	*prompt;

	dbracket_toggle(*str, in_db);
	prompt = try_parse_lexeme(str, ret);
	if (prompt)
		return (prompt);
	if (**str == '\n')
		emit_newline(str, ret);
	else if (is_space(**str))
		(*str)++;
	else if (*in_db && emit_dbracket_word(str, ret))
		return (NULL);
	else if (**str)
		parse_op(ret, str);
	return (NULL);
}

char	*tokenizer(char *str, t_deque_tok *ret)
{
	char	*prompt;
	t_token	tmp;
	int		in_db;

	prompt = 0;
	in_db = 0;
	deque_clear(&ret->deqtok, NULL);
	while (str && *str)
	{
		if (skip_noise(&str))
			continue ;
		prompt = tokenize_step(&str, ret, &in_db);
		if (prompt)
			break ;
	}
	tmp = create_token(0, 0, TT_END);
	deque_push_end(&ret->deqtok, &tmp);
	return (prompt);
}
