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
#include <signal.h>

/* Async dirty check. The prompt must NEVER wait for git: on a huge repo
   `git status` stats every tracked file and can take seconds, and the old
   blocking read froze the first prompt after every cd (and every TTL
   refresh) for the whole scan. Now a freshly entered repo gets one small
   bounded wait (fast repos keep their exact star); past that the child
   keeps running and a later render harvests it from the pipe — the star
   arrives a render late instead of the prompt arriving seconds late. */

/* Child body: stdout into the pipe, stderr silenced, then git status.
   --no-optional-locks keeps git from touching .git/index behind the
   user's back; -uno skips the (possibly huge) untracked scan, so the
   star means "tracked changes exist". _exit, not exit: this child must
   not run the shell's atexit/cleanup paths. */
static void	dirty_child(int *fd, const char *root)
{
	int	nul;

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

/* Fork the checker for c->root. The read end is non-blocking (harvest
   polls must never stall a render) and cloexec (it outlives this call, so
   command children forked while the check runs must not inherit it). */
static void	spawn_check(t_dcache *c)
{
	int	fd[2];

	c->pid = 0;
	if (pipe(fd) != 0)
		return ;
	c->pid = fork();
	if (c->pid == 0)
		dirty_child(fd, c->root);
	close(fd[1]);
	if (c->pid < 0)
	{
		c->pid = 0;
		close(fd[0]);
		return ;
	}
	c->fd = fd[0];
	fcntl(c->fd, F_SETFL, O_NONBLOCK);
	fcntl(c->fd, F_SETFD, FD_CLOEXEC);
	c->spawned = time(NULL);
}

/* Retire the in-flight child (SIGKILL first when abandoning it — e.g. a
   cd to another repo — so the reap cannot block on a live scan). Returns
   the raw wait status, or -1 when someone else already reaped it: the
   background-job WNOHANG loop does waitpid(-1) and may win the race. */
static int	drop_check(t_dcache *c, int killit)
{
	int	st;
	int	r;

	if (killit)
		kill(c->pid, SIGKILL);
	close(c->fd);
	r = waitpid(c->pid, &st, 0);
	while (r < 0 && errno == EINTR)
		r = waitpid(c->pid, &st, 0);
	c->pid = 0;
	if (r < 0)
		return (-1);
	return (st);
}

/* Harvest attempt with a wait budget. Any output byte means dirty; EOF
   with none means clean. A child that died on a signal (Ctrl-C at the
   prompt reaches it — same process group) proved nothing: drop the
   sample and let the stale c->at trigger a fresh spawn next render.
   Slow repos (scan >= 1s) stretch the TTL so git is not re-run near
   continuously; the checks are async either way. */
static int	poll_done(t_dcache *c, int wait_ms)
{
	struct pollfd	p;
	ssize_t			n;
	int				st;
	char			ch;

	p.fd = c->fd;
	p.events = POLLIN;
	if (poll(&p, 1, wait_ms) <= 0)
		return (0);
	n = read(c->fd, &ch, 1);
	if (n < 0)
		return (0);
	st = drop_check(c, 0);
	if (st != -1 && WIFSIGNALED(st))
		return (1);
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

	if (c.pid > 0 && ft_strcmp(c.root, root) != 0)
		drop_check(&c, 1);
	if (c.pid > 0)
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
	if (c.pid > 0)
		poll_done(&c, wait_ms);
	return (c.dirty);
}
