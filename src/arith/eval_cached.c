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
#include "parena.h"

/* Lex the entire expression into a freshly-malloc'd token array and return it.
   The array includes the terminating EOF or ERROR token. var_name and start
   fields in the tokens point into `expr`, so the caller must keep expr alive
   for as long as the cache lives. Returns NULL on alloc failure (the caller
   transparently falls back to uncached evaluation). */
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

/* Build or rebuild the token-array cache for `expr`. Always frees the old
   cache first so we don't leak. On any allocation failure *cachep is left
   NULL and the caller falls back to plain arith_expand -- graceful
   degradation, not a crash. */
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
	parena_note_attach();
}

/* Replay a cached token array through arith_run. The lexer is initialized in
   token-replay mode (arith_lexer_init_toks), so no byte-scanning happens.
   Variable lookups inside arith_run still hit the live environment, which
   means loop bodies like `while (( i++ < 10 ))` see the updated value of `i`
   on every iteration despite the tokens being pre-baked. */
long long	arith_eval_cached(t_shell *state, t_arith_cache *c, bool *error)
{
	t_arith_lexer	lexer;

	arith_lexer_init_toks(&lexer, c->toks, c->ntoks);
	return (arith_run(state, &lexer, error));
}

/* Cached $((...)) expansion: lex once, evaluate many times. The cache is
   keyed by (pointer identity, length) -- if the caller passes the same
   persistent source string every call (e.g. an AST node's byte slice), the
   lex work is done only once and subsequent iterations pay only for the
   parse+evaluate pass. Falls back to arith_expand if the cache fails to
   build or becomes stale. */
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
