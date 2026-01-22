/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   run_utils.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 17:02:53 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 17:10:37 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* helper: if builtin -> run and exit; return 0 if not builti
 (never returns on builtin) */
int	run_builtin_or_continue(t_shell *state, t_vec *args)
{
	int	(*bf)(t_shell *, t_vec);

	if (!args->len || !args->ctx)
		return (0);
	bf = builtin_func(((char **)args->ctx)[0]);
	if (bf)
		exit(bf(state, *args));
	return (0);
}

/* helper: find executable path and map special returns */
int	find_exe_path_wrapper(t_shell *state, char *cmd0, char **out_path)
{
	int	status;

	status = find_cmd_path(state, cmd0, out_path);
	if (status == COMMAND_NOT_FOUND)
		return (127);
	if (status == EXE_PERM_DENIED)
		return (126);
	return (status);
}

/* try exec, handle ENOEXEC fallback to /bin/sh */
void	try_exec_with_fallback(char *path_of_exe,
							t_vec *args,
							char **envp)
{
	char	*null_ptr;
	size_t	orig_elems;
	size_t	new_elems;
	char	**newargv;
	size_t	i;

	null_ptr = NULL;
	vec_push(args, &null_ptr);
	execve(path_of_exe, (char **)(args->ctx), envp);
	if (errno == ENOEXEC)
	{
		orig_elems = args->len - 1;
		new_elems = orig_elems + 1;
		newargv = malloc(sizeof(char *) * (new_elems + 1));
		if (newargv)
		{
			newargv[0] = "/bin/sh";
			newargv[1] = path_of_exe;
			i = 0;
			while (++i < orig_elems)
				newargv[i + 1] = ((char **)(args->ctx))[i];
			newargv[new_elems] = NULL;
			(execve("/bin/sh", newargv, envp), free(newargv));
		}
	}
}

/* cleanup after execve failure (child only) */
void	cleanup_after_exec_failure(t_vec *args,
			char *path_of_exe,
			char **envp)
{
	size_t	i;
	char	**arr;

	if (args->ctx)
	{
		i = 0;
		arr = (char **)args->ctx;
		while (i < args->len)
		{
			if (arr[i])
				free(arr[i]);
			i++;
		}
		free(args->ctx);
	}
	free(path_of_exe);
	free_tab(envp);
}

/* map errno to bash-like exit codes */
int	map_errno_to_exit(void)
{
	if (errno == EACCES)
		return (126);
	if (errno == ENOENT)
		return (127);
	if (errno == ENOEXEC)
		return (126);
	if (errno == ENOTDIR)
		return (127);
	if (errno == ENOMEM)
		return (127);
	if (errno == EISDIR)
		return (126);
	return (1);
}
