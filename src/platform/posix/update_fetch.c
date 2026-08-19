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
#include <time.h>

/* In the forked child: stdout to the pipe, stderr to /dev/null, then exec.
   curl's own diagnostics are silenced because we report failures ourselves
   -- a raw "curl: (22) ... 404" leaking into an interactive session is
   noise the user can do nothing with, and it used to be the only visible
   symptom of a misconfigured release URL. */
static void	run_pipe_child(int *fd, char *const argv[])
{
	int	nul;

	dup2(fd[1], STDOUT_FILENO);
	close(fd[0]);
	close(fd[1]);
	nul = open("/dev/null", O_WRONLY);
	if (nul >= 0)
	{
		dup2(nul, STDERR_FILENO);
		if (nul > 2)
			close(nul);
	}
	execvp(argv[0], argv);
	_exit(127);
}

/* Run argv, capturing its stdout into out (NUL-terminated). Bytes read, or
   -1. Lets the updater shell out to curl and sha256sum without dragging a
   TLS or hashing library into the shell. */
ssize_t	update_capture(char *const argv[], char *out, size_t n)
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

/* Pull "tag_name" out of the release metadata, dropping a leading 'v'. */
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

/* Ask the release endpoint for the latest tag. 1 = got one, 0 = could not
   reach the endpoint, -1 = it answered but published no release. Those are
   kept apart because "you are offline" and "that repository has no
   releases" need different fixes, and conflating them is what made a dead
   URL look like a network outage for the whole life of this feature. */
int	fetch_latest_tag(char *out, size_t n)
{
	char		url[640];
	char		buf[65536];
	char *const	argv[] = {"curl", "-fsSL", "--proto", "=https,http",
		"--max-time", "8", url, NULL};

	update_api_url(url, sizeof(url));
	if (update_capture(argv, buf, sizeof(buf)) <= 0)
		return (0);
	if (!parse_tag(buf, out, n))
		return (-1);
	return (1);
}

/* Detach from the terminal, refresh the latest tag, and persist it. Runs in
   the background grandchild, so the prompt is never delayed by the network. */
void	run_bg_update_check(void)
{
	t_upd_state	s;
	char		tag[64];
	int			nul;

	setsid();
	nul = open("/dev/null", O_RDWR);
	if (nul >= 0)
	{
		dup2(nul, STDOUT_FILENO);
		dup2(nul, STDERR_FILENO);
		if (nul > 2)
			close(nul);
	}
	update_state_load(&s);
	if (fetch_latest_tag(tag, sizeof(tag)) != 1)
		return ;
	if (ft_strcmp(s.latest, tag) != 0)
		s.notified = 0;
	ft_strlcpy(s.latest, tag, sizeof(s.latest));
	s.checked = (long)time(NULL);
	update_state_save(&s);
}
