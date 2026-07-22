/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tokenizer2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 23:04:21 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 23:04:21 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

/* True when the last slot pushed is a TT_NEWLINE. emit_newline only fires
   between lexemes at top quote level, so this marks "one logical line lexed"
   -- the point where lex_line stops and the pull driver can hand the range to
   the parser or resume for the next line. Reads the bitfield tt directly. */
static bool	deque_ends_nl(t_deque_tok *ret)
{
	t_ltoken	*last;

	if (ret->deqtok.len == 0)
		return (false);
	last = (t_ltoken *)deque_idx(&ret->deqtok, ret->deqtok.len - 1);
	return (last->tt == TT_NEWLINE);
}

/* Resumable single-logical-line lexer for the pull path. Tokenizes from *str
   until a newline is emitted at top quote level, or the buffer ends, appending
   slots to ret. Offsets are stored relative to `base` -- the WHOLE cycle
   buffer -- not *str, so a construct lexed across several calls keeps coherent
   offsets. Unlike tokenizer() it neither clears the deque nor pushes TT_END:
   the caller owns range boundaries and the sentinel, so multi-line constructs
   accumulate across calls into one deque. Returns a continuation prompt when a
   lexeme is unterminated, else NULL. */
char	*lex_line(char *base, char **str, t_deque_tok *ret, int *in_db)
{
	char	*prompt;

	ret->base = base;
	prompt = 0;
	while (*str && **str)
	{
		if (skip_noise(str))
			continue ;
		prompt = tokenize_step(str, ret, in_db);
		if (prompt)
			return (prompt);
		if (deque_ends_nl(ret))
			return (NULL);
	}
	return (NULL);
}
