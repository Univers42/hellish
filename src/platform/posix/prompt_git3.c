/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_git3.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/28 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <poll.h>

/* Async dirty check. The prompt must NEVER wait for git: on a huge repo
   `git status` stats every tracked file and can take seconds, and the old
   blocking read froze the first prompt after every cd (and every TTL
   refresh) for the whole scan. Now a freshly entered repo gets one small
   bounded wait (fast repos keep their exact star); past that the scan
   keeps running and a later render harvests it from the pipe — the star
   arrives a render late instead of the prompt arriving seconds late.

   The scanner is deliberately NOT our child. We double-fork and reap the
   throwaway middle process immediately, so `git status` is orphaned onto
   init. A direct child would have to be reaped by someone, and the only
   code that ever looked at it was the next prompt render — so any git
   that finished while a foreground command was running sat as a zombie
   for the whole duration of that command. Starting a nested shell parked
   a visible `git <defunct>` in ps for as long as the inner shell lived
   (issue #24). Orphaning removes the reaping obligation entirely.

   The scanner also puts itself in its own process group, so Ctrl-C at the
   prompt no longer reaches it. That used to kill it mid-scan, which is
   why the harvest had to special-case "died on a signal, sample proves
   nothing". Out of the shell's process group there is no such case, and
   EOF on the pipe means exactly what it says. */

/* Child body, entered by the throwaway middle process. It forks once more
   and leaves at once: the parent reaps the middle process immediately and
   the scanner below is reparented to init, so nothing here can ever become
   a zombie of the shell. setpgid takes the scanner out of the terminal's
   foreground group so Ctrl-C at the prompt cannot kill it mid-scan.

   Then: stdout into the pipe, stderr silenced, exec git status.
   --no-optional-locks keeps git from touching .git/index behind the
   user's back; -uno skips the (possibly huge) untracked scan, so the
   star means "tracked changes exist". _exit, not exit: neither process
   here may run the shell's atexit/cleanup paths. */
static void	dirty_child(int *fd, const char *root)
{
	int	nul;

	if (fork() != 0)
		_exit(0);
	setpgid(0, 0);
	close(fd[0]);
	dup2(fd[1], STDOUT_FILENO);
	close(fd[1]);
	nul = open("/dev/null", O_WRONLY);
	if (nul >= 0)
	{
		dup2(nul, STDERR_FILENO);
		close(nul);
	}
	execlp("git", "git", "-C", root, "--no-optional-locks", "status",
		"--porcelain", "-uno", NULL);
	_exit(127);
}

/* Start the scanner for c->root. `mid` is the throwaway middle process of
   the double fork: it forks the real scanner and exits immediately, so the
   waitpid here costs nothing and leaves us with no child to reap later.
   The read end is non-blocking (harvest polls must never stall a render)
   and cloexec (it outlives this call, so command children forked while the
   scan runs must not inherit it). */
static void	spawn_check(t_dcache *c)
{
	int		fd[2];
	pid_t	mid;

	c->busy = 0;
	if (pipe(fd) != 0)
		return ;
	mid = fork();
	if (mid == 0)
		dirty_child(fd, c->root);
	close(fd[1]);
	if (mid < 0)
	{
		close(fd[0]);
		return ;
	}
	while (waitpid(mid, NULL, 0) < 0 && errno == EINTR)
		;
	c->busy = 1;
	c->fd = fd[0];
	fcntl(c->fd, F_SETFL, O_NONBLOCK);
	fcntl(c->fd, F_SETFD, FD_CLOEXEC);
	c->spawned = time(NULL);
}

/* Let go of the in-flight scan. Closing the read end is the whole of it:
   the scanner is init's child, not ours, so there is nothing to wait for.
   Abandoning one (a cd to another repo) no longer needs a SIGKILL either —
   that kill only existed so the reap could not block on a live scan. The
   orphan finishes on its own, and dies on SIGPIPE the moment it writes to
   the pipe we just closed. */
static void	drop_check(t_dcache *c)
{
	close(c->fd);
	c->busy = 0;
}

/* Harvest attempt with a wait budget. Any output byte means dirty; EOF
   with none means clean — and now that the scanner sits in its own process
   group, EOF really does mean the scan ran to completion rather than "the
   prompt's Ctrl-C killed it and this sample proves nothing".
   Slow repos (scan >= 1s) stretch the TTL so git is not re-run near
   continuously; the checks are async either way. */
static int	poll_done(t_dcache *c, int wait_ms)
{
	struct pollfd	p;
	ssize_t			n;
	char			ch;

	p.fd = c->fd;
	p.events = POLLIN;
	if (poll(&p, 1, wait_ms) <= 0)
		return (0);
	n = read(c->fd, &ch, 1);
	if (n < 0)
		return (0);
	drop_check(c);
	c->dirty = (n > 0);
	c->at = time(NULL);
	c->ttl = 3;
	if (c->at - c->spawned >= 1)
		c->ttl = 30;
	return (1);
}

/* Non-blocking dirty flag for the repo rooted at `root`. A change of root
   abandons any in-flight check (its answer is for a repo we left). Only a
   freshly entered root gets a bounded wait — TTL refreshes poll with a
   zero budget, so a render never stalls once the shell is inside a repo. */
int	git_dirty_cached(const char *root)
{
	static t_dcache	c;
	int				wait_ms;

	if (c.busy && ft_strcmp(c.root, root) != 0)
		drop_check(&c);
	if (c.busy)
		return (poll_done(&c, 0), c.dirty);
	if (c.init && ft_strcmp(c.root, root) == 0
		&& time(NULL) - c.at < c.ttl)
		return (c.dirty);
	wait_ms = 0;
	if (ft_strcmp(c.root, root) != 0)
	{
		wait_ms = 60;
		c.dirty = 0;
	}
	c.init = 1;
	ft_strlcpy(c.root, root, sizeof(c.root));
	spawn_check(&c);
	if (c.busy)
		poll_done(&c, wait_ms);
	return (c.dirty);
}
