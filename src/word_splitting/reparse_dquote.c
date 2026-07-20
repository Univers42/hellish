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

/* Handle one special character inside a double-quoted region: flush any
   accumulated plain text before delegating. Returns 1 if the character was
   consumed (caller should continue), 0 if it's just a regular character
   (caller increments i and accumulates it into the pending segment).
   The split_eligible=false on backtick is POSIX: a `cmd` inside "..." must
   NOT be IFS-split -- its output is glued to the surrounding string. */
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
	if (rp->current_token.start[rp->i] == '`')
	{
		flush_pending_segment_rp(rp, pushed_any);
		reparse_backtick(&rp->current_node, &rp->i, rp->current_token);
		((t_ast_node *)rp->current_node.children.ctx)
		[rp->current_node.children.len - 1].token.split_eligible = false;
		*pushed_any = true;
		rp->prev_start = rp->i;
		return (1);
	}
	return (0);
}

/* Parse the content of a double-quoted string ("...") into a sequence of
   typed subtokens. Plain characters accumulate in [prev_start, i) until a
   special one appears (then flush_pending_segment_rp emits them), then the
   special is dispatched. The pushed_any flag catches the empty-string edge
   case: "" must still produce a TT_DQWORD subtoken (zero-length) so the
   expander knows the field exists rather than being absent entirely -- that
   distinction matters for `set -- "" foo`, where $1 must be the empty str. */
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
	if (getenv("HELLISH_DBG_DQ")
		&& rp.current_token.start[rp.i] != '"')
		fprintf(stderr, "[DQ-FAIL i=%d len=%d tok=<<%.*s>>]\n",
			rp.i, rp.current_token.len,
			rp.current_token.len, rp.current_token.start);
	ft_assert(rp.current_token.start[(rp.i)++] == '"');
	*i = rp.i;
	*ret = rp.current_node;
}
