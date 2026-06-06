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
   the whole file mid-session (cap_history handles that at startup only). */
static void	append_hist_entry(t_shell *state, char *hist_entry)
{
	char	*enc;

	add_history_line(hist_entry);
	vec_push(&state->hist.hist_cmds, &hist_entry);
	if (state->hist.append_fd < 0)
		return ;
	enc = (char *)encode_cmd_hist(hist_entry).ctx;
	if (write_to_file(enc, state->hist.append_fd))
	{
		warning_error("Failed to write to the history file");
		close(state->hist.append_fd);
		state->hist.append_fd = -1;
	}
	xfree(enc);
}

/* Called after each command: if the command is worth saving, extract the raw
   text from the ring buffer (or from the history-expanded form if expansion
   ran), and append it. The expanded form is saved so "!! ; echo done" records
   the real command, not "!!" — less confusing to navigate later.
   Clears input_expanded and compacts the ring buffer unconditionally. */
void	manage_history(t_shell *state)
{
	char	*hist_entry;

	if (worthy_of_being_remembered(state))
	{
		if (state->rl.cursor > 0 && state->rl.buff.ctx)
			((char *)state->rl.buff.ctx)[state->rl.cursor - 1] = '\0';
		if (state->input_expanded && state->input.ctx)
			hist_entry = ft_strndup((char *)state->input.ctx,
					state->input.len);
		else
			hist_entry = ft_strndup((char *)state->rl.buff.ctx,
					state->rl.cursor - 1);
		append_hist_entry(state, hist_entry);
	}
	state->input_expanded = false;
	buff_readline_reset(&state->rl);
}

/* True when the command should be saved: at least one byte was typed, history
   is active, and the new entry differs from the most recent one (dedup, like
   HISTCONTROL=ignoredups). A cursor of <= 1 means nothing was typed (the ring
   buffer only ever has the trailing '\n' that readline appends). */
bool	worthy_of_being_remembered(t_shell *state)
{
	if (state->rl.cursor > 1 && state->hist.hist_active
		&& (!state->hist.hist_cmds.len
			|| !str_slice_eq_str((char *)state->rl.buff.ctx,
				state->rl.cursor - 1,
				((char **)state->hist.hist_cmds.ctx)
				[state->hist.hist_cmds.len - 1]
			)
		)
	)
		return (true);
	return (false);
}

/* First-time history setup: zero the struct, open the file, load and cap the
   entries, and leave append_fd open for incremental writes. */
void	init_history(t_shell *state)
{
	state->hist = (t_history){.append_fd = -1, .hist_active = true};
	parse_history_file(state);
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
