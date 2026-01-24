/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   run_utils.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 17:02:53 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 19:25:41 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sys.h"

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
		return (EXIT_CMD_NOT_FOUND);
	if (status == EXE_PERM_DENIED)
		return (EXIT_CMD_NOT_EXEC);
	return (status);
}

/**
 * PURPOSE: handle the case when we try to execute
 * a file file that is not a binary, but a script without 
 * a shebang line. 
 * When we call execve(path_of_exe, ...) and the file is 
 * not a valid executable, execve fails with errno == ENOEXEC.
 * in this case the shell should try to run the script path as an
 * argument.
 * 
 * POSIX shell behavio r require to do this. if a file is not
 * a boinary and has no shebang, try to run it as a shell script
 * 
 * This makes it compatible with scripts that do not have a shebang line.
 * IMPLEMENTATION:
 * - first we try to execve(path_of_exe, args, envp)
 * - if it fails with ENOEXEC, we build a new argv array
 *  with /bin/sh as first argument, path_of_exe as second,
 * and the rest of args shifted by one.
 * - then we call execve("/bin/sh", new_argv, envp)
 * - if this fails, we just return (the caller should handle the error)
 * 
 */
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
			newargv[0] = FB_SH;
			newargv[1] = path_of_exe;
			i = 0;
			while (++i < orig_elems)
				newargv[i + 1] = ((char **)(args->ctx))[i];
			newargv[new_elems] = NULL;
			(execve(FB_SH, newargv, envp), free(newargv));
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

/**
 * This map gatger a common `errno values`to the exit
 * codes  used by POSIX shells when a commands fails to execute.
 * This is important for shell comptability and scripting.
 * as shells like Bash use specific exit codes to indecate
 * why a command failed.
 * 
 * here's what each mapped errno means:
 * - EACCES (Permission denied): exit code 126
 * - ENOENT (No such file or directory): exit code 127
 * - ENOEXEC (Exec format error): exit code 126
 * - ENOTDIR (Not a directory): exit code 127
 * - ENOMEM (Out of memory): exit code 127
 * - EISDIR (Is a directory): exit code 126
 * - Other errors: exit code 1 (general error)
 * 
 * Those ensure that out shell behaves like Bash and other POSIX
 * shell regarding exit codes
 * 
 */
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
