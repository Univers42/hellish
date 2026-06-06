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

/* Binding power of a binary operator: higher number = tighter binding.
   Returns 0 for any token that isn't in the collapsed range (bitor..mul),
   which is the loop-exit sentinel in arith_parse_binop. The numbers map to
   the C/POSIX precedence table: * / % (8) > + - (7) > << >> (6) > comparisons
   (5) > == != (4) > & (3) > ^ (2) > | (1). */
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

/* Precedence-climbing loop replacing ~8 nested descent functions (bitor down
   to multiplicative). One loop is enough: peek at the next operator, get its
   binding power, and stop if it's below min_prec. Left-associativity is the
   default -- achieved by recursing with (prec + 1) so an equal-precedence op
   to the right binds less tightly than the current one. Call with min_prec=1
   to consume the full bitor..mul range. Operators above this range (&&, ||,
   ternary, comma, assignment) are still handled by separate functions. */
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
