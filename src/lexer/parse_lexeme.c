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

bool	is_word_boundary(const char *s);

/* Advance one "generic" unit of a word: backslash-escaped char, bare
   non-special char, or a bare `$` not followed by `(` or `{` (those are
   handled by handle_special). Returns 0 if we hit a real boundary and the
   word should stop, 1 if we consumed something and should keep going. */
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

/* Finalise a word: everything from start..end becomes a single TT_WORD
   token. The span is a slice into the original input buffer -- no copy is
   made here. Copying happens later in the expander if needed. */
static void	push_word_token(t_deque_tok *tokens, char *start, char *end)
{
	t_token	tmp;

	tmp = create_token(start, (int)(end - start), TT_WORD);
	deque_push_end(&tokens->deqtok, &tmp);
}

/* Handle `$(...)`, backtick `` ` ``, and `${...}` expansions embedded in
   a word. These are scanned as opaque spans at lex time -- the actual
   expansion happens at execution. Returns -1 on incomplete input (caller
   returns the prompt string), 1 if something was consumed, 0 if nothing
   matched so parse_generic should try instead. */
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

/* Try each category of word content in priority order: special expansions
   first, then generic chars, then quotes. Returns 1 if we consumed chars
   and should keep scanning, 0 if we hit a real boundary (word done), or
   -1 with *out_prompt set if input is unterminated. */
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

/* Consume one complete word lexeme starting at *str. A word can contain
   quoted spans, $(...) substitutions, ${...} expansions, and backticks --
   all glued together without whitespace. When input ends inside any of those
   the function returns a continuation-prompt string; otherwise NULL. The
   word token is pushed onto tokens before returning NULL. */
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
