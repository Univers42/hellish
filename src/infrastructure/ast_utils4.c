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

/* Print a single child at index using ctx */
static void	print_single_child(t_print_seq_ctx *ctx, int idx)
{
	if (ctx->depth_stack)
		ctx->depth_stack[ctx->depth] = 1;
	print_tree_recursive(ctx->children[idx], ctx->depth_stack, ctx->depth);
}

/* Print children linearly from start to end using ctx */
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

/* Print operator node and recurse on left/right using ctx */
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
