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
   user's back. Porcelain v2 with --branch and --show-stash says in ONE run
   everything a prompt shows: the tracked bits, untracked and unmerged
   paths, ahead/behind and the stash count. --ignore-submodules=dirty is
   what zsh's vcs_info passes: without it git walks into every submodule
   to see whether its checkout is dirty, which cost 49 ms per scan in a
   ten-submodule repository against 6 ms with it -- a submodule whose
   recorded commit moved still shows. -uno skips the (possibly huge)
   untracked scan unless the prompt asked for it (git_untracked_cell).
   _exit, not exit: neither process may run the shell's cleanup paths. */
static void	dirty_child(int *fd, const char *root, int untracked)
{
	int			nul;
	const char	*u;

	if (fork() != 0)
		_exit(0);
	setpgid(0, 0);
	close(fd[0]);
	dup2(fd[1], STDOUT_FILENO);
	close(fd[1]);
	nul = open("/dev/null", O_WRONLY);
	if (nul >= 0)
		(dup2(nul, STDERR_FILENO), close(nul));
	u = "-uno";
	if (untracked)
		u = "-unormal";
	execlp("git", "git", "-C", root, "--no-optional-locks", "status",
		"--porcelain=v2", "--branch", "--show-stash",
		"--ignore-submodules=dirty", u, NULL);
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
		dirty_child(fd, c->root, c->untracked);
	close(fd[1]);
	if (mid < 0)
		return ((void)close(fd[0]));
	while (waitpid(mid, NULL, 0) < 0 && errno == EINTR)
		;
	c->busy = 1;
	c->fd = fd[0];
	ft_bzero(&c->acc, sizeof(c->acc));
	c->llen = 0;
	fcntl(c->fd, F_SETFL, O_NONBLOCK);
	fcntl(c->fd, F_SETFD, FD_CLOEXEC);
	c->spawned_ms = git_now_ms();
}

/* Harvest the scan, waiting up to wait_ms for it. The output is parsed as
   it arrives (gs_drain) and published once it ends, so a partial listing
   never reaches a render; EOF means exactly what it says, since the
   scanner sits in its own process group where the prompt's Ctrl-C cannot
   reach it. 1 when the scan was published. */
static int	poll_done(t_dcache *c, int wait_ms)
{
	struct pollfd	p;
	long long		end;
	int				left;

	end = git_now_ms() + wait_ms;
	left = wait_ms;
	while (c->busy)
	{
		p.fd = c->fd;
		p.events = POLLIN;
		if (poll(&p, 1, left) > 0 && gs_drain(c))
			return (git_scan_publish(c, wait_ms > 0), 1);
		left = (int)(end - git_now_ms());
		if (left <= 0)
			return (0);
	}
	return (0);
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
static int	scan_wait(t_dcache *c, const char *root)
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

/* The GIT_* bits for the repository rooted at `root`, never waiting longer
   than scan_wait allows. A change of root abandons any in-flight scan (its
   answer is for a repository we left). A generation change -- a command
   ran, so the tree may differ -- retires the cached answer whatever the
   TTL says; without that, the 30-second arm taken by a slow scan kept
   asserting "dirty" long after `git checkout` had made the tree clean. */
int	git_dirty_cached(const char *root)
{
	t_dcache	*c;
	int			wait_ms;

	c = git_dcache();
	if (c->busy && ft_strcmp(c->root, root) != 0)
		git_scan_drop(c);
	if (c->busy)
		return (poll_done(c, 0), c->cur.bits);
	if (c->init && ft_strcmp(c->root, root) == 0
		&& c->gen == *git_scan_gen()
		&& c->untracked == *git_untracked_cell()
		&& time(NULL) - c->at < c->ttl)
		return (c->cur.bits);
	wait_ms = scan_wait(c, root);
	spawn_check(c);
	if (c->busy)
		poll_done(c, wait_ms);
	return (c->cur.bits);
}
