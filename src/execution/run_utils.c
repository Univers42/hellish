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

static char	*select_fallback_shell(char *path)
{
	size_t	len;

	len = ft_strlen(path);
	if (len >= 8 && ft_strcmp(path + len - 8, ".hellish") == 0)
		return (PATH_HELLISH);
	if (len >= 5 && ft_strcmp(path + len - 5, ".hell") == 0)
		return (PATH_HELLISH);
	if (len >= 3 && ft_strcmp(path + len - 3, ".sh") == 0)
		return (PATH_HELLISH);
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
