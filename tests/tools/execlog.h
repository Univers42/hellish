/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execlog.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/07 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EXECLOG_H
# define EXECLOG_H

/*
** tests/tools/execlog -- log every exec a process tree performs.
**
** Built as a shared object and set in LD_PRELOAD, it interposes the exec
** family and posix_spawn (what GNU make uses) and appends one line per call
** to $EXECLOG before handing over to libc:
**
**   <pid>\t<caller>\t<path>\t<shebang interpreter or ->\t<argv, \x1f-joined>
**
** <caller> is the image doing the exec (/proc/self/exe at that moment: the
** shell or program that forked), so a `sh -c` can be charged to the script
** that ran it, and a third-party launcher that is itself a sh script (code,
** VBoxManage) told apart from born2root's own. The shebang column is read
** from the file, because the kernel, not the caller, decides which
** interpreter a `#!` script gets: a script started as ./x.sh shows up with
** `/bin/bash` or `/usr/bin/env hellish` beside it, whatever the caller
** thought it was running. That is what lets tests/born2root_shell_audit.sh
** say, for a whole `make` run, which shell interpreted each script.
**
** Static binaries and setuid programs (sudo) do not load it. Built by the
** audit script:  cc -shared -fPIC -O2 -o execlog.so tests/tools/execlog*.c
*/

# define _GNU_SOURCE
# include <dlfcn.h>
# include <fcntl.h>
# include <spawn.h>
# include <stdarg.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

extern char	**environ;

typedef int	(*t_execve)(const char *, char *const *, char *const *);
typedef int	(*t_spawn)(pid_t *, const char *,
		const posix_spawn_file_actions_t *, const posix_spawnattr_t *,
		char *const *, char *const *);

void		execlog_record(const char *path, char *const argv[]);
const char	*execlog_resolve(const char *file, char *buf, size_t n);
char		**execlog_collect(const char *arg0, va_list ap, char ***envp_out);
int			execvpe(const char *file, char *const argv[], char *const envp[]);

#endif
