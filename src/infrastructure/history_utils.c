/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_utils.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:07 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:14:56 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "history_private.h"

/* Push a new entry both into readline's in-memory list and into our own
   hist_cmds vector, then append the encoded form to the history file via the
   long-lived append_fd. The fd approach is O(1) per command; we never rewrite
   the whole file mid-session (cap_history handles that at startup only).
   The entry is joined FIRST so all three consumers agree: readline's recall
   buffer, `history` / `fc -l`, and the file. Joining only for readline left
   `history` listing the raw lines of a multi-line command -- a for-loop
   printed as three entryless rows where bash prints one joined line. The
   join is idempotent (an already-joined line has no boundary newlines
   left), so replaying an old raw file through here is safe.

   `shopt -s lithist` does NOT skip the scanner -- it changes what the
   scanner does with a command-boundary newline (keep it, instead of
   rewriting it as "; "). That distinction is the fix for issue #32:
   skipping the scanner also skipped the top-level \<newline> splice, so
   `echo one \` + newline + `rest` came back as two lines with a dangling
   backslash where bash gives one line, `echo one rest`. Everything else
   -- quote state, here-doc bodies -- is shared, which is the point.

   Recalling an if/for/while under lithist gives back the multi-line
   buffer that was typed instead of a flattened one-liner. The file format
   already escapes embedded newlines, so a literal entry round-trips
   across sessions with no format change. add_history is called directly
   rather than add_history_line because the entry is already in its final
   shape by this point. */
/* End of cycle: record the command if history_record did not already (the
   EOF and `exit` paths reach here without having run it), then clear the
   expansion flag and compact the ring buffer.

   The expanded form is what gets saved, so "!! ; echo done" records the
   real command rather than "!!" -- less confusing to navigate later. */
void	manage_history(t_shell *state)
{
	if (!state->hist.recorded)
	{
		if (state->rl.cursor > 0 && state->rl.buff.ctx)
			((char *)state->rl.buff.ctx)[state->rl.cursor - 1] = '\0';
		history_record(state, false);
	}
	state->hist.recorded = false;
	state->input_expanded = false;
	buff_readline_reset(&state->rl);
}

/* True when the command should be saved.
**
** This used to be "something was typed, and it differs from the previous
** entry" -- HISTCONTROL=ignoredups, hard-wired, whatever HISTCONTROL
** actually said. Two things were wrong with that. It ignored the user's
** configuration, so ignorespace, erasedups and HISTIGNORE did nothing at
** all. And it was not bash's default either: bash with HISTCONTROL unset
** keeps consecutive duplicates, so hellish silently dropped entries bash
** would have kept.
**
** It also compared the RAW typed text against the STORED entry, and the
** stored entry is the joined form (append_hist_entry joins before
** pushing). For a multi-line command those two are never equal -- `for i
** in 1 2` + `do echo $i` + `done` is stored as one `; `-joined line -- so
** the dedup could not fire for any multi-line command however many times
** it was repeated. The comparison is now against the joined candidate,
** which is what will actually be stored.
**
** A cursor of <= 1 means nothing was typed: the ring buffer only ever
** holds the trailing '\n' readline appends. */
bool	worthy_of_being_remembered(t_shell *state)
{
	char	*line;
	char	*prev;
	bool	keep;

	if (state->rl.cursor <= 1 || !state->hist.hist_active)
		return (false);
	line = hist_candidate(state);
	if (!line)
		return (false);
	keep = true;
	prev = hist_last(state);
	if (*line == ' ' && hist_control_has(state, "ignorespace"))
		keep = false;
	else if (prev && !ft_strcmp(prev, line)
		&& hist_control_has(state, "ignoredups"))
		keep = false;
	else if (hist_ignore_match(state, line))
		keep = false;
	return (xfree(line), keep);
}

/* First-time history setup: zero the struct, open the file, load and cap the
   entries, and leave append_fd open for incremental writes.

   The hist_cmds vector is made valid BEFORE parse_history_file runs, because
   every early return in there (no HOME, unopenable file) used to leave the
   zeroed struct as-is -- and a t_vec with elem_size 0 is a trap, not an empty
   vector: vec_push memcpys 0 bytes but still bumps len, so the first command
   grew a phantom entry and both the dedup check and the exit teardown then
   read 8-byte pointers out of a zero-stride buffer (issue #98, a SEGV the
   moment `exit` was typed under `env -i`). The success path replaces this
   empty vector wholesale (parse_hist_file builds its own), which is fine:
   nothing was allocated yet.

   readmark/appended start at what was just loaded: those entries are
   already both read and on disk, so a later `history -n` must not import
   them a second time and `history -a` must not re-append them. */
void	init_history(t_shell *state)
{
	state->hist = (t_history){.append_fd = -1, .hist_active = true};
	vec_init(&state->hist.hist_cmds);
	state->hist.hist_cmds.elem_size = sizeof(char *);
	parse_history_file(state);
	state->hist.readmark = state->hist.hist_cmds.len;
	state->hist.appended = state->hist.hist_cmds.len;
}

/* Release the heap strings in hist_cmds and their backing vector. The
   append_fd is not closed here; that is the caller's job at shell exit. */
void	free_hist(t_shell *state)
{
	size_t	i;

	i = -1;
	while (++i < state->hist.hist_cmds.len)
		xfree(((char **)state->hist.hist_cmds.ctx)[i]);
	xfree(state->hist.hist_cmds.ctx);
	vec_init(&state->hist.hist_cmds);
}
