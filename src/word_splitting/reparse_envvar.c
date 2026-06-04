/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_envvar.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 22:29:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 10:20:52 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

void	update_envvar_result(t_ast_node *ret, int *i, t_reparser *rp)
{
	*i = rp->i;
	*ret = rp->current_node;
}

bool	handle_envvar_paren_or_special(t_reparser *rp,
			int prev_start, t_tt tt)
{
	if (try_handle_paren_rp(rp, prev_start, tt))
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

void	handle_envvar_ident(t_reparser *rp, int prev_start, t_tt tt)
{
	push_subtoken_node(&rp->current_node, rp->current_token,
		create_interval(prev_start, rp->i), tt);
}

/* Advance rp->i to the matching close quote `q` (leaving it on the quote),
   so braces inside a quoted segment of ${...} are not counted. */
void	skip_quoted_in_brace(t_reparser *rp, char q)
{
	const char	*s;
	int			len;

	s = rp->current_token.start;
	len = rp->current_token.len;
	rp->i++;
	while (rp->i < len && s[rp->i] != q)
	{
		if (q == '"' && s[rp->i] == '\\' && rp->i + 1 < len)
			rp->i++;
		rp->i++;
	}
}
