/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   eval_cached.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/05 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/* Lex `expr` fully into a malloc'd token array (incl. the terminal EOF/ERROR).
   *ntoks receives the count. Returns NULL on allocation failure. The tokens'
   var_name/start pointers reference `expr`, which must outlive the cache. */
static t_arith_token	*lex_all(const char *expr, int len, int *ntoks)
{
	t_arith_lexer	lex;
	t_vec			v;

	arith_lexer_init(&lex, expr, len);
	vec_init(&v);
	v.elem_size = sizeof(t_arith_token);
	while (1)
	{
		if (!vec_push(&v, &lex.current))
			return (xfree(v.ctx), NULL);
		if (lex.current.type == ATOK_EOF || lex.current.type == ATOK_ERROR)
			break ;
		arith_lexer_advance(&lex);
	}
	*ntoks = (int)v.len;
	return ((t_arith_token *)v.ctx);
}

/* (Re)build the cache for `expr`; leaves *cachep NULL on failure so the caller
   transparently falls back to uncached evaluation. */
static void	cache_build(t_arith_cache **cachep, const char *expr, int len)
{
	t_arith_cache	*c;
	t_arith_token	*toks;
	int				ntoks;

	arith_cache_free(*cachep);
	*cachep = NULL;
	ntoks = 0;
	toks = lex_all(expr, len, &ntoks);
	if (!toks)
		return ;
	c = xmalloc(sizeof(t_arith_cache));
	if (!c)
	{
		xfree(toks);
		return ;
	}
	c->toks = toks;
	c->ntoks = ntoks;
	c->src = expr;
	c->srclen = len;
	*cachep = c;
}

/* Evaluate from the cached token array (no re-lexing). Variables resolve live
   inside arith_run, so loop bodies see updated values. */
long long	arith_eval_cached(t_shell *state, t_arith_cache *c, bool *error)
{
	t_arith_lexer	lexer;

	arith_lexer_init_toks(&lexer, c->toks, c->ntoks);
	return (arith_run(state, &lexer, error));
}

/* Expand a pure $((expr)) using the per-token cache: lex once, re-evaluate
   many times. Falls back to uncached arith_expand if the cache can't build. */
char	*arith_expand_cached(t_shell *state, const char *expr, int len,
			t_arith_cache **cachep)
{
	long long	result;
	bool		error;

	if (!*cachep || (*cachep)->src != expr || (*cachep)->srclen != len)
		cache_build(cachep, expr, len);
	if (!*cachep)
		return (arith_expand(state, expr, len));
	result = arith_eval_cached(state, *cachep, &error);
	if (error)
		return (arith_fail(state, expr, len));
	return (arith_lltoa(result));
}
