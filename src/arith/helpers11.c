/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers11.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 13:22:10 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:47:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

static long long	parse_bitor_nse(t_arith_parser *p)
{
	bool		old;
	long long	val;

	old = p->no_side_effects;
	p->no_side_effects = true;
	val = arith_parse_bitor(p);
	p->no_side_effects = old;
	return (val);
}

static long long	parse_and_nse(t_arith_parser *p)
{
	bool		old;
	long long	val;

	old = p->no_side_effects;
	p->no_side_effects = true;
	val = arith_parse_and(p);
	p->no_side_effects = old;
	return (val);
}

/* And: bitor ('&&' bitor)* */
long long	arith_parse_and(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_bitor(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_AND)
		{
			arith_lexer_advance(p->lexer);
			if (left == 0)
				(void)parse_bitor_nse(p);
			else
				left = (arith_parse_bitor(p) != 0);
		}
		else
			break ;
	}
	return (left);
}

/* Or: and ('||' and)* */
long long	arith_parse_or(t_arith_parser *p)
{
	long long		left;
	t_arith_token	tok;

	left = arith_parse_and(p);
	while (!p->error)
	{
		tok = arith_lexer_peek(p->lexer);
		if (tok.type == ATOK_OR)
		{
			arith_lexer_advance(p->lexer);
			if (left != 0)
			{
				left = 1;
				(void)parse_and_nse(p);
			}
			else
				left = (arith_parse_and(p) != 0);
		}
		else
			break ;
	}
	return (left);
}
