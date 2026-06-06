/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers10.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 13:18:44 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:25:02 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* '>' can start three different tokens: '>=' (ATOK_GE), '>>' (ATOK_RSHIFT),
   or plain '>' (ATOK_GT). This is the symmetric counterpart to
   handle_angle_left in helpers2.c; it lives in a separate file because the
   norm caps file length and helpers2.c was already full. */
void	handle_angle_right(t_arith_lexer *lex)
{
	if (lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == '=')
	{
		lex->current.type = ATOK_GE;
		lex->current.len = 2;
		lex->pos += 2;
	}
	else if (lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == '>')
	{
		lex->current.type = ATOK_RSHIFT;
		lex->current.len = 2;
		lex->pos += 2;
	}
	else
		set_simple_op(lex, ATOK_GT);
}
