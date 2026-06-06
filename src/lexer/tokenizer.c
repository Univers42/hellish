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

/* Decide whether the current position starts a quoted/dollar/word lexeme
   that needs parse_lexeme, or a bare operator. The OR with !is_word_boundary
   is the catch-all: any run of non-special chars is a plain word. */
static char	*try_parse_lexeme(char **str, t_deque_tok *ret)
{
	if (**str == '\'' || **str == '"' || **str == '$'
		|| !(is_word_boundary(*str)))
		return (parse_lexeme(ret, str));
	return (0);
}

/* Emit a single TT_NEWLINE token and advance past the '\n'. Newlines are
   kept as real tokens (not skipped like spaces) because the grammar uses
   them as statement separators inside compound commands. */
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

/* One tokenizing step: toggle [[ ]] mode, parse a lexeme, or emit an
   operator. Inside [[ ]], &&/||/(/) are emitted as WORD tokens so the parser
   does not split the conditional. Returns a continuation prompt if a lexeme
   is unterminated, else NULL. */
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

/* Top-level tokenizer. Walk the raw input string until exhausted, producing
   a token deque that ends with TT_END. Returns NULL on success or a
   continuation-prompt string when input is incomplete (unterminated quote,
   open subshell, line continuation). The caller re-prompts the user with
   that string and appends more text before calling back. The deque is cleared
   at the start so the same t_deque_tok can be reused across REPL turns
   without leaking the previous command's tokens. */
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
