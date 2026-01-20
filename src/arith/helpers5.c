/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers5.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:13:25 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 14:15:46 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

static long long	handle_mul(t_arith_parser *p, long long left)
{
	arith_lexer_advance(p->lexer);
	return (left * arith_parse_exponent(p));
}

static long long	handle_div(t_arith_parser *p, long long left)
{
	long long	right;

	arith_lexer_advance(p->lexer);
	right = arith_parse_exponent(p);
	if (right == 0)
	{
		p->error = true;
		return (0);
	}
	return (left / right);
}

static long long	handle_mod(t_arith_parser *p, long long left)
{
	long long	right;

	arith_lexer_advance(p->lexer);
	right = arith_parse_exponent(p);
	if (right == 0)
	{
		p->error = true;
		return (0);
	}
	return (left % right);
}

static bool	is_multiplicative_op(t_arith_tok type)
{
	return (type == ATOK_MUL || type == ATOK_DIV || type == ATOK_MOD);
}

long long	arith_parse_multiplicative(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_exponent(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (!is_multiplicative_op(tok.type))
			break ;
		if (tok.type == ATOK_MUL)
			left = handle_mul(p, left);
		else if (tok.type == ATOK_DIV)
			left = handle_div(p, left);
		else if (tok.type == ATOK_MOD)
			left = handle_mod(p, left);
	}
	return (left);
}
