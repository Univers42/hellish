/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   arith_climb2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/04 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Binding power of a binary operator (higher binds tighter), or 0 if `type`
   is not one of the collapsed bitor..multiplicative operators. Mirrors the
   precedence the old per-level descent encoded by nesting order. */
static int	binop_prec(t_arith_tok type)
{
	if (type == ATOK_MUL || type == ATOK_DIV || type == ATOK_MOD)
		return (8);
	if (type == ATOK_PLUS || type == ATOK_MINUS)
		return (7);
	if (type == ATOK_LSHIFT || type == ATOK_RSHIFT)
		return (6);
	if (type == ATOK_LT || type == ATOK_LE
		|| type == ATOK_GT || type == ATOK_GE)
		return (5);
	if (type == ATOK_EQ || type == ATOK_NE)
		return (4);
	if (type == ATOK_BAND)
		return (3);
	if (type == ATOK_BXOR)
		return (2);
	if (type == ATOK_BOR)
		return (1);
	return (0);
}

/* Precedence-climbing replacement for the bitor..multiplicative descent:
   one loop driven by binop_prec instead of ~8 nested functions. Operands are
   exponents (next-tighter level); left-assoc via the prec+1 recursive call.
   Pure ops, so the no_side_effects flag only matters inside apply_binop. */
long long	arith_parse_binop(t_arith_parser *p, int min_prec)
{
	long long		left;
	long long		right;
	t_arith_token	tok;
	int				prec;

	left = arith_parse_exponent(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		prec = binop_prec(tok.type);
		if (prec < min_prec)
			break ;
		arith_lexer_advance(p->lexer);
		right = arith_parse_binop(p, prec + 1);
		left = apply_binop(p, tok.type, left, right);
	}
	return (left);
}
