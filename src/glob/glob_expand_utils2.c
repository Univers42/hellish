/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_expand_utils2.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 11:51:44 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 11:58:58 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

int	handle_invalid_range(t_bracket_ctx *ctx)
{
	ctx->buf[ctx->buf_pos++] = ctx->start[ctx->i];
	ctx->buf[ctx->buf_pos++] = '-';
	ctx->i += 2;
	return (1);
}

void	consume_range(t_bracket_ctx *ctx, int range_count, int i_delta)
{
	ctx->buf_pos += range_count;
	ctx->i += i_delta;
}

// Generic utility: set the last element of an array to a value
void	set_last_elem(char *arr, int len, char val)
{
	if (len > 0)
		arr[len - 1] = val;
}

char	peek_elem(const void *arr, int base, int offset, size_t elem_size)
{
	return (((const char *)arr)[(base + offset) * elem_size]);
}

char	peek_bracket(const t_bracket_ctx *ctx, int offset)
{
	return (peek_elem(ctx->start, ctx->i, offset, sizeof(char)));
}
