/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_tokenizer_utils.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 12:29:56 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 12:53:07 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

// Helper: handle a slash token
void	handle_slash_token(t_tokenizer_ctx ctx)
{
	t_glob	g;

	g = init_glob(G_SLASH, ctx.pattern + *ctx.i, 1);
	vec_push(ctx.ret, &g);
	(*ctx.i)++;
}

// Helper: handle asterisk token
void	handle_asterisk_token(t_tokenizer_ctx ctx)
{
	t_glob	g;

	while (*ctx.i < ctx.len && ctx.pattern[*ctx.i] == '*')
		(*ctx.i)++;
	g = init_glob(G_ASTERISK, ctx.pattern, 1);
	vec_push(ctx.ret, &g);
}

// Helper: handle question mark token
void	handle_question_token(t_tokenizer_ctx ctx)
{
	t_glob	g;

	g = init_glob(G_QUESTION, ctx.pattern + *ctx.i, 1);
	vec_push(ctx.ret, &g);
	(*ctx.i)++;
}

// Helper: handle bracket token
void	handle_bracket_token(t_tokenizer_ctx ctx)
{
	t_glob	g;
	int		consumed;

	consumed = parse_bracket(ctx.pattern + *ctx.i, ctx.len - *ctx.i, &g);
	if (consumed > 0)
	{
		vec_push(ctx.ret, &g);
		*ctx.i += consumed;
	}
	else
		tokenize_literal_n(ctx, !ctx.quoted, 1);
}

// Helper: handle literal token
void	handle_literal_token(t_tokenizer_ctx ctx)
{
	tokenize_literal(ctx);
}
