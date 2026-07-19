/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_word3.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:41:16 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:41:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

char	*arith_expand(t_shell *state, const char *expr, int len);
char	*arith_expand_cached(t_shell *state, const char *expr, int len,
			t_arith_cache **cachep);

/* Append one token to `out` for the keep-as-one fast path.  Plain literal
   TT_WORD / TT_DQWORD text is appended directly; TT_ENVVAR / TT_DQENVVAR
   with a simple name is looked up and appended.  Returns 1 on success, 0
   when the token is something we cannot handle here (complex ${}, $@, etc.)
   and the caller must fall through to the full slow path.  An unset var
   under `set -u` also bails to the slow path: expand_token owns the
   nounset_abort diagnostics, and skipping it here silently is how
   `x=$unset` used to dodge nounset entirely. */
static int	concat_one_token(t_shell *state, t_string *out, t_token *t)
{
	char	*v;

	if (t->tt == TT_WORD || t->tt == TT_DQWORD)
	{
		if (!is_plain_literal_text(t->start, t->len))
			return (0);
		vec_push_nstr(out, t->start, t->len);
	}
	else if ((t->tt == TT_ENVVAR || t->tt == TT_DQENVVAR)
		&& name_is_plain(t->start, t->len))
	{
		v = env_expand_n(state, t->start, t->len);
		if (!v && state->opt_nounset)
			return (0);
		if (v)
			vec_push_str(out, v);
	}
	else
		return (0);
	return (1);
}

/* Fast path for multi-piece words like "prefix$VAR/suffix": if every child
   token is either plain literal text or a simple $name, concatenate them
   directly without a clone or the full pipeline.  Returns NULL the moment
   anything unsupported appears so the caller falls back gracefully. */
char	*try_simple_concat(t_shell *state, t_ast_node *node)
{
	t_string	out;
	t_token		*t;
	int			i;

	if (node->children.len < 2)
		return (NULL);
	vec_init(&out);
	out.elem_size = 1;
	i = -1;
	while (++i < (int)node->children.len)
	{
		if (((t_ast_node *)node->children.ctx)[i].node_type != AST_TOKEN)
			return (xfree(out.ctx), NULL);
		t = &((t_ast_node *)node->children.ctx)[i].token;
		if (!concat_one_token(state, &out, t))
			return (xfree(out.ctx), NULL);
	}
	if (!out.ctx)
		return (ft_strdup(""));
	return ((char *)out.ctx);
}

/* Return the unique non-empty TT_WORD child of `node`, or NULL.  Unlike
   lone_nonempty_token, this only accepts TT_WORD — TT_ENVVAR etc. return
   NULL immediately so try_pure_arith doesn't misidentify a $((…))-like
   token stored in a variable. */
static t_token	*lone_word_token(t_ast_node *node)
{
	t_token		*t;
	t_ast_node	*c;
	int			i;

	t = NULL;
	i = 0;
	while (i < (int)node->children.len)
	{
		c = &((t_ast_node *)node->children.ctx)[i++];
		if (c->node_type != AST_TOKEN || c->token.tt != TT_WORD)
			return (NULL);
		if (c->token.len == 0)
			continue ;
		if (t)
			return (NULL);
		t = &c->token;
	}
	return (t);
}

/* Fast path for a word that is just $((expr)) with no nested $ or backtick.
   No variable substitution needed — we can hand the raw expression bytes
   straight to arith_expand.  A cached result is reused if the arithmetic
   token has already been evaluated this session (t->arith_cache). */
char	*try_pure_arith(t_shell *state, t_ast_node *node)
{
	t_token	*t;
	int		i;

	t = lone_word_token(node);
	if (!t || t->len < 5 || t->start[0] != '$' || t->start[1] != '('
		|| t->start[2] != '(' || t->start[t->len - 1] != ')'
		|| t->start[t->len - 2] != ')')
		return (NULL);
	i = 3;
	while (i < t->len - 2)
	{
		if (t->start[i] == '$' || t->start[i] == '`')
			return (NULL);
		i++;
	}
	return (arith_expand_cached(state, t->start + 3, t->len - 5,
			&t->arith_cache));
}
