/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 23:56:24 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 23:58:41 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

bool	handle_envvar_empty(t_reparser *rp, int prev_start, t_tt tt)
{
	handle_envvar_quote(rp, prev_start, tt);
	if (prev_start < rp->current_token.len
		&& (rp->current_token.start[prev_start] == '\''
			|| rp->current_token.start[prev_start] == '"')
		&& tt != TT_DQENVVAR)
		return (true);
	handle_envvar_literal(rp, prev_start, tt);
	return (false);
}

void	handle_envvar_literal(t_reparser *rp, int prev_start, t_tt tt)
{
	t_tt	chosen;

	if (tt == TT_DQENVVAR)
	{
		push_subtoken_node(&rp->current_node, rp->current_token,
			create_interval(prev_start - 1, prev_start), TT_DQWORD);
		return ;
	}
	chosen = select_literal_tt(tt, &rp->current_token, prev_start);
	push_subtoken_node(&rp->current_node, rp->current_token,
		create_interval(prev_start - 1, rp->i), chosen);
}

bool	is_double_close_paren_rp(t_reparser *rp)
{
	return (is_double_close_paren(rp->current_token, rp->i));
}

bool	is_open_paren_rp(t_reparser *rp)
{
	return (is_open_paren(rp->current_token, rp->i));
}

bool	is_close_paren_rp(t_reparser *rp)
{
	return (is_close_paren(rp->current_token, rp->i));
}
