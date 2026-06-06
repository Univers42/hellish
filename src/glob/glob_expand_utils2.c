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

/* Called when a syntactic range "x-y" is invalid (x > y). POSIX leaves this
   undefined; bash treats the first character and the '-' as literals. We do
   the same: push ctx->start[ctx->i] and '-' onto the buffer, advance i by 2,
   and return 1 so the main loop knows we consumed something. */
int	handle_invalid_range(t_bracket_ctx *ctx)
{
	ctx->buf[ctx->buf_pos++] = ctx->start[ctx->i];
	ctx->buf[ctx->buf_pos++] = '-';
	ctx->i += 2;
	return (1);
}

/* Commit the result of a successful range expansion: advance buf_pos by the
   number of characters that were written and advance i by i_delta (the number
   of source bytes consumed by the range spec). Called after expand_range
   writes directly into ctx->buf[ctx->buf_pos]. */
void	consume_range(t_bracket_ctx *ctx, int range_count, int i_delta)
{
	ctx->buf_pos += range_count;
	ctx->i += i_delta;
}

/* Overwrite arr[len-1] with `val`. Used by handle_range to patch a '-' back
   as a literal when the start char was a backslash escape -- the escaped char
   was already placed at arr[len-1], and we replace it with '-' to reflect the
   "literal dash" fallback interpretation. */
void	set_last_elem(char *arr, int len, char val)
{
	if (len > 0)
		arr[len - 1] = val;
}

/* Generic peek: return the byte at position (base+offset)*elem_size inside
   `arr`. For our bracket use case elem_size is always 1 (char array), so
   this degenerates to arr[base+offset]. The abstraction exists so peek_bracket
   can be expressed in terms of it without casting. */
char	peek_elem(const void *arr, int base, int offset, size_t elem_size)
{
	return (((const char *)arr)[(base + offset) * elem_size]);
}

/* Peek at ctx->start[ctx->i + offset] -- looking ahead inside the bracket
   content without advancing i. Offset 0 is the current char, 1 is the next,
   2 the one after that (needed for range detection "c-c"). */
char	peek_bracket(const t_bracket_ctx *ctx, int offset)
{
	return (peek_elem(ctx->start, ctx->i, offset, sizeof(char)));
}
