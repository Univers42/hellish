/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast_utils2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:03 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:03:31 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ast_private.h"

void	ast_postorder_traversal(t_ast_node *node, void (*f)(t_ast_node *node))
{
	size_t	i;

	i = 0;
	while (i < node->children.len)
	{
		ast_postorder_traversal(vec_idx(&node->children, i), f);
		i++;
	}
	f(node);
}

void	free_node(t_ast_node *node)
{
	if (node->node_type == AST_TOKEN && node->token.allocated)
		free(node->token.start);
	free(node->children.ctx);
	*node = (t_ast_node){};
}

void	free_ast(t_ast_node *node)
{
	ast_postorder_traversal(node, free_node);
}

/* Helper: Get color for node type in DOT format */
char	*get_node_color(t_ast_type tn)
{
	if (tn == AST_COMMAND_PIPELINE)
		return ("#FFB6C1");
	if (tn == AST_REDIRECT)
		return ("#87CEEB");
	if (tn == AST_WORD || tn == AST_TOKEN)
		return ("#90EE90");
	if (tn == AST_SIMPLE_LIST || tn == AST_COMPOUND_LIST)
		return ("#DDA0DD");
	if (tn == AST_SIMPLE_COMMAND || tn == AST_COMMAND)
		return ("#F0E68C");
	if (tn == AST_SUBSHELL)
		return ("#FFB347");
	if (tn == AST_ASSIGNMENT_WORD)
		return ("#B0E0E6");
	if (tn == AST_PROC_SUB)
		return ("#98FB98");
	return ("#FFFFFF");
}

/* Helper: Get shape for node type in DOT format */
char	*get_node_shape(t_ast_type tn)
{
	if (tn == AST_TOKEN || tn == AST_WORD)
		return ("ellipse");
	if (tn == AST_COMMAND || tn == AST_SIMPLE_COMMAND)
		return ("box");
	if (tn == AST_SUBSHELL)
		return ("octagon");
	if (tn == AST_REDIRECT)
		return ("diamond");
	return ("box");
}
