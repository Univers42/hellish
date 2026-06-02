/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander_split2.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:26 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:26 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Dispatch one subtoken: envvar/$@ are split into fresh field nodes;
   plain words are moved into curr_node. */
static void	split_one_child(t_shell *state, t_ast_node *child,
				t_ast_node *curr_node, t_vec_nd *ret)
{
	t_token	*curr_t;

	curr_t = &child->token;
	if (curr_t->tt == TT_DQENVVAR && curr_t->len == 1
		&& curr_t->start[0] == '@')
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

/* node -> split node */
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
		ft_assert(((t_ast_node *)node->children.ctx)[i].node_type
			== AST_TOKEN);
		split_one_child(state, &((t_ast_node *)node->children.ctx)[i],
			&curr_node, &ret);
	}
	if (curr_node.children.len)
		vec_push(&ret, &curr_node);
	return (ft_reset(node, sizeof(t_ast_node), free_children), ret);
}
