/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_metadata.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 16:35:57 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 04:57:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include <signal.h>
#include <sys/wait.h>

static char	*run_git_cmd(const char *cmd)
{
	int		pp[2];
	pid_t	pid;
	char	buf[256];
	ssize_t	n;

	if (pipe(pp))
		return (NULL);
	pid = fork();
	if (pid == 0)
	{
		close(pp[0]);
		dup2(pp[1], STDOUT_FILENO);
		close(pp[1]);
		dup2(open("/dev/null", 0), STDERR_FILENO);
		execl("/bin/sh", "sh", "-c", cmd, NULL);
		_exit(127);
	}
	close(pp[1]);
	if (pid < 0)
		return (close(pp[0]), NULL);
	n = read(pp[0], buf, sizeof(buf) - 1);
	close(pp[0]);
	waitpid(pid, NULL, 0);
	if (n <= 0)
		return (NULL);
	buf[n] = '\0';
	while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
		buf[--n] = '\0';
	if (n == 0)
		return (NULL);
	return (ft_strdup(buf));
}

void	get_git_info(char **branch, int *dirty)
{
	char	*res;

	*branch = NULL;
	*dirty = 0;
	res = run_git_cmd(
			"timeout 0.15 git rev-parse --abbrev-ref HEAD 2>/dev/null");
	if (!res)
		return ;
	*branch = res;
	res = run_git_cmd(
			"timeout 0.15 git diff --quiet HEAD 2>/dev/null; echo $?");
	if (res)
	{
		if (res[0] == '1')
			*dirty = 1;
		free(res);
	}
}

char	*get_venv_name(void)
{
	const char	*venv;
	const char	*conda;
	const char	*p;

	venv = getenv("VIRTUAL_ENV");
	conda = getenv("CONDA_DEFAULT_ENV");
	p = NULL;
	if (conda && *conda)
		return (ft_strdup(conda));
	if (venv && *venv)
	{
		p = ft_strrchr(venv, '/');
		if (p)
			return (ft_strdup(p + 1));
		return (ft_strdup(venv));
	}
	return (NULL);
}

void	get_timebuf(char *buf, size_t buflen)
{
	time_t		now;
	struct tm	tm;

	now = time(NULL);
	localtime_r(&now, &tm);
	strftime(buf, buflen, "%H:%M:%S", &tm);
}
