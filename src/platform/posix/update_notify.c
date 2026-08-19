/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_notify.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 20:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/19 20:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include "version.h"
#include <unistd.h>
#include <time.h>

/* Tell the user an update is waiting -- at most once per discovered
   version.

   WHERE this runs is the whole design. Issue #20 is emphatic that the
   notice must never damage what the user has typed, and in this shell
   readline runs in a forked child that owns the terminal for the duration
   of a line: anything written from the parent while that child is live
   lands in the middle of the user's input. There is no safe way to
   interrupt an in-progress line, so we do not try. The notice is emitted
   from the REPL between commands, before the prompt for the NEXT line is
   built -- the "show it when the prompt is redrawn" option the issue
   lists. At that instant the input buffer is empty by construction, the
   cursor is at column zero, and nothing can be clobbered.

   The cost is that a check finishing mid-line is announced one prompt
   later. That is the right trade: a late notice is a cosmetic delay, a
   corrupted command line is a wrong command executed. */
void	update_notify_prompt(t_shell *state)
{
	t_upd_state	s;

	if (state->metinp != INP_RL || !isatty(STDERR_FILENO))
		return ;
	if (getenv("HELLISH_NO_UPDATE_CHECK"))
		return ;
	if (!update_state_load(&s) || !update_available(&s))
		return ;
	if (s.notified > 0)
		return ;
	ft_eprintf("\n\033[33m\xe2\xac\x86\033[0m  update available: "
		"\033[2m%s\033[0m \xe2\x86\x92 \033[1m%s\033[0m\n",
		HELLISH_VERSION, s.latest);
	ft_eprintf("   \033[1;38;5;203m[Update]\033[0m run \033[1mupdate\033[0m"
		"    \033[2m[Later]\033[0m ignore this, nothing breaks\n\n");
	update_mark_notified();
}

/* Stop announcing the version we already know about. Called both after the
   notice is shown and after an install succeeds: once the new binary is on
   disk, the running process is still the old one, and re-announcing an
   update the user has just performed reads as if it had failed. */
void	update_mark_notified(void)
{
	t_upd_state	s;

	update_state_load(&s);
	s.notified = (long)time(NULL);
	update_state_save(&s);
}
