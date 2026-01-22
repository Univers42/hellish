/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_match_at.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 12:05:29 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 12:14:15 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

static bool	match_literal_token(t_glob_match_ctx ctx, t_glob *g)
{
	int	consumed;

	consumed = match_literal(ctx.name, g);
	if (consumed < 0)
		return (false);
	ctx.name += consumed;
	ctx.offset += 1;
	ctx.is_first = (ctx.offset == 0
			|| ((t_glob *)ctx.pattern->ctx)[ctx.offset - 1].ty == G_SLASH);
	return (glob_match_at(ctx.name, ctx.pattern, ctx.offset));
}

static bool	match_question_token(t_glob_match_ctx ctx, t_glob *g)
{
	int	consumed;

	consumed = match_question(ctx.name, ctx.is_first);
	if (consumed < 0)
		return (false);
	ctx.name += consumed;
	ctx.offset += 1;
	ctx.is_first = (ctx.offset == 0
			|| ((t_glob *)ctx.pattern->ctx)[ctx.offset - 1].ty == G_SLASH);
	return (glob_match_at(ctx.name, ctx.pattern, ctx.offset));
	(void)g;
}

static bool	match_bracket_token(t_glob_match_ctx ctx, t_glob *g)
{
	int	consumed;

	consumed = match_bracket(ctx.name, g, ctx.is_first);
	if (consumed < 0)
		return (false);
	ctx.name += consumed;
	ctx.offset += 1;
	ctx.is_first = (ctx.offset == 0
			|| ((t_glob *)ctx.pattern->ctx)[ctx.offset - 1].ty == G_SLASH);
	return (glob_match_at(ctx.name, ctx.pattern, ctx.offset));
}

static bool	match_asterisk_token(t_glob_match_ctx ctx)
{
	return (match_asterisk_recursive(ctx.name,
			ctx.pattern, ctx.offset, ctx.is_first));
}

bool	glob_match_at(const char *name, t_vec_glob *pattern, size_t offset)
{
	t_glob				*g;
	t_glob_match_ctx	ctx;

	if (offset >= pattern->len)
		return (*name == '\0');
	g = &((t_glob *)pattern->ctx)[offset];
	ctx.name = name;
	ctx.pattern = pattern;
	ctx.offset = offset;
	ctx.is_first = (offset == 0
			|| ((t_glob *)pattern->ctx)[offset - 1].ty == G_SLASH);
	if (g->ty == G_SLASH)
		return (false);
	else if (g->ty == G_LITERAL)
		return (match_literal_token(ctx, g));
	else if (g->ty == G_QUESTION)
		return (match_question_token(ctx, g));
	else if (g->ty == G_BRACKET)
		return (match_bracket_token(ctx, g));
	else if (g->ty == G_ASTERISK)
		return (match_asterisk_token(ctx));
	return (false);
}
