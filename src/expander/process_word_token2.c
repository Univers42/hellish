/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process_word_token2.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 12:54:03 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 20:10:54 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "parena.h"

static void	update_token_if_changed_ctx(t_word_token_ctx *ctx)
{
	char	*newstr;

	if (ctx->changed)
	{
		newstr = xmalloc(ctx->outbuf->len + 1);
		if (newstr)
		{
			if (ctx->outbuf->len)
				memcpy(newstr, ctx->outbuf->ctx, ctx->outbuf->len);
			newstr[ctx->outbuf->len] = '\0';
			if (ctx->tok->allocated && ctx->tok->start)
				parena_free((char *)ctx->tok->start);
			ctx->tok->start = newstr;
			ctx->tok->len = ctx->outbuf->len;
			ctx->tok->allocated = true;
		}
		xfree(ctx->outbuf->ctx);
	}
	else
		xfree(ctx->outbuf->ctx);
}

/* Scan a TT_WORD token for $(...), $((...)), and `...` substitutions.
   Characters are copied character-by-character until a substitution prefix
   is recognised; substitution helpers flush any accumulated prefix, expand,
   and advance pos by the number of consumed input bytes.  If nothing
   changed `tok` is left untouched and the output buffer is discarded. */
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
		if (try_backtick_ctx(&ctx))
			continue ;
		push_single_char_ctx(&ctx);
	}
	update_token_if_changed_ctx(&ctx);
}
