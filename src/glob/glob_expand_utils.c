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

bool	is_leading_bracket_special(const char *start, int len)
{
	return (len >= 2 && start[0] == ']' && len == 2);
}

char	*expand_leading_bracket_special(const char *start, int *out_len)
{
	char	*buf;

	buf = malloc(2);
	if (!buf)
		return (NULL);
	buf[0] = start[1];
	buf[1] = '\0';
	*out_len = 1;
	return (buf);
}

void	handle_leading_bracket(t_bracket_ctx *ctx)
{
	if (ctx->len >= 2 && ctx->start[0] == ']' && ctx->len > 2)
	{
		ctx->start++;
		ctx->len--;
	}
}

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
