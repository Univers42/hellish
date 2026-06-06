/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers9.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:15:22 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:46:58 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Assert that the current token matches `type` and advance past it. If the
   token doesn't match, set p->error and return without advancing -- the error
   will propagate upward through every caller that guards on p->error. Used
   mainly to consume the mandatory ')' after a parenthesized sub-expression. */
void	expect(t_arith_parser *p, t_arith_tok type)
{
	if (p->lexer->current.type != type)
	{
		p->error = true;
		return ;
	}
	arith_lexer_advance(p->lexer);
}
