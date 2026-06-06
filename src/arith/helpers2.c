/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers2.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 13:18:44 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:25:02 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* '*' is ambiguous: look one character ahead. If the next char is also '*'
   we emit ATOK_POW (**); otherwise a plain multiplication ATOK_MUL. The
   two-char case advances pos by 2, the one-char case uses set_simple_op
   which advances by 1. */
static void	handle_star(t_arith_lexer *lex)
{
	if (lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == '*')
	{
		lex->current.type = ATOK_POW;
		lex->current.len = 2;
		lex->pos += 2;
	}
	else
		set_simple_op(lex, ATOK_MUL);
}

/* '<' can start three different tokens: '<=' (ATOK_LE), '<<' (ATOK_LSHIFT),
   or plain '<' (ATOK_LT). We peek one byte ahead and emit accordingly. The
   right-angle equivalent lives in helpers10.c as handle_angle_right. */
static void	handle_angle_left(t_arith_lexer *lex)
{
	if (lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == '=')
	{
		lex->current.type = ATOK_LE;
		lex->current.len = 2;
		lex->pos += 2;
	}
	else if (lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == '<')
	{
		lex->current.type = ATOK_LSHIFT;
		lex->current.len = 2;
		lex->pos += 2;
	}
	else
		set_simple_op(lex, ATOK_LT);
}

/* '+' doubles to '++' (ATOK_INC); '-' doubles to '--' (ATOK_DEC). If the
   next character doesn't match, emit the single-char operator. The parser
   then decides prefix vs. postfix based on whether this comes before or
   after a variable token. */
static void	handle_plus_minus(t_arith_lexer *lex, char c)
{
	if (c == '+' && lex->pos + 1 < lex->len
		&& lex->input[lex->pos + 1] == '+')
	{
		lex->current.type = ATOK_INC;
		lex->current.len = 2;
		lex->pos += 2;
	}
	else if (c == '-' && lex->pos + 1 < lex->len
		&& lex->input[lex->pos + 1] == '-')
	{
		lex->current.type = ATOK_DEC;
		lex->current.len = 2;
		lex->pos += 2;
	}
	else if (c == '+')
		set_simple_op(lex, ATOK_PLUS);
	else
		set_simple_op(lex, ATOK_MINUS);
}

/* Catch-all for the single-character operators that don't have a two-char
   variant and weren't already handled: / % ( ) ? : , ~ ^. Each maps to its
   ATOK_* constant. If none match, the caller falls through to set_lex_error. */
static void	handle_common_single(t_arith_lexer *lex, char c)
{
	if (c == '+' || c == '-')
		handle_plus_minus(lex, c);
	else if (c == '/')
		set_simple_op(lex, ATOK_DIV);
	else if (c == '%')
		set_simple_op(lex, ATOK_MOD);
	else if (c == '(')
		set_simple_op(lex, ATOK_LPAREN);
	else if (c == ')')
		set_simple_op(lex, ATOK_RPAREN);
	else if (c == '?')
		set_simple_op(lex, ATOK_TERNQ);
	else if (c == ':')
		set_simple_op(lex, ATOK_TERNC);
	else if (c == ',')
		set_simple_op(lex, ATOK_COMMA);
	else if (c == '~')
		set_simple_op(lex, ATOK_BNOT);
	else if (c == '^')
		set_simple_op(lex, ATOK_BXOR);
}

/* Top-level operator dispatcher: decode one operator token starting at
   lex->input[lex->pos]. All the multi-character operators are handled by
   dedicated helpers; the residual single-char ones go through
   handle_common_single. An unrecognised character sets ATOK_ERROR via
   set_lex_error -- the parser will then propagate p->error upward. */
void	lex_operator(t_arith_lexer *lex)
{
	char	c;

	c = lex->input[lex->pos];
	lex->current.start = lex->input + lex->pos;
	lex->current.len = 1;
	if (c == '*')
		handle_star(lex);
	else if (c == '<')
		handle_angle_left(lex);
	else if (c == '>')
		handle_angle_right(lex);
	else if (c == '=')
		lex_two_char_op(lex, '=', ATOK_ASSIGN, ATOK_EQ);
	else if (c == '!')
		lex_two_char_op(lex, '=', ATOK_NOT, ATOK_NE);
	else if (c == '&')
		lex_two_char_op(lex, '&', ATOK_BAND, ATOK_AND);
	else if (c == '|')
		lex_two_char_op(lex, '|', ATOK_BOR, ATOK_OR);
	else
	{
		handle_common_single(lex, c);
		if (lex->current.type == 0)
			set_lex_error(lex);
	}
}
