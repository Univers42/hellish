/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_dquote.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 22:28:04 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 23:47:05 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

static int	process_dquote_char_rp(t_reparser *rp, bool *pushed_any)
{
	if (rp->current_token.start[rp->i] == '\\')
	{
		flush_pending_segment_rp(rp, pushed_any);
		process_escape_seq_rp(rp, pushed_any);
		return (1);
	}
	if (rp->current_token.start[rp->i] == '$')
	{
		flush_pending_segment_rp(rp, pushed_any);
		process_dollar_in_dquote_rp(rp, pushed_any);
		rp->prev_start = rp->i;
		return (1);
	}
	return (0);
}

void	reparse_dquote(t_ast_node *ret, int *i, t_token t)
{
	t_reparser	rp;
	bool		pushed_any;

	ft_assert(t.start[(*i)++] == '"');
	create_reparser(&rp, *ret, t, i);
	rp.prev_start = rp.i;
	pushed_any = false;
	while (rp.i < rp.current_token.len && rp.current_token.start[rp.i] != '"')
	{
		if (process_dquote_char_rp(&rp, &pushed_any))
			continue ;
		rp.i++;
	}
	flush_pending_segment_rp(&rp, &pushed_any);
	if (!pushed_any)
		push_dqword_subtoken_rp(&rp, rp.i, rp.i);
	ft_assert(rp.current_token.start[(rp.i)++] == '"');
	*i = rp.i;
	*ret = rp.current_node;
}
