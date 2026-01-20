/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast_utils3.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 16:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 17:00:11 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ast_private.h"

// Helper: print all children of a node recursively
static void	print_all_children(t_ast_node node, int *depth_stack, int depth)
{
	size_t	i;
	int		is_last;

	i = 0;
	while (i < node.children.len)
	{
		is_last = (i == node.children.len - 1);
		if (depth_stack)
			depth_stack[depth] = is_last;
		print_tree_recursive(*(t_ast_node *)vec_idx(&node.children, i),
			depth_stack, depth + 1);
		i++;
	}
}

// Helper: print a collapsed pipeline node (single child)
static void	print_collapsed_pipeline(t_ast_node node,
									int *depth_stack,
									int depth)
{
	t_ast_node	cchild;
	size_t		i;
	int			is_last;

	cchild = *(t_ast_node *)vec_idx(&node.children, 0);
	print_node_line(cchild);
	i = 0;
	while (i < cchild.children.len)
	{
		is_last = (i == cchild.children.len - 1);
		if (depth_stack)
			depth_stack[depth] = is_last;
		print_tree_recursive(*(t_ast_node *)vec_idx(&cchild.children, i),
			depth_stack, depth + 1);
		i++;
	}
}

/* Helper: print a left-associative list node */
static void	print_left_associative_list(t_ast_node node,
										int *depth_stack,
										int depth)
{
	t_print_seq_ctx	ctx;

	print_node_line(node);
	ctx.children = (t_ast_node *)node.children.ctx;
	ctx.depth_stack = depth_stack;
	ctx.depth = depth + 1;
	if (depth_stack)
		depth_stack[depth] = 1;
	print_sequence_range_ctx(&ctx, 0, (int)node.children.len - 1);
}

// Main recursive print function
void	print_tree_recursive(t_ast_node node, int *depth_stack, int depth)
{
	print_tree_prefix(depth_stack, depth,
		depth > 0 && depth_stack[depth - 1]);
	if (node.node_type == AST_COMMAND_PIPELINE && node.children.len == 1)
	{
		print_collapsed_pipeline(node, depth_stack, depth);
		return ;
	}
	if ((node.node_type == AST_SIMPLE_LIST
			|| node.node_type == AST_COMPOUND_LIST) && node.children.len > 1)
	{
		print_left_associative_list(node, depth_stack, depth);
		return ;
	}
	print_node_line(node);
	print_all_children(node, depth_stack, depth);
}
