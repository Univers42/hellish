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

/* Emit a G_SLASH token for the '/' at the current position. Slashes are path
   separators; the directory walker (match_dir) uses them to decide when to
   recurse into a subdirectory. */
void	handle_slash_token(t_tokenizer_ctx ctx)
{
	t_glob	g;

	g = init_glob(G_SLASH, ctx.pattern + *ctx.i, 1);
	vec_push(ctx.ret, &g);
	(*ctx.i)++;
}

/* Consume one or more consecutive '*' characters and emit a single token.
   Stars collapse into one G_ASTERISK -- '***' is identical to '*' in POSIX
   glob semantics -- unless the run is a globstar segment, which becomes a
   G_GLOBSTAR the directory walker descends on. */
void	handle_asterisk_token(t_tokenizer_ctx ctx)
{
	t_glob	g;
	int		run;

	run = 0;
	while (*ctx.i + run < ctx.len && ctx.pattern[*ctx.i + run] == '*')
		run++;
	g = init_glob(G_ASTERISK, ctx.pattern, 1);
	if (is_globstar(ctx, run))
		g = init_glob(G_GLOBSTAR, ctx.pattern, 2);
	*ctx.i += run;
	vec_push(ctx.ret, &g);
}

/* Emit a G_QUESTION token for a single '?'. Each '?' matches exactly one
   character (excluding a leading dot), so "??" needs two separate tokens. */
void	handle_question_token(t_tokenizer_ctx ctx)
{
	t_glob	g;

	g = init_glob(G_QUESTION, ctx.pattern + *ctx.i, 1);
	vec_push(ctx.ret, &g);
	(*ctx.i)++;
}

/* Try to tokenize a '[...]' bracket expression. parse_bracket determines
   whether '[' starts a valid class or is just a literal. If parse_bracket
   returns 0 (invalid bracket, e.g. unclosed '[' or empty '[]'), the '[' is
   emitted as a literal character via tokenize_literal_n with force_n=1. */
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

/* Emit a G_LITERAL token for any character that isn't a wildcard or slash.
   Delegates to tokenize_literal, which runs until it hits a special char,
   so a run of plain letters becomes one token, not N single-char ones. */
void	handle_literal_token(t_tokenizer_ctx ctx)
{
	tokenize_literal(ctx);
}
