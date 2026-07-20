/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_helpers2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:18 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 16:38:42 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

/* Rewind a FAILED batched cycle so it can be replayed line-by-line. A
   batch can glue a complete command to an incomplete or broken construct
   after it; bash would have executed the complete part before hitting the
   error (`trap ... EXIT` then a parse error still fires the trap). Since
   a batched cycle's input is a verbatim slice of the ring (no readline,
   no history rewrite, no backslash-join — those are batch hazards), we
   can push the cursor back over it and mark the region exact_until, which
   forces single-line delivery up to the old high-water mark. The caller
   must then abandon the cycle SILENTLY: no status, no should_exit, no
   error output beyond what already leaked. Returns false when the cycle
   was not batched (genuine failure — abort normally). */
bool	try_replay_exact(t_shell *state)
{
	t_rl	*l;

	l = &state->rl;
	if (!l->batched || state->metinp == INP_RL
		|| state->input.len == 0 || l->cursor < state->input.len)
		return (false);
	l->exact_until = l->cursor;
	l->cursor -= state->input.len;
	l->has_line = true;
	l->has_finished = false;
	l->batched = false;
	l->line -= nl_count((const char *)state->input.ctx, state->input.len);
	return (true);
}
