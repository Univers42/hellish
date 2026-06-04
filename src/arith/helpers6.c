/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers6.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 13:22:10 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:47:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Bitand: equality ('&' equality)* */
long long	arith_parse_bitand(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_equality(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_BAND)
		{
			arith_lexer_advance(p->lexer);
			left = left & arith_parse_equality(p);
		}
		else
			break ;
	}
	return (left);
}

/* Bitxor: bitand ('^' bitand)* */
long long	arith_parse_bitxor(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_bitand(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_BXOR)
		{
			arith_lexer_advance(p->lexer);
			left = left ^ arith_parse_bitand(p);
		}
		else
			break ;
	}
	return (left);
}

/* Bitor: bitxor ('|' bitxor)* */
long long	arith_parse_bitor(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_bitxor(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_BOR)
		{
			arith_lexer_advance(p->lexer);
			left = left | arith_parse_bitxor(p);
		}
		else
			break ;
	}
	return (left);
}
