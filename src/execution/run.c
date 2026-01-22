/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   run.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:12:09 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 16:52:59 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* helper: if builtin -> run and exit; return 0 if not builtin (never returns on builtin) */
static int run_builtin_or_continue(t_shell *state, t_vec *args)
{
	int (*bf)(t_shell *, t_vec);

	if (!args->len || !args->ctx)
		return (0);
	bf = builtin_func(((char **)args->ctx)[0]);
	if (bf)
		exit(bf(state, *args));
	return (0);
}

/* helper: find executable path and map special returns */
static int find_exe_path_wrapper(t_shell *state, char *cmd0, char **out_path)
{
	int status;

	status = find_cmd_path(state, cmd0, out_path);
	if (status == COMMAND_NOT_FOUND)
		return (127);
	if (status == EXE_PERM_DENIED)
		return (126);
	return status;
}

/* try exec, handle ENOEXEC fallback to /bin/sh */
static void try_exec_with_fallback(char *path_of_exe, t_vec *args, char **envp)
{
	char *null_ptr = NULL;

	vec_push(args, &null_ptr);
	execve(path_of_exe, (char **)(args->ctx), envp);
	if (errno == ENOEXEC)
	{
		size_t orig_elems = args->len - 1;
		size_t new_elems = orig_elems + 1;
		char **newargv = malloc(sizeof(char *) * (new_elems + 1));
		if (newargv)
		{
			newargv[0] = "/bin/sh";
			newargv[1] = path_of_exe;
			for (size_t i = 1; i < orig_elems; ++i)
				newargv[i + 1] = ((char **)(args->ctx))[i];
			newargv[new_elems] = NULL;
			execve("/bin/sh", newargv, envp);
			free(newargv);
		}
	}
}

/* cleanup after execve failure (child only) */
static void cleanup_after_exec_failure(t_vec *args, char *path_of_exe, char **envp)
{
	if (args->ctx)
	{
		size_t i = 0;
		char **arr = (char **)args->ctx;
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
static int map_errno_to_exit(void)
{
	if (errno == EACCES)
		return 126;
	if (errno == ENOENT)
		return 127;
	if (errno == ENOEXEC)
		return 126;
	if (errno == ENOTDIR)
		return 127;
	if (errno == ENOMEM)
		return 127;
	if (errno == EISDIR)
		return 126;
	return 1;
}

/* returns status */
int actually_run(t_shell *state, t_vec *args)
{
	char *path_of_exe = NULL;
	char **envp;
	int status;

	ft_assert(args->len >= 1);

#if TRACE_DEBUG
	ft_eprintf("%s: debug: actually_run argv[0]=%s argc=%lu\n",
			   state->context, ((char **)args->ctx)[0], (unsigned long)args->len);
#endif

	/* builtin handling (exits inside) */
	run_builtin_or_continue(state, args);

	/* find executable path */
	status = find_exe_path_wrapper(state, ((char **)(args->ctx))[0], &path_of_exe);
	if (status != 0)
		return status;

	/* set '_' in environment for child */
	env_set(&state->env, (t_env){
							 .exported = true,
							 .key = ft_strdup("_"),
							 .value = ft_strdup(path_of_exe)});

#if TRACE_DEBUG
	ft_eprintf("%s: debug: exec path=%s\n", state->context, path_of_exe);
#endif

	envp = get_envp(state, path_of_exe);
	try_exec_with_fallback(path_of_exe, args, envp);

	/* exec failed: preserve errno, optionally print permission error, cleanup and map code */
	{
		int _saved_errno = errno;
		if (_saved_errno == EACCES)
			ft_eprintf("%s: %s: %s\n", state->context, path_of_exe, strerror(_saved_errno));
		cleanup_after_exec_failure(args, path_of_exe, envp);
		errno = _saved_errno;
	}
	return map_errno_to_exit();
}
