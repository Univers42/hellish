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

static void	consume_depth_idx(int *depth, int *i, int delta, int count)
{
	*depth += delta;
	*i += count;
}

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
