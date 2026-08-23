/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tty_snapshot.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/23 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/23 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include <termios.h>
#include <unistd.h>

/* The terminal settings this shell found when it started, and whether we
   actually got them. Static rather than a t_shell field so shell.h does not
   have to drag <termios.h> into every translation unit that includes it. */
static struct termios	g_tty;
static int				g_tty_ok;

/* Take the snapshot, once, at interactive startup.

   A full-screen program puts the terminal into raw mode and puts it back
   when it exits. A BACKGROUND one never gets the chance: `top &` sets raw
   mode, then touches the tty from a background process group, and the
   kernel stops it mid-way -- raw mode still in force. Nothing in that job
   will ever run again unless someone foregrounds it.

   So the shell has to be the one that remembers. Without this, leaving a
   shell in that state handed the parent a terminal with no echo and no
   line discipline: keystrokes vanished, and typed commands came back
   spliced into garbage (issue #58). The stopped-jobs guard is what stops
   you getting here by accident; this is what makes it recoverable when you
   ask to leave anyway. */
void	tty_snapshot_save(void)
{
	if (!isatty(STDIN_FILENO))
		return ;
	g_tty_ok = (tcgetattr(STDIN_FILENO, &g_tty) == 0);
}

/* Put it back on the way out. TCSANOW rather than TCSADRAIN: a drain waits
   for queued output, and a terminal a stopped job left in raw mode is
   exactly where that can block. Nothing is reported on failure -- the shell
   is exiting, the fd may already be gone, and there is no one left to tell. */
void	tty_snapshot_restore(void)
{
	if (!g_tty_ok)
		return ;
	tcsetattr(STDIN_FILENO, TCSANOW, &g_tty);
}
