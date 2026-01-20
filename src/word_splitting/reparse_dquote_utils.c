/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_dquote_utils.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 22:47:41 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 23:46:37 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"

static void	consume_and_push_dqword(t_reparser *rp, int start,
									int end, bool *pushed_any)
{
	push_dqword_subtoken_rp(rp, start, end);
	*pushed_any = true;
	rp->i = end;
}

void	push_dqword_subtoken_rp(t_reparser *rp, int start, int end)
{
	push_subtoken_node(&rp->current_node, rp->current_token,
		create_interval(start, end), TT_DQWORD);
}

void	flush_pending_segment_rp(t_reparser *rp, bool *pushed_any)
{
	int	prev_start;
	int	cur_i;

	cur_i = rp->i;
	prev_start = rp->prev_start;
	if (cur_i > prev_start)
	{
		push_dqword_subtoken_rp(rp, prev_start, cur_i);
		*pushed_any = true;
	}
}

void	process_dollar_in_dquote_rp(t_reparser *rp, bool *pushed_any)
{
	reparse_envvar(&rp->current_node, &rp->i, rp->current_token, TT_DQENVVAR);
	*pushed_any = true;
}

/* Process an escape sequence inside double quotes using reparser state */
void	process_escape_seq_rp(t_reparser *rp, bool *pushed_any)
{
	int		esc_pos;
	char	c;

	esc_pos = rp->i;
	rp->i++;
	if (rp->i < rp->current_token.len)
	{
		c = rp->current_token.start[rp->i];
		if (c == '\n')
			rp->i++;
		else if (c == '"' || c == '$' || c == '\\' || c == '`')
			consume_and_push_dqword(rp, rp->i, rp->i + 1, pushed_any);
		else
			consume_and_push_dqword(rp, esc_pos, rp->i + 1, pushed_any);
	}
	else
		consume_and_push_dqword(rp, esc_pos, rp->i, pushed_any);
	rp->prev_start = rp->i;
}
