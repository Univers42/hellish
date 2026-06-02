/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander_split.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:26 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:26 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

static void	init_word_node(t_ast_node *n)
{
	*n = (t_ast_node){.node_type = AST_WORD};
	vec_init(&n->children);
	n->children.elem_size = sizeof(t_ast_node);
}

static void	push_token_node(t_ast_node *curr_node, t_ast_node *child)
{
	vec_push(&curr_node->children, child);
}

static void	ft_reset(void *ptr, size_t size, void (*cust_act_bef_reset)(void *))
{
	if (cust_act_bef_reset)
		cust_act_bef_reset(ptr);
	ft_memset(ptr, 0, size);
}

/* free children.ctx pointer of an ast node prior to zeroing the node */
static void	free_children(void *p)
{
	t_ast_node	*n;

	if (!p)
		return ;
	n = (t_ast_node *)p;
	if (n->children.ctx)
		free(n->children.ctx);
}

/* Release a consumed subtoken's own allocations. split_envvar/emit_positional_at
   build fresh field nodes rather than moving this one into the output, so its
   start (clone-owned) and full_word (malloc'd by clone_ast) would otherwise leak.
   Pushed word tokens are moved instead, so they are never passed here. */
static void	free_token_res(t_token *t)
{
	if (t->allocated)
		free((char *)t->start);
	t->allocated = false;
	if (t->full_word)
	{
		if (t->full_word->allocated)
			free(t->full_word->start);
		free(t->full_word);
		t->full_word = NULL;
	}
}

/* Dispatch one subtoken: envvar/$@ subtokens are split into fresh field nodes
   (and their own allocations released), plain words are moved into curr_node. */
static void	split_one_child(t_shell *state, t_ast_node *child,
				t_ast_node *curr_node, t_vec_nd *ret)
{
	t_token	*curr_t;

	curr_t = &child->token;
	if (curr_t->tt == TT_DQENVVAR && curr_t->len == 1 && curr_t->start[0] == '@')
		(emit_positional_at(state, curr_node, ret), free_token_res(curr_t));
	else if (curr_t->tt == TT_ENVVAR
		|| (curr_t->tt == TT_WORD && curr_t->split_eligible))
		(split_envvar(state, curr_t, curr_node, ret), free_token_res(curr_t));
	else if (curr_t->tt == TT_WORD || curr_t->tt == TT_SQWORD
		|| curr_t->tt == TT_DQWORD || curr_t->tt == TT_DQENVVAR)
		push_token_node(curr_node, child);
	else
		ft_assert(0);
}

// node -> split node
t_vec_nd	split_words(t_shell *state, t_ast_node *node)
{
	t_vec_nd	ret;
	t_ast_node	curr_node;
	int			i;

	vec_init(&ret);
	ret.elem_size = sizeof(t_ast_node);
	init_word_node(&curr_node);
	i = -1;
	while (++i < (int)node->children.len)
	{
		ft_assert(((t_ast_node *)node->children.ctx)[i].node_type == AST_TOKEN);
		split_one_child(state, &((t_ast_node *)node->children.ctx)[i],
			&curr_node, &ret);
	}
	if (curr_node.children.len)
		vec_push(&ret, &curr_node);
	return (ft_reset(node, sizeof(t_ast_node), free_children), ret);
}
