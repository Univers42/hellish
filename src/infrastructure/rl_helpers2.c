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
#include "job_control.h"

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

/* May this end-of-input actually end the shell?

   There is more than one way a Ctrl-D reaches the REPL -- handle_eof() on
   the get-more-input path and handle_eof_or_error() on the tokenizer path
   both used to set should_exit themselves -- and issue #58 is what happens
   when only some of them ask the stopped-jobs question: the `exit` builtin
   refused to abandon a stopped `top`, Ctrl-D walked straight past it, and
   the terminal that job had put in raw mode was left that way. So both now
   route through here.

   Returning false means the keypress was SPENT on the warning and the shell
   keeps prompting, which requires taking the EOF back. state->rl.has_finished
   is a one-way latch on purpose -- once stdin is done it stays done, so a
   script whose input closes stops promptly instead of spinning -- and this
   is the single case where it has to be cleared, or the next REPL turn sees
   EOF again with the warning already given and leaves anyway. That is
   "warned and gone in one keypress", which is not what asking twice means.

   eof_refused is what makes one keypress get one answer. A single Ctrl-D
   reaches BOTH paths in the same REPL turn -- handle_eof() first, then
   handle_eof_or_error() as the tokenizer unwinds -- so without it the first
   call warned and set exit_warned, and the second call read that flag as
   "already told them once" and left. The user pressed Ctrl-D once and got
   the warning and the exit together. The flag is cleared at the top of each
   turn (open_cycle), so the NEXT Ctrl-D is a genuinely new question and
   exit_warned answers it the way it always did. */
bool	rl_eof_exit_ok(t_shell *state)
{
	if (state->rl.eof_refused)
		return (false);
	if (!exit_stopped_guard(state))
		return (true);
	state->rl.eof_refused = true;
	state->rl.has_finished = false;
	state->rl.has_line = false;
	return (false);
}
