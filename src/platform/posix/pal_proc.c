/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pal_proc.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/28 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "pal.h"

/* POSIX process shims: pure passthroughs to the kernel.  The t_shell
   handle exists for the win32 siblings, which must translate pids
   through the pal_procs handle registry; here the kernel already does
   that job, so st is deliberately unused throughout. */
int	pal_waitpid(t_shell *st, int pid, int *status, int options)
{
	(void)st;
	return (waitpid(pid, status, options));
}

int	pal_wait_any(t_shell *st, int *status, int options)
{
	(void)st;
	return (waitpid(-1, status, options));
}

int	pal_kill(t_shell *st, int pid, int sig)
{
	(void)st;
	return (kill(pid, sig));
}

int	pal_kill_pgid(t_shell *st, int pgid, int sig)
{
	(void)st;
	return (kill(-pgid, sig));
}

int	pal_pipe(int fds[2])
{
	return (pipe(fds));
}
