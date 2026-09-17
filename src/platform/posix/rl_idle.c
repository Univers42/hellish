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

/* A descriptor to wake up for besides the terminal, or -1. */
int	rl_idle_fd(void)
{
	return (-1);
}

/* The wait ended without a key: r == 0 is the idle timeout, r > 0 the idle
   descriptor, r < 0 an interrupted wait (the caller looks at signals). */
void	rl_idle_event(int r)
{
	if (r == 0)
		anim_tick();
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
