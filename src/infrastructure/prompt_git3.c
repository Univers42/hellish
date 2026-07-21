/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_git3.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

/* TTL cache for the dirty check: one git subprocess at most every 3s of
   prompting, keyed on the repo root. */
typedef struct s_dcache
{
	char	root[PATH_MAX];
	time_t	at;
	int		dirty;
	int		init;
}	t_dcache;

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
		"--porcelain", "-uno", (char *)NULL);
	_exit(127);
}

/* Any output byte at all means the tree is dirty — we never need the rest,
   so read exactly one byte and reap. A missing git (exec fails, 0 bytes)
   degrades to "clean" rather than an error in the prompt. */
static int	run_git_check(const char *root)
{
	int		fd[2];
	pid_t	pid;
	ssize_t	n;
	char	c;

	if (pipe(fd) != 0)
		return (0);
	pid = fork();
	if (pid == 0)
		dirty_child(fd, root);
	close(fd[1]);
	n = 0;
	if (pid > 0)
		n = read(fd[0], &c, 1);
	close(fd[0]);
	while (pid > 0 && waitpid(pid, NULL, 0) < 0 && errno == EINTR)
		continue ;
	return (n > 0);
}

int	git_dirty_cached(const char *root)
{
	static t_dcache	c;
	time_t			now;

	now = time(NULL);
	if (c.init && now - c.at < 3 && ft_strcmp(c.root, root) == 0)
		return (c.dirty);
	c.init = 1;
	c.at = now;
	ft_strlcpy(c.root, root, sizeof(c.root));
	c.dirty = run_git_check(root);
	return (c.dirty);
}
