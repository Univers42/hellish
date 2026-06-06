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

/* Check for a leading '!' or '^' (both negate the class in bash; POSIX only
   requires '!' but bash also accepts '^' for historical/ksh compatibility).
   If found, set BRACKET_NEGATED in ctx->flags and advance past it. */
static void	parse_bracket_negation(t_bracket_parse_ctx *ctx)
{
	if (ctx->i < ctx->max_len && (ctx->s[ctx->i] == '!'
			|| ctx->s[ctx->i] == '^'))
	{
		ctx->flags |= BRACKET_NEGATED;
		ctx->i++;
	}
}

/* Skip over a POSIX class like [:alpha:] without expanding it. Used during
   the closing-bracket search so that a ':' or ']' inside a class name isn't
   mistaken for the class terminator or the closing bracket. */
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

/* Scan forward from position `i` to find the closing ']'. POSIX classes
   ([::]) are skipped transparently to prevent a ':' or ']' inside them from
   being mistaken for the terminator. Returns the index of ']' in `s`, or
   max_len if none was found (unclosed bracket → parse_bracket returns 0). */
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

/* Fill the t_glob struct for a bracket token and expand its char_set. The
   start/len pointers cover the raw content between '[' and ']' (excluding
   the brackets themselves). glob_expand_bracket builds the flat char_set
   string used by glob_char_in_class at match time. If the expansion yields
   an empty set (no valid chars after range expansion etc.), treat the bracket
   as a failed parse and return 0 so the tokenizer falls back to literal. */
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
			xfree(g->char_set);
		g->char_set = NULL;
		return (0);
	}
	return (ctx->i + 1);
}

/* Parse a bracket expression starting at s[0] == '[' and fill *g. Returns the
   number of bytes consumed (including the two brackets) on success, or 0 if
   the bracket is invalid (unclosed, empty, or expansion failed). The tricky
   POSIX edge cases: a ']' immediately after '[' (or '[!') is treated as a
   literal char IN the class, not the closing bracket -- hence the content_start
   check and the `s[ctx.i] == ']' → ctx.i++` before find_closing_bracket. */
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
