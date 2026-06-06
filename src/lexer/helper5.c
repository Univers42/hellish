/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helper5.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 23:05:17 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 23:05:17 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

/* Advance past a quoted span and return the continuation prompt if the quote
   is not closed before end-of-input. The caller (parse_lexeme or
   tokenize_subshell) already verified that **str == q. On unterminated
   input we store the missing char in tokens->looking_for so the REPL can
   show the user a targeted prompt ('>' for squote, "> for dquote). */
char	*parse_quote(t_deque_tok *tokens, char **str, char q)
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
