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
	memset(ptr, 0, size);
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

// node -> split node
t_vec_nd	split_words(t_shell *state, t_ast_node *node)
{
	t_vec_nd	ret;
	t_token		*curr_t;
	t_ast_node	curr_node;
	int			i;

	vec_init(&ret);
	ret.elem_size = sizeof(t_ast_node);
	init_word_node(&curr_node);
	i = -1;
	while (++i < (int)node->children.len)
	{
		ft_assert(((t_ast_node *)node->children.ctx)[i].node_type == AST_TOKEN);
		curr_t = &((t_ast_node *)node->children.ctx)[i].token;
		if (curr_t->tt == TT_ENVVAR)
			split_envvar(state, curr_t, &curr_node, &ret);
		else if (curr_t->tt == TT_WORD || curr_t->tt == TT_SQWORD
			|| curr_t->tt == TT_DQWORD || curr_t->tt == TT_DQENVVAR)
			push_token_node(&curr_node, &((t_ast_node *)node->children.ctx)[i]);
		else
			ft_assert("Unreachable" == 0);
	}
	if (curr_node.children.len)
		vec_push(&ret, &curr_node);
	return (ft_reset(node, sizeof(t_ast_node), free_children), ret);
}
