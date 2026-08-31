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
#include "libft.h"
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
/* _NSGetExecutablePath is declared here rather than by including
   <mach-o/dyld.h>, and that is not laziness.

   That header contains `enum DYLD_BOOL { FALSE, TRUE };`, and libft's
   ft_stddef.h already has `typedef enum e_bool { FALSE, TRUE } t_bool;`.
   Two enums cannot define the same enumerator names in one translation
   unit, so including both is a hard error whichever order they come in:

     dyld.h:153:20: error: redefinition of enumerator 'FALSE'
     ft_stddef.h:203:2: note: previous definition is here

   Neither header is ours to change and the collision is symmetric, so the
   only fix that does not spread is to not include the header. One stable,
   documented ABI symbol is a cheaper thing to declare than an enum
   namespace is to negotiate. */
#ifdef __APPLE__

int	_NSGetExecutablePath(char *buf, uint32_t *bufsize);

#endif

/* Deliberately here and not in a shared header: this file is the one place
   that is allowed to know the name, so that porting to a kernel without
   procfs is one edit rather than a search. tests/linux_only_apis_test.py
   enforces that. */
#define PROC_SELF_EXE "/proc/self/exe"

/* Comfortably over PATH_MAX on every target here (Darwin says 1024,
   Linux 4096), so the "buffer too small" arm of either lookup is
   unreachable rather than merely unlikely. */
#define SELF_EXE_MAX 4096

/* Where the running binary lives, asked of the kernel.

   This used to be the literal string "/proc/self/exe", handed straight to
   execve. That works on Linux and on nothing else: procfs is not in POSIX,
   and Darwin has no /proc at all -- so every <(cmd) and >(cmd) on macOS
   exec'd a path that does not exist, and exited 127 with an empty result.

   Darwin's answer is _NSGetExecutablePath, which writes into a caller
   buffer and takes the size BY POINTER: on success it is left alone, and
   on overflow it is rewritten with the size actually needed. Passing the
   size by value would compile and corrupt the stack, so it is a uint32_t
   local rather than a cast in the call.

   It also does NOT promise an absolute, resolved path -- it hands back
   the path the process was exec'd through, which may be relative and may
   be a symlink. Caching a relative one would be a bug with a long fuse:
   correct until the shell cd's somewhere else, and then exec'ing a
   sibling of the wrong directory. Apple's own documentation says to run
   realpath() on it, so that is what happens here. /proc/self/exe needs
   none of this -- the kernel already resolves it. */
#ifdef __APPLE__

static int	self_exe_fill(char *buf, size_t n)
{
	uint32_t	sz;
	char		raw[SELF_EXE_MAX];

	sz = (uint32_t)SELF_EXE_MAX;
	if (_NSGetExecutablePath(raw, &sz) != 0)
		return (0);
	raw[SELF_EXE_MAX - 1] = '\0';
	if (!realpath(raw, buf))
		ft_strlcpy(buf, raw, n);
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

/* Linux marks an unlinked image by APPENDING this to the readlink result.
   It is a human annotation in a field that otherwise holds a path, and it
   is not quoted or escaped in any way. */
#define DELETED_SUFFIX " (deleted)"

/* Undo that annotation, and record that we saw it.
**
** Every in-place upgrade trips this. `install`(1) -- which is what
** `make my_shell`, `sudo install` and the updater's own elevated path all
** use -- UNLINKS the destination and creates a new inode, precisely so the
** running process keeps executing the old one. From that instant the kernel
** answers /proc/self/exe with
**
**     /usr/bin/hellish (deleted)
**
** and every caller that treats the result as a path is holding a filename
** with an English phrase stapled to the end of it. Two things broke, both
** silently:
**
**   * process substitution re-execs this path, so `cat <(echo hi)` printed
**     NOTHING -- not an error, just an empty result -- for the rest of the
**     session after any upgrade;
**   * the updater installed to a literal file called "hellish (deleted)"
**     next to the real one, leaving the binary it meant to replace
**     untouched while reporting success.
**
** The suffix is only stripped when the annotated path does NOT exist on
** disk. A file may legitimately be named "x (deleted)", and if such a file
** is really there then readlink was reporting its name, not commenting on
** it -- so the check distinguishes the two cases instead of assuming.
*/
static int	self_exe_undelete(char *buf)
{
	size_t	len;
	size_t	slen;

	len = ft_strlen(buf);
	slen = ft_strlen(DELETED_SUFFIX);
	if (len <= slen || ft_strcmp(buf + len - slen, DELETED_SUFFIX) != 0)
		return (0);
	if (access(buf, F_OK) == 0)
		return (0);
	buf[len - slen] = '\0';
	return (1);
}

/* One cell for the lookup's outcome, so the path and the "was it replaced"
   answer can never disagree: 0 not resolved, 1 resolved, 2 resolved and the
   image had been unlinked, -1 the kernel would not say. A function-local
   static rather than a file-scope one, the same way the glob and zle cells
   are done -- the house rule is no NEW globals, and this stays inside. */
int	*self_exe_state(void)
{
	static int	state;

	return (&state);
}

/* The path, resolved once and cached, or NULL if the kernel will not say.

   Cached because it cannot change: a process keeps the image it was
   exec'd from for its whole life. Callers must treat the result as
   read-only and must not free it.

   NULL is a real answer, not an oversight -- a kernel with no procfs and
   no Darwin lookup has nothing to tell us, and a caller that execs this
   is better off failing on a NULL check than on a path it invented. */
char	*self_exe_path(void)
{
	static char	buf[SELF_EXE_MAX];
	int			*state;

	state = self_exe_state();
	if (*state == 0)
	{
		if (self_exe_fill(buf, sizeof(buf)))
			*state = 1;
		else
			*state = -1;
		if (*state == 1 && self_exe_undelete(buf))
			*state = 2;
	}
	if (*state == 1 || *state == 2)
		return (buf);
	return (NULL);
}
