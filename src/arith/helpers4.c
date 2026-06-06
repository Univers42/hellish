/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers4.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:05:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:25:02 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Additive: multiplicative (('+' | '-') multiplicative)*. The left-to-right
   loop is the classic iterative descent for left-associative operators. Note:
   this function is kept for code paths that bypass the precedence climber
   (e.g. helpers5.c's mul/div which calls arith_parse_exponent directly). */
long long	arith_parse_additive(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_multiplicative(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_PLUS)
		{
			arith_lexer_advance(p->lexer);
			left = left + arith_parse_multiplicative(p);
		}
		else if (tok.type == ATOK_MINUS)
		{
			arith_lexer_advance(p->lexer);
			left = left - arith_parse_multiplicative(p);
		}
		else
			break ;
	}
	return (left);
}

/* Shift: additive (('<<' | '>>') additive)*. Shift binds tighter than
   comparison but looser than addition, matching the C precedence table. */
long long	arith_parse_shift(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_additive(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_LSHIFT)
		{
			arith_lexer_advance(p->lexer);
			left = left << arith_parse_additive(p);
		}
		else if (tok.type == ATOK_RSHIFT)
		{
			arith_lexer_advance(p->lexer);
			left = left >> arith_parse_additive(p);
		}
		else
			break ;
	}
	return (left);
}

/* Store a relational comparison result (0 or 1) into *res. The `op` enum
   encodes the operator: 0=<, 1=<=, 2=>, 3>=. Splitting into a helper keeps
   do_relop at a single-statement call and avoids repeating the advance. */
static void	cmp_result(int *res, long long a, long long b, int op)
{
	if (op == 0)
		*res = (a < b);
	else if (op == 1)
		*res = (a <= b);
	else if (op == 2)
		*res = (a > b);
	else if (op == 3)
		*res = (a >= b);
}

/* Advance past one relational operator, evaluate the right operand (a shift
   expression), and update *left with the 0/1 comparison result. Called once
   per iteration of the arith_parse_relational loop. */
static void	do_relop(t_arith_parser *p, int *res, long long *left, int op)
{
	long long	right;

	arith_lexer_advance(p->lexer);
	right = arith_parse_shift(p);
	cmp_result(res, *left, right, op);
	*left = *res;
}

/* Relational: shift (('<' | '<=' | '>' | '>=') shift)*. Returns 0 or 1 per
   comparison; chaining like `1 < 2 < 3` is valid (left-to-right) but gives
   the same result as in C (value, not mathematical chaining). */
long long	arith_parse_relational(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;
	int				res;

	left = arith_parse_shift(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_LT)
			do_relop(p, &res, &left, 0);
		else if (tok.type == ATOK_LE)
			do_relop(p, &res, &left, 1);
		else if (tok.type == ATOK_GT)
			do_relop(p, &res, &left, 2);
		else if (tok.type == ATOK_GE)
			do_relop(p, &res, &left, 3);
		else
			break ;
	}
	return (left);
}
