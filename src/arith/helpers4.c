/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers4.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:05:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 18:37:44 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"
#include <string.h>

/* Additive: multiplicative (('+' | '-') multiplicative)* */
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

/* Shift: additive (('<<' | '>>') additive)* */
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

static void	cmp_result(int *res, long long a, long long b, int op)
{
	int		cmp;

	cmp = ft_memcmp(&a, &b, sizeof(long long));
	if (op == 0)
		*res = (cmp < 0);
	else if (op == 1)
		*res = (cmp <= 0);
	else if (op == 2)
		*res = (cmp > 0);
	else if (op == 3)
		*res = (cmp >= 0);
}

static void	do_relop(t_arith_parser *p, int *res, long long *left, int op)
{
	long long	right;

	arith_lexer_advance(p->lexer);
	right = arith_parse_shift(p);
	cmp_result(res, *left, right, op);
	*left = *res;
}

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
