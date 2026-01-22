/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   run.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:12:09 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 15:12:15 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

// returns status
int actually_run(t_shell *state, t_vec *args)
{
	char *path_of_exe;
	char **envp;
	int status;

	ft_assert(args->len >= 1);

#if TRACE_DEBUG
	// debug
	ft_eprintf("%s: debug: actually_run argv[0]=%s argc=%lu\n",
			   state->context, ((char **)args->ctx)[0], (unsigned long)args->len);
#endif

	if (builtin_func(((char **)(args->ctx))[0]))
		exit(builtin_func(((char **)(args->ctx))[0])(state, *args));

	status = find_cmd_path(state, ((char **)(args->ctx))[0], &path_of_exe);
	if (status == COMMAND_NOT_FOUND)
		return 127;
	if (status == EXE_PERM_DENIED)
		return 126;
	if (status)
		return status;

	/* Ensure the child environment uses the executable path for _ (bash-like) */
	env_set(&state->env, (t_env){
							 .exported = true,
							 .key = ft_strdup("_"),
							 .value = ft_strdup(path_of_exe)});

#if TRACE_DEBUG
	ft_eprintf("%s: debug: exec path=%s\n", state->context, path_of_exe);
#endif
	char *null_ptr = NULL;
	vec_push(args, &null_ptr);
	envp = get_envp(state, path_of_exe);
	execve(path_of_exe, (char **)(args->ctx), envp);
	/* If execve failed with ENOEXEC (Exec format error), try /bin/sh file args... */
	if (errno == ENOEXEC)
	{
		size_t orig_elems;
		size_t new_elems;
		char **newargv;
		size_t i;

		orig_elems = args->len - 1; /* exclude final NULL sentinel pushed above */
		new_elems = orig_elems + 1; /* add "/bin/sh" at front */
		newargv = malloc(sizeof(char *) * (new_elems + 1));
		if (newargv)
		{
			newargv[0] = "/bin/sh";
			newargv[1] = path_of_exe;
			i = 1;
			while (i < orig_elems)
			{
				newargv[i + 1] = ((char **)(args->ctx))[i];
				i++;
			}
			newargv[new_elems] = NULL;
			execve("/bin/sh", newargv, envp);
			free(newargv);
		}
		/* if we fall through, the /bin/sh exec failed as well */
	}
	/* Preserve errno, optionally print permission error, then free allocations */
	{
		int _saved_errno = errno;
		if (_saved_errno == EACCES)
			ft_eprintf("%s: %s: %s\n", state->context, path_of_exe, strerror(_saved_errno));
		/* free argv strings and buffer (child only) */
		if (args->ctx)
		{
			size_t _i = 0;
			char **_arr = (char **)args->ctx;
			while (_i < args->len)
			{
				if (_arr[_i])
					free(_arr[_i]);
				_i++;
			}
			free(args->ctx);
		}
		free(path_of_exe);
		free_tab(envp);
		errno = _saved_errno;
	}
	// Match bash exit codes for execve errors
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
	if (errno == ENOENT || errno == ENOTDIR)
		return 127;
	if (errno == EACCES)
		return 126;
	if (errno == ENOEXEC)
		return 126;
	if (errno == ENOMEM)
		return 127;
	if (errno == EISDIR)
		return 126;
	return 1;
}
