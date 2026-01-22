/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_bracket.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 11:10:40 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 12:54:53 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

// Helper: check for negation and update index and flags
static void	parse_bracket_negation(t_bracket_parse_ctx *ctx)
{
	if (ctx->i < ctx->max_len && (ctx->s[ctx->i] == '!'
			|| ctx->s[ctx->i] == '^'))
	{
		ctx->flags |= BRACKET_NEGATED;
		ctx->i++;
	}
}

// Helper: skip POSIX class like [:name:]
static void	skip_posix_class(const char *s, int max_len, int *i)
{
	if (s[*i] == '[' && *i + 1 < max_len && s[*i + 1] == ':')
	{
		*i += 2;
		while (*i + 1 < max_len && !(s[*i] == ':' && s[*i + 1] == ']'))
			(*i)++;
		if (*i + 1 < max_len)
			*i += 2;
	}
}

// Helper: find closing bracket, handling POSIX classes
static int	find_closing_bracket(const char *s, int max_len, int i)
{
	while (i < max_len)
	{
		if (s[i] == ']')
			break ;
		skip_posix_class(s, max_len, &i);
		if (s[i] != ']')
			i++;
	}
	return (i);
}

// Helper: fill t_glob for bracket and expand char_set
static int	fill_bracket_glob(const t_bracket_parse_ctx *ctx, t_glob *g)
{
	g->ty = G_BRACKET;
	g->start = ctx->s + ctx->content_start;
	g->len = ctx->i - ctx->content_start;
	g->flags = ctx->flags;
	g->char_set = glob_expand_bracket(g->start, g->len, &g->char_set_len);
	if (!g->char_set || g->char_set_len == 0)
	{
		if (g->char_set)
			free(g->char_set);
		g->char_set = NULL;
		return (0);
	}
	return (ctx->i + 1);
}

/*
** Parse a bracket expression [...]
** For [[::]], the content includes the inner [::]
** Returns the length consumed (including brackets), or 0 on error
*/
int	parse_bracket(const char *s, int max_len, t_glob *g)
{
	t_bracket_parse_ctx	ctx;

	ctx.s = s;
	ctx.i = 1;
	ctx.max_len = max_len;
	ctx.flags = 0;
	if (*s != '[' || max_len < 2)
		return (0);
	parse_bracket_negation(&ctx);
	if (ctx.i >= max_len)
		return (0);
	ctx.content_start = ctx.i;
	if (s[ctx.i] == ']')
		ctx.i++;
	ctx.i = find_closing_bracket(s, max_len, ctx.i);
	if (ctx.i >= max_len)
		return (0);
	if (ctx.i == ctx.content_start)
		return (0);
	return (fill_bracket_glob(&ctx, g));
}
