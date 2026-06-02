/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 04:13:35 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

bool	is_compound_op(t_arith_tok op)
{
	return (op == ATOK_PLUS || op == ATOK_MINUS
		|| op == ATOK_MUL || op == ATOK_DIV || op == ATOK_MOD
		|| op == ATOK_LSHIFT || op == ATOK_RSHIFT
		|| op == ATOK_BAND || op == ATOK_BXOR || op == ATOK_BOR);
}

static long long	apply_divmod(long long l, long long r,
	t_arith_tok op, t_arith_parser *p)
{
	if (r == 0)
	{
		if (!p->no_side_effects)
			p->error = true;
		return (0);
	}
	if (op == ATOK_DIV)
	{
		if (l == LLONG_MIN && r == -1)
			return (LLONG_MIN);
		return (l / r);
	}
	if (l == LLONG_MIN && r == -1)
		return (0);
	return (l % r);
}

long long	apply_op(long long l, long long r, t_arith_tok op,
	t_arith_parser *p)
{
	if (op == ATOK_PLUS)
		return (l + r);
	if (op == ATOK_MINUS)
		return (l - r);
	if (op == ATOK_MUL)
		return (l * r);
	if (op == ATOK_DIV || op == ATOK_MOD)
		return (apply_divmod(l, r, op, p));
	if (op == ATOK_LSHIFT)
		return (l << r);
	if (op == ATOK_RSHIFT)
		return (l >> r);
	if (op == ATOK_BAND)
		return (l & r);
	if (op == ATOK_BXOR)
		return (l ^ r);
	return (l | r);
}

/* Handle post-increment, post-decrement, assign, compound-assign for VAR */
static long long	primary_var(t_arith_parser *p, t_arith_token tok)
{
	t_arith_token	next;
	long long		val;

	val = get_var_value(p, tok.var_name, tok.var_len);
	next = arith_lexer_peek(p->lexer);
	if (next.type == ATOK_INC)
	{
		arith_lexer_advance(p->lexer);
		set_var_value(p, tok.var_name, tok.var_len, val + 1);
		return (val);
	}
	if (next.type == ATOK_DEC)
	{
		arith_lexer_advance(p->lexer);
		set_var_value(p, tok.var_name, tok.var_len, val - 1);
		return (val);
	}
	if (next.type == ATOK_ASSIGN)
	{
		arith_lexer_advance(p->lexer);
		val = arith_parse_ternary(p);
		set_var_value(p, tok.var_name, tok.var_len, val);
		return (val);
	}
	return (try_compound_assign(p, &tok, val));
}

/* Primary: number | variable | '(' expr ')' */
long long	arith_parse_primary(t_arith_parser *p)
{
	t_arith_token	tok;
	long long		val;

	if (p->error)
		return (0);
	tok = arith_lexer_peek(p->lexer);
	if (tok.type == ATOK_NUM)
		return (arith_lexer_advance(p->lexer), tok.num_val);
	if (tok.type == ATOK_VAR)
	{
		arith_lexer_advance(p->lexer);
		return (primary_var(p, tok));
	}
	if (tok.type == ATOK_LPAREN)
	{
		arith_lexer_advance(p->lexer);
		val = arith_parse_expr(p);
		expect(p, ATOK_RPAREN);
		return (val);
	}
	p->error = true;
	return (0);
}
