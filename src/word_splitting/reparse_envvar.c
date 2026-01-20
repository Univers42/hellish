/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_envvar.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 22:29:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 23:57:47 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

static void	update_envvar_result(t_ast_node *ret, int *i, t_reparser *rp)
{
	*i = rp->i;
	*ret = rp->current_node;
}

static bool	handle_envvar_paren_or_special(t_reparser *rp,
				int prev_start, t_tt tt)
{
	if (try_handle_paren_rp(rp, prev_start))
		return (true);
	if (try_handle_special_rp(rp, tt))
		return (true);
	return (false);
}

void	handle_envvar_quote(t_reparser *rp, int prev_start, t_tt tt)
{
	if (prev_start < rp->current_token.len
		&& rp->current_token.start[prev_start] == '\'' && tt != TT_DQENVVAR)
	{
		reparse_squote(&rp->current_node, &rp->i, rp->current_token);
		return ;
	}
	if (prev_start < rp->current_token.len
		&& rp->current_token.start[prev_start] == '"' && tt != TT_DQENVVAR)
	{
		reparse_dquote(&rp->current_node, &rp->i, rp->current_token);
		return ;
	}
}

static void	handle_envvar_ident(t_reparser *rp, int prev_start, t_tt tt)
{
	push_subtoken_node(&rp->current_node, rp->current_token,
		create_interval(prev_start, rp->i), tt);
}

void	reparse_envvar(t_ast_node *ret, int *i, t_token t, t_tt tt)
{
	t_reparser	rp;
	int			prev_start;

	ft_assert(t.start[(*i)++] == '$');
	create_reparser(&rp, *ret, t, i);
	prev_start = rp.i;
	if (handle_envvar_paren_or_special(&rp, prev_start, tt))
	{
		update_envvar_result(ret, i, &rp);
		return ;
	}
	consume_ident_rp(&rp);
	if (prev_start == rp.i)
	{
		if (handle_envvar_empty(&rp, prev_start, tt))
		{
			update_envvar_result(ret, i, &rp);
			return ;
		}
	}
	else
		handle_envvar_ident(&rp, prev_start, tt);
	update_envvar_result(ret, i, &rp);
}
