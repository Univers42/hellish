/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execlog_spawn.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/07 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
** posix_spawn and posix_spawnp take six parameters: that is libc's
** signature, and an interposer has no say in it. This file is therefore the
** one the 42 norm cannot pass (TOO_MANY_ARGS), and the norm target leaves
** it out by name. GNU make starts every recipe shell through posix_spawn,
** so without these two hooks the audit would not see what make ran.
*/

#include "execlog.h"

int	posix_spawn(pid_t *pid, const char *path,
	const posix_spawn_file_actions_t *fa, const posix_spawnattr_t *at,
	char *const argv[], char *const envp[])
{
	static t_spawn	real;

	if (!real)
		real = dlsym(RTLD_NEXT, "posix_spawn");
	execlog_record(path, argv);
	return (real(pid, path, fa, at, argv, envp));
}

int	posix_spawnp(pid_t *pid, const char *file,
	const posix_spawn_file_actions_t *fa, const posix_spawnattr_t *at,
	char *const argv[], char *const envp[])
{
	static t_spawn	real;
	char			buf[4096];

	if (!real)
		real = dlsym(RTLD_NEXT, "posix_spawnp");
	execlog_record(execlog_resolve(file, buf, sizeof(buf)), argv);
	return (real(pid, file, fa, at, argv, envp));
}
