/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_envvar2.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 22:29:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 10:20:52 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

static void	scan_brace_depth(t_reparser *rp, int *depth)
{
	char	c;

	c = rp->current_token.start[rp->i];
	if (c == '\'' || c == '"')
	{
		skip_quoted_in_brace(rp, c);
		rp->i++;
	}
	else if (c == '{')
	{
		(*depth)++;
		rp->i++;
	}
	else if (c == '}' && --(*depth) == 0)
		return ;
	else
		rp->i++;
}

static bool	handle_envvar_brace(t_reparser *rp, t_tt tt)
{
	int	start;
	int	depth;

	if (rp->i >= rp->current_token.len
		|| rp->current_token.start[rp->i] != '{')
		return (false);
	rp->i++;
	start = rp->i;
	depth = 1;
	while (rp->i < rp->current_token.len && depth > 0)
		scan_brace_depth(rp, &depth);
	push_subtoken_node(&rp->current_node, rp->current_token,
		create_interval(start, rp->i), tt);
	if (rp->i < rp->current_token.len)
		rp->i++;
	return (true);
}

void	reparse_envvar(t_ast_node *ret, int *i, t_token t, t_tt tt)
{
	t_reparser	rp;
	int			prev_start;

	ft_assert(t.start[(*i)++] == '$');
	create_reparser(&rp, *ret, t, i);
	prev_start = rp.i;
	if (handle_envvar_brace(&rp, tt))
		return (update_envvar_result(ret, i, &rp));
	if (handle_envvar_paren_or_special(&rp, prev_start, tt))
		return (update_envvar_result(ret, i, &rp));
	consume_ident_rp(&rp);
	if (prev_start == rp.i)
	{
		if (handle_envvar_empty(&rp, prev_start, tt))
			return (update_envvar_result(ret, i, &rp));
	}
	else
		handle_envvar_ident(&rp, prev_start, tt);
	update_envvar_result(ret, i, &rp);
}
