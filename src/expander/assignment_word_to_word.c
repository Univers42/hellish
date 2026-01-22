/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   assignment_word_to_word.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:26:49 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:26:49 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

void	assignment_word_to_word(t_ast_node *node)
{
	t_ast_node	ret;
	t_ast_node	left;
	t_ast_node	right;

	ret = (t_ast_node){.node_type = AST_WORD};
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	ft_assert(node->node_type == AST_ASSIGNMENT_WORD);
	ft_assert(node->children.len == 2);
	left = ((t_ast_node *)node->children.ctx)[0];
	right = ((t_ast_node *)node->children.ctx)[1];
	left.token.len++;
	vec_push(&ret.children, &left);
	vec_push(&ret.children, &right);
	free(node->children.ctx);
	*node = ret;
}
