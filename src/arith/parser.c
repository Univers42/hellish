/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 14:19:28 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Primary: number | variable | '(' expr ')' */
long long	arith_parse_primary(t_arith_parser *p)
{
	t_arith_token	tok;
	long long		val;

	if (p->error)
		return (0);
	tok = arith_lexer_peek(p->lexer);
	if (tok.type == ATOK_NUM)
	{
		arith_lexer_advance(p->lexer);
		return (tok.num_val);
	}
	else if (tok.type == ATOK_VAR)
	{
		arith_lexer_advance(p->lexer);
		return (get_var_value(p, tok.var_name, tok.var_len));
	}
	else if (tok.type == ATOK_LPAREN)
	{
		arith_lexer_advance(p->lexer);
		val = arith_parse_expr(p);
		expect(p, ATOK_RPAREN);
		return (val);
	}
	p->error = true;
	return (0);
}

/* Unary: ('+' | '-' | '!' | '~') unary | primary */
long long	arith_parse_unary(t_arith_parser *p)
{
	t_arith_token	tok;

	if (p->error)
		return (0);
	tok = arith_lexer_peek(p->lexer);
	if (tok.type == ATOK_PLUS)
		return (arith_lexer_advance(p->lexer), arith_parse_unary(p));
	else if (tok.type == ATOK_MINUS)
		return (arith_lexer_advance(p->lexer), -arith_parse_unary(p));
	else if (tok.type == ATOK_NOT)
		return (arith_lexer_advance(p->lexer), !arith_parse_unary(p));
	else if (tok.type == ATOK_BNOT)
		return (arith_lexer_advance(p->lexer), ~arith_parse_unary(p));
	return (arith_parse_primary(p));
}
