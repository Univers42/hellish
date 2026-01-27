/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_envvar_utils.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 23:29:27 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:16:39 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

bool	try_handle_paren_rp(t_reparser *rp, int prev_start)
{
	if (is_double_open_paren(rp->current_token, rp->i))
	{
		reparse_envvar_paren(&rp->current_node, &rp->i,
			rp->current_token, prev_start);
		return (true);
	}
	if (is_open_paren(rp->current_token, rp->i))
	{
		reparse_envvar_paren(&rp->current_node, &rp->i,
			rp->current_token, prev_start);
		return (true);
	}
	return (false);
}

bool	try_handle_special_rp(t_reparser *rp, t_tt tt)
{
	return (reparse_special_envvar(&rp->current_node,
			&rp->i, rp->current_token, tt));
}

void	consume_ident_rp(t_reparser *rp)
{
	if (rp->i < rp->current_token.len
		&& is_var_name_p1(rp->current_token.start[rp->i]))
		rp->i++;
	while (rp->i < rp->current_token.len
		&& is_var_name_p2(rp->current_token.start[rp->i]))
		rp->i++;
}

t_tt	select_literal_tt(t_tt ctx_tt, t_token *t, int prev_start)
{
	t_tt	out;

	if (ctx_tt == TT_DQENVVAR && prev_start < t->len
		&& t->start[prev_start] == '"')
		return (TT_DQWORD);
	if (prev_start < t->len && (t->start[prev_start] == '\''
			|| t->start[prev_start] == '"'))
		return (TT_ENVVAR);
	if (ctx_tt == TT_DQENVVAR)
		out = TT_DQWORD;
	else
		out = TT_SQWORD;
	return (out);
}
