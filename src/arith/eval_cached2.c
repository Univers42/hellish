/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   eval_cached2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/05 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Free a token-array cache, including the token array it owns. Safe to call
   on NULL so callers don't have to guard. The t_arith_cache itself is also
   freed (it was malloc'd by cache_build). */
void	arith_cache_free(t_arith_cache *c)
{
	if (!c)
		return ;
	xfree(c->toks);
	xfree(c);
}

/* Step the lexer forward one token in cached-replay mode. We keep `pos` as
   the index of the NEXT token to load -- mirroring the scanning lexer where
   pos is the byte offset *after* the current token. That way try_compound_
   assign's save/restore of (pos, current) backtracks by exactly one token in
   both modes without needing separate logic. */
void	arith_advance_toks(t_arith_lexer *lex)
{
	if (lex->pos < lex->ntoks)
	{
		lex->current = lex->toks[lex->pos];
		lex->pos++;
	}
	else
	{
		lex->current.type = ATOK_EOF;
		lex->current.len = 0;
	}
}

/* Initialize the lexer in cached-replay mode: point it at a pre-built token
   array instead of a raw string. The first call to arith_advance_toks (via
   the final line here) loads token[0] into lex->current so the parser sees
   it immediately via arith_lexer_peek -- same invariant as the scanning init.
   lex->input is left NULL to crash loudly if scanning code accidentally runs
   on a replay lexer. */
void	arith_lexer_init_toks(t_arith_lexer *lex, const t_arith_token *toks,
			int ntoks)
{
	lex->input = NULL;
	lex->pos = 0;
	lex->len = 0;
	lex->error = false;
	lex->error_msg = NULL;
	lex->toks = toks;
	lex->ntoks = ntoks;
	lex->current.type = ATOK_EOF;
	arith_advance_toks(lex);
}
