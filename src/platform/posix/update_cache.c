/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_cache.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

/* Build "$HOME/.cache/hellish/latest" into buf. Returns 1 on success. */
int	hellish_cache_path(char *buf, size_t n)
{
	char	*home;

	home = getenv("HOME");
	if (!home || !*home || ft_strlen(home) + 24 >= n)
		return (0);
	ft_strlcpy(buf, home, n);
	ft_strlcat(buf, "/.cache/hellish/latest", n);
	return (1);
}

/* Load the tag the last background check cached, trimming trailing space. */
int	read_cached_latest(char *out, size_t n)
{
	char	path[512];
	int		fd;
	ssize_t	r;

	if (!hellish_cache_path(path, sizeof(path)))
		return (0);
	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (0);
	r = read(fd, out, n - 1);
	close(fd);
	if (r <= 0)
		return (0);
	out[r] = '\0';
	while (r > 0 && (out[r - 1] == '\n' || out[r - 1] == '\r'
			|| out[r - 1] == ' '))
		out[--r] = '\0';
	return (out[0] != '\0');
}

/* Persist the latest tag, creating ~/.cache/hellish first. 1 on success. */
int	hellish_write_cache(const char *tag)
{
	char	path[512];
	char	*home;
	int		fd;
	int		ret;

	home = getenv("HOME");
	if (!home || ft_strlen(home) + 24 >= sizeof(path))
		return (0);
	ft_strlcpy(path, home, sizeof(path));
	ft_strlcat(path, "/.cache", sizeof(path));
	mkdir(path, 0755);
	ft_strlcat(path, "/hellish", sizeof(path));
	mkdir(path, 0755);
	ft_strlcat(path, "/latest", sizeof(path));
	fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd < 0)
		return (0);
	ret = (write(fd, tag, ft_strlen(tag)) > 0);
	close(fd);
	return (ret);
}

/* True if the cache exists and is younger than a day (so we skip re-checking).
   A missing cache is "stale" so the first interactive run kicks off a check. */
static int	cache_is_fresh(void)
{
	char		path[512];
	struct stat	st;

	if (!hellish_cache_path(path, sizeof(path)))
		return (1);
	if (stat(path, &st) != 0)
		return (0);
	return (time(NULL) - st.st_mtime <= 86400);
}

/* Interactive only: if the cached tag is stale, fork a fully detached child
   (double-fork) to refresh it in the background. Returns at once; the prompt
   is never delayed by the network. Opt out with HELLISH_NO_UPDATE_CHECK. */
void	maybe_spawn_update_check(t_shell *state)
{
	pid_t	pid;

	if (state->metinp != INP_RL || !isatty(STDOUT_FILENO))
		return ;
	if (getenv("HELLISH_NO_UPDATE_CHECK") || cache_is_fresh())
		return ;
	pid = fork();
	if (pid != 0)
	{
		if (pid > 0)
			waitpid(pid, NULL, 0);
		return ;
	}
	if (fork() != 0)
		_exit(0);
	run_bg_update_check();
	_exit(0);
}
