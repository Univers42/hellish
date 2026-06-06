/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser2.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:47:01 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Evaluate arith_parse_expr with no_side_effects forced on. Used to silently
   consume the dead branch of a ternary or the false side of && / || without
   letting assignments, ++ or -- affect shell variables. The old flag is saved
   and restored so nested calls don't get confused. */
static long long	parse_expr_nse(t_arith_parser *p)
{
	bool		old;
	long long	val;

	old = p->no_side_effects;
	p->no_side_effects = true;
	val = arith_parse_expr(p);
	p->no_side_effects = old;
	return (val);
}

/* Same idea as parse_expr_nse but for arith_parse_ternary -- used when the
   ternary is nested in the dead branch of an outer ternary and must be
   consumed without side effects. */
static long long	parse_ternary_nse(t_arith_parser *p)
{
	bool		old;
	long long	val;

	old = p->no_side_effects;
	p->no_side_effects = true;
	val = arith_parse_ternary(p);
	p->no_side_effects = old;
	return (val);
}

/* Evaluate the true branch of a ternary (cond was non-zero). Parses the
   then-expression with full side effects, then silently consumes the ':' and
   the else-expression (no_side_effects = true) so both branches are
   syntactically consumed regardless of which is live. */
static long long	ternary_true(t_arith_parser *p)
{
	long long		then_val;
	t_arith_token	tok;

	then_val = arith_parse_expr(p);
	tok = arith_lexer_peek(p->lexer);
	if (tok.type != ATOK_TERNC)
	{
		p->error = true;
		return (0);
	}
	arith_lexer_advance(p->lexer);
	(void)parse_ternary_nse(p);
	return (then_val);
}

/* Ternary: or ('?' expr ':' ternary)? -- right-associative, so the else
   branch recurses on arith_parse_ternary rather than arith_parse_expr. The
   key invariant is that BOTH branches are always parsed: the dead one with
   no_side_effects forced so assignments don't fire. That matches bash and
   POSIX and avoids the "one branch not validated" pitfall. */
long long	arith_parse_ternary(t_arith_parser *p)
{
	long long		cond;
	long long		else_val;
	t_arith_token	tok;

	cond = arith_parse_or(p);
	tok = arith_lexer_peek(p->lexer);
	if (tok.type != ATOK_TERNQ)
		return (cond);
	arith_lexer_advance(p->lexer);
	if (cond != 0)
		return (ternary_true(p));
	(void)parse_expr_nse(p);
	tok = arith_lexer_peek(p->lexer);
	if (tok.type != ATOK_TERNC)
	{
		p->error = true;
		return (0);
	}
	arith_lexer_advance(p->lexer);
	else_val = arith_parse_ternary(p);
	return (else_val);
}

/* Top-level expression: ternary (',' ternary)*. The comma operator evaluates
   every operand left-to-right for side effects and returns the last value,
   exactly like C. It's the lowest-precedence operator in $((...)), sitting
   below even ternary and assignment. */
long long	arith_parse_expr(t_arith_parser *p)
{
	long long		val;
	t_arith_token	tok;

	val = arith_parse_ternary(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_COMMA)
		{
			arith_lexer_advance(p->lexer);
			val = arith_parse_ternary(p);
		}
		else
			break ;
	}
	return (val);
}
