/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process_word_token.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 12:54:03 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 13:38:19 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

static bool	try_arith_sub_ctx(t_word_token_ctx *ctx)
{
	const char		*s = ctx->tok->start + ctx->pos;
	int				consumed;
	const bool		result
		= (ctx->pos + 2 < ctx->total_len && s[0] == '$'
			&& s[1] == '(' && s[2] == '(');
	t_expand_ctx	ectx;

	if (result)
	{
		consumed = 0;
		ectx = init_expand(s, ctx->total_len - ctx->pos,
				ctx->outbuf, &consumed);
		if (ctx->pos > 0 && ctx->outbuf->len == 0)
			vec_push_nstr(ctx->outbuf, ctx->tok->start, (size_t)ctx->pos);
		if (process_arith_sub(ctx->state, &ectx))
		{
			ctx->pos += consumed;
			ctx->changed = true;
			return (true);
		}
	}
	return (false);
}

static bool	try_cmd_sub_ctx(t_word_token_ctx *ctx)
{
	const char		*s = ctx->tok->start + ctx->pos;
	t_expand_ctx	ectx;
	int				consumed;

	if (ctx->pos + 1 < ctx->total_len && s[0] == '$' && s[1] == '(')
	{
		consumed = 0;
		ectx = init_expand(s, ctx->total_len - ctx->pos,
				ctx->outbuf, &consumed);
		if (ctx->pos > 0 && ctx->outbuf->len == 0)
			vec_push_nstr(ctx->outbuf, ctx->tok->start, (size_t)ctx->pos);
		if (process_cmd_sub(ctx->state, &ectx))
		{
			ctx->pos += consumed;
			ctx->changed = true;
			return (true);
		}
	}
	return (false);
}

static void	push_single_char_ctx(t_word_token_ctx *ctx)
{
	const char	*s = ctx->tok->start + ctx->pos;
	char		c;

	c = s[0];
	vec_push(ctx->outbuf, &c);
	ctx->pos++;
}

static void	update_token_if_changed_ctx(t_word_token_ctx *ctx)
{
	char	*newstr;

	if (ctx->changed)
	{
		newstr = malloc(ctx->outbuf->len + 1);
		if (newstr)
		{
			if (ctx->outbuf->len)
				memcpy(newstr, ctx->outbuf->ctx, ctx->outbuf->len);
			newstr[ctx->outbuf->len] = '\0';
			if (ctx->tok->allocated && ctx->tok->start)
				free((char *)ctx->tok->start);
			ctx->tok->start = newstr;
			ctx->tok->len = ctx->outbuf->len;
			ctx->tok->allocated = true;
		}
		free(ctx->outbuf->ctx);
	}
	else
		free(ctx->outbuf->ctx);
}

void	process_word_token(t_shell *state, t_token *tok)
{
	t_string			outbuf;
	t_word_token_ctx	ctx;

	vec_init(&outbuf);
	outbuf.elem_size = 1;
	ctx.state = state;
	ctx.tok = tok;
	ctx.outbuf = &outbuf;
	ctx.total_len = tok->len;
	ctx.pos = 0;
	ctx.changed = false;
	while (ctx.pos < ctx.total_len)
	{
		if (try_arith_sub_ctx(&ctx))
			continue ;
		if (try_cmd_sub_ctx(&ctx))
			continue ;
		push_single_char_ctx(&ctx);
	}
	update_token_if_changed_ctx(&ctx);
}
