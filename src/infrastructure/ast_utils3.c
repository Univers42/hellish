/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast_utils3.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 16:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 17:12:27 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ast_private.h"

char	*node_name_compound(t_ast_type tn)
{
	if (tn == AST_IF)
		return ("AST_IF");
	if (tn == AST_WHILE)
		return ("AST_WHILE");
	if (tn == AST_UNTIL)
		return ("AST_UNTIL");
	if (tn == AST_FOR)
		return ("AST_FOR");
	if (tn == AST_CASE)
		return ("AST_CASE");
	if (tn == AST_CASE_ITEM)
		return ("AST_CASE_ITEM");
	if (tn == AST_BRACE_GROUP)
		return ("AST_BRACE_GROUP");
	if (tn == AST_FUNCTION_DEF)
		return ("AST_FUNCTION_DEF");
	ft_assert(0);
	return (0);
}

/* Helper: print all children of a node recursively */
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

/* Helper: print a collapsed pipeline node (single child) */
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

/* Main recursive print function */
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
