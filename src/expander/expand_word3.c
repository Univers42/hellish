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

/* A piece of a keep-as-one word: plain literal text (unquoted or inside double
   quotes) or a plain $name (likewise). Double-quoted vars expand without field
   splitting, exactly the keep-as-one semantics, so they share this fast path;
   special params ($?, $@, ${...}) fail name_is_plain and take the slow path. */
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
		if (v)
			vec_push_str(out, v);
	}
	else
		return (0);
	return (1);
}

/* A keep-as-one word built only from plain literal and plain $var pieces. */
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

/* The single non-empty TT_WORD child of `node`, or NULL. */
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

/* A word that is exactly $((expr)) with no $ or backtick inside is pure
   arithmetic: evaluate directly. */
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
