/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   self_exe.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "sys.h"
#include <unistd.h>
#include <stdint.h>
#ifdef __APPLE__
# include <mach-o/dyld.h>
#endif

/* Deliberately here and not in a shared header: this file is the one place
   that is allowed to know the name, so that porting to a kernel without
   procfs is one edit rather than a search. tests/linux_only_apis_test.py
   enforces that. */
#define PROC_SELF_EXE "/proc/self/exe"

/* Where the running binary lives, asked of the kernel.

   This used to be the literal string "/proc/self/exe", handed straight to
   execve. That works on Linux and on nothing else: procfs is not in POSIX,
   and Darwin has no /proc at all -- so every <(cmd) and >(cmd) on macOS
   exec'd a path that does not exist, and exited 127 with an empty result.

   Darwin's answer is _NSGetExecutablePath, which writes into a caller
   buffer and takes the size BY POINTER: on success it is left alone, and
   on overflow it is rewritten with the size actually needed. Passing the
   size by value would compile and corrupt the stack, so it is a uint32_t
   local rather than a cast in the call. */
#ifdef __APPLE__

static int	self_exe_fill(char *buf, size_t n)
{
	uint32_t	sz;

	sz = (uint32_t)n;
	if (_NSGetExecutablePath(buf, &sz) != 0)
		return (0);
	buf[n - 1] = '\0';
	return (1);
}

#else

static int	self_exe_fill(char *buf, size_t n)
{
	ssize_t	r;

	r = readlink(PROC_SELF_EXE, buf, n - 1);
	if (r <= 0)
		return (0);
	buf[r] = '\0';
	return (1);
}

#endif

/* The path, resolved once and cached, or NULL if the kernel will not say.

   Cached because it cannot change: a process keeps the image it was
   exec'd from for its whole life. Callers must treat the result as
   read-only and must not free it.

   NULL is a real answer, not an oversight -- a kernel with no procfs and
   no Darwin lookup has nothing to tell us, and a caller that execs this
   is better off failing on a NULL check than on a path it invented. */
char	*self_exe_path(void)
{
	static char	buf[4096];
	static int	state;

	if (state == 0)
	{
		if (self_exe_fill(buf, sizeof(buf)))
			state = 1;
		else
			state = -1;
	}
	if (state == 1)
		return (buf);
	return (NULL);
}
