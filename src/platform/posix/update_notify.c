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
#include <sys/stat.h>

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
   corrupted command line is a wrong command executed.

   It is also cheap, which it had to be made rather than declared: this
   runs once per REPL cycle, and an unconditional open+read+parse of the
   state file there is work on the path between every command and the next
   prompt, to re-read a file a daily background check writes.

   The throttle is the state file's own mtime, NOT a clock. A time-based
   one was tried first and was wrong in the one case that matters: the
   session that DISCOVERS a release reads the file before its background
   check has written it, so a 5-second hold meant the discovering session
   announced nothing at all -- update_freshness_test.py's "the discovering
   session says so, unprompted", which fails under the suite and passes
   standalone, because whether the check lands inside the hold is pure
   timing. Keyed on mtime, a write is seen on the very next prompt and an
   unchanged file costs one stat instead of an open, a read and a parse.
   No state file at all means nothing has ever been checked, so there is
   nothing to announce and the stat is the whole cost. */
static long	upd_state_stamp(void)
{
	char		path[512];
	struct stat	st;

	if (!update_cache_file("state", path, sizeof(path)))
		return (0);
	if (stat(path, &st) != 0)
		return (0);
	return ((long)st.st_mtim.tv_sec * 1000000000L + st.st_mtim.tv_nsec);
}

void	update_notify_prompt(t_shell *state)
{
	t_upd_state	s;
	long		stamp;

	if (state->metinp != INP_RL || state->upd_notified)
		return ;
	if (getenv("HELLISH_NO_UPDATE_CHECK") || !isatty(STDERR_FILENO))
		return ;
	stamp = upd_state_stamp();
	if (stamp == 0 || stamp == state->upd_state_stamp)
		return ;
	state->upd_state_stamp = stamp;
	if (!update_state_load(&s) || !update_available(&s))
		return ;
	if (s.notified > 0)
		return ((void)(state->upd_notified = true));
	ft_eprintf("\n\033[33m\xe2\xac\x86\033[0m  update available: "
		"\033[2m%s\033[0m \xe2\x86\x92 \033[1m%s\033[0m\n",
		HELLISH_VERSION, s.latest);
	ft_eprintf("   \033[1;38;5;203m[Update]\033[0m run \033[1mupdate\033[0m"
		"    \033[2m[Later]\033[0m ignore this, nothing breaks\n\n");
	update_mark_notified();
	state->upd_notified = true;
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
