/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers3.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 13:18:34 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 13:48:52 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

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

void	arith_lexer_init(t_arith_lexer *lex, const char *input, int len)
{
	lex->input = input;
	lex->pos = 0;
	lex->len = len;
	lex->error = false;
	lex->error_msg = NULL;
	lex->current.type = ATOK_EOF;
	arith_lexer_advance(lex);
}

t_arith_token	arith_lexer_peek(t_arith_lexer *lex)
{
	return (lex->current);
}
