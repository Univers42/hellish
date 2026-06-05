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

/* Free a token-array cache (safe on NULL). */
void	arith_cache_free(t_arith_cache *c)
{
	if (!c)
		return ;
	free(c->toks);
	free(c);
}

/* Advance the lexer while it replays a cached token array. `pos` is the index
   of the NEXT token, mirroring the scanning lexer's "pos = end of current" so
   the parser's save/restore of pos+current backtracks one token correctly. */
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

/* Initialise a lexer to replay `toks` instead of scanning a string. */
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
