/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   run_utils.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 17:02:53 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 01:14:50 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sys.h"
#include "libft.h"

/* Our own script extensions, run under this shell rather than /bin/sh. */
static int	is_hellish_script(char *path, size_t len)
{
	if (len >= 8 && ft_strcmp(path + len - 8, ".hellish") == 0)
		return (1);
	if (len >= 5 && ft_strcmp(path + len - 5, ".hell") == 0)
		return (1);
	if (len >= 3 && ft_strcmp(path + len - 3, ".sh") == 0)
		return (1);
	return (0);
}

/* Pick an interpreter for a file that failed execve with ENOEXEC.  We
   recognise our own script extensions and run them under the running
   binary; everything else falls back to /bin/sh (FB_SH).  This mirrors
   what bash does for the "#!" shebang-less case.

   self_exe_path() rather than a literal "/proc/self/exe": that path is
   Linux-only, so on any other kernel this handed execve something that
   does not exist and the script simply failed.  When the kernel will not
   say where we are, /bin/sh is a better answer than a made-up path -- it
   is a POSIX shell, and these are POSIX scripts. */
static char	*select_fallback_shell(char *path)
{
	char	*self;

	self = self_exe_path();
	if (self && is_hellish_script(path, ft_strlen(path)))
		return (self);
	return (FB_SH);
}

/* If execve fails with ENOEXEC, retry with a fallback shell interpreter. */
static void	exec_fallback_shell(char *path, t_vec *args, char **envp)
{
	size_t	orig;
	size_t	ne;
	char	**nv;
	size_t	i;

	orig = args->len - 1;
	ne = orig + 1;
	nv = xmalloc(sizeof(char *) * (ne + 1));
	if (!nv)
		return ;
	nv[0] = select_fallback_shell(path);
	nv[1] = path;
	i = 0;
	while (++i < orig)
		nv[i + 1] = ((char **)(args->ctx))[i];
	nv[ne] = NULL;
	execve(nv[0], nv, envp);
	xfree(nv);
}

/* Attempt execve; on ENOEXEC (file is executable but not in a format the
   kernel can load -- e.g. a shell script without a shebang), retry with
   a shell interpreter prepended.  The NULL sentinel is pushed onto args
   so that the execve call receives a proper argv terminator.  On any
   other execve failure this function returns and actually_run maps the
   errno to an exit code. */
void	try_exec_with_fallback(char *path_of_exe,
							t_vec *args,
							char **envp)
{
	char	*null_ptr;

	null_ptr = NULL;
	vec_push(args, &null_ptr);
	execve(path_of_exe, (char **)(args->ctx), envp);
	if (errno == ENOEXEC)
		exec_fallback_shell(path_of_exe, args, envp);
}
