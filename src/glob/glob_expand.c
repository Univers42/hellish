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

/* A '-' at the very start or very end of a bracket class is a literal dash,
   not a range operator. POSIX: "[-abc]" and "[abc-]" include '-' literally.
   Return 1 (consumed) if we handled it, 0 to let the caller try the range
   logic next. */
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

/* Determine whether the bytes starting at ctx->i form a range expression
   "c-c" (current char, dash, closing char). Guards: there must be at least
   two more bytes, the second must be '-', and the third must not be ']' or '['
   (a dash before the closing bracket is a literal, not a range). */
static bool	is_valid_range_pattern(t_bracket_ctx *ctx)
{
	return (
		ctx->i + 2 < ctx->len
		&& peek_bracket(ctx, 1) == '-'
		&& peek_bracket(ctx, 2) != ']'
		&& peek_bracket(ctx, 2) != '[');
}

/* Expand a range if the current position looks like one. Handles backslash-
   escaped endpoints: if the start char is '\\' we treat it as a literal '-'
   and collapse; if the end char is '\\' we look one byte further for the
   actual endpoint. On an invalid range (start > end) we fall back to treating
   the first char and the dash as literals. Returns 1 on success, 0 if no
   range was found (caller continues with char-by-char logic). */
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

/* Walk the bracket content byte-by-byte, expanding ranges, POSIX classes, and
   backslash escapes into ctx->buf. The 1000-byte cap on buf_pos is a safety
   limit -- a bracket with 1000+ distinct characters after expansion is
   degenerate and we'd rather truncate than overflow. Normal real-world cases
   (e.g. [a-z0-9_]) expand to at most ~63 characters. */
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

/* Expand a raw bracket expression content (the bytes between '[' and ']',
   not including the brackets or the optional '!' negation flag) into a flat
   string of matching characters. The result is malloc'd; *out_len receives its
   length. Returns NULL on allocation failure or if len <= 0. The "leading
   bracket special" case handles the POSIX rule that ']' as the first character
   is literal, not the closing bracket. */
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
	ctx.buf = xmalloc(1024);
	if (!ctx.buf)
		return (NULL);
	ctx.buf_pos = 0;
	ctx.i = 0;
	(handle_leading_bracket(&ctx), expand_bracket_content(&ctx));
	ctx.buf[ctx.buf_pos] = '\0';
	*out_len = ctx.buf_pos;
	return (ctx.buf);
}
