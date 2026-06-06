/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_envvar_paren.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 22:44:55 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 23:47:53 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

/* Determine if we're entering $((  (depth 2) or $( (depth 1) and advance *i
   past the opening paren(s). The depth is what scan_until_matching uses to
   know when to stop: for $((...)) both levels must close before we're done. */
static int	get_initial_paren_depth(int *i, t_token t)
{
	if (*i + 1 < t.len && t.start[*i] == '(' && t.start[*i + 1] == '(')
	{
		*i += 2;
		return (2);
	}
	(*i)++;
	return (1);
}

/* Atomic update: adjust depth by delta and advance position by count.
   Keeping both updates together prevents the scan loop from observing a
   partially updated state between the depth change and the index move. */
static void	consume_depth_idx(int *depth, int *i, int delta, int count)
{
	*depth += delta;
	*i += count;
}

/* Scan through a $(...)  or $((...)) until depth reaches 0. The $((...))
   case is tricky: at depth 2 we look for (( and )) pairs to handle nested
   arithmetic like $((a * (b + c))). At depth 1 a single ) closes; at depth 2
   only )) closes (single ) just opens a nested level). This correctly handles
   `echo $((1 + (2 * 3)))` without a separate arithmetic parser here. */
static void	scan_until_matching(int *i, t_token t, int *depth)
{
	while (*i < t.len && *depth > 0)
	{
		if (*depth == 2 && is_double_open_paren(t, *i))
			consume_depth_idx(depth, i, 2, 2);
		else if (*depth == 2 && is_double_close_paren(t, *i))
			consume_depth_idx(depth, i, -2, 2);
		else if (is_open_paren(t, *i))
			consume_depth_idx(depth, i, 1, 1);
		else if (is_close_paren(t, *i))
			consume_depth_idx(depth, i, -1, 1);
		else
			(*i)++;
	}
}

/* Emit a $(...) or $((...)) span as a single TT_WORD subtoken starting at
   prev_start - 1 (the '$') through the closing paren. split_eligible is
   false inside double-quotes (TT_DQENVVAR) -- the output of a double-quoted
   command substitution must not be IFS-split, matching bash behaviour. */
void	reparse_envvar_paren(t_paren_ctx ctx)
{
	int			depth;
	t_ast_node	*pushed;

	depth = get_initial_paren_depth(ctx.i, ctx.t);
	scan_until_matching(ctx.i, ctx.t, &depth);
	push_subtoken_node(ctx.ret, ctx.t,
		create_interval(ctx.prev_start - 1, *ctx.i), TT_WORD);
	pushed = &((t_ast_node *)ctx.ret->children.ctx)[ctx.ret->children.len - 1];
	pushed->token.split_eligible = (ctx.tt != TT_DQENVVAR);
}
