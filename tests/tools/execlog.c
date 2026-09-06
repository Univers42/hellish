/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execlog.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/07 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execlog.h"

int	execve(const char *path, char *const argv[], char *const envp[])
{
	static t_execve	real;

	if (!real)
		real = dlsym(RTLD_NEXT, "execve");
	execlog_record(path, argv);
	return (real(path, argv, envp));
}

int	execv(const char *path, char *const argv[])
{
	return (execve(path, argv, environ));
}

int	execvpe(const char *file, char *const argv[], char *const envp[])
{
	static t_execve	real;
	char			buf[4096];

	if (!real)
		real = dlsym(RTLD_NEXT, "execvpe");
	execlog_record(execlog_resolve(file, buf, sizeof(buf)), argv);
	return (real(file, argv, envp));
}

int	execvp(const char *file, char *const argv[])
{
	return (execvpe(file, argv, environ));
}

int	execl(const char *path, const char *arg0, ...)
{
	va_list	ap;
	char	**argv;

	va_start(ap, arg0);
	argv = execlog_collect(arg0, ap, NULL);
	va_end(ap);
	return (execve(path, argv, environ));
}
