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

/* Push a TT_DQWORD subtoken covering [start, end) and advance rp->i to end.
   The *pushed_any flag is set so the caller knows at least one subtoken was
   emitted (used by the empty-string "" special case in reparse_dquote). */
static void	consume_and_push_dqword(t_reparser *rp, int start,
									int end, bool *pushed_any)
{
	push_dqword_subtoken_rp(rp, start, end);
	*pushed_any = true;
	rp->i = end;
}

/* Emit one TT_DQWORD subtoken for the range [start, end) in the current
   token. This is the primitive that all double-quote literal emitters call;
   keeping it in one place makes it easy to add tracing or assertions later. */
void	push_dqword_subtoken_rp(t_reparser *rp, int start, int end)
{
	push_subtoken_node(&rp->current_node, rp->current_token,
		create_interval(start, end), TT_DQWORD);
}

/* Emit the plain-text segment accumulated since rp->prev_start if it is
   non-empty. Called before every special character inside a double-quoted
   region so we don't lose the text between two $-expansions. After the call,
   the caller is responsible for updating prev_start to rp->i. */
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

/* A '$' appeared inside a double-quoted region; hand off to reparse_envvar
   with TT_DQENVVAR so all the subtokens it emits are tagged as inside-dquote.
   That tag suppresses IFS splitting of the expansion result downstream. */
void	process_dollar_in_dquote_rp(t_reparser *rp, bool *pushed_any)
{
	reparse_envvar(&rp->current_node, &rp->i, rp->current_token, TT_DQENVVAR);
	*pushed_any = true;
}

/* Handle a backslash inside double quotes. POSIX says only \\ \" \$ \`
   and \<newline> are active escapes inside "..."; any other backslash is
   kept literally with the slash. \<newline> is a line-continuation: we
   just skip the newline, emitting nothing (the backslash is eaten). For the
   active escapes we push just the escaped character; for an inert backslash
   we include the backslash itself in the subtoken (esc_pos..rp->i+1). */
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
