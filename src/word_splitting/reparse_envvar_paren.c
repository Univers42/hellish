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

void	reparse_envvar_paren(t_ast_node *ret, int *i, t_token t, int prev_start)
{
	int	depth;

	depth = get_initial_paren_depth(i, t);
	scan_until_matching(i, t, &depth);
	push_subtoken_node(ret, t, create_interval(prev_start - 1, *i), TT_WORD);
}
