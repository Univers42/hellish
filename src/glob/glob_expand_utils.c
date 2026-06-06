/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_expand_utils.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 11:51:46 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 11:57:52 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/* Return true when the bracket content is the special form "]x" (len==2,
   first char ']'). POSIX says a ']' as the first character of a bracket
   expression is treated as a literal -- so "[]]" matches a literal ']' and
   "[]-]" matches ']' or '-'. This predicate triggers the fast path in
   glob_expand_bracket instead of the general content expander. */
bool	is_leading_bracket_special(const char *start, int len)
{
	return (len >= 2 && start[0] == ']' && len == 2);
}

/* Expand the special-case leading-']' bracket: the content is just one char
   (start[1]). We allocate a 2-byte buffer and return the single character.
   This is a separate function so glob_expand_bracket stays readable. */
char	*expand_leading_bracket_special(const char *start, int *out_len)
{
	char	*buf;

	buf = xmalloc(2);
	if (!buf)
		return (NULL);
	buf[0] = start[1];
	buf[1] = '\0';
	*out_len = 1;
	return (buf);
}

/* If the first character of the content is ']' and there are more characters
   after it, skip it (advance start and shrink len) so that the normal content
   loop never sees it -- it was already handled as a literal by the caller.
   This is the in-place variant of the leading-']' rule for multi-character
   bracket contents like "[]-z]" (matches ] through z). */
void	handle_leading_bracket(t_bracket_ctx *ctx)
{
	if (ctx->len >= 2 && ctx->start[0] == ']' && ctx->len > 2)
	{
		ctx->start++;
		ctx->len--;
	}
}

/* Handle a backslash escape inside a bracket: consume '\' and add the next
   raw character to the buffer, then skip two positions. This lets "[\\]"
   match a literal backslash. Returns 1 if a backslash was consumed. */
int	handle_backslash(t_bracket_ctx *ctx)
{
	if (ctx->start[ctx->i] == '\\' && ctx->i + 1 < ctx->len)
	{
		ctx->buf[ctx->buf_pos++] = ctx->start[ctx->i + 1];
		ctx->i += 2;
		return (1);
	}
	return (0);
}

/* Check whether the current position starts a POSIX class ([:name:]) and,
   if so, expand it into ctx->buf via check_posix_class. Returns 1 and
   advances ctx->i by the consumed bytes. Returns 0 if there's no class here,
   so the caller can fall through to range/literal handling. */
int	handle_posix_class(t_bracket_ctx *ctx)
{
	int	consumed;

	if (ctx->i + 1 < ctx->len && ctx->start[ctx->i] == '['
		&& ctx->start[ctx->i + 1] == ':')
	{
		consumed = check_posix_class(ctx->start + ctx->i,
				ctx->len - ctx->i, ctx->buf, &ctx->buf_pos);
		if (consumed > 0)
		{
			ctx->i += consumed;
			return (1);
		}
	}
	return (0);
}
