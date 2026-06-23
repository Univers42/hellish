/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_ghost_ai.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_ghost_ai.h"
#include "ai.h"
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

/* The single AI-ghost state (one per readline child). fd starts at -1. */
t_aig	*aig(void)
{
	static t_aig	s;
	static int		init;

	if (!init)
	{
		s.fd = -1;
		init = 1;
	}
	return (&s);
}

/* Cancel any in-flight worker and forget the pending suggestion. */
void	aig_reset(t_aig *a)
{
	if (a->pid > 0)
	{
		kill(a->pid, SIGKILL);
		waitpid(a->pid, NULL, 0);
		a->pid = 0;
	}
	if (a->fd >= 0)
	{
		close(a->fd);
		a->fd = -1;
	}
	a->sug[0] = '\0';
	a->fired = 0;
}

/* Fork a worker that computes an AI completion for `line` and writes it to a
   pipe we read non-blocking, so typing is never blocked. */
void	aig_fire(t_aig *a, const char *line)
{
	int		p[2];
	char	*sug;

	if (pipe(p))
		return ;
	a->pid = fork();
	if (a->pid < 0)
	{
		close(p[0]);
		close(p[1]);
		return ;
	}
	if (a->pid == 0)
	{
		close(p[0]);
		sug = ai_complete_line(line);
		if (sug && write(p[1], sug, ft_strlen(sug)) < 0)
			_exit(1);
		_exit(0);
	}
	close(p[1]);
	fcntl(p[0], F_SETFL, O_NONBLOCK);
	a->fd = p[0];
	a->fired = 1;
}

/* Non-blocking check for the worker's result; store it (newline-trimmed) when
   it lands. EAGAIN (-1) just means "not ready yet". */
void	aig_poll(t_aig *a)
{
	char	buf[1024];
	ssize_t	r;

	if (a->fd < 0)
		return ;
	r = read(a->fd, buf, sizeof(buf) - 1);
	if (r < 0)
		return ;
	close(a->fd);
	a->fd = -1;
	waitpid(a->pid, NULL, 0);
	a->pid = 0;
	while (r > 0 && (buf[r - 1] == '\n' || buf[r - 1] == ' '))
		r--;
	buf[r] = '\0';
	if (r > 0)
		ft_strlcpy(a->sug, buf, sizeof(a->sug));
}
