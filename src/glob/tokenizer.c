/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tokenizer.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 11:11:01 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 12:45:04 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/*
** Tokenize asterisk (collapse multiple *'s into one)
*/
void	tokenize_asterisk(t_tokenizer_ctx ctx)
{
	t_glob	g;

	while (*ctx.i < ctx.len && ctx.pattern[*ctx.i] == '*')
		(*ctx.i)++;
	g = (t_glob){.ty = G_ASTERISK, .start = ctx.pattern, .len = 1};
	vec_push(ctx.ret, &g);
}

/*
** Tokenize question mark
*/
void	tokenize_question(t_tokenizer_ctx ctx)
{
	t_glob	g;

	g = (t_glob){.ty = G_QUESTION, .start = ctx.pattern + *ctx.i, .len = 1};
	vec_push(ctx.ret, &g);
	(*ctx.i)++;
}

/*
** Tokenize literal characters until next special char or end
** If force_one is true, consume at least one character even if special
*/
void	tokenize_literal_n(t_tokenizer_ctx ctx, bool can_glob, int force_n)
{
	int		start;
	t_glob	g;

	start = *ctx.i;
	while (*ctx.i < ctx.len && force_n > 0)
	{
		(*ctx.i)++;
		force_n--;
	}
	while (*ctx.i < ctx.len)
	{
		if (ctx.pattern[*ctx.i] == '/')
			break ;
		if (can_glob && (ctx.pattern[*ctx.i] == '*'
				|| ctx.pattern[*ctx.i] == '?'
				|| ctx.pattern[*ctx.i] == '['))
			break ;
		(*ctx.i)++;
	}
	if (*ctx.i > start)
	{
		g = init_glob(G_LITERAL, ctx.pattern + start, *ctx.i - start);
		vec_push(ctx.ret, &g);
	}
}

/*
** Tokenize literal characters until next special char or end
*/
void	tokenize_literal(t_tokenizer_ctx ctx)
{
	tokenize_literal_n(ctx, !ctx.quoted, 0);
}

/*
** Tokenize a single token from the AST
*/
void	tokenize_ast_token(t_vec_glob *ret, t_token t)
{
	bool		can_glob;
	t_vec_glob	sub;
	size_t		j;

	can_glob = star_expandable(t.tt);
	sub = glob_tokenize(t.start, t.len, !can_glob);
	j = 0;
	while (j < sub.len)
	{
		vec_push(ret, &((t_glob *)sub.ctx)[j]);
		j++;
	}
	free(sub.ctx);
}
