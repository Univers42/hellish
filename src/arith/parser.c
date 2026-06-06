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

/* Tell whether `op` is one of the binary operators that can appear on the
   left side of a compound assignment like "+=" or ">>=" -- i.e. every
   arithmetic/bitwise op except the logical ones (&&, ||) and comparisons.
   Called by try_compound_assign to gate the two-token lookahead. */
bool	is_compound_op(t_arith_tok op)
{
	return (op == ATOK_PLUS || op == ATOK_MINUS
		|| op == ATOK_MUL || op == ATOK_DIV || op == ATOK_MOD
		|| op == ATOK_LSHIFT || op == ATOK_RSHIFT
		|| op == ATOK_BAND || op == ATOK_BXOR || op == ATOK_BOR);
}

/* Division and modulo share two UB guards that C doesn't give for free.
   First, divide-by-zero: set p->error (unless we're in the dead branch of a
   ternary, flagged by no_side_effects). Second, LLONG_MIN / -1 overflows in
   signed arithmetic; bash returns LLONG_MIN for / and 0 for % in that case,
   so we mirror that. */
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

/* Apply a binary operator to two already-evaluated operands. All the pure
   arithmetic lives here (the only tricky delegation is to apply_divmod for
   the UB-prone cases). Bitwise ops just use C operators directly on
   long long -- POSIX allows it, and on the LP64 targets this shell runs on
   that is exactly the right width. */
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

/* Handle every operator that immediately follows a variable name: post-++/--
   (return old value, update), plain =, and compound-assign (delegated to
   try_compound_assign). If none of those match the variable is just read.
   Note that the token has already been consumed by arith_parse_primary before
   we get here, so `tok` is a snapshot -- we only peek past it. */
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

/* Primary -- the bottom rung of the precedence hierarchy. Three cases: a
   numeric literal (just return its value), a variable name (delegate to
   primary_var which handles all the assignment/increment variants), or a
   parenthesized sub-expression that recursively calls arith_parse_expr.
   Anything else is a syntax error: we set p->error and return 0, which
   propagates up without crashing. */
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
