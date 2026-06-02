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

/* Handle '*' or '**' (exponentiation) */
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

/* Handle '<' and '>' with their two-char variants */
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
