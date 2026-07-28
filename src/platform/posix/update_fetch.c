/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_fetch.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include "version.h"
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

/* In the forked child: redirect curl's output to the pipe and exec it. */
static void	run_pipe_child(int *fd, char *const argv[])
{
	dup2(fd[1], STDOUT_FILENO);
	close(fd[0]);
	close(fd[1]);
	execvp(argv[0], argv);
	_exit(127);
}

/* Run argv, capturing its stdout into out (NUL-terminated). Bytes read, or -1.
   Used to shell out to curl without dragging in a TLS library. */
static ssize_t	capture_cmd(char *const argv[], char *out, size_t n)
{
	int		fd[2];
	pid_t	pid;
	size_t	total;
	ssize_t	r;

	if (pipe(fd) != 0)
		return (-1);
	pid = fork();
	if (pid < 0)
		return (close(fd[0]) + close(fd[1]) - 1);
	if (pid == 0)
		run_pipe_child(fd, argv);
	close(fd[1]);
	total = 0;
	while (total + 1 < n)
	{
		r = read(fd[0], out + total, n - 1 - total);
		if (r <= 0)
			break ;
		total += (size_t)r;
	}
	close(fd[0]);
	waitpid(pid, NULL, 0);
	out[total] = '\0';
	return ((ssize_t)total);
}

/* Pull the "tag_name" string out of the GitHub JSON, dropping a leading 'v'. */
static int	parse_tag(const char *buf, char *out, size_t n)
{
	const char	*p;
	size_t		i;

	p = ft_strstr(buf, "\"tag_name\"");
	if (!p)
		return (0);
	p += 10;
	while (*p && *p != '"')
		p++;
	if (*p != '"')
		return (0);
	p++;
	if (*p == 'v' || *p == 'V')
		p++;
	i = 0;
	while (p[i] && p[i] != '"' && i < n - 1)
	{
		out[i] = p[i];
		i++;
	}
	out[i] = '\0';
	return (i > 0);
}

/* Ask GitHub for the latest release tag (curl, ~4s timeout). 1 on success. */
int	fetch_latest_tag(char *out, size_t n)
{
	char		buf[8192];
	char *const	argv[] = {"curl", "-fsSL", "--max-time", "4",
		"https://api.github.com/repos/" HELLISH_REPO "/releases/latest", NULL};

	if (capture_cmd(argv, buf, sizeof(buf)) <= 0)
		return (0);
	return (parse_tag(buf, out, n));
}

/* Detach from the terminal, fetch the latest tag, and cache it. Runs in the
   background grandchild, so the prompt is never delayed by the network. */
void	run_bg_update_check(void)
{
	char	tag[64];
	int		nul;

	setsid();
	nul = open("/dev/null", O_RDWR);
	if (nul >= 0)
	{
		dup2(nul, STDOUT_FILENO);
		dup2(nul, STDERR_FILENO);
		if (nul > 2)
			close(nul);
	}
	if (fetch_latest_tag(tag, sizeof(tag)))
		hellish_write_cache(tag);
}
