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

/* Eat a run of '*' characters and emit a single G_ASTERISK token. This is
   the same collapsing logic as handle_asterisk_token -- both are entry points
   depending on which tokenizer path calls them. */
void	tokenize_asterisk(t_tokenizer_ctx ctx)
{
	t_glob	g;

	while (*ctx.i < ctx.len && ctx.pattern[*ctx.i] == '*')
		(*ctx.i)++;
	g = (t_glob){.ty = G_ASTERISK, .start = ctx.pattern, .len = 1};
	vec_push(ctx.ret, &g);
}

/* Emit one G_QUESTION token for the '?' at the current position. */
void	tokenize_question(t_tokenizer_ctx ctx)
{
	t_glob	g;

	g = (t_glob){.ty = G_QUESTION, .start = ctx.pattern + *ctx.i, .len = 1};
	vec_push(ctx.ret, &g);
	(*ctx.i)++;
}

/* Emit a G_LITERAL token for a run of non-special characters. `force_n`
   forces at least N characters to be consumed even if they look special --
   used by handle_bracket_token to treat a bare '[' as literal when parse_
   bracket rejects it. The can_glob flag controls whether *, ? and [ stop
   the run; when can_glob is false the entire remaining segment up to '/' or
   end-of-pattern is one big literal, which is how quoted text works. */
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

/* Thin wrapper: tokenize a literal run with no forced minimum length. */
void	tokenize_literal(t_tokenizer_ctx ctx)
{
	tokenize_literal_n(ctx, !ctx.quoted, 0);
}

/* Tokenize one AST token and append its glob tokens to `ret`. The token's
   type determines whether glob special characters are active: TT_SQWORD and
   TT_DQWORD suppress globbing (can_glob = false), so a '*' inside "..." is
   just a literal character. The intermediate `sub` vector is freed after
   copying to avoid leaking the inner allocation. */
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
	xfree(sub.ctx);
}
