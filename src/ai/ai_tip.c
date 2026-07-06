/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_tip.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"
#include "sh_input.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>

/* Cache path: $HOME/.cache/hellish/protip. 0 if $HOME is unset. */
static int	tip_path(char *buf, size_t n)
{
	char	*home;

	home = getenv("HOME");
	if (!home)
		return (0);
	ft_strlcpy(buf, home, n);
	ft_strlcat(buf, "/.cache/hellish/protip", n);
	return (1);
}

/* Read the cached tip (first line only, no newline). xfree it; NULL if none.
   One small read per prompt -- interactive-only, never on a benchmark path. */
char	*ai_tip_read(void)
{
	char	path[512];
	char	buf[512];
	int		fd;
	ssize_t	r;
	int		i;

	if (!tip_path(path, sizeof(path)))
		return (NULL);
	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (NULL);
	r = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (r <= 0)
		return (NULL);
	buf[r] = '\0';
	i = 0;
	while (buf[i] && buf[i] != '\n')
		i++;
	return (ft_substr(buf, 0, i));
}

/* Create ~/.cache/hellish, then atomically write the tip (tmp + rename). */
static void	tip_write(const char *path, const char *tip)
{
	char	*home;
	char	dir[512];
	char	tmp[512];
	int		fd;

	home = getenv("HOME");
	if (!home)
		return ;
	ft_strlcpy(dir, home, sizeof(dir));
	ft_strlcat(dir, "/.cache", sizeof(dir));
	mkdir(dir, 0755);
	ft_strlcat(dir, "/hellish", sizeof(dir));
	mkdir(dir, 0755);
	ft_strlcpy(tmp, path, sizeof(tmp));
	ft_strlcat(tmp, ".tmp", sizeof(tmp));
	fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		return ;
	if (write(fd, tip, ft_strlen(tip)) < 0)
	{
		close(fd);
		return ;
	}
	close(fd);
	rename(tmp, path);
}

/* Fully detached worker: the orphaned grandchild does the slow LLM call and
   writes the cache; the parent reaps the short-lived middle child and returns.
   The grandchild skips the refresh when the machine is already loaded (an
   inference burst on a busy box is what makes commands lag) and caps its own
   request budget at 8s -- llama-server aborts generation when the client
   disconnects, so the timeout bounds the CPU burst too. ponytail: silent (no
   writes to std fds), so no /dev/null redirect needed. */
static void	tip_worker(t_shell *state, const char *path)
{
	pid_t	pid;
	char	*tip;

	pid = fork();
	if (pid < 0)
		return ;
	if (pid > 0)
		return ((void)waitpid(pid, NULL, 0));
	if (fork() > 0)
		_exit(0);
	setsid();
	if (ai_load_high())
		_exit(0);
	ai_sync_env(state);
	setenv("HELLISH_AI_TIMEOUT_MS", "8000", 1);
	tip = ai_request("From the shell context above, give ONE short tip "
			"(<=120 chars) relevant to what the user is doing now. "
			"Output only the tip.", 0);
	if (tip)
		tip_write(path, tip);
	_exit(0);
}

/* If AI is on, interactive, and the cached tip is stale (>10 min) or missing,
   refresh it in a detached worker. NEVER probes the network in the parent: the
   old ai_reachable() connect stalled the prompt for seconds when the local
   server was up but busy (SO_*TIMEO does not bound connect()). The worker's
   ai_request handles an absent backend, and a 30s re-spawn throttle keeps a
   slow or down backend from piling up overlapping workers. The long cache
   window also keeps CPU-inference bursts rare. Non-blocking. */
void	ai_tip_spawn(t_shell *state)
{
	static time_t	last;
	char			path[512];
	struct stat		st;
	time_t			now;

	if (!state->opt_ai || state->metinp != INP_RL)
		return ;
	if (!tip_path(path, sizeof(path)))
		return ;
	now = time(NULL);
	if (stat(path, &st) == 0 && now - st.st_mtime <= 600)
		return ;
	if (now - last < 30)
		return ;
	last = now;
	tip_worker(state, path);
}
