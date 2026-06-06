/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers7.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:10:35 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 14:16:29 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Equality: relational (('==' | '!=') relational)*. Returns 0 or 1. Equality
   binds tighter than & but looser than the relational operators, exactly as
   in C -- `a < b == c < d` groups as `(a<b) == (c<d)`, not `a < (b==c) < d`.
   This ordering trips up beginners writing bitfield tests. */
long long	arith_parse_equality(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_relational(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_EQ)
		{
			arith_lexer_advance(p->lexer);
			left = left == arith_parse_relational(p);
		}
		else if (tok.type == ATOK_NE)
		{
			arith_lexer_advance(p->lexer);
			left = left != arith_parse_relational(p);
		}
		else
			break ;
	}
	return (left);
}

/* Exponent: unary ('**' exponent)? -- right-associative, so 2**3**2 is
   2**(3**2) = 512, not (2**3)**2 = 64. Achieved by recursing on
   arith_parse_exponent rather than calling unary again. Negative exponents
   return 0 (integer math, like bash). The iterative multiplication avoids
   importing <math.h> and stays on integer types throughout. */
long long	arith_parse_exponent(t_arith_parser *p)
{
	long long		base;
	long long		exp;
	long long		result;
	t_arith_token	tok;

	base = arith_parse_unary(p);
	tok = arith_lexer_peek(p->lexer);
	if (tok.type == ATOK_POW)
	{
		arith_lexer_advance(p->lexer);
		exp = arith_parse_exponent(p);
		if (p->error)
			return (0);
		if (exp < 0)
			return (0);
		result = 1;
		while (exp > 0)
		{
			result *= base;
			exp--;
		}
		return (result);
	}
	return (base);
}
