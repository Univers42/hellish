/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers3.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 13:18:34 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 18:05:04 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Consume [a-zA-Z_][a-zA-Z0-9_]* from lex->input into a VAR token. The
   var_name pointer into the source (not a copy) and var_len are both set so
   get_var_value can look up the name without a malloc. Called either directly
   by arith_lexer_advance (bare name) or by lex_dollar_var after stripping
   the '$' sigil and optional braces. */
void	lex_variable(t_arith_lexer *lex)
{
	int	start;

	start = lex->pos;
	while (lex->pos < lex->len && is_var_char(lex->input[lex->pos]))
		lex->pos++;
	lex->current.type = ATOK_VAR;
	lex->current.var_name = (char *)(lex->input + start);
	lex->current.var_len = lex->pos - start;
	lex->current.start = lex->input + start;
	lex->current.len = lex->pos - start;
}

/* Lex a '$'-prefixed variable reference: $name or ${name}. The '$' is already
   at lex->pos when we're called; we skip it and the optional '{', then call
   lex_variable to consume the name itself. The closing '}' (if any) is
   consumed here. Bare '$' without a valid name character calls set_lex_error.
   Numeric args like $1 are also accepted by treating them as var names here,
   though they're unusual inside $(()). */
void	lex_dollar_var(t_arith_lexer *lex)
{
	int	braced;

	lex->pos++;
	braced = (lex->pos < lex->len && lex->input[lex->pos] == '{');
	if (braced)
		lex->pos++;
	if (lex->pos < lex->len && is_var_start(lex->input[lex->pos]))
		lex_variable(lex);
	else if (lex->pos < lex->len && ft_isdigit(lex->input[lex->pos]))
		lex_variable(lex);
	else
		set_lex_error(lex);
	if (braced && lex->pos < lex->len && lex->input[lex->pos] == '}')
		lex->pos++;
}

/* Emit either a two-character operator (type `dbl`) if the next byte is `c2`,
   or a one-character operator (type `single`) otherwise. Used for the pairs
   == vs = and != vs !  and & vs && and | vs ||. Sets start/len correctly in
   both cases so the caller can reconstruct the token's source slice. */
void	lex_two_char_op(t_arith_lexer *lex, char c2,
							t_arith_tok single, t_arith_tok dbl)
{
	if (lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == c2)
	{
		lex->current.type = dbl;
		lex->current.start = lex->input + lex->pos;
		lex->current.len = 2;
		lex->pos += 2;
	}
	else
	{
		lex->current.type = single;
		lex->current.start = lex->input + lex->pos;
		lex->current.len = 1;
		lex->pos++;
	}
}

/* Initialize a string-scanning lexer and prime it by calling advance once,
   so lex->current holds the first real token when the parser starts. The
   toks/ntoks fields are set to NULL/0 -- arith_lexer_advance will take the
   scanning branch, not the replay branch. */
void	arith_lexer_init(t_arith_lexer *lex, const char *input, int len)
{
	lex->input = input;
	lex->pos = 0;
	lex->len = len;
	lex->error = false;
	lex->error_msg = NULL;
	lex->toks = NULL;
	lex->current.type = ATOK_EOF;
	arith_lexer_advance(lex);
}

/* Return the current token without advancing. Named "peek" because it lets
   the parser look at the next token before deciding whether to consume it.
   Returns by value -- cheap since t_arith_token is a small struct. */
t_arith_token	arith_lexer_peek(t_arith_lexer *lex)
{
	return (lex->current);
}
