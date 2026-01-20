/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 13:47:25 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 13:48:25 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Set a simple single-character operator and advance position */
void	set_simple_op(t_arith_lexer *lex, int type)
{
	lex->current.type = type;
	lex->pos++;
}

void	set_lex_error(t_arith_lexer *lex)
{
	lex->current.type = ATOK_ERROR;
	lex->error = true;
	lex->pos++;
}
