/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execlog_execl.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/07 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execlog.h"

int	execlp(const char *file, const char *arg0, ...)
{
	va_list	ap;
	char	**argv;

	va_start(ap, arg0);
	argv = execlog_collect(arg0, ap, NULL);
	va_end(ap);
	return (execvpe(file, argv, environ));
}

int	execle(const char *path, const char *arg0, ...)
{
	va_list	ap;
	char	**argv;
	char	**envp;

	va_start(ap, arg0);
	argv = execlog_collect(arg0, ap, &envp);
	va_end(ap);
	return (execve(path, argv, envp));
}
