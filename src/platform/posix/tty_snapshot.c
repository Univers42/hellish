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

/* Re-take the snapshot at the top of an interactive turn.
**
** Without this the shell only ever knows the settings it started with, and
** putting THOSE back would undo a deliberate `stty -echo` -- which bash
** does not do, and which would be its own bug.  Taken at the prompt, the
** snapshot is by definition "what the user last chose", because anything a
** command did to the terminal happened after the previous prompt and the
** user is looking at a prompt now.
*/
void	tty_snapshot_refresh(void)
{
	tty_snapshot_save();
}

/* Put the shell's terminal settings back after a foreground job was KILLED
** BY A SIGNAL.
**
** A program that reads a password (chsh, ssh, sudo) turns echo off and
** turns it back on when it finishes.  Interrupt it and it never reaches
** the second half, so the terminal is left with no echo -- and the shell
** is the only thing still running that can undo it.  Reported as #85:
** "the TTY is disactivated and crash, we can no longer see the input".
**
** Only on a signal, and that condition is the whole design.  Restoring
** after EVERY command would undo `stty -echo` typed as a command, which is
** a deliberate request the shell has no business reversing.  Measured
** against bash 5.3.9, which draws the line in exactly that place:
**
**     stty -echo                       -> echo stays off   (both shells)
**     <program that disables echo> ^C  -> echo restored     (bash only)
*/
void	tty_reclaim_after_signal(void)
{
	if (!isatty(STDIN_FILENO))
		return ;
	tty_snapshot_restore();
}
