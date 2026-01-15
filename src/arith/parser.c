/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/15 15:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith.h"
#include "shell.h"
#include "env.h"
#include "libft.h"

static long long	get_var_value(t_arith_parser *p, const char *name, int len)
{
	char	*val;
	char	*key;
	int		result;

	key = ft_strndup(name, len);
	if (!key)
		return (0);
	val = env_expand_n(p->shell, key, len);
	free(key);
	if (!val || !*val)
		return (0);
	if (ft_checked_atoi(val, &result, 42) != 0)
		return (0);
	return ((long long)result);
}

static void	expect(t_arith_parser *p, t_arith_tok type)
{
	if (p->lexer->current.type != type)
	{
		p->error = true;
		return;
	}
	arith_lexer_advance(p->lexer);
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
	{
		arith_lexer_advance(p->lexer);
		return (arith_parse_unary(p));
	}
	else if (tok.type == ATOK_MINUS)
	{
		arith_lexer_advance(p->lexer);
		return (-arith_parse_unary(p));
	}
	else if (tok.type == ATOK_NOT)
	{
		arith_lexer_advance(p->lexer);
		return (!arith_parse_unary(p));
	}
	else if (tok.type == ATOK_BNOT)
	{
		arith_lexer_advance(p->lexer);
		return (~arith_parse_unary(p));
	}
	return (arith_parse_primary(p));
}

/* Exponent: unary ('**' exponent)? - right associative */
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
		exp = arith_parse_exponent(p); /* right associative: recurse */
		if (p->error)
			return (0);
		/* Calculate base ** exp */
		if (exp < 0)
			return (0); /* negative exponents not supported in integer arith */
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

/* Multiplicative: exponent (('*' | '/' | '%') exponent)* */
long long	arith_parse_multiplicative(t_arith_parser *p)
{
	long long		left;
	long long		right;
	t_arith_token	tok;

	left = arith_parse_exponent(p);  /* Changed from arith_parse_unary */
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_MUL)
		{
			arith_lexer_advance(p->lexer);
			left = left * arith_parse_exponent(p);  /* Changed */
		}
		else if (tok.type == ATOK_DIV)
		{
			arith_lexer_advance(p->lexer);
			right = arith_parse_exponent(p);  /* Changed */
			if (right == 0)
				{ p->error = true; return (0); }
			left = left / right;
		}
		else if (tok.type == ATOK_MOD)
		{
			arith_lexer_advance(p->lexer);
			right = arith_parse_exponent(p);  /* Changed */
			if (right == 0)
				{ p->error = true; return (0); }
			left = left % right;
		}
		else
			break;
	}
	return (left);
}

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
			break;
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
			break;
	}
	return (left);
}

/* Relational: shift (('<' | '<=' | '>' | '>=') shift)* */
long long	arith_parse_relational(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_shift(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_LT)
		{
			arith_lexer_advance(p->lexer);
			left = left < arith_parse_shift(p);
		}
		else if (tok.type == ATOK_LE)
		{
			arith_lexer_advance(p->lexer);
			left = left <= arith_parse_shift(p);
		}
		else if (tok.type == ATOK_GT)
		{
			arith_lexer_advance(p->lexer);
			left = left > arith_parse_shift(p);
		}
		else if (tok.type == ATOK_GE)
		{
			arith_lexer_advance(p->lexer);
			left = left >= arith_parse_shift(p);
		}
		else
			break;
	}
	return (left);
}

/* Equality: relational (('==' | '!=') relational)* */
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
			break;
	}
	return (left);
}
