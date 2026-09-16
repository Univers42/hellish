/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_file.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 17:40:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 17:40:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "history_private.h"

/* Move the session to the history file the configuration asked for.
**
** init_history runs from cli_dispatch, inside on(), which is BEFORE
** ~/.hellishrc is sourced -- so an rc that sets HISTFILE (the ordinary
** place to set it, and where bash users expect it to work) arrives after
** the default file has already been opened and read. An exported HISTFILE
** was fine because the environment exists by then; an assigned one was
** not, which is the confusing half: the same line works in .bashrc and
** silently did nothing here.
**
** So the shell re-homes once, right after the rc has run. If the wanted
** path is the one already open, this is a strcmp and nothing else. If it
** differs, what was loaded belongs to a file this session is not using:
** it is dropped from both lists and the real file is loaded in its place,
** so `history` shows the history of the file the next command will be
** appended to. That is bash's arrangement, which reads its history after
** the startup files precisely so HISTFILE can be set in one.
**
** Mid-session assignment is deliberately NOT tracked, matching bash: the
** value is consulted when the history is loaded and when the builtin is
** asked to write, not on every command. */
void	hist_rehome(t_shell *state)
{
	char	*want;

	if (!state->hist.hist_active || !state->hist.file)
		return ;
	want = get_hist_file_path(state);
	if (!want)
		return ;
	if (!ft_strcmp(want, state->hist.file))
		return (xfree(want));
	xfree(want);
	if (state->hist.append_fd >= 0)
		close(state->hist.append_fd);
	state->hist.append_fd = -1;
	free_hist(state);
	clear_history();
	parse_history_file(state);
	state->hist.readmark = state->hist.hist_cmds.len;
	state->hist.appended = state->hist.hist_cmds.len;
}
