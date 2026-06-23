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

/* If AI is on, interactive, and the cached tip is stale (>5 min) or missing,
   fork a fully detached worker to refresh it. Non-blocking: the parent reaps
   the short-lived middle child; the orphaned grandchild does the slow LLM call,
   writes the cache, and exits. ponytail: the worker is silent (no writes to
   std fds), so no /dev/null redirect is needed. */
/* Fully detached worker: the orphaned grandchild does the slow LLM call and
   writes the cache; the parent reaps the short-lived middle child and returns.
   ponytail: silent (no writes to std fds), so no /dev/null redirect needed. */
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
	tip = ai_chat(state, "From the shell context above, give ONE short tip "
			"(<=120 chars) relevant to what the user is doing now. "
			"Output only the tip.");
	if (tip)
		tip_write(path, tip);
	_exit(0);
}

/* If AI is on, interactive, the cached tip is stale (>5 min) or missing, and a
   backend is actually reachable, refresh it in the background. Non-blocking. */
void	ai_tip_spawn(t_shell *state)
{
	char		path[512];
	struct stat	st;

	if (!state->opt_ai || state->metinp != INP_RL)
		return ;
	if (!tip_path(path, sizeof(path)))
		return ;
	if (stat(path, &st) == 0 && time(NULL) - st.st_mtime <= 240)
		return ;
	if (!getenv("HELLISH_AI_URL") && ai_reachable() != 0)
		return ;
	tip_worker(state, path);
}
