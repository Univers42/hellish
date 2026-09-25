/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_winch2.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 04:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 04:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

/* Act on a recorded resize, if any -- see rl_winch.c for why readline's
** own path cannot be used.
**
** An int with no argument so it can be rl_signal_event_hook directly.
**
** Width only. A tab switch raises SIGWINCH with the size unchanged and a
** zoom often changes only the row count, and neither moves anything on the
** line: redrawing then is precisely what stacked the copies.
**
** When the width DID change: clear the rows readline laid the line out on
** (rl_clear_visible_line knows how many), clear below as well because a
** narrowing terminal reflows the right prompt onto the next row, re-measure,
** and let readline lay it out again. The rows ABOVE the input -- a two-row
** prompt's top row -- are the terminal's to reflow; the next prompt is
** drawn at the new width anyway. */
int	rl_winch_poll(void)
{
	t_winch	*st;
	int		cols;

	st = rl_winch_cell();
	if (!st->winch)
		return (0);
	st->winch = 0;
	cols = rl_winch_cols();
	if (cols == st->cols)
		return (0);
	st->cols = cols;
	rl_clear_visible_line();
	rl_out_write("\033[J", 3);
	rl_reset_screen_size();
	rl_forced_update_display();
	return (0);
}
