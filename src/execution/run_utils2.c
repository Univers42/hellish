/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   run_utils2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 17:02:53 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 01:14:50 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sys.h"
#include "libft.h"

/* helper: if builtin -> run and exit; return 0 if not builtin */
int	run_builtin_or_continue(t_shell *state, t_vec *args)
{
	int				(*bf)(t_shell *, t_vec);
	t_shell_func	*fn;

	if (!args->len || !args->ctx)
		return (0);
	fn = func_lookup(state, ((char **)args->ctx)[0]);
	if (fn)
		exit(execute_func_call(state, fn, args).status);
	bf = builtin_func(((char **)args->ctx)[0]);
	if (bf)
		exit(bf(state, *args));
	return (0);
}

/* helper: find executable path and map special return codes */
int	find_exe_path_wrapper(t_shell *state, char *cmd0, char **out_path)
{
	int	status;

	status = find_cmd_path(state, cmd0, out_path);
	if (status == COMMAND_NOT_FOUND)
		return (EXIT_CMD_NOT_FOUND);
	if (status == EXE_PERM_DENIED)
		return (EXIT_CMD_NOT_EXEC);
	return (status);
}

/* cleanup after execve failure (child only). argv elements are simple-command
   words, which may be slab-allocated (word_strndup), so they must be released
   through word_free()/slab_free() — never xfree() — exactly as the parent does
   in free_executable_cmd(). The vec backing (args->ctx) is plain fn_* heap. */
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
				word_free(arr[i]);
			i++;
		}
		xfree(args->ctx);
	}
	xfree(path_of_exe);
	free_tab(envp);
}

/* Map errno to POSIX shell exit codes after execve failure. */
int	map_errno_to_exit(void)
{
	if (errno == EACCES)
		return (PERMISSION_DENIED);
	if (errno == ENOENT)
		return (NO_SUCH_FILE_OR_DIR);
	if (errno == ENOEXEC)
		return (EXIT_CMD_NOT_EXEC);
	if (errno == ENOTDIR)
		return (NO_SUCH_DIR);
	if (errno == ENOMEM)
		return (OUT_OF_MEM);
	if (errno == EISDIR)
		return (IS_A_DIR);
	return (EXIT_GENERAL_ERR);
}
