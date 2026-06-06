/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tree_utils.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:55 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:34:55 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ast_private.h"

/* Pretty-print the AST as a box-tree to stdout (--debug-parser mode).
   The display uses the box-drawing characters ├─ / └─ / │ familiar from
   `tree(1)` to show the parent-child relationships. A fixed 256-slot
   depth_stack tracks which columns still have siblings pending below. */
void	print_ast_tree(t_ast_node node)
{
	int	depth_stack[256];
	int	i;

	i = 0;
	while (i < 256)
		depth_stack[i++] = 0;
	ft_printf("\n\033[1;35m=== AST Tree Structure ===\033[0m\n\n");
	print_tree_recursive(node, depth_stack, 0);
	ft_printf("\n\033[1;35m=========================\033[0m\n\n");
}

/* Emit the indentation + connector for one tree node. depth_stack[i] is 1
   when level i is the last child (print "    " instead of "│   "), giving the
   right "L-shaped" junction at that level. The is_last flag selects "└── "
   vs "├── " for the connector of the current node. */
void	print_tree_prefix(int *depth_stack, int depth, int is_last)
{
	int	i;

	i = 0;
	while (i < depth - 1)
	{
		if (depth_stack[i])
			ft_printf("    ");
		else
			ft_printf("│   ");
		i++;
	}
	if (depth > 0)
	{
		if (is_last)
			ft_printf("└── ");
		else
			ft_printf("├── ");
	}
}

/* Emit the coloured node type name, and for AST_TOKEN nodes, also the token
   type string and the raw text span. The child count is appended in grey as
   a quick sanity check during debugging. */
void	print_node_line(t_ast_node node)
{
	ft_printf("\033[1;36m%s\033[0m", node_name(node.node_type));
	if (node.node_type == AST_TOKEN)
	{
		ft_printf(" \033[33m%s\033[0m: '\033[32m%.*s\033[0m'",
			tt_to_str(node.token.tt), node.token.len, node.token.start);
	}
	ft_printf(" \033[90m[%d children]\033[0m\n", (int)node.children.len);
}

/* Emit the operator node line (e.g. "OP &&") for a binary list/pipeline.
   These are rendered inline at the junction, not as a child, so the tree
   reads left → op → right naturally. */
void	print_op_line(t_ast_node op_node)
{
	ft_printf("\033[1;33mOP %s\033[0m", tt_to_str(op_node.token.tt));
	if (op_node.node_type == AST_TOKEN)
		ft_printf(" \033[90m'%.*s'\033[0m",
			op_node.token.len, op_node.token.start);
	ft_printf("\n");
}
