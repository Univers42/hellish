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

/* Emit a single-character operator token and advance lex->pos by one.
   The caller has already set lex->current.start and len=1, so we only need
   to write the type and bump the cursor. Used by every one-char operator
   that doesn't need the two-char peeking logic. */
void	set_simple_op(t_arith_lexer *lex, int type)
{
	lex->current.type = type;
	lex->pos++;
}

/* Record a lexer error and advance past the offending character to avoid an
   infinite loop. lex->error will be checked by arith_run after the parse is
   complete and cause the whole expression to fail. */
void	set_lex_error(t_arith_lexer *lex)
{
	lex->current.type = ATOK_ERROR;
	lex->error = true;
	lex->pos++;
}
