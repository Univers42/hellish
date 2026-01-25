/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_expand.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 11:08:48 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/25 20:41:46 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

static int	handle_dash_literal(t_bracket_ctx *ctx)
{
	if (ctx->start[ctx->i] == '-' && (ctx->i == 0 || ctx->i == ctx->len - 1))
	{
		ctx->buf[ctx->buf_pos++] = '-';
		ctx->i++;
		return (1);
	}
	return (0);
}

static bool	is_valid_range_pattern(t_bracket_ctx *ctx)
{
	return (
		ctx->i + 2 < ctx->len
		&& peek_bracket(ctx, 1) == '-'
		&& peek_bracket(ctx, 2) != ']'
		&& peek_bracket(ctx, 2) != '[');
}

static int	handle_range(t_bracket_ctx *ctx)
{
	int	range_count;

	if (is_valid_range_pattern(ctx))
	{
		if (peek_bracket(ctx, 0) == '\\')
			return (consume_range(ctx, 1, 2),
				set_last_elem(ctx->buf, ctx->buf_pos, '-'), 1);
		if (peek_bracket(ctx, 2) == '\\' && ctx->i + 3 < ctx->len)
		{
			range_count = expand_range(peek_bracket(ctx, 0),
					peek_bracket(ctx, 3), ctx->buf, ctx->buf_pos);
			if (range_count < 0)
				return (handle_invalid_range(ctx));
			return (consume_range(ctx, range_count, 4), 1);
		}
		range_count = expand_range(peek_bracket(ctx, 0),
				peek_bracket(ctx, 2), ctx->buf, ctx->buf_pos);
		if (range_count < 0)
			return (handle_invalid_range(ctx));
		return (consume_range(ctx, range_count, 3), 1);
	}
	return (0);
}

static void	expand_bracket_content(t_bracket_ctx *ctx)
{
	while (ctx->i < ctx->len && ctx->buf_pos < 1000)
	{
		if (handle_backslash(ctx))
			continue ;
		if (handle_posix_class(ctx))
			continue ;
		if (handle_dash_literal(ctx))
			continue ;
		if (handle_range(ctx))
			continue ;
		ctx->buf[ctx->buf_pos++] = ctx->start[ctx->i++];
	}
}

char	*glob_expand_bracket(const char *start, int len, int *out_len)
{
	t_bracket_ctx	ctx;

	if (len <= 0)
	{
		*out_len = 0;
		return (NULL);
	}
	if (is_leading_bracket_special(start, len))
		return (expand_leading_bracket_special(start, out_len));
	ctx.start = start;
	ctx.len = len;
	ctx.buf = malloc(1024);
	if (!ctx.buf)
		return (NULL);
	ctx.buf_pos = 0;
	ctx.i = 0;
	(handle_leading_bracket(&ctx), expand_bracket_content(&ctx));
	ctx.buf[ctx.buf_pos] = '\0';
	*out_len = ctx.buf_pos;
	return (ctx.buf);
}
