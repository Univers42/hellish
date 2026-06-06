/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast_utils4.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 16:20:26 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 16:58:23 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ast_private.h"

/* Print the child at `idx` from ctx->children, marking it as the last child
   at ctx->depth so the connector prints "└── " rather than "├── ". */
static void	print_single_child(t_print_seq_ctx *ctx, int idx)
{
	if (ctx->depth_stack)
		ctx->depth_stack[ctx->depth] = 1;
	print_tree_recursive(ctx->children[idx], ctx->depth_stack, ctx->depth);
}

/* When no operator token is found in [start, end], fall back to printing
   the children in order without special junction rendering. Covers list
   nodes that contain only commands and no visible operator token. */
static void	print_children_linear(t_print_seq_ctx *ctx, int start, int end)
{
	int	i;

	i = start;
	while (i <= end)
	{
		if (ctx->depth_stack)
			ctx->depth_stack[ctx->depth] = (i == end);
		print_tree_recursive(ctx->children[i],
			ctx->depth_stack, ctx->depth);
		i++;
	}
}

/* Emit the operator node (e.g. "OP &&") at ctx->depth, then recurse into the
   left subtree [start .. op_idx-1] (one level deeper) and the right sibling
   (the node immediately after the operator, at the same depth). This models
   the left-associative parse tree structure: "a && b && c" is "(a && b) && c".
*/
static void	print_op_and_left_right(t_print_seq_ctx *ctx,
									int start,
									int op_idx,
									int end)
{
	t_print_seq_ctx	left_ctx;

	print_tree_prefix(ctx->depth_stack, ctx->depth,
		ctx->depth > 0 && ctx->depth_stack[ctx->depth - 1]);
	print_op_line(ctx->children[op_idx]);
	if (ctx->depth_stack)
		ctx->depth_stack[ctx->depth] = 0;
	left_ctx = *ctx;
	left_ctx.depth = ctx->depth + 1;
	print_sequence_range_ctx(&left_ctx, start, op_idx - 1);
	if (op_idx + 1 <= end)
	{
		if (ctx->depth_stack)
			ctx->depth_stack[ctx->depth] = 1;
		print_tree_recursive(ctx->children[op_idx + 1],
			ctx->depth_stack, ctx->depth + 1);
	}
}

/* Render children [start .. end] of a list node. We scan from the right
   looking for the last AST_TOKEN (the operator); if found, split into the
   left subtree and the right operand with the operator as a junction; if not
   found, fall back to linear printing. The right-to-left scan preserves the
   left-associative shape: we always split at the rightmost operator first. */
void	print_sequence_range_ctx(t_print_seq_ctx *ctx,
								int start,
								int end)
{
	int	op_idx;

	if (start > end || start < 0)
		return ;
	if (start == end)
	{
		print_single_child(ctx, start);
		return ;
	}
	op_idx = end;
	while (op_idx > start && ctx->children[op_idx].node_type != AST_TOKEN)
		op_idx--;
	if (op_idx <= start || ctx->children[op_idx].node_type != AST_TOKEN)
	{
		print_children_linear(ctx, start, end);
		return ;
	}
	print_op_and_left_right(ctx, start, op_idx, end);
}
