/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_git6.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* Milliseconds on the monotonic clock, for timing scans. */
long long	git_now_ms(void)
{
	struct timespec	ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* Let go of the in-flight scan. Closing the read end is the whole of it:
   the scanner is init's child, not ours, so there is nothing to wait for.
   An abandoned scan (a cd to another repo) finishes on its own, and dies
   on SIGPIPE the moment it writes to the pipe we just closed. */
void	git_scan_drop(t_dcache *c)
{
	close(c->fd);
	c->busy = 0;
}

/* A scan is over: make what it said the answer.
**
** The TTL is 3 seconds, or 30 when the scan took a second or more, so a
** slow repository is not rescanned near continuously while nothing
** happens (a command in the tree retires the answer either way).
** `timed` says the scan was harvested inside a bounded wait, so its
** duration is real and decides whether the next command's rescan is worth
** waiting for (git_dirty_cached). A scan harvested at a later render only
** proves it finished by then, so it leaves that figure alone. */
void	git_scan_publish(t_dcache *c, int timed)
{
	long long	ms;

	git_scan_drop(c);
	if (c->llen > 0)
		gs_feed(c, "\n", 1);
	c->cur = c->acc;
	if (c->cur.bits & (GIT_STAGED | GIT_UNSTAGED))
		c->cur.bits |= GIT_DIRTY;
	ms = git_now_ms() - c->spawned_ms;
	c->at = time(NULL);
	c->ttl = 3;
	if (ms >= 1000)
		c->ttl = 30;
	if (timed)
		c->last_ms = (int)ms;
}

/* How long the render about to start a scan may wait for it, and the
** bookkeeping that goes with starting one.
**
** A freshly entered root gets up to GIT_WAIT_NEW_MS: fast repositories
** keep an exact first answer, and the last one is forgotten, since it
** described a repository the shell has left. After a command that may
** have touched the tree -- or a change of untracked mode -- the rescan is
** waited for up to GIT_WAIT_TOUCHED_MS, but only when the last timed scan
** of this repository fit in that budget: `git commit` in a normal repo is
** followed by an exact prompt, and a slow repo never makes the prompt
** wait, its answer arriving a render late instead. A TTL refresh -- the
** answer merely aged while nothing ran -- does not wait at all. */
int	git_scan_wait(t_dcache *c, const char *root)
{
	int	wait_ms;

	wait_ms = 0;
	if (!c->init || ft_strcmp(c->root, root) != 0)
	{
		wait_ms = GIT_WAIT_NEW_MS;
		ft_bzero(&c->cur, sizeof(c->cur));
		c->last_ms = -1;
	}
	else if ((c->gen != *git_scan_gen()
			|| c->untracked != *git_untracked_cell())
		&& c->last_ms >= 0 && c->last_ms <= GIT_WAIT_TOUCHED_MS)
		wait_ms = GIT_WAIT_TOUCHED_MS;
	c->init = 1;
	c->gen = *git_scan_gen();
	c->untracked = *git_untracked_cell();
	ft_strlcpy(c->root, root, sizeof(c->root));
	return (wait_ms);
}

/* The in-flight scan's pipe, for the line reader to wait on, or -1. */
int	git_scan_fd(void)
{
	t_dcache	*c;

	c = git_dcache();
	if (!c->busy)
		return (-1);
	return (c->fd);
}
