/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_size.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 16:55:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 16:55:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "history_private.h"

/* Removing entries from a live history, for HISTCONTROL=erasedups and for
** the HISTSIZE cap.
**
** Both lists have to move together. hist_cmds is the truth and readline's
** own list is the mirror the arrow keys read, so dropping an entry from
** one and not the other makes `history` and ↑ disagree about what the
** session contains -- which is the failure the header comment on t_history
** warns about.
**
** The two marks move too. `appended` and `readmark` are INDICES into
** hist_cmds ("everything below this is already on disk / already read"),
** so removing an entry beneath one of them shifts what it points at. This
** is the bug behind `history -d 5` followed by `history -a` writing
** nothing: the deletion shrank the list without moving the mark, the mark
** then sat at or past the end, and the range [appended, len) that -a
** writes was empty. */

/* Drop entry i from both lists and fix the marks that sat above it. */
static void	hist_drop_at(t_shell *state, size_t i)
{
	char	**a;

	a = (char **)state->hist.hist_cmds.ctx;
	xfree(a[i]);
	ft_memmove(a + i, a + i + 1,
		(state->hist.hist_cmds.len - i - 1) * sizeof(char *));
	state->hist.hist_cmds.len--;
	hist_rl_drop((int)i);
	if (state->hist.appended > i)
		state->hist.appended--;
	if (state->hist.readmark > i)
		state->hist.readmark--;
}

/* HISTCONTROL=erasedups: the entry just pushed is the only copy that
   survives. Stops one short of the end so the new entry itself is kept. */
void	hist_erase_dups(t_shell *state, const char *line)
{
	size_t	i;
	char	**a;

	if (!line || !hist_control_has(state, "erasedups"))
		return ;
	i = 0;
	while (i + 1 < state->hist.hist_cmds.len)
	{
		a = (char **)state->hist.hist_cmds.ctx;
		if (!ft_strcmp(a[i], line))
			hist_drop_at(state, i);
		else
			i++;
	}
}

/* Keep the newest HISTSIZE entries. bash applies this as the list grows,
   not once at startup: a session that runs for a week with the old
   compile-time cap of 2000 grew without bound until the NEXT launch
   trimmed the file. A negative HISTSIZE means unlimited. */
void	hist_trim_to_limit(t_shell *state)
{
	long	lim;

	lim = hist_limit(state, "HISTSIZE", HIST_MAX);
	if (lim < 0)
		return ;
	while (state->hist.hist_cmds.len > (size_t)lim)
		hist_drop_at(state, 0);
}
