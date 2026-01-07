/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 16:09:14 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 16:09:14 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incs/dsa.h"

void	ast_postorder_traversal(t_ast_node *node, void (*fn)(t_ast_node *node))
{
	size_t	i;

	if (!node || !fn)
		return ;
	i = 0;
	while (i < node->children.len)
	{
		ast_postorder_traversal((t_ast_node *)vec_ctx_idx(&node->children, i), fn);
		i++;
	}
	fn(node);
}

void	free_node(t_ast_node *node)
{
	if (!node)
		return ;
	free(node->children.ctx);
	*node = (t_ast_node){0};
}

void	free_ast(t_ast_node *node)
{
	ast_postorder_traversal(node, free_node);
}

/* Helper renamed to avoid C-name collision and match correct signature */
t_ast_node	make_ast_token(t_token t, int offset, int end_offset, t_tt tt)
{
	t_ast_node	ret;

	ret = (t_ast_node){0};
	ret.node_type = AST_TOKEN;
	(void)t; (void)offset; (void)end_offset; (void)tt;
	return (ret);
}
