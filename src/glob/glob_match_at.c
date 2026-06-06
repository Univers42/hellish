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

/* Match a G_LITERAL token and, on success, recurse for the remaining tokens.
   is_first tracks whether we're at the start of a path component (just after
   a slash, or at position 0). A G_SLASH token in the previous slot resets
   is_first to true for the next component. */
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

/* Match a G_QUESTION token: consume one character if allowed, then recurse.
   is_first is updated the same way as in match_literal_token. The (void)g
   suppresses a warning because g is unused here (the match_question function
   only needs the name pointer and the is_first flag). */
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

/* Match a G_BRACKET token against the current character using the pre-built
   char_set, then recurse for the rest of the pattern. Same is_first bookkeeping
   as the other token matchers. */
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

/* Delegate the backtracking asterisk match to match_asterisk_recursive. No
   is_first update here -- the recursive call handles the leading-dot guard. */
static bool	match_asterisk_token(t_glob_match_ctx ctx)
{
	return (match_asterisk_recursive(ctx.name,
			ctx.pattern, ctx.offset, ctx.is_first));
}

/* Core recursive matcher: does name match pattern starting at token `offset`?
   offset >= pattern->len means we've consumed all tokens -- success only if
   the name is also exhausted. A G_SLASH token at this position is a boundary
   between path components; it cannot match any character in name (the walker
   ensures names passed here are single components), so it returns false.
   All other token types dispatch to the per-type match helpers above. */
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
