/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_idle.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "zle.h"
#include <signal.h>

/* The line reader's surroundings: what it waits on besides the terminal,
   how long, what it does when that wait ends without a key, and the
   signals it keeps out of the gap between checking and waiting. */

/* Milliseconds to wait for a key before an idle tick, or -1: forever.
   Only a live prompt animation needs the tick. */
int	rl_idle_timeout(void)
{
	if (anim_armed())
		return (100);
	return (-1);
}

/* A descriptor to wake up for besides the terminal, or -1: the background
   git scan's pipe, while its answer could still be shown by a repaint
   (rl_repaint.c) -- a PS1 read in this process. */
int	rl_idle_fd(void)
{
	t_shell	*st;

	st = *zle_state_cell();
	if (!st || st->rl.use_fork || !st->rl.ps1_read)
		return (-1);
	return (git_scan_fd());
}

/* The wait ended without a key: r == 0 is the idle timeout, r > 0 the idle
   descriptor, r < 0 an interrupted wait (the caller looks at signals). A
   scan that finished with a new answer gets the prompt repainted.
     A resize is acted on from here too, and not only from
   rl_signal_event_hook: this is readline's OTHER wait, and while the
   animation is ticking it is the one the process sits in (rl_winch.c). */
void	rl_idle_event(int r)
{
	rl_winch_poll();
	if (r == 0)
		anim_tick();
	else if (r > 0 && git_scan_poll())
		rl_prompt_repaint(*zle_state_cell(), false);
}

/* Block the signals readline catches, keeping the previous mask in *old
   for the wait to restore. */
void	rl_block(sigset_t *old)
{
	sigset_t	set;

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGALRM);
	sigaddset(&set, SIGWINCH);
	sigaddset(&set, SIGTSTP);
	sigaddset(&set, SIGTTIN);
	sigaddset(&set, SIGTTOU);
	sigprocmask(SIG_BLOCK, &set, old);
}
