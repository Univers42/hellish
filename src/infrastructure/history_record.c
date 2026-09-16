/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_record.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 17:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 17:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "history_private.h"

/* Putting one entry into the history, and doing it BEFORE the command runs.
**
** bash records a command as it reads it, not after it finishes, and the
** visible consequence is that `history` lists itself as its own last line.
** Recording afterwards made the listing always stop one short, and it made
** HISTSIZE disagree too: with HISTSIZE=3 bash's list at the moment
** `history` runs already holds the `history`, so it shows one entry fewer
** than hellish did.
**
** The recording is moved earlier rather than the whole of manage_history,
** because manage_history also RESETS the input ring, and the ring is still
** being read while the command executes -- $LINENO maps token offsets
** through it. So the two halves are now separate: this one runs before
** execute_top_level, the reset stays where it always was, and the flag
** keeps the three other callers (EOF, `exit`) working unchanged by making
** the second attempt a no-op. */

/* Push one entry into readline's list, our vector and the file, then apply
   the HISTCONTROL and HISTSIZE rules that can retire older entries. The
   file write happens before those, because erasedups and the size cap may
   free the very string being written. */
void	append_hist_entry(t_shell *state, char *hist_entry)
{
	char	*enc;

	add_history(hist_entry);
	vec_push(&state->hist.hist_cmds, &hist_entry);
	if (state->hist.append_fd >= 0)
	{
		enc = (char *)encode_cmd_hist(hist_entry).ctx;
		if (write_to_file(enc, state->hist.append_fd))
		{
			warning_error("Failed to write to the history file");
			close(state->hist.append_fd);
			state->hist.append_fd = -1;
		}
		xfree(enc);
	}
	hist_erase_dups(state, hist_entry);
	hist_trim_to_limit(state);
}

/* Record this cycle's command, once.
**
** `early` says this is the call that runs BEFORE the tree executes. A
** cycle carrying a here-document declines that one and records at the end
** of the cycle, the way everything used to: a here-doc BODY is read while
** the command runs, so before execution the ring buffer does not yet hold
** the whole entry -- recording then stored a truncated command and left
** the reader waiting on a `heredoc>` continuation that never came.
**
** The gate belongs to `early` alone and not to the function, which is the
** bug this parameter exists to prevent: with it inside, manage_history's
** end-of-cycle call refused for the same reason and a here-doc was never
** recorded at all. The listing position of a here-doc is not worth
** wedging the shell for; dropping it entirely is worse still. */
void	history_record(t_shell *state, bool early)
{
	char	*entry;

	if (state->hist.recorded)
		return ;
	if (early && state->cycle_has_hd)
		return ;
	if (!worthy_of_being_remembered(state))
		return ;
	entry = hist_candidate(state);
	if (!entry)
		return ;
	append_hist_entry(state, entry);
	state->hist.recorded = true;
}
